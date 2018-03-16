/*
 * Copyright (C) 2018 dcurl Developers.
 * Copyright (C) 2017 IOTA AS, IOTA Foundation and Developers.
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE file.
 */

#include "pow_cl.h"
#include <CL/cl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "./hash/curl.h"
#include "clcontext.h"
#include "constants.h"

#define HASH_LENGTH 243               // trits
#define NONCE_LENGTH 81               // trits
#define STATE_LENGTH 3 * HASH_LENGTH  // trits
#define TRANSACTION_LENGTH 2673 * 3
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

CLContext **pow_ctx;

static void init_BufferInfo(CLContext *ctx)
{
    ctx->kernel_info.buffer_info[0] =
        (BufferInfo){sizeof(char) * HASH_LENGTH, CL_MEM_WRITE_ONLY};
    ctx->kernel_info.buffer_info[1] =
        (BufferInfo){sizeof(int64_t) * STATE_LENGTH, CL_MEM_READ_WRITE, 2};
    ctx->kernel_info.buffer_info[2] =
        (BufferInfo){sizeof(int64_t) * STATE_LENGTH, CL_MEM_READ_WRITE, 2};
    ctx->kernel_info.buffer_info[3] =
        (BufferInfo){sizeof(int64_t) * STATE_LENGTH, CL_MEM_READ_WRITE, 2};
    ctx->kernel_info.buffer_info[4] =
        (BufferInfo){sizeof(int64_t) * STATE_LENGTH, CL_MEM_READ_WRITE, 2};
    ctx->kernel_info.buffer_info[5] =
        (BufferInfo){sizeof(size_t), CL_MEM_READ_ONLY};
    ctx->kernel_info.buffer_info[6] =
        (BufferInfo){sizeof(char), CL_MEM_READ_WRITE};
    ctx->kernel_info.buffer_info[7] =
        (BufferInfo){sizeof(int64_t), CL_MEM_READ_WRITE, 2};
    ctx->kernel_info.buffer_info[8] =
        (BufferInfo){sizeof(size_t), CL_MEM_READ_ONLY};

    init_cl_buffer(ctx);
}

static void write_cl_buffer(CLContext *ctx,
                            int64_t *mid_low,
                            int64_t *mid_high,
                            int mwm,
                            int loop_count)
{
    cl_command_queue cmdq = ctx->cmdq;
    cl_mem *memobj = ctx->buffer;
    BufferInfo *buffer_info = ctx->kernel_info.buffer_info;

    clEnqueueWriteBuffer(cmdq, memobj[1], CL_TRUE, 0, buffer_info[1].size,
                         mid_low, 0, NULL, NULL);
    clEnqueueWriteBuffer(cmdq, memobj[2], CL_TRUE, 0, buffer_info[2].size,
                         mid_high, 0, NULL, NULL);
    clEnqueueWriteBuffer(cmdq, memobj[5], CL_TRUE, 0, buffer_info[5].size, &mwm,
                         0, NULL, NULL);
    clEnqueueWriteBuffer(cmdq, memobj[8], CL_TRUE, 0, buffer_info[8].size,
                         &loop_count, 0, NULL, NULL);
}

static void init_state(char *state,
                       int64_t *mid_low,
                       int64_t *mid_high,
                       size_t offset)
{
    for (int i = 0; i < STATE_LENGTH; i++) {
        switch (state[i]) {
        case 0:
            mid_low[i] = HIGH_BITS;
            mid_high[i] = HIGH_BITS;
            break;
        case 1:
            mid_low[i] = LOW_BITS;
            mid_high[i] = HIGH_BITS;
            break;
        default:
            mid_low[i] = HIGH_BITS;
            mid_high[i] = LOW_BITS;
        }
    }
    mid_low[offset] = LOW_0;
    mid_high[offset] = HIGH_0;
    mid_low[offset + 1] = LOW_1;
    mid_high[offset + 1] = HIGH_1;
    mid_low[offset + 2] = LOW_2;
    mid_high[offset + 2] = HIGH_2;
    mid_low[offset + 3] = LOW_3;
    mid_high[offset + 3] = HIGH_3;
}

void pwork_ctx_init(int context_size)
{
    char *kernel_name[] = {"init", "search", "finalize"};

    pow_ctx = (CLContext **) malloc(sizeof(CLContext *) * context_size);

    printf("Initializing OpenCL context ...\n");

    for (int i = 0; i < context_size; i++) {
        init_clcontext(&pow_ctx[i]);
        init_cl_kernel(pow_ctx[i], kernel_name);
        init_BufferInfo(pow_ctx[i]);
    }

    printf("Initialize End\n");
}

void pwork_ctx_destroy(int context_size)
{
    if (pow_ctx) {
        for (int i = 0; i < context_size; i++) {
            if (pow_ctx[i])
                free(pow_ctx[i]);
        }
        free(pow_ctx);
    }
}

static char *pwork(char *state, int mwm, int index)
{
    size_t local_work_size, global_work_size, global_offset, num_groups;
    char found = 0;
    cl_event ev, ev1;
    CLContext *titan = pow_ctx[index];
    global_offset = 0;
    num_groups = titan->num_cores;
    local_work_size = STATE_LENGTH;
    while (local_work_size > titan->num_work_group) {
        local_work_size /= 3;
    }

    global_work_size = local_work_size * num_groups;

    int64_t mid_low[STATE_LENGTH] = {0}, mid_high[STATE_LENGTH] = {0};
    init_state(state, mid_low, mid_high, HASH_LENGTH - NONCE_LENGTH);

    write_cl_buffer(titan, mid_low, mid_high, mwm, 32);

    if (CL_SUCCESS == clEnqueueNDRangeKernel(titan->cmdq, titan->kernel[0], 1,
                                             &global_offset, &global_work_size,
                                             &local_work_size, 0, NULL, &ev)) {
        clWaitForEvents(1, &ev);
        clReleaseEvent(ev);

        while (found == 0) {
            if (CL_SUCCESS !=
                clEnqueueNDRangeKernel(titan->cmdq, titan->kernel[1], 1, NULL,
                                       &global_work_size, &local_work_size, 0,
                                       NULL, &ev1)) {
                clReleaseEvent(ev1);
                printf("running search kernel failed\n");
                exit(0);
            }
            clWaitForEvents(1, &ev1);
            clReleaseEvent(ev1);
            if (CL_SUCCESS != clEnqueueReadBuffer(titan->cmdq, titan->buffer[6],
                                                  CL_TRUE, 0, sizeof(char),
                                                  &found, 0, NULL, NULL)) {
                printf("Reading finished bool failed\n");
                exit(0);
            }
        }
    } else {
        printf("Running init kernel failed\n");
        exit(0);
    }

    if (CL_SUCCESS != clEnqueueNDRangeKernel(titan->cmdq, titan->kernel[2], 1,
                                             NULL, &global_work_size,
                                             &local_work_size, 0, NULL, &ev)) {
        printf("Running finalize kernel failed\n");
        exit(0);
    }

    char *buf = malloc(HASH_LENGTH);

    if (found > 0) {
        if (CL_SUCCESS != clEnqueueReadBuffer(
                              titan->cmdq, titan->buffer[0], CL_TRUE, 0,
                              HASH_LENGTH * sizeof(char), buf, 1, &ev, NULL)) {
            printf("Reading buf failed\n");
            exit(0);
        }
    }
    clReleaseEvent(ev);
    return buf;
}

static char *tx_to_cstate(Trytes_t *tx)
{
    Curl *c = initCurl();
    char tyt[(transactionTrinarySize - HashSize) / 3] = {0};

    /* Copy tx->data[:(transactionTrinarySize - HashSize) / 3] to tyt */
    memcpy(tyt, tx->data, (transactionTrinarySize - HashSize) / 3);

    Trytes_t *inn = initTrytes(tyt, (transactionTrinarySize - HashSize) / 3);

    Absorb(c, inn);

    Trits_t *tr = trits_from_trytes(tx);

    char *c_state = (char *) malloc(c->state->len);

    /* Prepare an array storing tr[transactionTrinarySize - HashSize:] */
    memcpy(c_state, tr->data + transactionTrinarySize - HashSize,
           tr->len - (transactionTrinarySize - HashSize));
    memcpy(c_state + tr->len - (transactionTrinarySize - HashSize),
           c->state->data + tr->len - (transactionTrinarySize - HashSize),
           c->state->len - tr->len + (transactionTrinarySize - HashSize));

    freeTrobject(inn);
    freeTrobject(tr);
    freeCurl(c);

    return c_state;
}

Trytes_t *PowCL(Trytes_t *trytes, int mwm, int index)
{
    // Trytes_t *trytes_t = initTrytes(trytes, 2673);
    Trytes_t *trytes_t = trytes;

    Trits_t *tr = trits_from_trytes(trytes_t);

    char *c_state = tx_to_cstate(trytes_t);

    char *ret = pwork(c_state, mwm, index);

    memcpy(&tr->data[TRANSACTION_LENGTH - HASH_LENGTH], ret,
           HASH_LENGTH * sizeof(char));

    Trytes_t *last = trytes_from_trits(tr);

    freeTrobject(tr);
    free(c_state);
    free(ret);

    return last;
}
