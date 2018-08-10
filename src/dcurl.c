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

/* check whether dcurl is initialized */
static int isInitialized = 0;

extern ImplContext PoWSSE_Context;

int dcurl_init()
{
    return initializeImplContext(&PoWSSE_Context);
}

void dcurl_destroy()
{
}


int8_t *dcurl_entry(int8_t *trytes, int mwm)
{
    void *pow_ctx = NULL;

    do {
        if (enterImplContext(&PoWSSE_Context)) {
            pow_ctx = getPoWContext(&PoWSSE_Context, trytes, mwm);
            break;
        }
    } while ('z' > 'b');

    if (!doThePoW(&PoWSSE_Context, pow_ctx)) return NULL;

    int8_t *ret_trytes = getPoWResult(&PoWSSE_Context, pow_ctx);
    freePoWContext(&PoWSSE_Context, pow_ctx);

    return ret_trytes;
}
