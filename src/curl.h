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

typedef struct _curl {
    Trits_t *state;
} Curl;

void Absorb(Curl *c, Trytes_t *inn);
void Transform(Curl *c);
Trytes_t *Squeeze(Curl *c);

Curl *initCurl();
void freeCurl(Curl *c);

#endif
