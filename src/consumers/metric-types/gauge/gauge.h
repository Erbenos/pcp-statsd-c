#include "../../../config-reader/config-reader.h"
#include "../../../statsd-parsers/statsd-parsers.h"
#include "../../consumers.h"

void record_gauge(statsd_datagram* datagram, bucket_collection* buckets, agent_config* config);