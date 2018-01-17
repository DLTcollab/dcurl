#ifndef TRINARY_H_
#define TRINARY_H_

#include <stdio.h>
#include <stdlib.h>

extern char tryteToTritsMappings[][3];

typedef struct _trits Trits;

typedef struct _trytes Trytes;

struct _trits {
    char *data;
    int len;
    void (*toTrits)(Trits *thiz, char *orig, int len);
    int (*Equal)(Trits *thiz, Trits *b);
    long long (*Int)(Trits *thiz);
    Trytes *(*toTrytes)(Trits *thiz);
};

void init_Trits(Trits **t);

struct _trytes {
    char *data;
    int len;
    void (*toTrytes)(Trytes *thiz, char *orig, int len);
    Trits *(*toTrits)(Trytes *thiz);
    Trytes *(*Hash)(Trytes *thiz);
};

void init_Trytes(Trytes **t);

#endif
