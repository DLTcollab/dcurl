#ifndef CURL_H_
#define CURL_H_

#include "trinary.h"

#define HASH_LENGTH 243
#define STATE_LENGTH 3 * HASH_LENGTH

typedef struct _curl {
    Trits_t *state;
} Curl;

void Absorb(Curl *c, Trytes_t *inn);
void Transform(Curl *c);
Trytes_t *Squeeze(Curl *c);

Curl *initCurl();
void freeCurl(Curl *c);

#endif
