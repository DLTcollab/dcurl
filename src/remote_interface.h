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
#include "remote_common.h"
#include "uv.h"

#define CONN_MAX 20

typedef struct _pow_remote_context PoW_Remote_Context;
typedef struct _remote_impl_context RemoteImplContext;

struct _pow_remote_context {
    /* Thread management */
    int indexOfContext;
    /* Arguments of PoW */
    int8_t input_trytes[TRANSACTION_TRYTES_LENGTH];  /* 2673 */
    int8_t output_trytes[TRANSACTION_TRYTES_LENGTH]; /* 2673 */
    int mwm;
    /* PoW-related information */
    PoW_Info pow_info;
};

struct _remote_impl_context {
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
    bool (*initialize)(RemoteImplContext *remote_ctx);
    void (*destroy)(RemoteImplContext *remote_ctx);
    /* Private PoW Context for each thread */
    void *(*getPoWContext)(RemoteImplContext *remote_ctx,
                           int8_t *trytes,
                           int mwm);
    bool (*doThePoW)(RemoteImplContext *remote_ctx, void *pow_ctx);
    int8_t *(*getPoWResult)(void *pow_ctx);
    PoW_Info (*getPoWInfo)(void *pow_ctx);
    bool (*freePoWContext)(RemoteImplContext *remote_ctx, void *pow_ctx);
};

bool initializeRemoteContext(RemoteImplContext *remote_ctx);
void destroyRemoteContext(RemoteImplContext *remote_ctx);
bool enterRemoteContext(RemoteImplContext *remote_ctx);
void *getRemoteContext(RemoteImplContext *remote_ctx, int8_t *trytes, int mwm);
bool doRemoteContext(RemoteImplContext *remote_ctx, void *pow_ctx);
int8_t *getRemoteResult(RemoteImplContext *remote_ctx, void *pow_ctx);
bool freeRemoteContext(RemoteImplContext *remote_ctx, void *pow_ctx);
void exitRemoteContext(RemoteImplContext *remote_ctx);

#endif
