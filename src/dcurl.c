#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include "dcurl.h"
#include "pow_sse.h"
#include "pow_cl.h"

/* number of task that CPU can execute concurrently */
static int MAX_CPU_THREAD = -1;

/* number of task that GPU can execute concurrently */
static int MAX_GPU_THREAD = -1;

/* mutex protecting critical section */
pthread_mutex_t mtx;

/* Semaphore that blocks excessive task*/
sem_t notify;

/* check whether dcurl is initialized */
int isInitialized = 0;

/* Respective number for Mutex */
int *cpu_mutex_id = NULL;
int *gpu_mutex_id = NULL;

int get_mutex_id(int *mutex_id, int env)
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

void dcurl_init(int max_cpu_thread, int max_gpu_thread)
{
    if (max_cpu_thread < 0 || max_gpu_thread < 0) {
        printf("dcurl.c: Unavailable argument\n");
        exit(0);
    }

    isInitialized = 1;
    MAX_CPU_THREAD = max_cpu_thread;
    MAX_GPU_THREAD = max_gpu_thread;
    cpu_mutex_id = (int *) calloc(MAX_CPU_THREAD, sizeof(int));
    gpu_mutex_id = (int *) calloc(MAX_GPU_THREAD, sizeof(int));
    pthread_mutex_init(&mtx, NULL);
    sem_init(&notify, 0, 0);
    pow_sse_init(MAX_CPU_THREAD);
    pwork_ctx_init(MAX_GPU_THREAD);
}

void dcurl_entry(char *trytes, int mwm)
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
    } else if (num_gpu_thread < MAX_GPU_THREAD) {
        num_gpu_thread++;
        selected_mutex_id = get_mutex_id(gpu_mutex_id, 2);
        selected_entry = 2;
        pthread_mutex_unlock(&mtx);
    } else {
        num_waiting_thread++;
        pthread_mutex_unlock(&mtx);
        sem_wait(&notify);
        /* get mutex number */
        pthread_mutex_lock(&mtx);
        selected_entry = 1;
        /* get mutex number. If return value is -1, which means cpu queue full */
        if ((selected_mutex_id = get_mutex_id(cpu_mutex_id, 1)) == -1) {
            selected_mutex_id = get_mutex_id(gpu_mutex_id, 2);
            selected_entry = 2;
        }
        pthread_mutex_unlock(&mtx);
    }

    switch (selected_entry) {
        case 1:
            printf("cpu: %s\n", PowSSE(trytes, mwm, selected_mutex_id));
            break;
        case 2:
            printf("gpu: %s\n", PowCL(trytes, mwm, selected_mutex_id));
            break;
        default:
            printf("error produced\n");
            exit(0);
    }
   
    pthread_mutex_lock(&mtx);
    
    if (selected_entry == 1)
        cpu_mutex_id[selected_mutex_id] = 0;
    else
        gpu_mutex_id[selected_mutex_id] = 0;
    
    if (num_waiting_thread > 0) {
        sem_post(&notify);
        num_waiting_thread--;
    } else {
        if (selected_entry == 1)
            num_cpu_thread--;
        else
            num_gpu_thread--;
    }
    pthread_mutex_unlock(&mtx);
}


