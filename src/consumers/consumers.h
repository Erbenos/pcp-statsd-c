#include "../statsd-parsers/statsd-parsers.h"

void* consume_datagram(void* args);

void consumer_request_output();

void print_recorded_values(agent_config* config);
