/*
 * Copyright (C) 2019 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#ifndef REMOTE_INTERFACE_H_
#define REMOTE_INTERFACE_H_

#include <stdbool.h>
#include <stdint.h>
#include "common.h"
#include "constants.h"
#include "list.h"
#include "remote_common.h"
#include "uv.h"

#define CONN_MAX 20

typedef struct pow_remote_context_s pow_remote_context_t;
typedef struct remote_impl_context_s remote_impl_context_t;

struct pow_remote_context_s {
    /* Thread management */
    int index_of_context;
    /* Arguments of PoW */
    int8_t input_trytes[TRANSACTION_TRYTES_LENGTH];  /* 2673 */
    int8_t output_trytes[TRANSACTION_TRYTES_LENGTH]; /* 2673 */
    int mwm;
    /* PoW-related information */
    pow_info_t pow_info;
};

struct remote_impl_context_s {
    void *context;
    char *description;
    /* Connection parameters */
    amqp_connection_state_t conn[CONN_MAX];
    /* Thread management */
    uv_mutex_t lock;
    bool slots[CONN_MAX]; /* Used to tell which slot is
                                              available */
    int num_max_thread;
    int num_working_thread;

    /* Functions of Implementation Context */
    bool (*initialize)(remote_impl_context_t *remote_ctx);
    void (*destroy)(remote_impl_context_t *remote_ctx);
    /* Private PoW Context for each thread */
    void *(*get_pow_context)(remote_impl_context_t *remote_ctx,
                             int8_t *trytes,
                             int mwm);
    bool (*do_the_pow)(remote_impl_context_t *remote_ctx, void *pow_ctx);
    int8_t *(*get_pow_result)(void *pow_ctx);
    pow_info_t (*get_pow_info)(void *pow_ctx);
    bool (*free_pow_context)(remote_impl_context_t *remote_ctx, void *pow_ctx);

    /* Node in linked list */
    struct list_head node;
};

bool register_remote_context(remote_impl_context_t *remote_ctx);
bool initialize_remote_context(remote_impl_context_t *remote_ctx);
void destroy_remote_context(remote_impl_context_t *remote_ctx);
bool enter_remote_context(remote_impl_context_t *remote_ctx);
void *get_remote_context(remote_impl_context_t *remote_ctx,
                         int8_t *trytes,
                         int mwm);
bool do_remote_context(remote_impl_context_t *remote_ctx, void *pow_ctx);
int8_t *get_remote_result(remote_impl_context_t *remote_ctx, void *pow_ctx);
bool free_remote_context(remote_impl_context_t *remote_ctx, void *pow_ctx);
void exit_remote_context(remote_impl_context_t *remote_ctx);

#endif
