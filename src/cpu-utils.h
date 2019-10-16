/*
 * Copyright (C) 2018 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

/* Required for get_nprocs_conf() on Linux */
#if defined(__linux__)
#include <sys/sysinfo.h>
#endif

/**
 * @file cpu-utils.h
 * @brief Utility functions for acquiring CPU information.
 */

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

/**
 * @brief Get the thread number per physical processor.
 *
 * - GNU/Linux: Acquire the thread number by parsing the CPU information.
 * - macOS: Acquire the thread number by doing the calculation of
 * (logical processor number / physical processor number).
 * @return The thread number per physical processor.
 * @retval 1 Hyperthreading disabled.
 * @retval 2 Hyperthreading enabled.
 * @retval -1 Unexpected error.
 */
static inline int get_nthds_per_phys_proc()
{
    int nthread;
#if defined(__linux__)
    FILE *fd;
    char nthd[4];

    fd = popen("LC_ALL=C lscpu | grep 'Thread(s) per core' | awk '{printf $4}'",
               "r");
    if (fd == NULL)
        return -1;
    if (fgets(nthd, sizeof(nthd), fd) == NULL)
        return -1;
    nthread = (int) strtol(nthd, NULL, 10);
    if (errno == ERANGE || nthread == 0) {
        return -1;
    }

    if (pclose(fd) == -1)
        return -1;
#elif defined(__APPLE__)
    FILE *fd;
    char p_proc[4], l_proc[4];
    int phys_proc, logic_proc;

    fd = popen("sysctl hw.physicalcpu | awk '{printf $2}'", "r");
    if (fd == NULL)
        return -1;
    if (fgets(p_proc, sizeof(p_proc), fd) == NULL)
        return -1;
    fd = popen("sysctl hw.logicalcpu | awk '{printf $2}'", "r");
    if (fd == NULL)
        return -1;
    if (fgets(l_proc, sizeof(l_proc), fd) == NULL)
        return -1;
    phys_proc = (int) strtol(p_proc, NULL, 10);
    if (errno == ERANGE || phys_proc == 0) {
        return -1;
    }
    logic_proc = (int) strtol(l_proc, NULL, 10);
    if (errno == ERANGE || logic_proc == 0) {
        return -1;
    }

    nthread = logic_proc / phys_proc;

    if (pclose(fd) == -1)
        return -1;
#else
    nthread = 1;
#endif
    return nthread;
}

/**
 * @brief Get the available number of physical processor.
 *
 * Detect the physical processor number with `get_nproc_conf()`
 * or get the number with the environment variable **DCURL_NUM_CPU**.
 * @return The available number of physical processor.
 * @retval DCURL_NUM_CPU Return it if the environment variable
 * **DCURL_NUM_CPU** is set.
 * @retval Maximum_physical_processor_number-1 Return it if the environment
 * variable **DCURL_NUM_CPU** is not set. The minimum value would be 1.
 * @retval -1 Unexpected error.
 */
static inline int get_avail_phys_nprocs()
{
    int nthd;
    size_t nproc = 0;
    char *env_ncpu;

    nthd = get_nthds_per_phys_proc();
    env_ncpu = getenv("DCURL_NUM_CPU");
    if (nthd <= 0)
        return -1;
    else if (env_ncpu) {
        do {
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
    } else
        nproc = (get_nprocs_conf() / nthd) - 1;

    if (!nproc)
        nproc = 1;
    return nproc;
}
