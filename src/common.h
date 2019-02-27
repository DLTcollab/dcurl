#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>
#include <time.h>

#define __DCURL_MAJOR__ 0
#define __DCURL_MINOR__ 1
#define __DCURL_PATCH__ 0

double diff_in_second(struct timespec t1, struct timespec t2);

typedef struct _pow_info PoW_Info;

struct _pow_info {
    double time;
    uint64_t hash_count;
};

#endif
