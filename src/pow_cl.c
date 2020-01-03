/*
 * Copyright (C) 2018-2019 BiiLabs Co., Ltd. and Contributors
 * Copyright (C) 2017 IOTA AS, IOTA Foundation and Developers
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
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
#include "clcontext.h"
#include "curl.h"
#include "implcontext.h"

static cl_context_t opencl_ctx[MAX_NUM_DEVICES];

static bool write_cl_buffer(cl_context_t *ctx,
                            int64_t *mid_low,
                            int64_t *mid_high,
                            int mwm,
                            int loop_count)
{
    cl_command_queue cmd_q = ctx->cmd_q;
    cl_mem *mem_obj = ctx->buffer;
    buffer_info_t *buffer_info = ctx->kernel_info.buffer_info;

    if (clEnqueueWriteBuffer(cmd_q, mem_obj[INDEX_OF_MID_LOW], CL_TRUE, 0,
                             buffer_info[INDEX_OF_MID_LOW].size, mid_low, 0,
                             NULL, NULL) != CL_SUCCESS)
        return false;
    if (clEnqueueWriteBuffer(cmd_q, mem_obj[INDEX_OF_MID_HIGH], CL_TRUE, 0,
                             buffer_info[INDEX_OF_MID_HIGH].size, mid_high, 0,
                             NULL, NULL) != CL_SUCCESS)
        return false;
    if (clEnqueueWriteBuffer(cmd_q, mem_obj[INDEX_OF_MWM], CL_TRUE, 0,
                             buffer_info[INDEX_OF_MWM].size, &mwm, 0, NULL,
                             NULL) != CL_SUCCESS)
        return false;
    if (clEnqueueWriteBuffer(cmd_q, mem_obj[INDEX_OF_LOOP_COUNT], CL_TRUE, 0,
                             buffer_info[INDEX_OF_LOOP_COUNT].size, &loop_count,
                             0, NULL, NULL) != CL_SUCCESS)
        return false;
    return true;
}

static void init_state(int8_t *state,
                       int64_t *mid_low,
                       int64_t *mid_high,
                       size_t offset)
{
    for (int i = 0; i < STATE_TRITS_LENGTH; i++) {
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

static int8_t *pwork(int8_t *state, int mwm, cl_context_t *ctx)
{
    size_t local_work_size, global_work_size, global_offset, num_groups;
    char found = 0;
    cl_event ev, ev1;
    cl_context_t *titan = ctx;
    ctx->hash_count = 0;
    global_offset = 0;
    num_groups = titan->num_cores;
    local_work_size = STATE_TRITS_LENGTH;
    while (local_work_size > titan->num_work_group) {
        local_work_size /= 3;
    }

    global_work_size = local_work_size * num_groups;

    int64_t mid_low[STATE_TRITS_LENGTH] = {0},
            mid_high[STATE_TRITS_LENGTH] = {0};
    init_state(state, mid_low, mid_high,
               HASH_TRITS_LENGTH - NONCE_TRITS_LENGTH);

    if (!write_cl_buffer(titan, mid_low, mid_high, mwm, LOOP_COUNT))
        return NULL;

    if (CL_SUCCESS == clEnqueueNDRangeKernel(
                          titan->cmd_q, titan->kernel[INDEX_OF_KERNEL_INIT], 1,
                          &global_offset, &global_work_size, &local_work_size,
                          0, NULL, &ev)) {
        clWaitForEvents(1, &ev);
        clReleaseEvent(ev);
        ctx->hash_count += 64 * num_groups * LOOP_COUNT;

        while (found == 0) {
            if (CL_SUCCESS !=
                clEnqueueNDRangeKernel(
                    titan->cmd_q, titan->kernel[INDEX_OF_KERNEL_SEARCH], 1,
                    NULL, &global_work_size, &local_work_size, 0, NULL, &ev1)) {
                clReleaseEvent(ev1);
                return NULL; /* Running "search" kernel function failed */
            }
            clWaitForEvents(1, &ev1);
            clReleaseEvent(ev1);
            if (CL_SUCCESS != clEnqueueReadBuffer(titan->cmd_q,
                                                  titan->buffer[INDEX_OF_FOUND],
                                                  CL_TRUE, 0, sizeof(char),
                                                  &found, 0, NULL, NULL)) {
                return NULL; /* Read variable "found" failed */
            }
        }
    } else {
        return NULL; /* Running "init" kernel function failed */
    }

    if (CL_SUCCESS != clEnqueueNDRangeKernel(
                          titan->cmd_q, titan->kernel[INDEX_OF_KERNEL_FINALIZE],
                          1, NULL, &global_work_size, &local_work_size, 0, NULL,
                          &ev)) {
        return NULL; /* Running "finalize" kernel function failed */
    }

    int8_t *buf = malloc(HASH_TRITS_LENGTH);
    if (!buf)
        return NULL;

    if (found > 0) {
        if (CL_SUCCESS !=
            clEnqueueReadBuffer(titan->cmd_q, titan->buffer[INDEX_OF_TRIT_HASH],
                                CL_TRUE, 0, HASH_TRITS_LENGTH * sizeof(int8_t),
                                buf, 1, &ev, NULL)) {
            return NULL; /* Read buffer failed */
        }
    }
    clReleaseEvent(ev);
    return buf;
}

static int8_t *tx_to_cstate(trytes_t *tx)
{
    trytes_t *inn = NULL;
    trits_t *tr = NULL;
    int8_t tyt[TRANSACTION_TRYTES_LENGTH - HASH_TRYTES_LENGTH] = {0};

    curl_t *c = init_curl();
    int8_t *c_state = (int8_t *) malloc(STATE_TRITS_LENGTH);
    if (!c || !c_state)
        goto fail;

    /* Copy tx->data[:TRANSACTION_TRYTES_LENGTH - HASH_TRYTES_LENGTH] to tyt */
    memcpy(tyt, tx->data, TRANSACTION_TRYTES_LENGTH - HASH_TRYTES_LENGTH);

    inn = init_trytes(tyt, TRANSACTION_TRYTES_LENGTH - HASH_TRYTES_LENGTH);
    if (!inn)
        goto fail;

    absorb(c, inn);

    tr = trits_from_trytes(tx);
    if (!tr)
        goto fail;

    /* Prepare an array storing tr[TRANSACTION_TRITS_LENGTH -
     * HASH_TRITS_LENGTH:] */
    memcpy(c_state, tr->data + TRANSACTION_TRITS_LENGTH - HASH_TRITS_LENGTH,
           tr->len - (TRANSACTION_TRITS_LENGTH - HASH_TRITS_LENGTH));
    memcpy(c_state + tr->len - (TRANSACTION_TRITS_LENGTH - HASH_TRITS_LENGTH),
           c->state->data + tr->len -
               (TRANSACTION_TRITS_LENGTH - HASH_TRITS_LENGTH),
           c->state->len - tr->len +
               (TRANSACTION_TRITS_LENGTH - HASH_TRITS_LENGTH));

    free_trinary_object(inn);
    free_trinary_object(tr);
    free_curl(c);
    return c_state;
fail:
    free_trinary_object(inn);
    free_trinary_object(tr);
    free_curl(c);
    free(c_state);
    return NULL;
}

static bool pow_cl(void *pow_ctx)
{
    bool res = true;
    int8_t *c_state = NULL, *pow_result = NULL;
    trits_t *tx_trit = NULL;
    trytes_t *tx_tryte, *res_tryte = NULL;
    struct timespec start_time, end_time;

    pow_cl_context_t *ctx = (pow_cl_context_t *) pow_ctx;

    tx_tryte = init_trytes(ctx->input_trytes, TRANSACTION_TRYTES_LENGTH);
    if (!tx_tryte) {
        res = false;
        goto fail;
    }

    tx_trit = trits_from_trytes(tx_tryte);
    if (!tx_trit) {
        res = false;
        goto fail;
    }

    c_state = tx_to_cstate(tx_tryte);
    if (!c_state) {
        res = false;
        goto fail;
    }

    clock_gettime(CLOCK_REALTIME, &start_time);
    pow_result = pwork(c_state, ctx->mwm, ctx->cl_ctx);
    clock_gettime(CLOCK_REALTIME, &end_time);
    ctx->pow_info.time = diff_in_second(start_time, end_time);
    if (!pow_result) {
        res = false;
        goto fail;
    }
    memcpy(&tx_trit->data[TRANSACTION_TRITS_LENGTH - HASH_TRITS_LENGTH],
           pow_result, HASH_TRITS_LENGTH * sizeof(int8_t));

    res_tryte = trytes_from_trits(tx_trit);
    if (!res_tryte) {
        res = false;
        goto fail;
    }
    memcpy(ctx->output_trytes, res_tryte->data, TRANSACTION_TRYTES_LENGTH);

    ctx->pow_info.hash_count = ctx->cl_ctx->hash_count;

fail:
    free_trinary_object(tx_trit);
    free_trinary_object(tx_tryte);
    free_trinary_object(res_tryte);
    free(c_state);
    free(pow_result);
    return res;
}

static bool pow_cl_context_initialize(impl_context_t *impl_ctx)
{
    impl_ctx->num_max_thread = init_clcontext(opencl_ctx);
    pow_cl_context_t *ctx = (pow_cl_context_t *) malloc(
        sizeof(pow_cl_context_t) * impl_ctx->num_max_thread);
    if (!ctx)
        goto fail;

    for (int i = 0; i < impl_ctx->num_max_thread; i++) {
        ctx[i].cl_ctx = &opencl_ctx[i];
        impl_ctx->bitmap = impl_ctx->bitmap << 1 | 0x1;
    }
    impl_ctx->context = ctx;
    uv_mutex_init(&impl_ctx->lock);
    return true;

fail:
    free(ctx);
    return false;
}

static void pow_cl_context_destroy(impl_context_t *impl_ctx)
{
    pow_cl_context_t *ctx = (pow_cl_context_t *) impl_ctx->context;
    free(ctx);
}

static void *pow_cl_get_pow_context(impl_context_t *impl_ctx,
                                    int8_t *trytes,
                                    int mwm,
                                    int threads)
{
    uv_mutex_lock(&impl_ctx->lock);
    for (int i = 0; i < impl_ctx->num_max_thread; i++) {
        if (impl_ctx->bitmap & (0x1 << i)) {
            impl_ctx->bitmap &= ~(0x1 << i);
            uv_mutex_unlock(&impl_ctx->lock);
            pow_cl_context_t *ctx = (pow_cl_context_t *) impl_ctx->context + i;
            memcpy(ctx->input_trytes, trytes, TRANSACTION_TRYTES_LENGTH);
            ctx->mwm = mwm;
            ctx->index_of_context = i;
            return ctx;
        }
    }
    uv_mutex_unlock(&impl_ctx->lock);
    return NULL; /* It should not happen */
}

static bool pow_cl_free_pow_context(impl_context_t *impl_ctx, void *pow_ctx)
{
    uv_mutex_lock(&impl_ctx->lock);
    impl_ctx->bitmap |= 0x1 << ((pow_cl_context_t *) pow_ctx)->index_of_context;
    uv_mutex_unlock(&impl_ctx->lock);
    return true;
}

static int8_t *pow_cl_get_pow_result(void *pow_ctx)
{
    int8_t *ret =
        (int8_t *) malloc(sizeof(int8_t) * (TRANSACTION_TRYTES_LENGTH));
    if (!ret)
        return NULL;
    memcpy(ret, ((pow_cl_context_t *) pow_ctx)->output_trytes,
           TRANSACTION_TRYTES_LENGTH);
    return ret;
}

static pow_info_t pow_cl_get_pow_info(void *pow_ctx)
{
    return ((pow_cl_context_t *) pow_ctx)->pow_info;
}

impl_context_t pow_cl_context = {
    .context = NULL,
    .description = "GPU (OpenCL)",
    .bitmap = 0,
    .num_max_thread = 0,
    .num_working_thread = 0,
    .initialize = pow_cl_context_initialize,
    .destroy = pow_cl_context_destroy,
    .get_pow_context = pow_cl_get_pow_context,
    .free_pow_context = pow_cl_free_pow_context,
    .do_the_pow = pow_cl,
    .get_pow_result = pow_cl_get_pow_result,
    .get_pow_info = pow_cl_get_pow_info,
};
