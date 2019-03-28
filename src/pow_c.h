#ifndef POW_C_H_
#define POW_C_H_

#include <stdbool.h>
#include <stdint.h>
#include "common.h"
#include "constants.h"
#include "trinary.h"
#include "uv.h"

typedef struct _pwork_struct Pwork_struct;

struct _pwork_struct {
    int8_t *mid;
    int mwm;
    int8_t *nonce;
    int n;
    uv_rwlock_t *lock;
    int *stopPoW;
    int index;
    int64_t ret;
};

typedef struct _pow_c_context PoW_C_Context;

struct _pow_c_context {
    /* Resource of computing */
    uv_rwlock_t lock;
    /* Data type of libtuv */
    uv_loop_t loop;
    uv_work_t *work_req;
    Pwork_struct *pitem;
    int8_t **nonce_array;
    int stopPoW;
    int num_threads;
    int num_max_threads;
    /* Management of Multi-thread */
    int indexOfContext;
    /* Arguments of PoW */
    int8_t input_trytes[TRANSACTION_TRYTES_LENGTH];  /* 2673 */
    int8_t output_trytes[TRANSACTION_TRYTES_LENGTH]; /* 2673 */
    int mwm;
    /* PoW-related information */
    PoW_Info pow_info;
};

#define HBITS 0xFFFFFFFFFFFFFFFFuLL
#define LBITS 0x0000000000000000uLL
#define INCR_START HASH_TRITS_LENGTH - NONCE_TRITS_LENGTH + 4 + 27
#define LOW0 \
    0xDB6DB6DB6DB6DB6DuLL  // 0b1101101101101101101101101101101101101101101101101101101101101101L;
#define HIGH0 \
    0xB6DB6DB6DB6DB6DBuLL  // 0b1011011011011011011011011011011011011011011011011011011011011011L;
#define LOW1 \
    0xF1F8FC7E3F1F8FC7uLL  // 0b1111000111111000111111000111111000111111000111111000111111000111L;
#define HIGH1 \
    0x8FC7E3F1F8FC7E3FuLL  // 0b1000111111000111111000111111000111111000111111000111111000111111L;
#define LOW2 \
    0x7FFFE00FFFFC01FFuLL  // 0b0111111111111111111000000000111111111111111111000000000111111111L;
#define HIGH2 \
    0xFFC01FFFF803FFFFuLL  // 0b1111111111000000000111111111111111111000000000111111111111111111L;
#define LOW3 \
    0xFFC0000007FFFFFFuLL  // 0b1111111111000000000000000000000000000111111111111111111111111111L;
#define HIGH3 \
    0x003FFFFFFFFFFFFFuLL  // 0b0000000000111111111111111111111111111111111111111111111111111111L;

#endif
