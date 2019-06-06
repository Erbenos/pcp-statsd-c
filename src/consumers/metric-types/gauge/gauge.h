#ifndef GAUGE_
#define GAUGE_

void init_gauge_consumer(agent_config* config);

/**
 * Processes datagram struct into gauge metric 
 * @arg m - Metrics struct acting as metrics wrapper
 * @arg datagram - Datagram to be processed
 */
void process_gauge(metrics* m, statsd_datagram* datagram);

/**
 * Frees gauge metric record
 * @arg metric - Metric to be freed
 */
void free_gauge_metric(gauge_metric* metric);

/**
 * Writes information about recorded gauges into file
 * @arg m - Metrics struct (what values to print)
 * @arg config - Config containing information about where to output
 */
void print_recorded_gauges(metrics* m, agent_config* config);

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
dict* add_gauge_record(metrics* m, gauge_metric* gauge);

/**
 * Update gauge record
 * @arg gauge - Gauge metric to be updated
 * @arg datagram - Data with which to update
 * @return 1 on success
 */
int update_gauge_record(metrics* m, gauge_metric* gauge, statsd_datagram* datagram);

#endif