#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include "../src/trinary/trinary.h"
#include "../src/hash/curl.h"

/* FIXME: conditional inclusion of architecture-specific headers */ 
#include "../src/pow_sse.h"

#if defined(BUILD_OPENCL)
#include "../src/pow_cl.h"
#endif

#include <stdlib.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

#endif
