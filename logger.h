#ifndef LOGGER_H
#define LOGGER_H

#include <syslog.h>

#define LOG_FACILITY_LEN 10
#define LOG_SERVERITY_LEN 10
#define LOG(serverity, ...) logger_log(__func__, __LINE__, (serverity), __VA_ARGS__)

int logger_filter(const char *serverity);
int logger_open(const char *ident, int option, const char *facility);
int logger_log(const char *func, int line, int serverity, const char *format, ...);
void logger_close(void);

#endif
