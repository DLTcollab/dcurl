#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include "curl.h"
#include "trinary.h"

/* FIXME: conditional inclusion of architecture-specific headers */
#if defined(ENABLE_AVX)
#include "pow_avx.h"
#else
#include "pow_sse.h"
#endif

#if defined(ENABLE_OPENCL)
#include "pow_cl.h"
#endif

#include <stdlib.h>
#include <stdint.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

#endif
