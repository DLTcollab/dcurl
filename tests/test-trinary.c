#include "common.h"

int main()
{
    char *trytes_test_case =
        "SCYLJDWIM9LIXCSLETSHLQOOFDKYOVFZLAHQYCCNMYHRTNIKBZRIRACFYPOWYNSOWDNXFZ"
        "UG9OEOZPOTD";
    int length_test_case = 81;

    Trytes_t *trytes =
        initTrytes((signed char *) trytes_test_case, length_test_case);
    assert(trytes);
    Trits_t *trits = trits_from_trytes(trytes);
    assert(trits);
    Trytes_t *ret_trytes = trytes_from_trits(trits);
    assert(ret_trytes);

    int ret = compareTrobject(trytes, ret_trytes);

    freeTrobject(trytes);
    freeTrobject(trits);
    freeTrobject(ret_trytes);

    assert(ret != 0);

    return 0;
}
