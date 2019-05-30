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

void* consume_datagram(void* args) {
    chan_t* parsed = ((consumer_args*)args)->parsed_datagrams;
    agent_config* config = ((consumer_args*)args)->config;
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
                        process_duration(datagram);
                    } else if (strcmp(datagram->type, "c") == 0) {
                        process_counter(datagram);
                    } else if (strcmp(datagram->type, "g") == 0) {
                        process_gauge(datagram);
                    }
                }
                break;
            default:
                {
                    if (g_output_requested) {
                        verbose_log("Output of recorded values request caught.");
                        print_recorded_values(config);
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

void print_recorded_values(agent_config* config) {
    if (strlen(config->debug_output_filename) == 0) return; 
    FILE* f;
    f = fopen(config->debug_output_filename, "w+");
    long long int record_count = 0;
    if (f != NULL) {
        record_count += print_counter_metric_collection(f);
        record_count += print_gauge_metric_collection(f);
        record_count += print_duration_metric_collection(f);
    }
    fprintf(f, "Total number of records: %llu \n", record_count);
    fclose(f);
}
