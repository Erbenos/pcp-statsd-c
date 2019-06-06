#include <hdr/hdr_histogram.h>
#include <string.h>
#include "../../../config-reader/config-reader.h"
#include "../../../statsd-parsers/statsd-parsers.h"
#include "../../../utils/utils.h"
#include "../../consumers.h"
#include "duration.h"

void init_duration_consumer(agent_config* config) {
    (void)config;    
}

/**
 * Processes datagram struct into duration metric 
 * @arg m - Metrics struct acting as metrics wrapper
 * @arg datagram - Datagram to be processed
 */
void process_duration(metrics* m, statsd_datagram* datagram) {
    duration_metric* duration = (struct duration_metric*) malloc(sizeof(duration_metric));
    ALLOC_CHECK("Unable to allocate memory for placeholder duration metric");
    *duration = (struct duration_metric) { 0 };
    if (find_histogram_by_name(m, datagram->metric, &duration)) {
        if (!update_duration_record(m, duration, datagram)) {
            verbose_log("Thrown away. REASON: semantically incorrect values. (%s)", datagram->value);
            free_datagram(datagram);
        }
    } else {
        if (check_metric_name_available(m, datagram->metric)) {
            if (create_duration_record(datagram, &duration)) {
                add_duration_record(m, duration);
            } else {
                verbose_log("Thrown away. REASON: semantically incorrect values. (%s)", datagram->value);
                free_datagram(datagram);
            }
        }
    }
}

/**
 * Frees duration metric record
 * @arg metric - Metric to be freed
 */
void free_duration_metric(duration_metric* metric) {
    if (metric->name != NULL) {
        free(metric->name);
    }
    if (metric->meta != NULL) {
        free_metric_metadata(metric->meta);
    }
    // cant free histogram here
    free(metric);
}

/**
 * Writes information about recorded durations into file
 * @arg m - Metrics struct (what values to print)
 * @arg config - Config containing information about where to output
 */
void print_recorded_durations(metrics* m, agent_config* config) {
    if (strlen(config->debug_output_filename) == 0) return; 
    FILE* f;
    f = fopen(config->debug_output_filename, "a+");
    if (f == NULL) {
        return;
    }
    dictIterator* iterator = dictGetSafeIterator(m->durations);
    dictEntry* current;
    while ((current = dictNext(iterator)) != NULL) {
        duration_metric* duration = (duration_metric*)current->v.val;
        fprintf(f, "%s (duration) \n", duration->name);
        hdr_percentiles_print(
            duration->histogram,
            f,
            5,
            1.0,
            CLASSIC
        );
        fprintf(f, "\n");
    }
    fprintf(f, "Total number of duration records: %lu \n", m->durations->ht[0].size);
    fclose(f);
}

/**
 * Find histogram by name
 * @arg name - Metric name to search for
 * @arg out - Placeholder histogram metric
 * @return 1 when any found
 */
int find_histogram_by_name(metrics* m, char* name, duration_metric** out) {
    dict* durations = m->durations;
    dictEntry* result = dictFind(durations, name);
    if (result == NULL) {
        return 0;
    }
    if (out != NULL) {
        duration_metric* metric = (duration_metric*)result->v.val;
        (*out)->name = malloc(sizeof(char) * (strlen(metric->name)));
        strcpy((*out)->name, metric->name);
        (*out)->histogram = metric->histogram;
        copy_metric_meta((*out)->meta, metric->meta);
    }
    return 1;
}

/**
 * Create duration record 
 * @arg datagram - Datagram with data that should populate new duration record
 * @arg out - Placeholder duration_metric
 * @return 1 on success
 */
int create_duration_record(statsd_datagram* datagram, duration_metric** out) {
    if (datagram->value[0] == '-' || datagram->value[0] == '+') {
        return 0;
    }
    struct hdr_histogram* histogram;
    hdr_init(1, INT64_C(3600000000), 3, &histogram);
    ALLOC_CHECK("Unable to allocate memory for histogram");
    long long unsigned int value = strtoull(datagram->value, NULL, 10);
    if (errno == ERANGE) {
        return 0;
    }
    hdr_record_value(histogram, value);
    (*out)->name = datagram->metric;
    (*out)->histogram = histogram;
    (*out)->meta = create_record_meta(datagram);
    return 1;
}

/**
 * Adds duration record
 * @arg duration - Duration metric to me added
 * @return all durations
 */
dict* add_duration_record(metrics* m, duration_metric* duration) {
    dict* durations = m->durations;
    dictAdd(durations, duration->name, duration);
    return durations;
}

/**
 * Update duration record
 * @arg duration - Duration metric to be updated
 * @arg datagram - Data with which to update
 * @return 1 on success
 */
int update_duration_record(metrics* m, duration_metric* duration, statsd_datagram* datagram) {
    dict* durations = m->durations;
    if (datagram->value[0] == '-' || datagram->value[0] == '+') {
        return 0;
    }
    long long unsigned int value = strtoull(datagram->value, NULL, 10);
    if (errno == ERANGE) {
        return 0;
    }
    hdr_record_value(duration->histogram, value);
    dictReplace(durations, duration->name, duration);
    return 1;
}
