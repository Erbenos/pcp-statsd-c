#include "../statsd-parsers.h"

#ifndef HISTOGRAM_
#define HISTOGRAM_

void consume_datagram(struct statsd_datagram* datagram);

#endif