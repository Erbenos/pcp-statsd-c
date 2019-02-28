#include "../config-reader/config-reader.h"

#ifndef UTILS_
#define UTILS_

void die(int line_number, const char* format, ...);

void warn(int line_number, const char* format, ...);

void sanitize_string(char *src);

void init_loggers(agent_config* config);

void verbose_log(const char* format, ...);

void debug_log(const char* format, ...);

void trace_log(const char* format, ...);

#endif