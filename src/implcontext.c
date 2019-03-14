#include "implcontext.h"
#include <stdio.h>

#define MSG_PREFIX "[dcurl] "

extern struct list_head IMPL_LIST;

bool registerImplContext(ImplContext *impl_ctx)
{
    list_add(&impl_ctx->list, &IMPL_LIST);
    return initializeImplContext(impl_ctx);
}

bool initializeImplContext(ImplContext *impl_ctx)
{
    bool res = impl_ctx->initialize(impl_ctx);
#if defined(ENABLE_DEBUG)
    if (res) {
        printf(MSG_PREFIX "Implementation %s is initialized successfully\n",
               impl_ctx->description);
    }
#endif
    return res;
}

void destroyImplContext(ImplContext *impl_ctx)
{
    return impl_ctx->destroy(impl_ctx);
}

bool enterImplContext(ImplContext *impl_ctx)
{
    uv_mutex_lock(&impl_ctx->lock);
    if (impl_ctx->num_working_thread >= impl_ctx->num_max_thread) {
        uv_mutex_unlock(&impl_ctx->lock);
        return false; /* Access Failed */
    }
    impl_ctx->num_working_thread++;
    uv_mutex_unlock(&impl_ctx->lock);
    return true; /* Access Success */
}

void exitImplContext(ImplContext *impl_ctx)
{
    uv_mutex_lock(&impl_ctx->lock);
    impl_ctx->num_working_thread--;
    uv_mutex_unlock(&impl_ctx->lock);
}

void *getPoWContext(ImplContext *impl_ctx, int8_t *trytes, int mwm, int threads)
{
    return impl_ctx->getPoWContext(impl_ctx, trytes, mwm, threads);
}

bool doThePoW(ImplContext *impl_ctx, void *pow_ctx)
{
    return impl_ctx->doThePoW(pow_ctx);
}

bool freePoWContext(ImplContext *impl_ctx, void *pow_ctx)
{
    return impl_ctx->freePoWContext(impl_ctx, pow_ctx);
}

int8_t *getPoWResult(ImplContext *impl_ctx, void *pow_ctx)
{
    return impl_ctx->getPoWResult(pow_ctx);
}

PoW_Info getPoWInfo(ImplContext *impl_ctx, void *pow_ctx)
{
    return impl_ctx->getPoWInfo(pow_ctx);
}
