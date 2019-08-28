/*
 * Copyright (C) 2019 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#ifndef REMOTE_COMMON_H_
#define REMOTE_COMMON_H_

#include <stdbool.h>
#include <stdint.h>
#include "amqp.h"
#include "amqp_tcp_socket.h"

#define MSG_PREFIX "[dcurl-remote] "

bool connect_broker(amqp_connection_state_t *conn, const char *hostName);

bool declare_queue(amqp_connection_state_t *conn,
                   amqp_channel_t channel,
                   char const *queue_name);

bool declare_callback_queue(amqp_connection_state_t *conn,
                            amqp_channel_t channel,
                            amqp_bytes_t *reply_to_queue);

bool set_consuming_queue(amqp_connection_state_t *conn,
                         amqp_channel_t channel,
                         char const *queue_name);

bool consume_message(amqp_connection_state_t *conn,
                     amqp_channel_t channel,
                     amqp_envelope_t *envelope);

bool wait_response_message(amqp_connection_state_t *conn,
                           amqp_channel_t channel,
                           amqp_bytes_t callback_queue,
                           char *frame_body,
                           int body_len);

bool publish_message(amqp_connection_state_t *conn,
                     amqp_channel_t channel,
                     char *queue_name,
                     char *message);

bool publish_message_with_reply_to(amqp_connection_state_t *conn,
                                   amqp_channel_t channel,
                                   char *queue_name,
                                   amqp_bytes_t reply_to_queue,
                                   char *message);

bool acknowledge_broker(amqp_connection_state_t *conn,
                        amqp_channel_t channel,
                        amqp_envelope_t *envelope);

void disconnect_broker(amqp_connection_state_t *conn);

#endif
