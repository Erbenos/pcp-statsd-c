#include <errno.h>
#include <string.h>
#include "../../../config-reader/config-reader.h"
#include "../../../statsd-parsers/statsd-parsers.h"
#include "../../../utils/utils.h"
#include "../../consumers.h"
#include "./counter.h"

void init_counter_consumer(agent_config* config) {
    (void)config;
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
        if (!update_counter_record(m, counter, datagram)) {
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
 * Frees counter metric record
 * @arg metric - Metric to be freed
 */
void free_counter_metric(counter_metric* metric) {
    if (metric->name != NULL) {
        free(metric->name);
    }
    if (metric->meta != NULL) {
        free_metric_metadata(metric->meta);
    }
    free(metric);
}

/**
 * Writes information about recorded counters into file
 * @arg m - Metrics struct (what values to print)
 * @arg config - Config containing information about where to output
 */
void print_recorded_counters(metrics* m, agent_config* config) {
    if (strlen(config->debug_output_filename) == 0) return; 
    FILE* f;
    f = fopen(config->debug_output_filename, "a+");
    if (f == NULL) {
        return;
    }
    dictIterator* iterator = dictGetSafeIterator(m->counters);
    dictEntry* current;
    while ((current = dictNext(iterator)) != NULL) {
        counter_metric* metric = (counter_metric*)current->v.val;
        fprintf(f, "%s = %llu (counter)\n", metric->name, metric->value);
    }
    dictReleaseIterator(iterator);
    fprintf(f, "Total number of counter records: %lu \n", m->counters->ht[0].size);
    fclose(f);
}

/**
 * Find counter by name
 * @arg name - Metric name to search for
 * @arg out - Placeholder counter metric
 * @return 1 when any found
 */
int find_counter_by_name(metrics* m, char* name, counter_metric** out) {
    dict* counters = m->counters;
    dictEntry* result = dictFind(counters, name);
    if (result == NULL) {
        return 0;
    }
    if (out != NULL) {
        counter_metric* metric = (counter_metric*)result->v.val; 
        (*out)->name = malloc(sizeof(char) * (strlen(metric->name)));
        strcpy((*out)->name, metric->name);
        (*out)->value = metric->value;
        copy_metric_meta((*out)->meta, metric->meta);
    }
    return 1;
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
dict* add_counter_record(metrics* m, counter_metric* counter) {
    dict* counters = m->counters;
    dictAdd(counters, counter->name, counter);
    return counters;
}

/**
 * Update counter record
 * @arg counter - Counter metric to be updated
 * @arg datagram - Data with which to update
 * @return 1 on success
 */
int update_counter_record(metrics* m, counter_metric* counter, statsd_datagram* datagram) {
    dict* counters = m->counters;
    if (datagram->value[0] == '-' || datagram->value[0] == '+') {
        return 0;
    }
    long long unsigned int value = strtoull(datagram->value, NULL, 10);
    if (errno == ERANGE) {
        return 0;
    }
    counter->value += value;
    dictReplace(counters, counter->name, counter);
    return 1;
}
