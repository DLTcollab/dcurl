/*
 * Copyright (C) 2018 dcurl Developers.
 * Copyright (c) 2018 Ievgen Korokyi.
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE file.
 */

#include "pow_fpga_accel.h"
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

#define INT2STRING(I, S)         \
    {                            \
        S[0] = I & 0xff;         \
        S[1] = (I >> 8) & 0xff;  \
        S[2] = (I >> 16) & 0xff; \
        S[3] = (I >> 24) & 0xff; \
    }

static bool PoWFPGAAccel(void *pow_ctx)
{
    PoW_FPGA_Accel_Context *ctx = (PoW_FPGA_Accel_Context *) pow_ctx;
    ctx->pow_info.time = 0;
    ctx->pow_info.hash_count = 0;

    int8_t fpga_out_nonce_trit[NONCE_TRITS_LENGTH];

    char result[4];
    char buf[4];
    bool res = true;

    uint32_t tick_cnt_l = 0;
    uint32_t tick_cnt_h = 0;
    uint64_t tick_cnt = 0;

    Trytes_t *object_tryte = NULL, *nonce_tryte = NULL;
    Trits_t *object_trit = NULL, *object_nonce_trit = NULL;

    object_tryte = initTrytes(ctx->input_trytes, TRANSACTION_TRYTES_LENGTH);
    if (!object_tryte)
        return false;

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

    INT2STRING(ctx->mwm, buf);
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

    object_nonce_trit = initTrits(fpga_out_nonce_trit, NONCE_TRITS_LENGTH);
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

    memcpy(ctx->output_trytes, ctx->input_trytes, (NonceTrinaryOffset) / 3);
    memcpy(ctx->output_trytes + ((NonceTrinaryOffset) / 3), nonce_tryte->data,
           ((TRANSACTION_TRITS_LENGTH) - (NonceTrinaryOffset)) / 3);

fail:
    freeTrobject(object_tryte);
    freeTrobject(object_trit);
    freeTrobject(object_nonce_trit);
    freeTrobject(nonce_tryte);
    return res;
}

static bool PoWFPGAAccel_Context_Initialize(ImplContext *impl_ctx)
{
    PoW_FPGA_Accel_Context *ctx =
        (PoW_FPGA_Accel_Context *) malloc(sizeof(PoW_FPGA_Accel_Context));
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

    ctx->devmem_fd = open(DEV_MEM_FPGA, O_RDWR | O_SYNC);
    if (ctx->devmem_fd < 0) {
        perror("devmem open fail");
        goto fail_to_open_mem;
    }

    ctx->fpga_regs_map =
        (uint32_t *) mmap(NULL, HPS_TO_FPGA_SPAN, PROT_READ | PROT_WRITE,
                          MAP_SHARED, ctx->devmem_fd, HPS_TO_FPGA_BASE);
    if (ctx->fpga_regs_map == MAP_FAILED) {
        perror("devmem mmap fial");
        goto fail_to_mmap;
    }

    ctx->cpow_map = (uint32_t *) (ctx->fpga_regs_map + CPOW_BASE);

    impl_ctx->context = ctx;

    return true;

fail_to_mmap:
    close(ctx->devmem_fd);
fail_to_open_mem:
    close(ctx->out_fd);
fail_to_open_odata:
    close(ctx->in_fd);
fail_to_open_idata:
    close(ctx->ctrl_fd);
fail_to_open_ctrl:
fail_to_malloc:
    return false;
}

static void PoWFPGAAccel_Context_Destroy(ImplContext *impl_ctx)
{
    PoW_FPGA_Accel_Context *ctx = (PoW_FPGA_Accel_Context *) impl_ctx->context;

    close(ctx->in_fd);
    close(ctx->out_fd);
    close(ctx->ctrl_fd);

    int result = munmap(ctx->fpga_regs_map, HPS_TO_FPGA_SPAN);
    if (result < 0) {
        perror("devmem munmap fail");
    }
    close(ctx->devmem_fd);

    free(ctx);
}

static void *PoWFPGAAccel_getPoWContext(ImplContext *impl_ctx,
                                        int8_t *trytes,
                                        int mwm,
                                        int threads)
{
    PoW_FPGA_Accel_Context *ctx = impl_ctx->context;
    memcpy(ctx->input_trytes, trytes, TRANSACTION_TRYTES_LENGTH);
    ctx->mwm = mwm;
    ctx->indexOfContext = 0;

    return ctx;
}

static bool PoWFPGAAccel_freePoWContext(ImplContext *impl_ctx, void *pow_ctx)
{
    return true;
}

static int8_t *PoWFPGAAccel_getPoWResult(void *pow_ctx)
{
    int8_t *ret = (int8_t *) malloc(sizeof(int8_t) * TRANSACTION_TRYTES_LENGTH);
    if (!ret)
        return NULL;
    memcpy(ret, ((PoW_FPGA_Accel_Context *) pow_ctx)->output_trytes,
           TRANSACTION_TRYTES_LENGTH);
    return ret;
}

static PoW_Info PoWFPGAAccel_getPoWInfo(void *pow_ctx)
{
    return ((PoW_FPGA_Accel_Context *) pow_ctx)->pow_info;
}

ImplContext PoWFPGAAccel_Context = {
    .context = NULL,
    .description = "FPGA",
    .bitmap = 0,
    .num_max_thread = 1,  // num_max_thread >= 1
    .num_working_thread = 0,
    .initialize = PoWFPGAAccel_Context_Initialize,
    .destroy = PoWFPGAAccel_Context_Destroy,
    .getPoWContext = PoWFPGAAccel_getPoWContext,
    .freePoWContext = PoWFPGAAccel_freePoWContext,
    .doThePoW = PoWFPGAAccel,
    .getPoWResult = PoWFPGAAccel_getPoWResult,
    .getPoWInfo = PoWFPGAAccel_getPoWInfo,
};
