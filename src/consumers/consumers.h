#ifndef CONSUMERS_
#define CONSUMERS_

typedef struct metric_metadata {
    tag_collection* tags;
    char* instance;
    char* sampling;
} metric_metadata;

typedef struct counter_metric {
    char* name;
    metric_metadata* meta;
    unsigned long long int value;
} counter_metric;

typedef struct counter_metric_collection {
    counter_metric** values;
    long int length;
} counter_metric_collection;

typedef struct gauge_metric {
    char* name;
    metric_metadata* meta;
    signed long long int value;
} gauge_metric;

typedef struct gauge_metric_collection {
    gauge_metric** values;
    long int length;
} gauge_metric_collection;

typedef struct duration_metric {
    char* name;
    metric_metadata* meta;
    struct hdr_histogram* histogram;
} duration_metric;

typedef struct duration_metric_collection {
    duration_metric** values;
    long int length;
} duration_metric_collection;


typedef struct metrics {
    counter_metric_collection* counters;
    gauge_metric_collection* gauges;
    duration_metric_collection* durations;
} metrics;

/**
 * Initializes metrics struct to empty values
 * @arg config - Config (should there be need to pass detailed info into metrics)
 */
metrics* init_metrics(agent_config* config);

/**
 * Thread startpoint - passes down given datagram to consumer to record value it contains
 * @arg args - (consumer_args), see ~/statsd-parsers/statsd-parsers.h
 */
void* consume_datagram(void* args);

/**
 * Sets flag notifying that output was requested
 */
void consumer_request_output();

/**
 * Checks if given metric name is available (it isn't recorded yet)
 * @arg m - Metrics struct (storage in which to check for name availability)
 * @arg name - Name to be checked
 * @return 1 on success else 0
 */
int check_metric_name_available(metrics* m, char* name);

/**
 * Prints values of currently recorded metrics with total count, if config allows it, into file that is specified in it under "debug" key
 * @arg m - Metrics struct (what values to print)
 * @arg config - Config to check against
 */
void print_recorded_values(metrics* m, agent_config* config);

metric_metadata* create_record_meta(statsd_datagram* datagram);

#endif
