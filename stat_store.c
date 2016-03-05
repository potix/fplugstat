#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "common_macros.h"
#include "common_define.h"
#include "logger.h"
#include "config.h"
#include "file_util.h"
#include "stat_store.h"

#define DEFAULT_STORE_POINT "6400000"
#define DEFAULT_SAVE_ENABLE "true"
#define DEFAULT_SAVE_PATH "/var/tmp/fplugstatd"

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
        int save_enable;
        char save_path[CONFIG_MAX_STR_LEN];
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
		errno = EINVAL;
		return 1;
	}

	new = malloc(sizeof(stat_store_t));
	if (new == NULL) {
		LOG(LOG_ERR, "failed in create stat store");
		goto fail;
	}
	memset(new, 0, sizeof(stat_store_t));
        if (config_get_uint32(config, (uint32_t *)&new->max_point, "stat", "storePoint", DEFAULT_STORE_POINT, 10, 15000000)) {
                LOG(LOG_ERR, "faile in get store point (10 to 15000000)");
                goto fail;
        }
        if (config_get_bool(config, &new->save_enable, "stat", "save_enable", DEFAULT_SAVE_PATH)) {
                LOG(LOG_ERR, "faile in get save path");
                goto fail;
        }
        if (config_get_string(config, new->save_path, sizeof(new->save_path), "stat", "save_path", DEFAULT_SAVE_PATH, sizeof(new->save_path) - 1)) {
                LOG(LOG_ERR, "faile in get save path");
                goto fail;
        }
	if (new->save_enable) {
		if (mkdirs(new->save_path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
			LOG(LOG_ERR, "faile in create save path (path = %s)", new->save_path);
			goto fail;
		}
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
    void (*foreach_cb)(time_t stat_time, double temperature, unsigned int humidity, unsigned int illuminance, double rwatt, void *cb_arg),
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

int
stat_store_stat_save(
    stat_store_t *stat_store,
    char *file_name,
    time_t stat_time,
    double temperature,
    unsigned int humidity,
    unsigned int illuminance,
    double rwatt)
{
	int error = 0;
	stat_value_t stat_value;
	char path[MAXPATHLEN];
	int fd = -1;
	ssize_t wlen;

	if (stat_store == NULL ||
	    file_name == NULL) {
		errno = EINVAL;
		return 1;
	}
	if (stat_store->save_enable == 0) {
		return 0;
	}
	snprintf(path, sizeof(path), "%s/%s", stat_store->save_path, file_name);
	stat_value.stat_time = stat_time;  
	stat_value.temperature = temperature;  
	stat_value.humidity = humidity;  
	stat_value.illuminance = illuminance;  
	stat_value.rwatt = rwatt;
	if ((fd = open(path, O_APPEND|O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) == -1) {
		LOG(LOG_ERR, "failed in open file (path = %s)", path);
		error = 1;
		goto last;
	}
	LOG_DUMP(LOG_DEBUG, (unsigned char *)&stat_value, sizeof(stat_value_t));
	LOG(LOG_DEBUG, "save: stat_time = %ld, temperature = %llf, humidity = %u, illuminance = %u, rwatt = %lf",
	    stat_value.stat_time, stat_value.temperature, stat_value.humidity, stat_value.illuminance, stat_value.rwatt);
	wlen = write(fd, &stat_value, sizeof(stat_value_t));
	if (wlen != sizeof(stat_value_t)) {
		LOG(LOG_ERR, "failed in save stat (path = %s, wlen = %lld)", path, wlen);
		error = 1;
		goto last;
	}

last:
	if (fd != -1) {
		close(fd);
	}

	return error;
}

int
stat_store_restore(
    stat_store_t *stat_store,
    char *file_name)
{
	int error = 0;
	stat_value_t stat_value;
	char path[MAXPATHLEN];
	struct stat st;
	int fd = -1;
	ssize_t rlen;
	off_t pos = 0;
	unsigned long long i, count;

	if (stat_store == NULL ||
	    file_name == NULL) {
		errno = EINVAL;
		return 1;
	}
	
	snprintf(path, sizeof(path), "%s/%s", stat_store->save_path, file_name);
	if (stat(path, &st)) {
		LOG(LOG_WARNING, "failed in stat faile (path = %s)", path);
		return 0;
	}
	count = st.st_size/sizeof(stat_value_t);
	if (count > stat_store->max_point) {
		pos = st.st_size - (stat_store->max_point * sizeof(stat_value_t));
        } else {
		pos = 0;
        }
	if ((fd = open(path, O_RDONLY)) == -1) {
		LOG(LOG_ERR, "failed in open file (path = %s)", path);
		error = 1;
		goto last;
	}
	if (lseek(fd, pos, SEEK_SET) == -1) {
		LOG(LOG_ERR, "failed in seek (path = %s, pos = %lld)", path, pos);
		error = 1;
		goto last;
	}
	for (i = 0; i < count; i++) {
		rlen = read(fd, &stat_value, sizeof(stat_value_t));	
		if (rlen != sizeof(stat_value_t)) {	
			LOG(LOG_ERR, "failed in read data (path = %s, rlen = %lld)", path, rlen);
			error = 1;
			goto last;
		}
		LOG_DUMP(LOG_DEBUG, (unsigned char *)&stat_value, sizeof(stat_value_t));
		LOG(LOG_DEBUG, "loaded: stat_time = %ld, temperature = %llf, humidity = %u, illuminance = %u, rwatt = %lf",
		    stat_value.stat_time, stat_value.temperature, stat_value.humidity, stat_value.illuminance, stat_value.rwatt);
		if (stat_store_stat_add(stat_store, stat_value.stat_time, stat_value.temperature, stat_value.humidity, stat_value.illuminance, stat_value.rwatt)) {
			LOG(LOG_ERR, "failed in add stat value (path = %s)", path);
			error = 1;
			goto last;
		}
	}
	
last:
	if (fd != -1) {
		close(fd);
	}

	return error;
}

