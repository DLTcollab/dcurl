/*
 * Copyright (C) 2018-2019 BiiLabs Co., Ltd. and Contributors
 * Copyright (C) 2016 Shinya Yagyu
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#include "pow_avx.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cpu-utils.h"
#include "curl.h"
#include "implcontext.h"

#ifdef __AVX2__
static void transform_256(__m256i *lmid, __m256i *hmid)
{
    __m256i one = _mm256_set_epi64x(HBITS, HBITS, HBITS, HBITS);
    int t1, t2;
    __m256i alpha, beta, delta;
    __m256i *lto = lmid + STATE_TRITS_LENGTH, *hto = hmid + STATE_TRITS_LENGTH;
    __m256i *lfrom = lmid, *hfrom = hmid;

    for (int r = 0; r < 80; r++) {
        for (int j = 0; j < STATE_TRITS_LENGTH; j++) {
            t1 = indices[j];
            t2 = indices[j + 1];

            alpha = lfrom[t1];
            beta = hfrom[t1];
            delta = _mm256_and_si256(alpha, _mm256_xor_si256(lfrom[t2], beta));
            /* alpha & (lfrom[t2] ^ beta) */
            lto[j] = _mm256_andnot_si256(delta, one); /* ~delta */
            hto[j] = _mm256_or_si256(_mm256_xor_si256(alpha, hfrom[t2]),
                                     delta); /* (alpha ^ hfrom[t2]) | delta */
        }
        __m256i *lswap = lfrom, *hswap = hfrom;
        lfrom = lto;
        hfrom = hto;
        lto = lswap;
        hto = hswap;
    }
    for (int j = 0; j < HASH_TRITS_LENGTH; j++) {
        t1 = indices[j];
        t2 = indices[j + 1];
        alpha = lfrom[t1];
        beta = hfrom[t1];
        delta = _mm256_and_si256(alpha, _mm256_xor_si256(lfrom[t2], beta));
        /* alpha & (lfrom[t2] ^ beta) */
        lto[j] = _mm256_andnot_si256(delta, one); /* ~delta */
        hto[j] = _mm256_or_si256(_mm256_xor_si256(alpha, hfrom[t2]),
                                 delta); /* (alpha ^ hfrom[t2]) | delta */
    }
}

static int incr_256(__m256i *mid_low, __m256i *mid_high)
{
    int i;
    __m256i carry = _mm256_set_epi64x(LOW00, LOW01, LOW02, LOW03);
    __m256i one = _mm256_set_epi64x(HBITS, HBITS, HBITS, HBITS);
    for (i = INCR_START; i < HASH_TRITS_LENGTH && (i == INCR_START || carry[0]);
         i++) {
        __m256i low = mid_low[i], high = mid_high[i];
        __m256i nlow = _mm256_andnot_si256(low, one);
        mid_low[i] = _mm256_xor_si256(high, low);
        mid_high[i] = low;
        carry = _mm256_and_si256(nlow, high); /* high & (~low) */
    }
    return i == HASH_TRITS_LENGTH;
}

static void seri_256(__m256i *low, __m256i *high, int n, int8_t *r)
{
    int index = n >> 6;
    n = n % 64;

    for (int i = HASH_TRITS_LENGTH - NONCE_TRITS_LENGTH; i < HASH_TRITS_LENGTH;
         i++) {
        uint64_t ll = (low[i][index] >> n) & 1;
        uint64_t hh = (high[i][index] >> n) & 1;
        if (hh == 0 && ll == 1) {
            r[i + NONCE_TRITS_LENGTH - HASH_TRITS_LENGTH] = -1;
        }
        if (hh == 1 && ll == 1) {
            r[i + NONCE_TRITS_LENGTH - HASH_TRITS_LENGTH] = 0;
        }
        if (hh == 1 && ll == 0) {
            r[i + NONCE_TRITS_LENGTH - HASH_TRITS_LENGTH] = 1;
        }
    }
}

static int check_256(__m256i *l, __m256i *h, int m)
{
    __m256i nonce_probe = _mm256_set_epi64x(HBITS, HBITS, HBITS, HBITS);
    __m256i one = _mm256_set_epi64x(HBITS, HBITS, HBITS, HBITS);

    for (int i = HASH_TRITS_LENGTH - m; i < HASH_TRITS_LENGTH; i++) {
        __m256i tmp = _mm256_andnot_si256(_mm256_xor_si256(l[i], h[i]), one);
        nonce_probe = _mm256_and_si256(
            tmp, nonce_probe); /* nonce_probe &= ~(l[i] ^ h[i]) */
        if (nonce_probe[0] == LBITS && nonce_probe[1] == LBITS &&
            nonce_probe[2] == LBITS && nonce_probe[3] == LBITS) {
            return -1;
        }
    }

    for (int j = 0; j < 4; j++) {
        for (int i = 0; i < 64; i++) {
            if ((nonce_probe[j] >> i) & 1) {
                return i + j * 64;
            }
        }
    }
    return -2;
}

static void para_256(int8_t in[], __m256i l[], __m256i h[])
{
    for (int i = 0; i < STATE_TRITS_LENGTH; i++) {
        switch (in[i]) {
        case 0:
            l[i] = _mm256_set_epi64x(HBITS, HBITS, HBITS, HBITS);
            h[i] = _mm256_set_epi64x(HBITS, HBITS, HBITS, HBITS);
            break;
        case 1:
            l[i] = _mm256_set_epi64x(LBITS, LBITS, LBITS, LBITS);
            h[i] = _mm256_set_epi64x(HBITS, HBITS, HBITS, HBITS);
            break;
        case -1:
            l[i] = _mm256_set_epi64x(HBITS, HBITS, HBITS, HBITS);
            h[i] = _mm256_set_epi64x(LBITS, LBITS, LBITS, LBITS);
            break;
        }
    }
}

static void incr_n_256(int n, __m256i *mid_low, __m256i *mid_high)
{
    __m256i one = _mm256_set_epi64x(HBITS, HBITS, HBITS, HBITS);
    for (int j = 0; j < n; j++) {
        __m256i carry = _mm256_set_epi64x(HBITS, HBITS, HBITS, HBITS);
        for (int i = HASH_TRYTES_LENGTH * 2 + 4;
             i < HASH_TRYTES_LENGTH * 2 + 4 + 27 && carry[0]; i++) {
            __m256i low = mid_low[i], high = mid_high[i];
            __m256i nlow = _mm256_andnot_si256(low, one);
            mid_low[i] = _mm256_xor_si256(high, low);
            mid_high[i] = low;
            carry = _mm256_and_si256(nlow, high); /* high & (~low) */
        }
    }
}


static int loop_256(__m256i *lmid,
                    __m256i *hmid,
                    int m,
                    int8_t *nonce,
                    int *stop_pow,
                    uv_rwlock_t *lock)
{
    int i = 0;
    __m256i lcpy[STATE_TRITS_LENGTH * 2], hcpy[STATE_TRITS_LENGTH * 2];

    uv_rwlock_rdlock(lock);
    for (i = 0; !incr_256(lmid, hmid) && !*stop_pow; i++) {
        uv_rwlock_rdunlock(lock);
        int n;
        for (int j = 0; j < STATE_TRITS_LENGTH; j++) {
            lcpy[j] = lmid[j];
            hcpy[j] = hmid[j];
        }
        transform_256(lcpy, hcpy);
        if ((n = check_256(lcpy + STATE_TRITS_LENGTH, hcpy + STATE_TRITS_LENGTH,
                           m)) >= 0) {
            seri_256(lmid, hmid, n, nonce);
            return i * 256;
        }
        uv_rwlock_rdlock(lock);
    }
    uv_rwlock_rdunlock(lock);
    return -i * 256 - 1;
}

static int64_t pwork_256(int8_t mid[],
                         int mwm,
                         int8_t nonce[],
                         int n,
                         int *stop_pow,
                         uv_rwlock_t *lock)
{
    __m256i lmid[STATE_TRITS_LENGTH], hmid[STATE_TRITS_LENGTH];
    int offset = HASH_TRITS_LENGTH - NONCE_TRITS_LENGTH;
    para_256(mid, lmid, hmid);
    lmid[offset] = _mm256_set_epi64x(LOW00, LOW01, LOW02, LOW03);
    hmid[offset] = _mm256_set_epi64x(HIGH00, HIGH01, HIGH02, HIGH03);
    lmid[offset + 1] = _mm256_set_epi64x(LOW10, LOW11, LOW12, LOW13);
    hmid[offset + 1] = _mm256_set_epi64x(HIGH10, HIGH11, HIGH12, HIGH13);
    lmid[offset + 2] = _mm256_set_epi64x(LOW20, LOW21, LOW22, LOW23);
    hmid[offset + 2] = _mm256_set_epi64x(HIGH20, HIGH21, HIGH22, HIGH23);
    lmid[offset + 3] = _mm256_set_epi64x(LOW30, LOW31, LOW32, LOW33);
    hmid[offset + 3] = _mm256_set_epi64x(HIGH30, HIGH31, HIGH32, HIGH33);
    lmid[offset + 4] = _mm256_set_epi64x(LOW40, LOW41, LOW42, LOW43);
    hmid[offset + 4] = _mm256_set_epi64x(HIGH40, HIGH41, HIGH42, HIGH43);
    lmid[offset + 5] = _mm256_set_epi64x(LOW50, LOW51, LOW52, LOW53);
    hmid[offset + 5] = _mm256_set_epi64x(HIGH50, HIGH51, HIGH52, HIGH53);
    incr_n_256(n, lmid, hmid);

    return loop_256(lmid, hmid, mwm, nonce, stop_pow, lock);
}

#else /* AVX1 */

static void transform_256(__m256d *lmid, __m256d *hmid)
{
    __m256d one = _mm256_set_pd(HBITS, HBITS, HBITS, HBITS);
    int j, r, t1, t2;
    __m256d alpha, beta, delta;
    __m256d *lto = lmid + STATE_TRITS_LENGTH, *hto = hmid + STATE_TRITS_LENGTH;
    __m256d *lfrom = lmid, *hfrom = hmid;
    for (r = 0; r < 80; r++) {
        for (j = 0; j < STATE_TRITS_LENGTH; j++) {
            t1 = indices[j];
            t2 = indices[j + 1];

            alpha = lfrom[t1];
            beta = hfrom[t1];
            delta = _mm256_and_pd(alpha, _mm256_xor_pd(lfrom[t2], beta));
            /* alpha & (lfrom[t2] ^ beta) */
            lto[j] = _mm256_andnot_pd(delta, one); /* ~delta */
            hto[j] = _mm256_or_pd(_mm256_xor_pd(alpha, hfrom[t2]),
                                  delta); /* (alpha ^ hfrom[t2]) | delta */
        }
        __m256d *lswap = lfrom, *hswap = hfrom;
        lfrom = lto;
        hfrom = hto;
        lto = lswap;
        hto = hswap;
    }
    for (j = 0; j < HASH_TRITS_LENGTH; j++) {
        t1 = indices[j];
        t2 = indices[j + 1];

        alpha = lfrom[t1];
        beta = hfrom[t1];
        delta = _mm256_and_pd(alpha, _mm256_xor_pd(lfrom[t2], beta));
        /* alpha & (lfrom[t2] ^ beta) */
        lto[j] = _mm256_andnot_pd(delta, one); /* ~delta */
        hto[j] = _mm256_or_pd(_mm256_xor_pd(alpha, hfrom[t2]),
                              delta); /* (alpha ^ hfrom[t2]) | delta */
    }
}

static int incr_256(__m256d *mid_low, __m256d *mid_high)
{
    int i;
    __m256d carry;
    carry = _mm256_set_pd(LBITS, LBITS, LBITS, LBITS);
    for (i = INCR_START; i < HASH_TRITS_LENGTH && (i == INCR_START || carry[0]);
         i++) {
        __m256d low = mid_low[i], high = mid_high[i];
        mid_low[i] = _mm256_xor_pd(high, low);
        mid_high[i] = low;
        carry = _mm256_andnot_pd(low, high);  // high & (~low);
    }
    return i == HASH_TRITS_LENGTH;
}

static void seri_256(__m256d *low, __m256d *high, int n, int8_t *r)
{
    int i = 0, index = 0;
    if (n > 63 && n < 128) {
        n -= 64;
        index = 1;
    }
    if (n >= 128 && n < 192) {
        n -= 128;
        index = 2;
    }
    if (n >= 192 && n < 256) {
        n -= 192;
        index = 3;
    }
    for (i = HASH_TRITS_LENGTH - NONCE_TRITS_LENGTH; i < HASH_TRITS_LENGTH;
         i++) {
        long long l = ((dl) low[i][index]).l;
        long long h = ((dl) high[i][index]).l;
        long ll = (l >> n) & 1;
        long hh = (h >> n) & 1;
        if (hh == 0 && ll == 1) {
            r[i + NONCE_TRITS_LENGTH - HASH_TRITS_LENGTH] = -1;
        }
        if (hh == 1 && ll == 1) {
            r[i + NONCE_TRITS_LENGTH - HASH_TRITS_LENGTH] = 0;
        }
        if (hh == 1 && ll == 0) {
            r[i + NONCE_TRITS_LENGTH - HASH_TRITS_LENGTH] = 1;
        }
    }
}

static int check_256(__m256d *l, __m256d *h, int m)
{
    int i, j;  // omit init for speed

    __m256d nonce_probe = _mm256_set_pd(HBITS, HBITS, HBITS, HBITS);
    for (i = HASH_TRITS_LENGTH - m; i < HASH_TRITS_LENGTH; i++) {
        nonce_probe = _mm256_andnot_pd(_mm256_xor_pd(l[i], h[i]),
                                       nonce_probe);  //&= ~(l[i] ^ h[i]);
        if (nonce_probe[0] == LBITS && nonce_probe[1] == LBITS &&
            nonce_probe[2] == LBITS && nonce_probe[3] == LBITS) {
            return -1;
        }
    }
    for (j = 0; j < 4; j++) {
        for (i = 0; i < 64; i++) {
            long long np = ((dl) nonce_probe[j]).l;
            if ((np >> i) & 1) {
                return i + j * 64;
            }
        }
    }
    return -2;
}

static void para_256(int8_t in[], __m256d l[], __m256d h[])
{
    int i = 0;
    for (i = 0; i < STATE_TRITS_LENGTH; i++) {
        switch (in[i]) {
        case 0:
            l[i] = _mm256_set_pd(HBITS, HBITS, HBITS, HBITS);
            h[i] = _mm256_set_pd(HBITS, HBITS, HBITS, HBITS);
            break;
        case 1:
            l[i] = _mm256_set_pd(LBITS, LBITS, LBITS, LBITS);
            h[i] = _mm256_set_pd(HBITS, HBITS, HBITS, HBITS);
            break;
        case -1:
            l[i] = _mm256_set_pd(HBITS, HBITS, HBITS, HBITS);
            h[i] = _mm256_set_pd(LBITS, LBITS, LBITS, LBITS);
            break;
        }
    }
}

static void incr_n_256(int n, __m256d *mid_low, __m256d *mid_high)
{
    int i, j;
    for (j = 0; j < n; j++) {
        __m256d carry;
        carry = _mm256_set_pd(HBITS, HBITS, HBITS, HBITS);
        for (i = HASH_TRYTES_LENGTH * 2 + 4;
             i < HASH_TRYTES_LENGTH * 2 + 4 + 27 && carry[0]; i++) {
            __m256d low = mid_low[i], high = mid_high[i];
            mid_low[i] = _mm256_xor_pd(high, low);
            mid_high[i] = low;
            carry = _mm256_andnot_pd(low, high);  // high & (~low);
        }
    }
}

static int loop_256(__m256d *lmid,
                    __m256d *hmid,
                    int m,
                    int8_t *nonce,
                    int *stop_pow,
                    uv_rwlock_t *lock)
{
    int i = 0;

    __m256d lcpy[STATE_TRITS_LENGTH * 2], hcpy[STATE_TRITS_LENGTH * 2];
    uv_rwlock_rdlock(lock);
    for (i = 0; !incr_256(lmid, hmid) && !*stop_pow; i++) {
        uv_rwlock_rdunlock(lock);
        int n, j;
        for (j = 0; j < STATE_TRITS_LENGTH; j++) {
            lcpy[j] = lmid[j];
            hcpy[j] = hmid[j];
        }
        transform_256(lcpy, hcpy);
        if ((n = check_256(lcpy + STATE_TRITS_LENGTH, hcpy + STATE_TRITS_LENGTH,
                           m)) >= 0) {
            seri_256(lmid, hmid, n, nonce);
            return i * 256;
        }
        uv_rwlock_rdlock(lock);
    }
    uv_rwlock_rdunlock(lock);
    return -i * 256 - 1;
}

static long long int pwork_256(int8_t mid[],
                               int mwm,
                               int8_t nonce[],
                               int n,
                               int *stop_pow,
                               uv_rwlock_t *lock)
{
    __m256d lmid[STATE_TRITS_LENGTH], hmid[STATE_TRITS_LENGTH];
    int offset = HASH_TRITS_LENGTH - NONCE_TRITS_LENGTH;
    para_256(mid, lmid, hmid);
    lmid[offset] = _mm256_set_pd(LOW00, LOW01, LOW02, LOW03);
    hmid[offset] = _mm256_set_pd(HIGH00, HIGH01, HIGH02, HIGH03);
    lmid[offset + 1] = _mm256_set_pd(LOW10, LOW11, LOW12, LOW13);
    hmid[offset + 1] = _mm256_set_pd(HIGH10, HIGH11, HIGH12, HIGH13);
    lmid[offset + 2] = _mm256_set_pd(LOW20, LOW21, LOW22, LOW23);
    hmid[offset + 2] = _mm256_set_pd(HIGH20, HIGH21, HIGH22, HIGH23);
    lmid[offset + 3] = _mm256_set_pd(LOW30, LOW31, LOW32, LOW33);
    hmid[offset + 3] = _mm256_set_pd(HIGH30, HIGH31, HIGH32, HIGH33);
    lmid[offset + 4] = _mm256_set_pd(LOW40, LOW41, LOW42, LOW43);
    hmid[offset + 4] = _mm256_set_pd(HIGH40, HIGH41, HIGH42, HIGH43);
    lmid[offset + 5] = _mm256_set_pd(LOW50, LOW51, LOW52, LOW53);
    hmid[offset + 5] = _mm256_set_pd(HIGH50, HIGH51, HIGH52, HIGH53);
    incr_n_256(n, lmid, hmid);

    return loop_256(lmid, hmid, mwm, nonce, stop_pow, lock);
}

#endif /* __AVX2__ */

static void work_cb(uv_work_t *req)
{
    pwork_t *pwork_info = (pwork_t *) req->data;
    pwork_info->ret =
        pwork_256(pwork_info->mid, pwork_info->mwm, pwork_info->nonce,
                  pwork_info->n, pwork_info->stop_pow, pwork_info->lock);

    uv_rwlock_wrlock(pwork_info->lock);
    if (pwork_info->ret >= 0) {
        *pwork_info->stop_pow = 1;
        /* This means this thread got the result */
        pwork_info->n = -1;
    }
    uv_rwlock_wrunlock(pwork_info->lock);
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

static void nonce_to_result(trytes_t *tx, trytes_t *nonce, int8_t *ret)
{
    int rst_len = tx->len - NONCE_TRYTES_LENGTH + nonce->len;

    memcpy(ret, tx->data, tx->len - NONCE_TRYTES_LENGTH);
    memcpy(ret + tx->len - NONCE_TRYTES_LENGTH, nonce->data,
           rst_len - (tx->len - NONCE_TRYTES_LENGTH));
}

static bool pow_avx(void *pow_ctx)
{
    bool res = true;
    trits_t *nonce_trit = NULL;
    trytes_t *tx_tryte = NULL, *nonce_tryte = NULL;
    struct timespec start_time, end_time;

    /* Initialize the context */
    pow_avx_context_t *ctx = (pow_avx_context_t *) pow_ctx;
    ctx->stop_pow = 0;
    ctx->pow_info.time = 0;
    ctx->pow_info.hash_count = 0;
    uv_rwlock_init(&ctx->lock);
    uv_loop_t *loop_ptr = &ctx->loop;
    uv_work_t *work_req = ctx->work_req;
    pwork_t *pitem = ctx->pitem;
    int8_t **nonce_array = ctx->nonce_array;

    /* Prepare the input trytes for algorithm */
    tx_tryte = init_trytes(ctx->input_trytes, TRANSACTION_TRYTES_LENGTH);
    if (!tx_tryte)
        return false;

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
        pitem[i].stop_pow = &ctx->stop_pow;
        pitem[i].ret = 0;
        work_req[i].data = &pitem[i];
        uv_queue_work(loop_ptr, &work_req[i], work_cb, NULL);
    }

    uv_run(loop_ptr, UV_RUN_DEFAULT);

    int completed_index = -1;
    for (int i = 0; i < ctx->num_threads; i++) {
        if (pitem[i].n == -1)
            completed_index = i;
        ctx->pow_info.hash_count +=
            (uint64_t)(pitem[i].ret >= 0 ? pitem[i].ret : -pitem[i].ret + 1);
    }
    clock_gettime(CLOCK_REALTIME, &end_time);
    ctx->pow_info.time = diff_in_second(start_time, end_time);

    nonce_trit = init_trits(nonce_array[completed_index], NONCE_TRITS_LENGTH);
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
    /* Free resource */
    uv_rwlock_destroy(&ctx->lock);
    free(c_state);
    free_trinary_object(tx_tryte);
    free_trinary_object(nonce_trit);
    free_trinary_object(nonce_tryte);

    return res;
}

static bool pow_avx_context_initialize(impl_context_t *impl_ctx)
{
    impl_ctx->num_max_thread = get_nthds_per_phys_proc();
    int nproc = get_avail_phys_nprocs();
    if (impl_ctx->num_max_thread <= 0 || nproc <= 0)
        return false;

    pow_avx_context_t *ctx = (pow_avx_context_t *) malloc(
        sizeof(pow_avx_context_t) * impl_ctx->num_max_thread);
    if (!ctx)
        return false;

    /* Pre-allocate Memory Chunk for each field */
    void *work_req_chunk =
        malloc(impl_ctx->num_max_thread * sizeof(uv_work_t) * nproc);
    void *pitem_chunk =
        malloc(impl_ctx->num_max_thread * sizeof(pwork_t) * nproc);
    void *nonce_ptr_chunk =
        malloc(impl_ctx->num_max_thread * sizeof(int8_t *) * nproc);
    void *nonce_chunk =
        malloc(impl_ctx->num_max_thread * NONCE_TRITS_LENGTH * nproc);
    if (!work_req_chunk || !pitem_chunk || !nonce_ptr_chunk || !nonce_chunk)
        goto fail;

    for (int i = 0; i < impl_ctx->num_max_thread; i++) {
        ctx[i].work_req = (uv_work_t *) (work_req_chunk) + i * nproc;
        ctx[i].pitem = (pwork_t *) (pitem_chunk) + i * nproc;
        ctx[i].nonce_array = (int8_t **) (nonce_ptr_chunk) + i * nproc;
        for (int j = 0; j < nproc; j++)
            ctx[i].nonce_array[j] = (int8_t *) (nonce_chunk) +
                                    i * NONCE_TRITS_LENGTH * nproc +
                                    j * NONCE_TRITS_LENGTH;
        ctx[i].num_max_threads = nproc;
        impl_ctx->bitmap = impl_ctx->bitmap << 1 | 0x1;
        uv_loop_init(&ctx[i].loop);
    }
    uv_set_threadpool_size(impl_ctx->num_max_thread * nproc);
    impl_ctx->context = ctx;
    uv_mutex_init(&impl_ctx->lock);
    return true;

fail:
    for (int i = 0; i < impl_ctx->num_max_thread; i++) {
        uv_loop_close(&ctx[i].loop);
    }
    free(ctx);
    free(work_req_chunk);
    free(pitem_chunk);
    free(nonce_ptr_chunk);
    free(nonce_chunk);
    return false;
}

static void pow_avx_context_destroy(impl_context_t *impl_ctx)
{
    pow_avx_context_t *ctx = (pow_avx_context_t *) impl_ctx->context;
    for (int i = 0; i < impl_ctx->num_max_thread; i++) {
        uv_loop_close(&ctx[i].loop);
    }
    free(ctx[0].work_req);
    free(ctx[0].pitem);
    free(ctx[0].nonce_array[0]);
    free(ctx[0].nonce_array);
    free(ctx);
}

static void *pow_avx_get_pow_context(impl_context_t *impl_ctx,
                                     int8_t *trytes,
                                     int mwm,
                                     int threads)
{
    uv_mutex_lock(&impl_ctx->lock);
    for (int i = 0; i < impl_ctx->num_max_thread; i++) {
        if (impl_ctx->bitmap & (0x1 << i)) {
            impl_ctx->bitmap &= ~(0x1 << i);
            uv_mutex_unlock(&impl_ctx->lock);
            pow_avx_context_t *ctx =
                (pow_avx_context_t *) impl_ctx->context + i;
            memcpy(ctx->input_trytes, trytes, TRANSACTION_TRYTES_LENGTH);
            ctx->mwm = mwm;
            ctx->index_of_context = i;
            if (threads > 0 && threads < ctx->num_max_threads)
                ctx->num_threads = threads;
            else
                ctx->num_threads = ctx->num_max_threads;
            return ctx;
        }
    }
    uv_mutex_unlock(&impl_ctx->lock);
    return NULL; /* It should not happen */
}

static bool pow_avx_free_pow_context(impl_context_t *impl_ctx, void *pow_ctx)
{
    uv_mutex_lock(&impl_ctx->lock);
    impl_ctx->bitmap |= 0x1
                        << ((pow_avx_context_t *) pow_ctx)->index_of_context;
    uv_mutex_unlock(&impl_ctx->lock);
    return true;
}

static int8_t *pow_avx_get_pow_result(void *pow_ctx)
{
    int8_t *ret =
        (int8_t *) malloc(sizeof(int8_t) * (TRANSACTION_TRYTES_LENGTH));
    if (!ret)
        return NULL;
    memcpy(ret, ((pow_avx_context_t *) pow_ctx)->output_trytes,
           TRANSACTION_TRYTES_LENGTH);
    return ret;
}

static pow_info_t pow_avx_get_pow_info(void *pow_ctx)
{
    return ((pow_avx_context_t *) pow_ctx)->pow_info;
}

impl_context_t pow_avx_context = {
    .context = NULL,
    .description = "CPU (Intel AVX)",
    .bitmap = 0,
    .num_max_thread = 0,
    .num_working_thread = 0,
    .initialize = pow_avx_context_initialize,
    .destroy = pow_avx_context_destroy,
    .get_pow_context = pow_avx_get_pow_context,
    .free_pow_context = pow_avx_free_pow_context,
    .do_the_pow = pow_avx,
    .get_pow_result = pow_avx_get_pow_result,
    .get_pow_info = pow_avx_get_pow_info,
};
