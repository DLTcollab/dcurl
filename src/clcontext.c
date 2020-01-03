/*
 * Copyright (C) 2018 BiiLabs Co., Ltd. and Contributors
 * Copyright (C) 2017 IOTA AS, IOTA Foundation and Developers
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#include "clcontext.h"
#include <stdbool.h>
#include <stdio.h>
#include "constants.h"
#include "pearl.cl.h"

static bool init_cl_devices(cl_context_t *ctx)
{
    cl_int errno;

    /* Create OpenCL context */
    ctx->context =
        (cl_context) clCreateContext(NULL, 1, &ctx->device, NULL, NULL, &errno);
    if (errno != CL_SUCCESS)
        return false; /* Failed to create OpenCL Context */

    /* Get Device Info (num_cores) */
    if (CL_SUCCESS != clGetDeviceInfo(ctx->device, CL_DEVICE_MAX_COMPUTE_UNITS,
                                      sizeof(cl_uint), &ctx->num_cores, NULL))
        return false; /* Failed to get num_cores of GPU */

    /* Get Device Info (max_memory) */
    if (CL_SUCCESS != clGetDeviceInfo(ctx->device, CL_DEVICE_MAX_MEM_ALLOC_SIZE,
                                      sizeof(cl_ulong), &ctx->max_memory, NULL))
        return false; /* Failed to get Max memory of GPU */

    /* Get Device Info (num work group) */
    if (CL_SUCCESS !=
        clGetDeviceInfo(ctx->device, CL_DEVICE_MAX_WORK_GROUP_SIZE,
                        sizeof(size_t), &ctx->num_work_group, NULL))
        ctx->num_work_group = 1;

    /* Create Command Queue */
    ctx->cmd_q = clCreateCommandQueue(ctx->context, ctx->device, 0, &errno);
    if (errno != CL_SUCCESS)
        return false; /* Failed to create command queue */

    return true;
}

static bool init_cl_program(cl_context_t *ctx)
{
    unsigned char *source_str = pearl_cl;
    size_t source_size = pearl_cl_len;
    cl_int errno;

    ctx->program = clCreateProgramWithSource(
        ctx->context, ctx->kernel_info.num_src, (const char **) &source_str,
        (const size_t *) &source_size, &errno);
    if (CL_SUCCESS != errno)
        return false; /* Failed to create OpenCL program */

    errno =
        clBuildProgram(ctx->program, 1, &ctx->device, "-Werror", NULL, NULL);
    if (CL_SUCCESS != errno)
        return false; /* Failed to build OpenCL program */

    return true;
}

static bool init_cl_kernel(cl_context_t *ctx)
{
    char *kernel_name[] = {"init", "search", "finalize"};
    cl_int errno;

    for (int i = 0; i < ctx->kernel_info.num_kernels; i++) {
        ctx->kernel[i] = clCreateKernel(ctx->program, kernel_name[i], &errno);
        if (CL_SUCCESS != errno)
            return false; /* Failed to create OpenCL kernel */
    }
    return true;
}

static bool init_cl_buffer(cl_context_t *ctx)
{
    cl_ulong mem, max_mem = 0;
    cl_int errno;

    for (int i = 0; i < ctx->kernel_info.num_buffers; i++) {
        mem = ctx->kernel_info.buffer_info[i].size;
        if (ctx->kernel_info.buffer_info[i].init_flags & 2) {
            mem *= ctx->num_cores * ctx->num_work_group;
            if (mem > ctx->max_memory) {
                int temp =
                    ctx->max_memory / ctx->kernel_info.buffer_info[i].size;
                ctx->num_cores = temp;
                mem = temp * ctx->kernel_info.buffer_info[i].size;
            }
        }
        /* Check Memory bound */
        max_mem += mem;
        if (max_mem >= ctx->max_memory)
            return false; /* GPU Memory is not enough */

        /* Create OpenCL Buffer */
        ctx->buffer[i] =
            clCreateBuffer(ctx->context, ctx->kernel_info.buffer_info[i].flags,
                           mem, NULL, &errno);
        if (CL_SUCCESS != errno)
            return false; /* Failed to create OpenCL Memory Buffer */

        /* Set Kernel Arguments */
        for (int j = 0; j < ctx->kernel_info.num_kernels; j++) {
            if (CL_SUCCESS != clSetKernelArg(ctx->kernel[j], i, sizeof(cl_mem),
                                             (void *) &ctx->buffer[i]))
                return false; /* Failed to set OpenCL kernel arguments */
        }
    }
    return true;
}

static bool init_buffer_info(cl_context_t *ctx)
{
    ctx->kernel_info.buffer_info[INDEX_OF_TRIT_HASH] =
        (buffer_info_t){sizeof(char) * HASH_TRITS_LENGTH, CL_MEM_WRITE_ONLY};
    ctx->kernel_info.buffer_info[INDEX_OF_MID_LOW] = (buffer_info_t){
        sizeof(int64_t) * STATE_TRITS_LENGTH, CL_MEM_READ_WRITE, 2};
    ctx->kernel_info.buffer_info[INDEX_OF_MID_HIGH] = (buffer_info_t){
        sizeof(int64_t) * STATE_TRITS_LENGTH, CL_MEM_READ_WRITE, 2};
    ctx->kernel_info.buffer_info[INDEX_OF_STATE_LOW] = (buffer_info_t){
        sizeof(int64_t) * STATE_TRITS_LENGTH, CL_MEM_READ_WRITE, 2};
    ctx->kernel_info.buffer_info[INDEX_OF_STATE_HIGH] = (buffer_info_t){
        sizeof(int64_t) * STATE_TRITS_LENGTH, CL_MEM_READ_WRITE, 2};
    ctx->kernel_info.buffer_info[INDEX_OF_MWM] =
        (buffer_info_t){sizeof(size_t), CL_MEM_READ_ONLY};
    ctx->kernel_info.buffer_info[INDEX_OF_FOUND] =
        (buffer_info_t){sizeof(char), CL_MEM_READ_WRITE};
    ctx->kernel_info.buffer_info[INDEX_OF_NONCE_PROBE] =
        (buffer_info_t){sizeof(int64_t), CL_MEM_READ_WRITE, 2};
    ctx->kernel_info.buffer_info[INDEX_OF_LOOP_COUNT] =
        (buffer_info_t){sizeof(size_t), CL_MEM_READ_ONLY};

    return init_cl_buffer(ctx);
}

static bool set_clcontext(cl_context_t *ctx, cl_device_id device)
{
    ctx->device = device;
    ctx->kernel_info.num_buffers = 9;
    ctx->kernel_info.num_kernels = 3;
    ctx->kernel_info.num_src = 1;

    return init_cl_devices(ctx) && init_cl_program(ctx);
}

int init_clcontext(cl_context_t *ctx)
{
    int ctx_idx = 0;

    cl_uint num_platform = 0;
    cl_platform_id *platform;

    /* Get the platform */
    clGetPlatformIDs(0, NULL, &num_platform);
    platform = (cl_platform_id *) malloc(sizeof(cl_platform_id) * num_platform);
    if (!platform)
        return 0;
    clGetPlatformIDs(num_platform, platform, NULL);

    cl_uint num_devices = 0;
    cl_device_id *devices;

    /* Iterate the platform list and get its devices */
    for (int i = 0; i < num_platform; i++) {
        clGetDeviceIDs(platform[i], CL_DEVICE_TYPE_GPU, 0, NULL, &num_devices);
        devices = (cl_device_id *) malloc(sizeof(cl_device_id) * num_devices);
        if (!devices)
            goto leave;
        clGetDeviceIDs(platform[i], CL_DEVICE_TYPE_GPU, num_devices, devices,
                       NULL);
        for (int j = 0; j < num_devices; j++, ctx_idx++) {
            int ret = 1;
            ret &= set_clcontext(&ctx[ctx_idx], devices[j]);
            ret &= init_cl_kernel(&ctx[ctx_idx]);
            ret &= init_buffer_info(&ctx[ctx_idx]);
            if (!ret) {
                free(devices);
                goto leave;
            }
        }
        free(devices);
    }
leave:
    free(platform);
    return ctx_idx;
}
