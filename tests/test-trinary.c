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

    trytes_t *trytes =
        init_trytes((signed char *) trytes_test_case, length_test_case);
    assert(trytes);
    trits_t *trits = trits_from_trytes(trytes);
    assert(trits);
    trytes_t *ret_trytes = trytes_from_trits(trits);
    assert(ret_trytes);

    int ret = compare_trinary_object(trytes, ret_trytes);

    free_trinary_object(trytes);
    free_trinary_object(trits);
    free_trinary_object(ret_trytes);

    assert(ret != 0);

    return 0;
}
