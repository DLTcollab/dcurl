/*
 * Copyright (C) 2018 dcurl Developers.
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE file.
 */

#include "curl.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static const int8_t truthTable[11] = {1, 0, -1, 2, 1, -1, 0, 2, -1, 1, 0};

static void _transform(int8_t state[])
{
    int r = 0, i = 0;
    int8_t copy[STATE_TRITS_LENGTH] = {0};
    int8_t *from = state, *to = copy;
    for (r = 0; r < 81; r++) {
        for (i = 0; i < STATE_TRITS_LENGTH; i++) {
            int aa = indices[i];
            int bb = indices[i + 1];
            to[i] = truthTable[from[aa] + (from[bb] * 4) + 5];
        }
        int8_t *tmp = from;
        from = to;
        to = tmp;
    }
    memcpy(state, copy, STATE_TRITS_LENGTH);
}

void Transform(Curl *c)
{
    _transform(c->state->data);
}

void Absorb(Curl *c, Trytes_t *inn)
{
    Trits_t *in = trits_from_trytes(inn);
    int lenn = 0;

    for (int i = 0; i < in->len; i += lenn) {
        lenn = 243;
        if (in->len - i < 243)
            lenn = in->len - i;

        /* Copy in[i, i + lenn] to c->state->data[0, lenn] */
        memcpy(c->state->data, in->data + i, lenn);

        Transform(c);
    }
    freeTrobject(in);
}

Trytes_t *Squeeze(Curl *c)
{
    int8_t src[HASH_TRITS_LENGTH] = {0};

    /* Get trits[:HASH_TRITS_LENGTH] to an array */
    memcpy(src, c->state->data, HASH_TRITS_LENGTH);

    Trits_t *trits = initTrits(src, HASH_TRITS_LENGTH);
    Trytes_t *trytes = trytes_from_trits(trits);

    Transform(c);
    freeTrobject(trits);

    return trytes;
}

Curl *initCurl()
{
    Curl *c = (Curl *) malloc(sizeof(Curl));
    if (!c)
        return NULL;

    int8_t src[STATE_TRITS_LENGTH] = {0};
    c->state = initTrits(src, STATE_TRITS_LENGTH);

    return c;
}

void freeCurl(Curl *c)
{
    if (c) {
        freeTrobject(c->state);
        free(c);
    }
}
