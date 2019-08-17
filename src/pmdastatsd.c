#ifndef _TEST_TARGET

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <chan/chan.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <pcp/pmapi.h>

#include "pmdastatsd.h"
#include "config-reader.h"
#include "network-listener.h"
#include "aggregators.h"
#include "aggregator-metrics.h"
#include "aggregator-stats.h"
#include "pmda-callbacks.h"
#include "dict-callbacks.h"
#include "utils.h"
#include "../domain.h"

#define VERSION 0.9

void output_request_handler(int num) {
    if (num == SIGUSR1) {
        DEBUG_LOG("Handling SIGUSR1.");
        aggregator_request_output();
    }
    if (num == SIGINT) {
        DEBUG_LOG("Handling SIGINT.");
        set_exit_flag();
    }
}

#define SET_INST_NAME(instance, name, index) \
    instance[index].i_inst = index; \
    len = pmsprintf(buff, 20, "%s", name) + 1; \
    instance[index].i_name = (char*) malloc(sizeof(char) * len); \
    ALLOC_CHECK("Unable to allocate memory for static PMDA instance descriptor."); \
    memcpy(instance[index].i_name, buff, len);

/**
 * Registers hardcoded instances before PMDA initializes itself fully
 * @arg pmda - PMDA extension structure (contains agent-specific private data)
 */
static void
create_statsd_hardcoded_instances(struct pmda_data_extension* data) {
    size_t len = 0;
    char buff[20];
    size_t hardcoded_count = 3;

    data->pcp_instance_domains = (pmdaIndom*) malloc(hardcoded_count * sizeof(pmdaIndom));
    ALLOC_CHECK("Unable to allocate memory for static PMDA instance domains.");
    
    pmdaInstid* stats_metric_counters_indom = (pmdaInstid*) malloc(sizeof(pmdaInstid) * 4);
    ALLOC_CHECK("Unable to allocate memory for static PMDA instance domain descriptor.");
    data->pcp_instance_domains[0].it_indom = STATS_METRIC_COUNTERS_INDOM;
    data->pcp_instance_domains[0].it_numinst = 4;
    data->pcp_instance_domains[0].it_set = stats_metric_counters_indom;
    SET_INST_NAME(stats_metric_counters_indom, "counter", 0);
    SET_INST_NAME(stats_metric_counters_indom, "gauge", 1);
    SET_INST_NAME(stats_metric_counters_indom, "duration", 2);
    SET_INST_NAME(stats_metric_counters_indom, "total", 3);

    pmdaInstid* statsd_metric_default_duration_indom = (pmdaInstid*) malloc(sizeof(pmdaInstid) * 9);
    ALLOC_CHECK("Unable to allocate memory for static PMDA instance domain descriptors.");
    data->pcp_instance_domains[1].it_indom = STATSD_METRIC_DEFAULT_DURATION_INDOM;
    data->pcp_instance_domains[1].it_numinst = 9;
    data->pcp_instance_domains[1].it_set = statsd_metric_default_duration_indom;
    SET_INST_NAME(statsd_metric_default_duration_indom, "/min", 0);
    SET_INST_NAME(statsd_metric_default_duration_indom, "/max", 1);
    SET_INST_NAME(statsd_metric_default_duration_indom, "/median", 2);
    SET_INST_NAME(statsd_metric_default_duration_indom, "/average", 3);
    SET_INST_NAME(statsd_metric_default_duration_indom, "/percentile90", 4);
    SET_INST_NAME(statsd_metric_default_duration_indom, "/percentile95", 5);
    SET_INST_NAME(statsd_metric_default_duration_indom, "/percentile99", 6);
    SET_INST_NAME(statsd_metric_default_duration_indom, "/count", 7);
    SET_INST_NAME(statsd_metric_default_duration_indom, "/std_deviation", 8);

    pmdaInstid* statsd_metric_default_indom = (pmdaInstid*) malloc(sizeof(pmdaInstid));
    ALLOC_CHECK("Unable to allocate memory for default dynamic metric instance domain descriptior");
    data->pcp_instance_domains[2].it_indom = STATSD_METRIC_DEFAULT_INDOM;
    data->pcp_instance_domains[2].it_numinst = 1;
    data->pcp_instance_domains[2].it_set = statsd_metric_default_indom;
    SET_INST_NAME(statsd_metric_default_indom, "/", 0);

    data->pcp_instance_domain_count = hardcoded_count;
    data->pcp_hardcoded_instance_domain_count = hardcoded_count;
}

/**
 * Registers hardcoded metrics before PMDA initializes itself fully
 * @arg pmda - PMDA extension structure (contains agent-specific private data)
 */
static void
create_statsd_hardcoded_metrics(struct pmda_data_extension* data) {
    size_t i;
    size_t hardcoded_count = 15;
    data->pcp_metrics = (pmdaMetric*) malloc(hardcoded_count * sizeof(pmdaMetric));
    ALLOC_CHECK("Unable to allocate space for static PMDA metrics.");
    // helper containing only reference to priv data same for all hardcoded metrics
    static struct pmda_metric_helper helper;
    size_t agent_stat_count = 7;
    helper.data = data;
    for (i = 0; i < hardcoded_count; i++) {
        data->pcp_metrics[i].m_user = &helper;
        data->pcp_metrics[i].m_desc.pmid = pmID_build(STATSD, 0, i);
        data->pcp_metrics[i].m_desc.sem = PM_SEM_INSTANT;
        if (i < agent_stat_count) {
            data->pcp_metrics[i].m_desc.type = PM_TYPE_U64;
            if (i == 4) {
                data->pcp_metrics[i].m_desc.indom = STATS_METRIC_COUNTERS_INDOM;
            } else {
                data->pcp_metrics[i].m_desc.indom = PM_INDOM_NULL;
            }            
        } else {
            if (i == 7) {
                data->pcp_metrics[i].m_desc.type = PM_TYPE_U64;
            } else if (i < 11 || i == 12) {
                data->pcp_metrics[i].m_desc.type = PM_TYPE_U32;
            } else {
                data->pcp_metrics[i].m_desc.type = PM_TYPE_STRING;
            }
            data->pcp_metrics[i].m_desc.indom = PM_INDOM_NULL;
        }
        if (i == 5 || i == 6) {
            // time_spent_parsing / time_spent_aggregating
            data->pcp_metrics[i].m_desc.units.dimSpace = 0;
            data->pcp_metrics[i].m_desc.units.dimTime = 0;
            data->pcp_metrics[i].m_desc.units.dimCount = 0;
            data->pcp_metrics[i].m_desc.units.pad = 0;
            data->pcp_metrics[i].m_desc.units.scaleSpace = 0;
            data->pcp_metrics[i].m_desc.units.scaleTime = PM_TIME_NSEC;
            data->pcp_metrics[i].m_desc.units.scaleCount = 1;
        } else {
            // rest
            memset(&data->pcp_metrics[i].m_desc.units, 0, sizeof(pmUnits));
        }
    }
    data->pcp_metric_count = hardcoded_count;
    data->pcp_hardcoded_metric_count = hardcoded_count;
}

static void
free_shared_data(struct agent_config* config, struct pmda_data_extension* data) {
    // frees config
    free(config->debug_output_filename);
    // remove metrics dictionary and related
    dictRelease(data->metrics_storage->metrics);
    // privdata will be left behind, need to remove manually
    free(data->metrics_storage->metrics_privdata);
    pthread_mutex_destroy(&data->metrics_storage->mutex);
    free(data->metrics_storage);
    // remove stats dictionary and related
    free(data->stats_storage->stats->metrics_recorded);
    free(data->stats_storage->stats);
    pthread_mutex_destroy(&data->stats_storage->mutex);
    free(data->stats_storage);
    // free instance map
    dictRelease(data->instance_map);
    // clear PCP metric table
    size_t i;
    for (i = 0; i < data->pcp_metric_count; i++) {
        size_t j = data->pcp_hardcoded_metric_count;
        if (i == j) {
            free(data->pcp_metrics[i].m_user);
        }
        if (!(i < j)) {
            free(data->pcp_metrics[i].m_user);
        }
    }
    free(data->pcp_metrics);
    // clear PCP instance domains
    for (i = 0; i < data->pcp_instance_domain_count; i++) {
        int j = 0;
        VERBOSE_LOG("FREEING %lu -th instance domain", i);
        for (j; j < data->pcp_instance_domains[i].it_numinst; j++) {
            VERBOSE_LOG("FREEING %d -th instance domain name", j);
            free(data->pcp_instance_domains[i].it_set[j].i_name);
        }
        free(data->pcp_instance_domains[i].it_set);
    }
    free(data->pcp_instance_domains);
    // Release PMNS tree
    pmdaTreeRelease(data->pcp_pmns);
}

/**
 * Initializes structure which is used as private data container accross all PCP related callbacks
 * @arg args - All args passed to 'PCP exchange' thread
 */
static void
init_data_ext(
    struct pmda_data_extension* data,
    struct agent_config* config,
    struct pmda_metrics_container* metrics_storage,
    struct pmda_stats_container* stats_storage
) {
    static dictType instance_map_callbacks = {
        .hashFunction	= str_hash_callback,
        .keyCompare		= str_compare_callback,
        .keyDup		    = str_duplicate_callback,
        .keyDestructor	= str_hash_free_callback,
    };
    data->config = config;
    create_statsd_hardcoded_metrics(data);
    create_statsd_hardcoded_instances(data);
    data->metrics_storage = metrics_storage;
    data->stats_storage = stats_storage;
    data->instance_map = dictCreate(&instance_map_callbacks, NULL);
    data->generation = -1; // trigger first mapping of metrics for PMNS 
    data->notify = 0;
}

static void
main_PDU_loop(pmdaInterface* dispatch) {
    VERBOSE_LOG("Entering main loop");
    for(;;) {
        VERBOSE_LOG("INSIDE LOOP");
        if (check_exit_flag() != 0) break;
        if (__pmdaMainPDU(dispatch) < 0) break;
    }
    VERBOSE_LOG("Exiting main loop");
}

int
main(int argc, char** argv)
{
    struct sigaction new_action, old_action;

    /* Set up the structure to specify the new action. */
    new_action.sa_handler = output_request_handler;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = SA_INTERRUPT;

    sigaction (SIGUSR1, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN) {
        sigaction (SIGUSR1, &new_action, NULL);
    }
    sigaction (SIGINT, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN) {
        sigaction (SIGINT, &new_action, NULL);
    }

    struct agent_config config = { 0 };
    struct pmda_data_extension data = { 0 };
    pthread_t network_listener;
    pthread_t parser;
    pthread_t aggregator;
    pmdaInterface dispatch = { 0 };

    int sep = pmPathSeparator();
    char config_file_path[MAXPATHLEN];
    char help_file_path[MAXPATHLEN];
    pmsprintf(
        config_file_path,
        MAXPATHLEN,
        "%s" "%c" "statsd" "%c" "pmdastatsd.ini",
        pmGetConfig("PCP_PMDAS_DIR"),
        sep, sep);
    pmsprintf(
        help_file_path,
        MAXPATHLEN,
        "%s%c" "statsd" "%c" "help",
        pmGetConfig("PCP_PMDAS_DIR"),
        sep, sep);

    pmSetProgname(argv[0]);
    pmdaDaemon(&dispatch, PMDA_INTERFACE_7, pmGetProgname(), STATSD, "statsd.log", help_file_path);

    read_agent_config(&config, &dispatch, config_file_path, argc, argv);
    init_loggers(&config);
    pmdaOpenLog(&dispatch);
    if (config.debug) {
        print_agent_config(&config);
    }
    if (config.show_version) {
        pmNotifyErr(LOG_INFO, "Version: %f", VERSION);
    }

    struct pmda_metrics_container* metrics = init_pmda_metrics(&config);
    struct pmda_stats_container* stats = init_pmda_stats(&config);
    init_data_ext(&data, &config, metrics, stats);

    chan_t* network_listener_to_parser = chan_init(config.max_unprocessed_packets);
    if (network_listener_to_parser == NULL) {
        DIE("Unable to create channel network listener -> parser.");        
    }
    ALLOC_CHECK("Unable to create channel network listener -> parser.");
    chan_t* parser_to_aggregator = chan_init(config.max_unprocessed_packets);
    if (parser_to_aggregator == NULL) {
        DIE("Unable to create channel parser -> aggregator.");        
    }
    ALLOC_CHECK("Unable to create channel parser -> aggregator.");
    struct network_listener_args* listener_args = create_listener_args(&config, network_listener_to_parser);
    struct parser_args* parser_args = create_parser_args(&config, network_listener_to_parser, parser_to_aggregator);
    struct aggregator_args* aggregator_args = create_aggregator_args(&config, parser_to_aggregator, metrics, stats);

    int pthread_errno = 0; 
    pthread_errno = pthread_create(&network_listener, NULL, network_listener_exec, listener_args);
    PTHREAD_CHECK(pthread_errno);
    pthread_errno = pthread_create(&parser, NULL, parser_exec, parser_args);
    PTHREAD_CHECK(pthread_errno);
    pthread_errno = pthread_create(&aggregator, NULL, aggregator_exec, aggregator_args);
    PTHREAD_CHECK(pthread_errno);

    pmSetProcessIdentity(config.username);

    if (dispatch.status != 0) {
        pthread_exit(NULL);
    }
    dispatch.version.seven.fetch = statsd_fetch;
	dispatch.version.seven.desc = statsd_desc;
	dispatch.version.seven.text = statsd_text;
	dispatch.version.seven.instance = statsd_instance;
	dispatch.version.seven.pmid = statsd_pmid;
	dispatch.version.seven.name = statsd_name;
	dispatch.version.seven.children = statsd_children;
	dispatch.version.seven.label = statsd_label;
    // Callbacks
	pmdaSetFetchCallBack(&dispatch, statsd_fetch_callback);
    pmdaSetLabelCallBack(&dispatch, statsd_label_callback);
    pmdaSetData(&dispatch, (void*) &data);
    pmdaSetFlags(&dispatch, PMDA_EXT_FLAG_HASHED);
    pmdaInit(
        &dispatch,
        data.pcp_instance_domains,
        data.pcp_instance_domain_count,
        data.pcp_metrics,
        data.pcp_metric_count
    );
    pmdaConnect(&dispatch);
    main_PDU_loop(&dispatch);
    VERBOSE_LOG("Main PDU loop interrupted.");
    
    if (pthread_join(network_listener, NULL) != 0) {
        DIE("Error joining network network listener thread.");
    } else {
        VERBOSE_LOG("Network listener thread joined.");
        set_parser_exit();
    }
    if (pthread_join(parser, NULL) != 0) {
        DIE("Error joining datagram parser thread.");
    } else {
        VERBOSE_LOG("Parser thread joined.");
        set_aggregator_exit();
    }
    if (pthread_join(aggregator, NULL) != 0) {
        DIE("Error joining datagram aggregator thread.");
    } else {
        VERBOSE_LOG("Aggregator thread joined.");
    }

    free_shared_data(&config, &data);
    free(listener_args);
    free(parser_args);
    free(aggregator_args);

    chan_close(network_listener_to_parser);
    chan_close(parser_to_aggregator);
    chan_dispose(network_listener_to_parser);
    chan_dispose(parser_to_aggregator);
    return EXIT_SUCCESS;
}

#endif
