#ifndef COMMON_H_
#define COMMON_H_

#include <time.h>
#include <stdint.h>

double diff_in_second(struct timespec t1, struct timespec t2);

typedef struct _pow_info PoW_Info;

struct _pow_info {
    double time;
    uint64_t hash_count;
};

#endif
