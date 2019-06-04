#include <errno.h>
#include <string.h>
#include "../../../config-reader/config-reader.h"
#include "../../../statsd-parsers/statsd-parsers.h"
#include "../../../utils/utils.h"
#include "../../consumers.h"
#include "./gauge.h"

void init_gauge_consumer(agent_config* config) {
    (void)config;
}

/**
 * Processes datagram struct into gauge metric 
 * @arg m - Metrics struct acting as metrics wrapper
 * @arg datagram - Datagram to be processed
 */
void process_gauge(metrics* m, statsd_datagram* datagram) {
    gauge_metric* gauge = (struct gauge_metric*) malloc(sizeof(gauge_metric));
    ALLOC_CHECK("Unable to allocate memory for placeholder gauge record");
    *gauge = (struct gauge_metric) { 0 };
    if (find_gauge_by_name(m, datagram->metric, &gauge)) {
        if (!update_gauge_record(m, gauge, datagram)) {
            verbose_log("Thrown away. REASON: semantically incorrect values. (%s)", datagram->value);
            free_datagram(datagram);
        }
    } else {
        if (check_metric_name_available(m, datagram->metric)) {
            if (create_gauge_record(datagram, &gauge)) {
                add_gauge_record(m, gauge);
            } else {
                verbose_log("Thrown away. REASON: semantically incorrect values. (%s)", datagram->value);
                free_datagram(datagram);
            }
        }
    }
}

/**
 * Frees gauge metric record
 * @arg metric - Metric to be freed
 */
void free_gauge_metric(gauge_metric* metric) {
    if (metric->name != NULL) {
        free(metric->name);
    }
    if (metric->meta != NULL) {
        free_metric_metadata(metric->meta);
    }
    free(metric);
}


/**
 * Writes information about recorded gauges into file
 * @arg out - OPENED file handle
 * @return Total count of counters printed
 */
int print_gauge_metric_collection(metrics* m, FILE* out) {
    (void)out;
    // gauge_metric_collection* gauges = m->gauges;
    // long int i;
    // for (i = 0; i < gauges->length; i++) {
    //     fprintf(out, "%s = %lli (gauge)\n", gauges->values[i]->name, gauges->values[i]->value);
    // }
    return m->gauges->ht[0].size;
}

/**
 * Find gauge by name
 * @arg name - Metric name to search for
 * @arg out - Placeholder gauge metric into which contents of item are passed into
 * @return 1 when any found
 */
int find_gauge_by_name(metrics* m, char* name, gauge_metric** out) {
    dict* counters = m->counters;
    dictEntry* result = dictFind(counters, name);
    if (result == NULL) {
        return 0;
    }
    if (out != NULL) {
        counter_metric* metric = (counter_metric*)result->v.val; 
        strcpy((*out)->name, metric->name);
        (*out)->value = metric->value;
        copy_metric_meta((*out)->meta, metric->meta);
    }
    return 1;
}

/**
 * Create gauge record 
 * @arg datagram - Datagram with data that should populate new gauge record
 * @arg out - Placeholder gauge_metric
 * @return 1 on success
 */
int create_gauge_record(statsd_datagram* datagram, gauge_metric** out) {
    if (datagram->metric == NULL) {
        return 0;
    }
    signed long long int value = strtoll(datagram->value, NULL, 10);
    if (errno == ERANGE) {
        return 0;
    }
    (*out)->name = datagram->metric;
    (*out)->value = value;
    (*out)->meta = create_record_meta(datagram);
    return 1;
}

/**
 * Adds gauge record
 * @arg gauge - Gauge metric to me added
 * @return all gauges
 */
dict* add_gauge_record(metrics* m, gauge_metric* gauge) {
    dict* gauges = m->gauges;
    dictAdd(gauges, gauge->name, gauge);
    return gauges;
}

/**
 * Update gauge record
 * @arg gauge - Gauge metric to be updated
 * @arg datagram - Data with which to update
 * @return 1 on success
 */
int update_gauge_record(metrics* m, gauge_metric* gauge, statsd_datagram* datagram) {
    dict* gauges = m->gauges;
    int substract = 0;
    int add = 0;
    if (datagram->value[0] == '+') {
        add = 1;
    }
    if (datagram->value[0] == '-') {
        substract = 1; 
    }
    signed long long int value;
    if (substract || add) {
        char* substr = &(datagram->value[1]);
        value = strtoll(substr, NULL, 10);
    } else {
        value = strtoll(datagram->value, NULL, 10);
    }
    if (errno == ERANGE) {
        return 0;
    }
    if (add || substract) {
        if (add) {
            gauge->value += value;
        }
        if (substract) {
            gauge->value -= value;
        }
    } else {
        gauge->value = value;
    }
    dictReplace(gauges, gauge->name, gauge);
    return 1;
}
