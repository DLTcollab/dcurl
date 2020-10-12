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

extern struct list_head remote_impl_list;

bool register_remote_context(remote_impl_context_t *remote_ctx)
{
    bool res = initialize_remote_context(remote_ctx);
    if (res)
        list_add(&remote_ctx->node, &remote_impl_list);
    return res;
}

bool initialize_remote_context(remote_impl_context_t *remote_ctx)
{
    bool res = remote_ctx->initialize(remote_ctx);
    if (res) {
        log_debug(0,
                  MSG_PREFIX "Implementation %s is initialized successfully\n",
                  remote_ctx->description);
    }
    return res;
}

void destroy_remote_context(remote_impl_context_t *remote_ctx)
{
    return remote_ctx->destroy(remote_ctx);
}

bool enter_remote_context(remote_impl_context_t *remote_ctx)
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

void *get_remote_context(remote_impl_context_t *remote_ctx,
                         int8_t *trytes,
                         int mwm)
{
    return remote_ctx->get_pow_context(remote_ctx, trytes, mwm);
}

bool do_remote_context(remote_impl_context_t *remote_ctx, void *pow_ctx)
{
    return remote_ctx->do_the_pow(remote_ctx, pow_ctx);
}

int8_t *get_remote_result(remote_impl_context_t *remote_ctx, void *pow_ctx)
{
    return remote_ctx->get_pow_result(pow_ctx);
}

bool free_remote_context(remote_impl_context_t *remote_ctx, void *pow_ctx)
{
    return remote_ctx->free_pow_context(remote_ctx, pow_ctx);
}

void exit_remote_context(remote_impl_context_t *remote_ctx)
{
    uv_mutex_lock(&remote_ctx->lock);
    remote_ctx->num_working_thread--;
    uv_mutex_unlock(&remote_ctx->lock);
}

bool pow_validation(int8_t *output_trytes, int mwm)
{
    trytes_t *trytes = init_trytes(output_trytes, TRANSACTION_TRYTES_LENGTH);
    if (!trytes) {
        log_debug(0, "PoW Validation: Initialization of Trytes fails\n");
        goto fail_to_inittrytes;
    }

    trytes_t *hashed_trytes = hash_trytes(trytes);
    if (!hashed_trytes) {
        log_debug(0, "PoW Validation: Hashing trytes fails\n");
        goto fail_to_hashtrytes;
    }

    trits_t *ret_trits = trits_from_trytes(hashed_trytes);
    for (int i = 243 - 1; i >= 243 - mwm; i--) {
        if (ret_trits->data[i] != 0) {
            log_debug(0, "PoW Validation fails\n");
            goto fail_to_validation;
        }
    }

    return true;

fail_to_validation:
    free_trinary_object(ret_trits);
    free_trinary_object(hashed_trytes);
fail_to_hashtrytes:
    free_trinary_object(trytes);
fail_to_inittrytes:
    return false;
}

static bool remote_do_pow(remote_impl_context_t *remote_ctx, void *pow_ctx)
{
    char buf[4];
    char queue_name[12];
    char messagebody[TRANSACTION_TRYTES_LENGTH + 4];

    amqp_bytes_t reply_to_queue;

    pow_remote_context_t *ctx = (pow_remote_context_t *) pow_ctx;

    /* Message body format: transacton | mwm */
    memcpy(messagebody, ctx->input_trytes, TRANSACTION_TRYTES_LENGTH);
    snprintf(buf, sizeof(buf), "%d", ctx->mwm);
    memcpy(messagebody + TRANSACTION_TRYTES_LENGTH, buf, 4);

    /* Generate callback queue name with the format "number#" */
    snprintf(queue_name, sizeof(queue_name), "number%d", ctx->index_of_context);
    reply_to_queue = amqp_cstring_bytes(queue_name);

    if (!declare_callback_queue(&remote_ctx->conn[ctx->index_of_context], 1,
                                reply_to_queue))
        goto fail;

    if (!publish_message_with_reply_to(&remote_ctx->conn[ctx->index_of_context],
                                       1, "incoming_queue", reply_to_queue,
                                       messagebody))
        goto fail;

    if (!wait_response_message(&remote_ctx->conn[ctx->index_of_context], 1,
                               reply_to_queue, (char *) (ctx->output_trytes),
                               TRANSACTION_TRYTES_LENGTH))
        goto fail;

    pow_validation(ctx->output_trytes, ctx->mwm);

    return true;

fail:
    return false;
}

static bool remote_init(remote_impl_context_t *remote_ctx)
{
    if (remote_ctx->num_max_thread <= 0)
        goto fail_to_max_thread;

    pow_remote_context_t *ctx = (pow_remote_context_t *) malloc(
        sizeof(pow_remote_context_t) * remote_ctx->num_max_thread);

    memset(remote_ctx->slots, 0, remote_ctx->num_max_thread * sizeof(bool));

    for (int i = 0; i < CONN_MAX; i++) {
        if (!connect_broker(&remote_ctx->conn[i], remote_ctx->broker_host))
            goto fail_to_init;
    }
    if (!declare_queue(&remote_ctx->conn[0], 1, "incoming_queue"))
        goto fail_to_init;

    remote_ctx->context = ctx;

    uv_mutex_init(&remote_ctx->lock);

    return true;

fail_to_init:
    free(ctx);
fail_to_max_thread:
    return false;
}

static void remote_destroy(remote_impl_context_t *remote_ctx)
{
    pow_remote_context_t *ctx = (pow_remote_context_t *) remote_ctx->context;

    for (int i = 0; i < CONN_MAX; i++)
        disconnect_broker(&remote_ctx->conn[i]);

    free(ctx);
}

static void *remote_get_pow_context(remote_impl_context_t *remote_ctx,
                                    int8_t *trytes,
                                    int mwm)
{
    uv_mutex_lock(&remote_ctx->lock);

    for (int i = 0; i < remote_ctx->num_max_thread; i++) {
        if (!remote_ctx->slots[i]) {
            remote_ctx->slots[i] = true;

            uv_mutex_unlock(&remote_ctx->lock);
            pow_remote_context_t *ctx =
                (pow_remote_context_t *) remote_ctx->context + i;
            memcpy(ctx->input_trytes, trytes, TRANSACTION_TRYTES_LENGTH);
            ctx->mwm = mwm;
            ctx->index_of_context = i;

            return ctx;
        }
    }

    uv_mutex_unlock(&remote_ctx->lock);

    return NULL; /* It should not happen */
}

static bool remote_free_pow_context(remote_impl_context_t *remote_ctx,
                                    void *pow_ctx)
{
    uv_mutex_lock(&remote_ctx->lock);

    remote_ctx->slots[((pow_remote_context_t *) pow_ctx)->index_of_context] =
        false;

    uv_mutex_unlock(&remote_ctx->lock);

    return true;
}

static int8_t *remote_get_pow_result(void *pow_ctx)
{
    int8_t *ret = (int8_t *) malloc(sizeof(int8_t) * TRANSACTION_TRYTES_LENGTH);
    if (!ret)
        return NULL;
    memcpy(ret, ((pow_remote_context_t *) pow_ctx)->output_trytes,
           TRANSACTION_TRYTES_LENGTH);
    return ret;
}

static pow_info_t remote_get_pow_info(void *pow_ctx)
{
    return ((pow_remote_context_t *) pow_ctx)->pow_info;
}

remote_impl_context_t remote_context = {
    .context = NULL,
    .description = "Remote interface",
    .num_max_thread = CONN_MAX,  // 1 <= num_max_thread
    .num_working_thread = 0,
    .initialize = remote_init,
    .destroy = remote_destroy,
    .get_pow_context = remote_get_pow_context,
    .free_pow_context = remote_free_pow_context,
    .do_the_pow = remote_do_pow,
    .get_pow_result = remote_get_pow_result,
    .get_pow_info = remote_get_pow_info,
};
