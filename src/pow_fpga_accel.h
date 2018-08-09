#ifndef POW_FPGA_ACCEL_H_
#define POW_FPGA_ACCEL_H_

#include <stdint.h>

int8_t *PowFPGAAccel(int8_t *itrytes, int mwm, int index);
int pow_fpga_accel_init();
void pow_fpga_accel_destroy();

#endif
