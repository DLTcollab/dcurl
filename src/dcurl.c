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
#include "pow_fpga.h"
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

/* for checking whether the corresponding implementation is initialized */
enum Capability {
    CAP_NONE = 0U,
    CAP_C = 1U,
    CAP_SSE = 1U << 1,
    CAP_AVX = 1U << 2,
    CAP_GPU = 1U << 3,
    CAP_FPGA = 1U << 4,
    CAP_REMOTE = 1U << 5
};

/* check whether dcurl is initialized */
static bool isInitialized = false;
static uint8_t runtimeCaps = CAP_NONE;
static uv_sem_t notify;

LIST_HEAD(IMPL_LIST);
LIST_HEAD(REMOTE_IMPL_LIST);

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
extern ImplContext PoWFPGA_Context;
#endif

#if defined(ENABLE_REMOTE)
extern RemoteImplContext Remote_Context;
static uv_sem_t notify_remote;
#endif

bool dcurl_init()
{
    bool ret = false;

#if defined(ENABLE_AVX)
    if (registerImplContext(&PoWAVX_Context)) {
        runtimeCaps |= CAP_AVX;
        ret |= true;
    }
#elif defined(ENABLE_SSE)
    if (registerImplContext(&PoWSSE_Context)) {
        runtimeCaps |= CAP_SSE;
        ret |= true;
    }
#elif defined(ENABLE_GENERIC)
    if (registerImplContext(&PoWC_Context)) {
        runtimeCaps |= CAP_C;
        ret |= true;
    }
#endif

#if defined(ENABLE_OPENCL)
    if (registerImplContext(&PoWCL_Context)) {
        runtimeCaps |= CAP_GPU;
        ret |= true;
    }
#endif

#if defined(ENABLE_FPGA_ACCEL)
    if (registerImplContext(&PoWFPGA_Context)) {
        runtimeCaps |= CAP_FPGA;
        ret |= true;
    }
#endif

#if defined(ENABLE_REMOTE)
    if (registerRemoteContext(&Remote_Context)) {
        runtimeCaps |= CAP_REMOTE;
        ret |= true;
    }
    uv_sem_init(&notify_remote, 0);
#endif

    uv_sem_init(&notify, 0);
    return isInitialized = ret;
}

void dcurl_destroy()
{
    ImplContext *impl = NULL;
    struct list_head *p;

#if defined(ENABLE_REMOTE)
    RemoteImplContext *remoteImpl = NULL;
    struct list_head *pRemote;

    list_for_each (pRemote, &REMOTE_IMPL_LIST) {
        remoteImpl = list_entry(pRemote, RemoteImplContext, node);
        destroyRemoteContext(remoteImpl);
        list_del(pRemote);
    }
#endif

    list_for_each (p, &IMPL_LIST) {
        impl = list_entry(p, ImplContext, node);
        destroyImplContext(impl);
        list_del(p);
    }

    runtimeCaps = CAP_NONE;
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
    if (runtimeCaps & CAP_REMOTE) {
        RemoteImplContext *remoteImpl = NULL;
        struct list_head *pRemote;

        do {
            list_for_each (pRemote, &REMOTE_IMPL_LIST) {
                remoteImpl = list_entry(pRemote, RemoteImplContext, node);
                if (enterRemoteContext(remoteImpl)) {
                    pow_ctx = getRemoteContext(remoteImpl, trytes, mwm);
                    goto remote_pow;
                }
            }
            uv_sem_wait(&notify_remote);
        } while (1);

    remote_pow:
        if (!doRemoteContext(remoteImpl, pow_ctx)) {
            /* The remote interface can not work without activated RabbitMQ
             * broker and remote worker. If it is not working, the PoW would be
             * calculated by the local machine. And the remote interface
             * resource should be released.
             */
            freeRemoteContext(remoteImpl, pow_ctx);
            exitRemoteContext(remoteImpl);
            uv_sem_post(&notify_remote);
            goto local_pow;
        } else {
            res = getRemoteResult(remoteImpl, pow_ctx);
        }
        freeRemoteContext(remoteImpl, pow_ctx);
        exitRemoteContext(remoteImpl);
        uv_sem_post(&notify_remote);
        return res;
    }

local_pow:
#endif
    do {
        list_for_each (p, &IMPL_LIST) {
            impl = list_entry(p, ImplContext, node);
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
