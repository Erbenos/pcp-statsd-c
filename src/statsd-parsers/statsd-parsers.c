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

const int PARSER_TRIVIAL = 0;
const int PARSER_RAGEL = 1;

void* statsd_network_listen(void* args) {
    agent_config* config = ((statsd_listener_args*)args)->config;
    void (*handle_datagram)(char[], ssize_t, void (statsd_datagram*));
    handle_datagram = &basic_parser_parse;
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
            warn(__LINE__, "datagram too large for buffer: truncated and skipped");
        } else {
            handle_datagram(buffer, count, print_out_datagram);
        }
        memset(buffer, 0, max_udp_packet_size);
    }
    free(buffer);
    return NULL;
}


// In case we need different params for different threads
statsd_listener_args* create_listener_args(agent_config* config) {
    struct statsd_listener_args* listener_args = (struct statsd_listener_args*) malloc(sizeof(struct statsd_listener_args));
    listener_args->config = (agent_config*) malloc(sizeof(agent_config*));
    listener_args->config = config;
    return listener_args;
}

// In case we need different params for different threads
statsd_parser_args* create_parser_args(agent_config* config) {
    struct statsd_parser_args* parser_args = (struct statsd_parser_args*) malloc(sizeof(struct statsd_parser_args));
    parser_args->config = (agent_config*) malloc(sizeof(agent_config*));
    parser_args->config = config;
    return parser_args;
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
    return NULL;
}