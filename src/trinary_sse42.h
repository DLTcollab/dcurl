#ifndef TRINARY_SSE42_H_
#define TRINARY_SSE42_H_

#include <nmmintrin.h>

#define BLOCK_8BIT(type) (sizeof(type) / sizeof(int8_t))

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

#endif
