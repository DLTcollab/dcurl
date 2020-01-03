/*
 * Copyright (C) 2018-2019 BiiLabs Co., Ltd. and Contributors
 * Copyright (c) 2018 Ievgen Korokyi
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#include "pow_fpga.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "implcontext.h"
#include "trinary.h"

/* Set FPGA configuration for device files */
#define DEV_CTRL_FPGA "/dev/cpow-ctrl"
#define DEV_IDATA_FPGA "/dev/cpow-idata"
#define DEV_ODATA_FPGA "/dev/cpow-odata"
#define DEV_MEM_FPGA "/dev/mem"

/* Set memory map of fpga-accelerator PoW */
#define HPS_TO_FPGA_BASE 0xC0000000
#define HPS_TO_FPGA_SPAN 0x0020000
#define HASH_CNT_REG_OFFSET 4
#define TICK_CNT_LOW_REG_OFFSET 5
#define TICK_CNT_HI_REG_OFFSET 6
#define CPOW_BASE 0

/* Set FPGA operation frequency 100 MHz */
#define FPGA_OPERATION_FREQUENCY 100000000

#define INT_TO_STRING(I, S)      \
    {                            \
        S[0] = I & 0xff;         \
        S[1] = (I >> 8) & 0xff;  \
        S[2] = (I >> 16) & 0xff; \
        S[3] = (I >> 24) & 0xff; \
    }

static bool pow_fpga(void *pow_ctx)
{
    pow_fpga_context_t *ctx = (pow_fpga_context_t *) pow_ctx;

    int8_t fpga_out_nonce_trit[NONCE_TRITS_LENGTH];

    char result[4];
    char buf[4];
    bool res = true;

    uint32_t tick_cnt_l;
    uint32_t tick_cnt_h;
    uint64_t tick_cnt;

    trytes_t *object_tryte, *nonce_tryte = NULL;
    trits_t *object_trit = NULL, *object_nonce_trit = NULL;

    object_tryte = init_trytes(ctx->input_trytes, TRANSACTION_TRYTES_LENGTH);
    if (!object_tryte) {
        res = false;
        goto fail;
    }

    object_trit = trits_from_trytes(object_tryte);
    if (!object_trit) {
        res = false;
        goto fail;
    }

    lseek(ctx->in_fd, 0, 0);
    lseek(ctx->ctrl_fd, 0, 0);
    lseek(ctx->out_fd, 0, 0);

    if (write(ctx->in_fd, (char *) object_trit->data,
              TRANSACTION_TRITS_LENGTH) < 0) {
        res = false;
        goto fail;
    }

    INT_TO_STRING(ctx->mwm, buf);
    if (write(ctx->ctrl_fd, buf, sizeof(buf)) < 0) {
        res = false;
        goto fail;
    }
    if (read(ctx->ctrl_fd, result, sizeof(result)) < 0) {
        res = false;
        goto fail;
    }

    if (read(ctx->out_fd, (char *) fpga_out_nonce_trit, NONCE_TRITS_LENGTH) <
        0) {
        res = false;
        goto fail;
    }

    object_nonce_trit = init_trits(fpga_out_nonce_trit, NONCE_TRITS_LENGTH);
    if (!object_nonce_trit) {
        res = false;
        goto fail;
    }

    nonce_tryte = trytes_from_trits(object_nonce_trit);
    if (!nonce_tryte) {
        res = false;
        goto fail;
    }

    tick_cnt_l = *(ctx->cpow_map + TICK_CNT_LOW_REG_OFFSET);
    tick_cnt_h = *(ctx->cpow_map + TICK_CNT_HI_REG_OFFSET);
    tick_cnt = tick_cnt_h;
    tick_cnt = (tick_cnt << 32) | tick_cnt_l;
    ctx->pow_info.time = (double) tick_cnt / (double) FPGA_OPERATION_FREQUENCY;
    ctx->pow_info.hash_count = *(ctx->cpow_map + HASH_CNT_REG_OFFSET);

    memcpy(ctx->output_trytes, ctx->input_trytes, (NONCE_TRINARY_OFFSET) / 3);
    memcpy(ctx->output_trytes + ((NONCE_TRINARY_OFFSET) / 3), nonce_tryte->data,
           ((TRANSACTION_TRITS_LENGTH) - (NONCE_TRINARY_OFFSET)) / 3);

fail:
    free_trinary_object(object_tryte);
    free_trinary_object(object_trit);
    free_trinary_object(object_nonce_trit);
    free_trinary_object(nonce_tryte);
    return res;
}

static bool pow_fpga_context_initialize(impl_context_t *impl_ctx)
{
    pow_fpga_context_t *ctx =
        (pow_fpga_context_t *) malloc(sizeof(pow_fpga_context_t));
    if (!ctx)
        goto fail_to_malloc;

    ctx->ctrl_fd = open(DEV_CTRL_FPGA, O_RDWR);
    if (ctx->ctrl_fd < 0) {
        perror("cpow-ctrl open fail");
        goto fail_to_open_ctrl;
    }
    ctx->in_fd = open(DEV_IDATA_FPGA, O_RDWR);
    if (ctx->in_fd < 0) {
        perror("cpow-idata open fail");
        goto fail_to_open_idata;
    }
    ctx->out_fd = open(DEV_ODATA_FPGA, O_RDWR);
    if (ctx->out_fd < 0) {
        perror("cpow-odata open fail");
        goto fail_to_open_odata;
    }

    ctx->dev_mem_fd = open(DEV_MEM_FPGA, O_RDWR | O_SYNC);
    if (ctx->dev_mem_fd < 0) {
        perror("devmem open fail");
        goto fail_to_open_mem;
    }

    ctx->fpga_regs_map =
        (uint32_t *) mmap(NULL, HPS_TO_FPGA_SPAN, PROT_READ | PROT_WRITE,
                          MAP_SHARED, ctx->dev_mem_fd, HPS_TO_FPGA_BASE);
    if (ctx->fpga_regs_map == MAP_FAILED) {
        perror("devmem mmap fial");
        goto fail_to_mmap;
    }

    ctx->cpow_map = (uint32_t *) (ctx->fpga_regs_map) + CPOW_BASE;

    impl_ctx->context = ctx;

    return true;

fail_to_mmap:
    close(ctx->dev_mem_fd);
fail_to_open_mem:
    close(ctx->out_fd);
fail_to_open_odata:
    close(ctx->in_fd);
fail_to_open_idata:
    close(ctx->ctrl_fd);
fail_to_open_ctrl:
    free(ctx);
fail_to_malloc:
    return false;
}

static void pow_fpga_context_destroy(impl_context_t *impl_ctx)
{
    pow_fpga_context_t *ctx = (pow_fpga_context_t *) impl_ctx->context;

    close(ctx->in_fd);
    close(ctx->out_fd);
    close(ctx->ctrl_fd);

    int result = munmap(ctx->fpga_regs_map, HPS_TO_FPGA_SPAN);
    if (result < 0) {
        perror("devmem munmap fail");
    }
    close(ctx->dev_mem_fd);

    free(ctx);
}

static void *pow_fpga_get_pow_context(impl_context_t *impl_ctx,
                                      int8_t *trytes,
                                      int mwm,
                                      int threads)
{
    pow_fpga_context_t *ctx = impl_ctx->context;
    memcpy(ctx->input_trytes, trytes, TRANSACTION_TRYTES_LENGTH);
    ctx->mwm = mwm;
    ctx->index_of_context = 0;

    return ctx;
}

static bool pow_fpga_free_pow_context(impl_context_t *impl_ctx, void *pow_ctx)
{
    return true;
}

static int8_t *pow_fpga_get_pow_result(void *pow_ctx)
{
    int8_t *ret = (int8_t *) malloc(sizeof(int8_t) * TRANSACTION_TRYTES_LENGTH);
    if (!ret)
        return NULL;
    memcpy(ret, ((pow_fpga_context_t *) pow_ctx)->output_trytes,
           TRANSACTION_TRYTES_LENGTH);
    return ret;
}

static pow_info_t pow_fpga_get_pow_info(void *pow_ctx)
{
    return ((pow_fpga_context_t *) pow_ctx)->pow_info;
}

impl_context_t pow_fpga_context = {
    .context = NULL,
    .description = "FPGA",
    .bitmap = 0,
    .num_max_thread = 1,  // num_max_thread >= 1
    .num_working_thread = 0,
    .initialize = pow_fpga_context_initialize,
    .destroy = pow_fpga_context_destroy,
    .get_pow_context = pow_fpga_get_pow_context,
    .free_pow_context = pow_fpga_free_pow_context,
    .do_the_pow = pow_fpga,
    .get_pow_result = pow_fpga_get_pow_result,
    .get_pow_info = pow_fpga_get_pow_info,
};
