#include <stdio.h>
#include <string.h>
#include <chan/chan.h>
#include <hdr/hdr_histogram.h>
#include <float.h>

#include "../../statsd-parsers/statsd-parsers.h"
#include "../../utils/utils.h"
#include "consumers.h"

/**
 * Flag to capture USR1 signal event, which is supposed mark request to output currently recorded data in debug file 
 */
static int g_output_requested = 0;
pthread_mutex_t g_output_requested_lock;

static void metricFreeCallBack(void *privdata, void *val)
{
    (void)privdata;
    free_metric((metric*)val);
}

static void * metricKeyDupCallBack(void *privdata, const void *key)
{
    (void)privdata;
    char* duplicate = malloc(strlen(key));
    strcpy(duplicate, key);
    return duplicate;
}

static int metricCompareCallBack(void* privdata, const void* key1, const void* key2)
{
    (void)privdata;
    return strcmp((char*)key1, (char*)key2) == 0;
}

static uint64_t metricHashCallBack(const void *key)
{
    return dictGenCaseHashFunction((unsigned char *)key, strlen((char *)key));
}

/**
 * Callbacks for counter hashtable
 */
dictType metricDictCallBacks = {
    .hashFunction	= metricHashCallBack,
    .keyCompare		= metricCompareCallBack,
    .keyDup		    = metricKeyDupCallBack,
    .keyDestructor	= metricFreeCallBack,
    .valDestructor	= metricFreeCallBack,
};

/**
 * Initializes metrics struct to empty values
 * @arg config - Config (should there be need to pass detailed info into metrics)
 */
metrics* init_metrics(agent_config* config) {
    (void)config;
    metrics* m = dictCreate(&metricDictCallBacks, "metrics");
    return m;
}

/**
 * Thread startpoint - passes down given datagram to consumer to record value it contains
 * @arg args - (consumer_args), see ~/statsd-parsers/statsd-parsers.h
 */
void* consume_datagram(void* args) {
    chan_t* parsed = ((consumer_args*)args)->parsed_datagrams;
    agent_config* config = ((consumer_args*)args)->config;
    metrics* m = ((consumer_args*)args)->metrics_wrapper;
    statsd_datagram *datagram = (statsd_datagram*) malloc(sizeof(statsd_datagram));
    while(1) {
        switch(chan_select(&parsed, 1, (void *)&datagram, NULL, 0, NULL)) {
            case 0:
                process_datagram(m, datagram);
                break;
            default:
                {
                    pthread_mutex_lock(&g_output_requested_lock);
                    if (g_output_requested) {
                        verbose_log("Output of recorded values request caught.");
                        print_metrics(m, config);
                        verbose_log("Recorded values output.");
                        g_output_requested = 0;
                    }
                    pthread_mutex_unlock(&g_output_requested_lock);
                }
        }
    }
    free_datagram(datagram);
}

/**
 * Sets flag notifying that output was requested
 */
void consumer_request_output() {
    pthread_mutex_lock(&g_output_requested_lock);
    g_output_requested = 1;
    pthread_mutex_unlock(&g_output_requested_lock);
}

static char* create_metric_dict_key(statsd_datagram* datagram) {
    int key_size = snprintf(
        NULL,
        0,
        "%s&%s&%s&%s",
        datagram->metric,
        datagram->tags != NULL ? datagram->tags : "-",
        datagram->instance != NULL ? datagram->instance : "-",
        datagram->type
    );
    if (key_size > 4095) {
        return NULL;
    }
    char* result = malloc(key_size + 1);
    ALLOC_CHECK("Unable to allocate memory for hashtable key");
    snprintf(
        result,
        4096,
        "%s&%s&%s&%s",
        datagram->metric,
        datagram->tags != NULL ? datagram->tags : "-",
        datagram->instance != NULL ? datagram->instance : "-",
        datagram->type
    );
    return result;
}

/**
 * Processes datagram struct into metric 
 * @arg m - Metrics struct acting as metrics wrapper
 * @arg datagram - Datagram to be processed
 */
void process_datagram(metrics* m, statsd_datagram* datagram) {
    metric* item;
    char* key = create_metric_dict_key(datagram);
    if (key == NULL) {
        verbose_log("Throwing away datagram, unable to create hashtable key for metric record.");
        return;
    }
    int metric_exists = find_metric_by_name(m, key, &item);
    if (metric_exists) {
        int res = update_metric(item, datagram);
        if (res == 0) {
            verbose_log("Throwing away datagram, semantically incorrect values.");
        }
    } else {
        int name_available = check_metric_name_available(m, key);
        if (name_available) {
            int correct_semantics = create_metric(datagram, &item);
            if (correct_semantics) {
                add_metric(m, key, item);
            } else {
                free_metric(item);
                verbose_log("Throwing away datagram, semantically incorrect values.");
            }
        }
    }
}

/**
 * Frees metric
 * @arg metric - Metric to be freed
 */
void free_metric(metric* item) {
    if (item->name != NULL) {
        free(item->name);
    }
    if (item->meta != NULL) {
        free_metric_metadata(item->meta);
    }
    if (item->type != NULL) {
        switch (*(item->type)) {
            case COUNTER:
            case GAUGE:
                if (item->value != NULL) {
                    free(item->value);
                }
                break;
            case DURATION:
                if (item->value != NULL) {
                    hdr_close((struct hdr_histogram*)item->value);
                }
                break;
        }
        free(item->type);
    }
    if (item != NULL) {
        free(item);
    }
}


static void print_metric_meta(FILE* f, metric_metadata* meta) {
    if (meta != NULL) {
        if (meta->tags != NULL) {
            fprintf(f, "tags = %s\n", meta->tags);
        }
        if (meta->sampling != NULL) {
            fprintf(f, "sampling = %s\n", meta->sampling);
        }
        if (meta->instance != NULL) {
            fprintf(f, "instance = %s\n", meta->instance);
        }
    }
}

/**
 * Writes information about recorded metrics into file
 * @arg m - Metrics struct (what values to print)
 * @arg config - Config containing information about where to output
 */
void print_metrics(metrics* m, agent_config* config) {
    if (strlen(config->debug_output_filename) == 0) return; 
    FILE* f;
    f = fopen(config->debug_output_filename, "a+");
    if (f == NULL) {
        return;
    }
    dictIterator* iterator = dictGetSafeIterator(m);
    dictEntry* current;
    long int count = 0;
    while ((current = dictNext(iterator)) != NULL) {
        metric* item = (metric*)current->v.val;
        switch (*(item->type)) {
            case COUNTER:
                fprintf(f, "-----------------\n");
                fprintf(f, "name = %s\n", item->name);
                fprintf(f, "value = %llu (counter)\n", *(unsigned long long int*)(item->value));
                print_metric_meta(f, item->meta);
                break;
            case GAUGE:
                fprintf(f, "-----------------\n");
                fprintf(f, "name = %s\n", item->name);
                fprintf(f, "value = %f (gauge)\n", *(double*)(item->value));
                print_metric_meta(f, item->meta);
                break;
            case DURATION:
                fprintf(f, "-----------------\n");
                fprintf(f, "name = %s\n", item->name);
                fprintf(f, "value = (duration)\n");
                print_metric_meta(f, item->meta);
                hdr_percentiles_print(
                    item->value,
                    f,
                    5,
                    1.0,
                    CLASSIC
                );
                fprintf(f, "\n");
        }
        count++;
    }
    dictReleaseIterator(iterator);
    fprintf(f, "-----------------\n");
    fprintf(f, "Total number of records: %lu \n", count);
    fclose(f);
}

/**
 * Finds metric by name
 * @arg name - Metric name to search for
 * @arg out - Placeholder metric
 * @return 1 when any found
 */
int find_metric_by_name(metrics* m, char* name, metric** out) {
    dictEntry* result = dictFind(m, name);
    if (result == NULL) {
        return 0;
    }
    if (out != NULL) {
        metric* item = (metric*)result->v.val;
        *out = item;
    }
    return 1;
}

static int create_duration_metric(statsd_datagram* datagram, metric** out) {
    if (datagram->value[0] == '-' || datagram->value[0] == '+') {
        return 0;
    }
    struct hdr_histogram* histogram;
    hdr_init(1, INT64_C(3600000000), 3, &histogram);
    ALLOC_CHECK("Unable to allocate memory for histogram");
    long long unsigned int value = strtoull(datagram->value, NULL, 10);
    if (errno == ERANGE) {
        return 0;
    }
    hdr_record_value(histogram, value);
    (*out)->name = malloc(strlen(datagram->metric));
    strcpy((*out)->name, datagram->metric);
    (*out)->type = (METRIC_TYPE*) malloc(sizeof(METRIC_TYPE));
    *((*out)->type) = 4;
    (*out)->value = histogram;
    (*out)->meta = create_metric_meta(datagram);
    return 1;
}

static int create_gauge_metric(statsd_datagram* datagram, metric** out) {
    if (datagram->metric == NULL) {
        return 0;
    }
    double value = strtod(datagram->value, NULL);
    if (errno == ERANGE) {
        return 0;
    }
    (*out)->name = malloc(strlen(datagram->metric));
    strcpy((*out)->name, datagram->metric);
    (*out)->type = (METRIC_TYPE*) malloc(sizeof(METRIC_TYPE));
    *((*out)->type) = 2;
    (*out)->value = (double*) malloc(sizeof(double));
    *((double*)(*out)->value) = value;
    (*out)->meta = create_metric_meta(datagram);
    return 1;
}

static int create_counter_metric(statsd_datagram* datagram, metric** out) {
    if (datagram->value[0] == '-' || datagram->value[0] == '+') {
        return 0;
    }
    long long unsigned int value = strtoull(datagram->value, NULL, 10);
    if (errno == ERANGE) {
        return 0;
    }
    (*out)->name = malloc(strlen(datagram->metric));
    strcpy((*out)->name, datagram->metric);
    (*out)->type = (METRIC_TYPE*) malloc(sizeof(METRIC_TYPE));
    *((*out)->type) = 1;
    (*out)->value = (unsigned long long int*) malloc(sizeof(unsigned long long int));
    *((long long unsigned int*)(*out)->value) = value;
    (*out)->meta = create_metric_meta(datagram);
    return 1;
}

/**
 * Creates metric
 * @arg datagram - Datagram with data that should populate new metric
 * @arg out - Placeholder metric
 * @return 1 on success
 */
int create_metric(statsd_datagram* datagram, metric** out) {
    metric* item = (struct metric*) malloc(sizeof(metric));
    ALLOC_CHECK("Unable to allocate memory for metric.");
    *out = item;
    if (strcmp(datagram->type, "ms") == 0) {
        return create_duration_metric(datagram, out);
    } else if (strcmp(datagram->type, "c") == 0) {
        return create_counter_metric(datagram, out);
    } else if (strcmp(datagram->type, "g") == 0) {
        return create_gauge_metric(datagram, out);
    }
    return 0;
}

/**
 * Adds gauge record
 * @arg gauge - Gauge metric to me added
 * @return all gauges
 */
void add_metric(metrics* m, char* key, metric* item) {
    dictAdd(m, key, item);
}

static int update_counter_metric(metric* item, statsd_datagram* datagram) {
    if (datagram->value[0] == '-' || datagram->value[0] == '+') {
        return 0;
    }
    long long unsigned int value = strtoull(datagram->value, NULL, 10);
    if (errno == ERANGE) {
        return 0;
    }
    *(long long unsigned int*)(item->value) += value;
    return 1;
}

static int update_gauge_metric(metric* item, statsd_datagram* datagram) {
    int substract = 0;
    int add = 0;
    if (datagram->value[0] == '+') {
        add = 1;
    }
    if (datagram->value[0] == '-') {
        substract = 1; 
    }
    double value;
    if (substract || add) {
        char* substr = &(datagram->value[1]);
        value = strtod(substr, NULL);
    } else {
        value = strtod(datagram->value, NULL);
    }
    if (errno == ERANGE) {
        return 0;
    }
    double old_value = *(double*)(item->value);
    if (add || substract) {
        if (add) {
            if (old_value + value >= DBL_MAX) return 0;
            *(double*)(item->value) += value;
        }
        if (substract) {
            if (old_value - value <= DBL_MIN) return 0;
            *(double*)(item->value) -= value;
        }
    } else {
        *(double*)(item->value) = value;
    }
    return 1;
}

static int update_duration_metric(metric* item, statsd_datagram* datagram) {
    if (datagram->value[0] == '-' || datagram->value[0] == '+') {
        return 0;
    }
    long long unsigned int value = strtoull(datagram->value, NULL, 10);
    if (errno == ERANGE) {
        return 0;
    }
    hdr_record_value((struct hdr_histogram*)item->value, value);
    return 1;
}

/**
 * Updates counter record
 * @arg counter - Metric to be updated
 * @arg datagram - Data with which to update
 * @return 1 on success
 */
int update_metric(metric* item, statsd_datagram* datagram) {
    switch (*(item->type)) {
        case COUNTER:
            return update_counter_metric(item, datagram);
        case GAUGE:
            return update_gauge_metric(item, datagram);
        case DURATION:
            return update_duration_metric(item, datagram);
    }
    return 0;
}

/**
 * Checks if given metric name is available (it isn't recorded yet)
 * @arg m - Metrics struct (storage in which to check for name availability)
 * @arg name - Name to be checked
 * @return 1 on success else 0
 */
int check_metric_name_available(metrics* m, char* name) {
    if (!find_metric_by_name(m, name, NULL)) {
        return 1;
    }
    return 0;
}

/**
 * Creates metric metadata
 * @arg datagram - Datagram from which to build metadata
 * @return metric metadata
 */
metric_metadata* create_metric_meta(statsd_datagram* datagram) {
    if (datagram->sampling == NULL && datagram->tags == NULL && datagram->instance == NULL) {
        return NULL;
    }
    metric_metadata* meta = (metric_metadata*) malloc(sizeof(metric_metadata));
    *meta = (metric_metadata) { 0 };
    if (datagram->sampling != NULL) {
        meta->sampling = (char*) malloc(strlen(datagram->sampling) + 1);
        memcpy(meta->sampling, datagram->sampling, strlen(datagram->sampling) + 1);
    }
    if (datagram->tags != NULL) {
        meta->tags = (char*) malloc(strlen(datagram->tags) + 1);
        memcpy(meta->tags, datagram->tags, strlen(datagram->tags) + 1);
    }
    if (datagram->instance != NULL) {
        meta->instance = (char*) malloc(strlen(datagram->instance) + 1);
        memcpy(meta->instance, datagram->instance, strlen(datagram->instance) + 1);
    }
    return meta;
}

/**
 * Frees metric metadata
 * @arg metadata - Metadata to be freed
 */ 
void free_metric_metadata(metric_metadata* meta) {
    if (meta->sampling != NULL) {
        free(meta->sampling);
    }
    if (meta->tags != NULL) {
        free(meta->tags);
    }
    if (meta->instance != NULL) {
        free(meta->instance);
    }
    if (meta != NULL) {
        free(meta);
    }
}

