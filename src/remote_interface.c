/*
 * Copyright (C) 2019 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#include "remote_interface.h"
#include <string.h>
#include "trinary.h"

bool initializeRemoteContext(RemoteImplContext *remote_ctx)
{
    bool res = remote_ctx->initialize(remote_ctx);
    if (res) {
        ddprintf(MSG_PREFIX "Implementation %s is initialized successfully\n",
                 remote_ctx->description);
    }
    return res;
}

void destroyRemoteContext(RemoteImplContext *remote_ctx)
{
    return remote_ctx->destroy(remote_ctx);
}

bool enterRemoteContext(RemoteImplContext *remote_ctx)
{
    uv_mutex_lock(&remote_ctx->lock);
    if (remote_ctx->num_working_thread >= remote_ctx->num_max_thread) {
        uv_mutex_unlock(&remote_ctx->lock);
        return false; /* Access Failed */
    }
    remote_ctx->num_working_thread++;
    uv_mutex_unlock(&remote_ctx->lock);
    return true; /* Access Success */
}

void *getRemoteContext(RemoteImplContext *remote_ctx, int8_t *trytes, int mwm)
{
    return remote_ctx->getPoWContext(remote_ctx, trytes, mwm);
}

bool doRemoteContext(RemoteImplContext *remote_ctx, void *pow_ctx)
{
    return remote_ctx->doThePoW(remote_ctx, pow_ctx);
}

int8_t *getRemoteResult(RemoteImplContext *remote_ctx, void *pow_ctx)
{
    return remote_ctx->getPoWResult(pow_ctx);
}

bool freeRemoteContext(RemoteImplContext *remote_ctx, void *pow_ctx)
{
    return remote_ctx->freePoWContext(remote_ctx, pow_ctx);
}

void exitRemoteContext(RemoteImplContext *remote_ctx)
{
    uv_mutex_lock(&remote_ctx->lock);
    remote_ctx->num_working_thread--;
    uv_mutex_unlock(&remote_ctx->lock);
}

bool PoWValidation(int8_t *output_trytes, int mwm)
{
    Trytes_t *trytes_t = initTrytes(output_trytes, TRANSACTION_TRYTES_LENGTH);
    if (!trytes_t) {
        ddprintf("PoW Validation: Initialization of Trytes fails\n");
        goto fail_to_inittrytes;
    }

    Trytes_t *hash_trytes = hashTrytes(trytes_t);
    if (!hash_trytes) {
        ddprintf("PoW Validation: Hashing trytes fails\n");
        goto fail_to_hashtrytes;
    }

    Trits_t *ret_trits = trits_from_trytes(hash_trytes);
    for (int i = 243 - 1; i >= 243 - mwm; i--) {
        if (ret_trits->data[i] != 0) {
            ddprintf("PoW Validation fails\n");
            goto fail_to_validation;
        }
    }

    return true;

fail_to_validation:
    freeTrobject(ret_trits);
    freeTrobject(hash_trytes);
fail_to_hashtrytes:
    freeTrobject(trytes_t);
fail_to_inittrytes:
    return false;
}

static bool Remote_doPoW(RemoteImplContext *remote_ctx, void *pow_ctx)
{
    char buf[4];
    char messagebody[TRANSACTION_TRYTES_LENGTH + 4];

    amqp_bytes_t reply_to_queue;

    PoW_Remote_Context *ctx = (PoW_Remote_Context *) pow_ctx;

    /* Message body format: transacton | mwm */
    memcpy(messagebody, ctx->input_trytes, TRANSACTION_TRYTES_LENGTH);
    sprintf(buf, "%d", ctx->mwm);
    memcpy(messagebody + TRANSACTION_TRYTES_LENGTH, buf, 4);

    if (!declare_callback_queue(&remote_ctx->conn[ctx->indexOfContext], 1,
                                &reply_to_queue))
        goto fail;

    if (!publish_message_with_reply_to(&remote_ctx->conn[ctx->indexOfContext],
                                       1, "incoming_queue", reply_to_queue,
                                       messagebody))
        goto fail;

    if (!wait_response_message(&remote_ctx->conn[ctx->indexOfContext], 1,
                               reply_to_queue, (char *) (ctx->output_trytes),
                               TRANSACTION_TRYTES_LENGTH))
        goto fail;

    amqp_bytes_free(reply_to_queue);

    PoWValidation(ctx->output_trytes, ctx->mwm);

    return true;

fail:
    return false;
}

static bool Remote_init(RemoteImplContext *remote_ctx)
{
    if (remote_ctx->num_max_thread <= 0)
        goto fail_to_init;

    PoW_Remote_Context *ctx = (PoW_Remote_Context *) malloc(
        sizeof(PoW_Remote_Context) * remote_ctx->num_max_thread);

    memset(remote_ctx->slots, 0, remote_ctx->num_max_thread * sizeof(bool));

    for (int i = 0; i < CONN_MAX; i++) {
        if (!connect_broker(&remote_ctx->conn[i], NULL))
            goto fail_to_init;
    }
    if (!declare_queue(&remote_ctx->conn[0], 1, "incoming_queue"))
        goto fail_to_init;

    remote_ctx->context = ctx;

    uv_mutex_init(&remote_ctx->lock);

    return true;

fail_to_init:
    return false;
}

static void Remote_destroy(RemoteImplContext *remote_ctx)
{
    PoW_Remote_Context *ctx = (PoW_Remote_Context *) remote_ctx->context;

    for (int i = 0; i < CONN_MAX; i++)
        disconnect_broker(&remote_ctx->conn[i]);

    free(ctx);
}

static void *Remote_getPoWContext(RemoteImplContext *remote_ctx,
                                  int8_t *trytes,
                                  int mwm)
{
    uv_mutex_lock(&remote_ctx->lock);

    for (int i = 0; i < remote_ctx->num_max_thread; i++) {
        if (!remote_ctx->slots[i]) {
            remote_ctx->slots[i] = true;

            uv_mutex_unlock(&remote_ctx->lock);
            PoW_Remote_Context *ctx =
                remote_ctx->context + sizeof(PoW_Remote_Context) * i;
            memcpy(ctx->input_trytes, trytes, TRANSACTION_TRYTES_LENGTH);
            ctx->mwm = mwm;
            ctx->indexOfContext = i;

            return ctx;
        }
    }

    uv_mutex_unlock(&remote_ctx->lock);

    return NULL; /* It should not happen */
}

static bool Remote_freePoWContext(RemoteImplContext *remote_ctx, void *pow_ctx)
{
    uv_mutex_lock(&remote_ctx->lock);

    remote_ctx->slots[((PoW_Remote_Context *) pow_ctx)->indexOfContext] = false;

    uv_mutex_unlock(&remote_ctx->lock);

    return true;
}

static int8_t *Remote_getPoWResult(void *pow_ctx)
{
    int8_t *ret = (int8_t *) malloc(sizeof(int8_t) * TRANSACTION_TRYTES_LENGTH);
    if (!ret)
        return NULL;
    memcpy(ret, ((PoW_Remote_Context *) pow_ctx)->output_trytes,
           TRANSACTION_TRYTES_LENGTH);
    return ret;
}

static PoW_Info Remote_getPoWInfo(void *pow_ctx)
{
    return ((PoW_Remote_Context *) pow_ctx)->pow_info;
}

RemoteImplContext Remote_Context = {
    .context = NULL,
    .description = "Remote interface",
    .num_max_thread = CONN_MAX,  // 1 <= num_max_thread
    .num_working_thread = 0,
    .initialize = Remote_init,
    .destroy = Remote_destroy,
    .getPoWContext = Remote_getPoWContext,
    .freePoWContext = Remote_freePoWContext,
    .doThePoW = Remote_doPoW,
    .getPoWResult = Remote_getPoWResult,
    .getPoWInfo = Remote_getPoWInfo,
};
