#include "../../../config-reader/config-reader.h"
#include "../../../statsd-parsers/statsd-parsers.h"
#include "../../../utils/utils.h"
#include "../../consumers.h"
#include "./duration.h"
    
static duration_metric_collection g_durations = { 0 };    

// TODO: empty implementation
void process_duration(statsd_datagram* datagram, agent_config* config) {

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

// TODO: empty implementation
int create_duration_record(statsd_datagram* datagram, duration_metric** out) {
    return 0;
}

duration_metric_collection* add_duration_record(duration_metric* duration) {
    g_durations.values = realloc(g_durations.values, g_durations.length + 1);
    ALLOC_CHECK("Unable to allocate memory for duration values");
    g_durations.values[g_durations.length] = duration;
    g_durations.length++;
    return &g_durations;
}

// TODO: empty implementation
int update_duration_record(duration_metric* duration, statsd_datagram* datagram) {
    return 0;
}

duration_metric_collection* get_durations() {
    return &g_durations;
}
