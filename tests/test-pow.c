/*
 * Copyright (C) 2018-2019 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

/* Test program for pow_*.c */
#include <math.h>
#include "common.h"
#include "implcontext.h"

#define POW_TOTAL 100

#if defined(ENABLE_AVX)
extern impl_context_t pow_avx_context;
#elif defined(ENABLE_SSE)
extern impl_context_t pow_sse_context;
#elif defined(ENABLE_GENERIC)
extern impl_context_t pow_c_context;
#endif

#if defined(ENABLE_OPENCL)
extern impl_context_t pow_cl_context;
#endif

#if defined(ENABLE_FPGA)
extern impl_context_t pow_fpga_context;
#endif

const char *description[] = {
#if defined(ENABLE_AVX)
    "CPU - AVX",
#elif defined(ENABLE_SSE)
    "CPU - SSE",
#elif defined(ENABLE_GENERIC)
    "CPU - pure C",
#endif

#if defined(ENABLE_OPENCL)
    "GPU - OpenCL",
#endif

#if defined(ENABLE_FPGA)
    "FPGA",
#endif
};

double get_avg(const double arr[], int arr_len)
{
    double avg, sum = 0;

    for (int idx = 0; idx < arr_len; idx++) {
        sum += arr[idx];
    }
    avg = sum / arr_len;

    return avg;
}

double get_std_deviation(const double arr[], int arr_len)
{
    double sigma, variance = 0;
    double avg = get_avg(arr, arr_len);

    for (int idx = 0; idx < arr_len; idx++) {
        variance += pow(arr[idx] - avg, 2);
    }
    sigma = sqrt(variance / arr_len);

    return sigma;
}

int main()
{
    char *transaction_trytes =
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "99999999999999999A9RGRKVGWMWMKOLVMDFWJUHNUNYWZTJADGGPZGXNLERLXYWJE9WQH"
        "WWBMCPZMVVMJUMWWBLZLNMLDCGDJ999999999999999999999999999999999999999999"
        "999999999999YGYQIVD99999999999999999999TXEFLKNPJRBYZPORHZU9CEMFIFVVQBU"
        "STDGSJCZMBTZCDTTJVUFPTCCVHHORPMGCURKTH9VGJIXUQJVHK99999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999";

    int mwm = 14;

    impl_context_t impl_context_arr[] = {
#if defined(ENABLE_AVX)
        pow_avx_context,
#elif defined(ENABLE_SSE)
        pow_sse_context,
#elif defined(ENABLE_GENERIC)
        pow_c_context,
#endif

#if defined(ENABLE_OPENCL)
        pow_cl_context,
#endif

#if defined(ENABLE_FPGA)
        pow_fpga_context,
#endif
    };
#if defined(ENABLE_STAT)
    double hash_rate_arr[POW_TOTAL];
    int pow_total = POW_TOTAL;
#else
    int pow_total = 1;
#endif


    for (int idx = 0; idx < sizeof(impl_context_arr) / sizeof(impl_context_t);
         idx++) {
        log_info(0, "%s\n", description[idx]);

        impl_context_t *pow_context_ptr = &impl_context_arr[idx];

        /* test implementation with mwm = 14 */
        initialize_impl_context(pow_context_ptr);
        void *pow_ctx =
            get_pow_context(pow_context_ptr, (int8_t *) transaction_trytes, mwm, 8);
        assert(pow_ctx);

        for (int count = 0; count < pow_total; count++) {
            do_the_pow(pow_context_ptr, pow_ctx);
            int8_t *ret_trytes = get_pow_result(pow_context_ptr, pow_ctx);
            assert(ret_trytes);
#if defined(ENABLE_STAT)
            pow_info_t pow_info = get_pow_info(pow_context_ptr, pow_ctx);
#endif

            trytes_t *trytes =
                init_trytes(ret_trytes, TRANSACTION_TRYTES_LENGTH);
            assert(trytes);
            trytes_t *hashed_trytes = hash_trytes(trytes);
            assert(hashed_trytes);
            trits_t *ret_trits = trits_from_trytes(hashed_trytes);
            assert(ret_trits);

            /* Validation */
            for (int i = HASH_TRITS_LENGTH - 1; i >= HASH_TRITS_LENGTH - mwm;
                 i--) {
                assert(ret_trits->data[i] == 0);
            }

            free(ret_trytes);
            free_trinary_object(trytes);
            free_trinary_object(hashed_trytes);
            free_trinary_object(ret_trits);

#if defined(ENABLE_STAT)
            hash_rate_arr[count] = pow_info.hash_count / pow_info.time;
#endif
        }

        free_pow_context(pow_context_ptr, pow_ctx);
        destroy_impl_context(pow_context_ptr);

        log_info(0, "PoW execution times: %d times.\n", pow_total);
#if defined(ENABLE_STAT)
        log_info(0, "Hash rate average value: %.3lf kH/sec,\n",
                 get_avg(hash_rate_arr, pow_total) / 1000);
        log_info(
            0,
            "with the range +- %.3lf kH/sec including 95%% of the hash rate "
            "values.\n",
            2 * get_std_deviation(hash_rate_arr, pow_total) / 1000);
#endif
        log_info(0, "Success.\n");
    }

    return 0;
}
