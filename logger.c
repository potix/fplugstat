#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <stdarg.h>
#include <errno.h>

#define MAX_FORMAT_LEN 2048

struct key_value_map {
	const char *key;
	int value;
};
typedef struct key_value_map key_value_map_t;

key_value_map_t facilities[] = {
	{ "auth",     LOG_AUTH     },
	{ "authpriv", LOG_AUTHPRIV },
	{ "cron",     LOG_CRON     },
	{ "daemon",   LOG_DAEMON   },
	{ "ftp",      LOG_FTP      },
	{ "kern",     LOG_KERN     },
	{ "local0",   LOG_LOCAL0   },
	{ "local1",   LOG_LOCAL1   },
	{ "local2",   LOG_LOCAL2   },
	{ "local3",   LOG_LOCAL3   },
	{ "local4",   LOG_LOCAL4   },
	{ "local5",   LOG_LOCAL5   },
	{ "local6",   LOG_LOCAL6   },
	{ "local7",   LOG_LOCAL7   },
	{ "lpr",      LOG_LPR      },
	{ "mail",     LOG_MAIL     },
	{ "news",     LOG_NEWS     },
	{ "syslog",   LOG_SYSLOG   },
	{ "user",     LOG_USER     },
	{ "uucp",     LOG_UUCP     }
};

key_value_map_t serverities[] = {
	{ "emerg",   LOG_EMERG   },
	{ "alert",   LOG_ALERT   },
	{ "crit",    LOG_CRIT    },
	{ "err",     LOG_ERR     },
	{ "warning", LOG_WARNING },
	{ "notice",  LOG_NOTICE  },
	{ "info",    LOG_INFO    },
	{ "debug",   LOG_DEBUG   }
};

int filter_serverity = LOG_WARNING;

static int get_value(key_value_map_t *entries, int member_count, const char *search_key);

static int
get_value(key_value_map_t *entries, int member_count, const char *search_key)
{
	int i;

	for (i = 0; i < member_count; i++) {
		if(strcasecmp(entries[i].key, search_key) == 0) {
			return entries[i].value;
		}
	}
 
	return -1;
}

int
logger_filter(const char *serverity)
{
	int serverity_int;

	serverity_int = get_value(serverities, sizeof(serverities)/sizeof(key_value_map_t), serverity);
	if (serverity_int < 0) {
		return 1;
	}
	filter_serverity = serverity_int;

	return 0;
}

int
logger_open(const char *ident, int option, const char *facility)
{
	int facility_int;

	facility_int = get_value(facilities, sizeof(facilities)/sizeof(key_value_map_t), facility);
	if (facility_int < 0) {
		return 1;
	}
	openlog(ident, option, facility_int);

	return 0;
}

int
logger_log(const char *func, int line, int serverity, const char *format, ...)
{
	char new_format[MAX_FORMAT_LEN];

	if (serverity > filter_serverity) {
		return 0;
	} 
	snprintf(new_format, sizeof(new_format), "%s:%d[%d] %s", func, line, errno, format);
	va_list list;
	va_start(list, format);
	vsyslog(serverity, new_format, list);
	va_end(list);

	return 0;
}

void
logger_close(void)
{
	closelog();
}
