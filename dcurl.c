#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

/* number of task that CPU can execute concurrently */
#define MAX_CPU_THREAD 5

/* number of task that GPU can execute concurrently */
#define MAX_GPU_THREAD 5

/* mutex protecting critical section */
pthread_mutex_t mtx;

/* Semaphore that blocks excessive task*/
sem_t notify;

/* check whether dcurl is initialized */
int isInitialized = 0;

void dcurl_init(void)
{
    isInitialized = 1;
    pthread_mutex_init(&mtx, NULL);
    sem_init(&notify, 0, 0);
}

void dcurl_entry(int data)
{
    static int num_cpu_thread = 0;
    static int num_gpu_thread = 0;
    static int num_waiting_thread = 0;

    pthread_mutex_lock(&mtx);
    if (num_cpu_thread < MAX_CPU_THREAD) {
        num_cpu_thread++;
        pthread_mutex_unlock(&mtx);
    } else {
        num_waiting_thread++;
        pthread_mutex_unlock(&mtx);
        sem_wait(&notify);
    }

    /* TODO: GPU entry */

    /* Do something */
    printf("data: %d num_cpu_thread: %d\n", data, num_cpu_thread);

    pthread_mutex_lock(&mtx);
    if (num_waiting_thread > 0) {
        sem_post(&notify);
        num_waiting_thread--;
    } else {
        num_cpu_thread--;
    }
    pthread_mutex_lock(&mtx);
}


