/*
 * Copyright (C) 2018 dcurl Developers.
 * Copyright (C) 2016 Shinya Yagyu.
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE file.
 */

#include "pow_c.h"
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cpu-utils.h"
#include "curl.h"
#include "implcontext.h"

void transform64(uint64_t *lmid, uint64_t *hmid)
{
    uint64_t alpha, beta, gamma, delta;
    uint64_t *lfrom = lmid, *hfrom = hmid;
    uint64_t *lto = lmid + STATE_TRITS_LENGTH, *hto = hmid + STATE_TRITS_LENGTH;

    for (int r = 0; r < 80; r++) {
        for (int j = 0; j < STATE_TRITS_LENGTH; j++) {
            int t1 = indices[j];
            int t2 = indices[j + 1];
            alpha = lfrom[t1];
            beta = hfrom[t1];
            gamma = hfrom[t2];
            delta = (alpha | (~gamma)) & (lfrom[t2] ^ beta);
            lto[j] = ~delta;
            hto[j] = (alpha ^ gamma) | delta;
        }
        uint64_t *lswap = lfrom, *hswap = hfrom;
        lfrom = lto;
        hfrom = hto;
        lto = lswap;
        hto = hswap;
    }

    for (int j = 0; j < HASH_TRITS_LENGTH; j++) {
        int t1 = indices[j];
        int t2 = indices[j + 1];
        alpha = lfrom[t1];
        beta = hfrom[t1];
        gamma = hfrom[t2];
        delta = (alpha | (~gamma)) & (lfrom[t2] ^ beta);
        lto[j] = ~delta;  // 6
        hto[j] = (alpha ^ gamma) | delta;
    }
}

int incr(uint64_t *mid_low, uint64_t *mid_high)
{
    int i;
    uint64_t carry = 1;

    for (i = INCR_START; i < HASH_TRITS_LENGTH && carry; i++) {
        uint64_t low = mid_low[i], high = mid_high[i];
        mid_low[i] = high ^ low;
        mid_high[i] = low;
        carry = high & (~low);
    }

    return i == HASH_TRITS_LENGTH;
}

void seri(uint64_t *l, uint64_t *h, int n, int8_t *r)
{
    for (int i = HASH_TRITS_LENGTH - NONCE_TRITS_LENGTH; i < HASH_TRITS_LENGTH; i++) {
        int ll = (l[i] >> n) & 1;
        int hh = (h[i] >> n) & 1;

        if (hh == 0 && ll == 1) {
            r[i - HASH_TRITS_LENGTH + NONCE_TRITS_LENGTH] = -1;
        }
        if (hh == 1 && ll == 1) {
            r[i - HASH_TRITS_LENGTH + NONCE_TRITS_LENGTH] = 0;
        }
        if (hh == 1 && ll == 0) {
            r[i - HASH_TRITS_LENGTH + NONCE_TRITS_LENGTH] = 1;
        }
    }
}

int check(uint64_t *l, uint64_t *h, int m)
{
    uint64_t nonce_probe = HBITS;

    for (int i = HASH_TRITS_LENGTH - m; i < HASH_TRITS_LENGTH; i++) {
        nonce_probe &= ~(l[i] ^ h[i]);
        if (nonce_probe == 0)
            return -1;
    }

    for (int i = 0; i < 64; i++) {
        if ((nonce_probe >> i) & 1)
            return i;
    }

    return -1;
}

long long int loop_cpu(uint64_t *lmid,
                       uint64_t *hmid,
                       int m,
                       int8_t *nonce,
                       int *stopPoW)
{
    int n = 0;
    long long int i = 0;
    uint64_t lcpy[STATE_TRITS_LENGTH * 2], hcpy[STATE_TRITS_LENGTH * 2];

    for (i = 0; !incr(lmid, hmid) && !*stopPoW; i++) {
        memcpy(lcpy, lmid, STATE_TRITS_LENGTH * sizeof(uint64_t));
        memcpy(hcpy, hmid, STATE_TRITS_LENGTH * sizeof(uint64_t));
        transform64(lcpy, hcpy);
        if ((n = check(lcpy + STATE_TRITS_LENGTH, hcpy + STATE_TRITS_LENGTH, m)) >= 0) {
            seri(lmid, hmid, n, nonce);
            return i * 64;
        }
    }
    return -i * 64 + 1;
}

void para(int8_t in[], uint64_t l[], uint64_t h[])
{
    for (int i = 0; i < STATE_TRITS_LENGTH; i++) {
        switch (in[i]) {
        case 0:
            l[i] = HBITS;
            h[i] = HBITS;
            break;
        case 1:
            l[i] = LBITS;
            h[i] = HBITS;
            break;
        case -1:
            l[i] = HBITS;
            h[i] = LBITS;
            break;
        }
    }
}

void incrN(int n, uint64_t *mid_low, uint64_t *mid_high)
{
    for (int j = 0; j < n; j++) {
        uint64_t carry = 1;
        for (int i = INCR_START - 27; i < INCR_START && carry; i++) {
            uint64_t low = mid_low[i], high = mid_high[i];
            mid_low[i] = high ^ low;
            mid_high[i] = low;
            carry = high & (~low);
        }
    }
}

static int64_t pwork(int8_t mid[], int mwm, int8_t nonce[], int n,
                     int *stopPoW)
{
    uint64_t lmid[STATE_TRITS_LENGTH] = {0}, hmid[STATE_TRITS_LENGTH] = {0};
    para(mid, lmid, hmid);
    int offset = HASH_TRITS_LENGTH - NONCE_TRITS_LENGTH;

    lmid[offset] = LOW0;
    hmid[offset] = HIGH0;
    lmid[offset + 1] = LOW1;
    hmid[offset + 1] = HIGH1;
    lmid[offset + 2] = LOW2;
    hmid[offset + 2] = HIGH2;
    lmid[offset + 3] = LOW3;
    hmid[offset + 3] = HIGH3;
    incrN(n, lmid, hmid);

    return loop_cpu(lmid, hmid, mwm, nonce, stopPoW);
}

static void *pworkThread(void *pitem)
{
    Pwork_struct *pworkInfo = (Pwork_struct *) pitem;
    pworkInfo->ret = pwork(pworkInfo->mid, pworkInfo->mwm,
                              pworkInfo->nonce, pworkInfo->n,
                              pworkInfo->stopPoW);

    pthread_mutex_lock(pworkInfo->lock);
    if (pworkInfo->ret >= 0) {
        *pworkInfo->stopPoW = 1;
        /* This means this thread got the result */
        pworkInfo->n = -1;
    }
    pthread_mutex_unlock(pworkInfo->lock);
    pthread_exit(NULL);
}

static int8_t *tx_to_cstate(Trytes_t *tx)
{
    Trytes_t *inn = NULL;
    Trits_t *tr = NULL;
    int8_t tyt[TRANSACTION_TRYTES_LENGTH - HASH_TRYTES_LENGTH] = {0};

    Curl *c = initCurl();
    int8_t *c_state = (int8_t *) malloc(STATE_TRITS_LENGTH);
    if (!c || !c_state) goto fail;

    /* Copy tx->data[:TRANSACTION_TRYTES_LENGTH - HASH_TRYTES_LENGTH] to tyt */
    memcpy(tyt, tx->data, TRANSACTION_TRYTES_LENGTH - HASH_TRYTES_LENGTH);

    inn = initTrytes(tyt, TRANSACTION_TRYTES_LENGTH - HASH_TRYTES_LENGTH);
    if (!inn) goto fail;

    Absorb(c, inn);

    tr = trits_from_trytes(tx);
    if (!tr) goto fail;

    /* Prepare an array storing tr[TRANSACTION_TRITS_LENGTH - HASH_TRITS_LENGTH:] */
    memcpy(c_state, tr->data + TRANSACTION_TRITS_LENGTH - HASH_TRITS_LENGTH,
           tr->len - (TRANSACTION_TRITS_LENGTH - HASH_TRITS_LENGTH));
    memcpy(c_state + tr->len - (TRANSACTION_TRITS_LENGTH - HASH_TRITS_LENGTH),
           c->state->data + tr->len - (TRANSACTION_TRITS_LENGTH - HASH_TRITS_LENGTH),
           c->state->len - tr->len + (TRANSACTION_TRITS_LENGTH - HASH_TRITS_LENGTH));

    freeTrobject(inn);
    freeTrobject(tr);
    freeCurl(c);
    return c_state;
fail:
    freeTrobject(inn);
    freeTrobject(tr);
    freeCurl(c);
    free(c_state);
    return NULL;
}

static void nonce_to_result(Trytes_t *tx, Trytes_t *nonce, int8_t *ret)
{
    int rst_len = tx->len - NONCE_TRYTES_LENGTH + nonce->len;

    memcpy(ret, tx->data, tx->len - NONCE_TRYTES_LENGTH);
    memcpy(ret + tx->len - NONCE_TRYTES_LENGTH, nonce->data,
           rst_len - (tx->len - NONCE_TRYTES_LENGTH));
}

bool PowC(void *pow_ctx)
{
    bool res = true;
    Trits_t *nonce_trit = NULL;
    Trytes_t *tx_tryte = NULL, *nonce_tryte = NULL;
    struct timespec start_time, end_time;

    /* Initialize the context */
    PoW_C_Context *ctx = (PoW_C_Context *) pow_ctx;
    ctx->stopPoW = 0;
    ctx->pow_info.time = 0;
    ctx->pow_info.hash_count = 0;
    pthread_mutex_init(&ctx->lock, NULL);
    pthread_t *threads = ctx->threads;
    Pwork_struct *pitem = ctx->pitem;
    int8_t **nonce_array = ctx->nonce_array;

    /* Prepare the input trytes for algorithm */
    tx_tryte = initTrytes(ctx->input_trytes, TRANSACTION_TRYTES_LENGTH);
    if (!tx_tryte) return false;

    int8_t *c_state = tx_to_cstate(tx_tryte);
    if (!c_state) {
        res = false;
        goto fail;
    }

    clock_gettime(CLOCK_REALTIME, &start_time);
    /* Prepare arguments for pthread */
    for (int i = 0; i < ctx->num_threads; i++) {
        pitem[i].mid = c_state;
        pitem[i].mwm = ctx->mwm;
        pitem[i].nonce = nonce_array[i];
        pitem[i].n = i;
        pitem[i].lock = &ctx->lock;
        pitem[i].stopPoW = &ctx->stopPoW;
        pitem[i].ret = 0;
        pthread_create(&threads[i], NULL, pworkThread, (void *) &pitem[i]);
    }

    int completedIndex = -1;
    for (int i = 0; i < ctx->num_threads; i++) {
        pthread_join(threads[i], NULL);
        if (pitem[i].n == -1)
            completedIndex = i;
        ctx->pow_info.hash_count += (uint64_t) (pitem[i].ret >= 0 ? pitem[i].ret : -pitem[i].ret + 1);
    }
    clock_gettime(CLOCK_REALTIME, &end_time);
    ctx->pow_info.time = diff_in_second(start_time, end_time);

    nonce_trit = initTrits(nonce_array[completedIndex], NONCE_TRITS_LENGTH);
    if (!nonce_trit) {
        res = false;
        goto fail;
    }

    nonce_tryte = trytes_from_trits(nonce_trit);
    if (!nonce_tryte) {
        res = false;
        goto fail;
    }

    nonce_to_result(tx_tryte, nonce_tryte, ctx->output_trytes);

fail:
    /* Free memory */
    free(c_state);
    freeTrobject(tx_tryte);
    freeTrobject(nonce_trit);
    freeTrobject(nonce_tryte);
    return res;
}

static bool PoWC_Context_Initialize(ImplContext *impl_ctx)
{
    int nproc = get_avail_nprocs();
    if (impl_ctx->num_max_thread <= 0 || nproc <= 0) return false;

    PoW_C_Context *ctx = (PoW_C_Context *) malloc(sizeof(PoW_C_Context) * impl_ctx->num_max_thread);
    if (!ctx) return false;

    /* Pre-allocate Memory Chunk for each field */
    void *threads_chunk = malloc(impl_ctx->num_max_thread * sizeof(pthread_t) * nproc);
    void *pitem_chunk = malloc(impl_ctx->num_max_thread * sizeof(Pwork_struct) * nproc);
    void *nonce_ptr_chunk = malloc(impl_ctx->num_max_thread * sizeof(int8_t *) * nproc);
    void *nonce_chunk = malloc(impl_ctx->num_max_thread * NONCE_TRITS_LENGTH * nproc);
    if (!threads_chunk || !pitem_chunk || !nonce_ptr_chunk || !nonce_chunk) goto fail;

    for (int i = 0; i < impl_ctx->num_max_thread; i++) {
        ctx[i].threads = (pthread_t *) (threads_chunk + i * sizeof(pthread_t) * nproc);
        ctx[i].pitem = (Pwork_struct *) (pitem_chunk + i * sizeof(Pwork_struct) * nproc);
        ctx[i].nonce_array = (int8_t **) (nonce_ptr_chunk + i * sizeof(int8_t *) * nproc);
        for (int j = 0; j < nproc; j++)
            ctx[i].nonce_array[j] = (int8_t *) (nonce_chunk + i * NONCE_TRITS_LENGTH * nproc +
                                                j * NONCE_TRITS_LENGTH);
        ctx[i].num_max_threads = nproc;
        impl_ctx->bitmap = impl_ctx->bitmap << 1 | 0x1;
    }
    impl_ctx->context = ctx;
    pthread_mutex_init(&impl_ctx->lock, NULL);
    return true;

fail:
    free(ctx);
    free(threads_chunk);
    free(pitem_chunk);
    free(nonce_ptr_chunk);
    free(nonce_chunk);
    return false;
}

static void PoWC_Context_Destroy(ImplContext *impl_ctx)
{
    PoW_C_Context *ctx = (PoW_C_Context *) impl_ctx->context;
    free(ctx[0].threads);
    free(ctx[0].pitem);
    free(ctx[0].nonce_array[0]);
    free(ctx[0].nonce_array);
    free(ctx);
}

static void *PoWC_getPoWContext(ImplContext *impl_ctx, int8_t *trytes, int mwm, int threads)
{
    pthread_mutex_lock(&impl_ctx->lock);
    for (int i = 0; i < impl_ctx->num_max_thread; i++) {
        if (impl_ctx->bitmap & (0x1 << i)) {
            impl_ctx->bitmap &= ~(0x1 << i);
            pthread_mutex_unlock(&impl_ctx->lock);
            PoW_C_Context *ctx = impl_ctx->context + sizeof(PoW_C_Context) * i;
            memcpy(ctx->input_trytes, trytes, TRANSACTION_TRYTES_LENGTH);
            ctx->mwm = mwm;
            ctx->indexOfContext = i;
            if (threads > 0 && threads < ctx->num_max_threads)
                ctx->num_threads = threads;
            else
                ctx->num_threads = ctx->num_max_threads;
            return ctx;
        }
    }
    pthread_mutex_unlock(&impl_ctx->lock);
    return NULL; /* It should not happen */
}

static bool PoWC_freePoWContext(ImplContext *impl_ctx, void *pow_ctx)
{
    pthread_mutex_lock(&impl_ctx->lock);
    impl_ctx->bitmap |= 0x1 << ((PoW_C_Context *) pow_ctx)->indexOfContext;
    pthread_mutex_unlock(&impl_ctx->lock);
    return true;
}

static int8_t *PoWC_getPoWResult(void *pow_ctx)
{
    int8_t *ret = (int8_t *) malloc(sizeof(int8_t) * (TRANSACTION_TRYTES_LENGTH));
    if (!ret) return NULL;
    memcpy(ret, ((PoW_C_Context *) pow_ctx)->output_trytes, TRANSACTION_TRYTES_LENGTH);
    return ret;
}

static PoW_Info PoWC_getPoWInfo(void *pow_ctx)
{
    return ((PoW_C_Context *) pow_ctx)->pow_info;
}

ImplContext PoWC_Context = {
    .context = NULL,
    .description = "CPU (Pure C)",
    .bitmap = 0,
    .num_max_thread = 2,
    .num_working_thread = 0,
    .initialize = PoWC_Context_Initialize,
    .destroy = PoWC_Context_Destroy,
    .getPoWContext = PoWC_getPoWContext,
    .freePoWContext = PoWC_freePoWContext,
    .doThePoW = PowC,
    .getPoWResult = PoWC_getPoWResult,
    .getPoWInfo = PoWC_getPoWInfo,
};
