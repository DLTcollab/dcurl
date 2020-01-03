/*
 * Copyright (C) 2018-2019 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define __DCURL_MAJOR__ 0
#define __DCURL_MINOR__ 5
#define __DCURL_PATCH__ 0

double diff_in_second(struct timespec t1, struct timespec t2);

/* Copy from logger project:
 * https://bitbucket.org/embear/logger/src/abef6b0a6c991545a3d3fecfbc39d2b0448fb85a/include/logger.h#lines-199*/
typedef int16_t logger_id_t;

static inline void log_debug(logger_id_t const logger_id,
                             const char *format,
                             ...)
{
#if defined(ENABLE_DEBUG)
    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
    fflush(stdout);
#endif
}

static inline void log_info(logger_id_t const logger_id,
                             const char *format,
                             ...)
{
    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
    fflush(stdout);
}

typedef struct pow_info_s pow_info_t;

struct pow_info_s {
    double time;
    uint64_t hash_count;
};

#endif
