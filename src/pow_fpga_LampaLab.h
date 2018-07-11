#ifndef POW_FPGA_LAMPALAB_H_
#define POW_FPGA_LAMPALAB_H_


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <error.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "converter.h"

FILE *ctrl_fd;
FILE *in_fd;
FILE *out_fd;
int devmem_fd;
void *fpga_regs_map;
uint32_t *cpow_map;
int result;

char *PowFPGALampaLab(char *itrytes, int mwm, int index);
int pow_fpga_LampaLab_init();
void pow_fpga_LampaLab_destroy();

#endif
