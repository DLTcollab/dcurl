#include "remote_common.h"

REMOTE_STATE connect_broker(amqp_connection_state_t *conn, int channel)
{
    amqp_socket_t *socket = NULL;
    REMOTE_STATE remote_state = REMOTE_SUCCESS;
    amqp_rpc_reply_t rpc_reply;

    /* Connect to the rabbitmq broker */
    *conn = amqp_new_connection();
    socket = amqp_tcp_socket_new(*conn);
    if (amqp_socket_open(socket, HOSTNAME, PORT) != AMQP_STATUS_OK) {
        remote_state = BROKER_CLOSE;
        goto fail_to_broker_close;
    }

    /* Login to the rabbitmq broker */
    rpc_reply = amqp_login(*conn, "/", AMQP_DEFAULT_MAX_CHANNELS,
                           AMQP_DEFAULT_FRAME_SIZE, 0, AMQP_SASL_METHOD_PLAIN,
                           "guest", "guest");
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        remote_state = LOGIN_ERROR;
        goto fail_to_login_error;
    }

    /* Open a channel in the rabbitmq broker */
    amqp_channel_open(*conn, channel);
    rpc_reply = amqp_get_rpc_reply(*conn);
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        remote_state = CHANNEL_ERROR;
        goto fail_to_channel_error;
    }

    return remote_state;

fail_to_channel_error:
    amqp_channel_close(*conn, channel, AMQP_REPLY_SUCCESS);

fail_to_login_error:
    amqp_connection_close(*conn, AMQP_REPLY_SUCCESS);

fail_to_broker_close:
    amqp_destroy_connection(*conn);

    return remote_state;
}

REMOTE_STATE set_consume_queue(amqp_connection_state_t *conn,
                               int channel,
                               char const *queue_name)
{
    REMOTE_STATE remote_state = REMOTE_SUCCESS;
    amqp_rpc_reply_t rpc_reply;

    amqp_basic_consume(*conn, channel, amqp_cstring_bytes(queue_name),
                       amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
    rpc_reply = amqp_get_rpc_reply(*conn);
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        disconnect_broker(conn, channel);
        return QUEUE_NOT_EXIST;
    }

    return remote_state;
}

REMOTE_STATE consume_message(amqp_connection_state_t *conn,
                             int channel,
                             amqp_envelope_t *envelope)
{
    REMOTE_STATE remote_state = REMOTE_SUCCESS;
    amqp_rpc_reply_t rpc_reply;

    /* Consume a message from the queue in the rabbitmq broker */
    amqp_maybe_release_buffers(*conn);
    rpc_reply = amqp_consume_message(*conn, envelope, NULL, 0);
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        disconnect_broker(conn, channel);
        return CONSUME_ERROR;
    }

    return remote_state;
}

REMOTE_STATE ack_to_broker(amqp_connection_state_t *conn,
                           int channel,
                           amqp_envelope_t *envelope)
{
    REMOTE_STATE remote_state = REMOTE_SUCCESS;

    /* Make sure a message is never lost */
    if (amqp_basic_ack(*conn, channel, envelope->delivery_tag, 0) != 0) {
        disconnect_broker(conn, channel);
        return ACK_ERROR;
    }

    amqp_destroy_envelope(envelope);

    return remote_state;
}

REMOTE_STATE declare_queue(amqp_connection_state_t *conn,
                           int channel,
                           char const *queue_name)
{
    REMOTE_STATE remote_state = REMOTE_SUCCESS;
    amqp_rpc_reply_t rpc_reply;
    
    /* Declare a durable queue */
    amqp_queue_declare(*conn, channel, amqp_cstring_bytes(queue_name), 0, 1, 0,
                       0, amqp_empty_table);
    rpc_reply = amqp_get_rpc_reply(*conn);
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        disconnect_broker(conn, channel);
        return DECLARE_QUEUE_ERROR;
    }

    return remote_state;
}

REMOTE_STATE publish_message(amqp_connection_state_t *conn,
                             int channel,
                             char const *queue_name,
                             char *message)
{
    REMOTE_STATE remote_state = REMOTE_SUCCESS;

    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("text/plain");
    props.delivery_mode = AMQP_DELIVERY_PERSISTENT;  // Persistent delivery mode

    /* Publish messages by default exchange */
    if (amqp_basic_publish(*conn, channel, amqp_cstring_bytes(""),
                           amqp_cstring_bytes(queue_name), 0, 0, &props,
                           amqp_cstring_bytes(message)) != AMQP_STATUS_OK) {
        disconnect_broker(conn, channel);
        return PUBLISH_ERROR;
    }

    return remote_state;
}

void disconnect_broker(amqp_connection_state_t *conn, int channel)
{
    amqp_channel_close(*conn, channel, AMQP_REPLY_SUCCESS);
    amqp_connection_close(*conn, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(*conn);
}
