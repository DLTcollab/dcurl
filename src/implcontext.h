#ifndef IMPL_CTX_H_
#define IMPL_CTX_H_

#include <stdbool.h>
#include <stdint.h>
#include "common.h"
#include "list.h"
#include "uv.h"

typedef struct _impl_context ImplContext;

struct _impl_context {
    void *context;
    char *description;

    /* Multi-thread Management */
    uv_mutex_t lock;
    int bitmap; /* Used to tell which slot is available */
    int num_max_thread;
    int num_working_thread;

    /* Functions of Implementation Context */
    bool (*initialize)(ImplContext *impl_ctx);
    void (*destroy)(ImplContext *impl_ctx);
    /* Private PoW Context for each thread */
    void *(*getPoWContext)(ImplContext *impl_ctx,
                           int8_t *trytes,
                           int mwm,
                           int threads);
    bool (*doThePoW)(void *pow_ctx);
    int8_t *(*getPoWResult)(void *pow_ctx);
    PoW_Info (*getPoWInfo)(void *pow_ctx);
    bool (*freePoWContext)(ImplContext *impl_ctx, void *pow_ctx);

    /* Linked list */
    struct list_head list;
};

bool registerImplContext(ImplContext *impl_ctx);
bool initializeImplContext(ImplContext *impl_ctx);
void destroyImplContext(ImplContext *impl_ctx);
bool enterImplContext(ImplContext *impl_ctx);
void exitImplContext(ImplContext *impl_ctx);
void *getPoWContext(ImplContext *impl_ctx,
                    int8_t *trytes,
                    int mwm,
                    int threads);
bool doThePoW(ImplContext *impl_ctx, void *pow_ctx);
bool freePoWContext(ImplContext *impl_ctx, void *pow_ctx);
int8_t *getPoWResult(ImplContext *impl_ctx, void *pow_ctx);
PoW_Info getPoWInfo(ImplContext *impl_ctx, void *pow_ctx);

#endif
