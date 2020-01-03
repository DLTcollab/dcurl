/*
 * Copyright (C) 2018 BiiLabs Co., Ltd. and Contributors
 * Copyright (C) 2017 IOTA AS, IOTA Foundation and Developers
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#ifndef CLCONTEXT_H_
#define CLCONTEXT_H_

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#define MAX_NUM_BUFFERS 9
#define MAX_NUM_KERNEL 3

typedef struct {
    size_t size;
    cl_mem_flags flags;
    size_t init_flags;
} buffer_info_t;

typedef struct {
    buffer_info_t buffer_info[MAX_NUM_BUFFERS];
    size_t num_buffers;
    size_t num_kernels;
    size_t num_src;
} kernel_info_t;

/* Every GPU device has its own cl_context_t */
typedef struct {
    cl_device_id device;
    cl_command_queue cmd_q;
    cl_mem buffer[MAX_NUM_BUFFERS];
    cl_kernel kernel[MAX_NUM_KERNEL];
    cl_program program;
    cl_context context;
    cl_uint num_cores;
    cl_ulong max_memory;
    size_t num_work_group;
    kernel_info_t kernel_info;
    uint64_t hash_count;
} cl_context_t;

enum {
    INDEX_OF_TRIT_HASH,
    INDEX_OF_MID_LOW,
    INDEX_OF_MID_HIGH,
    INDEX_OF_STATE_LOW,
    INDEX_OF_STATE_HIGH,
    INDEX_OF_MWM,
    INDEX_OF_FOUND,
    INDEX_OF_NONCE_PROBE,
    INDEX_OF_LOOP_COUNT
};

enum {
    INDEX_OF_KERNEL_INIT,
    INDEX_OF_KERNEL_SEARCH,
    INDEX_OF_KERNEL_FINALIZE,
};

/* return the number of available device */
int init_clcontext(cl_context_t *ctx);

#define KERNEL_PATH "./src/pow_kernel.cl"

#endif
