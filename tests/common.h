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
#if defined(ENABLE_FPGA_ACCEL)
#include "pow_fpga_accel.h"
#endif

#include <stdint.h>
#include <stdlib.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

#endif
