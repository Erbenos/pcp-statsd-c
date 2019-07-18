#ifndef AGGREGATOR_METRICS_
#define AGGREGATOR_METRICS_

#include <stddef.h>
#include <pcp/dict.h>
#include <chan/chan.h>

#include "config-reader.h"
#include "parsers.h"
#include "network-listener.h"

typedef dict metrics;

struct metric_metadata {
    char* tags;
    char* instance;
    double sampling;
} metric_metadata;

struct metric {
    char* name;
    struct metric_metadata* meta;
    void* value;
    enum METRIC_TYPE type;
} metric;

struct pmda_metrics_container {
    metrics* metrics;
    pthread_mutex_t mutex;
} pmda_metrics_container;

struct pmda_metrics_dict_privdata {
    struct agent_config* config;
    struct pmda_metrics_container* container;
} pmda_metrics_dict_privdata;

/**
 * Creates new pmda_metrics_container structure, initializes all stats to 0
 */
struct pmda_metrics_container*
init_pmda_metrics(struct agent_config* config);

/**
 * Processes datagram struct into metric 
 * @arg config - Agent config
 * @arg container - Metrics struct acting as metrics wrapper
 * @arg datagram - Datagram to be processed
 * @return status - 1 when successfully saved or updated, 0 when thrown away
 */
int
process_datagram(struct agent_config* config, struct pmda_metrics_container* container, struct statsd_datagram* datagram);

/**
 * Frees metric
 * @arg config - Agent config
 * @arg m - Metrics struct acting as metrics wrapper (optional)
 * @arg metric - Metric to be freed
 * 
 * Synchronized by mutex on pmda_metrics_container (if any passed)
 */
void
free_metric(struct agent_config* config, struct pmda_metrics_container* container, struct metric* item);

/**
 * Writes information about recorded metrics into file
 * @arg config - Config containing information about where to output
 * @arg container - Metrics struct acting as metrics wrapper
 * 
 * Synchronized by mutex on pmda_metrics_container
 */
void
write_metrics_to_file(struct agent_config* config, struct pmda_metrics_container* container);

/**
 * Finds metric by name
 * @arg container - Metrics container
 * @arg name - Metric name to search for
 * @arg out - Placeholder metric
 * @return 1 when any found
 * 
 * Synchronized by mutex on pmda_metrics_container
 */
int
find_metric_by_name(struct pmda_metrics_container* container, char* name, struct metric** out);

/**
 * Creates metric
 * @arg config - Agent config
 * @arg datagram - Datagram with data that should populate new metric
 * @arg out - Placeholder metric
 * @return 1 on success
 */
int
create_metric(struct agent_config* config, struct statsd_datagram* datagram, struct metric** out);

/**
 * Adds metric to hashtable
 * @arg container - Metrics container 
 * @arg counter - Metric to be saved
 * 
 * Synchronized by mutex on pmda_metrics_container
 */
void
add_metric(struct pmda_metrics_container* container, char* key, struct metric* item);

/**
 * Updates counter record
 * @arg config - Agent config
 * @arg container - Metrics container
 * @arg counter - Metric to be updated
 * @arg datagram - Data with which to update
 * @return 1 on success
 * 
 * Synchronized by mutex on pmda_metrics_container
 */
int
update_metric(
    struct agent_config* config,
    struct pmda_metrics_container* container,
    struct metric* item,
    struct statsd_datagram* datagram
);

/**
 * Checks if given metric name is available (it isn't recorded yet or is blacklisted)
 * @arg container - Metrics container
 * @arg name - Name to be checked
 * @return 1 on success else 0
 */
int
check_metric_name_available(struct pmda_metrics_container* container, char* name);

/**
 * Creates metric metadata
 * @arg datagram - Datagram from which to build metadata
 * @return metric metadata
 */
struct metric_metadata*
create_metric_meta(struct statsd_datagram* datagram);

/**
 * Frees metric metadata
 * @arg metadata - Metadata to be freed
 */ 
void
free_metric_metadata(struct metric_metadata* meta);

/**
 * Prints metadata 
 * @arg f - Opened file handle, doesn't close it after finishing
 * @arg meta - Metric metadata
 */
void
print_metric_meta(FILE* f, struct metric_metadata* meta);

#endif
