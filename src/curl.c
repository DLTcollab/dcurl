/*
 * Copyright (C) 2018-2019 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#include "curl.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static const int8_t truth_table[11] = {1, 0, -1, 2, 1, -1, 0, 2, -1, 1, 0};

static void _transform(int8_t state[])
{
    int r, i;
    int8_t copy[STATE_TRITS_LENGTH] = {0};
    int8_t *from = state, *to = copy;
    for (r = 0; r < 81; r++) {
        for (i = 0; i < STATE_TRITS_LENGTH; i++) {
            int aa = indices[i];
            int bb = indices[i + 1];
            to[i] = truth_table[from[aa] + (from[bb] * 4) + 5];
        }
        int8_t *tmp = from;
        from = to;
        to = tmp;
    }
    memcpy(state, copy, STATE_TRITS_LENGTH);
}

void transform(curl_t *c)
{
    _transform(c->state->data);
}

void absorb(curl_t *c, trytes_t *inn)
{
    trits_t *in = trits_from_trytes(inn);
    int lenn = 0;

    if (!in)
        return;

    for (int i = 0; i < in->len; i += lenn) {
        lenn = 243;
        if (in->len - i < 243)
            lenn = in->len - i;

        /* Copy in[i, i + lenn] to c->state->data[0, lenn] */
        memcpy(c->state->data, in->data + i, lenn);

        transform(c);
    }
    free_trinary_object(in);
}

trytes_t *squeeze(curl_t *c)
{
    int8_t src[HASH_TRITS_LENGTH] = {0};

    /* Get trits[:HASH_TRITS_LENGTH] to an array */
    memcpy(src, c->state->data, HASH_TRITS_LENGTH);

    trits_t *trits = init_trits(src, HASH_TRITS_LENGTH);
    trytes_t *trytes = trytes_from_trits(trits);

    transform(c);
    free_trinary_object(trits);

    return trytes;
}

curl_t *init_curl()
{
    curl_t *c = (curl_t *) malloc(sizeof(curl_t));
    if (!c)
        return NULL;

    int8_t src[STATE_TRITS_LENGTH] = {0};
    c->state = init_trits(src, STATE_TRITS_LENGTH);

    return c;
}

void free_curl(curl_t *c)
{
    if (c) {
        free_trinary_object(c->state);
        free(c);
    }
}
