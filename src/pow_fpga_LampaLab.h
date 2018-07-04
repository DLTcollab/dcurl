#ifndef POW_FPGA_LAMPALAB_H_
#define POW_FPGA_LAMPALAB_H_

FILE *ctrl_fd;
FILE *in_fd;
FILE *out_fd;
int devmem_fd;
void *fpga_regs_map;
uint32_t *cpow_map = 0;

int8_t *PowFPGALampaLab(int8_t *itrytes, int mwm, int index);
int pow_fpga_LampaLab_init();
void pow_fpga_LampaLab_destroy();

#endif
