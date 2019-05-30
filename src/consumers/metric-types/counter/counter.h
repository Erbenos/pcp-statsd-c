#include "../../../config-reader/config-reader.h"
#include "../../../statsd-parsers/statsd-parsers.h"
#include "../../shared/shared.h"
#include "../../consumers.h"

typedef struct counter_metric {
    char* name;
    metric_metadata* meta;
    unsigned long long int value;
} counter_metric;

typedef struct counter_metric_collection {
    counter_metric** values;
    long int length;
} counter_metric_collection;

void init_counter_consumer(agent_config* config);

void process_counter(statsd_datagram* datagram);

int print_counter_metric_collection(FILE* out);

int find_counter_by_name(char* name, counter_metric** out);

int create_counter_record(statsd_datagram* datagram, counter_metric** out);

counter_metric_collection* add_counter_record(counter_metric* counter);

int update_counter_record(counter_metric* counter, statsd_datagram* datagram);

counter_metric_collection* get_counters();
