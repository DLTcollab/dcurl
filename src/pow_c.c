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
#include "constants.h"
#include "cpu-utils.h"
#include "curl.h"

static pthread_mutex_t *pow_c_mutex;
static int *stopC;
static int64_t *countC;

static const int indices_[] = {
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

static const int indices[] = {
    0,   727, 725, 723, 721, 719, 717, 715, 713, 711, 709, 707, 705, 703, 701,
    699, 697, 695, 693, 691, 689, 687, 685, 683, 681, 679, 677, 675, 673, 671,
    669, 667, 665, 663, 661, 659, 657, 655, 653, 651, 649, 647, 645, 643, 641,
    639, 637, 635, 633, 631, 629, 627, 625, 623, 621, 619, 617, 615, 613, 611,
    609, 607, 605, 603, 601, 599, 597, 595, 593, 591, 589, 587, 585, 583, 581,
    579, 577, 575, 573, 571, 569, 567, 565, 563, 561, 559, 557, 555, 553, 551,
    549, 547, 545, 543, 541, 539, 537, 535, 533, 531, 529, 527, 525, 523, 521,
    519, 517, 515, 513, 511, 509, 507, 505, 503, 501, 499, 497, 495, 493, 491,
    489, 487, 485, 483, 481, 479, 477, 475, 473, 471, 469, 467, 465, 463, 461,
    459, 457, 455, 453, 451, 449, 447, 445, 443, 441, 439, 437, 435, 433, 431,
    429, 427, 425, 423, 421, 419, 417, 415, 413, 411, 409, 407, 405, 403, 401,
    399, 397, 395, 393, 391, 389, 387, 385, 383, 381, 379, 377, 375, 373, 371,
    369, 367, 365, 363, 361, 359, 357, 355, 353, 351, 349, 347, 345, 343, 341,
    339, 337, 335, 333, 331, 329, 327, 325, 323, 321, 319, 317, 315, 313, 311,
    309, 307, 305, 303, 301, 299, 297, 295, 293, 291, 289, 287, 285, 283, 281,
    279, 277, 275, 273, 271, 269, 267, 265, 263, 261, 259, 257, 255, 253, 251,
    249, 247, 245, 243, 241, 239, 237, 235, 233, 231, 229, 227, 225, 223, 221,
    219, 217, 215, 213, 211, 209, 207, 205, 203, 201, 199, 197, 195, 193, 191,
    189, 187, 185, 183, 181, 179, 177, 175, 173, 171, 169, 167, 165, 163, 161,
    159, 157, 155, 153, 151, 149, 147, 145, 143, 141, 139, 137, 135, 133, 131,
    129, 127, 125, 123, 121, 119, 117, 115, 113, 111, 109, 107, 105, 103, 101,
     99,  97,  95,  93,  91,  89,  87,  85,  83,  81,  79,  77,  75,  73,  71,
     69,  67,  65,  63,  61,  59,  57,  55,  53,  51,  49,  47,  45,  43,  41,
     39,  37,  35,  33,  31,  29,  27,  25,  23,  21,  19,  17,  15,  13,  11,
      9,   7,   5,   3,   1, 728, 726, 724, 722, 720, 718, 716, 714, 712, 710,
    708, 706, 704, 702, 700, 698, 696, 694, 692, 690, 688, 686, 684, 682, 680,
    678, 676, 674, 672, 670, 668, 666, 664, 662, 660, 658, 656, 654, 652, 650,
    648, 646, 644, 642, 640, 638, 636, 634, 632, 630, 628, 626, 624, 622, 620,
    618, 616, 614, 612, 610, 608, 606, 604, 602, 600, 598, 596, 594, 592, 590,
    588, 586, 584, 582, 580, 578, 576, 574, 572, 570, 568, 566, 564, 562, 560,
    558, 556, 554, 552, 550, 548, 546, 544, 542, 540, 538, 536, 534, 532, 530,
    528, 526, 524, 522, 520, 518, 516, 514, 512, 510, 508, 506, 504, 502, 500,
    498, 496, 494, 492, 490, 488, 486, 484, 482, 480, 478, 476, 474, 472, 470,
    468, 466, 464, 462, 460, 458, 456, 454, 452, 450, 448, 446, 444, 442, 440,
    438, 436, 434, 432, 430, 428, 426, 424, 422, 420, 418, 416, 414, 412, 410,
    408, 406, 404, 402, 400, 398, 396, 394, 392, 390, 388, 386, 384, 382, 380,
    378, 376, 374, 372, 370, 368, 366, 364, 362, 360, 358, 356, 354, 352, 350,
    348, 346, 344, 342, 340, 338, 336, 334, 332, 330, 328, 326, 324, 322, 320,
    318, 316, 314, 312, 310, 308, 306, 304, 302, 300, 298, 296, 294, 292, 290,
    288, 286, 284, 282, 280, 278, 276, 274, 272, 270, 268, 266, 264, 262, 260,
    258, 256, 254, 252, 250, 248, 246, 244, 242, 240, 238, 236, 234, 232, 230,
    228, 226, 224, 222, 220, 218, 216, 214, 212, 210, 208, 206, 204, 202, 200,
    198, 196, 194, 192, 190, 188, 186, 184, 182, 180, 178, 176, 174, 172, 170,
    168, 166, 164, 162, 160, 158, 156, 154, 152, 150, 148, 146, 144, 142, 140,
    138, 136, 134, 132, 130, 128, 126, 124, 122, 120, 118, 116, 114, 112, 110,
    108, 106, 104, 102, 100,  98,  96,  94,  92,  90,  88,  86,  84,  82,  80,
     78,  76,  74,  72,  70,  68,  66,  64,  62,  60,  58,  56,  54,  52,  50,
     48,  46,  44,  42,  40,  38,  36,  34,  32,  30,  28,  26,  24,  22,  20,
     18,  16,  14,  12,  10,   8,   6,   4,   2};

void transform64(uint64_t *lmid, uint64_t *hmid)
{
    uint64_t alpha, beta, gamma, delta;
    uint64_t *lfrom = lmid, *hfrom = hmid;
    uint64_t *lto = lmid + STATE_LENGTH, *hto = hmid + STATE_LENGTH;

    for (int r = 0; r < 80; r++) {
        for (int j = 0; j < STATE_LENGTH; j++) {
            int t2 = (j + 364) % 729;
            alpha = lfrom[j];
            beta = hfrom[j];
            gamma = hfrom[t2];
            delta = (alpha | ~gamma) & (lfrom[t2] ^ beta);
            lto[indices[j]] = ~delta;
            hto[indices[j]] = (alpha ^ gamma) | delta;
        }
        uint64_t *lswap = lfrom, *hswap = hfrom;
        lfrom = lto;
        hfrom = hto;
        lto = lswap;
        hto = hswap;
    }

    for (int j = 0; j < 1; j++) {
        int t2 = (j + 364) % 729;
        alpha = lfrom[j];
        beta = hfrom[j];
        gamma = hfrom[t2];
        delta = (alpha | ~gamma) & (lfrom[t2] ^ beta);
        lto[indices[j]] = ~delta;
        hto[indices[j]] = (alpha ^ gamma) | delta;
    }

    for (int j = 244, z = 608; j < 365; j++, z++) {
        int t2 = (j + 364) % 729;
        alpha = lfrom[j];
        beta = hfrom[j];
        gamma = hfrom[t2];
        int t3 = (z + 364) % 729;
        uint64_t alpha2 = lfrom[z];
        uint64_t beta2 = hfrom[z];
        uint64_t gamma2 = hfrom[t3];
        uint64_t delta2 = (alpha2 | ~gamma2) & (lfrom[t3] ^ beta2);
        delta = (alpha | ~gamma) & (lfrom[t2] ^ beta);
        lto[indices[j]] = ~delta;
        hto[indices[j]] = (alpha ^ gamma) | delta;
        lto[indices[z]] = ~delta2;
        hto[indices[z]] = (alpha2 ^ gamma2) | delta2;
    }
}

int incr(uint64_t *mid_low, uint64_t *mid_high)
{
    int i;
    uint64_t carry = 1;

    for (i = INCR_START; i < HASH_LENGTH && carry; i++) {
        uint64_t low = mid_low[i], high = mid_high[i];
        mid_low[i] = high ^ low;
        mid_high[i] = low;
        carry = high & (~low);
    }

    return i == HASH_LENGTH;
}

void seri(uint64_t *l, uint64_t *h, int n, int8_t *r)
{
    for (int i = HASH_LENGTH - NONCE_LENGTH; i < HASH_LENGTH; i++) {
        int ll = (l[i] >> n) & 1;
        int hh = (h[i] >> n) & 1;

        if (hh == 0 && ll == 1) {
            r[i - HASH_LENGTH + NONCE_LENGTH] = -1;
        }
        if (hh == 1 && ll == 1) {
            r[i - HASH_LENGTH + NONCE_LENGTH] = 0;
        }
        if (hh == 1 && ll == 0) {
            r[i - HASH_LENGTH + NONCE_LENGTH] = 1;
        }
    }
}

int check(uint64_t *l, uint64_t *h, int m)
{
    uint64_t nonce_probe = HBITS;

    for (int i = HASH_LENGTH - m; i < HASH_LENGTH; i++) {
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
                       int id)
{
    int n = 0;
    long long int i = 0;
    uint64_t lcpy[STATE_LENGTH * 2], hcpy[STATE_LENGTH * 2];

    for (i = 0; !incr(lmid, hmid) && !stopC[id]; i++) {
        memcpy(lcpy, lmid, STATE_LENGTH * sizeof(uint64_t));
        memcpy(hcpy, hmid, STATE_LENGTH * sizeof(uint64_t));
        transform64(lcpy, hcpy);
        if ((n = check(lcpy + STATE_LENGTH, hcpy + STATE_LENGTH, m)) >= 0) {
            seri(lmid, hmid, n, nonce);
            return i * 64;
        }
    }
    return -i * 64 + 1;
}

void para(int8_t in[], uint64_t l[], uint64_t h[])
{
    for (int i = 0; i < STATE_LENGTH; i++) {
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

static int64_t pwork(int8_t mid[], int mwm, int8_t nonce[], int n, int id)
{
    uint64_t lmid[STATE_LENGTH] = {0}, hmid[STATE_LENGTH] = {0};
    para(mid, lmid, hmid);
    int offset = HASH_LENGTH - NONCE_LENGTH;

    lmid[offset] = LOW0;
    hmid[offset] = HIGH0;
    lmid[offset + 1] = LOW1;
    hmid[offset + 1] = HIGH1;
    lmid[offset + 2] = LOW2;
    hmid[offset + 2] = HIGH2;
    lmid[offset + 3] = LOW3;
    hmid[offset + 3] = HIGH3;
    incrN(n, lmid, hmid);

    return loop_cpu(lmid, hmid, mwm, nonce, id);
}

static void *pworkThread(void *pitem)
{
    Pwork_struct *pworkInfo = (Pwork_struct *) pitem;
    int task_id = pworkInfo->index;
    pworkInfo->ret = pwork(pworkInfo->mid, pworkInfo->mwm, pworkInfo->nonce,
                           pworkInfo->n, task_id);

    pthread_mutex_lock(&pow_c_mutex[task_id]);
    if (pworkInfo->ret >= 0) {
        stopC[task_id] = 1;
        countC[task_id] += pworkInfo->ret;
        /* This means this thread got the result */
        pworkInfo->n = -1;
    } else {
        countC[task_id] += 1 - pworkInfo->ret;
    }
    pthread_mutex_unlock(&pow_c_mutex[task_id]);
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
    int8_t *rst = (int8_t *) malloc(rst_len + 1);
    if (!rst)
        return NULL;

    memcpy(rst, tx->data, tx->len - NonceTrinarySize / 3);
    memcpy(rst + tx->len - NonceTrinarySize / 3, nonce->data,
           rst_len - (tx->len - NonceTrinarySize / 3));
    rst[rst_len] = '\0';
    return rst;
}

static size_t nproc;

int pow_c_init(int num_task)
{
    pow_c_mutex =
        (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t) * num_task);
    stopC = (int *) malloc(sizeof(int) * num_task);
    countC = (int64_t *) malloc(sizeof(int64_t) * num_task);

    if (!pow_c_mutex || !stopC || !countC)
        return 0;

    nproc = get_avail_nprocs();
    return 1;
}

void pow_c_destroy()
{
    free(pow_c_mutex);
    free(stopC);
    free(countC);
}

int8_t *PowC(int8_t *trytes, int mwm, int index)
{
    stopC[index] = 0;
    countC[index] = 0;

    Trytes_t *trytes_t = initTrytes(trytes, 2673);

    int8_t *c_state = tx_to_cstate(trytes_t);
    if (!c_state)
        return NULL;

    pthread_t *threads = (pthread_t *) malloc(sizeof(pthread_t) * nproc);
    if (!threads)
        return NULL;

    Pwork_struct *pitem = (Pwork_struct *) malloc(sizeof(Pwork_struct) * nproc);
    if (!pitem)
        return NULL;

    /* Prepare nonce to each thread */
    int8_t **nonce_array = (int8_t **) malloc(sizeof(int8_t *) * nproc);
    if (!nonce_array)
        return NULL;

    /* init pthread mutex */
    pthread_mutex_init(&pow_c_mutex[index], NULL);

    for (int i = 0; i < nproc; i++) {
        pitem[i].mid = c_state;
        pitem[i].mwm = mwm;
        pitem[i].nonce = nonce_array[i] = (int8_t *) malloc(NonceTrinarySize);
        pitem[i].n = i;
        pitem[i].ret = 0;
        pitem[i].index = index;
        pthread_create(&threads[i], NULL, pworkThread, (void *) &pitem[i]);
    }

    int completedIndex = -1;
    for (int i = 0; i < nproc; i++) {
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
    for (int i = 0; i < nproc; i++) {
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
