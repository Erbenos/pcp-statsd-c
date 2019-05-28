#include <errno.h>
#include <string.h>
#include "../../../config-reader/config-reader.h"
#include "../../../statsd-parsers/statsd-parsers.h"
#include "../../../utils/utils.h"
#include "../../consumers.h"
#include "./gauge.h"

static gauge_metric_collection g_gauges = { 0 };

void process_gauge(statsd_datagram* datagram, agent_config* config) {
    gauge_metric* gauge = (struct gauge_metric*) malloc(sizeof(gauge_metric));
    ALLOC_CHECK("Unable to allocate memory for placeholder gauge record");
    *gauge = (struct gauge_metric) { 0 };
    if (find_gauge_by_name(datagram->metric, &gauge)) {
        update_gauge_record(gauge, datagram);
    } else {
        if (create_gauge_record(datagram, &gauge)) {
            add_gauge_record(gauge);
        }
    }
}

int find_gauge_by_name(char* name, gauge_metric** out) {
    long int i;
    for (i = 0; i < g_gauges.length; i++) {
        if (strcmp(name, g_gauges.values[i]->name) == 0) {
            *out = g_gauges.values[i];
            return 1;
        }
    }
    return 0;
}

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

gauge_metric_collection* add_gauge_record(gauge_metric* gauge) {
    g_gauges.values = realloc(g_gauges.values, g_gauges.length + 1);
    ALLOC_CHECK("Unable to allocate memory for gauge values");
    g_gauges.values[g_gauges.length] = gauge;
    g_gauges.length++;
    return &g_gauges;
}

int update_gauge_record(gauge_metric* gauge, statsd_datagram* datagram) {
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
    return 1;
}

gauge_metric_collection* get_gauges() {
    return &g_gauges;
}
