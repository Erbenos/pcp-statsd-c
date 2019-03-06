#include <pthread.h>

#ifndef QUEUE_H_
#define QUEUE_H_

typedef struct queue {
    void** items;
    int limit;
    int front;
    int back;
    int full;
    int empty;
    int member_size;
    pthread_mutex_t* mutex;
    pthread_cond_t* not_empty;
    pthread_cond_t* not_full;
} queue;

queue* queue_init(int limit, int member_size);

void queue_free(queue* q);

void queue_enqueue(queue* q, void* item);

void* queue_dequeue(queue* q);

#endif

