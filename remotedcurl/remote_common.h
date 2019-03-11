#ifndef REMOTE_COMMON_H_
#define REMOTE_COMMON_H_

#include "amqp.h"
#include "amqp_tcp_socket.h"

#define HOSTNAME "localhost"
#define PORT 5672
#define CHANNEL 1

typedef enum {
    REMOTE_SUCCESS = 0,
    BROKER_CLOSE,
    LOGIN_ERROR,
    CHANNEL_ERROR,
    DECLARE_QUEUE_ERROR,
    QUEUE_NOT_EXIST,
    CONSUME_ERROR,
    PUBLISH_ERROR,
    ACK_ERROR
} REMOTE_STATE;

REMOTE_STATE connect_broker(amqp_connection_state_t *conn, int channel);

REMOTE_STATE declare_queue(amqp_connection_state_t *conn,
                           int channel,
                           char const *queue_name);

REMOTE_STATE set_consume_queue(amqp_connection_state_t *conn,
                               int channel,
                               char const *queue_name);

REMOTE_STATE consume_message(amqp_connection_state_t *conn,
                             int channel,
                             amqp_envelope_t *envelope);

REMOTE_STATE publish_message(amqp_connection_state_t *conn,
                             int channel,
                             char const *queue_name,
                             char *message);

REMOTE_STATE ack_to_broker(amqp_connection_state_t *conn,
                           int channel,
                           amqp_envelope_t *envelope);

void disconnect_broker(amqp_connection_state_t *conn, int channel);

#endif
