#ifndef POW_FPGA_ACCEL_H_
#define POW_FPGA_ACCEL_H_

#include <stdint.h>
#include "common.h"
#include "constants.h"

typedef struct _pow_fpga_accel_context PoW_FPGA_Accel_Context;

struct _pow_fpga_accel_context {
    /* Management of Multi-thread */
    int indexOfContext;
    /* Arguments of PoW */
    int8_t input_trytes[TRANSACTION_TRYTES_LENGTH];  /* 2673 */
    int8_t output_trytes[TRANSACTION_TRYTES_LENGTH]; /* 2673 */
    int mwm;
    /* PoW-related information */
    PoW_Info pow_info;
    /* Device files for the PFGA accelerator*/
    int ctrl_fd;
    int in_fd;
    int out_fd;
    int devmem_fd;
    /* Memory map of fpga */
    void *fpga_regs_map;
    uint32_t *cpow_map;
};

#endif
