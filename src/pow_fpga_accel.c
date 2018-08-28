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

#define HPS_TO_FPGA_BASE 0xC0000000
#define HPS_TO_FPGA_SPAN 0x0020000
#define HASH_CNT_REG_OFFSET 4
#define TICK_CNT_LOW_REG_OFFSET 5
#define TICK_CNT_HI_REG_OFFSET 6
#define MWM_MASK_REG_OFFSET 3
#define CPOW_BASE 0

/* Set FPGA configuration for device files */
#define DEV_CTRL_FPGA "/dev/cpow-ctrl"
#define DEV_IDATA_FPGA "/dev/cpow-idata"
#define DEV_ODATA_FPGA "/dev/cpow-odata"

#define INT2STRING(I, S)         \
    {                            \
        S[0] = I & 0xff;         \
        S[1] = (I >> 8) & 0xff;  \
        S[2] = (I >> 16) & 0xff; \
        S[3] = (I >> 24) & 0xff; \
    }

static int devmem_fd;
static void *fpga_regs_map;
static uint32_t *cpow_map;

static bool PoWFPGAAccel(void *pow_ctx)
{
    PoW_FPGA_Accel_Context *ctx = (PoW_FPGA_Accel_Context *) pow_ctx;

    int8_t fpga_out_nonce_trits[NonceTrinarySize];

    char result[4];
    char buf[4];

    Trytes_t *object_trytes =
        initTrytes(ctx->input_trytes, (transactionTrinarySize) / 3);
    if (!object_trytes)
        return false;

    Trits_t *object_trits = trits_from_trytes(object_trytes);
    if (!object_trits)
        return false;

    lseek(ctx->in_fd, 0, 0);
    lseek(ctx->ctrl_fd, 0, 0);
    lseek(ctx->out_fd, 0, 0);

    if (write(ctx->in_fd, (char *) object_trits->data, transactionTrinarySize) <
        0)
        return false;

    INT2STRING(ctx->mwm, buf);
    if (write(ctx->ctrl_fd, buf, sizeof(buf)) < 0)
        return false;
    if (read(ctx->ctrl_fd, result, sizeof(result)) < 0)
        return false;

    if (read(ctx->out_fd, (char *) fpga_out_nonce_trits, NonceTrinarySize) < 0)
        return false;

    Trits_t *object_nonce_trits =
        initTrits(fpga_out_nonce_trits, NonceTrinarySize);
    if (!object_nonce_trits)
        return false;

    Trytes_t *nonce_trytes = trytes_from_trits(object_nonce_trits);
    if (!nonce_trytes)
        return false;

    memcpy(ctx->output_trytes, ctx->input_trytes, (NonceTrinaryOffset) / 3);
    memcpy(ctx->output_trytes + ((NonceTrinaryOffset) / 3), nonce_trytes->data,
           ((transactionTrinarySize) - (NonceTrinaryOffset)) / 3);

    freeTrobject(object_trytes);
    freeTrobject(object_trits);
    freeTrobject(object_nonce_trits);
    freeTrobject(nonce_trytes);

    return true;
}

static bool PoWFPGAAccel_Context_Initialize(ImplContext *impl_ctx)
{
    int i = 0;
    devmem_fd = 0;
    fpga_regs_map = 0;
    cpow_map = 0;

    PoW_FPGA_Accel_Context *ctx = (PoW_FPGA_Accel_Context *) malloc(
        sizeof(PoW_FPGA_Accel_Context) * impl_ctx->num_max_thread);
    if (!ctx)
        goto fail_to_malloc;

    for (i = 0; i < impl_ctx->num_max_thread; i++) {
        ctx[i].ctrl_fd = open(DEV_CTRL_FPGA, O_RDWR);
        if (ctx[i].ctrl_fd < 0) {
            perror("cpow-ctrl open fail");
            goto fail_to_open_ctrl;
        }
        ctx[i].in_fd = open(DEV_IDATA_FPGA, O_RDWR);
        if (ctx[i].in_fd < 0) {
            perror("cpow-idata open fail");
            goto fail_to_open_idata;
        }
        ctx[i].out_fd = open(DEV_ODATA_FPGA, O_RDWR);
        if (ctx[i].out_fd < 0) {
            perror("cpow-odata open fail");
            goto fail_to_open_odata;
        }
        impl_ctx->bitmap = impl_ctx->bitmap << 1 | 0x1;
    }
    impl_ctx->context = ctx;
    pthread_mutex_init(&impl_ctx->lock, NULL);

    devmem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (devmem_fd < 0) {
        perror("devmem open");
        goto fail_to_open_memopen;
    }

    fpga_regs_map =
        (uint32_t *) mmap(NULL, HPS_TO_FPGA_SPAN, PROT_READ | PROT_WRITE,
                          MAP_SHARED, devmem_fd, HPS_TO_FPGA_BASE);
    if (fpga_regs_map == MAP_FAILED) {
        perror("devmem mmap");
        goto fail_to_open_memmap;
    }

    cpow_map = (uint32_t *) (fpga_regs_map + CPOW_BASE);

    return true;

fail_to_open_memmap:
    close(devmem_fd);
fail_to_open_memopen:
    close(ctx[i].out_fd);
fail_to_open_odata:
    close(ctx[i].in_fd);
fail_to_open_idata:
    close(ctx[i].ctrl_fd);
fail_to_open_ctrl:
fail_to_malloc:
    for (int j = i - 1; j > 0; j--) {
        close(ctx[j].in_fd);
        close(ctx[j].out_fd);
        close(ctx[j].ctrl_fd);
    }
    return false;
}

static void PoWFPGAAccel_Context_Destroy(ImplContext *impl_ctx)
{
    PoW_FPGA_Accel_Context *ctx = (PoW_FPGA_Accel_Context *) impl_ctx->context;
    for (int i = 0; i < impl_ctx->num_max_thread; i++) {
        close(ctx[i].in_fd);
        close(ctx[i].out_fd);
        close(ctx[i].ctrl_fd);
    }
    free(ctx);

    int result = munmap(fpga_regs_map, HPS_TO_FPGA_SPAN);
    if (result < 0) {
        perror("devmem munmap");
    }

    close(devmem_fd);
}

static void *PoWFPGAAccel_getPoWContext(ImplContext *impl_ctx,
                                        int8_t *trytes,
                                        int mwm)
{
    pthread_mutex_lock(&impl_ctx->lock);
    for (int i = 0; i < impl_ctx->num_max_thread; i++) {
        if (impl_ctx->bitmap & (0x1 << i)) {
            impl_ctx->bitmap &= ~(0x1 << i);
            pthread_mutex_unlock(&impl_ctx->lock);
            PoW_FPGA_Accel_Context *ctx =
                impl_ctx->context + sizeof(PoW_FPGA_Accel_Context) * i;
            memcpy(ctx->input_trytes, trytes, (transactionTrinarySize) / 3);
            ctx->mwm = mwm;
            ctx->indexOfContext = i;
            return ctx;
        }
    }

    pthread_mutex_unlock(&impl_ctx->lock);
    return NULL; /* It should not happen */
}

static bool PoWFPGAAccel_freePoWContext(ImplContext *impl_ctx, void *pow_ctx)
{
    pthread_mutex_lock(&impl_ctx->lock);
    impl_ctx->bitmap |= 0x1
                        << ((PoW_FPGA_Accel_Context *) pow_ctx)->indexOfContext;
    pthread_mutex_unlock(&impl_ctx->lock);
    return true;
}

static int8_t *PoWFPGAAccel_getPoWResult(void *pow_ctx)
{
    int8_t *ret =
        (int8_t *) malloc(sizeof(int8_t) * ((transactionTrinarySize) / 3));
    if (!ret)
        return NULL;
    memcpy(ret, ((PoW_FPGA_Accel_Context *) pow_ctx)->output_trytes,
           (transactionTrinarySize) / 3);
    return ret;
}

ImplContext PoWFPGAAccel_Context = {
    .context = NULL,
    .bitmap = 0,
    .num_max_thread = 1,  // num_max_thread >= 1
    .num_working_thread = 0,
    .initialize = PoWFPGAAccel_Context_Initialize,
    .destroy = PoWFPGAAccel_Context_Destroy,
    .getPoWContext = PoWFPGAAccel_getPoWContext,
    .freePoWContext = PoWFPGAAccel_freePoWContext,
    .doThePoW = PoWFPGAAccel,
    .getPoWResult = PoWFPGAAccel_getPoWResult,
};
