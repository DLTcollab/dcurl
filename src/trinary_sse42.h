/*
 * Copyright (C) 2019 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#ifndef TRINARY_SSE42_H_
#define TRINARY_SSE42_H_

#if defined(__SSE4_2__)
#include <nmmintrin.h>
#endif
#include "constants.h"
#if defined(__ARM_NEON)
#include "sse2neon.h"
#endif
#if !defined(__SSE4_2__) && !defined(__ARM_NEON)
#error "The hardware architecture should support SSE4.2 or NEON instruction."
#endif

#define BLOCK_8BIT(type) (sizeof(type) / sizeof(int8_t))
#define BYTE_OF_128BIT 16
#define COMMA0
#define COMMA1 ,
#define COMMA(x) COMMA##x
#define INDEX_3DIFF_0F 0x00, 0x03, 0x06, 0x09, 0x0C, 0x0F
#define INDEX_3DIFF_1D 0x01, 0x04, 0x07, 0x0A, 0x0D
#define INDEX_3DIFF_2E 0x02, 0x05, 0x08, 0x0B, 0x0E
#define REPEAT0(str)
#define REPEAT1(str) str
#define REPEAT2(str) REPEAT1(str), str
#define REPEAT3(str) REPEAT2(str), str
#define REPEAT4(str) REPEAT3(str), str
#define REPEAT5(str) REPEAT4(str), str
#define REPEAT6(str) REPEAT5(str), str
#define REPEAT7(str) REPEAT6(str), str
#define REPEAT8(str) REPEAT7(str), str
#define REPEAT9(str) REPEAT8(str), str
#define REPEAT10(str) REPEAT9(str), str
#define REPEAT11(str) REPEAT10(str), str
#define REPEAT(n, str) REPEAT##n(str)

static inline bool validateTrits_sse42(Trobject_t *trits)
{
    const int block_8bit = BLOCK_8BIT(__m128i);
    const int posOneElement = 0x01010101;
    const int negOneElement = 0xFFFFFFFF;
    const __m128i posOne = _mm_set_epi32(posOneElement, posOneElement,
                                         posOneElement, posOneElement);
    const __m128i negOne = _mm_set_epi32(negOneElement, negOneElement,
                                         negOneElement, negOneElement);
    /* The for loop handles the group of the 128-bit characters without the
     * end-of-string */
    for (int i = 0; i < (trits->len) / block_8bit; i++) {
        __m128i data = _mm_loadu_si128((__m128i *) (trits->data) + i);
        __m128i result = _mm_or_si128(
            /* > 1 */
            _mm_cmpgt_epi8(data, posOne),
            /* < -1 */
            _mm_cmplt_epi8(data, negOne));
        int notValid = !_mm_test_all_zeros(result, result);
        if (notValid)
            return false;
    }
    /* The for loop handles the rest of the characters until the end-of-string
     */
    for (int i = ((trits->len) / block_8bit) * block_8bit; i < trits->len;
         i++) {
        if (trits->data[i] < -1 || trits->data[i] > 1)
            return false;
    }
    return true;
}

#if defined(__SSE4_2__)
static inline bool validateTrytes_sse42(Trobject_t *trytes)
{
    const int block_8bit = BLOCK_8BIT(__m128i);
    /* Characters from 'A' to 'Z' and '9' to '9' */
    const char range[BYTE_OF_128BIT] = "AZ99";
    __m128i pattern = _mm_loadu_si128((__m128i *) (range));
    /* The for loop handles the group of the 128-bit characters without the
     * end-of-string */
    for (int i = 0; i < (trytes->len) / block_8bit; i++) {
        __m128i src = _mm_loadu_si128((__m128i *) (trytes->data) + i);
        /* Check whether the characters are in the defined range or not
         * Return 0 if all the characters are in the range, otherwise return 1
         */
        int notValid = _mm_cmpistrc(pattern, src,
                                    /* Signed byte comparison */
                                    _SIDD_SBYTE_OPS |
                                        /* Compare with the character range */
                                        _SIDD_CMP_RANGES |
                                        /* Negate the comparison result */
                                        _SIDD_MASKED_NEGATIVE_POLARITY);

        if (notValid)
            return false;
    }
    /* The for loop handles the rest of the characters until the end-of-string
     */
    for (int i = ((trytes->len) / block_8bit) * block_8bit; i < trytes->len;
         i++) {
        if ((trytes->data[i] < 'A' || trytes->data[i] > 'Z') &&
            trytes->data[i] != '9')
            return false;
    }
    return true;
}

static inline Trobject_t *trytes_from_trits_sse42(Trobject_t *trits)
{
    Trobject_t *trytes = NULL;
    int8_t *src = (int8_t *) malloc(trits->len / 3);

    const int block_8bit = BLOCK_8BIT(__m128i);
    const int8_t setMSB = 0x80;
    const __m128i tryteAlphabet[2] = {
        _mm_setr_epi8(TryteAlphabet[0], TryteAlphabet[1], TryteAlphabet[2],
                      TryteAlphabet[3], TryteAlphabet[4], TryteAlphabet[5],
                      TryteAlphabet[6], TryteAlphabet[7], TryteAlphabet[8],
                      TryteAlphabet[9], TryteAlphabet[10], TryteAlphabet[11],
                      TryteAlphabet[12], TryteAlphabet[13], TryteAlphabet[14],
                      TryteAlphabet[15]),
        _mm_setr_epi8(TryteAlphabet[16], TryteAlphabet[17], TryteAlphabet[18],
                      TryteAlphabet[19], TryteAlphabet[20], TryteAlphabet[21],
                      TryteAlphabet[22], TryteAlphabet[23], TryteAlphabet[24],
                      TryteAlphabet[25], TryteAlphabet[26], 0, 0, 0, 0, 0)};
    /* For shuffling the bytes of the input trits */
    const __m128i shuffleLow[3] = {
        _mm_setr_epi8(REPEAT(0, setMSB) COMMA(0) INDEX_3DIFF_0F COMMA(1)
                          REPEAT(10, setMSB)),
        _mm_setr_epi8(REPEAT(6, setMSB) COMMA(1) INDEX_3DIFF_2E COMMA(1)
                          REPEAT(5, setMSB)),
        _mm_setr_epi8(REPEAT(11, setMSB) COMMA(1) INDEX_3DIFF_1D COMMA(0)
                          REPEAT(0, setMSB))};
    const __m128i shuffleMid[3] = {
        _mm_setr_epi8(REPEAT(0, setMSB) COMMA(0) INDEX_3DIFF_1D COMMA(1)
                          REPEAT(11, setMSB)),
        _mm_setr_epi8(REPEAT(5, setMSB) COMMA(1) INDEX_3DIFF_0F COMMA(1)
                          REPEAT(5, setMSB)),
        _mm_setr_epi8(REPEAT(11, setMSB) COMMA(1) INDEX_3DIFF_2E COMMA(0)
                          REPEAT(0, setMSB))};
    const __m128i shuffleHigh[3] = {
        _mm_setr_epi8(REPEAT(0, setMSB) COMMA(0) INDEX_3DIFF_2E COMMA(1)
                          REPEAT(11, setMSB)),
        _mm_setr_epi8(REPEAT(5, setMSB) COMMA(1) INDEX_3DIFF_1D COMMA(1)
                          REPEAT(6, setMSB)),
        _mm_setr_epi8(REPEAT(10, setMSB) COMMA(1) INDEX_3DIFF_0F COMMA(0)
                          REPEAT(0, setMSB))};
    /* The mask with interleaved bytes of 0xFF and 0x00 */
    const __m128i byteInterMask =
        _mm_set_epi32(0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00);

    /* Start converting */
    for (int i = 0; i < trits->len / 3 / block_8bit; i++) {
        /* Get trit data */
        __m128i dataFirst = _mm_loadu_si128((__m128i *) (trits->data) + i * 3);
        __m128i dataMid =
            _mm_loadu_si128((__m128i *) (trits->data) + i * 3 + 1);
        __m128i dataLast =
            _mm_loadu_si128((__m128i *) (trits->data) + i * 3 + 2);
        /*
         * Each block represents a trit.
         *                                           shuffle
         *             ----------------        ------                  ------     ------     ------
         * dataFirst = | a1 | a2 | a3 | ...... | f1 |       lowTrit  = | a1 | ... | f1 | ... | p1 |
         *             ----------------        ------                  ------     ------     ------
         *             ----------------        ------                  ------     ------     ------
         * dataMid   = | f2 | f3 | g1 | ...... | k2 |  =>   midTrit  = | a2 | ... | f2 | ... | p2 |
         *             ----------------        ------                  ------     ------     ------
         *             ----------------        ------                  ------     ------     ------
         * dataLast  = | k3 | l1 | l2 | ...... | p3 |       highTrit = | a3 | ... | f3 | ... | p3 |
         *             ----------------        ------                  ------     ------     ------
         */
        __m128i lowTrit = _mm_or_si128(
            _mm_shuffle_epi8(dataFirst, shuffleLow[0]),
            _mm_or_si128(_mm_shuffle_epi8(dataMid, shuffleLow[1]),
                         _mm_shuffle_epi8(dataLast, shuffleLow[2])));
        __m128i midTrit = _mm_or_si128(
            _mm_shuffle_epi8(dataFirst, shuffleMid[0]),
            _mm_or_si128(_mm_shuffle_epi8(dataMid, shuffleMid[1]),
                         _mm_shuffle_epi8(dataLast, shuffleMid[2])));
        __m128i highTrit = _mm_or_si128(
            _mm_shuffle_epi8(dataFirst, shuffleHigh[0]),
            _mm_or_si128(_mm_shuffle_epi8(dataMid, shuffleHigh[1]),
                         _mm_shuffle_epi8(dataLast, shuffleHigh[2])));
        /* lowResult = (lowTrit) */
        __m128i lowResult = lowTrit;
        /* midResult = (midTrit * 3) */
        __m128i midResult = _mm_or_si128(
            _mm_and_si128(
                byteInterMask,
                _mm_mullo_epi16(_mm_and_si128(midTrit, byteInterMask),
                                _mm_set_epi16(0x0003, 0x0003, 0x0003, 0x0003,
                                              0x0003, 0x0003, 0x0003, 0x0003))),
            _mm_andnot_si128(
                byteInterMask,
                _mm_mullo_epi16(
                    _mm_and_si128(midTrit, ~byteInterMask),
                    _mm_set_epi16(0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
                                  0x0003, 0x0003, 0x0003))));
        /* highResult = (highTrit * 9) */
        __m128i highResult = _mm_or_si128(
            _mm_and_si128(
                byteInterMask,
                _mm_mullo_epi16(_mm_and_si128(highTrit, byteInterMask),
                                _mm_set_epi16(0x0009, 0x0009, 0x0009, 0x0009,
                                              0x0009, 0x0009, 0x0009, 0x0009))),
            _mm_andnot_si128(
                byteInterMask,
                _mm_mullo_epi16(
                    _mm_and_si128(highTrit, ~byteInterMask),
                    _mm_set_epi16(0x0009, 0x0009, 0x0009, 0x0009, 0x0009,
                                  0x0009, 0x0009, 0x0009))));
        /* alphabetOffset = (lowResult + midResult + highResult) */
        __m128i alphabetOffset =
            _mm_add_epi8(lowResult, _mm_add_epi8(midResult, highResult));
        /* Check whether the offset is < 0 */
        __m128i tmpMask =
            _mm_cmplt_epi8(alphabetOffset, _mm_set_epi32(0, 0, 0, 0));
        /* If the offset is < 0, then offset += 27 */
        __m128i alphabetOffsetAdd = _mm_add_epi8(
            alphabetOffset,
            _mm_set_epi32(0x1B1B1B1B, 0x1B1B1B1B, 0x1B1B1B1B, 0x1B1B1B1B));
        alphabetOffset =
            _mm_or_si128(_mm_and_si128(tmpMask, alphabetOffsetAdd),
                         _mm_andnot_si128(tmpMask, alphabetOffset));

        /* Assign tryte alphabet */
        /* If the offset is >= 16 (> 15), then the compared result byte = 0xFF,
         * else = 0x00 */
        __m128i cmpResult = _mm_cmpgt_epi8(
            alphabetOffset, _mm_set_epi8(15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
                                         15, 15, 15, 15, 15, 15));
        /* Use the offset to get the correct tryte alphabet from tryteAlphabet[]
         */
        __m128i resultLt = _mm_shuffle_epi8(tryteAlphabet[0], alphabetOffset);
        __m128i resultGe = _mm_shuffle_epi8(
            tryteAlphabet[1],
            /* alphabetOffset - 16 */
            _mm_sub_epi8(alphabetOffset,
                         _mm_set_epi8(16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
                                      16, 16, 16, 16, 16, 16)));
        __m128i result = _mm_or_si128(_mm_andnot_si128(cmpResult, resultLt),
                                      _mm_and_si128(cmpResult, resultGe));
        /* Store the tryte result */
        _mm_store_si128((__m128i *) (src + i * block_8bit), result);
    }
    for (int i = ((trits->len) / 3 / block_8bit) * block_8bit;
         i < trits->len / 3; i++) {
        int j = trits->data[i * 3] + trits->data[i * 3 + 1] * 3 +
                trits->data[i * 3 + 2] * 9;

        if (j < 0)
            j += 27;
        src[i] = TryteAlphabet[j];
    }

    trytes = initTrytes(src, trits->len / 3);
    free(src);

    return trytes;
}

static inline Trobject_t *trits_from_trytes_sse42(Trobject_t *trytes)
{
    Trobject_t *trits = NULL;
    int8_t *src = (int8_t *) malloc(trytes->len * 3);

    const int block_8bit = BLOCK_8BIT(__m128i);
    /* For setting the most significant bit of a byte */
    const int8_t setMSB = 0x80;
    static int8_t TrytesToTritsMappings[][3] = {
        {0, 0, 0},   {1, 0, 0},   {-1, 1, 0},  {0, 1, 0},   {1, 1, 0},
        {-1, -1, 1}, {0, -1, 1},  {1, -1, 1},  {-1, 0, 1},  {0, 0, 1},
        {1, 0, 1},   {-1, 1, 1},  {0, 1, 1},   {1, 1, 1},   {-1, -1, -1},
        {0, -1, -1}, {1, -1, -1}, {-1, 0, -1}, {0, 0, -1},  {1, 0, -1},
        {-1, 1, -1}, {0, 1, -1},  {1, 1, -1},  {-1, -1, 0}, {0, -1, 0},
        {1, -1, 0},  {-1, 0, 0}};
    /* The set and range for indicating the trits value (0, 1, -1)
     * of the corresponding trytes */
    /* '9', 'C', 'F', 'I', 'L', 'O', 'R', 'U', 'X' */
    const char setLowTrit0[BYTE_OF_128BIT] = "9CFILORUX";
    /* 'A', 'D', 'G', 'J', 'M', 'P', 'S', 'V', 'Y' */
    const char setLowTritP1[BYTE_OF_128BIT] = "ADGJMPSVY";
    /* 'B', 'E', 'H', 'K', 'N', 'Q', 'T', 'W', 'Z' */
    const char setLowTritN1[BYTE_OF_128BIT] = "BEHKNQTWZ";
    /* '9', 'A', 'H', 'I', 'J', 'Q', 'R', 'S', 'Z' */
    const char rangeMidTrit0[BYTE_OF_128BIT] = "99AAHJQSZZ";
    /* 'B', 'C', 'D', 'K', 'L', 'M', 'T', 'U', 'V' */
    const char rangeMidTritP1[BYTE_OF_128BIT] = "BDKMTV";
    /* 'E', 'F', 'G', 'N', 'O', 'P', 'W', 'X', 'Y' */
    const char rangeMidTritN1[BYTE_OF_128BIT] = "EGNPWY";
    /* '9', 'A', 'B', 'C', 'D', 'W', 'X', 'Y', 'Z' */
    const char rangeHighTrit0[BYTE_OF_128BIT] = "99ADWZ";
    /* 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M' */
    const char rangeHighTritP1[BYTE_OF_128BIT] = "EM";
    /* 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V' */
    const char rangeHighTritN1[BYTE_OF_128BIT] = "NV";
    /* Convert the char array to the 128-bit data */
    const __m128i patternLowTrit0 = _mm_loadu_si128((__m128i *) (setLowTrit0));
    const __m128i patternLowTritP1 =
        _mm_loadu_si128((__m128i *) (setLowTritP1));
    const __m128i patternLowTritN1 =
        _mm_loadu_si128((__m128i *) (setLowTritN1));
    const __m128i patternMidTrit0 =
        _mm_loadu_si128((__m128i *) (rangeMidTrit0));
    const __m128i patternMidTritP1 =
        _mm_loadu_si128((__m128i *) (rangeMidTritP1));
    const __m128i patternMidTritN1 =
        _mm_loadu_si128((__m128i *) (rangeMidTritN1));
    const __m128i patternHighTrit0 =
        _mm_loadu_si128((__m128i *) (rangeHighTrit0));
    const __m128i patternHighTritP1 =
        _mm_loadu_si128((__m128i *) (rangeHighTritP1));
    const __m128i patternHighTritN1 =
        _mm_loadu_si128((__m128i *) (rangeHighTritN1));
    /* The 128-bit data with the repeated same bytes */
    const __m128i posOne = _mm_set1_epi8(1);
    const __m128i negOne = _mm_set1_epi8(-1);
    const __m128i zero = _mm_set1_epi8(0);
    /* For shuffling the bytes of the trits transformed from the input trytes */
    const __m128i shuffleFirst[3] = {
        _mm_setr_epi8(0x00, REPEAT2(setMSB), 0x01, REPEAT2(setMSB), 0x02,
                      REPEAT2(setMSB), 0x03, REPEAT2(setMSB), 0x04,
                      REPEAT2(setMSB), 0x05),
        _mm_setr_epi8(REPEAT1(setMSB), 0x00, REPEAT2(setMSB), 0x01,
                      REPEAT2(setMSB), 0x02, REPEAT2(setMSB), 0x03,
                      REPEAT2(setMSB), 0x04, REPEAT2(setMSB)),
        _mm_setr_epi8(REPEAT2(setMSB), 0x00, REPEAT2(setMSB), 0x01,
                      REPEAT2(setMSB), 0x02, REPEAT2(setMSB), 0x03,
                      REPEAT2(setMSB), 0x04, REPEAT1(setMSB))};
    const __m128i shuffleMid[3] = {
        _mm_setr_epi8(REPEAT2(setMSB), 0x06, REPEAT2(setMSB), 0x07,
                      REPEAT2(setMSB), 0x08, REPEAT2(setMSB), 0x09,
                      REPEAT2(setMSB), 0x0A, REPEAT1(setMSB)),
        _mm_setr_epi8(0x05, REPEAT2(setMSB), 0x06, REPEAT2(setMSB), 0x07,
                      REPEAT2(setMSB), 0x08, REPEAT2(setMSB), 0x09,
                      REPEAT2(setMSB), 0x0A),
        _mm_setr_epi8(REPEAT1(setMSB), 0x05, REPEAT2(setMSB), 0x06,
                      REPEAT2(setMSB), 0x07, REPEAT2(setMSB), 0x08,
                      REPEAT2(setMSB), 0x09, REPEAT2(setMSB))};
    const __m128i shuffleLast[3] = {
        _mm_setr_epi8(REPEAT1(setMSB), 0x0B, REPEAT2(setMSB), 0x0C,
                      REPEAT2(setMSB), 0x0D, REPEAT2(setMSB), 0x0E,
                      REPEAT2(setMSB), 0x0F, REPEAT2(setMSB)),
        _mm_setr_epi8(REPEAT2(setMSB), 0x0B, REPEAT2(setMSB), 0x0C,
                      REPEAT2(setMSB), 0x0D, REPEAT2(setMSB), 0x0E,
                      REPEAT2(setMSB), 0x0F, REPEAT1(setMSB)),
        _mm_setr_epi8(0x0A, REPEAT2(setMSB), 0x0B, REPEAT2(setMSB), 0x0C,
                      REPEAT2(setMSB), 0x0D, REPEAT2(setMSB), 0x0E,
                      REPEAT2(setMSB), 0x0F)};

    /* Start converting */
    /* The for loop handles the group of the 128-bit characters without the
     * end-of-string */
    for (int i = 0; i < (trytes->len) / block_8bit; i++) {
        /* Get tryte data */
        __m128i data = _mm_loadu_si128((__m128i *) (trytes->data) + i);

        /* The masks for setting the corresponding trits */
        __m128i maskLowTrit0 = _mm_cmpistrm(
            patternLowTrit0, data,
            /* Signed byte comparison */
            _SIDD_SBYTE_OPS |
                /* Compare with the character set */
                _SIDD_CMP_EQUAL_ANY |
                /* Expand the corrsponding bit result to byte unit */
                _SIDD_UNIT_MASK);
        __m128i maskLowTritP1 = _mm_cmpistrm(
            patternLowTritP1, data,
            _SIDD_SBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_UNIT_MASK);
        __m128i maskLowTritN1 = _mm_cmpistrm(
            patternLowTritN1, data,
            _SIDD_SBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_UNIT_MASK);
        __m128i maskMidTrit0 = _mm_cmpistrm(
            patternMidTrit0, data,
            /* Signed byte comparison */
            _SIDD_SBYTE_OPS |
                /* Compare with the character range */
                _SIDD_CMP_RANGES |
                /* Expand the corrsponding bit result to byte unit */
                _SIDD_UNIT_MASK);
        __m128i maskMidTritP1 =
            _mm_cmpistrm(patternMidTritP1, data,
                         _SIDD_SBYTE_OPS | _SIDD_CMP_RANGES | _SIDD_UNIT_MASK);
        __m128i maskMidTritN1 =
            _mm_cmpistrm(patternMidTritN1, data,
                         _SIDD_SBYTE_OPS | _SIDD_CMP_RANGES | _SIDD_UNIT_MASK);
        __m128i maskHighTrit0 =
            _mm_cmpistrm(patternHighTrit0, data,
                         _SIDD_SBYTE_OPS | _SIDD_CMP_RANGES | _SIDD_UNIT_MASK);
        __m128i maskHighTritP1 =
            _mm_cmpistrm(patternHighTritP1, data,
                         _SIDD_SBYTE_OPS | _SIDD_CMP_RANGES | _SIDD_UNIT_MASK);
        __m128i maskHighTritN1 =
            _mm_cmpistrm(patternHighTritN1, data,
                         _SIDD_SBYTE_OPS | _SIDD_CMP_RANGES | _SIDD_UNIT_MASK);

        /*
         * Each block represents a trit.
         *                                        shuffle
         *            ------     ------     ------                   ----------------        ------ 
         * lowTrit  = | a1 | ... | f1 | ... | p1 |       dataFirst = | a1 | a2 | a3 | ...... | f1 |    
         *            ------     ------     ------                   ----------------        ------    
         *            ------     ------     ------                   ----------------        ------    
         * midTrit  = | a2 | ... | f2 | ... | p2 |   =>  dataMid   = | f2 | f3 | g1 | ...... | k2 |    
         *            ------     ------     ------                   ----------------        ------    
         *            ------     ------     ------                   ----------------        ------    
         * highTrit = | a3 | ... | f3 | ... | p3 |       dataLast  = | k3 | l1 | l2 | ...... | p3 |    
         *            ------     ------     ------                   ----------------        ------    
         */
        __m128i lowTrit =
            _mm_or_si128(_mm_and_si128(maskLowTrit0, zero),
                         _mm_or_si128(_mm_and_si128(maskLowTritP1, posOne),
                                      _mm_and_si128(maskLowTritN1, negOne)));
        __m128i midTrit =
            _mm_or_si128(_mm_and_si128(maskMidTrit0, zero),
                         _mm_or_si128(_mm_and_si128(maskMidTritP1, posOne),
                                      _mm_and_si128(maskMidTritN1, negOne)));
        __m128i highTrit =
            _mm_or_si128(_mm_and_si128(maskHighTrit0, zero),
                         _mm_or_si128(_mm_and_si128(maskHighTritP1, posOne),
                                      _mm_and_si128(maskHighTritN1, negOne)));
        /* Initialize with 0 */
        __m128i dataFirst = _mm_set1_epi8(0);
        __m128i dataMid = _mm_set1_epi8(0);
        __m128i dataLast = _mm_set1_epi8(0);
        dataFirst = _mm_or_si128(
            _mm_shuffle_epi8(lowTrit, shuffleFirst[0]),
            _mm_or_si128(_mm_shuffle_epi8(midTrit, shuffleFirst[1]),
                         _mm_shuffle_epi8(highTrit, shuffleFirst[2])));
        dataMid = _mm_or_si128(
            _mm_shuffle_epi8(lowTrit, shuffleMid[0]),
            _mm_or_si128(_mm_shuffle_epi8(midTrit, shuffleMid[1]),
                         _mm_shuffle_epi8(highTrit, shuffleMid[2])));
        dataLast = _mm_or_si128(
            _mm_shuffle_epi8(lowTrit, shuffleLast[0]),
            _mm_or_si128(_mm_shuffle_epi8(midTrit, shuffleLast[1]),
                         _mm_shuffle_epi8(highTrit, shuffleLast[2])));

        /* Store the 3 * 128-bit trits converted from trytes */
        _mm_store_si128((__m128i *) (src + (3 * i) * block_8bit), dataFirst);
        _mm_store_si128((__m128i *) (src + (3 * i + 1) * block_8bit), dataMid);
        _mm_store_si128((__m128i *) (src + (3 * i + 2) * block_8bit), dataLast);
    }
    /* The rest of the trytes */
    for (int i = (trytes->len / block_8bit) * block_8bit; i < trytes->len;
         i++) {
        int idx = (trytes->data[i] == '9') ? 0 : trytes->data[i] - 'A' + 1;
        for (int j = 0; j < 3; j++) {
            src[i * 3 + j] = TrytesToTritsMappings[idx][j];
        }
    }

    trits = initTrits(src, trytes->len * 3);
    free(src);

    return trits;
}
#endif  // #if defined(__SSE4_2__)

#endif
