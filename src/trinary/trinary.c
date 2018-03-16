/*
 * Copyright (C) 2018 dcurl Developers.
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE file.
 */

#include "trinary.h"
#include "../constants.h"
#include "../hash/curl.h"

static char TrytesToTritsMappings[][3] = {
    {0, 0, 0},  {1, 0, 0},  {-1, 1, 0},   {0, 1, 0},   {1, 1, 0},   {-1, -1, 1},
    {0, -1, 1}, {1, -1, 1}, {-1, 0, 1},   {0, 0, 1},   {1, 0, 1},   {-1, 1, 1},
    {0, 1, 1},  {1, 1, 1},  {-1, -1, -1}, {0, -1, -1}, {1, -1, -1}, {-1, 0, -1},
    {0, 0, -1}, {1, 0, -1}, {-1, 1, -1},  {0, 1, -1},  {1, 1, -1},  {-1, -1, 0},
    {0, -1, 0}, {1, -1, 0}, {-1, 0, 0}};

void freeTrobject(Trobject_t *t)
{
    if (t) {
        if (t->data)
            free(t->data);
        free(t);
    }
}

static int validateTrits(Trobject_t *trits)
{
    if (trits->type != TYPE_TRITS)
        return 0;

    for (int i = 0; i < trits->len; i++)
        if (trits->data[i] < -1 || trits->data[i] > 1)
            return 0;
    return 1;
}

static int validateTrytes(Trobject_t *trytes)
{
    if (trytes->type != TYPE_TRYTES)
        return 0;

    for (int i = 0; i < trytes->len; i++)
        if ((trytes->data[i] <= 'A' && trytes->data[i] >= 'Z') &&
            trytes->data[i] != '9')
            return 0;
    return 1;
}

Trobject_t *initTrits(signed char *src, int len)
{
    Trobject_t *trits = NULL;

    trits = (Trobject_t *) malloc(sizeof(Trobject_t));
    if (!trits)
        return NULL;

    trits->data = (signed char *) malloc(len + 1);
    if (!trits->data) {
        free(trits);
        return NULL;
    }

    /* Copy data from src to Trits */
    memcpy(trits->data, src, len);

    trits->type = TYPE_TRITS;
    trits->len = len;
    trits->data[len] = '\0';

    /* Check validation */
    if (!validateTrits(trits)) {
        freeTrobject(trits);
        /* Not availabe src */
        return NULL;
    }

    return trits;
}

Trobject_t *initTrytes(signed char *src, int len)
{
    Trobject_t *trytes = NULL;

    trytes = (Trobject_t *) malloc(sizeof(Trobject_t));
    if (!trytes) {
        return NULL;
    }

    trytes->data = (signed char *) malloc(len + 1);
    if (!trytes->data) {
        free(trytes);
        return NULL;
    }

    /* Copy data from src to Trytes */
    memcpy(trytes->data, src, len);

    trytes->type = TYPE_TRYTES;
    trytes->len = len;
    trytes->data[len] = '\0';

    /* Check validation */
    if (!validateTrytes(trytes)) {
        freeTrobject(trytes);
        /* Not available src */
        return NULL;
    }

    return trytes;
}

Trobject_t *trytes_from_trits(Trobject_t *trits)
{
    if (!trits) {
        return NULL;
    }

    if (trits->len % 3 != 0 || !validateTrits(trits)) {
        /* Not available trits to convert */
        return NULL;
    }

    Trobject_t *trytes = NULL;
    signed char *src = (signed char *) malloc(trits->len / 3);

    /* Start converting */
    for (int i = 0; i < trits->len / 3; i++) {
        int j = trits->data[i * 3] + trits->data[i * 3 + 1] * 3 +
                trits->data[i * 3 + 2] * 9;

        if (j < 0)
            j += 27;
        src[i] = TryteAlphabet[j];
    }

    trytes = initTrytes(src, trits->len / 3);
    free(src);

    return trytes;
}

Trobject_t *trits_from_trytes(Trobject_t *trytes)
{
    if (!trytes)
        return NULL;

    if (!validateTrytes(trytes)) {
        /* trytes is not available to convert */
        return NULL;
    }

    Trobject_t *trits = NULL;
    signed char *src = (signed char *) malloc(trytes->len * 3);

    /* Start converting */
    for (int i = 0; i < trytes->len; i++) {
        int idx = (trytes->data[i] == '9') ? 0 : trytes->data[i] - 'A' + 1;
        for (int j = 0; j < 3; j++) {
            src[i * 3 + j] = TrytesToTritsMappings[idx][j];
        }
    }

    trits = initTrits(src, trytes->len * 3);
    free(src);

    return trits;
}

Trobject_t *hashTrytes(Trobject_t *t)
{
    if (t->type != TYPE_TRYTES)
        return NULL;

    Curl *c = initCurl();
    Absorb(c, t);
    Trobject_t *ret = Squeeze(c);

    freeCurl(c);
    return ret;
}

int compareTrobject(Trobject_t *a, Trobject_t *b)
{
    if (a->type != b->type || a->len != b->len)
        return 0;

    for (int i = 0; i < a->len; i++)
        if (a->data[i] != b->data[i])
            return 0;

    return 1;
}
