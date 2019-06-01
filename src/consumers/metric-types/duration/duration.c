#include "../../../config-reader/config-reader.h"
#include "../../../statsd-parsers/statsd-parsers.h"
#include "../../../utils/utils.h"
#include "../../consumers.h"
#include "./duration.h"
#include <hdr/hdr_histogram.h>

#ifndef DURATION_
#define DURATION_

static duration_metric_collection g_durations = { 0 };    

// TODO: do any initialization local to duration consumer, should that be needed
void init_duration_consumer(agent_config* config) {
    
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
        if (!update_duration_record(duration, datagram)) {
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
 * Writes information about recorded histograms into file
 * @arg out - OPENED file handle
 * @return Total count of histograms printed
 */
int print_duration_metric_collection(metrics* m, FILE* out) {
    duration_metric_collection* durations = m->durations;
    long int i;
    for (i = 0; i < durations->length; i++) {
        fprintf(out, "%s (duration) \n", durations->values[i]->name);
        hdr_percentiles_print(
            durations->values[i]->histogram,
            out,
            5,
            1.0,
            CLASSIC
        );
        fprintf(out, "\n");
    }
    return durations->length;
}

/**
 * Find histogram by name
 * @arg name - Metric name to search for
 * @arg out - Placeholder histogram metric
 * @return 1 when any found
 */
int find_histogram_by_name(metrics* m, char* name, duration_metric** out) {
    duration_metric_collection* durations = m->durations;
    long int i;
    for (i = 0; i < durations->length; i++) {
        if (strcmp(name, durations->values[i]->name) == 0) {
            if (out != NULL) {
                *out = durations->values[i];
            }
            return 1;
        }
    }
    return 0;
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
duration_metric_collection* add_duration_record(metrics* m, duration_metric* duration) {
    duration_metric_collection* durations = m->durations;
    durations->values = realloc(durations->values, durations->length + 1);
    ALLOC_CHECK("Unable to allocate memory for duration values");
    durations->values[durations->length] = duration;
    durations->length++;
    return durations;
}

/**
 * Update duration record
 * @arg duration - Duration metric to be updated
 * @arg datagram - Data with which to update
 * @return 1 on success
 */
int update_duration_record(duration_metric* duration, statsd_datagram* datagram) {
    if (datagram->value[0] == '-' || datagram->value[0] == '+') {
        return 0;
    }
    long long unsigned int value = strtoull(datagram->value, NULL, 10);
    if (errno == ERANGE) {
        return 0;
    }
    hdr_record_value(duration->histogram, value);
    return 1;
}

#endif
