#ifndef HTTP_H
#define HTTP_H

typedef struct http_server http_server_t;

int http_server_create(
    http_server_t **http_server,
    struct event_base *event_base,
    config_t *config,
    fplug_devicies_t *fplug_devicies);

int http_server_start(
    http_server_t *http_server);

int http_server_stop(
    http_server_t *http_server);

int http_server_destroy(
    http_server_t *http_server);

#endif
