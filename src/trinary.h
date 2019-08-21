/*
 * Copyright (C) 2018 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#ifndef TRINARY_H_
#define TRINARY_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TYPE_TRITS 1
#define TYPE_TRYTES 2

typedef struct _trinary_object {
    int8_t *data;
    int len;
    int type;
} Trobject_t;

typedef Trobject_t Trits_t;
typedef Trobject_t Trytes_t;

Trobject_t *initTrits(int8_t *src, int len);
Trobject_t *initTrytes(int8_t *src, int len);

Trobject_t *trytes_from_trits(Trobject_t *trits);
Trobject_t *trits_from_trytes(Trobject_t *trytes);
Trobject_t *hashTrytes(Trobject_t *t);

bool compareTrobject(Trobject_t *a, Trobject_t *b);
void freeTrobject(Trobject_t *t);

#endif
