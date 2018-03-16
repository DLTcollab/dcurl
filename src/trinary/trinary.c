#include "trinary.h"
#include "../hash/curl.h"

static char TrytesToTritsMappings[][3] = {
    {0, 0, 0},  {1, 0, 0},  {-1, 1, 0},   {0, 1, 0},   {1, 1, 0},   {-1, -1, 1},
    {0, -1, 1}, {1, -1, 1}, {-1, 0, 1},   {0, 0, 1},   {1, 0, 1},   {-1, 1, 1},
    {0, 1, 1},  {1, 1, 1},  {-1, -1, -1}, {0, -1, -1}, {1, -1, -1}, {-1, 0, -1},
    {0, 0, -1}, {1, 0, -1}, {-1, 1, -1},  {0, 1, -1},  {1, 1, -1},  {-1, -1, 0},
    {0, -1, 0}, {1, -1, 0}, {-1, 0, 0}};

static char TryteAlphabet[] = "9ABCDEFGHIJKLMNOPQRSTUVWXYZ";

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
    if (!trits) {
        printf("trinary.c: initTrits: Malloc Unavailable\n");
        exit(1);
    }

    trits->data = (signed char *) malloc(len + 1);
    if (!trits->data) {
        printf("trinary.c: initTrits: Malloc Unavailable\n");
        exit(1);
    }

    /* Copy data from src to Trits */
    memcpy(trits->data, src, len);

    trits->type = TYPE_TRITS;
    trits->len = len;
    trits->data[len] = '\0';

    /* Check validation */
    if (!validateTrits(trits)) {
        freeTrobject(trits);
        printf("trinary.c: initTrits: Not availabe src!\n");
        exit(1);
    }

    return trits;
}

Trobject_t *initTrytes(signed char *src, int len)
{
    Trobject_t *trytes = NULL;

    trytes = (Trobject_t *) malloc(sizeof(Trobject_t));
    if (!trytes) {
        printf("trinary.c: initTrytes: Malloc Unavailable\n");
        exit(1);
    }

    trytes->data = (signed char *) malloc(len + 1);
    if (!trytes->data) {
        printf("trinary.c: initTrytes: Malloc Unavailable\n");
        exit(1);
    }

    /* Copy data from src to Trytes */
    memcpy(trytes->data, src, len);

    trytes->type = TYPE_TRYTES;
    trytes->len = len;
    trytes->data[len] = '\0';

    /* Check validation */
    if (!validateTrytes(trytes)) {
        freeTrobject(trytes);
        printf("trinary.c: initTrytes: Not available src!\n");
        exit(1);
    }

    return trytes;
}

Trobject_t *trytes_from_trits(Trobject_t *trits)
{
    if (!trits) {
        printf("trinary.c: trytes_from_trits: trits not initialized\n");
        exit(1);
    }

    if (trits->len % 3 != 0 || !validateTrits(trits)) {
        printf(
            "trinary.c: trytes_from_trits: Not available trits to convert\n");
        exit(1);
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
    if (!trytes) {
        printf("trinary.c: trits_from_trytes: trytes not initialized\n");
        exit(1);
    }

    if (!validateTrytes(trytes)) {
        printf(
            "trinary.c: trits_from_trytes: trytes is not available to "
            "convert\n");
        exit(1);
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
    if (t->type != TYPE_TRYTES) {
        printf("trinary.c: hashTrytes: This is not trytes\n");
        exit(1);
    }

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
