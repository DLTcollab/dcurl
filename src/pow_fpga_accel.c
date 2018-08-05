/*MIT License

Copyright (c) 2018 Ievgen Korokyi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#include "pow_fpga_accel.h"


#define NONCE_LEN 81
#define TRANSACTION_LEN 2673
#define NONCE_OFFSET 2646
#define HPS_TO_FPGA_BASE 0xC0000000
#define HPS_TO_FPGA_SPAN 0x0020000
#define HASH_CNT_REG_OFFSET 4
#define TICK_CNT_LOW_REG_OFFSET 5
#define TICK_CNT_HI_REG_OFFSET 6
#define MWM_MASK_REG_OFFSET 3
#define CPOW_BASE 0

static FILE *ctrl_fd;
static FILE *in_fd;
static FILE *out_fd;
static int devmem_fd;
static void *fpga_regs_map;
static uint32_t *cpow_map;
static int result;
static int8_t otrytes[TRANSACTION_LEN];

int pow_fpga_accel_init()
{
    ctrl_fd = 0;
    in_fd = 0;
    out_fd = 0;
    devmem_fd = 0;
    fpga_regs_map = 0;
    cpow_map = 0;

    ctrl_fd = fopen("/dev/cpow-ctrl", "r+");

    if (ctrl_fd == NULL) {
        perror("cpow-ctrl open fail");
        return 0;
    }

    in_fd = fopen("/dev/cpow-idata", "wb");

    if (in_fd == NULL) {
        perror("cpow-idata open fail");
        fclose(ctrl_fd);
        return 0;
    }

    out_fd = fopen("/dev/cpow-odata", "rb");

    if (out_fd == NULL) {
        perror("cpow-odata open fail");
        fclose(ctrl_fd);
        fclose(in_fd);
        return 0;
    }

    devmem_fd = open("/dev/mem", O_RDWR | O_SYNC);

    if (devmem_fd < 0) {
        perror("devmem open");
        fclose(ctrl_fd);
        fclose(in_fd);
        fclose(out_fd);
        return 0;
    }

    fpga_regs_map =
        (uint32_t *) mmap(NULL, HPS_TO_FPGA_SPAN, PROT_READ | PROT_WRITE,
                          MAP_SHARED, devmem_fd, HPS_TO_FPGA_BASE);
    cpow_map = (uint32_t *) (fpga_regs_map + CPOW_BASE);

    if (fpga_regs_map == MAP_FAILED) {
        perror("devmem mmap");
        close(devmem_fd);
        fclose(ctrl_fd);
        fclose(in_fd);
        fclose(out_fd);
        return 0;
    }

    return 1;
}

void pow_fpga_accel_destroy()
{
    fclose(in_fd);
    fclose(out_fd);
    fclose(ctrl_fd);

    result = munmap(fpga_regs_map, HPS_TO_FPGA_SPAN);

    close(devmem_fd);

    if (result < 0) {
        perror("devmem munmap");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

int8_t *PowFPGAAccel(int8_t *itrytes, int mwm, int index)
{
    int8_t fpga_out_nonce_trits[NONCE_LEN];

    size_t itrytelen = 0;
    size_t itritlen = 0;

    itrytelen = strnlen((char *) itrytes, TRANSACTION_LEN);
    itritlen = 3 * itrytelen;

    Trytes_t *object_trytes = initTrytes(itrytes, itrytelen);

    Trits_t *object_trits = trits_from_trytes(object_trytes);

    fwrite((char *) object_trits->data, 1, itritlen, in_fd);
    fflush(in_fd);

    fwrite(&mwm, 1, 1, ctrl_fd);
    fread(&result, sizeof(result), 1, ctrl_fd);
    fflush(ctrl_fd);

    fread((char *) fpga_out_nonce_trits, 1, NONCE_LEN, out_fd);

    Trits_t *object_nonce_trits = initTrits(fpga_out_nonce_trits, NONCE_LEN);
    Trytes_t *nonce_trytes = trytes_from_trits(object_nonce_trits);

    for (int i = 0; i < TRANSACTION_LEN; i = i + 1)
        if (i < NONCE_OFFSET)
            otrytes[i] = itrytes[i];
        else
            otrytes[i] = nonce_trytes->data[i - NONCE_OFFSET];

    freeTrobject(object_trytes);
    freeTrobject(object_trits);
    freeTrobject(object_nonce_trits);
    freeTrobject(nonce_trytes);

    return otrytes;
}
