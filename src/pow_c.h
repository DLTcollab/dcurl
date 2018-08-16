#ifndef POW_C_H_
#define POW_C_H_

#include "trinary.h"
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include "constants.h"

typedef struct _pwork_struct Pwork_struct;

struct _pwork_struct {
    int8_t *mid;
    int mwm;
    int8_t *nonce;
    int n;
    pthread_mutex_t *lock;
    int *stopSignal;
    int index;
    int64_t ret;
};

typedef struct _pow_c_context PoW_C_Context;

struct _pow_c_context {
    /* Resource of computing */
    pthread_mutex_t lock;
    pthread_t *threads;
    Pwork_struct *pitem;
    int8_t **nonce_array;
    int stopSignal;
    int num_threads;
    /* Management of Multi-thread */
    int indexOfContext;
    /* Arguments of PoW */
    int8_t input_trytes[TRANSACTION_LENGTH / 3]; /* 2673 */
    int8_t output_trytes[TRANSACTION_LENGTH / 3]; /* 2673 */
    int mwm;
};

bool PowC(void *pow_ctx);

#define HBITS 0xFFFFFFFFFFFFFFFFuLL
#define LBITS 0x0000000000000000uLL
#define HASH_LENGTH 243               // trits
#define NONCE_LENGTH 81               // trits
#define STATE_LENGTH 3 * HASH_LENGTH  // trits
#define TX_LENGTH 2673                // trytes
#define INCR_START HASH_LENGTH - NONCE_LENGTH + 4 + 27
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
