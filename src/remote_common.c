/*
 * Copyright (C) 2019 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#include "remote_common.h"
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "common.h"

bool die_on_amqp_error(amqp_rpc_reply_t x, char const *context)
{
    switch (x.reply_type) {
    case AMQP_RESPONSE_NORMAL:
        return true;

    case AMQP_RESPONSE_NONE:
        ddprintf("%s: missing RPC reply type!\n", context);
        break;

    case AMQP_RESPONSE_LIBRARY_EXCEPTION:
        ddprintf("%s: %s\n", context, amqp_error_string2(x.library_error));
        break;

    case AMQP_RESPONSE_SERVER_EXCEPTION:
        switch (x.reply.id) {
        case AMQP_CONNECTION_CLOSE_METHOD: {
            amqp_connection_close_t *m =
                (amqp_connection_close_t *) x.reply.decoded;
            ddprintf("%s: server connection error %u, message: %.*s\n",
                     context, m->reply_code, (int) m->reply_text.len,
                     (char *) m->reply_text.bytes);
            break;
        }
        case AMQP_CHANNEL_CLOSE_METHOD: {
            amqp_channel_close_t *m = (amqp_channel_close_t *) x.reply.decoded;
            ddprintf("%s: server channel error %u, message: %.*s\n", context,
                     m->reply_code, (int) m->reply_text.len,
                     (char *) m->reply_text.bytes);
            break;
        }
        default:
            ddprintf("%s: unknown server error, method id 0x%08X\n", context,
                     x.reply.id);
            break;
        }
        break;
    }

    return false;
}

bool die_on_error(int x, char const *context)
{
    if (x < 0) {
        ddprintf("%s: %s\n", context, amqp_error_string2(x));
        return false;
    }

    return true;
}

bool connect_broker(amqp_connection_state_t *conn, const char *hostName)
{
    amqp_socket_t *socket = NULL;
    const char *host = (hostName != NULL) ? hostName : "localhost";

    /* Connect to the rabbitmq broker */
    *conn = amqp_new_connection();
    socket = amqp_tcp_socket_new(*conn);
    if (amqp_socket_open(socket, host, 5672) != AMQP_STATUS_OK) {
        ddprintf("The rabbitmq broker of %s is closed\n", host);
        goto destroy_connection;
    }

    /* Login to the rabbitmq broker */
    if (!die_on_amqp_error(amqp_login(*conn, "/", AMQP_DEFAULT_MAX_CHANNELS,
                                      AMQP_DEFAULT_FRAME_SIZE, 0,
                                      AMQP_SASL_METHOD_PLAIN, "guest", "guest"),
                           "Logging in"))
        goto connection_close;

    /* Open the channel in the rabbitmq broker */
    amqp_channel_open(*conn, 1);
    if (!(die_on_amqp_error(amqp_get_rpc_reply(*conn), "Opening the channel")))
        goto channel_close;

    return true;

channel_close:
    amqp_channel_close(*conn, 1, AMQP_REPLY_SUCCESS);

connection_close:
    amqp_connection_close(*conn, AMQP_REPLY_SUCCESS);

destroy_connection:
    amqp_destroy_connection(*conn);

    return false;
}

bool declare_queue(amqp_connection_state_t *conn,
                   amqp_channel_t channel,
                   char const *queue_name)
{
    /* Declare a durable queue */
    amqp_queue_declare(*conn, channel, amqp_cstring_bytes(queue_name), 0, 1, 0,
                       0, amqp_empty_table);
    if (!die_on_amqp_error(amqp_get_rpc_reply(*conn), "Declaring the queue"))
        return false;

    return true;
}

bool declare_callback_queue(amqp_connection_state_t *conn,
                            amqp_channel_t channel,
                            amqp_bytes_t *reply_to_queue)
{
    /* Declare a exclusive private queues with TTL = 10s */
    amqp_table_entry_t entries[1];
    amqp_table_t table;
    entries[0].key = amqp_cstring_bytes("x-message-ttl");
    entries[0].value.kind = AMQP_FIELD_KIND_U32;
    entries[0].value.value.u32 = 10 * 1000;  // 10s
    table.num_entries = 1;
    table.entries = entries;

    amqp_queue_declare_ok_t *r =
        amqp_queue_declare(*conn, channel, amqp_empty_bytes, 0, 0, 1, 0, table);
    if (!die_on_amqp_error(amqp_get_rpc_reply(*conn),
                           "Declaring the private queue with TTL = 10s"))
        return false;

    *reply_to_queue = amqp_bytes_malloc_dup(r->queue);
    return true;
}

bool set_consuming_queue(amqp_connection_state_t *conn,
                         amqp_channel_t channel,
                         char const *queue_name)
{
    amqp_basic_consume(*conn, channel, amqp_cstring_bytes(queue_name),
                       amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
    if (!die_on_amqp_error(amqp_get_rpc_reply(*conn),
                           "Set the consuming queue"))
        return false;

    return true;
}

bool consume_message(amqp_connection_state_t *conn,
                     amqp_channel_t channel,
                     amqp_envelope_t *envelope)
{
    amqp_maybe_release_buffers(*conn);
    /* Consume a message from the queue in the rabbitmq broker */
    if (!die_on_amqp_error(amqp_consume_message(*conn, envelope, NULL, 0),
                           "Consuming the message"))
        return false;

    return true;
}

bool wait_response_message(amqp_connection_state_t *conn,
                           amqp_channel_t channel,
                           amqp_bytes_t callback_queue,
                           char *frame_body,
                           int body_len)
{
    amqp_frame_t frame;
#if defined(ENABLE_DEBUG)
    amqp_basic_deliver_t *d;
    amqp_basic_properties_t *p;
#endif
    size_t body_target;
    size_t body_received;

    amqp_basic_consume(*conn, channel, callback_queue, amqp_empty_bytes, 0, 1,
                       0, amqp_empty_table);
    if (!die_on_amqp_error(amqp_get_rpc_reply(*conn), "Set the callback queue"))
        return false;

    /*Wait for three frames: AMQP_FRAME_METHOD, AMQP_FRAME_HEADER,
     * AMQP_FRAME_BODY*/
    for (;;) {
        amqp_maybe_release_buffers(*conn);

        struct timeval t = {10, 0}; /* RPC timeout: 10s*/
        if (!die_on_error(amqp_simple_wait_frame_noblock(*conn, &frame, &t),
                          "RPC timeout"))
            return false;

        if (!die_on_amqp_error(amqp_get_rpc_reply(*conn), "Wait method frame"))
            return false;

        ddprintf(MSG_PREFIX "Frame type: %u channel: %u\n", frame.frame_type,
                 frame.channel);

        if (frame.frame_type != AMQP_FRAME_METHOD)
            continue;

        ddprintf(MSG_PREFIX "Method: %s\n",
                 amqp_method_name(frame.payload.method.id));

        if (frame.payload.method.id != AMQP_BASIC_DELIVER_METHOD)
            continue;

#if defined(ENABLE_DEBUG)
        d = (amqp_basic_deliver_t *) frame.payload.method.decoded;
        ddprintf(MSG_PREFIX "Delivery: %u exchange: %.*s routingkey: %.*s\n",
                 (unsigned) d->delivery_tag, (int) d->exchange.len,
                 (char *) d->exchange.bytes, (int) d->routing_key.len,
                 (char *) d->routing_key.bytes);
#endif

        amqp_maybe_release_buffers(*conn);

        if (!die_on_error(amqp_simple_wait_frame_noblock(*conn, &frame, &t),
                          "RPC timeout"))
            return false;

        if (!die_on_amqp_error(amqp_get_rpc_reply(*conn), "Wait header frame"))
            return false;

        if (frame.frame_type != AMQP_FRAME_HEADER) {
            ddprintf("Unexpected header!\n");
            return false;
        }

#if defined(ENABLE_DEBUG)
        p = (amqp_basic_properties_t *) frame.payload.properties.decoded;
        if (p->_flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
            ddprintf(MSG_PREFIX "Content-type: %.*s\n",
                     (int) p->content_type.len, (char *) p->content_type.bytes);
        }
#endif
        ddprintf("---\n");

        body_target = (size_t) frame.payload.properties.body_size;
        body_received = 0;
        while (body_received < body_target) {
            if (!die_on_error(amqp_simple_wait_frame_noblock(*conn, &frame, &t),
                              "RPC timeout"))
                return false;

            if (!die_on_amqp_error(amqp_get_rpc_reply(*conn),
                                   "Wait body frame"))
                return false;

            if (frame.frame_type != AMQP_FRAME_BODY) {
                ddprintf("Unexpected body\n");
                return false;
            }

            body_received += frame.payload.body_fragment.len;
        }
        if (body_received != body_target) {
            ddprintf("Received body is smaller than body target\n");
            return false;
        }

        memcpy(frame_body, (char *) frame.payload.body_fragment.bytes,
               body_len);

        ddprintf(MSG_PREFIX "PoW result: %.*s\n",
                 (int) frame.payload.body_fragment.len,
                 (char *) frame.payload.body_fragment.bytes);
        ddprintf("---\n");

        /* everything was fine, we can quit now because we received the reply */
        return true;
    }
}

bool publish_message(amqp_connection_state_t *conn,
                     amqp_channel_t channel,
                     char *queue_name,
                     char *message)
{
    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("text/plain");
    props.delivery_mode = AMQP_DELIVERY_PERSISTENT;

    /* Publish messages by default exchange */
    if (!die_on_error(amqp_basic_publish(*conn, channel, amqp_cstring_bytes(""),
                                         amqp_cstring_bytes(queue_name), 0, 0,
                                         &props, amqp_cstring_bytes(message)),
                      "Publish the message"))
        return false;

    return true;
}

bool publish_message_with_reply_to(amqp_connection_state_t *conn,
                                   amqp_channel_t channel,
                                   char *queue_name,
                                   amqp_bytes_t reply_to_queue,
                                   char *message)
{
    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG |
                   AMQP_BASIC_DELIVERY_MODE_FLAG | AMQP_BASIC_REPLY_TO_FLAG;
    props.content_type = amqp_cstring_bytes("text/plain");
    props.delivery_mode = AMQP_DELIVERY_PERSISTENT;
    props.reply_to = amqp_bytes_malloc_dup(reply_to_queue);

    if (!die_on_error(amqp_basic_publish(*conn, channel, amqp_cstring_bytes(""),
                                         amqp_cstring_bytes(queue_name), 0, 0,
                                         &props, amqp_cstring_bytes(message)),
                      "Publishing the message with reply_to"))
        return false;

    ddprintf(MSG_PREFIX "callback queue %s \n", (char *) props.reply_to.bytes);
    amqp_bytes_free(props.reply_to);

    return true;
}

bool acknowledge_broker(amqp_connection_state_t *conn,
                        amqp_channel_t channel,
                        amqp_envelope_t *envelope)
{
    /* Make sure a message is never lost */
    if (!die_on_error(amqp_basic_ack(*conn, channel, envelope->delivery_tag, 0),
                      "Acknowledging the broker"))
        return false;

    return true;
}

void disconnect_broker(amqp_connection_state_t *conn)
{
    amqp_channel_close(*conn, 1, AMQP_REPLY_SUCCESS);
    amqp_connection_close(*conn, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(*conn);
}
