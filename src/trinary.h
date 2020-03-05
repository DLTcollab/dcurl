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

typedef struct trinary_object_s {
    int8_t *data;
    int len;
    int type;
} trinary_object_t;

typedef trinary_object_t trits_t;
typedef trinary_object_t trytes_t;

trinary_object_t *init_trits(int8_t *src, int len);
trinary_object_t *init_trytes(int8_t *src, int len);

trinary_object_t *trytes_from_trits(trinary_object_t *trits);
trinary_object_t *trits_from_trytes(trinary_object_t *trytes);
trinary_object_t *hash_trytes(trinary_object_t *t);

bool compare_trinary_object(trinary_object_t *a, trinary_object_t *b);
void free_trinary_object(trinary_object_t *t);

#endif
