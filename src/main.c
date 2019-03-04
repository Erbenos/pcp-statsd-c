#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "config-reader/config-reader.h"
#include "statsd-parsers/statsd-parsers.h"
#include "histograms/histograms.h"
#include "utils/utils.h"

int main(int argc, char **argv)
{
    pthread_t network_listener;
    pthread_t datagram_parser;

    agent_config* config = (agent_config*) malloc(sizeof(agent_config));
    int config_src_type = argc >= 2 ? READ_FROM_CMD : READ_FROM_FILE;
    config = read_agent_config(config_src_type, "config", argc, argv);
    init_loggers(config);
    print_agent_config(config);

    statsd_listener_args* listener_args = create_listener_args(config);
    statsd_parser_args* parser_args = create_parser_args(config);

    pthread_create(&network_listener, NULL, statsd_network_listen, listener_args);
    pthread_create(&datagram_parser, NULL, statsd_parser_consume, parser_args);

    if (pthread_join(network_listener, NULL) != 0) {
        die(__LINE__, "Error joining network listener thread.");
    }
    if (pthread_join(datagram_parser, NULL) != 0) {
        die(__LINE__, "Error joining datagram parser thread.");
    }
    return 1;
}