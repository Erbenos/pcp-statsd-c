#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <chan/chan.h>
#include "statsd-parsers.h"
#include "basic/basic.h"
#include "ragel/ragel.h"
#include "../utils/utils.h"

const int PARSER_TRIVIAL = 0;
const int PARSER_RAGEL = 1;

void* statsd_network_listen(void* args) {
    agent_config* config = ((statsd_listener_args*)args)->config;
    chan_t* unprocessed_datagrams = ((statsd_listener_args*)args)->unprocessed_datagrams;
    
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
    verbose_log("Socket enstablished.");
    verbose_log("Waiting for datagrams.");
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
            ALLOC_CHECK("Unable to assign memory for struct representing unprocessed datagrams.");
            datagram->value = (char*) malloc(sizeof(char) * (count + 1));
            ALLOC_CHECK("Unable to assign memory for datagram value.");
            strncpy(datagram->value, buffer, count);
            datagram->value[count + 1] = '\0';
            chan_send(unprocessed_datagrams, datagram);
        }
        memset(buffer, 0, max_udp_packet_size);
    }
    free(buffer);
    return NULL;
}


statsd_listener_args* create_listener_args(agent_config* config, chan_t* unprocessed_channel) {
    struct statsd_listener_args* listener_args = (struct statsd_listener_args*) malloc(sizeof(struct statsd_listener_args));
    ALLOC_CHECK("Unable to assign memory for listener arguments.");
    listener_args->config = (agent_config*) malloc(sizeof(agent_config));
    ALLOC_CHECK("Unable to assign memory for listener config.");
    listener_args->config = config;
    listener_args->unprocessed_datagrams = unprocessed_channel;
    return listener_args;
}

statsd_parser_args* create_parser_args(agent_config* config, chan_t* unprocessed_channel, chan_t* parsed_channel) {
    struct statsd_parser_args* parser_args = (struct statsd_parser_args*) malloc(sizeof(struct statsd_parser_args));
    ALLOC_CHECK("Unable to assign memory for parser arguments.");
    parser_args->config = (agent_config*) malloc(sizeof(agent_config*));
    ALLOC_CHECK("Unable to assign memory for parser config.");
    parser_args->config = config;
    parser_args->unprocessed_datagrams = unprocessed_channel;
    parser_args->parsed_datagrams = parsed_channel;
    return parser_args;
}

consumer_args* create_consumer_args(agent_config* config, chan_t* parsed_channel) {
    struct consumer_args* consumer_args = (struct consumer_args*) malloc(sizeof(struct consumer_args));
    ALLOC_CHECK("Unable to assign memory for parser aguments.");
    consumer_args->config = (agent_config*) malloc(sizeof(agent_config*));
    ALLOC_CHECK("Unable to assign memory for parser config.");
    consumer_args->config = config;
    consumer_args->parsed_datagrams = parsed_channel;
    return consumer_args;
}

void print_out_datagram(statsd_datagram* datagram) {
    printf("DATAGRAM: \n");
    printf("metric: %s \n", datagram->metric);
    printf("instance: %s \n", datagram->instance);
    if (datagram->tags != NULL) {
        print_out_datagram_tags(datagram->tags);
    }
    printf("value: %s \n", datagram->value);
    printf("type: %s \n", datagram->type);
    printf("sampling: %s \n", datagram->sampling);
    printf("------------------------------ \n");
}

void print_out_datagram_tags(tag_collection* collection) {
    printf("tags: \n");
    for (int i = 0; i < collection->length; i++) {
        printf("\t %s: %s \n", collection->values[i]->key, collection->values[i]->value);
    }
}

void free_datagram(statsd_datagram* datagram) {
    free(datagram->metric);
    free(datagram->instance);
    free_datagram_tags(datagram->tags);
    free(datagram->type);
    free(datagram->sampling);
    free(datagram);
}

void free_datagram_tags(tag_collection* tags) {
    for (int i = 0; i < tags->length; i++) {
        free(tags->values[i]->key);
        free(tags->values[i]->value);
    }
    free(tags->values);
    free(tags);
}

void* statsd_parser_consume(void* args) {
    chan_t* unprocessed_channel = ((statsd_parser_args*)args)->unprocessed_datagrams;
    chan_t* parsed_channel = ((statsd_parser_args*)args)->parsed_datagrams;
    agent_config* config = ((statsd_parser_args*)args)->config;
    statsd_datagram* (*parse_datagram)(char*);
    if (config->parser_type == PARSER_TRIVIAL) {
        parse_datagram = &basic_parser_parse;
    } else {
        // parse_datagram = &ragel_parser_parse;
        return NULL;
    }
    unprocessed_statsd_datagram* datagram = (unprocessed_statsd_datagram*) malloc(sizeof(unprocessed_statsd_datagram));
    ALLOC_CHECK("Unable to allocate space for unprocessed statsd datagram.");
    while(1) {
        *datagram = (unprocessed_statsd_datagram) { 0 };
        chan_recv(unprocessed_channel, (void *)&datagram);
        chan_send(parsed_channel, parse_datagram(datagram->value));
    }
}