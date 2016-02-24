#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "common_macros.h"
#include "common_define.h"
#include "config.h"
#include "stat_store.h"

#define DEFAULT_STORE_POINT 6400000

struct stat_value {
        time_t time;
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

	if (stat_store == NULL ||
	    config == NULL) {
		return EINVAL;
	}

	new = malloc(sizeof(stat_store_t));
	if (new == NULL) {
		LOG(LOG_ERR, "failed in create stat store");
		goto fail;
	}
	memset(new, 0, izeof(stat_store_t))

        if (config_get_uint64(config, (uint64_t *)&new->max_point, "stat", "storePoint", DEFAULT_STORE_POINT, 10, 15000000)) {
                LOG(LOG_ERR, "faile in get store point (10 to 15000000)");
                goto fail;
        }

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


/*
 * stat storeを削除する
 */
void stat_store_destroy(stat_store_t *stat_store);
/*
 * stat_storeにstatを追加する
 */
int stat_store_stat_add(stat_store_t *stat_store, time_t time, double temperature, unsigned int humidity, unsigned int illuminance, double rwatt);
/*
 * stat情報をループ処理で取得する
 */
int stat_store_stat_foreach(stat_store_t *stat_store, void (*foreach_cb)(time_t time, double temperature, unsigned int humidity, unsigned intilluminance, double rwatt, void *cb_arg), void *cb_arg);

