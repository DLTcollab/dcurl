/*
 * Copyright (C) 2019 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#include <getopt.h>
#include "common.h"
#include "constants.h"
#include "dcurl.h"
#include "remote_common.h"

int main(int argc, char *const *argv)
{
    char trytes[TRANSACTION_TRYTES_LENGTH];
    char buf[4];
    int mwm;
    int cmdOpt;
    int optIdx;
    const struct option longOpt[] = {{"broker", required_argument, NULL, 'b'},
                                     {NULL, 0, NULL, 0}};

    amqp_connection_state_t conn;
    amqp_envelope_t envelope;
    char *hostIP = NULL;

    /* Parse the command line options */
    /* TODO: Support macOS since getopt_long() is GNU extension */
    while (1) {
        cmdOpt = getopt_long(argc, argv, "b:", longOpt, &optIdx);
        if (cmdOpt == -1)
            break;

        /* Invalid option */
        if (cmdOpt == '?')
            break;

        if (cmdOpt == 'b') {
            hostIP = optarg;
        }
    }

    dcurl_init();

    if (!connect_broker(&conn, hostIP))
        goto fail;

    if (!declare_queue(&conn, 1, "incoming_queue"))
        goto fail;

    if (!set_consuming_queue(&conn, 1, "incoming_queue"))
        goto fail;

    for (;;) {
        if (!consume_message(&conn, 1, &envelope))
            goto fail;

        log_debug(
            0,
            MSG_PREFIX
            "Delivery %u, exchange %.*s, routingkey %.*s, callback queue: %s "
            "\n",
            (unsigned) envelope.delivery_tag, (int) envelope.exchange.len,
            (char *) envelope.exchange.bytes, (int) envelope.routing_key.len,
            (char *) envelope.routing_key.bytes,
            (char *) envelope.message.properties.reply_to.bytes);
        if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
            log_debug(0, MSG_PREFIX "Content-type: %.*s\n",
                      (int) envelope.message.properties.content_type.len,
                      (char *) envelope.message.properties.content_type.bytes);
        }

        /* Message body format: transacton | mwm */
        memcpy(trytes, envelope.message.body.bytes, TRANSACTION_TRYTES_LENGTH);
        memcpy(
            buf,
            (int8_t *) envelope.message.body.bytes + TRANSACTION_TRYTES_LENGTH,
            4);
        mwm = strtol(buf, NULL, 10);

        log_debug(0, MSG_PREFIX "Doing PoW with mwm = %d...\n", mwm);

        int8_t *ret_trytes = dcurl_entry((int8_t *) trytes, mwm, 0);
        memset(buf, '0', sizeof(buf));
        log_debug(0, MSG_PREFIX "PoW is done\n");

        if (!acknowledge_broker(&conn, 1, &envelope))
            goto fail;
        log_debug(0, MSG_PREFIX "Sending an ack is done\n");

        /* Publish a message of remote PoW result */
        if (!publish_message(
                &conn, 1, (char *) envelope.message.properties.reply_to.bytes,
                (char *) ret_trytes))
            goto fail;

        free(ret_trytes);
        amqp_destroy_envelope(&envelope);
        log_debug(
            0, MSG_PREFIX "Publishing PoW result to callback queue is done\n");
        log_debug(0, MSG_PREFIX "---\n");
    }

fail:
    dcurl_destroy();

    return -1;
}
