/*
 * Copyright (C) 2018-2019 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#include "implcontext.h"
#include <stdio.h>

#define MSG_PREFIX "[dcurl] "

extern struct list_head impl_list;

bool register_impl_context(impl_context_t *impl_ctx)
{
    bool res = initialize_impl_context(impl_ctx);
    if (res)
        list_add(&impl_ctx->node, &impl_list);
    return res;
}

bool initialize_impl_context(impl_context_t *impl_ctx)
{
    bool res = impl_ctx->initialize(impl_ctx);
    if (res) {
        log_debug(0,
                  MSG_PREFIX "Implementation %s is initialized successfully\n",
                  impl_ctx->description);
    }
    return res;
}

void destroy_impl_context(impl_context_t *impl_ctx)
{
    return impl_ctx->destroy(impl_ctx);
}

bool enter_impl_context(impl_context_t *impl_ctx)
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

void exit_impl_context(impl_context_t *impl_ctx)
{
    uv_mutex_lock(&impl_ctx->lock);
    impl_ctx->num_working_thread--;
    uv_mutex_unlock(&impl_ctx->lock);
}

void *get_pow_context(impl_context_t *impl_ctx,
                      int8_t *trytes,
                      int mwm,
                      int threads)
{
    return impl_ctx->get_pow_context(impl_ctx, trytes, mwm, threads);
}

bool do_the_pow(impl_context_t *impl_ctx, void *pow_ctx)
{
    return impl_ctx->do_the_pow(pow_ctx);
}

bool free_pow_context(impl_context_t *impl_ctx, void *pow_ctx)
{
    return impl_ctx->free_pow_context(impl_ctx, pow_ctx);
}

int8_t *get_pow_result(impl_context_t *impl_ctx, void *pow_ctx)
{
    return impl_ctx->get_pow_result(pow_ctx);
}

pow_info_t get_pow_info(impl_context_t *impl_ctx, void *pow_ctx)
{
    return impl_ctx->get_pow_info(pow_ctx);
}
