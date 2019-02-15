/*
 * Copyright (C) 2018 dcurl Developers.
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE file.
 */

#include <pthread.h>
#include <stdbool.h>
#include "dcurl.h"

static bool isInitialized = false;

/* mutex protecting initialization section */
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

char *ccurl_pow(char *trytes, int mwm)
{
    pthread_mutex_lock(&mtx);
    if (!isInitialized) {
        dcurl_init();
        isInitialized = true;
    }
    pthread_mutex_unlock(&mtx);
    return (char *) dcurl_entry((int8_t *) trytes, mwm, 1);
}

void ccurl_pow_finalize(void)
{
    dcurl_destroy();
}

void ccurl_pow_interrupt(void)
{
    /* Do Nothing */
}
