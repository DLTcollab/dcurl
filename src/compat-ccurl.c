/*
 * Copyright (C) 2018 dcurl Developers.
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE file.
 */

#include "dcurl.h"
#include <stdbool.h>

static bool isInitialized = false;

char *ccurl_pow(char *trytes, int mwm)
{
    if (!isInitialized) {
        dcurl_init(1, 0);
        isInitialized = true;
    }
    return (char *) dcurl_entry((int8_t *) trytes, mwm);
}

void ccurl_pow_finalize(void)
{
    dcurl_destroy();
}

void ccurl_pow_interrupt(void)
{
    /* Do Nothing */
}
