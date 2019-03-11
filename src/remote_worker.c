#include "remote_common.h"

#include "constants.h"
#include "dcurl.h"

int main(int argc, char const *const *argv)
{
    char trytes[TRANSACTION_TRYTES_LENGTH];
    char buf[4];
    int mwm;

    amqp_connection_state_t conn;
    amqp_envelope_t envelope;
    REMOTE_STATE remote_state = REMOTE_SUCCESS;

    dcurl_init();

    remote_state = connect_broker(&conn, CHANNEL);
    if (remote_state == BROKER_CLOSE) {
        printf("The rabbitmq broker is closed\n");
        goto fail_to_exit;
    } else if (remote_state == LOGIN_ERROR) {
        printf("Login error \n");
        goto fail_to_exit;
    } else if (remote_state == LOGIN_ERROR) {
        printf("Opennng the channel fails\n");
        goto fail_to_exit;
    }

    remote_state = set_consume_queue(&conn, CHANNEL, "incomming_queue");
    if (remote_state == QUEUE_NOT_EXIST) {
        printf("Incomming queue does not exsit\n");
        goto fail_to_exit;
    }

    for (;;) {
        remote_state = consume_message(&conn, CHANNEL, &envelope);
        if (remote_state == CONSUME_ERROR) {
            printf("Consuming a message fails\n");
        }
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

        remote_state = ack_to_broker(&conn, CHANNEL, &envelope);
        if (remote_state == ACK_ERROR) {
            printf("Sending a ack fails\n");
            goto fail_to_exit;
        }
        printf("Sending a ack is done\n");

        remote_state = declare_queue(&conn, CHANNEL, "completed_queue");
        if (remote_state == DECLARE_QUEUE_ERROR) {
            printf("Declaring the completed queue fails\n");
            goto fail_to_exit;
        }

        /* Publish a message of remote PoW result */
        remote_state = publish_message(&conn, CHANNEL, "completed_queue",
                                       (char *) ret_trytes);
        if (remote_state == PUBLISH_ERROR) {
            printf("Publishing trytes to the completed queue fails\n");
            goto fail_to_exit;
        }

        free(ret_trytes);
        printf("Publishing PoW result to compeleted queue is done\n");
        printf("---\n");
    }

fail_to_exit:
    dcurl_destroy();

    return 0;
}
