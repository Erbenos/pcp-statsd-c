#include <stdio.h>
#include <string.h>
#include <chan/chan.h>

#include "consumers.h"
#include "./metric-types/counter/counter.h"
#include "./metric-types/duration/duration.h"
#include "./metric-types/gauge/gauge.h"
#include "../../utils/utils.h"
#include "../../statsd-parsers/statsd-parsers.h"


void* consume_datagram(void* args) {
    chan_t* parsed = ((consumer_args*)args)->parsed_datagrams;
    agent_config* config = ((consumer_args*)args)->config;
    init_duration_consumer(config);
    init_counter_consumer(config);
    init_gauge_consumer(config);
    statsd_datagram* datagram = (statsd_datagram*) malloc(sizeof(statsd_datagram));
    while(1) {
        *datagram = (statsd_datagram) { 0 };
        chan_recv(parsed, (void *)&datagram);
        if (strcmp(datagram->type, "ms") == 0) {
            process_duration(datagram);
        } else if (strcmp(datagram->type, "c") == 0) {
            process_counter(datagram);
        } else if (strcmp(datagram->type, "g") == 0) {
            process_gauge(datagram);
        }
    }
}
