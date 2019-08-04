#include <hdr/hdr_histogram.h>
#include <string.h>
#include <float.h>

#include "config-reader.h"
#include "network-listener.h"
#include "aggregators.h"
#include "aggregator-metric-labels.h"
#include "aggregator-metric-duration.h"
#include "aggregator-metric-duration-exact.h"
#include "aggregator-metric-duration-hdr.h"
#include "errno.h"
#include "utils.h"

/**
 * Creates duration value in given dest
 */
int 
create_duration_value(struct agent_config* config, struct statsd_datagram* datagram, void** out) {
    double new_value;
    switch (datagram->explicit_sign) {
        case SIGN_MINUS:
            new_value = -1.0 * datagram->value;
            break;
        default:
            new_value = datagram->value;
    }
    if (new_value < 0 || new_value >= DBL_MAX) {
        return 0;
    }
    if (config->duration_aggregation_type == DURATION_AGGREGATION_TYPE_HDR_HISTOGRAM) {
        create_hdr_duration_value(
            (unsigned long long) new_value, 
            out
        );
    } else {
        create_exact_duration_value(
            (unsigned long long) new_value,
            out
        );
    }
    return 1;
}

/**
 * Updates duration metric record of value subtype
 * @arg config - Config from which we know what duration type is, either HDR or exact
 * @arg item - Item to be updated
 * @arg datagram - Data to update the item with
 * @return 1 on success, 0 on fail
 */
int
update_duration_value(struct agent_config* config, struct statsd_datagram* datagram, void* value) {
    double new_value;
    switch (datagram->explicit_sign) {
        case SIGN_MINUS:
            new_value = -1.0 * datagram->value;
            break;
        default:
            new_value = datagram->value;
    }
    if (new_value < 0) {
        return 0;
    }
    if (config->duration_aggregation_type == DURATION_AGGREGATION_TYPE_HDR_HISTOGRAM) {
        update_hdr_duration_value(
            (unsigned long long) new_value,
            (struct hdr_histogram*) value
        );
    } else {
        update_exact_duration_value(
            (unsigned long long) new_value,
            (struct exact_duration_collection*) value
        );
    }
    return 1;
}

/**
 * Extracts duration metric meta values from duration metric record
 * @arg config - Config which contains info on which duration aggregating type we are using
 * @arg item - Metric item from which to extract duration values
 * @arg instance - What information to extract
 * @return duration instance value
 */
double
get_duration_instance(struct agent_config* config, struct metric* item, enum DURATION_INSTANCE instance) {
    double result = 0;
    if (config->duration_aggregation_type == DURATION_AGGREGATION_TYPE_BASIC) {
        result = get_exact_duration_instance((struct exact_duration_collection*)item->value, instance);
    } else {
        result = get_hdr_histogram_duration_instance((struct hdr_histogram*)item->value, instance);
    }
    return result;
}

/**
 * Print duration metric value
 * @arg config - Config where duration subtype is specified
 * @arg f - Opened file handle
 * @arg value
 */
void
print_duration_metric_value(struct agent_config* config, FILE* f, void* value) {
    switch (config->duration_aggregation_type) {
        case DURATION_AGGREGATION_TYPE_BASIC:
            print_exact_duration_value(f, (struct exact_duration_collection*)value);
            break;
        case DURATION_AGGREGATION_TYPE_HDR_HISTOGRAM:
            print_hdr_duration_value(f, (struct hdr_histogram*)value);
            break;
    }
}

/**
 * Prints duration metric information
 * @arg config - Config where duration subtype is specified
 * @arg f - Opened file handle
 * @arg item - Metric to print out
 */
void
print_duration_metric(struct agent_config* config, FILE* f, struct metric* item) {
    fprintf(f, "-----------------\n");
    fprintf(f, "name = %s\n", item->name);
    fprintf(f, "type = duration\n");
    print_duration_metric_value(config, f, item->value);
    print_labels(config, f, item->children);
    print_metric_meta(f, item->meta);
    fprintf(f, "\n");
}

/**
 * Frees duration metric value
 * @arg config
 * @arg metric - Metric value to be freed
 */
void
free_duration_value(struct agent_config* config, struct metric* item) {
    switch (config->duration_aggregation_type) {
        case DURATION_AGGREGATION_TYPE_BASIC:
            free_exact_duration_value(config, item);
            break;
        case DURATION_AGGREGATION_TYPE_HDR_HISTOGRAM:
            free_hdr_duration_value(config, item);
            break;
    }
}
