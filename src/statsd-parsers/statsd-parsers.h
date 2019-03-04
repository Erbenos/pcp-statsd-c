#include "../config-reader/config-reader.h"

#ifndef STATSD_PARSERS_
#define STATSD_PARSERS_

const int PARSER_TRIVIAL;
const int PARSER_RAGEL;


typedef struct statsd_datagram
{
    char *data_namespace;
    char *type;
    char *modifier;
    char *tags;
    double value;
    char *metric;
    char *instance;
    char *sampling;
} statsd_datagram;

typedef struct statsd_listener_args
{
    agent_config* config;
} statsd_listener_args;

statsd_listener_args* create_listener_args(agent_config* config);

void* statsd_network_listen(void* args);

typedef struct statsd_parser_args
{
    agent_config* config;
} statsd_parser_args;

statsd_parser_args* create_parser_args(agent_config* config);

void* statsd_parser_consume(void* args);

void print_out_datagram(statsd_datagram* datagram);

void free_datagram(statsd_datagram* datagram);

#endif