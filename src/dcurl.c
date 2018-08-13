/*
 * Copyright (C) 2018 dcurl Developers.
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE file.
 */

#include "dcurl.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pow_cl.h"
#include "trinary.h"
#include "implcontext.h"
#if defined(ENABLE_AVX)
#include "pow_avx.h"
#elif defined(ENABLE_SSE)
#include "pow_sse.h"
#else
#include "pow_c.h"
#endif

#ifdef __APPLE__
#include <dispatch/dispatch.h>
#else
#include <semaphore.h>
#endif

/* check whether dcurl is initialized */
static int isInitialized = 0;

static sem_t notify;

LIST_HEAD(IMPL_LIST);

#if defined(ENABLE_SSE)
extern ImplContext PoWSSE_Context;
#endif

int dcurl_init()
{
#if defined(ENABLE_SSE)
    registerImplContext(&PoWSSE_Context);
#endif

#ifdef __APPLE__
    notify = dispatch_semaphore_create(0);
#else
    sem_init(&notify, 0, 0);
#endif
    return 1; /* success */
}

void dcurl_destroy()
{
}


int8_t *dcurl_entry(int8_t *trytes, int mwm)
{
    void *pow_ctx = NULL;

    ImplContext *impl = NULL;
    struct list_head *p;

    do {
        list_for_each(p, &IMPL_LIST) {
            impl = list_entry(p, ImplContext, list);
            if (enterImplContext(impl)) {
                pow_ctx = getPoWContext(impl, trytes, mwm);
                goto pow;
            }
        }
#ifdef __APPLE__
        dispatch_semaphore_wait(notify, DISPATCH_TIME_FOREVER);
#else
        sem_wait(&notify);
#endif
    } while ('z' > 'b');

pow:
    if (!doThePoW(impl, pow_ctx)) return NULL;

    int8_t *ret_trytes = getPoWResult(impl, pow_ctx);
    freePoWContext(impl, pow_ctx);
    exitImplContext(impl);
#ifdef __APPLE__
    dispatch_semaphore_signal(notify);
#else
    sem_post(&notify);
#endif
    return ret_trytes;
}
