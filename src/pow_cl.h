#ifndef POW_CL_H_
#define POW_CL_H_

#include <stdbool.h>
#include "clcontext.h"
#include "common.h"
#include "constants.h"
#include "trinary.h"

typedef struct _pow_cl_context PoW_CL_Context;

struct _pow_cl_context {
    CLContext *clctx;
    /* Management of Multi-thread */
    int indexOfContext;
    /* Arguments of PoW */
    int8_t input_trytes[TRANSACTION_TRYTES_LENGTH];  /* 2673 */
    int8_t output_trytes[TRANSACTION_TRYTES_LENGTH]; /* 2673 */
    int mwm;
    /* PoW-related information */
    PoW_Info pow_info;
};

/* Number of GPU devices supported */
#define MAX_NUM_DEVICES 8

#define HIGH_BITS 0xFFFFFFFFFFFFFFFF
#define LOW_BITS 0x0000000000000000
#define LOW_0 0xDB6DB6DB6DB6DB6D
#define HIGH_0 0xB6DB6DB6DB6DB6DB
#define LOW_1 0xF1F8FC7E3F1F8FC7
#define HIGH_1 0x8FC7E3F1F8FC7E3F
#define LOW_2 0x7FFFE00FFFFC01FF
#define HIGH_2 0xFFC01FFFF803FFFF
#define LOW_3 0xFFC0000007FFFFFF
#define HIGH_3 0x003FFFFFFFFFFFFF

#define LOOP_COUNT 32

#endif
