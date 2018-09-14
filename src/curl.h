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
