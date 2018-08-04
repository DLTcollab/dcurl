/*
 * Copyright (C) 2018 dcurl Developers.
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE file.
 */

#include "dcurl.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pow_cl.h"
#include "trinary.h"

#if defined(ENABLE_AVX)
#include "pow_avx.h"
#elif defined(ENABLE_SSE)
#include "pow_sse.h"
#elif defined(ENABLE_FPGA_LAMPALAB)
#include "pow_fpga_LampaLab.h"
#else
#include "pow_c.h"
#endif

#include <pthread.h>
#ifdef __APPLE__
#include <dispatch/dispatch.h>
#else
#include <semaphore.h>
#endif

/* number of task that CPU can execute concurrently */
static int MAX_CPU_THREAD = -1;

/* number of task that GPU can execute concurrently */
static int MAX_GPU_THREAD = -1;

/* mutex protecting critical section */
static pthread_mutex_t mtx;

/* Semaphore that blocks excessive task */
#ifdef __APPLE__
static dispatch_semaphore_t notify;
#else
static sem_t notify;
#endif

/* check whether dcurl is initialized */
static int isInitialized = 0;

/* Respective number for Mutex */
static int *cpu_mutex_id = NULL;
#if defined(ENABLE_OPENCL)
static int *gpu_mutex_id = NULL;
#endif

static int get_mutex_id(int *mutex_id, int env)
{
    int MAX = (env == 1) ? MAX_CPU_THREAD : MAX_GPU_THREAD;
    for (int i = 0; i < MAX; i++) {
        if (mutex_id[i] == 0) {
            mutex_id[i] = 1;
            return i;
        }
    }
    return -1;
}

int dcurl_init(int max_cpu_thread, int max_gpu_thread)
{
    int ret = 1;
    isInitialized = 1;

    if (max_cpu_thread < 0)
        return 0; /* Unavailable argument passed */
    MAX_CPU_THREAD = max_cpu_thread;
    cpu_mutex_id = (int *) calloc(MAX_CPU_THREAD, sizeof(int));
    if (!cpu_mutex_id)
        return 0;
#if defined(ENABLE_OPENCL)
    if (max_gpu_thread < 0)
        return 0; /* Unavailable argument passed */
    MAX_GPU_THREAD = max_gpu_thread;
    gpu_mutex_id = (int *) calloc(MAX_GPU_THREAD, sizeof(int));
    if (!gpu_mutex_id)
        return 0;
#endif
    pthread_mutex_init(&mtx, NULL);

#ifdef __APPLE__
    notify = dispatch_semaphore_create(0);
#else
    sem_init(&notify, 0, 0);
#endif

#if defined(ENABLE_AVX)
    ret &= pow_avx_init(MAX_CPU_THREAD);
#elif defined(ENABLE_SSE)
    ret &= pow_sse_init(MAX_CPU_THREAD);
#elif defined(ENABLE_FPGA_LAMPALAB)
    ret &= pow_fpga_LampaLab_init();
#else
    ret &= pow_c_init(MAX_CPU_THREAD);
#endif
#if defined(ENABLE_OPENCL)
    ret &= pwork_ctx_init(MAX_GPU_THREAD);
#endif
    return ret;
}

void dcurl_destroy()
{
    free(cpu_mutex_id);
#if defined(ENABLE_AVX)
    pow_avx_destroy();
#elif defined(ENABLE_SSE)
    pow_sse_destroy();
#elif defined(ENABLE_FPGA_LAMPALAB)
    pow_fpga_LampaLab_destroy();
#else
    pow_c_destroy();
#endif
#if defined(ENABLE_OPENCL)
    free(gpu_mutex_id);
    pwork_ctx_destroy(MAX_GPU_THREAD);
#endif
}

int8_t *dcurl_entry(int8_t *trytes, int mwm)
{
    static int num_cpu_thread = 0;
#if defined(ENABLE_OPENCL)
    static int num_gpu_thread = 0;
#endif
    static int num_waiting_thread = 0;
    int selected_mutex_id = -1;
    int selected_entry = -1;

    pthread_mutex_lock(&mtx);
    if (num_cpu_thread < MAX_CPU_THREAD) {
        num_cpu_thread++;
        /* get mutex number */
        selected_mutex_id = get_mutex_id(cpu_mutex_id, 1);
        selected_entry = 1;
        pthread_mutex_unlock(&mtx);
#if defined(ENABLE_OPENCL)
    } else if (num_gpu_thread < MAX_GPU_THREAD) {
        num_gpu_thread++;
        selected_mutex_id = get_mutex_id(gpu_mutex_id, 2);
        selected_entry = 2;
        pthread_mutex_unlock(&mtx);
#endif
    } else {
        num_waiting_thread++;
        pthread_mutex_unlock(&mtx);
#ifdef __APPLE__
        dispatch_semaphore_wait(notify, DISPATCH_TIME_FOREVER);
#else
        sem_wait(&notify);
#endif

        /* Waiting thread acquire permission */
        num_waiting_thread--;
        selected_entry = 1;
        /* get mutex number */
        selected_mutex_id = get_mutex_id(cpu_mutex_id, 1);
#if defined(ENABLE_OPENCL)
        /* if return value == -1, meaning cpu queue full */
        if (selected_mutex_id == -1) {
            selected_mutex_id = get_mutex_id(gpu_mutex_id, 2);
            selected_entry = 2;
        }
#endif
        pthread_mutex_unlock(&mtx);
    }

    int8_t *ret_trytes = NULL;

    switch (selected_entry) {
    case 1:
#if defined(ENABLE_AVX)
        ret_trytes = PowAVX(trytes, mwm, selected_mutex_id);
#elif defined(ENABLE_SSE)
        ret_trytes = PowSSE(trytes, mwm, selected_mutex_id);
#elif defined(ENABLE_FPGA_LAMPALAB)
        ret_trytes =
            (int8_t *) PowFPGALampaLab((char *) trytes, mwm, selected_mutex_id);
#else
        ret_trytes = PowC(trytes, mwm, selected_mutex_id);
#endif
        break;
#if defined(ENABLE_OPENCL)
    case 2:
        ret_trytes = PowCL(trytes, mwm, selected_mutex_id);
        break;
#endif
    default:
        return NULL;
    }

    pthread_mutex_lock(&mtx);

#if defined(ENABLE_OPENCL)
    if (selected_entry == 1)
        cpu_mutex_id[selected_mutex_id] = 0;
    else
        gpu_mutex_id[selected_mutex_id] = 0;
#else
    cpu_mutex_id[selected_mutex_id] = 0;
#endif

    if (num_waiting_thread > 0) {
/* Don't unlock mutex, giving waiting thread permision */
#ifdef __APPLE__
        dispatch_semaphore_signal(notify);
#else
        sem_post(&notify);
#endif
    } else {
#if defined(ENABLE_OPENCL)
        if (selected_entry == 1)
            num_cpu_thread--;
        else
            num_gpu_thread--;
#else
        num_cpu_thread--;
#endif
        pthread_mutex_unlock(&mtx);
    }

    return ret_trytes;
}
