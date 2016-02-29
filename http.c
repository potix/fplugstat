#define _XOPEN_SOURCE
#define _BSD_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <time.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>

#include "common_macros.h"
#include "common_define.h"
#include "logger.h"
#include "string_util.h"
#include "config.h"
#include "fplug_device.h"
#include "http.h"

#define ENTRY_BUF_MAX 4096
#define URL_PATH_MAX CONFIG_MAX_STR_LEN
#define HTTP_API_URL_PATH "/api/"
#define DATETIME_LEN 16

#ifndef DEFAULT_HTTP_RESOURCE_PATH
#define DEFAULT_HTTP_RESOURCE_PATH FPLUGSTAT_PATH "/www"
#endif

#ifndef DEFAULT_HTTP_ADDRESS
#define DEFAULT_HTTP_ADDRESS "0.0.0.0"
#endif

#ifndef DEFAULT_HTTP_PORT
#define DEFAULT_HTTP_PORT "80"
#endif

#define API_DEVICIES_URL                   "devicies"
#define API_DEVICIE_REALTIME_URL           "device/realtime"
#define API_DEVICIE_HOURLY_POWER_TOTAL_URL "device/hourly/power/total"
#define API_DEVICIE_HOURLY_OTHER_URL       "device/hourly/other"
#define API_DEVICIE_RESET_URL              "device/reset"
#define API_DEVICIE_DATETIME_URL           "device/datetime"

struct http_server {
	struct evhttp *evhttp;
	struct evhttp_bound_socket *bound_socket;
	char address[NI_MAXHOST];
	unsigned short port;
        char resource_path[URL_PATH_MAX];
        fplug_device_t *fplug_device;
};

struct content_type_map {
	const char *extension;
	const char *content_type;
};
typedef struct content_type_map content_type_map_t;

struct api_callback_arg {
	int idx;
	struct evbuffer *response;
};
typedef struct api_callback_arg api_callback_arg_t;

static void default_cb(struct evhttp_request *req, void *arg);
static int api_cb(struct evhttp_request *req, const char *decoded_path, enum evhttp_cmd_type cmd_type,
    http_server_t *http_server, int *status_code, const char **reason);
static void create_active_device_response(const char *device_name, const char *device_address, void *cb_arg);
static void create_stat_store_response(time_t stat_time, double temperature,
   unsigned int humidity, unsigned int illuminance, double rwatt, void *cb_arg);
static void create_hourly_power_total_response(double watt, unsigned char reliability, void *cb_arg);
static void create_hourly_other_response(double temperature, unsigned int humidity, unsigned int illuminance, void *cb_arg);

content_type_map_t content_types[] = {
	{ ".html", "text/html"       },
	{ ".htm",  "text/htm"        },
	{ ".js",   "text/javascript" },
	{ ".css",  "text/css"        },
	{ ".png",  "image/png"       },
	{ ".gif",  "image/gif"       },
	{ ".jpg",  "image/jpeg"      },
	{ ".jpeg", "image/jpeg"      },
	{ ".txt",  "text/plain"      }
};

int
http_server_create(
    http_server_t **http_server,
    config_t *config,
    struct event_base *event_base,
    fplug_device_t *fplug_device)
{
	http_server_t *new = NULL;
	struct evhttp *evhttp = NULL;
	char address[NI_MAXHOST];
	unsigned short port;
        char resource_path[URL_PATH_MAX];
	
	if (http_server == NULL ||
	    config == NULL ||
	    event_base == NULL ||
	    fplug_device == NULL) {
		errno = EINVAL;
		return 1;
	}
	if ((new = malloc(sizeof(http_server_t))) == NULL) {
		LOG(LOG_ERR, "failed in create http server context");
		goto fail;
	}
	memset(new, 0, sizeof(http_server_t));
	if ((evhttp = evhttp_new(event_base)) == NULL) {
		LOG(LOG_ERR, "failed in create evhttp");
		goto fail;
	}
        if (config_get_string(config, address, sizeof(address),
	    "controller", "httpAddress", DEFAULT_HTTP_ADDRESS, sizeof(address) - 1)) {
                LOG(LOG_ERR, "faile in get http address from config");
                goto fail;
        }
	if (config_get_uint16(config, (uint16_t *)&port, "controller", "httpPort", DEFAULT_HTTP_PORT, 1, 65535)) {
                LOG(LOG_ERR, "faile in get http port from config");
                goto fail;
	} 
        if (config_get_string(config, resource_path, sizeof(resource_path),
	    "controller", "httpResourcePath",  DEFAULT_HTTP_RESOURCE_PATH, sizeof(resource_path) - 1)) {
                LOG(LOG_ERR, "faile in get resource path from config");
                goto fail;
        }
	evhttp_set_gencb(evhttp, default_cb, new);
	new->evhttp = evhttp;
	strlcpy(new->address, address, sizeof(new->address));
	new->port = port;
	strlcpy(new->resource_path, resource_path, sizeof(new->resource_path));
        new->fplug_device = fplug_device;
	*http_server = new;

	return 0;

fail:
	
	if (evhttp) {
		evhttp_free(evhttp);
	}
	if (new) {
		free(new);	
	}

	return 1;
}

int
http_server_start(
    http_server_t *http_server)
{
	struct evhttp_bound_socket *bound_socket = NULL;

	if (http_server == NULL) {
		errno = EINVAL;
		return 1;
	}
	if ((bound_socket = evhttp_bind_socket_with_handle(http_server->evhttp, http_server->address, http_server->port)) == NULL) {
               	LOG(LOG_ERR, "faile in bind socket %s:%d", http_server->address, http_server->port);
		goto fail;
	}
	http_server->bound_socket = bound_socket;

	return 0;

fail:
	if (bound_socket) {
		evhttp_del_accept_socket(http_server->evhttp, bound_socket);
	}
	
	return 1;
}

int
http_server_stop(
    http_server_t *http_server)
{
	if (http_server == NULL) {
		errno = EINVAL;
		return 1;
	}
	if (http_server->bound_socket) {
		evhttp_del_accept_socket(http_server->evhttp, http_server->bound_socket);
	}

	return 0;
}

int
http_server_destroy(
    http_server_t *http_server)
{
	if (http_server == NULL) {
		errno = EINVAL;
		return 1;
	}
	if (http_server->evhttp) {
		evhttp_free(http_server->evhttp);
	}
	free(http_server);

	return 0;
}

static void
default_cb(
    struct evhttp_request *req,
    void *arg)
{
	http_server_t *http_server = arg;
	const char *uri;
	const char *path;
	struct evhttp_uri *decoded = NULL;
	char *decoded_path = NULL;
	char *file_path = NULL;
	size_t url_len;
	size_t len;
	struct stat st;
	struct evbuffer *evb = NULL;
	const char *extension;
	const char *content_type = "text/html";
	int i;
	int fd = -1;
	
	int error = 0;
	int status_code = 0;
	const char *reason = NULL;

	ASSERT(arg != NULL);

        LOG(LOG_DEBUG, "default callback");

	uri = evhttp_request_get_uri(req);
	decoded = evhttp_uri_parse(uri);
	if (!decoded) {
		error = 1;
		status_code = HTTP_BADREQUEST;
		reason = "could not parse uri";
                LOG(LOG_DEBUG, reason);
		goto last; 
	}
	path = evhttp_uri_get_path(decoded);
	if (path == NULL || evutil_ascii_strcasecmp(path, "/") == 0) {
		path = "/index.html";
	}
        LOG(LOG_DEBUG, "path = %s", path);
	decoded_path = evhttp_uridecode(path, 0, NULL);
	if (decoded_path == NULL) {
		error = 1;
		status_code = HTTP_BADREQUEST;
		reason = "could not decode url";
                LOG(LOG_DEBUG, reason);
		goto last; 
	}
        LOG(LOG_DEBUG, "decoded path = %s", decoded_path);
	if (strstr(decoded_path, "..")) {
		error = 1;
		status_code = HTTP_BADREQUEST;
		reason = "unsupport path format";
                LOG(LOG_DEBUG, reason);
		goto last;
	}

	url_len = strlen(decoded_path);
	if (url_len >= sizeof(HTTP_API_URL_PATH) - 1 &&
	    strncmp(decoded_path, HTTP_API_URL_PATH, sizeof(HTTP_API_URL_PATH) - 1) == 0) {
		if (evhttp_request_get_command(req) != EVHTTP_REQ_GET 
		    && evhttp_request_get_command(req) != EVHTTP_REQ_POST) {
			error = 1;
			status_code = HTTP_BADMETHOD;
			reason = "unsupport method";
			LOG(LOG_DEBUG, reason);
			goto last; 
		}
		// api handling
		evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
		if (api_cb(req, decoded_path, evhttp_request_get_command(req), http_server, &status_code, &reason)) {
			error = 1;
		}
		goto last;
	}

	if (evhttp_request_get_command(req) != EVHTTP_REQ_GET) {
		error = 1;
		status_code = HTTP_BADMETHOD;
		reason = "unsupport method";
                LOG(LOG_DEBUG, reason);
		goto last; 
	}

	len = strlen(http_server->resource_path) + 1 /* '/' */ + strlen(decoded_path) + 1 /* '\0' */;
	if ((file_path = malloc(len)) == NULL) {
		error = 1;
		status_code = HTTP_INTERNAL;
		reason = "could not allocate memory of resource path";
                LOG(LOG_DEBUG, reason);
		goto last;
	}
	evutil_snprintf(file_path, len, "%s/%s", http_server->resource_path, decoded_path);

	if (stat(file_path, &st)<0) {
		error = 1;
		status_code = HTTP_NOTFOUND;
		reason = "Not found";
                LOG(LOG_DEBUG, "failed in stat");
		goto last;
	}
        LOG(LOG_DEBUG, "file path = %s", path);

	if (S_ISDIR(st.st_mode)) {
		error = 1;
		status_code = HTTP_NOTFOUND;
		reason = "Not found";
                LOG(LOG_DEBUG, "file path is directory");
		goto last;
	}

	if ((evb = evbuffer_new()) == NULL) {
		error = 1;
		status_code = HTTP_INTERNAL;
		reason = "could not allocate memory of response buffer";
                LOG(LOG_DEBUG, reason);
		goto last;
	}
	
	extension = strrchr(file_path, '.');
	if (extension == NULL || strchr(extension, '/')) {
		error = 1;
		status_code = HTTP_NOTFOUND;
		reason = "Not found";
                LOG(LOG_DEBUG, "invalid extension");
		goto last;
	}
	for (i = 0; i < sizeof(content_types)/sizeof(content_type_map_t); i++) {
		if (evutil_ascii_strcasecmp(content_types[i].extension, extension) == 0) {
			content_type = content_types[i].content_type;
		}
	}
	if (content_type == NULL) {
		error = 1;
		status_code = HTTP_NOTFOUND;
		reason = "Not found";
                LOG(LOG_DEBUG, "content type mismatch");
		goto last;
	}
	if ((fd = open(file_path, O_RDONLY)) < 0) {
		error = 1;
		status_code = HTTP_INTERNAL;
		reason = "could not open resource";
                LOG(LOG_DEBUG, reason);
		goto last;
	}
	if (fstat(fd, &st)<0) {
		error = 1;
		status_code = HTTP_NOTFOUND;
		reason = "Not found";
                LOG(LOG_DEBUG, "failed in fstat");
		goto last;
	}
	evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", content_type);
	evbuffer_add_file(evb, fd, 0, st.st_size);
	evhttp_send_reply(req, 200, "OK", evb);
last:
	if (error) {
		evhttp_send_error(req, status_code, reason);
	}
	if (evb) {
		evbuffer_free(evb);
	}
	if (file_path) {
		free(file_path);
	}
	if (decoded_path) {
		free(decoded_path);
	}
	if (decoded) {
		evhttp_uri_free(decoded);
	}

	return;
}

static int
api_cb(
    struct evhttp_request *req,
    const char *decoded_path,
    enum evhttp_cmd_type cmd_type,
    http_server_t *http_server,
    int *error_status_code,
    const char **error_reason)
{
	int error = 0;
	int api_url_path_len;
	struct evbuffer *postbuf;
	int postbuf_len;
	struct evkeyvalq hdr_qs;
	char *tmp;
	const char  *var;
	char address[ADDRESS_MAX_LEN];
	struct evbuffer *response = NULL;
	api_callback_arg_t api_callback_arg;
	time_t default_start;
	struct tm start_tm;
	time_t default_end;
	struct tm end_tm;
	
	ASSERT(decoded_path != NULL);
	ASSERT(http_server != NULL);
	ASSERT(error_status_code != NULL);
	ASSERT(error_reason != NULL);

	/* 初期化 */
	address[0] = '\0';
	default_end = time(NULL);
	default_start = default_end - 86400;
	localtime_r(&default_start, &start_tm);
	localtime_r(&default_end, &end_tm);

	/* post body 処理 */
	postbuf = evhttp_request_get_input_buffer(req);
	if (evbuffer_get_length(postbuf) > 0) {
		postbuf_len = evbuffer_get_length(postbuf);
		tmp = malloc(postbuf_len + 1);
		memcpy(tmp, evbuffer_pullup(postbuf, -1), postbuf_len);
		tmp[postbuf_len] = '\0';
		evhttp_parse_query_str(tmp, &hdr_qs);
		if ((var = evhttp_find_header(&hdr_qs, "address")) != NULL) {
			strlcpy(address, var, sizeof(address));
		}
		if ((var = evhttp_find_header(&hdr_qs, "start")) != NULL) {
			strptime(var, "%Y%m%d%H%M%S", &start_tm);
		}
		if ((var = evhttp_find_header(&hdr_qs, "end")) != NULL) {
			strptime(var, "%Y%m%d%H%M%S", &end_tm);
		}
		free(tmp);
	}

	/* response buffer 作成 */
	if ((response = evbuffer_new()) == NULL) {
		LOG(LOG_ERR, "failed in memory allocate of response buffer");
		*error_status_code = HTTP_INTERNAL;
		*error_reason = "failed in memory allocate of response buffer";
		error = 1;
		goto last;
	}

	/* api callback argumentの初期化 */
	api_callback_arg.idx = 0;
	api_callback_arg.response = response;

	/* api urlで振り分け */
	api_url_path_len = sizeof(HTTP_API_URL_PATH) - 1;
	if (cmd_type == EVHTTP_REQ_GET && strcmp(&decoded_path[api_url_path_len], API_DEVICIES_URL) == 0) {
		evbuffer_add(response, "[", 1);
		if (fplug_device_active_device_foreach(http_server->fplug_device, create_active_device_response, &api_callback_arg)) {
			LOG(LOG_ERR, "failed in active device foreach");
			*error_status_code = HTTP_INTERNAL;
			*error_reason = "failed in active device foreach";
			error = 1;
			goto last;
		}
		evbuffer_add(response, "]", 1);
		evhttp_send_reply(req, 200, "OK", response);
	} else if (cmd_type == EVHTTP_REQ_POST && strcmp(&decoded_path[api_url_path_len], API_DEVICIE_REALTIME_URL) == 0) {
		evbuffer_add(response, "[", 1);
		if (fplug_device_stat_store_foreach(http_server->fplug_device, address, &start_tm, &end_tm, create_stat_store_response, &api_callback_arg)) {
			LOG(LOG_ERR, "failed in stat store foreach");
			*error_status_code = HTTP_INTERNAL;
			*error_reason = "failed in stat store foreach";
			error = 1;
			goto last;
		}
		evbuffer_add(response, "]", 1);
		evhttp_send_reply(req, 200, "OK", response);
	} else if (cmd_type == EVHTTP_REQ_POST && strcmp(&decoded_path[api_url_path_len], API_DEVICIE_HOURLY_POWER_TOTAL_URL) == 0) {
		evbuffer_add(response, "[", 1);
		if (fplug_device_hourly_power_total_foreach(http_server->fplug_device, address, &start_tm, create_hourly_power_total_response, &api_callback_arg)) {
			LOG(LOG_ERR, "failed in hourly power total foreach");
			*error_status_code = HTTP_INTERNAL;
			*error_reason = "failed in hourly power total foreach";
			error = 1;
			goto last;
		}
		evbuffer_add(response, "]", 1);
		evhttp_send_reply(req, 200, "OK", response);
	} else if (cmd_type == EVHTTP_REQ_POST && strcmp(&decoded_path[api_url_path_len], API_DEVICIE_HOURLY_OTHER_URL) == 0) {
		evbuffer_add(response, "[", 1);
		if (fplug_device_hourly_other_foreach(http_server->fplug_device, address, &start_tm, create_hourly_other_response, &api_callback_arg)) {
			LOG(LOG_ERR, "failed in hourly power total foreach");
			*error_status_code = HTTP_INTERNAL;
			*error_reason = "failed in hourly power total foreach";
			error = 1;
			goto last;
		}
		evbuffer_add(response, "]", 1);
		evhttp_send_reply(req, 200, "OK", response);
	} else if (cmd_type == EVHTTP_REQ_POST && strcmp(&decoded_path[api_url_path_len], API_DEVICIE_RESET_URL) == 0) {
		if (fplug_device_reset(http_server->fplug_device, address)) {
			LOG(LOG_ERR, "failed in reset");
			*error_status_code = HTTP_INTERNAL;
			*error_reason = "failed in reset";
			error = 1;
                        goto last;
		}
		evhttp_send_reply(req, 200, "OK", NULL);
	} else if (cmd_type == EVHTTP_REQ_POST && strcmp(&decoded_path[api_url_path_len], API_DEVICIE_DATETIME_URL) == 0) {
		if (fplug_device_set_datetime(http_server->fplug_device, address)) {
			LOG(LOG_ERR, "failed in set datetime");
			*error_status_code = HTTP_INTERNAL;
			*error_reason = "failed in set datetime";
			error = 1;
                        goto last;
		}
		evhttp_send_reply(req, 200, "OK", NULL);
	} else {
		LOG(LOG_ERR, "unsupported api: method = %d, url = %s, partial url = %s", cmd_type, decoded_path, &decoded_path[api_url_path_len]);
		*error_status_code = HTTP_NOTFOUND;
		*error_reason = "unsupported api";
		error = 1;
		goto last;
	}


last:
	if (response) {
		evbuffer_free(response);
	}
	
	return error;
}

static void
create_active_device_response(
    const char *device_name,
    const char *device_address,
    void *cb_arg)
{
	api_callback_arg_t *api_callback_arg = cb_arg;
	char buf[ENTRY_BUF_MAX];
	int len;
	
	ASSERT(cb_arg != NULL);

	if (api_callback_arg->idx != 0) {
		evbuffer_add(api_callback_arg->response, ",", 1);
	}
	len = snprintf(buf, sizeof(buf), "{\"name\":\"%s\",\"address\":\"%s\"}", device_name, device_address);
	evbuffer_add(api_callback_arg->response, buf, len);
	api_callback_arg->idx++;
}

static void
create_stat_store_response(
   time_t stat_time,
   double temperature,
   unsigned int humidity,
   unsigned int illuminance,
   double rwatt,
   void *cb_arg)
{
	api_callback_arg_t *api_callback_arg = cb_arg;
	char buf[ENTRY_BUF_MAX];
	int len;
	
	ASSERT(cb_arg != NULL);

	if (api_callback_arg->idx != 0) {
		evbuffer_add(api_callback_arg->response, ",", 1);
	}
	len = snprintf(buf, sizeof(buf),
            "{\"index\":%d,\"time\":%lu,\"temperature\":%lf,\"humidity\":%u,\"intilluminance\":%u,\"rwatt\":%lf}",
            api_callback_arg->idx, stat_time, temperature, humidity, illuminance, rwatt);
	evbuffer_add(api_callback_arg->response, buf, len);
	api_callback_arg->idx++;
}

static void
create_hourly_power_total_response(
    double watt,
    unsigned char reliability,
    void *cb_arg)
{
	api_callback_arg_t *api_callback_arg = cb_arg;
	char buf[ENTRY_BUF_MAX];
	int len;
	
	ASSERT(cb_arg != NULL);

	if (api_callback_arg->idx != 0) {
		evbuffer_add(api_callback_arg->response, ",", 1);
	}
	len = snprintf(buf, sizeof(buf),
            "{\"index\":%d,\"reliability\":%u,\"watt\":%lf}",
            api_callback_arg->idx, reliability, watt);
	evbuffer_add(api_callback_arg->response, buf, len);
	api_callback_arg->idx++;
}

static void
create_hourly_other_response(
    double temperature,
    unsigned int humidity,
    unsigned int illuminance,
    void *cb_arg)
{
	api_callback_arg_t *api_callback_arg = cb_arg;
	char buf[ENTRY_BUF_MAX];
	int len;
	
	ASSERT(cb_arg != NULL);

	if (api_callback_arg->idx != 0) {
		evbuffer_add(api_callback_arg->response, ",", 1);
	}
	len = snprintf(buf, sizeof(buf),
            "{\"index\":%d,\"temperature\":%lf,\"humidity\":%u,\"intilluminance\":%u}",
            api_callback_arg->idx, temperature, humidity, illuminance);
	evbuffer_add(api_callback_arg->response, buf, len);
	api_callback_arg->idx++;
}

