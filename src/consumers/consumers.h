#ifndef CONSUMERS_
#define CONSUMERS_

#include "../config-reader/config-reader.h"
#include "./metric-types/counter/counter.h"
#include "./metric-types/gauge/gauge.h"
#include "./metric-types/duration/duration.h"

typedef struct metrics {
    counter_metric_collection* counters;
    gauge_metric_collection* gauges;
    duration_metric_collection* durations;
} metrics;

metrics* init_metrics(agent_config* config);

void* consume_datagram(void* args);

void consumer_request_output();

void print_recorded_values(metrics* m, agent_config* config);

int check_metric_name_available(metrics* m, char* name);

#endif
