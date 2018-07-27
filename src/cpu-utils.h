/*
 * Copyright (C) 2018 dcurl Developers.
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

/* Required for get_nprocs_conf() on Linux */
#if defined(__linux__)
#include <sys/sysinfo.h>
#endif

/* On Mac OS X, define our own get_nprocs_conf() */
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/sysctl.h>
static unsigned int get_nprocs_conf()
{
    int num_proc = 0;
    size_t size = sizeof(num_proc);
    if (sysctlbyname("hw.ncpu", &num_proc, &size, NULL, 0))
        return 1;
    return (unsigned int) num_proc;
}
#endif

static inline int get_avail_nprocs()
{
    size_t nproc = get_nprocs_conf() - 1;

    do {
        char *env_ncpu = getenv("DCURL_NUM_CPU");
        if (!env_ncpu) {
            break;
        }

        char *end;
        signed int num = strtol(env_ncpu, &end, 10);
        if (end == env_ncpu) {
            /* if no characters were converted these pointers are equal */
            break;
        }
        if (errno == ERANGE || num > INT_MAX || num < 0) {
            /* because strtol produces a long, check for overflow */
            break;
        }
        nproc = num;
    } while (0);

    if (!nproc)
        nproc = 1;
    return nproc;
}
