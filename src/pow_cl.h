#ifndef POW_CL_H_
#define POW_CL_H_

#include "./trinary/trinary.h"

int8_t *PowCL(int8_t *trytes, int mwm, int index);
int pwork_ctx_init(int context_size);
void pwork_ctx_destroy(int context_size);

#endif
