#ifndef DCURL_H_
#define DCURL_H_

#include "trinary.h"

int dcurl_init();
void dcurl_destroy();
int8_t *dcurl_entry(int8_t *trytes, int mwm);

#endif
