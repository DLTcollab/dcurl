/*
 * Copyright (C) 2018 dcurl Developers.
 * Copyright (C) 2016 Shinya Yagyu.
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE file.
 */

#include "pow_sse.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include "curl.h"
#include "constants.h"
#include <stdint.h>

/* Required for get_nprocs_conf() on Linux */
#if defined(__linux__)
#include <sys/sysinfo.h>
#endif

/* On Mac OS X, define our own get_nprocs_conf() */
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/sysctl.h>
static unsigned int get_nprocs_conf()
{
    int numProcessors = 0;
    size_t size = sizeof(numProcessors);
    if (sysctlbyname("hw.ncpu", &numProcessors, &size, NULL, 0))
        return 1;
    return (unsigned int) numProcessors;
}
#define NPROCS
#endif

static pthread_mutex_t *pow_sse_mutex;
static int *stopSSE;
static long long int *countSSE;
static int DCURL_NUM_CPU = 0;

static const int indices[] = {
    0,   364, 728, 363, 727, 362, 726, 361, 725, 360, 724, 359, 723, 358, 722,
    357, 721, 356, 720, 355, 719, 354, 718, 353, 717, 352, 716, 351, 715, 350,
    714, 349, 713, 348, 712, 347, 711, 346, 710, 345, 709, 344, 708, 343, 707,
    342, 706, 341, 705, 340, 704, 339, 703, 338, 702, 337, 701, 336, 700, 335,
    699, 334, 698, 333, 697, 332, 696, 331, 695, 330, 694, 329, 693, 328, 692,
    327, 691, 326, 690, 325, 689, 324, 688, 323, 687, 322, 686, 321, 685, 320,
    684, 319, 683, 318, 682, 317, 681, 316, 680, 315, 679, 314, 678, 313, 677,
    312, 676, 311, 675, 310, 674, 309, 673, 308, 672, 307, 671, 306, 670, 305,
    669, 304, 668, 303, 667, 302, 666, 301, 665, 300, 664, 299, 663, 298, 662,
    297, 661, 296, 660, 295, 659, 294, 658, 293, 657, 292, 656, 291, 655, 290,
    654, 289, 653, 288, 652, 287, 651, 286, 650, 285, 649, 284, 648, 283, 647,
    282, 646, 281, 645, 280, 644, 279, 643, 278, 642, 277, 641, 276, 640, 275,
    639, 274, 638, 273, 637, 272, 636, 271, 635, 270, 634, 269, 633, 268, 632,
    267, 631, 266, 630, 265, 629, 264, 628, 263, 627, 262, 626, 261, 625, 260,
    624, 259, 623, 258, 622, 257, 621, 256, 620, 255, 619, 254, 618, 253, 617,
    252, 616, 251, 615, 250, 614, 249, 613, 248, 612, 247, 611, 246, 610, 245,
    609, 244, 608, 243, 607, 242, 606, 241, 605, 240, 604, 239, 603, 238, 602,
    237, 601, 236, 600, 235, 599, 234, 598, 233, 597, 232, 596, 231, 595, 230,
    594, 229, 593, 228, 592, 227, 591, 226, 590, 225, 589, 224, 588, 223, 587,
    222, 586, 221, 585, 220, 584, 219, 583, 218, 582, 217, 581, 216, 580, 215,
    579, 214, 578, 213, 577, 212, 576, 211, 575, 210, 574, 209, 573, 208, 572,
    207, 571, 206, 570, 205, 569, 204, 568, 203, 567, 202, 566, 201, 565, 200,
    564, 199, 563, 198, 562, 197, 561, 196, 560, 195, 559, 194, 558, 193, 557,
    192, 556, 191, 555, 190, 554, 189, 553, 188, 552, 187, 551, 186, 550, 185,
    549, 184, 548, 183, 547, 182, 546, 181, 545, 180, 544, 179, 543, 178, 542,
    177, 541, 176, 540, 175, 539, 174, 538, 173, 537, 172, 536, 171, 535, 170,
    534, 169, 533, 168, 532, 167, 531, 166, 530, 165, 529, 164, 528, 163, 527,
    162, 526, 161, 525, 160, 524, 159, 523, 158, 522, 157, 521, 156, 520, 155,
    519, 154, 518, 153, 517, 152, 516, 151, 515, 150, 514, 149, 513, 148, 512,
    147, 511, 146, 510, 145, 509, 144, 508, 143, 507, 142, 506, 141, 505, 140,
    504, 139, 503, 138, 502, 137, 501, 136, 500, 135, 499, 134, 498, 133, 497,
    132, 496, 131, 495, 130, 494, 129, 493, 128, 492, 127, 491, 126, 490, 125,
    489, 124, 488, 123, 487, 122, 486, 121, 485, 120, 484, 119, 483, 118, 482,
    117, 481, 116, 480, 115, 479, 114, 478, 113, 477, 112, 476, 111, 475, 110,
    474, 109, 473, 108, 472, 107, 471, 106, 470, 105, 469, 104, 468, 103, 467,
    102, 466, 101, 465, 100, 464, 99,  463, 98,  462, 97,  461, 96,  460, 95,
    459, 94,  458, 93,  457, 92,  456, 91,  455, 90,  454, 89,  453, 88,  452,
    87,  451, 86,  450, 85,  449, 84,  448, 83,  447, 82,  446, 81,  445, 80,
    444, 79,  443, 78,  442, 77,  441, 76,  440, 75,  439, 74,  438, 73,  437,
    72,  436, 71,  435, 70,  434, 69,  433, 68,  432, 67,  431, 66,  430, 65,
    429, 64,  428, 63,  427, 62,  426, 61,  425, 60,  424, 59,  423, 58,  422,
    57,  421, 56,  420, 55,  419, 54,  418, 53,  417, 52,  416, 51,  415, 50,
    414, 49,  413, 48,  412, 47,  411, 46,  410, 45,  409, 44,  408, 43,  407,
    42,  406, 41,  405, 40,  404, 39,  403, 38,  402, 37,  401, 36,  400, 35,
    399, 34,  398, 33,  397, 32,  396, 31,  395, 30,  394, 29,  393, 28,  392,
    27,  391, 26,  390, 25,  389, 24,  388, 23,  387, 22,  386, 21,  385, 20,
    384, 19,  383, 18,  382, 17,  381, 16,  380, 15,  379, 14,  378, 13,  377,
    12,  376, 11,  375, 10,  374, 9,   373, 8,   372, 7,   371, 6,   370, 5,
    369, 4,   368, 3,   367, 2,   366, 1,   365, 0};

static void transform128(__m128i *lmid, __m128i *hmid)
{
    int t1, t2;
    __m128i alpha, beta, gamma, delta;
    __m128i *lto = lmid + STATE_LENGTH, *hto = hmid + STATE_LENGTH;
    __m128i *lfrom = lmid, *hfrom = hmid;

    for (int r = 0; r < 80; r++) {
        for (int j = 0; j < STATE_LENGTH; j++) {
            t1 = indices[j];
            t2 = indices[j + 1];
            alpha = lfrom[t1];
            beta = hfrom[t1];
            gamma = hfrom[t2];
            delta = (alpha | (~gamma)) & (lfrom[t2] ^ beta);
            lto[j] = ~delta;
            hto[j] = (alpha ^ gamma) | delta;
        }
        __m128i *lswap = lfrom, *hswap = hfrom;
        lfrom = lto;
        hfrom = hto;
        lto = lswap;
        hto = hswap;
    }
    for (int j = 0; j < HASH_LENGTH; j++) {
        t1 = indices[j];
        t2 = indices[j + 1];
        alpha = lfrom[t1];
        beta = hfrom[t1];
        gamma = hfrom[t2];
        delta = (alpha | (~gamma)) & (lfrom[t2] ^ beta);
        lto[j] = ~delta;
        hto[j] = (alpha ^ gamma) | delta;
    }
}

static int incr128(__m128i *mid_low, __m128i *mid_high)
{
    int i;
    __m128i carry;
    carry = _mm_set_epi64x(LOW00, LOW01);

    for (i = INCR_START; i < HASH_LENGTH && (i == INCR_START || carry[0]);
         i++) {
        __m128i low = mid_low[i], high = mid_high[i];
        mid_low[i] = high ^ low;
        mid_high[i] = low;
        carry = high & (~low);
    }
    return i == HASH_LENGTH;
}

static void seri128(__m128i *low, __m128i *high, int n, int8_t *r)
{
    int index = 0;

    if (n > 63) {
        n -= 64;
        index = 1;
    }

    for (int i = HASH_LENGTH - NONCE_LENGTH; i < HASH_LENGTH; i++) {
        unsigned long long ll = (low[i][index] >> n) & 1;
        unsigned long long hh = (high[i][index] >> n) & 1;
        if (hh == 0 && ll == 1) {
            r[i + NONCE_LENGTH - HASH_LENGTH] = -1;
        }
        if (hh == 1 && ll == 1) {
            r[i + NONCE_LENGTH - HASH_LENGTH] = 0;
        }
        if (hh == 1 && ll == 0) {
            r[i + NONCE_LENGTH - HASH_LENGTH] = 1;
        }
    }
}

static int check128(__m128i *l, __m128i *h, int m)
{
    __m128i nonce_probe = _mm_set_epi64x(HBITS, HBITS);

    for (int i = HASH_LENGTH - m; i < HASH_LENGTH; i++) {
        nonce_probe &= ~(l[i] ^ h[i]);
        if (nonce_probe[0] == LBITS && nonce_probe[1] == LBITS) {
            return -1;
        }
    }
    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < 64; i++) {
            if ((nonce_probe[j] >> i) & 1) {
                return i + j * 64;
            }
        }
    }
    return -2;
}

static long long int loop128(__m128i *lmid,
                             __m128i *hmid,
                             int m,
                             int8_t *nonce,
                             int id)
{
    int n = 0;
    long long int i = 0;
    __m128i lcpy[STATE_LENGTH * 2], hcpy[STATE_LENGTH * 2];

    for (i = 0; !incr128(lmid, hmid) && !stopSSE[id]; i++) {
        for (int j = 0; j < STATE_LENGTH; j++) {
            lcpy[j] = lmid[j];
            hcpy[j] = hmid[j];
        }

        transform128(lcpy, hcpy);

        if ((n = check128(lcpy + STATE_LENGTH, hcpy + STATE_LENGTH, m)) >= 0) {
            seri128(lmid, hmid, n, nonce);
            return i * 128;
        }
    }
    return -i * 128 - 1;
}

static void para128(int8_t in[], __m128i l[], __m128i h[])
{
    for (int i = 0; i < STATE_LENGTH; i++) {
        switch (in[i]) {
        case 0:
            l[i] = _mm_set_epi64x(HBITS, HBITS);
            h[i] = _mm_set_epi64x(HBITS, HBITS);
            break;
        case 1:
            l[i] = _mm_set_epi64x(LBITS, LBITS);
            h[i] = _mm_set_epi64x(HBITS, HBITS);
            break;
        case -1:
            l[i] = _mm_set_epi64x(HBITS, HBITS);
            h[i] = _mm_set_epi64x(LBITS, LBITS);
            break;
        }
    }
}

static void incrN128(int n, __m128i *mid_low, __m128i *mid_high)
{
    for (int j = 0; j < n; j++) {
        __m128i carry = _mm_set_epi64x(HBITS, HBITS);
        for (int i = HASH_LENGTH * 2 / 3 + 4;
             i < HASH_LENGTH * 2 / 3 + 4 + 27 && carry[0]; i++) {
            __m128i low = mid_low[i], high = mid_high[i];
            mid_low[i] = high ^ low;
            mid_high[i] = low;
            carry = high & (~low);
        }
    }
}

static long long int pwork128(int8_t mid[],
                              int mwm,
                              int8_t nonce[],
                              int n,
                              int id)
{
    __m128i lmid[STATE_LENGTH], hmid[STATE_LENGTH];
    para128(mid, lmid, hmid);
    int offset = HASH_LENGTH - NONCE_LENGTH;

    lmid[offset] = _mm_set_epi64x(LOW00, LOW01);
    hmid[offset] = _mm_set_epi64x(HIGH00, HIGH01);
    lmid[offset + 1] = _mm_set_epi64x(LOW10, LOW11);
    hmid[offset + 1] = _mm_set_epi64x(HIGH10, HIGH11);
    lmid[offset + 2] = _mm_set_epi64x(LOW20, LOW21);
    hmid[offset + 2] = _mm_set_epi64x(HIGH20, HIGH21);
    lmid[offset + 3] = _mm_set_epi64x(LOW30, LOW31);
    hmid[offset + 3] = _mm_set_epi64x(HIGH30, HIGH31);
    lmid[offset + 4] = _mm_set_epi64x(LOW40, LOW41);
    hmid[offset + 4] = _mm_set_epi64x(HIGH40, HIGH41);
    incrN128(n, lmid, hmid);

    return loop128(lmid, hmid, mwm, nonce, id);
}

static void *pworkThread(void *pitem)
{
    Pwork_struct *pworkInfo = (Pwork_struct *) pitem;
    int task_id = pworkInfo->index;
    pworkInfo->ret = pwork128(pworkInfo->mid, pworkInfo->mwm, pworkInfo->nonce,
                              pworkInfo->n, task_id);

    pthread_mutex_lock(&pow_sse_mutex[task_id]);
    if (pworkInfo->ret >= 0) {
        stopSSE[task_id] = 1;
        countSSE[task_id] += pworkInfo->ret;
        /* This means this thread got the result */
        pworkInfo->n = -1;
    } else {
        countSSE[task_id] += 1 - pworkInfo->ret;
    }
    pthread_mutex_unlock(&pow_sse_mutex[task_id]);
    pthread_exit(NULL);
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

static int8_t *nonce_to_result(Trytes_t *tx, Trytes_t *nonce)
{
    int rst_len = tx->len - NonceTrinarySize / 3 + nonce->len;
    int8_t *rst = (int8_t *) malloc(rst_len);
    if (!rst)
        return NULL;

    memcpy(rst, tx->data, tx->len - NonceTrinarySize / 3);
    memcpy(rst + tx->len - NonceTrinarySize / 3, nonce->data,
           rst_len - (tx->len - NonceTrinarySize / 3));

    return rst;
}

int pow_sse_init(int num_task)
{
    pow_sse_mutex =
        (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t) * num_task);
    stopSSE = (int *) malloc(sizeof(int) * num_task);
    countSSE = (long long int *) malloc(sizeof(long long int) * num_task);

    if (!pow_sse_mutex || !stopSSE || !countSSE)
        return 0;

    char *env_num_cpu = getenv("DCURL_NUM_CPU");
    DCURL_NUM_CPU = get_nprocs_conf() - 1;
    if (env_num_cpu) {
        DCURL_NUM_CPU = atoi(env_num_cpu);
    }
    return 1;
}

void pow_sse_destroy()
{
    free(pow_sse_mutex);
    free(stopSSE);
    free(countSSE);
}

int8_t *PowSSE(int8_t *trytes, int mwm, int index)
{
    stopSSE[index] = 0;
    countSSE[index] = 0;

    Trytes_t *trytes_t = initTrytes(trytes, 2673);

    int8_t *c_state = tx_to_cstate(trytes_t);
    if (!c_state)
        return NULL;

    int num_cpu = DCURL_NUM_CPU;

    pthread_t *threads = (pthread_t *) malloc(sizeof(pthread_t) * num_cpu);
    if (!threads)
        return NULL;

    Pwork_struct *pitem =
        (Pwork_struct *) malloc(sizeof(Pwork_struct) * num_cpu);
    if (!pitem)
        return NULL;

    /* Prepare nonce to each thread */
    int8_t **nonce_array = (int8_t **) malloc(sizeof(int8_t *) * num_cpu);
    if (!nonce_array)
        return NULL;

    /* init pthread mutex */
    pthread_mutex_init(&pow_sse_mutex[index], NULL);

    for (int i = 0; i < num_cpu; i++) {
        pitem[i].mid = c_state;
        pitem[i].mwm = mwm;
        pitem[i].nonce = nonce_array[i] = (int8_t *) malloc(NonceTrinarySize);
        pitem[i].n = i;
        pitem[i].ret = 0;
        pitem[i].index = index;
        pthread_create(&threads[i], NULL, pworkThread, (void *) &pitem[i]);
    }

    int completedIndex = -1;
    for (int i = 0; i < num_cpu; i++) {
        pthread_join(threads[i], NULL);
        if (pitem[i].n == -1)
            completedIndex = i;
    }

    Trits_t *nonce_t = initTrits(nonce_array[completedIndex], NonceTrinarySize);
    if (!nonce_t)
        return NULL;

    Trytes_t *nonce = trytes_from_trits(nonce_t);
    if (!nonce)
        return NULL;

    int8_t *last_result = nonce_to_result(trytes_t, nonce);

    /* Free memory */
    free(c_state);
    for (int i = 0; i < num_cpu; i++) {
        free(nonce_array[i]);
    }
    free(nonce_array);
    free(threads);
    free(pitem);
    freeTrobject(trytes_t);
    freeTrobject(nonce_t);
    freeTrobject(nonce);

    return last_result;
}
