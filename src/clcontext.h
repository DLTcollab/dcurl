/*
 * Copyright (C) 2018 dcurl Developers.
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE file.
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
} BufferInfo;

typedef struct {
    BufferInfo buffer_info[MAX_NUM_BUFFERS];
    size_t num_buffers;
    size_t num_kernels;
    size_t num_src;
} KernelInfo;

/* Every GPU device has own CLContext */
typedef struct {
    cl_device_id device;
    cl_command_queue cmdq;
    cl_mem buffer[MAX_NUM_BUFFERS];
    cl_kernel kernel[MAX_NUM_KERNEL];
    cl_program program;
    cl_context context;
    cl_uint num_cores;
    cl_ulong max_memory;
    size_t num_work_group;
    KernelInfo kernel_info;
} CLContext;

/* return the number of available device */
int init_clcontext(CLContext *ctx);

#define KERNEL_PATH "./src/pow_kernel.cl"

#endif
