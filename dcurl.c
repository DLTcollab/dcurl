#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include "dcurl.h"

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

/* Respective number for Mutex */
int mutex_id[MAX_CPU_THREAD] = {0};

int get_mutex_id(int *mutex_id)
{
    for (int i = 0; i < MAX_CPU_THREAD; i++) {
        if (mutex_id[i] == 0) {
            mutex_id[i] = 1;
            return i;
        }
    }
    return -1;
}

void dcurl_init(void)
{
    isInitialized = 1;
    pthread_mutex_init(&mtx, NULL);
    sem_init(&notify, 0, 0);

    for (int i = 0; i < MAX_CPU_THREAD; i++) {
        mutex_id[i] = 0;
    }
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
        selected_mutex_id = get_mutex_id(mutex_id);
        selected_entry = 1;
        pthread_mutex_unlock(&mtx);
    } else if (num_gpu_thread < MAX_GPU_THREAD) {
        num_gpu_thread++;
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
        if ((selected_mutex_id = get_mutex_id(mutex_id)) == -1) {
            selected_entry = 2;
        }
        pthread_mutex_unlock(&mtx);
    }

    //printf("%s\n", PowC(trytes, mwm, selected_mutex_id));
    
    sleep(1);

    switch (selected_entry) {
        /* CPU */
        case 1:
            printf("It's CPU! num_cpu_thread: %d\n", num_cpu_thread);
            break;
        /* GPU */
        case 2:
            printf("It's GPU! num_gpu_thread: %d\n", num_gpu_thread);
            break;
        default:
            printf("error produced\n");
            exit(0);
    }

    pthread_mutex_lock(&mtx);
    
    if (selected_entry == 1)
        mutex_id[selected_mutex_id] = 0;
    
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


