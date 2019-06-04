#ifndef COUNTER_
#define COUNTER_

void init_counter_consumer(agent_config* config);

/**
 * Processes datagram struct into counter metric 
 * @arg m - Metrics struct acting as metrics wrapper
 * @arg datagram - Datagram to be processed
 */
void process_counter(metrics* m, statsd_datagram* datagram);

/**
 * Frees counter metric record
 * @arg metric - Metric to be freed
 */
void free_counter_metric(counter_metric* metric);

/**
 * Writes information about recorded counters into file
 * @arg out - OPENED file handle
 * @return Total count of counters printed
 */
int print_counter_metric_collection(metrics* m, FILE* out);

/**
 * Find counter by name
 * @arg name - Metric name to search for
 * @arg out - Placeholder counter metric into which contents of item are passed into
 * @return 1 when any found
 */
int find_counter_by_name(metrics* m, char* name, counter_metric** out);

/**
 * Create counter record 
 * @arg datagram - Datagram with data that should populate new counter record
 * @arg out - Placeholder counter_metric
 * @return 1 on success
 */
int create_counter_record(statsd_datagram* datagram, counter_metric** out);

/**
 * Adds counter record
 * @arg counter - Counter metric to me added
 * @return all counters
 */
dict* add_counter_record(metrics* m, counter_metric* counter);

/**
 * Update counter record
 * @arg counter - Counter metric to be updated
 * @arg datagram - Data with which to update
 * @return 1 on success
 */
int update_counter_record(metrics* m, counter_metric* counter, statsd_datagram* datagram);

#endif
