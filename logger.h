#ifndef LOGGER_H
#define LOGGER_H

#include <syslog.h>

#define LOG_FACILITY_LEN 10
#define LOG_SERVERITY_LEN 10
#define LOG(serverity, ...) logger_log(__func__, __LINE__, (serverity), __VA_ARGS__)
#define LOG_DUMP(serverity, buf, buf_size) logger_dump(__func__, __LINE__, (serverity), (buf), (buf_size))

int logger_filter(const char *serverity);
int logger_open(const char *ident, int option, const char *facility);
int logger_log(const char *func, int line, int serverity, const char *format, ...);
int logger_dump(const char *func, int line, int serverity, const unsigned char *buf, size_t buf_size);
void logger_close(void);

#endif
