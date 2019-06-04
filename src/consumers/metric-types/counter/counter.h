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
 * Writes information about recorded counters into file
 * @arg m - Metrics struct (what values to print)
 * @arg config - Config containing information about where to output
 */
void print_recorded_counters(metrics* m, agent_config* config);

/**
 * Find counter by name
 * @arg name - Metric name to search for
 * @arg out - Placeholder counter metric
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
counter_metric_collection* add_counter_record(metrics* m, counter_metric* counter);

/**
 * Update counter record
 * @arg counter - Counter metric to be updated
 * @arg datagram - Data with which to update
 * @return 1 on success
 */
int update_counter_record(counter_metric* counter, statsd_datagram* datagram);

#endif