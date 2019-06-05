#include <stdio.h>
#include <string.h>
#include <chan/chan.h>

#include "../../statsd-parsers/statsd-parsers.h"
#include "../../utils/utils.h"
#include "consumers.h"
#include "./metric-types/counter/counter.h"
#include "./metric-types/duration/duration.h"
#include "./metric-types/gauge/gauge.h"

/**
 * Flag to capture USR1 signal event, which is supposed mark request to output currently recorded data in debug file 
 */
static int g_output_requested = 0;
pthread_mutex_t g_output_requested_lock;

/**
 * Initializes metrics struct to empty values
 * @arg config - Config (should there be need to pass detailed info into metrics)
 */
metrics* init_metrics(agent_config* config) {
    metrics* m = (metrics*) malloc(sizeof(metrics));
    ALLOC_CHECK("Unable to allocate memory for metrics wrapper");
    m->counters = (counter_metric_collection*) malloc(sizeof(counter_metric_collection));
    ALLOC_CHECK("Unable to allocate memory for counter collection");
    m->gauges = (gauge_metric_collection*) malloc(sizeof(gauge_metric_collection));
    ALLOC_CHECK("Unable to allocate memory for gauge collection");
    m->durations = (duration_metric_collection*) malloc(sizeof(duration_metric_collection));
    ALLOC_CHECK("Unable to allocate memory for duration collection");
    *(m->counters) = (counter_metric_collection) {0}; 
    *(m->gauges) = (gauge_metric_collection) {0}; 
    *(m->durations) = (duration_metric_collection) {0}; 
    return m;
}

/**
 * Thread startpoint - passes down given datagram to consumer to record value it contains
 * @arg args - (consumer_args), see ~/statsd-parsers/statsd-parsers.h
 */
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
                    pthread_mutex_lock(&g_output_requested_lock);
                    if (g_output_requested) {
                        verbose_log("Output of recorded values request caught.");
                        print_recorded_counters(m, config);
                        print_recorded_gauges(m, config);
                        print_recorded_durations(m, config);
                        verbose_log("Recorded values output.");
                        g_output_requested = 0;
                    }
                    pthread_mutex_unlock(&g_output_requested_lock);
                }
        }
    }
}

/**
 * Sets flag notifying that output was requested
 */
void consumer_request_output() {
    pthread_mutex_lock(&g_output_requested_lock);
    g_output_requested = 1;
    pthread_mutex_unlock(&g_output_requested_lock);
}

/**
 * Checks if given metric name is available (it isn't recorded yet)
 * @arg m - Metrics struct (storage in which to check for name availability)
 * @arg name - Name to be checked
 * @return 1 on success else 0
 */
int check_metric_name_available(metrics* m, char* name) {
    if (!find_counter_by_name(m, name, NULL) ||
        !find_gauge_by_name(m, name, NULL) ||
        !find_histogram_by_name(m, name, NULL)) {
            return 1;
        }
    return 0;
}

metric_metadata* create_record_meta(statsd_datagram* datagram) {
    // TODO: this is not yet implemented
    return NULL;
}
