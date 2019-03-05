#include <pthread.h>

typedef struct queue {
    void** items;
    int limit;
    long front;
    long back;
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


