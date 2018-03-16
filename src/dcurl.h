#ifndef DCURL_H_
#define DCURL_H_

#include "trinary/trinary.h"

int dcurl_init(int max_cpu_thread, int max_gpu_thread);
void dcurl_destroy();
int8_t *dcurl_entry(int8_t *trytes, int mwm);

#endif
