#ifndef TRINARY_SSE42_H_
#define TRINARY_SSE42_H_

#include <nmmintrin.h>
#include "constants.h"

#define BLOCK_8BIT(type) (sizeof(type) / sizeof(int8_t))
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

static inline bool validateTrytes_sse42(Trobject_t *trytes)
{
    const int block_8bit = BLOCK_8BIT(__m128i);
    /* Characters from 'A' to 'Z' and '9' to '9' */
    const char *range = "AZ99";
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

#endif
