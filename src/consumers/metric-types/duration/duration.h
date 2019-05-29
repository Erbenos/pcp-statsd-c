#include "../../../config-reader/config-reader.h"
#include "../../../statsd-parsers/statsd-parsers.h"
#include "../../shared/shared.h"
#include "../../consumers.h"
#include <hdr/hdr_histogram.h>

typedef struct duration_metric {
    char* name;
    metric_metadata* meta;
    struct hdr_histogram* histogram;
} duration_metric;

typedef struct duration_metric_collection {
    duration_metric** values;
    long int length;
} duration_metric_collection;

void init_duration_consumer(agent_config* config);

void process_duration(statsd_datagram* datagram);

int find_histogram_by_name(char* name, duration_metric** out);

int create_duration_record(statsd_datagram* datagram, duration_metric** out);

duration_metric_collection* add_duration_record(duration_metric* duration);

int update_duration_record(duration_metric* duration, statsd_datagram* datagram);

duration_metric_collection* get_durations();
