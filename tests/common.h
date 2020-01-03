/*
 * Copyright (C) 2018-2019 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include "curl.h"
#include "trinary.h"

/* FIXME: conditional inclusion of architecture-specific headers */
#if defined(ENABLE_AVX)
#include "pow_avx.h"
#elif defined(ENABLE_SSE)
#include "pow_sse.h"
#elif defined(ENABLE_GENERIC)
#include "pow_c.h"
#endif

#if defined(ENABLE_OPENCL)
#include "pow_cl.h"
#endif
#if defined(ENABLE_FPGA)
#include "pow_fpga.h"
#endif

#include <stdint.h>
#include <stdlib.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

#endif
