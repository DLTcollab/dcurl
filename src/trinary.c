#include "trinary.h"
#include "curl.h"
#include <stdio.h>
#include "constants.h"

char tryteToTritsMappings[][3] = {
     {0, 0, 0}, {1, 0, 0}, {-1, 1, 0}, {0, 1, 0},
     {1, 1, 0}, {-1, -1, 1}, {0, -1, 1}, {1, -1, 1},
     {-1, 0, 1}, {0, 0, 1}, {1, 0, 1}, {-1, 1, 1},
     {0, 1, 1}, {1, 1, 1}, {-1, -1, -1}, {0, -1, -1},
     {1, -1, -1}, {-1, 0, -1}, {0, 0, -1}, {1, 0, -1},
     {-1, 1, -1}, {0, 1, -1}, {1, 1, -1}, {-1, -1, 0},
     {0, -1, 0}, {1, -1, 0}, {-1, 0, 0}
};

static void isValid_Trits(Trits *thiz)
{
    for (int i = 0; i < thiz->len; i++) {
        if (thiz->data[i] < -1 || thiz->data[i] > 1) {
            printf("trinary.c: isValid_Trits: illegal Trits");
            exit(0);
        }
    }
}

static void toTrits(Trits *thiz, char *orig, int len)
{
    thiz->data = (char *) malloc(len + 1);
    thiz->data[len] = '\0';
    thiz->len = len;

    if(!thiz->data) {
        printf("trinary.c: toTrits: Malloc Unavailable\n");
        exit(0);
    }

    for (int i = 0; i < len; i++)
        thiz->data[i] = orig[i];
    
    isValid_Trits(thiz);
}

static int Equal(Trits *thiz, Trits *b)
{
    if (thiz->len != b->len)
        return -1;
    
    for (int i = 0; i < thiz->len; i++) {
        if (thiz->data[i] != b->data[i])
            return -1;
    }

    return 1;
}

static long long Int(Trits *thiz)
{
    long long ret = 0;
    
    for (int i = thiz->len - 1; i >= 0; i--) {
        ret = ret * 3 + thiz->data[i];
    }
    
    return ret;
}

static int CanTrytes(Trits *thiz)
{
    if (thiz->len % 3 == 0)
        return 1;
    return -1;
}

static Trytes *TritstoTrytes(Trits *thiz)
{
    if (!CanTrytes(thiz)) {
        printf("trinary.c: toTrytes: Length of trits must be a mmultiple of three\n");
        exit(0);
    }

    Trytes *ret = NULL;
    init_Trytes(&ret);

    /* Prepare array passing to Trytes */
    char *tyt = (char *) malloc(thiz->len / 3);

    for (int i = 0; i < thiz->len / 3; i++) {
        char *curr = thiz->data;
        int j = curr[i * 3] + curr[i * 3 + 1] * 3 + curr[i * 3 + 2] * 9;
        
        if (j < 0) j += 27;

        tyt[i] = TryteAlphabet[j];
    }

    ret->toTrytes(ret, tyt, thiz->len / 3);
    return ret;
}

void init_Trits(Trits **t)
{
    (*t) = (Trits *) malloc(sizeof(Trits));

    if (!(*t)) {
        printf("trinary.c: init_Trits: Malloc Unavailable\n");
        exit(0);
    }

    (*t)->data = NULL;
    (*t)->len = 0;
    (*t)->toTrits = toTrits;
    (*t)->Equal = Equal;
    (*t)->Int = Int;
    (*t)->toTrytes = TritstoTrytes;
}

static void isValid_Trytes(Trytes *thiz)
{
    for (int i = 0; i < thiz->len; i++) {
        if ((thiz->data[i] <= 'A' && thiz->data[i] >= 'Z') && \
            thiz->data[i] != '9') {
            printf("trinary.c: isValid_Trytes: Invalid character\n");
            exit(0);
        }
    }
}

static void toTrytes(Trytes *thiz, char *orig, int len)
{
    thiz->data = (char *) malloc(len + 1);
    thiz->data[len] = '\0';
    thiz->len = len;

    if(!thiz->data) {
        printf("trinary.c: toTrytes: Malloc Unavailable\n");
        exit(0);
    }

    for (int i = 0; i < len; i++)
        thiz->data[i] = orig[i];

    isValid_Trytes(thiz);
}

static int string_index(char *str, char target, int max_len)
{
    for (int i = 0; i < max_len; i++) {
        if (str[i] == target)
            return i;
    }

    printf("trinary.c: string_index: Unavailbe target\n");
    exit(0);
}

static Trits *TrytestoTrits(Trytes *thiz)
{
    Trits *ret = NULL;
    init_Trits(&ret);

    /* Prepare array passing to Trits */
    char *tyt = (char *) malloc(thiz->len * 3);

    for (int i = 0; i < thiz->len; i++) {
        int idx = string_index(TryteAlphabet, thiz->data[i], 27);
        /* Copy tryte mapping to this character */
        for (int j = 0; j < 3; j++) {
            tyt[i * 3 + j] = tryteToTritsMappings[idx][j];
        }
    }

    ret->toTrits(ret, tyt, thiz->len * 3);
    return ret;
}

static Trytes *Hash(Trytes *thiz)
{
    Curl *c = NewCurl();
    c->Absorb(c, thiz);
    return c->Squeeze(c);
}

void init_Trytes(Trytes **t)
{
    (*t) = (Trytes *) malloc(sizeof(Trytes));

    if (!(*t)) {
        printf("trinary.c: init_Trytes: Malloc Unavailable\n");
        exit(0);
    }

    (*t)->data = NULL;
    (*t)->len = 0;
    (*t)->toTrytes = toTrytes;
    (*t)->toTrits = TrytestoTrits;
    (*t)->Hash = Hash;
}

