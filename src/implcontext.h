/*
 * Copyright (C) 2018-2019 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#ifndef IMPL_CTX_H_
#define IMPL_CTX_H_

#include <stdbool.h>
#include <stdint.h>
#include "common.h"
#include "list.h"
#include "uv.h"

typedef struct impl_context_s impl_context_t;

struct impl_context_s {
    void *context;
    char *description;

    /* Multi-thread Management */
    uv_mutex_t lock;
    int bitmap; /* Used to tell which slot is available */
    int num_max_thread;
    int num_working_thread;

    /* Functions of Implementation Context */
    bool (*initialize)(impl_context_t *impl_ctx);
    void (*destroy)(impl_context_t *impl_ctx);
    /* Private PoW Context for each thread */
    void *(*get_pow_context)(impl_context_t *impl_ctx,
                             int8_t *trytes,
                             int mwm,
                             int threads);
    bool (*do_the_pow)(void *pow_ctx);
    int8_t *(*get_pow_result)(void *pow_ctx);
    pow_info_t (*get_pow_info)(void *pow_ctx);
    bool (*free_pow_context)(impl_context_t *impl_ctx, void *pow_ctx);

    /* Node in linked list */
    struct list_head node;
};

bool register_impl_context(impl_context_t *impl_ctx);
bool initialize_impl_context(impl_context_t *impl_ctx);
void destroy_impl_context(impl_context_t *impl_ctx);
bool enter_impl_context(impl_context_t *impl_ctx);
void exit_impl_context(impl_context_t *impl_ctx);
void *get_pow_context(impl_context_t *impl_ctx,
                      int8_t *trytes,
                      int mwm,
                      int threads);
bool do_the_pow(impl_context_t *impl_ctx, void *pow_ctx);
bool free_pow_context(impl_context_t *impl_ctx, void *pow_ctx);
int8_t *get_pow_result(impl_context_t *impl_ctx, void *pow_ctx);
pow_info_t get_pow_info(impl_context_t *impl_ctx, void *pow_ctx);

#endif
