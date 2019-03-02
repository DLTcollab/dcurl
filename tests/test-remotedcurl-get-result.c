#include "common.h"

#include "amqp.h"
#include "amqp_tcp_socket.h"

#define HOSTNAME "localhost"
#define PORT 5672
#define HEATBEATS 0  // disables heartbeats
#define ACCOUNT "guest"
#define PASSWORD "guest"
#define COMPLETED_QUEUE "completed_queue"
#define CHANNEL 1

int main(int argc, char const *const *argv)
{
    char ret_trytes[TRANSACTION_TRYTES_LENGTH];
    int mwm = 9;

    amqp_socket_t *socket = NULL;
    amqp_connection_state_t conn;
    amqp_rpc_reply_t rpc_reply;
    amqp_envelope_t envelope;

    /* Connect to the rabbitmq broker */
    conn = amqp_new_connection();
    assert(conn != NULL);
    socket = amqp_tcp_socket_new(conn);
    assert(socket != NULL);
    if (amqp_socket_open(socket, HOSTNAME, PORT) != AMQP_STATUS_OK) {
        printf("Is rabbitmq broker running?\n");
        exit(1);
    }

    /* Login to the rabbitmq broker */
    rpc_reply = amqp_login(conn, "/", AMQP_DEFAULT_MAX_CHANNELS,
                           AMQP_DEFAULT_FRAME_SIZE, HEATBEATS,
                           AMQP_SASL_METHOD_PLAIN, ACCOUNT, PASSWORD);
    assert(rpc_reply.reply_type == AMQP_RESPONSE_NORMAL);

    /* Open a channel in the rabbitmq broker */
    amqp_channel_open(conn, CHANNEL);
    rpc_reply = amqp_get_rpc_reply(conn);
    assert(rpc_reply.reply_type == AMQP_RESPONSE_NORMAL);

    amqp_basic_consume(conn, CHANNEL, amqp_cstring_bytes(COMPLETED_QUEUE),
                       amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
    rpc_reply = amqp_get_rpc_reply(conn);
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        printf("Completed queue does not exsit \n");
        exit(1);
    }

    for (;;) {
        amqp_maybe_release_buffers(conn);

        /* Consume a message of remote dcurl result from completed queue in the
         * rabbitmq broker */
        rpc_reply = amqp_consume_message(conn, &envelope, NULL, 0);
        assert(rpc_reply.reply_type == AMQP_RESPONSE_NORMAL);
        printf("Delivery %u, exchange %.*s, routingkey %.*s\n",
               (unsigned) envelope.delivery_tag, (int) envelope.exchange.len,
               (char *) envelope.exchange.bytes, (int) envelope.routing_key.len,
               (char *) envelope.routing_key.bytes);
        if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
            printf("Content-type: %.*s\n",
                   (int) envelope.message.properties.content_type.len,
                   (char *) envelope.message.properties.content_type.bytes);
        }

        /* Message body format: transacton */
        memcpy(ret_trytes, envelope.message.body.bytes,
               TRANSACTION_TRYTES_LENGTH);

        /* Make sure a message is never lost */
        assert(amqp_basic_ack(conn, CHANNEL, envelope.delivery_tag, 0) == 0);
        printf("Sending a ack is done\n");

        amqp_destroy_envelope(&envelope);

        /* Validation */
        Trytes_t *trytes_t =
            initTrytes((int8_t *) ret_trytes, TRANSACTION_TRYTES_LENGTH);
        assert(trytes_t);
        Trytes_t *hash_trytes = hashTrytes(trytes_t);
        assert(hash_trytes);
        Trits_t *ret_trits = trits_from_trytes(hash_trytes);
        for (int i = 243 - 1; i >= 243 - mwm; i--) {
            assert(ret_trits->data[i] == 0);
        }
        freeTrobject(trytes_t);
        freeTrobject(hash_trytes);
        freeTrobject(ret_trits);
        printf("Remote PoW result is valid\n");

        printf("---\n");
    }

    /* Leave the rabbitmq broker */
    rpc_reply = amqp_channel_close(conn, CHANNEL, AMQP_REPLY_SUCCESS);
    rpc_reply = amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
    assert(rpc_reply.reply_type == AMQP_RESPONSE_NORMAL);
    assert(amqp_destroy_connection(conn) == AMQP_STATUS_OK);
    printf("End connection\n");

    return 0;
}
