#ifndef DCURL_H_
#define DCURL_H_

#include <stdbool.h>
#include "trinary.h"

bool dcurl_init();
void dcurl_destroy();
int8_t *dcurl_entry(int8_t *trytes, int mwm, int threads);

#endif
