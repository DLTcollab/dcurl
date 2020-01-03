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
#if defined(ENABLE_FPGA)
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
enum capability {
    CAP_NONE = 0U,
    CAP_C = 1U,
    CAP_SSE = 1U << 1,
    CAP_AVX = 1U << 2,
    CAP_GPU = 1U << 3,
    CAP_FPGA = 1U << 4,
    CAP_REMOTE = 1U << 5
};

/* check whether dcurl is initialized */
static bool is_initialized = false;
static uint8_t runtime_caps = CAP_NONE;
static uv_sem_t notify;

LIST_HEAD(impl_list);
LIST_HEAD(remote_impl_list);

#if defined(ENABLE_AVX)
extern impl_context_t pow_avx_context;
#elif defined(ENABLE_SSE)
extern impl_context_t pow_sse_context;
#elif defined(ENABLE_GENERIC)
extern impl_context_t pow_c_context;
#endif

#if defined(ENABLE_OPENCL)
extern impl_context_t pow_cl_context;
#endif

#if defined(ENABLE_FPGA)
extern impl_context_t pow_fpga_context;
#endif

#if defined(ENABLE_REMOTE)
extern remote_impl_context_t remote_context;
static uv_sem_t notify_remote;
#endif

bool dcurl_init()
{
    bool ret = false;

#if defined(ENABLE_AVX)
    if (register_impl_context(&pow_avx_context)) {
        runtime_caps |= CAP_AVX;
        ret |= true;
    }
#elif defined(ENABLE_SSE)
    if (register_impl_context(&pow_sse_context)) {
        runtime_caps |= CAP_SSE;
        ret |= true;
    }
#elif defined(ENABLE_GENERIC)
    if (register_impl_context(&pow_c_context)) {
        runtime_caps |= CAP_C;
        ret |= true;
    }
#endif

#if defined(ENABLE_OPENCL)
    if (register_impl_context(&pow_cl_context)) {
        runtime_caps |= CAP_GPU;
        ret |= true;
    }
#endif

#if defined(ENABLE_FPGA)
    if (register_impl_context(&pow_fpga_context)) {
        runtime_caps |= CAP_FPGA;
        ret |= true;
    }
#endif

#if defined(ENABLE_REMOTE)
    if (register_remote_context(&remote_context)) {
        runtime_caps |= CAP_REMOTE;
        ret |= true;
    }
    uv_sem_init(&notify_remote, 0);
#endif

    uv_sem_init(&notify, 0);
    return is_initialized = ret;
}

void dcurl_destroy()
{
    impl_context_t *impl = NULL;
    struct list_head *p;

#if defined(ENABLE_REMOTE)
    remote_impl_context_t *remote_impl = NULL;
    struct list_head *p_remote;

    list_for_each (p_remote, &remote_impl_list) {
        remote_impl = list_entry(p_remote, remote_impl_context_t, node);
        destroy_remote_context(remote_impl);
        list_del(p_remote);
    }
#endif

    list_for_each (p, &impl_list) {
        impl = list_entry(p, impl_context_t, node);
        destroy_impl_context(impl);
        list_del(p);
    }

    runtime_caps = CAP_NONE;
}


int8_t *dcurl_entry(int8_t *trytes, int mwm, int threads)
{
    void *pow_ctx = NULL;
    int8_t *res = NULL;

    impl_context_t *impl = NULL;
    struct list_head *p;

    if (!is_initialized)
        return NULL;

#if defined(ENABLE_REMOTE)
    if (runtime_caps & CAP_REMOTE) {
        remote_impl_context_t *remote_impl = NULL;
        struct list_head *p_remote;

        do {
            list_for_each (p_remote, &remote_impl_list) {
                remote_impl = list_entry(p_remote, remote_impl_context_t, node);
                if (enter_remote_context(remote_impl)) {
                    pow_ctx = get_remote_context(remote_impl, trytes, mwm);
                    goto remote_pow;
                }
            }
            uv_sem_wait(&notify_remote);
        } while (1);

    remote_pow:
        if (!do_remote_context(remote_impl, pow_ctx)) {
            /* The remote interface can not work without activated RabbitMQ
             * broker and remote worker. If it is not working, the PoW would be
             * calculated by the local machine. And the remote interface
             * resource should be released.
             */
            free_remote_context(remote_impl, pow_ctx);
            exit_remote_context(remote_impl);
            uv_sem_post(&notify_remote);
            goto local_pow;
        } else {
            res = get_remote_result(remote_impl, pow_ctx);
        }
        free_remote_context(remote_impl, pow_ctx);
        exit_remote_context(remote_impl);
        uv_sem_post(&notify_remote);
        return res;
    }

local_pow:
#endif
    do {
        list_for_each (p, &impl_list) {
            impl = list_entry(p, impl_context_t, node);
            if (enter_impl_context(impl)) {
                pow_ctx = get_pow_context(impl, trytes, mwm, threads);
                goto do_pow;
            }
        }
        uv_sem_wait(&notify);
    } while (1);

do_pow:
    if (!do_the_pow(impl, pow_ctx)) {
        res = NULL;
    } else {
        res = get_pow_result(impl, pow_ctx);
    }
    free_pow_context(impl, pow_ctx);
    exit_impl_context(impl);
    uv_sem_post(&notify);
    return res;
}
