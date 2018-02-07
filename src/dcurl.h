#ifndef DCURL_H_
#define DCURL_H_

void dcurl_init(int max_cpu_thread, int max_gpu_thread);
void dcurl_destroy();
char *dcurl_entry(char *trytes, int mwm);

#endif
