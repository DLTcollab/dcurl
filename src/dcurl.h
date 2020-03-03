/*
 * Copyright (C) 2018-2019 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

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

/*! The default value of the broker hostname */
#define DEFAULT_BROKER_HOST "localhost"

/**
 * A structure representing the configuration of the initialization.
 */
typedef struct {
    char *broker_host; /**< The broker hostname used in the remote mode */
} dcurl_config;

/**
 * @brief dcurl initialization.
 *
 * Register the determined hardware into the list and initialize the
 * corresponding resource.
 * @param [in] config
 * @parblock
 * The configuration of the initialization.
 *
 * NULL: Use default configuration.
 * @endparblock
 * @return
 * - true: one of the initialization succeeded.
 * - false: all the initialization failed.
 */
bool dcurl_init(dcurl_config *config);

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
 * 0: use (maximum physical CPU - 1).
 * @endparblock
 * @return The result of PoW.
 */
int8_t *dcurl_entry(int8_t *trytes, int mwm, int threads);

#endif
