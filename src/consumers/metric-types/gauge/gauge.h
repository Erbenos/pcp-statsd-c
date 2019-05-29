#include "../../../config-reader/config-reader.h"
#include "../../../statsd-parsers/statsd-parsers.h"
#include "../../shared/shared.h"
#include "../../consumers.h"

typedef struct gauge_metric {
    char* name;
    metric_metadata* meta;
    signed long long int value;
} gauge_metric;

typedef struct gauge_metric_collection {
    gauge_metric** values;
    long int length;
} gauge_metric_collection;

void init_gauge_consumer(agent_config* config);

void process_gauge(statsd_datagram* datagram);

int find_gauge_by_name(char* name, gauge_metric** out);

int create_gauge_record(statsd_datagram* datagram, gauge_metric** out);

gauge_metric_collection* add_gauge_record(gauge_metric* gauge);

int update_gauge_record(gauge_metric* gauge, statsd_datagram* datagram);

gauge_metric_collection* get_gauges();
