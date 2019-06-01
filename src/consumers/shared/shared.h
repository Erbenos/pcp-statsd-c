#ifndef METRIC_TYPES_SHARED_
#define METRIC_TYPES_SHARED_

typedef struct _statsd_datagram statsd_datagram;
typedef struct _tag_collection tag_collection;

typedef struct metric_metadata {
    tag_collection* tags;
    char* instance;
    char* sampling;
} metric_metadata;

metric_metadata* create_record_meta(statsd_datagram* datagram);

#endif
