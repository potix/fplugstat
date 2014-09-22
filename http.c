#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>

#include "common_macros.h"
#include "common_define.h"
#include "config.h"
#include "logger.h"
#include "string_util.h"
#include "http.h"

#ifndef URL_PATH_MAX
#define URL_PATH_MAX CONFIG_MAX_STR_LEN
#endif

#ifndef DEFAULT_HTTP_RESOURCE_PATH
#define DEFAULT_HTTP_RESOURCE_PATH FPLUGSTAT_PATH "/www"
#endif

#ifndef DEFAULT_HTTP_ROOT_URL_PATH
#define DEFAULT_HTTP_ROOT_URL_PATH "/"
#endif

#ifndef DEFAULT_HTTP_API_URL_PATH
#define DEFAULT_HTTP_API_URL_PATH "/api/"
#endif

#ifndef DEFAULT_HTTP_ADDRESS
#define DEFAULT_HTTP_ADDRESS "0.0.0.0"
#endif

#ifndef DEFAULT_HTTP_PORT
#define DEFAULT_HTTP_PORT "80"
#endif

#ifndef LOCAL_HTTP_ADDRESS
#define LOCAL_HTTP_ADDRESS "127.0.0.1"
#endif

struct http_server {
	struct evhttp *evhttp;
	struct evhttp_bound_socket *bound_socket;
	struct evhttp_bound_socket *local_bound_socket;
	int localhost_only;
	char address[NI_MAXHOST];
	unsigned short port;
	char root_url_path[URL_PATH_MAX];
        char api_url_path[URL_PATH_MAX];
	int api_url_path_len;
        char resource_path[URL_PATH_MAX];
        //fplug_devicies_t *fplug_devicies;
};

struct content_type_map {
	const char *extension;
	const char *content_type;
};

static void default_cb(
    struct evhttp_request *req,
    void *arg);

static int api_cb(
    struct evhttp_request *req,
    const char *decoded_path,
    http_server_t *http_server,
    int *status_code,
    const char **reason);

struct content_type_map content_types[] = {
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
typedef struct content_type_map content_type_map_t;

int
http_server_create(
    http_server_t **http_server,
    struct event_base *event_base,
    config_t *config)
{
	http_server_t *new = NULL;
	struct evhttp *evhttp = NULL;
	int localhost_only = 0;
	char address[NI_MAXHOST];
	unsigned short port;
	char root_url_path[URL_PATH_MAX];
        char api_url_path[URL_PATH_MAX];
        char resource_path[URL_PATH_MAX];
	
	if (http_server == NULL || event_base == NULL || config == NULL) {
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
	if (config_get_bool(config, &localhost_only, "controller", "localhostOnly", "true")) {
                LOG(LOG_ERR, "faile in get localhost only flag from config");
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
        if (config_get_string(config, root_url_path, sizeof(root_url_path),
	    "controller", "httpRootUrlPath",  DEFAULT_HTTP_ROOT_URL_PATH, sizeof(root_url_path) - 1)) {
                LOG(LOG_ERR, "faile in get root url path from config");
                goto fail;
        }
        if (config_get_string(config, api_url_path, sizeof(api_url_path),
	    "controller", "httpApiUrlPath",  DEFAULT_HTTP_API_URL_PATH, sizeof(api_url_path) - 1)) {
                LOG(LOG_ERR, "faile in get api url path from config");
                goto fail;
        }
        if (config_get_string(config, resource_path, sizeof(resource_path),
	    "controller", "httpResourcePath",  DEFAULT_HTTP_RESOURCE_PATH, sizeof(resource_path) - 1)) {
                LOG(LOG_ERR, "faile in get resource path from config");
                goto fail;
        }
	evhttp_set_gencb(evhttp, default_cb, new);
	new->evhttp = evhttp;
	new->localhost_only = localhost_only;
	strlcpy(new->address, address, sizeof(new->address));
	new->port = port;
	strlcpy(new->root_url_path, root_url_path, sizeof(new->root_url_path));
	strlcpy(new->api_url_path, api_url_path, sizeof(new->api_url_path));
	new->api_url_path_len = strlen(api_url_path);
	strlcpy(new->resource_path, resource_path, sizeof(new->resource_path));
	*http_server = new;

	return 0;

fail:
	
	if (evhttp) {
		evhttp_free(evhttp);
	}
	free(new);	

	return 1;
}

int
http_server_start(
    http_server_t *http_server)
{
	struct evhttp_bound_socket *bound_socket = NULL;
	struct evhttp_bound_socket *local_bound_socket = NULL;

	if (http_server == NULL) {
		errno = EINVAL;
		return 1;
	}
	if (!http_server->localhost_only) {
		if ((bound_socket = evhttp_bind_socket_with_handle(http_server->evhttp, http_server->address, http_server->port)) == NULL) {
       	        	LOG(LOG_ERR, "faile in bind socket");
			goto fail;
		}
	}
	if ((local_bound_socket = evhttp_bind_socket_with_handle(http_server->evhttp, LOCAL_HTTP_ADDRESS, http_server->port)) == NULL) {
                LOG(LOG_ERR, "faile in bind local socket");
		goto fail;
	}
	http_server->bound_socket = bound_socket;
	http_server->local_bound_socket = local_bound_socket;

	return 0;

fail:
	if (bound_socket) {
		evhttp_del_accept_socket(http_server->evhttp, bound_socket);
	}
	if (local_bound_socket) {
		evhttp_del_accept_socket(http_server->evhttp, local_bound_socket);
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
	if (http_server->local_bound_socket) {
		evhttp_del_accept_socket(http_server->evhttp, http_server->local_bound_socket);
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
	const char *content_type = NULL;
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
	if (url_len >= http_server->api_url_path_len &&
	    strncmp(decoded_path, http_server->api_url_path, http_server->api_url_path_len) == 0) {
		if (evhttp_request_get_command(req) != EVHTTP_REQ_GET 
		    && evhttp_request_get_command(req) != EVHTTP_REQ_DELETE) {
			error = 1;
			status_code = HTTP_BADMETHOD;
			reason = "unsupport method";
			LOG(LOG_DEBUG, reason);
			goto last; 
		}
		// api handling
		if (api_cb(req, decoded_path, http_server, &status_code, &reason)) {
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
	if (fd >= 0) {
		close(fd);
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
    http_server_t *http_server,
    int *status_code,
    const char **reason)
{
	/* GET /api/statistics/realtime/power リアルタイム消費電力取得要求 リアルタイム消費電力取得要求受理応答 リアルタイム消費電力取得要求不可応答*/	
	/* GET /api/statistics/realtime/humidity 湿度取得要求 湿度取得要求受理応答 湿度取得要求不可応答*/	
	/* GET /api/statistics/realtime/illumination 照度取得要求 照度取得要求受理応答 照度取得要求不可応答*/	
	/* GET /api/statistics/realtime/temperature 温度取得要求 温度取得要求受理応答 温度取得要求不可応答 */	
	/* GET /api/statistics/total/power/current 積算電力量取得要求 積算電力量取得応答 // 24時間分の積算 */	
	/* GET /api/statistics/total/power/past 積算電力量取得要求 積算電力量取得応答 (過去分) // 24時間分の積算 */	
	/* GET /api/statistics/total/other  温度、湿度、照度データ取得要求  温度、湿度、照度データ取得応答 // 24時間分 */	
	/* DELETE /api/statistics プラグ初期設定要求, プラグ初期設定要求受理応答, プラグ初期設定要求受理応答*/	

	/* 初期化時にやるもの */
	/* 日時設定要求 日時設定応答 */


	return 0;
}
