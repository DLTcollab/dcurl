#ifndef CLCONTEXT_H_
#define CLCONTEXT_H_

#include <CL/cl.h>

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

typedef struct {
    cl_uint num_devices;
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

int init_clcontext(CLContext **ctx);
void init_cl_kernel(CLContext *ctx, char **kernel_name);
void init_cl_buffer(CLContext *ctx);

#define KERNEL_PATH "src/pow_kernel.cl"
#define MAX_SOURCE_SIZE (0x100000)

#endif
