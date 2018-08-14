#ifndef POW_CL_H_
#define POW_CL_H_

#include "trinary.h"
#include "clcontext.h"

typedef struct _pow_cl_context PoW_CL_Context;

struct _pow_cl_context {
    CLContext *clctx;
    int indexOfContext;
    /* Arguments of PoW */
    int8_t input_trytes[2673]; /* 2673 */
    int8_t output_trytes[2673]; /* 2673 */
    int mwm;
};

int PowCL(void *pow_ctx);
int pwork_ctx_init();
void pwork_ctx_destroy(int context_size);

#endif
