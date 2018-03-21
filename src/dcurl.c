/*
 * Copyright (C) 2018 dcurl Developers.
 * Use of this source code is governed by MIT license that can be
 * found in the LICENSE file.
 */

#include "dcurl.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "trinary.h"
#include "pow_cl.h"

#if defined(ENABLE_AVX)
#include "pow_avx.h"
#else
#include "pow_sse.h"
#endif

/* number of task that CPU can execute concurrently */
static int MAX_CPU_THREAD = -1;

/* number of task that GPU can execute concurrently */
static int MAX_GPU_THREAD = -1;

/* mutex protecting critical section */
static pthread_mutex_t mtx;

/* Semaphore that blocks excessive task*/
static sem_t notify;

/* check whether dcurl is initialized */
static int isInitialized = 0;

/* Respective number for Mutex */
static int *cpu_mutex_id = NULL;
static int *gpu_mutex_id = NULL;

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
    if (max_cpu_thread < 0 || max_gpu_thread < 0)
        return 0; /* Unavailable argument passed */

    int ret = 1;
    isInitialized = 1;
    MAX_CPU_THREAD = max_cpu_thread;
    MAX_GPU_THREAD = max_gpu_thread;
    cpu_mutex_id = (int *) calloc(MAX_CPU_THREAD, sizeof(int));
    if (!cpu_mutex_id)
        return 0;
#if defined(ENABLE_OPENCL)
    gpu_mutex_id = (int *) calloc(MAX_GPU_THREAD, sizeof(int));
    if (!gpu_mutex_id)
        return 0;
#endif
    pthread_mutex_init(&mtx, NULL);
    sem_init(&notify, 0, 0);
#if defined(ENABLE_AVX)
    ret &= pow_avx_init(MAX_CPU_THREAD);
#else
    ret &= pow_sse_init(MAX_CPU_THREAD);
#endif
#if defined(ENABLE_OPENCL)
    ret &= pwork_ctx_init(MAX_GPU_THREAD);
#endif
    return ret;
}

void dcurl_destroy()
{
    free(cpu_mutex_id);
    free(gpu_mutex_id);
#if defined(ENABLE_AVX)
    pow_avx_destroy();
#else
    pow_sse_destroy();
#endif
#if defined(ENABLE_OPENCL)
    pwork_ctx_destroy(MAX_GPU_THREAD);
#endif
}

int8_t *dcurl_entry(int8_t *trytes, int mwm)
{
    static int num_cpu_thread = 0;
    static int num_gpu_thread = 0;
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
        sem_wait(&notify);
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
#if defined(ENABLE_AVX)
    case 1:
        ret_trytes = PowAVX(trytes, mwm, selected_mutex_id);
#else
        ret_trytes = PowSSE(trytes, mwm, selected_mutex_id);
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
        sem_post(&notify);
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
