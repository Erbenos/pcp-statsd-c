#include <pthread.h>

typedef struct queue {
    char** strings;
    long head;
    long tail;
    int full;
    int empty;
    pthread_mutex_t *mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} queue;

queue* init_queue(queue* q);

void queue_free(queue* q);

void queue_insert_item(queue* q, char* string);

void queue_delete_item(queue* q, char* string);


