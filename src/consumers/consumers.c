#include <stdio.h>
#include <string.h>
#include <chan/chan.h>

#include "consumers.h"
#include "./metric-types/counter/counter.h"
#include "./metric-types/duration/duration.h"
#include "./metric-types/gauge/gauge.h"
#include "../../utils/utils.h"
#include "../../statsd-parsers/statsd-parsers.h"

bucket_collection buckets = { 0 };

void* consume_datagram(void* args) {
    chan_t* parsed = ((consumer_args*)args)->parsed_datagrams;
    agent_config* config = ((consumer_args*)args)->config;
    statsd_datagram* datagram = (statsd_datagram*) malloc(sizeof(statsd_datagram));
    while(1) {
        *datagram = (statsd_datagram) { 0 };
        chan_recv(parsed, (void *)&datagram);
        if (strcmp(datagram->type, "ms") == 0) {
            record_duration(datagram, &buckets, config);
        } else if (strcmp(datagram->type, "c") == 0) {
            record_counter(datagram, &buckets, config);
        } else if (strcmp(datagram->type, "g") == 0) {
            record_gauge(datagram, &buckets, config);
        }
    }
}

void print_out_bucket(bucket* bucket) {
    printf("BUCKET: \n");
    printf("%s \n", bucket->metric);
    printf("------------------------------ \n");
}

bucket* find_bucket(int (*predicate)(bucket* bucket)) {
    for (int i = 0; i < buckets.length; i++) {
        if (predicate(buckets.values[i])) {
            return buckets.values[i];
        }
    }
    return NULL;
}

bucket_collection* add_bucket(bucket* bucket) {
    buckets.values[buckets.length++] = bucket;
    return &buckets;
}

bucket* create_bucket_from_datagram(statsd_datagram* datagram) {
    bucket* b = (struct bucket*) malloc(sizeof(struct bucket));
    b->instance = datagram->instance;
    b->type = datagram->type;
    b->metric = datagram->metric;
    b->sampling = datagram->sampling;
    b->tags = create_tags_from_datagram_tags(datagram->tags);
    b->value = datagram->value;
    return b;
}

tag_collection* create_tags_from_datagram_tags(char* tags_string) {
    tag_collection* tags = (struct tag_collection*) malloc(sizeof(struct tag_collection));
    return tags;
}

tag* find_tag(bucket* bucket, int (*predicate)(tag*)) {
    for (int i = 0; i < bucket->tags->length; i++) {
        if (predicate(bucket->tags->values[i])) {
            return bucket->tags->values[i];
        }
    }
    return NULL;
}