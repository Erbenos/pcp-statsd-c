#include <stdio.h>
#include <string.h>
#include <hdr_histogram.h>

#include "../statsd-parsers.h"
#include "../../utils/utils.h"

struct hdr_histogram* counter_histogram;
struct hdr_histogram* gauge_histogram;
struct hdr_histogram* duration_histogram;

int consumed_datagrams = 0;

void check_histograms_existence() {
    if (counter_histogram == NULL) {
        hdr_init(1, INT64_C(3600000000), 3, &counter_histogram);
    }
    if (gauge_histogram == NULL) {
        hdr_init(1, INT64_C(3600000000), 3, &gauge_histogram);
    }
    if (duration_histogram == NULL) {
        hdr_init(1, INT64_C(3600000000), 3, &duration_histogram);
    }
}

int categorize_datagram(struct statsd_datagram* datagram) {
    if (strcmp("counter", datagram->metric) == 0) {
        hdr_record_value(counter_histogram, datagram->value);
    } else if (strcmp("gauge", datagram->metric) == 0) {
        hdr_record_value(counter_histogram, datagram->value);
    } else if (strcmp("duration", datagram->metric) == 0) {
        hdr_record_value(duration_histogram, datagram->value);
    } else {
        return -1;
    }
    return 0;
}

void print_histograms() {
    FILE *histogram_target = fopen("histogram_output.csv", "a+");
    if (counter_histogram == NULL) {
        hdr_percentiles_print(counter_histogram, histogram_target, 5, 1.0, CSV);
    }
    if (gauge_histogram == NULL) {
        hdr_percentiles_print(gauge_histogram, histogram_target, 5, 1.0, CSV);
    }
    if (duration_histogram == NULL) {
        hdr_percentiles_print(duration_histogram, histogram_target, 5, 1.0, CSV);
    }
    fclose(histogram_target);
}

void consume_datagram(struct statsd_datagram* datagram) {
    check_histograms_existence();
    int err = categorize_datagram(datagram);
    if (err == -1) {
        warn(__LINE__, "Unable to categorize datagram, throwing out.");
    }
    consumed_datagrams++;
    if (consumed_datagrams % 20 == 0) {
        warn(__LINE__, "Printed output.");
        print_histograms();
    }
}