#ifndef COMMON_H_
#define COMMON_H_

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define __DCURL_MAJOR__ 0
#define __DCURL_MINOR__ 1
#define __DCURL_PATCH__ 0

double diff_in_second(struct timespec t1, struct timespec t2);

static inline void ddprintf(const char *format, ...) {
#if defined(ENABLE_DEBUG)
    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
#endif
}

typedef struct _pow_info PoW_Info;

struct _pow_info {
    double time;
    uint64_t hash_count;
};

#endif
