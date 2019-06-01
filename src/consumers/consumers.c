#include <stdio.h>
#include <string.h>
#include <chan/chan.h>

#include "consumers.h"
#include "./metric-types/counter/counter.h"
#include "./metric-types/duration/duration.h"
#include "./metric-types/gauge/gauge.h"
#include "../../utils/utils.h"
#include "../../statsd-parsers/statsd-parsers.h"

static int g_output_requested = 0;

metrics* init_metrics(agent_config* config) {
    metrics* m = (metrics*) malloc(sizeof(metrics));
    ALLOC_CHECK("Unable to allocate memory for metrics wrapper");
    m->counters = (counter_metric_collection*) malloc(sizeof(counter_metric_collection));
    ALLOC_CHECK("Unable to allocate memory for counter collection");
    m->gauges = (gauge_metric_collection*) malloc(sizeof(gauge_metric_collection));
    ALLOC_CHECK("Unable to allocate memory for gauge collection");
    m->durations = (duration_metric_collection*) malloc(sizeof(duration_metric_collection));
    ALLOC_CHECK("Unable to allocate memory for duration collection");
    m->counters = (counter_metric_collection*) {0}; 
    m->gauges = (gauge_metric_collection*) {0}; 
    m->durations = (duration_metric_collection*) {0}; 
    return m;
}

void* consume_datagram(void* args) {
    chan_t* parsed = ((consumer_args*)args)->parsed_datagrams;
    agent_config* config = ((consumer_args*)args)->config;
    metrics* m = ((consumer_args*)args)->metrics_wrapper;
    init_duration_consumer(config);
    init_counter_consumer(config);
    init_gauge_consumer(config);
    statsd_datagram* datagram = (statsd_datagram*) malloc(sizeof(statsd_datagram));
    while(1) {
        *datagram = (statsd_datagram) { 0 };
        switch(chan_select(&parsed, 1, (void *)&datagram, NULL, 0, NULL)) {
            case 0:
                {
                    if (strcmp(datagram->type, "ms") == 0) {
                        process_duration(m, datagram);
                    } else if (strcmp(datagram->type, "c") == 0) {
                        process_counter(m, datagram);
                    } else if (strcmp(datagram->type, "g") == 0) {
                        process_gauge(m, datagram);
                    }
                }
                break;
            default:
                {
                    if (g_output_requested) {
                        verbose_log("Output of recorded values request caught.");
                        print_recorded_values(m, config);
                        verbose_log("Recorded values output.");
                        g_output_requested = 0;
                    }
                }
        }
    }
}

void consumer_request_output() {
    g_output_requested = 1;
}

int check_metric_name_available(metrics* m, char* name) {
    if (!find_counter_by_name(m, name, NULL) ||
        !find_gauge_by_name(m, name, NULL) ||
        !find_histogram_by_name(m, name, NULL)) {
            return 1;
        }
    return 0;
}

void print_recorded_values(metrics* m, agent_config* config) {
    if (strlen(config->debug_output_filename) == 0) return; 
    FILE* f;
    f = fopen(config->debug_output_filename, "w+");
    long long int record_count = 0;
    if (f != NULL) {
        record_count += print_counter_metric_collection(m, f);
        record_count += print_gauge_metric_collection(m, f);
        record_count += print_duration_metric_collection(m, f);
    }
    fprintf(f, "Total number of records: %llu \n", record_count);
    fclose(f);
}
