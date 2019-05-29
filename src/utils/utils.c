#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "../config-reader/config-reader.h"

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

static int verbose_flag = 0;
static int trace_flag = 0;
static int debug_flag = 0;

void die(int line_number, const char* format, ...)
{
    va_list vargs;
    va_start(vargs, format);
    fprintf(stderr, "%d: ", line_number);
    vfprintf(stderr, format, vargs);
    fprintf(stderr, "\n");
    va_end(vargs);
    exit(1);
}

void warn(int line_number, const char* format, ...) {
    va_list vargs;
    va_start(vargs, format);
    fprintf(stderr, YEL "WARNING on line %d: " RESET, line_number);
    vfprintf(stderr, format, vargs);
    fprintf(stderr, "\n");
    va_end(vargs);
}

int sanitize_string(char *src) {
    int segment_length = strlen(src);
    if (segment_length == 0) {
        return 0;
    }
    int i;
    for (i = 0; i < segment_length; i++) {
        char current_char = src[i];
        if (((int) current_char >= (int) 'a' && (int) current_char <= (int) 'z') ||
            ((int) current_char >= (int) 'A' && (int) current_char <= (int) 'A') ||
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

int sanitize_metric_val_string(char* src) {
    int segment_length = strlen(src);
    if (segment_length == 0) {
        return 0;
    }
    int i;
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
            if ((int) current_char >= (int) '0' && (int) current_char <= (int) '9') {
                continue;
            } else {
                return 0;
            }
        }
    }
    return 1;
}

int sanitize_sampling_val_string(char* src) {
    int segment_length = strlen(src);
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

int sanitize_type_val_string(char* src) {
    if (strcmp(src, GAUGE_METRIC) == 0 ||
        strcmp(src, COUNTER_METRIC) == 0 ||
        strcmp(src, DURATION_METRIC) == 0) {
            return 1;
        }
    return 0;
}

void verbose_log(const char* format, ...) {
    if (verbose_flag) {
        va_list vargs;
        va_start(vargs, format);
        fprintf(stdout, YEL "VERBOSE LOG: " RESET);
        vfprintf(stdout, format, vargs);
        fprintf(stdout, "\n");
        va_end(vargs);
    }
}

void debug_log(const char* format, ...) {
    if (debug_flag) {
        va_list vargs;
        va_start(vargs, format);
        fprintf(stdout, MAG "DEBUG LOG: " RESET);
        vfprintf(stdout, format, vargs);
        fprintf(stdout, "\n");
        va_end(vargs);
    }
}

void trace_log(const char* format, ...) {
    if (trace_flag) {
        va_list vargs;
        va_start(vargs, format);
        fprintf(stdout, CYN "TRACE LOG: " RESET);
        vfprintf(stdout, format, vargs);
        fprintf(stdout, "\n");
        va_end(vargs);
    }
}

void init_loggers(agent_config* config) {
    verbose_flag = config->verbose;
    trace_flag = config->trace;
    debug_flag = config->debug;
}