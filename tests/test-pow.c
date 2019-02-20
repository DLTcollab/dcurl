/* Test program for pow_*.c */
#include "common.h"
#include "implcontext.h"
#include "math.h"

#define POW_TOTAL 100

#if defined(ENABLE_AVX)
extern ImplContext PoWAVX_Context;
#elif defined(ENABLE_SSE)
extern ImplContext PoWSSE_Context;
#elif defined(ENABLE_GENERIC)
extern ImplContext PoWC_Context;
#endif

#if defined(ENABLE_OPENCL)
extern ImplContext PoWCL_Context;
#endif

#if defined(ENABLE_FPGA_ACCEL)
extern ImplContext PoWFPGAAccel_Context;
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

#if defined(ENABLE_FPGA_ACCEL)
    "FPGA",
#endif
};

double getAvg(const double arr[], int arrLen)
{
    double avg, sum = 0;

    for (int idx = 0; idx < arrLen; idx++) {
        sum += arr[idx];
    }
    avg = sum / arrLen;

    return avg;
}

double getStdDeviation(const double arr[], int arrLen)
{
    double sigma, variance = 0;
    double avg = getAvg(arr, arrLen);

    for (int idx = 0; idx < arrLen; idx++) {
        variance += pow(arr[idx] - avg, 2);
    }
    sigma = sqrt(variance / arrLen);

    return sigma;
}

int main()
{
    char *trytes =
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

    ImplContext ImplContextArr[] = {
#if defined(ENABLE_AVX)
        PoWAVX_Context,
#elif defined(ENABLE_SSE)
        PoWSSE_Context,
#elif defined(ENABLE_GENERIC)
        PoWC_Context,
#endif

#if defined(ENABLE_OPENCL)
        PoWCL_Context,
#endif

#if defined(ENABLE_FPGA_ACCEL)
        PoWFPGAAccel_Context,
#endif
    };
#if defined(ENABLE_STAT)
    double hashRateArr[POW_TOTAL];
    int pow_total = POW_TOTAL;
#else
    int pow_total = 1;
#endif


    for (int idx = 0; idx < sizeof(ImplContextArr) / sizeof(ImplContext);
         idx++) {
        printf("%s\n", description[idx]);

        ImplContext *PoW_Context_ptr = &ImplContextArr[idx];

        /* test implementation with mwm = 14 */
        initializeImplContext(PoW_Context_ptr);
        void *pow_ctx =
            getPoWContext(PoW_Context_ptr, (int8_t *) trytes, mwm, 0);
        assert(pow_ctx);

        for (int count = 0; count < pow_total; count++) {
            doThePoW(PoW_Context_ptr, pow_ctx);
            int8_t *ret_trytes = getPoWResult(PoW_Context_ptr, pow_ctx);
            assert(ret_trytes);
#if defined(ENABLE_STAT)
            PoW_Info pow_info = getPoWInfo(PoW_Context_ptr, pow_ctx);
#endif

            Trytes_t *trytes_t =
                initTrytes(ret_trytes, TRANSACTION_TRYTES_LENGTH);
            assert(trytes_t);
            Trytes_t *hash_trytes = hashTrytes(trytes_t);
            assert(hash_trytes);
            Trits_t *ret_trits = trits_from_trytes(hash_trytes);
            assert(ret_trits);

            /* Validation */
            for (int i = HASH_TRITS_LENGTH - 1; i >= HASH_TRITS_LENGTH - mwm;
                 i--) {
                assert(ret_trits->data[i] == 0);
            }

            free(ret_trytes);
            freeTrobject(trytes_t);
            freeTrobject(hash_trytes);
            freeTrobject(ret_trits);

#if defined(ENABLE_STAT)
            hashRateArr[count] = pow_info.hash_count / pow_info.time;
#endif
        }

        freePoWContext(PoW_Context_ptr, pow_ctx);
        destroyImplContext(PoW_Context_ptr);

        printf("PoW execution times: %d times.\n", pow_total);
#if defined(ENABLE_STAT)
        printf("Hash rate average value: %.3lf kH/sec,\n",
               getAvg(hashRateArr, pow_total) / 1000);
        printf(
            "with the range +- %.3lf kH/sec including 95%% of the hash rate "
            "values.\n",
            2 * getStdDeviation(hashRateArr, pow_total) / 1000);
#endif
        printf("Success.\n");
    }

    return 0;
}
