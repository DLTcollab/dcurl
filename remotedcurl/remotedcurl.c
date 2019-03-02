#include <assert.h>

#include "amqp.h"
#include "amqp_tcp_socket.h"

#include "constants.h"
#include "dcurl.h"

#define HOSTNAME "localhost"
#define PORT 5672
#define HEATBEATS 0  // disables heartbeats
#define ACCOUNT "guest"
#define PASSWORD "guest"
#define INCOMMING_QUEUE "incomming_queue"
#define COMPLETED_QUEUE "completed_queue"
#define CHANNEL 1
#define PASSIVE 0
#define DURABLE 1
#define EXCLUSIVE 0
#define AUTODELETE 0
#define EXCHANGE ""  // default exchange

int main(int argc, char const *const *argv)
{
    char trytes[TRANSACTION_TRYTES_LENGTH];
    char buf[4];
    int mwm;

    amqp_socket_t *socket = NULL;
    amqp_connection_state_t conn;
    amqp_rpc_reply_t rpc_reply;
    amqp_envelope_t envelope;
    amqp_bytes_t queuename;

    dcurl_init();

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

    amqp_basic_consume(conn, CHANNEL, amqp_cstring_bytes(INCOMMING_QUEUE),
                       amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
    rpc_reply = amqp_get_rpc_reply(conn);
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        printf("Incomming queue does not exsit \n");
        exit(1);
    }

    for (;;) {
        amqp_maybe_release_buffers(conn);

        /* Consume a message from incomming queue in the rabbitmq broker */
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

        /* Message body format: transacton | mwm */
        memcpy(trytes, envelope.message.body.bytes, TRANSACTION_TRYTES_LENGTH);
        memcpy(buf, envelope.message.body.bytes + TRANSACTION_TRYTES_LENGTH, 4);
        sscanf(buf, "%d", &mwm);

        printf("Doing PoW ...\n");
        int8_t *ret_trytes = dcurl_entry((int8_t *) trytes, mwm, 0);
        printf("PoW is done\n");

        /* Make sure a message is never lost */
        assert(amqp_basic_ack(conn, CHANNEL, envelope.delivery_tag, 0) == 0);
        printf("Sending a ack is done\n");

        amqp_destroy_envelope(&envelope);

        /* Declare completed queue */
        amqp_queue_declare_ok_t *res = amqp_queue_declare(
            conn, CHANNEL, amqp_cstring_bytes(COMPLETED_QUEUE), PASSIVE,
            DURABLE, EXCLUSIVE, AUTODELETE, amqp_empty_table);
        rpc_reply = amqp_get_rpc_reply(conn);
        assert(rpc_reply.reply_type == AMQP_RESPONSE_NORMAL);
        queuename = amqp_bytes_malloc_dup(res->queue);
        if (queuename.bytes == NULL) {
            printf("Out of memory while copying queue name");
            exit(1);
        }

        /* Publish a message of remote PoW result */
        amqp_basic_properties_t props;
        props._flags =
            AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
        props.content_type = amqp_cstring_bytes("text/plain");
        props.delivery_mode =
            AMQP_DELIVERY_PERSISTENT;  // Persistent delivery mode
        assert(amqp_basic_publish(
                   conn, CHANNEL, amqp_cstring_bytes(EXCHANGE),
                   amqp_cstring_bytes(COMPLETED_QUEUE), 0, 0, &props,
                   amqp_cstring_bytes((char *) ret_trytes)) == AMQP_STATUS_OK);
        free(ret_trytes);
        printf("Publishing PoW result to compeleted queue is done\n");
        printf("---\n");
    }

    // Leave the rabbitmq broker
    rpc_reply = amqp_channel_close(conn, CHANNEL, AMQP_REPLY_SUCCESS);
    rpc_reply = amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
    assert(rpc_reply.reply_type == AMQP_RESPONSE_NORMAL);
    assert(amqp_destroy_connection(conn) == AMQP_STATUS_OK);
    printf("End coonection\n");

    dcurl_destroy();

    return 0;
}
