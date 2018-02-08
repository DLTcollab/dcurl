#ifndef DCURL_H_
#define DCURL_H_

#include "trinary/trinary.h"

void dcurl_init(int max_cpu_thread, int max_gpu_thread);
void dcurl_destroy();
Trytes_t *dcurl_entry(Trytes_t *trytes, int mwm);

#endif
