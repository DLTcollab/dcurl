#include "common.h"

#include "remote_common.h"

int main(int argc, char const *const *argv)
{
    char ret_trytes[TRANSACTION_TRYTES_LENGTH];
    int mwm = 9;

    amqp_connection_state_t conn;
    amqp_envelope_t envelope;
    REMOTE_STATE remote_state = REMOTE_SUCCESS;

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

    remote_state = set_consume_queue(&conn, CHANNEL, "completed_queue");
    if (remote_state == QUEUE_NOT_EXIST) {
        printf("Completed queue does not exsit\n");
        goto fail_to_exit;
    }

    for (;;) {
        /* Consume a message of remote dcurl result from completed queue in the
         * rabbitmq broker */
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

        /* Message body format: transacton */
        memcpy(ret_trytes, envelope.message.body.bytes,
               TRANSACTION_TRYTES_LENGTH);

        remote_state = ack_to_broker(&conn, CHANNEL, &envelope);
        if (remote_state == ACK_ERROR) {
            printf("Sending a ack fails\n");
            goto fail_to_exit;
        }
        printf("Sending a ack is done\n");

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

fail_to_exit:

    return 0;
}
