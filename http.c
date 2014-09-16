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
#define DEFAULT_HTTP_ADDRESS "127.0.0.1"
#endif

#ifndef DEFAULT_HTTP_PORT
#define DEFAULT_HTTP_PORT 80
#endif

struct http_server {
	struct evhttp *evhttp;
	struct evhttp_bound_socket *bound_socket;
	char address[NI_MAXHOST];
	unsigned short port;
	char root_url_path[URL_PATH_MAX];
	int set_root_cb = 0;
        char api_url_path[URL_PATH_MAX];
	int set_api_cb = 0;
        char resource_path[URL_PATH_MAX];
        //fplug_devicies_t *fplug_devicies;
};

int
http_server_create(
    http_server_t **http_server,
    struct event_base *event_base,
    config_t *config)
{
	http_server_t *new = NULL;
	struct evhttp *evhttp = NULL;
	struct evhttp_bound_socket *bound_socket = NULL;
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
	evhttp_set_gencb(http, default_cb, NULL);
	new->evhttp = evhttp:
	strlcpy(new->address, address, sizeof(new->address));
	new->port = port;
	strlcpy(new->root_url_path, root_url_path, sizeof(new->root_url_path));
	strlcpy(new->api_url_path, api_url_path, sizeof(new->api_url_path));
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
	int set_root_cb = 0;
	int set_api_cb = 0;

	if (http_server == NULL) {
		errno = EINVAL;
		return 1;
	}
	if (evhttp_set_cb(http, root_url_path, root_cb, new)) {
                LOG(LOG_ERR, "faile in set root callback");
                goto fail;
	}
	set_root_cb = 1;
	if (evhttp_set_cb(http, api_url_path, api_cb, new)) {
                LOG(LOG_ERR, "faile in set api callback");
		goto fail:
	}
	set_api_cb = 1;
	if ((bound_socket = evhttp_bind_socket_with_handle(http_server->evhttp, http_server->address, http_server->port)) == NULL) {
                LOG(LOG_ERR, "faile in bind socket");
		goto fail:
	}
	http_pserver->bound_socket = bound_socket;
	http_pserver->set_root_cb = set_root_cb;
	http_pserver->set_api_cb = set_api_cb;

	return 0;

fail:
	if (set_api_cb) {
		evhttp_del_cb(evhttp, api_url_path);
	}
	if (set_root_cb) {
		evhttp_del_cb(evhttp, root_url_path);
	}
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
	if (http_server->set_api_cb) {
		evhttp_del_cb(http_server->evhttp, http_server->api_url_path);
	}
	if (http_server->set_root_cb) {
		evhttp_del_cb(http_server->evhttp, http_server->root_url_path);
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
		evhttp_free(evhttp);
	}
	free(http_server);
}


static void
default_cb(
    struct evhttp_request *req,
    void *arg)
{
	evhttp_send_error(req, HTTP_NOTFOUND, "Not Found");
}

static void
root_cb(
    struct evhttp_request *req,
    void *arg)
{
	http_server_t *http_server = arg;
	const char *uri;
	const char *path;
	struct evhttp_uri *decoded = NULL;
	char *decoded_path = NULL;
	char *file_path = NULL;

	/* XXXX writing XXX */
	struct evbuffer *evb = NULL;
	size_t len;
	int fd = -1;
	struct stat st;
	/* XXX */
	
	int error = 0;
	int status_code = 0;
	const char *reason = NULL;

	ASSERT(arg != NULL);
	uri = evhttp_request_get_uri(req);
	if (evhttp_request_get_command(req) != EVHTTP_REQ_GET) {
		error = 1;
		status_code = HTTP_BADMETHOD;
		reason = "Not support method";
		goto last; 
	}
	decoded = evhttp_uri_parse(uri);
	if (!decoded) {
		error = 1;
		status_code = HTTP_BADREQUEST;
		reason = "could not parse uri";
		goto last; 
	}
	path = evhttp_uri_get_path(decoded);
	if (!path) path = "/";

	decoded_path = evhttp_uridecode(path, 0, NULL);
	if (decoded_path == NULL) {
		error = 1;
		status_code = HTTP_BADREQUEST;
		reason = "could not decode url";
		goto last; 
	}
	if (strstr(decoded_path, "..")) {
		error = 1;
		status_code = HTTP_BADREQUEST;
		reason = "unsupport path format";
		goto last;
	}

	len = strlen(http_server->resource_path) + 1 /* '/' */ + strlen(decoded_path) + 1 /* '\0' */;
	if ((file_path = malloc(len)) == NULL) {
		error = 1;
		status_code = HTTP_INTERNAL;
		reason = "could not allocate memory of resource path";
		goto last;
	}
	evutil_snprintf(file_path, len, "%s/%s", http_server->resource_path, decoded_path);

	if (stat(file_path, &st)<0) {
		error = 1;
		status_code = HTTP_NOTFOUND;
		reason = "could not allocate memory of resource path";
		goto last;
	}

	if (S_ISDIR(st.st_mode)) {
		error = 1;
		status_code = HTTP_NOTFOUND;
		reason = "could not allocate memory of resource path";
		goto last;
	}

	if (evb = evbuffer_new()) == NULL) {
		error = 1;
		status_code = HTTP_INTERNAL;
		reason = "could not allocate memory of response buffer";
		goto last;
	}
	
	/* XXX writing XXX */
	const char *type = guess_content_type(decoded_path);
	if ((fd = open(file_path, O_RDONLY)) < 0) {
		perror("open");
		goto err;
	}

	if (fstat(fd, &st)<0) {
		/* Make sure the length still matches, now that we
		 * opened the file :/ */
		perror("fstat");
		goto err;
	}
	evhttp_add_header(evhttp_request_get_output_headers(req),
	    "Content-Type", type);
	evbuffer_add_file(evb, fd, 0, st.st_size);
	evhttp_send_reply(req, 200, "OK", evb);
	/* XXX XXX XXX*/


last:
	if (error) {
		evhttp_send_error(req, status_code, reason);
	}

	if (decoded) {
		evhttp_uri_free(decoded);
	}
	if (decoded_path) {
		free(decoded_path);
	}
	if (file_path) {
		free(file_path);
	}
	if (evb) {
		evbuffer_free(evb);
	}

	return;
}

static void
api_cb(
    struct evhttp_request *req,
    void *arg)
{
}
