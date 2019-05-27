#include "../statsd-parsers/statsd-parsers.h"

#ifndef CONSUMERS_
#define CONSUMERS_

typedef struct bucket {
    char* metric;
    char* type;
    tag_collection* tags;
    char* instance;
    char* sampling;
    double value;
} bucket;

typedef struct bucket_collection {
    bucket** values;
    long int length;
} bucket_collection;


void* consume_datagram(void* args);

void print_out_bucket(bucket* bucket);

bucket* create_bucket_from_datagram(statsd_datagram* datagram);

tag_collection* create_tags_from_datagram_tags(char* tags);

bucket* find_bucket(int (*predicate)(bucket* bucket));

tag* find_tag(bucket* bucket, int (*predicate)(tag*));

#endif
