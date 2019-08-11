#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>

#include "config-reader.h"
#include "parsers.h"
#include "utils.h"

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

#define DURATION_METRIC "ms"
#define COUNTER_METRIC "c"
#define GAUGE_METRIC "g"

/**
 * Used to prevent racing between pmErrLog function calls in different threads 
 */
static pthread_mutex_t g_log_mutex;

/**
 * Flag used to determine if VERBOSE output is allowed to be printed
 */
static int g_verbose_flag = 0;

/**
 * Flag used to determine if DEBUG output is allowed to be printed
 */
static int g_debug_flag = 0;

/**
 * Sanitizes string
 * Swaps '/', '-', ' ' characters with '_'. Should the message contain any other characters then a-z, A-Z, 0-9 and specified above, fails. 
 * First character needs to be in a-zA-Z
 * @arg src - String to be sanitized
 * @return 1 on success
 */
int
sanitize_string(char *src, size_t num) {
    size_t segment_length = strlen(src);
    if (segment_length == 0) {
        return 0;
    }
    if (segment_length > num) {
        segment_length = num;
    }
    size_t i;
    for (i = 0; i < segment_length; i++) {
        char current_char = src[i];
        if (i == 0) {
            if (((int) current_char >= (int) 'a' && (int) current_char <= (int) 'z') ||
                ((int) current_char >= (int) 'A' && (int) current_char <= (int) 'Z')) {
                continue;
            } else {
                return 0;
            }
        }
        if (((int) current_char >= (int) 'a' && (int) current_char <= (int) 'z') ||
            ((int) current_char >= (int) 'A' && (int) current_char <= (int) 'Z') ||
            ((int) current_char >= (int) '0' && (int) current_char <= (int) '9') ||
            (int) current_char == (int) '.' ||
            (int) current_char == (int) '_') {
            continue;
        } else if ((int) current_char == (int) '/' || 
                 (int) current_char == (int) '-' ||
                 (int) current_char == (int) ' ') {
            src[i] = '_';
        } else {
            return 0;
        }
    }
    return 1;
}

/**
 * Validates metric val string
 * Checks if there are any non numerical characters (0-9), excluding '+' and '-' on first position and is not empty.
 * @arg src - String to be validated
 * @return 1 on success
 */
int
sanitize_metric_val_string(char* src) {
    size_t segment_length = strlen(src);
    if (segment_length == 0) {
        return 0;
    }
    size_t i;
    for (i = 0; i < segment_length; i++) {
        char current_char = src[i];
        if (i == 0) {
            if (((int) current_char >= (int) '0' && (int) current_char <= (int) '9') ||
                (current_char == '+') ||
                (current_char == '-')) {
                continue;
            } else {
                return 0;
            }
        } else {
            if (((int) current_char >= (int) '0' && (int) current_char <= (int) '9') || current_char == '.') {
                continue;
            } else {
                return 0;
            }
        }
    }
    return 1;
}

/**
 * Validates string
 * Checks if string is convertible to double and is not empty.
 * @arg src - String to be validated
 * @return 1 on success
 */
int
sanitize_sampling_val_string(char* src) {
    size_t segment_length = strlen(src);
    if (segment_length == 0) {
        return 0;
    }
    char* end;
    strtod(src, &end);
    if (src == end || *end != '\0') {
        return 0;
    }
    return 1;
}

/**
 * Validates type string
 * Checks if string is matching one of metric identifiers ("ms" = duration, "g" = gauge, "c" = counter)
 * @arg src - String to be validated
 * @arg out - What metric string contained
 * @return 1 on success
 */
int
sanitize_type_val_string(char* src, enum METRIC_TYPE* out) {
    if (strcmp(src, GAUGE_METRIC) == 0) {
        *out = METRIC_TYPE_GAUGE;
        return 1;
    } else if (strcmp(src, COUNTER_METRIC) == 0) {
        *out = METRIC_TYPE_COUNTER;
        return 1;
    } else if (strcmp(src, DURATION_METRIC) == 0) {
        *out = METRIC_TYPE_DURATION;
        return 1;
    } else {
        return 0;
    }
}

/**
 * Check *verbose* flag
 * @return verbose flag
 */
int 
is_verbose() {
    return g_verbose_flag;
}

/**
 * Check *debug* flag
 * @return debug flag
 */
int 
is_debug() {
    return g_debug_flag;
}

void
log_mutex_lock() {
    pthread_mutex_lock(&g_log_mutex);
}

void
log_mutex_unlock() {
    pthread_mutex_unlock(&g_log_mutex);
}

/**
 * Initializes debugging/verbose/tracing flags based on given config
 * @arg config - Config to check against
 */
void
init_loggers(struct agent_config* config) {
    g_verbose_flag = config->verbose;
    g_debug_flag = config->debug;
}
