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
 * Writes information about recorded durations into file
 * @arg m - Metrics struct (what values to print)
 * @arg config - Config containing information about where to output
 */
void print_recorded_durations(metrics* m, agent_config* config);

/**
 * Find histogram by name
 * @arg name - Metric name to search for
 * @arg out - Placeholder histogram metric
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
duration_metric_collection* add_duration_record(metrics* m, duration_metric* duration);

/**
 * Update duration record
 * @arg duration - Duration metric to be updated
 * @arg datagram - Data with which to update
 * @return 1 on success
 */
int update_duration_record(duration_metric* duration, statsd_datagram* datagram);

#endif