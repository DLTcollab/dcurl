/*
 * Copyright (C) 2018-2019 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

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
