/*
 * Copyright (C) 2018 dcurl Developers.
 * Copyright (C) 2016 Shinya Yagyu.
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE file.
 */

#include "pow_avx.h"
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cpu-utils.h"
#include "curl.h"
#include "implcontext.h"

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

#ifdef __AVX2__
static void transform256(__m256i *lmid, __m256i *hmid)
{
    __m256i one = _mm256_set_epi64x(HBITS, HBITS, HBITS, HBITS);
    int t1, t2;
    __m256i alpha, beta, gamma, delta, ngamma;
    __m256i *lto = lmid + STATE_TRITS_LENGTH, *hto = hmid + STATE_TRITS_LENGTH;
    __m256i *lfrom = lmid, *hfrom = hmid;

    for (int r = 0; r < 80; r++) {
        for (int j = 0; j < STATE_TRITS_LENGTH; j++) {
            t1 = indices[j];
            t2 = indices[j + 1];

            alpha = lfrom[t1];
            beta = hfrom[t1];
            gamma = hfrom[t2];
            ngamma = _mm256_andnot_si256(gamma, one);
            delta = _mm256_and_si256(
                _mm256_or_si256(alpha, ngamma),
                _mm256_xor_si256(
                    lfrom[t2],
                    beta)); /* (alpha | (~gamma)) & (lfrom[t2] ^ beta) */


            lto[j] = _mm256_andnot_si256(delta, one); /* ~delta */
            hto[j] = _mm256_or_si256(_mm256_xor_si256(alpha, gamma),
                                     delta); /* (alpha ^ gamma) | delta */
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
        gamma = hfrom[t2];
        ngamma = _mm256_andnot_si256(gamma, one);
        delta = _mm256_and_si256(
            _mm256_or_si256(alpha, ngamma),
            _mm256_xor_si256(
                lfrom[t2], beta)); /* (alpha | (~gamma)) & (lfrom[t2] ^ beta) */
        lto[j] = _mm256_andnot_si256(delta, one); /* ~delta */
        hto[j] = _mm256_or_si256(_mm256_xor_si256(alpha, gamma),
                                 delta); /* (alpha ^ gamma) | delta */
    }
}

static int incr256(__m256i *mid_low, __m256i *mid_high)
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

static void seri256(__m256i *low, __m256i *high, int n, int8_t *r)
{
    int index = n >> 6;
    n = n % 64;

    for (int i = HASH_TRITS_LENGTH - NONCE_TRITS_LENGTH; i < HASH_TRITS_LENGTH; i++) {
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

static int check256(__m256i *l, __m256i *h, int m)
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

static void para256(int8_t in[], __m256i l[], __m256i h[])
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

static void incrN256(int n, __m256i *mid_low, __m256i *mid_high)
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


static int loop256(__m256i *lmid,
                   __m256i *hmid,
                   int m,
                   int8_t *nonce,
                   int *stopPoW)
{
    int i = 0, n = 0;
    __m256i lcpy[STATE_TRITS_LENGTH * 2], hcpy[STATE_TRITS_LENGTH * 2];

    for (i = 0; !incr256(lmid, hmid) && !*stopPoW; i++) {
        for (int j = 0; j < STATE_TRITS_LENGTH; j++) {
            lcpy[j] = lmid[j];
            hcpy[j] = hmid[j];
        }
        transform256(lcpy, hcpy);
        if ((n = check256(lcpy + STATE_TRITS_LENGTH, hcpy + STATE_TRITS_LENGTH, m)) >= 0) {
            seri256(lmid, hmid, n, nonce);
            return i * 256;
        }
    }
    return -i * 256 - 1;
}

static int64_t pwork256(int8_t mid[], int mwm, int8_t nonce[], int n,
                        int *stopPoW)
{
    __m256i lmid[STATE_TRITS_LENGTH], hmid[STATE_TRITS_LENGTH];
    int offset = HASH_TRITS_LENGTH - NONCE_TRITS_LENGTH;
    para256(mid, lmid, hmid);
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
    incrN256(n, lmid, hmid);

    return loop256(lmid, hmid, mwm, nonce, stopPoW);
}
#else
void transform256(__m256d *lmid, __m256d *hmid)
{
    __m256d one = _mm256_set_pd(HBITS, HBITS, HBITS, HBITS);
    int j, r, t1, t2;
    __m256d alpha, beta, gamma, delta, ngamma;
    __m256d *lto = lmid + STATE_TRITS_LENGTH, *hto = hmid + STATE_TRITS_LENGTH;
    __m256d *lfrom = lmid, *hfrom = hmid;
    for (r = 0; r < 80; r++) {
        for (j = 0; j < STATE_TRITS_LENGTH; j++) {
            t1 = indices___[j];
            t2 = indices___[j + 1];

            alpha = lfrom[t1];
            beta = hfrom[t1];
            gamma = hfrom[t2];
            ngamma = _mm256_andnot_pd(gamma, one);
            delta = _mm256_and_pd(
                _mm256_or_pd(alpha, ngamma),
                _mm256_xor_pd(
                    lfrom[t2],
                    beta));  //(alpha | (~gamma)) & (lfrom[t2] ^ beta);


            lto[j] = _mm256_andnot_pd(delta, one);  //~delta;
            hto[j] = _mm256_or_pd(_mm256_xor_pd(alpha, gamma),
                                  delta);  //(alpha ^ gamma) | delta;
        }
        __m256d *lswap = lfrom, *hswap = hfrom;
        lfrom = lto;
        hfrom = hto;
        lto = lswap;
        hto = hswap;
    }
    for (j = 0; j < HASH_TRITS_LENGTH; j++) {
        t1 = indices___[j];
        t2 = indices___[j + 1];

        alpha = lfrom[t1];
        beta = hfrom[t1];
        gamma = hfrom[t2];
        ngamma = _mm256_andnot_pd(gamma, one);
        delta = _mm256_and_pd(
            _mm256_or_pd(alpha, ngamma),
            _mm256_xor_pd(lfrom[t2],
                          beta));  //(alpha | (~gamma)) & (lfrom[t2] ^ beta);


        lto[j] = _mm256_andnot_pd(delta, one);  //~delta;
        hto[j] = _mm256_or_pd(_mm256_xor_pd(alpha, gamma),
                              delta);  //(alpha ^ gamma) | delta;
    }
}

int incr256(__m256d *mid_low, __m256d *mid_high)
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

void seri256(__m256d *low, __m256d *high, int n, int8_t *r)
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
    for (i = HASH_TRITS_LENGTH - NONCE_TRITS_LENGTH; i < HASH_TRITS_LENGTH; i++) {
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

int check256(__m256d *l, __m256d *h, int m)
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

void para256(int8_t in[], __m256d l[], __m256d h[])
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

void incrN256(int n, __m256d *mid_low, __m256d *mid_high)
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

int loop256(__m256d *lmid, __m256d *hmid, int m, int8_t *nonce,
            int *stopPoW)
{
    int i = 0, n = 0, j = 0;

    __m256d lcpy[STATE_TRITS_LENGTH * 2], hcpy[STATE_TRITS_LENGTH * 2];
    for (i = 0; !incr256(lmid, hmid) && !*stopPoW; i++) {
        for (j = 0; j < STATE_TRITS_LENGTH; j++) {
            lcpy[j] = lmid[j];
            hcpy[j] = hmid[j];
        }
        transform256(lcpy, hcpy);
        if ((n = check256(lcpy + STATE_TRITS_LENGTH, hcpy + STATE_TRITS_LENGTH, m)) >= 0) {
            seri256(lmid, hmid, n, nonce);
            return i * 256;
        }
    }
    return -i * 256 - 1;
}

long long int pwork256(int8_t mid[], int mwm, int8_t nonce[], int n,
                       int *stopPoW)
{
    __m256d lmid[STATE_TRITS_LENGTH], hmid[STATE_TRITS_LENGTH];
    int offset = HASH_TRITS_LENGTH - NONCE_TRITS_LENGTH;
    para256(mid, lmid, hmid);
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
    incrN256(n, lmid, hmid);

    return loop256(lmid, hmid, mwm, nonce, stopPoW);
}
#endif

static void *pworkThread(void *pitem)
{
    Pwork_struct *pworkInfo = (Pwork_struct *) pitem;
    pworkInfo->ret = pwork256(pworkInfo->mid, pworkInfo->mwm,
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

bool PowAVX(void *pow_ctx)
{
    bool res = true;
    Trits_t *nonce_trit = NULL;
    Trytes_t *tx_tryte = NULL, *nonce_tryte = NULL;
    time_t start_time, end_time;

    /* Initialize the context */
    PoW_AVX_Context *ctx = (PoW_AVX_Context *) pow_ctx;
    ctx->stopPoW = 0;
    ctx->pow_info->time = 0;
    ctx->pow_info->hash_count = 0;
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

    time(&start_time);
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
        ctx->pow_info->hash_count += (uint64_t) (pitem[i].ret >= 0 ? pitem[i].ret : -pitem[i].ret + 1);
    }
    time(&end_time);
    ctx->pow_info->time = difftime(end_time, start_time);

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

static bool PoWAVX_Context_Initialize(ImplContext *impl_ctx)
{
    int nproc = get_avail_nprocs();
    if (impl_ctx->num_max_thread <= 0 || nproc <= 0) return false;

    PoW_AVX_Context *ctx = (PoW_AVX_Context *) malloc(sizeof(PoW_AVX_Context) * impl_ctx->num_max_thread);
    if (!ctx) return false;

    /* Pre-allocate Memory Chunk for each field */
    void *threads_chunk = malloc(impl_ctx->num_max_thread * sizeof(pthread_t) * nproc);
    void *pitem_chunk = malloc(impl_ctx->num_max_thread * sizeof(Pwork_struct) * nproc);
    void *nonce_ptr_chunk = malloc(impl_ctx->num_max_thread * sizeof(int8_t *) * nproc);
    void *nonce_chunk = malloc(impl_ctx->num_max_thread * NONCE_TRITS_LENGTH * nproc);
    void *pow_info_chunk = malloc(impl_ctx->num_max_thread * sizeof(PoW_Info));
    if (!threads_chunk || !pitem_chunk || !nonce_ptr_chunk || !nonce_chunk || !pow_info_chunk) goto fail;

    for (int i = 0; i < impl_ctx->num_max_thread; i++) {
        ctx[i].threads = (pthread_t *) (threads_chunk + i * sizeof(pthread_t) * nproc);
        ctx[i].pitem = (Pwork_struct *) (pitem_chunk + i * sizeof(Pwork_struct) * nproc);
        ctx[i].nonce_array = (int8_t **) (nonce_ptr_chunk + i * sizeof(int8_t *) * nproc);
        for (int j = 0; j < nproc; j++)
            ctx[i].nonce_array[j] = (int8_t *) (nonce_chunk + i * NONCE_TRITS_LENGTH * nproc +
                                                j * NONCE_TRITS_LENGTH);
        ctx[i].pow_info = (PoW_Info *) (pow_info_chunk + i * sizeof(PoW_Info));
        ctx[i].num_threads = nproc;
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
    free(pow_info_chunk);
    return false;
}

static void PoWAVX_Context_Destroy(ImplContext *impl_ctx)
{
    PoW_AVX_Context *ctx = (PoW_AVX_Context *) impl_ctx->context;
    free(ctx[0].threads);
    free(ctx[0].pitem);
    free(ctx[0].nonce_array[0]);
    free(ctx[0].nonce_array);
    free(ctx[0].pow_info);
    free(ctx);
}

static void *PoWAVX_getPoWContext(ImplContext *impl_ctx, int8_t *trytes, int mwm)
{
    pthread_mutex_lock(&impl_ctx->lock);
    for (int i = 0; i < impl_ctx->num_max_thread; i++) {
        if (impl_ctx->bitmap & (0x1 << i)) {
            impl_ctx->bitmap &= ~(0x1 << i);
            pthread_mutex_unlock(&impl_ctx->lock);
            PoW_AVX_Context *ctx = impl_ctx->context + sizeof(PoW_AVX_Context) * i;
            memcpy(ctx->input_trytes, trytes, TRANSACTION_TRYTES_LENGTH);
            ctx->mwm = mwm;
            ctx->indexOfContext = i;
            return ctx;
        }
    }
    pthread_mutex_unlock(&impl_ctx->lock);
    return NULL; /* It should not happen */
}

static bool PoWAVX_freePoWContext(ImplContext *impl_ctx, void *pow_ctx)
{
    pthread_mutex_lock(&impl_ctx->lock);
    impl_ctx->bitmap |= 0x1 << ((PoW_AVX_Context *) pow_ctx)->indexOfContext;
    pthread_mutex_unlock(&impl_ctx->lock);
    return true;
}

static int8_t *PoWAVX_getPoWResult(void *pow_ctx)
{
    int8_t *ret = (int8_t *) malloc(sizeof(int8_t) * (TRANSACTION_TRYTES_LENGTH));
    if (!ret) return NULL;
    memcpy(ret, ((PoW_AVX_Context *) pow_ctx)->output_trytes, TRANSACTION_TRYTES_LENGTH);
    return ret;
}

static void *PoWAVX_getPoWInfo(void *pow_ctx)
{
    return ((PoW_AVX_Context *) pow_ctx)->pow_info;
}

ImplContext PoWAVX_Context = {
    .context = NULL,
    .description = "CPU (Intel AVX)",
    .bitmap = 0,
    .num_max_thread = 2,
    .num_working_thread = 0,
    .initialize = PoWAVX_Context_Initialize,
    .destroy = PoWAVX_Context_Destroy,
    .getPoWContext = PoWAVX_getPoWContext,
    .freePoWContext = PoWAVX_freePoWContext,
    .doThePoW = PowAVX,
    .getPoWResult = PoWAVX_getPoWResult,
    .getPoWInfo = PoWAVX_getPoWInfo,
};
