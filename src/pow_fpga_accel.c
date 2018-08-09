/* 
 * Copyright (C) 2018 dcurl Developers. 
 * Copyright (c) 2018 Ievgen Korokyi. 
 * Use of this source code is governed by MIT license that can be 
 * found in the LICENSE file.
 */

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "trinary.h"
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
    int result;
    
    fclose(in_fd);
    fclose(out_fd);
    fclose(ctrl_fd);

    result = munmap(fpga_regs_map, HPS_TO_FPGA_SPAN);

    close(devmem_fd);

    if (result < 0) {
        perror("devmem munmap");
    }
}

int8_t *PowFPGAAccel(int8_t *itrytes, int mwm, int index)
{
    int8_t fpga_out_nonce_trits[NONCE_LEN];
    int8_t *otrytes = (int8_t *)malloc(sizeof(int8_t) * TRANSACTION_LEN);

    size_t itrytelen = 0;
    size_t itritlen = 0;

    int result;

    itrytelen = strnlen((char *) itrytes, TRANSACTION_LEN);
    itritlen = 3 * itrytelen;

    Trytes_t *object_trytes = initTrytes(itrytes, itrytelen);
    if (!object_trytes)
        return NULL;

    Trits_t *object_trits = trits_from_trytes(object_trytes);
    if (!object_trits)
        return NULL;

    fwrite((char *) object_trits->data, 1, itritlen, in_fd);
    fflush(in_fd);

    fwrite(&mwm, 1, 1, ctrl_fd);
    fread(&result, sizeof(result), 1, ctrl_fd);
    fflush(ctrl_fd);

    fread((char *) fpga_out_nonce_trits, 1, NONCE_LEN, out_fd);

    Trits_t *object_nonce_trits = initTrits(fpga_out_nonce_trits, NONCE_LEN);
    if (!object_nonce_trits)
        return NULL;    

    Trytes_t *nonce_trytes = trytes_from_trits(object_nonce_trits);
    if (!nonce_trytes)
        return NULL; 

    for (int i = 0; i < TRANSACTION_LEN; i++)
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
