/*
 * Copyright (C) 2018 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#ifndef CURL_H_
#define CURL_H_

#include "constants.h"
#include "trinary.h"

typedef struct curl_s {
    trits_t *state;
} curl_t;

void absorb(curl_t *c, trytes_t *inn);
void transform(curl_t *c);
trytes_t *squeeze(curl_t *c);

curl_t *init_curl();
void free_curl(curl_t *c);

#endif
