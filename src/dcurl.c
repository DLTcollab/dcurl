/*
 * Copyright (C) 2018 dcurl Developers.
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE file.
 */

#include "dcurl.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#if defined(ENABLE_OPENCL)
#include "pow_cl.h"
#endif
#if defined(ENABLE_FPGA_ACCEL)
#include "pow_fpga_accel.h"
#endif
#include "implcontext.h"
#include "trinary.h"
#include "uv.h"
#if defined(ENABLE_AVX)
#include "pow_avx.h"
#elif defined(ENABLE_SSE)
#include "pow_sse.h"
#elif defined(ENABLE_GENERIC)
#include "pow_c.h"
#endif

/* check whether dcurl is initialized */
static bool isInitialized = false;
static uv_sem_t notify;

LIST_HEAD(IMPL_LIST);

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

bool dcurl_init()
{
    bool ret = true;

#if defined(ENABLE_AVX)
    ret &= registerImplContext(&PoWAVX_Context);
#elif defined(ENABLE_SSE)
    ret &= registerImplContext(&PoWSSE_Context);
#elif defined(ENABLE_GENERIC)
    ret &= registerImplContext(&PoWC_Context);
#endif

#if defined(ENABLE_OPENCL)
    ret &= registerImplContext(&PoWCL_Context);
#endif

#if defined(ENABLE_FPGA_ACCEL)
    ret &= registerImplContext(&PoWFPGAAccel_Context);
#endif

    uv_sem_init(&notify, 0);
    return isInitialized = ret;
}

void dcurl_destroy()
{
    ImplContext *impl = NULL;
    struct list_head *p;

    list_for_each (p, &IMPL_LIST) {
        impl = list_entry(p, ImplContext, list);
        destroyImplContext(impl);
        list_del(p);
    }
}


int8_t *dcurl_entry(int8_t *trytes, int mwm, int threads)
{
    void *pow_ctx = NULL;
    int8_t *res = NULL;

    ImplContext *impl = NULL;
    struct list_head *p;

    if (!isInitialized)
        return NULL;

    do {
        list_for_each (p, &IMPL_LIST) {
            impl = list_entry(p, ImplContext, list);
            if (enterImplContext(impl)) {
                pow_ctx = getPoWContext(impl, trytes, mwm, threads);
                goto do_pow;
            }
        }
        uv_sem_wait(&notify);
    } while (1);

do_pow:
    if (!doThePoW(impl, pow_ctx)) {
        res = NULL;
    } else {
        res = getPoWResult(impl, pow_ctx);
    }
    freePoWContext(impl, pow_ctx);
    exitImplContext(impl);
    uv_sem_post(&notify);
    return res;
}
