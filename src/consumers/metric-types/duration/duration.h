#ifndef DURATION_
#define DURATION_

#include <hdr/hdr_histogram.h>

void init_duration_consumer(agent_config* config);

/**
 * Processes datagram struct into duration metric 
 * @arg m - Metrics struct acting as metrics wrapper
 * @arg datagram - Datagram to be processed
 */
void process_duration(metrics* m, statsd_datagram* datagram);

/**
 * Frees duration metric record
 * @arg metric - Metric to be freed
 */
void free_duration_metric(duration_metric* metric);


/**
 * Writes information about recorded histograms into file
 * @arg out - OPENED file handle
 * @return Total count of histograms printed
 */
int print_duration_metric_collection(metrics* m, FILE* out);

/**
 * Find histogram by name
 * @arg name - Metric name to search for
 * @arg out - @arg out - Placeholder counter metric into which contents of item are passed into, histogram itself is copied by reference
 * @return 1 when any found
 */
int find_histogram_by_name(metrics* m, char* name, duration_metric** out);

/**
 * Create duration record 
 * @arg datagram - Datagram with data that should populate new duration record
 * @arg out - Placeholder duration_metric
 * @return 1 on success
 */
int create_duration_record(statsd_datagram* datagram, duration_metric** out);

/**
 * Adds duration record
 * @arg duration - Duration metric to me added
 * @return all durations
 */
dict* add_duration_record(metrics* m, duration_metric* duration);

/**
 * Update duration record
 * @arg duration - Duration metric to be updated
 * @arg datagram - Data with which to update
 * @return 1 on success
 */
int update_duration_record(metrics* m, duration_metric* duration, statsd_datagram* datagram);

#endif