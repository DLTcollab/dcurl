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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <error.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "converter.h"

#define NONCE_LEN                   81
#define TRANSACTION_LEN             2673
#define NONCE_OFFSET                2646
#define HPS_TO_FPGA_BASE            0xC0000000
#define HPS_TO_FPGA_SPAN            0x0020000
#define HASH_CNT_REG_OFFSET         4
#define TICK_CNT_LOW_REG_OFFSET     5
#define TICK_CNT_HI_REG_OFFSET      6
#define MWM_MASK_REG_OFFSET         3
#define CPOW_BASE                   0

#define HINTS                                                                  \
  "### CPOW Hardware Accelerated ###\nUsage:\n\t./curl_pow_hard MWM TRYTES(length: %d) \n"

int main(int argc, char* argv[]) 
{
    FILE *ctrl_fd = 0;
    FILE *in_fd = 0;
    FILE *out_fd = 0;

    uint8_t mwm = 0;

    int devmem_fd = 0;

    void *fpga_regs_map = 0;
    uint32_t *cpow_map = 0;

    uint32_t hash_cnt = 0;
    uint32_t tick_cnt_l = 0;
    uint32_t tick_cnt_h = 0;
    uint64_t tick_cnt = 0;
    uint32_t hrate = 0;

    int result;

    char* itrytes = NULL;
    char* itrits = NULL;

    char  nonce_trits[NONCE_LEN];
    char* nonce_trytes = NULL;
    char  otrytes[TRANSACTION_LEN];

    size_t itrytelen = 0;
    size_t itritlen = 0;

    if (argc == 3) {
        mwm = atoi(argv[1]);
        itrytes = argv[2];
        itrytelen = strnlen(itrytes, TRANSACTION_LEN);
        if (TRANSACTION_LEN != itrytelen) {
            fprintf(stderr, HINTS, TRANSACTION_LEN);
            return 1;
        }
        itritlen = 3*itrytelen;
    } else {
        fprintf(stderr, HINTS, TRANSACTION_LEN);
        return 1;
    }

    ctrl_fd = fopen("/dev/cpow-ctrl", "r+");

    if(ctrl_fd == NULL) {
        perror("cpow-ctrl open fail");
        exit(EXIT_FAILURE);
    }

    in_fd = fopen("/dev/cpow-idata", "wb");

    if(in_fd == NULL) {
        perror("cpow-idata open fail");
        fclose(ctrl_fd);
	exit(EXIT_FAILURE);
    }

    out_fd = fopen("/dev/cpow-odata", "rb");

    if(out_fd == NULL) {
        perror("cpow-odata open fail");
 	    fclose(ctrl_fd);
	    fclose(in_fd);
        exit(EXIT_FAILURE);
    }

    devmem_fd = open("/dev/mem", O_RDWR | O_SYNC);

    if(devmem_fd < 0) {
        perror("devmem open");
        fclose(ctrl_fd);
	    fclose(in_fd);
        fclose(out_fd);
        exit(EXIT_FAILURE);
    }

    fpga_regs_map = (uint32_t*)mmap(NULL, HPS_TO_FPGA_SPAN, PROT_READ|PROT_WRITE, MAP_SHARED, devmem_fd, HPS_TO_FPGA_BASE);

    if(fpga_regs_map == MAP_FAILED) {
        perror("devmem mmap");
        close(devmem_fd);
        fclose(ctrl_fd);
	    fclose(in_fd);
        fclose(out_fd);
        exit(EXIT_FAILURE);
    }

    cpow_map = (uint32_t*)(fpga_regs_map + CPOW_BASE);

    itrits = trits_from_trytes(itrytes, itrytelen);

    fwrite(itrits, 1, itritlen, in_fd);
    fflush(in_fd);

    fwrite(&mwm, 1, 1, ctrl_fd);
    fread(&result, sizeof(result), 1, ctrl_fd);
    fflush(ctrl_fd);

    fread(nonce_trits, 1, NONCE_LEN, out_fd);

    nonce_trytes = trytes_from_trits(nonce_trits, 0, NONCE_LEN);

    for (int i = 0; i < TRANSACTION_LEN; i=i+1)
        if (i < NONCE_OFFSET)
            otrytes[i] = itrytes[i];
        else
            otrytes[i] = nonce_trytes[i - NONCE_OFFSET];

    printf("\nTransaction with valid NONCE\n");
    printf("\n%s\n", otrytes);

    hash_cnt = *(cpow_map + HASH_CNT_REG_OFFSET);
    tick_cnt_l = *(cpow_map + TICK_CNT_LOW_REG_OFFSET);
    tick_cnt_h = *(cpow_map + TICK_CNT_HI_REG_OFFSET);
    tick_cnt = tick_cnt_h;
    tick_cnt = (tick_cnt << 32) | tick_cnt_l;

    hrate = (float)hash_cnt / tick_cnt * 100000000;
    printf("Hash rate = %d hash/sec\n", hrate);

    if (nonce_trytes)
        free(nonce_trytes);

    if (itrits)
        free(itrits);

    fclose(in_fd);
    fclose(out_fd);
    fclose(ctrl_fd);

    result = munmap(fpga_regs_map, HPS_TO_FPGA_SPAN); 

    close(devmem_fd);

    if(result < 0) {
        perror("devmem munmap");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

