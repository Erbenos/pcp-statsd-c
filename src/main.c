#include <stdio.h>
#include <stdlib.h>

#include "config-reader/config-reader.h"
#include "statsd-parsers/statsd-parsers.h"
#include "histograms/histograms.h"
#include "utils/utils.h"

int main(int argc, char **argv)
{
    agent_config* config = (agent_config*) malloc(sizeof(agent_config));
    int config_src_type = argc >= 2 ? READ_FROM_CMD : READ_FROM_FILE;
    config = read_agent_config(config_src_type, "config", argc, argv);
    init_loggers(config);
    print_agent_config(config);
    // first lets record stuff and worry about sharing it accross program later
    statsd_parser_listen(config, config->parser_type, consume_datagram);
    return 1;
}