#include "../../../config-reader/config-reader.h"
#include "../../../statsd-parsers/statsd-parsers.h"
#include "../../../utils/utils.h"
#include "../../consumers.h"
#include "./duration.h"
#include <hdr/hdr_histogram.h>
    
static duration_metric_collection g_durations = { 0 };    

// TODO: do any initialization local to duration consumer, should that be needed
void init_duration_consumer(agent_config* config) {
    
}

void process_duration(statsd_datagram* datagram) {
    duration_metric* duration = (struct duration_metric*) malloc(sizeof(duration_metric));
    ALLOC_CHECK("Unable to allocate memory for placeholder duration metric");
    *duration = (struct duration_metric) { 0 };
    if (find_histogram_by_name(datagram->metric, &duration)) {
        if (!update_duration_record(duration, datagram)) {
            verbose_log("Thrown away. REASON: semantically incorrect values. (%s)", datagram->value);
            free_datagram(datagram);
        }
    } else {
        if (create_duration_record(datagram, &duration)) {
            add_duration_record(duration);
        } else {
            verbose_log("Thrown away. REASON: semantically incorrect values. (%s)", datagram->value);
            free_datagram(datagram);
        }
    }
}

int print_duration_metric_collection(FILE* out) {
    long int i;
    for (i = 0; i < g_durations.length; i++) {
        fprintf(out, "%s (duration) \n", g_durations.values[i]->name);
        hdr_percentiles_print(
            g_durations.values[i]->histogram,
            out,
            5,
            1.0,
            CLASSIC
        );
        fprintf(out, "\n");
    }
    return g_durations.length;
}

int find_histogram_by_name(char* name, duration_metric** out) {
    long int i;
    for (i = 0; i < g_durations.length; i++) {
        if (strcmp(name, g_durations.values[i]->name) == 0) {
            *out = g_durations.values[i];
            return 1;
        }
    }
    return 0;
}

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

duration_metric_collection* add_duration_record(duration_metric* duration) {
    g_durations.values = realloc(g_durations.values, g_durations.length + 1);
    ALLOC_CHECK("Unable to allocate memory for duration values");
    g_durations.values[g_durations.length] = duration;
    g_durations.length++;
    return &g_durations;
}

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

duration_metric_collection* get_durations() {
    return &g_durations;
}
