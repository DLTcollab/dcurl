/*
 * Copyright (C) 2018 dcurl Developers.
 * Copyright (C) 2017 IOTA AS, IOTA Foundation and Developers.
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE file.
 */

#include "pow_cl.h"

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "curl.h"
#include "clcontext.h"
#include "constants.h"
#include "implcontext.h"

static CLContext _opencl_ctx[MAX_NUM_DEVICES];

static int write_cl_buffer(CLContext *ctx,
                           int64_t *mid_low,
                           int64_t *mid_high,
                           int mwm,
                           int loop_count)
{
    cl_command_queue cmdq = ctx->cmdq;
    cl_mem *memobj = ctx->buffer;
    BufferInfo *buffer_info = ctx->kernel_info.buffer_info;

    if (clEnqueueWriteBuffer(cmdq, memobj[INDEX_OF_MID_LOW], CL_TRUE, 0,
                             buffer_info[INDEX_OF_MID_LOW].size,
                             mid_low, 0, NULL, NULL) != CL_SUCCESS)
        return 0;
    if (clEnqueueWriteBuffer(cmdq, memobj[INDEX_OF_MID_HIGH], CL_TRUE, 0,
                             buffer_info[INDEX_OF_MID_HIGH].size,
                             mid_high, 0, NULL, NULL) != CL_SUCCESS)
        return 0;
    if (clEnqueueWriteBuffer(cmdq, memobj[INDEX_OF_MWM], CL_TRUE, 0,
                             buffer_info[INDEX_OF_MWM].size,
                             &mwm, 0, NULL, NULL) != CL_SUCCESS)
        return 0;
    if (clEnqueueWriteBuffer(cmdq, memobj[INDEX_OF_LOOP_COUNT], CL_TRUE, 0,
                             buffer_info[INDEX_OF_LOOP_COUNT].size,
                             &loop_count, 0, NULL, NULL) != CL_SUCCESS)
        return 0;
    return 1;
}

static void init_state(int8_t *state,
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

static int8_t *pwork(int8_t *state, int mwm, CLContext *ctx)
{
    size_t local_work_size, global_work_size, global_offset, num_groups;
    char found = 0;
    cl_event ev, ev1;
    CLContext *titan = ctx;
    global_offset = 0;
    num_groups = titan->num_cores;
    local_work_size = STATE_LENGTH;
    while (local_work_size > titan->num_work_group) {
        local_work_size /= 3;
    }

    global_work_size = local_work_size * num_groups;

    int64_t mid_low[STATE_LENGTH] = {0}, mid_high[STATE_LENGTH] = {0};
    init_state(state, mid_low, mid_high, HASH_LENGTH - NONCE_LENGTH);

    if (!write_cl_buffer(titan, mid_low, mid_high, mwm, 32))
        return NULL;

    if (CL_SUCCESS == clEnqueueNDRangeKernel(titan->cmdq, titan->kernel[INDEX_OF_KERNEL_INIT],
                                             1, &global_offset, &global_work_size,
                                             &local_work_size, 0, NULL, &ev)) {
        clWaitForEvents(1, &ev);
        clReleaseEvent(ev);

        while (found == 0) {
            if (CL_SUCCESS !=
                clEnqueueNDRangeKernel(titan->cmdq, titan->kernel[INDEX_OF_KERNEL_SEARCH],
                                       1, NULL, &global_work_size, &local_work_size, 0,
                                       NULL, &ev1)) {
                clReleaseEvent(ev1);
                return NULL; /* Running "search" kernel function failed */
            }
            clWaitForEvents(1, &ev1);
            clReleaseEvent(ev1);
            if (CL_SUCCESS != clEnqueueReadBuffer(titan->cmdq, titan->buffer[INDEX_OF_FOUND],
                                                  CL_TRUE, 0, sizeof(char),
                                                  &found, 0, NULL, NULL)) {
                return NULL; /* Read variable "found" failed */
            }
        }
    } else {
        return NULL; /* Running "init" kernel function failed */
    }

    if (CL_SUCCESS != clEnqueueNDRangeKernel(titan->cmdq,
                                             titan->kernel[INDEX_OF_KERNEL_FINALIZE], 1,
                                             NULL, &global_work_size,
                                             &local_work_size, 0, NULL, &ev)) {
        return NULL; /* Running "finalize" kernel function failed */
    }

    int8_t *buf = malloc(HASH_LENGTH);
    if (!buf) return NULL;

    if (found > 0) {
        if (CL_SUCCESS != clEnqueueReadBuffer(titan->cmdq, titan->buffer[INDEX_OF_TRIT_HASH],
                                              CL_TRUE, 0, HASH_LENGTH * sizeof(int8_t), buf,
                                              1, &ev, NULL)) {
            return NULL; /* Read buffer failed */
        }
    }
    clReleaseEvent(ev);
    return buf;
}

static int8_t *tx_to_cstate(Trytes_t *tx)
{
    Curl *c = initCurl();
    if (!c)
        return NULL;

    int8_t *c_state = (int8_t *) malloc(c->state->len);
    if (!c_state)
        return NULL;

    int8_t tyt[(transactionTrinarySize - HashSize) / 3] = {0};

    /* Copy tx->data[:(transactionTrinarySize - HashSize) / 3] to tyt */
    memcpy(tyt, tx->data, (transactionTrinarySize - HashSize) / 3);
    Trytes_t *inn = initTrytes(tyt, (transactionTrinarySize - HashSize) / 3);
    if (!inn)
        return NULL;

    Absorb(c, inn);

    Trits_t *tr = trits_from_trytes(tx);
    if (!tr)
        return NULL;

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

int PowCL(void *pow_ctx)
{
    PoW_CL_Context *ctx = (PoW_CL_Context *) pow_ctx;

    Trytes_t *trytes_t = initTrytes(ctx->input_trytes, TRANSACTION_LENGTH / 3);
    Trits_t *tr = trits_from_trytes(trytes_t);
    if (!tr)
        return 0;

    int8_t *c_state = tx_to_cstate(trytes_t);
    if (!c_state)
        return 0;

    int8_t *ret = pwork(c_state, ctx->mwm, ctx->clctx);
    if (!ret)
        return 0;

    memcpy(&tr->data[TRANSACTION_LENGTH - HASH_LENGTH], ret,
           HASH_LENGTH * sizeof(int8_t));

    Trytes_t *last = trytes_from_trits(tr);
    memcpy(ctx->output_trytes, last->data, TRANSACTION_LENGTH / 3);

    freeTrobject(tr);
    freeTrobject(trytes_t);
    freeTrobject(last);
    /* hack */
    free(c_state);
    free(ret);

    return 1;
}

static int PoWCL_Context_Initialize(ImplContext *impl_ctx)
{
    impl_ctx->num_max_thread = init_clcontext(_opencl_ctx);
    PoW_CL_Context *ctx = (PoW_CL_Context *) malloc(sizeof(PoW_CL_Context) * impl_ctx->num_max_thread);
    if (!ctx) return 0;
    for (int i = 0; i < impl_ctx->num_max_thread; i++) {
        ctx[i].clctx = &_opencl_ctx[i];
        impl_ctx->bitmap = impl_ctx->bitmap << 1 | 0x1;
    }
    impl_ctx->context = ctx;
    pthread_mutex_init(&impl_ctx->lock, NULL);
    return 1;
}

static void PoWCL_Context_Destroy(ImplContext *impl_ctx)
{
    free(impl_ctx->context);
}

static void *PoWCL_getPoWContext(ImplContext *impl_ctx, int8_t *trytes, int mwm)
{
    pthread_mutex_lock(&impl_ctx->lock);
    for (int i = 0; i < impl_ctx->num_max_thread; i++) {
        if (impl_ctx->bitmap & (0x1 << i)) {
            impl_ctx->bitmap &= ~(0x1 << i);
            pthread_mutex_unlock(&impl_ctx->lock);
            PoW_CL_Context *ctx = impl_ctx->context + sizeof(PoW_CL_Context) * i;
            memcpy(ctx->input_trytes, trytes, TRANSACTION_LENGTH / 3);
            ctx->mwm = mwm;
            ctx->indexOfContext = i;
            return ctx;
        }
    }
    pthread_mutex_unlock(&impl_ctx->lock);
    return NULL; /* It should not happen */
}

static int PoWCL_freePoWContext(ImplContext *impl_ctx, void *pow_ctx)
{
    pthread_mutex_lock(&impl_ctx->lock);
    impl_ctx->bitmap |= 0x1 << ((PoW_CL_Context *) pow_ctx)->indexOfContext;
    pthread_mutex_unlock(&impl_ctx->lock);
    return 1;
}

static int8_t *PoWCL_getPoWResult(void *pow_ctx)
{
    int8_t *ret = (int8_t *) malloc(sizeof(int8_t) * (TRANSACTION_LENGTH / 3));
    if (!ret) return NULL;
    memcpy(ret, ((PoW_CL_Context *) pow_ctx)->output_trytes, TRANSACTION_LENGTH / 3);
    return ret;
}

ImplContext PoWCL_Context = {
    .context = NULL,
    .bitmap = 0,
    .num_max_thread = 0,
    .num_working_thread = 0,
    .initialize = PoWCL_Context_Initialize,
    .destroy = PoWCL_Context_Destroy,
    .getPoWContext = PoWCL_getPoWContext,
    .freePoWContext = PoWCL_freePoWContext,
    .doThePoW = PowCL,
    .getPoWResult = PoWCL_getPoWResult,
};
