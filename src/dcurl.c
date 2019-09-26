/*
 * Copyright (C) 2018-2019 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
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
#if defined(ENABLE_REMOTE)
#include "remote_interface.h"
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

#if defined(ENABLE_REMOTE)
extern RemoteImplContext Remote_Context;
static uv_sem_t notify_remote;
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

#if defined(ENABLE_REMOTE)
    ret &= initializeRemoteContext(&Remote_Context);
    uv_sem_init(&notify_remote, 0);
#endif

    printf("init result: %d\n", ret);
    uv_sem_init(&notify, 0);
    return isInitialized = ret;
}

void dcurl_destroy()
{
    ImplContext *impl = NULL;
    struct list_head *p;

#if defined(ENABLE_REMOTE)
    destroyRemoteContext(&Remote_Context);
#endif

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

#if defined(ENABLE_REMOTE)
    do {
        if (enterRemoteContext(&Remote_Context)) {
            pow_ctx = getRemoteContext(&Remote_Context, trytes, mwm);
            goto remote_pow;
        }
        uv_sem_wait(&notify_remote);
    } while (1);

remote_pow:
    if (!doRemoteContext(&Remote_Context, pow_ctx)) {
        /* The remote interface can not work without activated RabbitMQ broker
         * and remote worker. If it is not working, the PoW would be calculated
         * by the local machine. And the remote interface resource should be
         * released.
         */
        freeRemoteContext(&Remote_Context, pow_ctx);
        exitRemoteContext(&Remote_Context);
        uv_sem_post(&notify_remote);
        goto local_pow;
    } else {
        res = getRemoteResult(&Remote_Context, pow_ctx);
    }
    freeRemoteContext(&Remote_Context, pow_ctx);
    exitRemoteContext(&Remote_Context);
    uv_sem_post(&notify_remote);
    return res;

local_pow:
#endif
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
