/* Test program for dcurl */
#include "common.h"
#include "dcurl.h"

int main()
{
    char *trytes =
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "99999999999999999A9RGRKVGWMWMKOLVMDFWJUHNUNYWZTJADGGPZGXNLERLXYWJE9WQH"
        "WWBMCPZMVVMJUMWWBLZLNMLDCGDJ999999999999999999999999999999999999999999"
        "999999999999YGYQIVD99999999999999999999TXEFLKNPJRBYZPORHZU9CEMFIFVVQBU"
        "STDGSJCZMBTZCDTTJVUFPTCCVHHORPMGCURKTH9VGJIXUQJVHK99999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999999999999999999999999999999999999999999999999999999999999"
        "9999999999999";

    int mwm = 9;

    /* test dcurl Implementation with mwm = 9 */
    dcurl_init();
    int8_t *ret_trytes = dcurl_entry((int8_t *) trytes, mwm, 0);
    assert(ret_trytes);
    dcurl_destroy();

    Trytes_t *trytes_t = initTrytes(ret_trytes, 2673);
    assert(trytes_t);
    Trytes_t *hash_trytes = hashTrytes(trytes_t);
    assert(hash_trytes);

    /* Validation */
    Trits_t *ret_trits = trits_from_trytes(hash_trytes);
    for (int i = 243 - 1; i >= 243 - mwm; i--) {
        assert(ret_trits->data[i] == 0);
    }

    free(ret_trytes);
    freeTrobject(trytes_t);
    freeTrobject(hash_trytes);
    freeTrobject(ret_trits);

    return 0;
}
