#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include "statsd-parsers.h"
#include "basic/basic.h"
#include "ragel/ragel.h"
#include "../utils/utils.h"
#include "../utils/queue.h"

const int PARSER_TRIVIAL = 0;
const int PARSER_RAGEL = 1;

void* statsd_network_listen(void* args) {
    agent_config* config = ((statsd_listener_args*)args)->config;
    queue* q = ((statsd_listener_args*)args)->unprocessed_datagrams_queue;
    
    const char* hostname = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    struct addrinfo* res = 0;
    int err = getaddrinfo(hostname, config->port, &hints, &res);
    if (err != 0) {
        die(__LINE__, "failed to resolve local socket address (err=%s)", gai_strerror(err));
    }
    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == -1) {
        die(__LINE__, "failed creating socket (err=%s)", strerror(errno));
    }
    if (bind(fd, res->ai_addr, res->ai_addrlen) == -1) {
        die(__LINE__, "failed binding socket (err=%s)", strerror(errno));
    }
    if (config->verbose == 1) {
        printf("Socket enstablished. \n");
        printf("Waiting for datagrams. \n");
    }
    freeaddrinfo(res);
    int max_udp_packet_size = config->max_udp_packet_size;
    char *buffer = (char *) malloc(max_udp_packet_size);
    struct sockaddr_storage src_addr;
    socklen_t src_addr_len = sizeof(src_addr);
    while(1) {
        ssize_t count = recvfrom(fd, buffer, max_udp_packet_size, 0, (struct sockaddr*)&src_addr, &src_addr_len);
        if (count == -1) {
            die(__LINE__, "%s", strerror(errno));
        } 
        // since we checked for -1
        else if ((signed int)count == max_udp_packet_size) { 
            warn(__LINE__, "Datagram too large for buffer: truncated and skipped");
        } else {
            unprocessed_statsd_datagram* datagram = (unprocessed_statsd_datagram*) malloc(sizeof(unprocessed_statsd_datagram));
            if (datagram == NULL) {
                die(__LINE__, "Unable to assign memory for struct representing unprocessed datagram.");
            }
            datagram->value = (char*) malloc(sizeof(char) * (count + 1));
            if (datagram->value == NULL) {
                die(__LINE__, "Unable to assign memory for datagram value.");
            }
            strncpy(datagram->value, buffer, count);
            datagram->value[count + 1] = '\0';
            pthread_mutex_lock(q->mutex);
            while(q->full) {
                pthread_cond_wait(q->not_full, q->mutex);
            }
            queue_enqueue(q, datagram);
            pthread_mutex_unlock(q->mutex);
            pthread_cond_signal(q->not_empty);
        }
        memset(buffer, 0, max_udp_packet_size);
    }
    free(buffer);
    return NULL;
}


statsd_listener_args* create_listener_args(agent_config* config, queue* q) {
    struct statsd_listener_args* listener_args = (struct statsd_listener_args*) malloc(sizeof(struct statsd_listener_args));
    if (listener_args == NULL) {
        die(__LINE__, "Unable to assign memory for listener arguments.");
    }
    listener_args->config = (agent_config*) malloc(sizeof(agent_config));
    if (listener_args->config == NULL) {
        die(__LINE__, "Unable to assign memory for listener config.");
    }
    listener_args->config = config;
    listener_args->unprocessed_datagrams_queue = q;
    return listener_args;
}

statsd_parser_args* create_parser_args(agent_config* config, queue* unprocessed_q, queue* parsed_q) {
    struct statsd_parser_args* parser_args = (struct statsd_parser_args*) malloc(sizeof(struct statsd_parser_args));
    if (parser_args == NULL) {
        die(__LINE__, "Unable to assign memory for parser arguments.");
    }
    parser_args->config = (agent_config*) malloc(sizeof(agent_config*));
    if (parser_args->config == NULL) {
        die(__LINE__, "Unable to assign memory for parser config.");
    }
    parser_args->config = config;
    parser_args->unprocessed_datagrams_queue = unprocessed_q;
    parser_args->parsed_datagrams_queue = parsed_q;
    return parser_args;
}

consumer_args* create_consumer_args(agent_config* config, queue* parsed_q) {
    struct consumer_args* consumer_args = (struct consumer_args*) malloc(sizeof(struct consumer_args));
    if (consumer_args == NULL) {
        die(__LINE__, "Unable to assign memory for parser arguments.");
    }
    consumer_args->config = (agent_config*) malloc(sizeof(agent_config*));
    if (consumer_args->config == NULL) {
        die(__LINE__, "Unable to assign memory for parser config.");
    }
    consumer_args->config = config;
    consumer_args->parsed_datagrams_queue = parsed_q;
    return consumer_args;
}

void print_out_datagram(statsd_datagram* datagram) {
    printf("DATAGRAM: \n");
    printf("data_namespace: %s \n", datagram->data_namespace);
    printf("metric: %s \n", datagram->metric);
    printf("instance: %s \n", datagram->instance);
    printf("tags: %s \n", datagram->tags);
    printf("value: %f \n", datagram->value);
    printf("type: %s \n", datagram->type);
    printf("sampling: %s \n", datagram->sampling);
    printf("------------------------------ \n");
}

void free_datagram(statsd_datagram* datagram) {
    free(datagram->data_namespace);
    free(datagram->metric);
    free(datagram->instance);
    free(datagram->tags);
    free(datagram->type);
    free(datagram->sampling);
    free(datagram);
}

void* statsd_parser_consume(void* args) {
    queue* unparsed_q = ((statsd_parser_args*)args)->unprocessed_datagrams_queue;
    queue* parsed_q = ((statsd_parser_args*)args)->parsed_datagrams_queue;
    agent_config* config = ((statsd_parser_args*)args)->config;
    statsd_datagram* (*parse_datagram)(char*);
    if (config->parser_type == PARSER_TRIVIAL) {
        parse_datagram = &basic_parser_parse;
    } else {
        // not implemented
        // parse_datagram = &ragel_parser_parse;
        return NULL;
    }
    while(1) {
        pthread_mutex_lock(unparsed_q->mutex);
		while (unparsed_q->empty) {
			pthread_cond_wait(unparsed_q->not_empty, unparsed_q->mutex);
		}
        statsd_datagram* datagram = parse_datagram(((unprocessed_statsd_datagram*) queue_dequeue(unparsed_q))->value);
        // <insert into queue for categorization>
        pthread_mutex_lock(parsed_q->mutex);
        while(parsed_q->full) {
            pthread_cond_wait(parsed_q->not_full, parsed_q->mutex);
        }
        queue_enqueue(parsed_q, datagram);
        pthread_mutex_unlock(parsed_q->mutex);
        pthread_cond_signal(parsed_q->not_empty);
        // </insert into queue for categorization>
		pthread_mutex_unlock(unparsed_q->mutex);
		pthread_cond_signal(unparsed_q->not_full);
    }
}