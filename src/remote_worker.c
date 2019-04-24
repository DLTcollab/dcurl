/*
 * Copyright (C) 2019 dcurl Developers.
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE file.
 */

#include "constants.h"
#include "dcurl.h"
#include "remote_common.h"
#include "common.h"

int main(int argc, char const *const *argv)
{
    char trytes[TRANSACTION_TRYTES_LENGTH];
    char buf[4];
    int mwm;

    amqp_connection_state_t conn;
    amqp_envelope_t envelope;

    dcurl_init();

    if (!connect_broker(&conn))
        goto fail;

    if (!set_consuming_queue(&conn, 1, "incoming_queue"))
        goto fail;

    for (;;) {
        if (!consume_message(&conn, 1, &envelope))
            goto fail;

        ddprintf(MSG_PREFIX "Delivery %u, exchange %.*s, routingkey %.*s, callback queue: %s "
            "\n",
            (unsigned) envelope.delivery_tag, (int) envelope.exchange.len,
            (char *) envelope.exchange.bytes, (int) envelope.routing_key.len,
            (char *) envelope.routing_key.bytes,
            (char *) envelope.message.properties.reply_to.bytes);
        if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
            ddprintf(MSG_PREFIX "Content-type: %.*s\n",
                   (int) envelope.message.properties.content_type.len,
                   (char *) envelope.message.properties.content_type.bytes);
        }

        /* Message body format: transacton | mwm */
        memcpy(trytes, envelope.message.body.bytes, TRANSACTION_TRYTES_LENGTH);
        memcpy(buf, envelope.message.body.bytes + TRANSACTION_TRYTES_LENGTH, 4);
        mwm = strtol(buf, NULL, 10);

        ddprintf(MSG_PREFIX "Doing PoW with mwm = %d...\n", mwm);

        int8_t *ret_trytes = dcurl_entry((int8_t *) trytes, mwm, 0);
        memset(buf, '0', sizeof(buf));
        ddprintf(MSG_PREFIX "PoW is done\n");

        if (!acknowledge_broker(&conn, 1, &envelope))
            goto fail;
        ddprintf(MSG_PREFIX "Sending an ack is done\n");

        /* Publish a message of remote PoW result */
        if (!publish_message(
                &conn, 1,
                (char *) envelope.message.properties.reply_to.bytes,
                (char *) ret_trytes))
            goto fail;

        free(ret_trytes);
        amqp_destroy_envelope(&envelope);
        ddprintf(MSG_PREFIX "Publishing PoW result to callback queue is done\n");
        ddprintf(MSG_PREFIX "---\n");
    }

fail:
    dcurl_destroy();

    return -1;
}
