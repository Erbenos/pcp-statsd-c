#ifndef GAUGE_
#define GAUGE_

#include "../../../config-reader/config-reader.h"
#include "../../../statsd-parsers/statsd-parsers.h"
#include "../../shared/shared.h"
#include "../../consumers.h"

typedef struct gauge_metric {
    char* name;
    metric_metadata* meta;
    signed long long int value;
} gauge_metric;

typedef struct gauge_metric_collection {
    gauge_metric** values;
    long int length;
} gauge_metric_collection;

void init_gauge_consumer(agent_config* config);

/**
 * Processes datagram struct into gauge metric 
 * @arg m - Metrics struct acting as metrics wrapper
 * @arg datagram - Datagram to be processed
 */
void process_gauge(metrics* m, statsd_datagram* datagram);

/**
 * Writes information about recorded gauges into file
 * @arg out - OPENED file handle
 * @return Total count of counters printed
 */
int print_gauge_metric_collection(metrics* m, FILE* out);

/**
 * Find gauge by name
 * @arg name - Metric name to search for
 * @arg out - Placeholder gauge_metric
 * @return 1 when any found
 */
int find_gauge_by_name(metrics* m, char* name, gauge_metric** out);

/**
 * Create gauge record 
 * @arg datagram - Datagram with data that should populate new gauge record
 * @arg out - Placeholder gauge_metric
 * @return 1 on success
 */
int create_gauge_record(statsd_datagram* datagram, gauge_metric** out);

/**
 * Adds gauge record
 * @arg gauge - Gauge metric to me added
 * @return all gauges
 */
gauge_metric_collection* add_gauge_record(metrics* m, gauge_metric* gauge);

/**
 * Update gauge record
 * @arg gauge - Gauge metric to be updated
 * @arg datagram - Data with which to update
 * @return 1 on success
 */
int update_gauge_record(gauge_metric* gauge, statsd_datagram* datagram);

#endif