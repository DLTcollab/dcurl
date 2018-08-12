#include "implcontext.h"

extern struct list_head IMPL_LIST;

int registerImplContext(ImplContext *impl_ctx)
{
    initializeImplContext(impl_ctx);
    list_add(&impl_ctx->list, &IMPL_LIST);
}

int initializeImplContext(ImplContext *impl_ctx)
{
    return impl_ctx->initialize(impl_ctx);
}

int enterImplContext(ImplContext *impl_ctx)
{
    pthread_mutex_lock(&impl_ctx->lock);
    if (impl_ctx->num_working_thread >= impl_ctx->num_max_thread) {
        pthread_mutex_unlock(&impl_ctx->lock);
        return 0; /* Access Failed */
    }
    impl_ctx->num_working_thread++;
    pthread_mutex_unlock(&impl_ctx->lock);
    return 1; /* Access Success */
}

void *getPoWContext(ImplContext *impl_ctx, int8_t *trytes, int mwm)
{
    return impl_ctx->getPoWContext(impl_ctx, trytes, mwm);
}

int doThePoW(ImplContext *impl_ctx, void *pow_ctx)
{
    return impl_ctx->doThePoW(pow_ctx);
}

int freePoWContext(ImplContext *impl_ctx, void *pow_ctx)
{
    return impl_ctx->freePoWContext(impl_ctx, pow_ctx);
}

int8_t *getPoWResult(ImplContext *impl_ctx, void *pow_ctx)
{
    return impl_ctx->getPoWResult(pow_ctx);
}
