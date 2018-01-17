#ifndef DCURL_H_
#define DCURL_H_

/* Foreign Functions */
char *PowC(char *trytes, int mwm, int index);
char *PowSSE(char *trytes, int mwm, int index);
char *PowCL(char *trytes, int mwm, int index);
void pwork_ctx_init(int context_size);
void pow_c_init(int num_task);

#endif
