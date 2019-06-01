#include <errno.h>
#include <string.h>
#include "../../../config-reader/config-reader.h"
#include "../../../statsd-parsers/statsd-parsers.h"
#include "../../../utils/utils.h"
#include "../../consumers.h"
#include "./counter.h"

void init_counter_consumer(agent_config* config) {

}

/**
 * Processes datagram struct into counter metric 
 * @arg m - Metrics struct acting as metrics wrapper
 * @arg datagram - Datagram to be processed
 */
void process_counter(metrics* m, statsd_datagram* datagram) {
    counter_metric* counter = (struct counter_metric*) malloc(sizeof(counter_metric));
    ALLOC_CHECK("Unable to allocate memory for placeholder counter record");
    *counter = (struct counter_metric) { 0 };
    if (find_counter_by_name(m, datagram->metric, &counter)) {
        if (!update_counter_record(counter, datagram)) {
            verbose_log("Throwing away datagram, semantically incorrect values.");
            free_datagram(datagram);
        }
    } else {
        if (check_metric_name_available(m, datagram->metric)) {
            if (create_counter_record(datagram, &counter)) {
                add_counter_record(m, counter);
            } else {
                verbose_log("Throwing away datagram, semantically incorrect values.");
                free_datagram(datagram);
            }
        }
    }
}

/**
 * Writes information about recorded counters into file
 * @arg out - OPENED file handle
 * @return Total count of counters printed
 */
int print_counter_metric_collection(metrics* m, FILE* out) {
    counter_metric_collection* counters = m->counters;
    long int i;
    for (i = 0; i < counters->length; i++) {
        fprintf(out, "%s = %llu (counter)\n", counters->values[i]->name, counters->values[i]->value);
    }
    return counters->length;
}

/**
 * Find counter by name
 * @arg name - Metric name to search for
 * @arg out - Placeholder counter metric
 * @return 1 when any found
 */
int find_counter_by_name(metrics* m, char* name, counter_metric** out) {
    counter_metric_collection* counters = m->counters;
    long int i;
    for (i = 0; i < counters->length; i++) {
        if (strcmp(name, counters->values[i]->name) == 0) {
            *out = counters->values[i];
            return 1;
        }
    }
    return 0;
}

/**
 * Create counter record 
 * @arg datagram - Datagram with data that should populate new counter record
 * @arg out - Placeholder counter_metric
 * @return 1 on success
 */
int create_counter_record(statsd_datagram* datagram, counter_metric** out) {
    if (datagram->value[0] == '-' || datagram->value[0] == '+') {
        return 0;
    }
    long long unsigned int value = strtoull(datagram->value, NULL, 10);
    if (errno == ERANGE) {
        return 0;
    }
    (*out)->name = datagram->metric;
    (*out)->value = value;
    (*out)->meta = create_record_meta(datagram);
    return 1;
}

/**
 * Adds counter record
 * @arg counter - Counter metric to me added
 * @return all counters
 */
counter_metric_collection* add_counter_record(metrics* m, counter_metric* counter) {
    counter_metric_collection* counters = m->counters;
    counters->values = realloc(counters->values, counters->length + 1);
    ALLOC_CHECK("Unable to allocate memory for counter values");
    counters->values[counters->length] = counter;
    counters->length++;
    return counters;
}

/**
 * Update counter record
 * @arg counter - Counter metric to be updated
 * @arg datagram - Data with which to update
 * @return 1 on success
 */
int update_counter_record(counter_metric* counter, statsd_datagram* datagram) {
    if (datagram->value[0] == '-' || datagram->value[0] == '+') {
        return 0;
    }
    long long unsigned int value = strtoull(datagram->value, NULL, 10);
    if (errno == ERANGE) {
        return 0;
    }
    counter->value += value;
    return 1;
}
