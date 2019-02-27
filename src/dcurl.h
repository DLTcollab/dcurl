#ifndef DCURL_H_
#define DCURL_H_

#include <stdbool.h>
#include "trinary.h"

/**
 * @file dcurl.h
 * @brief dcurl API functions.
 *
 * dcurl is a hardware-accelerated implementation for IOTA PearlDiver.
 * It supports multi-threaded SIMD on CPU, OpenCL on GPU and FPGA to
 * make a faster proof-of-work(PoW) for IOTA.
 * The API functions can be used to initialize, execute and destroy dcurl
 * easily.
 */

/**
 * @brief dcurl initialization.
 *
 * Register the determined hardware into the list and initialize the
 * corresponding resource.
 * @return
 * - true: initialization succeeded.
 * - false: initialization failed.
 */
bool dcurl_init();

/**
 * @brief dcurl destruction.
 *
 * Remove the registered hardware from the list and release the corresponding
 * resource.
 */
void dcurl_destroy();

/**
 * @brief dcurl execution.
 *
 * Retrieve the available hardware from the list and use it to do the
 * PoW(Proof-of-Work).
 * @param [in] trytes The trytes for PoW calculation.
 * @param [in] mwm The minimum weight magnitude.
 * @param [in] threads
 * @parblock
 * The thread number of calculating the PoW. It affects CPU only.
 *
 * 0: use (maximum threads - 1).
 * @endparblock
 * @return The result of PoW.
 */
int8_t *dcurl_entry(int8_t *trytes, int mwm, int threads);

#endif
