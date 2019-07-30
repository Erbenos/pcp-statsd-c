#ifndef AGGREGATOR_DURATION_EXACT_
#define AGGREGATOR_DURATION_EXACT_

#include <stdio.h>
#include <stddef.h>

#include "aggregator-metrics.h"
#include "aggregator-metric-duration.h"
#include "config-reader.h"

/**
 * Represents basic duration aggregation unit
 */
struct exact_duration_collection {
    double** values;
    size_t length;
} exact_duration_collection;

/**
 * Creates exact duration value
 * @arg value - initial value
 * @return hdr_histogram
 */
struct exact_duration_collection*
create_exact_duration_value(long long unsigned int value);

/**
 * Adds item to duration collection, no ordering happens on add
 * @arg collection - Collection to which value should be added
 * @arg value - New value
 */
void
update_exact_duration_value(struct exact_duration_collection* collection, double value);
 
/**
 * Removes item from duration collection
 * @arg collection - Target collection
 * @arg value - Value to be removed, assuming primitive type
 * @return 1 on success
 */
int
remove_exact_duration_item(struct exact_duration_collection* collection, double value);

/**
 * Gets duration values meta data from given collection, as a sideeffect it sorts the values
 * @arg collection - Target collection
 * @arg instance - What information to extract
 * @return duration instance value
 */
double
get_exact_duration_instance(struct exact_duration_collection* collection, enum DURATION_INSTANCE instance);

/**
 * Prints duration collection metadata in human readable way
 * @arg f - Opened file handle, doesn't close it when finished
 * @arg collection - Target collection
 */
void
print_exact_durations(FILE* f, struct exact_duration_collection* collection);

/**
 * Frees exact duration metric value
 * @arg config
 * @arg metric - Metric value to be freed
 */
void
free_exact_duration_value(struct agent_config* config, struct metric* item);

#endif
