#include <string.h>
#include "../../../config-reader/config-reader.h"
#include "../../../statsd-parsers/statsd-parsers.h"
#include "../../../utils/utils.h"
#include "../../consumers.h"
#include "counter.h"

static counter_metric_collection g_counters = { 0 };

void process_counter(statsd_datagram* datagram, agent_config* config) {
    counter_metric* counter = (struct counter_metric*) malloc(sizeof(counter_metric));
    ALLOC_CHECK("Unable to allocate memory for placeholder counter record");
    *counter = (struct counter_metric) { 0 };
    if (find_counter_by_name(datagram->metric, &counter)) {
        update_counter_record(counter, datagram);
    } else {
        if (create_counter_record(datagram, &counter)) {
          add_counter_record(counter);
        }
    }
}

int find_counter_by_name(char* name, counter_metric** out) {
    long int i;
    for (i = 0; i < g_counters.length; i++) {
        if (strcmp(name, g_counters.values[i]->name) == 0) {
            *out = g_counters.values[i];
            return 1;
        }
    }
    return 0;
}

int create_counter_record(statsd_datagram* datagram, counter_metric** out) {
    if (datagram->metric == NULL) {
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

counter_metric_collection* add_counter_record(counter_metric* counter) {
    g_counters.values = realloc(g_counters.values, g_counters.length + 1);
    ALLOC_CHECK("Unable to allocate memory for counter values");
    g_counters.values[g_counters.length] = counter;
    g_counters.length++;
    return &g_counters;
}

int update_counter_record(counter_metric* counter, statsd_datagram* datagram) {
    long long unsigned int value = strtoull(datagram->value, NULL, 10);
    if (errno == ERANGE) {
        return 0;
    }
    counter->value += value;
    return 1;
}

counter_metric_collection* get_counters() {
    return &g_counters;
}
