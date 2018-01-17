#ifndef CURL_H_
#define CURL_H_

#include "trinary.h"

#define stateSize 729
#define numberOfRounds 81

typedef struct _curl Curl;

struct _curl {
    Trits *state;
    Trytes *(*Squeeze)(Curl *thiz);
    void (*Absorb)(Curl *thiz, Trytes *inn);
    void (*Transform)(Curl *thiz);

};

Curl *NewCurl();

#endif
