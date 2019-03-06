#include <stdio.h>
#include <string.h>

#include "../statsd-parsers.h"
#include "../../utils/utils.h"

void* consume_datagram(void* args) {
    queue* parsed_q = ((consumer_args*)args)->parsed_datagrams_queue;
    agent_config* config = ((consumer_args*)args)->config;
    while(1) {
        pthread_mutex_lock(parsed_q->mutex);
		while (parsed_q->empty) {
			pthread_cond_wait(parsed_q->not_empty, parsed_q->mutex);
		}
        printf("Arrived at consumer: \n");
        print_out_datagram((statsd_datagram*) queue_dequeue(parsed_q));
		pthread_mutex_unlock(parsed_q->mutex);
		pthread_cond_signal(parsed_q->not_full);
    }
}
