#ifndef POW_FPGA_ACCEL_H_
#define POW_FPGA_ACCEL_H_

#include <error.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "trinary.h"

int8_t *PowFPGAAccel(int8_t *itrytes, int mwm, int index);
int pow_fpga_accel_init();
void pow_fpga_accel_destroy();

#endif
