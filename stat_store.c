#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "common_macros.h"
#include "common_define.h"
#include "logger.h"
#include "config.h"
#include "stat_store.h"

#define DEFAULT_STORE_POINT "6400000"

struct stat_value {
        time_t stat_time;
        unsigned int humidity;
        unsigned int illuminance;
        double temperature;
        double rwatt;
};
typedef struct stat_value stat_value_t;

struct stat_store {
        stat_value_t *stat_value;
        unsigned long long  max_point;
        unsigned long long  current_point; /* 0 to max_point - 1 */
        int full; /* full flag */
};

int
stat_store_create(
    stat_store_t **stat_store,
    config_t *config)
{

	stat_store_t *new = NULL;
	stat_value_t *stat_value_new = NULL;
	uint32_t max_point;

	if (stat_store == NULL ||
	    config == NULL) {
		errno = EINVAL;
		return 1;
	}

	new = malloc(sizeof(stat_store_t));
	if (new == NULL) {
		LOG(LOG_ERR, "failed in create stat store");
		goto fail;
	}
	memset(new, 0, sizeof(stat_store_t));

        if (config_get_uint32(config, &max_point, "stat", "storePoint", DEFAULT_STORE_POINT, 10, 15000000)) {
                LOG(LOG_ERR, "faile in get store point (10 to 15000000)");
                goto fail;
        }
	new->max_point = max_point;

	stat_value_new = malloc(sizeof(stat_value_t) * new->max_point);
	if (stat_value_new == NULL) {
		LOG(LOG_ERR, "failed in create stat value");
		goto fail;
	}
	memset(stat_value_new, 0, sizeof(stat_value_t) * new->max_point);
	new->stat_value = stat_value_new;

	*stat_store = new;

	return 0;

fail:

	if (stat_value_new) {
		free(stat_value_new);
	}
	if (new) {
		free(new);
	}

	return 1;
}


int
stat_store_destroy(
    stat_store_t *stat_store)
{
	
	if (stat_store == NULL) {
		errno = EINVAL;
		return 1;
	}
	
	if (stat_store->stat_value) {
		free(stat_store->stat_value);
	}

	if (stat_store) {
		free(stat_store);
	}
	
	return 0;
}

int
stat_store_stat_add(
    stat_store_t *stat_store,
    time_t stat_time,
    double temperature,
    unsigned int humidity,
    unsigned int illuminance,
    double rwatt)
{
	stat_value_t *stat_value;

	if (stat_store == NULL) {
		errno = EINVAL;
		return 1;
	}
	stat_value = &stat_store->stat_value[stat_store->current_point];
	stat_value->stat_time = stat_time;  
	stat_value->temperature = temperature;  
	stat_value->humidity = humidity;  
	stat_value->illuminance = illuminance;  
	stat_value->rwatt = rwatt;
	stat_store->current_point++;
	if (stat_store->current_point == stat_store->max_point) {
		stat_store->current_point = 0;
		stat_store->full = 1;
	}

	return 0;
}

int
stat_store_stat_foreach(
    stat_store_t *stat_store,
    time_t start,
    time_t end,
    void (*foreach_cb)(time_t stat_time, double temperature, unsigned int humidity, unsigned intilluminance, double rwatt, void *cb_arg),
    void *cb_arg)
{
	stat_value_t *stat_value;
	int i;

	if (stat_store == NULL) {
		errno = EINVAL;
		return 1;
	}

	if (stat_store->full) {
		for (i = stat_store->current_point + 1; i < stat_store->max_point; i++) {
			stat_value = &stat_store->stat_value[i];
			if (start <= stat_value->stat_time && stat_value->stat_time <= end) {
				foreach_cb(stat_value->stat_time, stat_value->temperature, stat_value->humidity, stat_value->illuminance, stat_value->rwatt, cb_arg);
			}
		}
	}
	for (i = 0; i < stat_store->current_point; i++) {
		stat_value = &stat_store->stat_value[i];
		if (start <= stat_value->stat_time && stat_value->stat_time <= end) {
			foreach_cb(stat_value->stat_time, stat_value->temperature, stat_value->humidity, stat_value->illuminance, stat_value->rwatt, cb_arg);
		}
	}

	return 0;
}

