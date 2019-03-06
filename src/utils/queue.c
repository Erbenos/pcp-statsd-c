#include <pthread.h>
#include <stdlib.h>
#include "queue.h"
#include "utils.h"

queue* queue_init(int limit, int member_size) {
    queue* q = (queue*) malloc(sizeof(queue));
    if (q == NULL) {
        die(__LINE__, "Unable to assign memory for queue.");
    }
    q->limit = limit;
    q->member_size = member_size;
    q->items = (void**) malloc(sizeof(void*) * limit);
    if (q->items == NULL) {
        die(__LINE__, "Unable to assign static space for queue.");
    }
    int i;
    for (i = 0; i < limit; i++) {
        q->items[i] = (void*) malloc(member_size);
    }
    q->empty = 1;
    q->full = 0;
    q->front = -1;
    q->back = -1;
    q->mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if (q->mutex == NULL) {
        die(__LINE__, "Unable to assign memory for mutex on queue.");
    }
    pthread_mutex_init(q->mutex, NULL);
    q->not_full = (pthread_cond_t*) malloc(sizeof(pthread_cond_t));
    if (q->not_full == NULL) {
        die(__LINE__, "Unable to assign memory for queue.");
    }
    pthread_cond_init(q->not_full, NULL);
    q->not_empty = (pthread_cond_t*) malloc(sizeof(pthread_cond_t));
    if (q->not_empty == NULL) {
        die(__LINE__, "Unable to assign memory for queue.");
    }
    pthread_cond_init(q->not_empty, NULL);
    return q;
}

void queue_free(queue* q) {
    pthread_mutex_destroy(q->mutex);
    pthread_cond_destroy(q->not_full);
    pthread_cond_destroy(q->not_empty);
    free(q->mutex);
    free(q->not_full);
    free(q->not_empty);
    free(q);
}

void queue_enqueue(queue* q, void* item) {
    if ((q->front == q->back + 1) || (q->front == 0 && q->back == q->limit - 1)) {
        q->full = 1;
        return;
    }
    if (q->front == -1) {
        q->front = 0;
    }
    q->back = (q->back + 1) % q->limit;
    q->items[q->back] = item;
    q->empty = 0;
}

void* queue_dequeue(queue* q) {
    if (q->front == -1) return NULL;
    void* target = q->items[q->front];
    if (q->front == q->back) {
        q->empty = 1;
        q->front = -1;
        q->back = -1;
    } else {
        q->front = (q->front + 1) % q->limit;
    }
    q->full = 0;
    return target;
}
