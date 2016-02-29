#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <event2/event.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include "common_macros.h"
#include "common_define.h"
#include "logger.h"
#include "string_util.h"
#include "config.h"
#include "stat_store.h"
#include "echonet_lite.h"
#include "fplug_device.h"

#define DEFAULT_MAX_DEVICE "3"
#define DEFAULT_POLLING_INTERVAL "5"

#define NAME_MAX_LEN 64

struct bluetooth_device {
        char device_name[NAME_MAX_LEN];
        char device_address[ADDRESS_MAX_LEN];
        struct sockaddr_rc saddr;
        int sd;
        int connected;
        stat_store_t *stat_store;
	enl_request_frame_info_t enl_request_frame_info;
	enl_response_frame_info_t enl_response_frame_info;
	enl_request_any_frame_info_t enl_request_any_frame_info;
	enl_response_any_frame_info_t enl_response_any_frame_info;
};
typedef struct bluetooth_device bluetooth_device_t;

struct fplug_device {
        bluetooth_device_t *bluetooth_device;
        unsigned int max_device;
        unsigned int available_count;
	struct event_base *event_base;
	struct event *polling_event;
	struct timeval polling_interval;
};

/* not echonet lite frame */
struct fplug_set_datetime_request_frame {
	unsigned char req_type;	
	unsigned char hour;	
	unsigned char min;	
	unsigned short year;	
	unsigned char month;	
	unsigned char day;	
}__attribute__((__packed__));
typedef struct fplug_set_datetime_request_frame fplug_set_datetime_request_frame_t;
typedef struct fplug_set_datetime_request_frame fplug_hourly_power_total_edata_t;
typedef struct fplug_set_datetime_request_frame fplug_hourly_other_edata_t;

struct fplug_set_datetime_response_frame {
	unsigned char res_type;	
	unsigned char result;	
}__attribute__((__packed__));
typedef struct fplug_set_datetime_response_frame fplug_set_datetime_response_frame_t;

struct fplug_hourly_power_total_data_edt {
	unsigned short watt;
	unsigned char reliability;
}__attribute__((__packed__));
typedef struct fplug_hourly_power_total_data_edt fplug_hourly_power_total_data_edt_t;

struct fplug_hourly_other_data_edt {
	unsigned short temperature;
	unsigned char humidity;
	unsigned char illuminance;
}__attribute__((__packed__));
typedef struct fplug_hourly_other_data_edt fplug_hourly_other_data_edt_t;

static int connect_bluetooth_device(bluetooth_device_t *bluetooth_device);
static void close_bluetooth_device(bluetooth_device_t *bluetooth_device);
static int bluetooth_device_set_datetime(bluetooth_device_t *bluetooth_device);
static int fplug_device_get_bluetooth_device(fplug_device_t *fplug_device,
     const char *device_address, bluetooth_device_t **bluetooth_device);
static void fplug_device_polling(evutil_socket_t fd, short events, void *arg);
static int bluetooth_device_realtime_stat(bluetooth_device_t *bluetooth_device);
static int bluetooth_device_write_request_frame(bluetooth_device_t *bluetooth_device, int type);
static int bluetooth_device_read_response_frame(bluetooth_device_t *bluetooth_device, int type);
static int device_write_request(int sd, unsigned char *frame, size_t frame_len);
static int device_read_response(int sd, unsigned char *buffer, size_t buffer_len);

int
fplug_device_create(
    fplug_device_t **fplug_device,
    config_t *config,
    struct event_base *event_base) {
	fplug_device_t *new = NULL;
	bluetooth_device_t *bluetooth_device_new = NULL;
	int i;
        char dev_section[CONFIG_LINE_BUF];
	bluetooth_device_t *bluetooth_device;

	if (fplug_device == NULL ||
	    config == NULL ||
	    event_base == NULL) {
		errno = EINVAL;
		return 1;
	}

	new = malloc(sizeof(fplug_device_t));
	if (new == NULL) {
                LOG(LOG_ERR, "faile in create fplug device");
		goto fail;
	}
	memset(new, 0, sizeof(fplug_device_t));
	new->event_base = event_base;
	new->polling_event = evtimer_new(event_base, fplug_device_polling, new);
	if (new->polling_event == NULL) {
                LOG(LOG_ERR, "faile in create polling event");
                goto fail;
	}
        if (config_get_uint32(config, (uint32_t *)&new->max_device, "fplug", "maxDevice", DEFAULT_MAX_DEVICE, 1, 7)) {
                LOG(LOG_ERR, "faile in get max device (1 to 7)");
                goto fail;
        }
        if (config_get_uint32(config, (uint32_t *)&new->polling_interval.tv_sec, "fplug", "pollingInterval", DEFAULT_POLLING_INTERVAL, 3, 86400)) {
                LOG(LOG_ERR, "faile in get polling interval (3 to 86400)");
                goto fail;
        }

	bluetooth_device_new = malloc(sizeof(bluetooth_device_t) * new->max_device);
	if (bluetooth_device_new == NULL) {
                LOG(LOG_ERR, "faile in create bluetooth device");
		goto fail;
	}
	memset(bluetooth_device_new, 0, sizeof(bluetooth_device_t) * new->max_device);
	new->bluetooth_device = bluetooth_device_new;
 
	for (i = 0; i < new->max_device; i++) {
		bluetooth_device = &new->bluetooth_device[i];
		bluetooth_device->sd = -1;
		snprintf(dev_section, sizeof(dev_section), "%s%d", "device", i + 1);
		if (config_get_string(config, bluetooth_device->device_name,
		    sizeof(bluetooth_device->device_name), dev_section, "name",  NULL, sizeof(bluetooth_device->device_name) - 1)) {
			continue;
		}
                if (config_get_string(config, bluetooth_device->device_address,
		    sizeof(bluetooth_device->device_address), dev_section, "address",  NULL, sizeof(bluetooth_device->device_address) - 1)) {
                        continue;
                }
		if (stat_store_create(&bluetooth_device->stat_store, config)) {
                	LOG(LOG_ERR, "faile in create stat store");
			goto fail;
		} 
        }
	*fplug_device = new;

	return 0;

fail:

	if (bluetooth_device_new) {
		for (i = 0; i < new->max_device; i++) {
			bluetooth_device = &bluetooth_device_new[i];
			if (bluetooth_device->stat_store) {
				stat_store_destroy(bluetooth_device->stat_store);
				bluetooth_device->stat_store = NULL;
			}
		}
		free(bluetooth_device_new);
	}
	if (new) {
		if (new->polling_event) {
			free(new->polling_event);
		}
		free(new);
	}

	return 1;
}

int
fplug_device_connect(
    fplug_device_t *fplug_device) {
	int i;

	if (fplug_device == NULL) {
		errno = EINVAL;
		return 1;
	}
        for (i = 0; i < fplug_device->max_device; i++) {
		if (fplug_device->bluetooth_device[i].device_name[0] != '\0' &&
		    fplug_device->bluetooth_device[i].device_address[0] != '\0') {
			if (connect_bluetooth_device(&fplug_device->bluetooth_device[i])) {
				continue;
			}
			if (bluetooth_device_set_datetime(&fplug_device->bluetooth_device[i])) {
				LOG(LOG_WARNING, "faild in set datetime to device");
			}
			fplug_device->available_count++;
		}
        }
	if (fplug_device->available_count == 0) {
                LOG(LOG_ERR, "no connected bluetooth devicies\n");
		return 1;
	}

	return 0;
}

int
fplug_device_polling_start(
    fplug_device_t *fplug_device) {

	if (fplug_device == NULL) {
		errno = EINVAL;
		return 1;
	}
	if (evtimer_add(fplug_device->polling_event, &fplug_device->polling_interval)) {
                LOG(LOG_ERR, "faile in create fplug device");
	}

	return 0;
}

int
fplug_device_polling_stop(
    fplug_device_t *fplug_device) {

	if (fplug_device == NULL) {
		errno = EINVAL;
		return 1;
	}

	evtimer_del(fplug_device->polling_event);

	return 0;
}

int
fplug_device_destroy(
    fplug_device_t *fplug_device)
{
	int i;
	bluetooth_device_t *bluetooth_device;

	if (fplug_device == NULL) {
		errno = EINVAL;
		return 1;
	}
        for (i = 0; i < fplug_device->max_device; i++) {
		bluetooth_device = &fplug_device->bluetooth_device[i];
		close_bluetooth_device(bluetooth_device);
		if (bluetooth_device->stat_store) {
			stat_store_destroy(bluetooth_device->stat_store);
			bluetooth_device->stat_store = NULL;
		}
	}
	if (fplug_device->bluetooth_device) {
		free(fplug_device->bluetooth_device);
	}
	if (fplug_device->polling_event) {
		event_free(fplug_device->polling_event);
	}
	free(fplug_device);

	return 0;
}

int
fplug_device_active_device_foreach(
    fplug_device_t *fplug_device,
    void (*foreach_cb)(const char *device_name, const char *device_address, void *cb_arg),
    void *cb_arg)
{
	int i;
	bluetooth_device_t *bluetooth_device;

	if (fplug_device == NULL ||
	    foreach_cb == NULL) {
		errno = EINVAL;
		return 1;
	}

	for (i = 0; i < fplug_device->max_device; i++) {
                bluetooth_device = &fplug_device->bluetooth_device[i];
		if (bluetooth_device->connected) {
			foreach_cb(bluetooth_device->device_name, bluetooth_device->device_address, cb_arg);
                }
        }

	return 0;
}

int
fplug_device_stat_store_foreach(
    fplug_device_t *fplug_device,
    const char *device_address,
    struct tm *start_tm,
    struct tm *end_tm,
    void (*foreach_cb)(time_t stat_time, double temperature, unsigned int humidity, unsigned intilluminance, double rwatt, void *cb_arg),
    void *cb_arg) 
{
	int i;
	char addr[ADDRESS_MAX_LEN];
        bluetooth_device_t *bluetooth_device;

	if (fplug_device == NULL ||
	    device_address == NULL ||
	    start_tm == NULL ||
	    end_tm == NULL) {
		errno = EINVAL;
		return 1;
	}

	strlcpy(addr, device_address, sizeof(addr));
	for (i = 0; i < fplug_device->max_device; i++) {
                bluetooth_device = &fplug_device->bluetooth_device[i];
		if (bluetooth_device->connected && strcmp(bluetooth_device->device_address, addr) == 0) {
			if (stat_store_stat_foreach(bluetooth_device->stat_store, mktime(start_tm), mktime(end_tm), foreach_cb, cb_arg)) {
				LOG(LOG_ERR, "failed in stat store foreach"); 
				return 1;
			}
			return 0;
		}
        }
	LOG(LOG_ERR, "not found device address (%s)", addr); 

	return 1;
}

/* デバイスの初期化 */
int
fplug_device_reset(
    fplug_device_t *fplug_device,
    const char *device_address)
{
	bluetooth_device_t *bluetooth_device;
	time_t now;
	struct tm now_tm;
	struct {
		unsigned char hour;
		unsigned char min;
	}__attribute__((__packed__)) edt1; 
	struct {
		unsigned short year;
		unsigned char month;
		unsigned char day;
	}__attribute__((__packed__)) edt2;
	unsigned char *request_frame;
	size_t request_frame_len;
	unsigned short request_tid;
	unsigned char *response_buffer;
	size_t response_buffer_len;
	unsigned char esv;
	unsigned char opc;

	if (fplug_device == NULL ||
	    device_address == NULL) {
		errno = EINVAL;
		return 1;
	}
	if (fplug_device_get_bluetooth_device(fplug_device, device_address, &bluetooth_device)) {
		LOG(LOG_ERR, "not found bluetooth device"); 
		return 1;
	}

	now = time(NULL);
	if(localtime_r(&now, &now_tm) == NULL) {
		LOG(LOG_ERR, "failed in get local time");
		return 1;
	}
	edt1.hour = now_tm.tm_hour;
	edt1.min = now_tm.tm_min;
	edt2.year = now_tm.tm_year + 1900;
	edt2.month = now_tm.tm_mon + 1;
	edt2.day = now_tm.tm_mday;
	/* リクエストフレーム書き込み */
	if (enl_request_frame_init(&bluetooth_device->enl_request_frame_info, 0x0e, 0xf0, 0x00, 0x00, 0x22, 0x00, 0x61)) {
		LOG(LOG_ERR, "failed in initialize echonet lite frame of initialize");
		return 1;
	}
	if (enl_request_frame_add(&bluetooth_device->enl_request_frame_info, 0x97, 0x02, (unsigned char *)&edt1)) {
		LOG(LOG_ERR, "failed in add data (edt1) to echonet lite frame of initialize");
		return 1;
	}
	if (enl_request_frame_add(&bluetooth_device->enl_request_frame_info, 0x98, 0x04, (unsigned char *)&edt2)) {
		LOG(LOG_ERR, "failed in add data (edt2) to echonet lite frame of initialize");
		return 1;
	}
	if (enl_request_frame_get(&bluetooth_device->enl_request_frame_info, &request_frame, &request_frame_len, &request_tid)) {
		LOG(LOG_ERR, "failed in add data to echonet lite frame of initialize");
		return 1;
	}
	if (device_write_request(bluetooth_device->sd, request_frame, request_frame_len)) {
		LOG(LOG_ERR, "can not write echonet lite request frame of initialize");
		return 1;	
	}
	/* レスポンスフレームの読み出し */
	if (enl_response_frame_init(&bluetooth_device->enl_response_frame_info, &response_buffer, &response_buffer_len)) {
		LOG(LOG_ERR, "failed in initalize echonet lite frame of response");
		return 1;
	}
	if (device_read_response(bluetooth_device->sd, response_buffer, response_buffer_len)) {
		LOG(LOG_ERR, "failed in read response of initialize");
		return 1;
	}
	while (1) {
		if (enl_response_frame_add(&bluetooth_device->enl_response_frame_info, &response_buffer, &response_buffer_len)) {
			LOG(LOG_ERR, "failed in add data to echonet lite frame of response of initialize");
			return 1;
		}
		if (response_buffer == NULL) {
			break;
		}	
		if (device_read_response(bluetooth_device->sd, response_buffer, response_buffer_len)) {
			LOG(LOG_ERR, "failed in read response of initialize");
			return 1;
		}
	}
	/* レスポンスの確認 */
	enl_response_frame_get_esv(&bluetooth_device->enl_response_frame_info, &esv);
	if (esv == 0x51) {
		LOG(LOG_ERR, "nack response (initialize: service = %d)", esv);
		return 1;
	}
	enl_response_frame_get_opc(&bluetooth_device->enl_response_frame_info, &opc);
	LOG(LOG_DEBUG, "initalize: esv = %u, opc = %u", esv, opc);
	if (esv == 0x71) {
		/*
		 * device bug workaround
		 * 6 bytes longer than the response is written to specification
		 */ 
		LOG(LOG_DEBUG, "workaround read a byte");
		unsigned char dummy[6];
		if (device_read_response(bluetooth_device->sd, dummy, sizeof(dummy))) {
			LOG(LOG_ERR, "failed in read response");
			return 1;
		}
	}
	
	return 0;
}

/* 時刻設定 */
int
fplug_device_set_datetime(
    fplug_device_t *fplug_device,
    const char *device_address)
{
	bluetooth_device_t *bluetooth_device;

	if (fplug_device == NULL ||
	    device_address == NULL) {
		errno = EINVAL;
		return 1;
	}
	if (fplug_device_get_bluetooth_device(fplug_device, device_address, &bluetooth_device)) {
		LOG(LOG_ERR, "not found bluetooth device"); 
		return 1;
	}
	if (bluetooth_device_set_datetime(bluetooth_device)) {
		LOG(LOG_ERR, "failed in set datetime to bluetooth device"); 
	}

	return 0;
}

/* 24時間分の電力の積算値取得 */
int
fplug_device_hourly_power_total_foreach(
    fplug_device_t *fplug_device,
    const char *device_address,
    struct tm *start_tm,
    void (*foreach_cb)(double watt, unsigned char reliability, void *cb_arg),
    void *cb_arg)
{
	bluetooth_device_t *bluetooth_device;
	time_t now;
	struct tm now_tm;
	int past = 1;
	fplug_hourly_power_total_edata_t fplug_hourly_power_total_edata;
	unsigned char *request_frame;
	size_t request_frame_len;
	unsigned short request_tid;
	unsigned char *response_buffer;
	size_t response_buffer_len;
	unsigned char *response_edata_ptr;
	fplug_hourly_power_total_data_edt_t *fplug_hourly_power_total_data_edt;
	int i;

	if (fplug_device == NULL ||
	    device_address == NULL ||
	    foreach_cb == NULL) {
		errno = EINVAL;
		return 1;
	}
	if (fplug_device_get_bluetooth_device(fplug_device, device_address, &bluetooth_device)) {
		LOG(LOG_ERR, "not found bluetooth device"); 
		return 1;
	}

	/* start_tmがNULLなら現在時刻としつつ、初期化する方使う */
	if (start_tm == NULL) {
		now = time(NULL);
		if(localtime_r(&now, &now_tm) == NULL) {
			LOG(LOG_ERR, "failed in get local time");
			return 1;
		}
		start_tm = &now_tm;
		past = 0;
	}
	if (past) {
		fplug_hourly_power_total_edata.req_type = 0x16;
	} else {
		fplug_hourly_power_total_edata.req_type = 0x11;
	}
	fplug_hourly_power_total_edata.hour = start_tm->tm_hour;
	fplug_hourly_power_total_edata.min = start_tm->tm_min;
	fplug_hourly_power_total_edata.year = start_tm->tm_year + 1900;
	fplug_hourly_power_total_edata.month = start_tm->tm_mon + 1;
	fplug_hourly_power_total_edata.day = start_tm->tm_mday;
	/* リクエストフレーム書き込み */
	if (enl_request_any_frame_init(&bluetooth_device->enl_request_any_frame_info, (unsigned char *)&fplug_hourly_power_total_edata, sizeof(fplug_hourly_power_total_edata_t))) {
		LOG(LOG_ERR, "failed in initialize echonet lite frame of hourly power");
		return 1;
	}
	if (enl_request_any_frame_get(&bluetooth_device->enl_request_any_frame_info, &request_frame, &request_frame_len, &request_tid)) {
		LOG(LOG_ERR, "failed in add data to echonet lite frame of hourly power");
		return 1;
	}
	if (device_write_request(bluetooth_device->sd, request_frame, request_frame_len)) {
		LOG(LOG_ERR, "can not write echonet lite request frame of hourly power");
		return 1;	
	}
	/* レスポンスフレームの読み出し */
	if (enl_response_any_frame_init(&bluetooth_device->enl_response_any_frame_info, 74, &response_buffer, &response_buffer_len)) {
		LOG(LOG_ERR, "failed in initalize echonet lite frame of response of hourly power");
		return 1;
	}
	if (device_read_response(bluetooth_device->sd, response_buffer, response_buffer_len)) {
		LOG(LOG_ERR, "failed in read response of hourly power");
		return 1;
	}
	/* レスポンスの取得 */
	enl_response_any_frame_get_edata(&bluetooth_device->enl_response_any_frame_info, &response_edata_ptr, NULL);
	/* レスポンスチェック */
	if (past) {
		if (*response_edata_ptr != 0x96) {
			LOG(LOG_ERR, "invalid response hourly power total (%d)", *response_edata_ptr);
			return 1;
		}
	} else {
		if (*response_edata_ptr != 0x91) {
			LOG(LOG_ERR, "invalid response hourly power total (%d)", *response_edata_ptr);
			return 1;
		}
	}
	if (*(response_edata_ptr + 1) == 0x01) {
		LOG(LOG_ERR, "failed in get hourly other data");
		return 1;
	}
	fplug_hourly_power_total_data_edt = (fplug_hourly_power_total_data_edt_t *)(response_edata_ptr + 2);
	for (i = 0; i < 24; i++) {
		foreach_cb(((int)fplug_hourly_power_total_data_edt->watt)/10, fplug_hourly_power_total_data_edt->reliability, cb_arg);
		fplug_hourly_power_total_data_edt++;
	}
	
	return 0;
}

/* 24時間分の温度、湿度、照度を取得 */
int
fplug_device_hourly_other_foreach(
    fplug_device_t *fplug_device,
    const char *device_address,
    struct tm *start_tm,
    void (*foreach_cb)(double temperature, unsigned int humidity, unsigned int illuminance, void *cb_arg),
    void *cb_arg)
{
	bluetooth_device_t *bluetooth_device;
	fplug_hourly_other_edata_t fplug_hourly_other_edata;
	unsigned char *request_frame;
	size_t request_frame_len;
	unsigned short request_tid;
	unsigned char *response_buffer;
	size_t response_buffer_len;
	unsigned char *response_edata_ptr;
	fplug_hourly_other_data_edt_t *fplug_hourly_other_data_edt;
	int i;

	if (fplug_device == NULL ||
	    device_address == NULL ||
	    foreach_cb == NULL) {
		errno = EINVAL;
		return 1;
	}
	if (fplug_device_get_bluetooth_device(fplug_device, device_address, &bluetooth_device)) {
		LOG(LOG_ERR, "not found bluetooth device"); 
		return 1;
	}

	/* start_tmがNULLなら現在時刻としつつ、初期化する方使う */
	fplug_hourly_other_edata.req_type = 0x17;
	fplug_hourly_other_edata.hour = start_tm->tm_hour;
	fplug_hourly_other_edata.min = start_tm->tm_min;
	fplug_hourly_other_edata.year = start_tm->tm_year + 1900;
	fplug_hourly_other_edata.month = start_tm->tm_mon + 1;
	fplug_hourly_other_edata.day = start_tm->tm_mday;
	/* リクエストフレーム書き込み */
	if (enl_request_any_frame_init(&bluetooth_device->enl_request_any_frame_info, (unsigned char *)&fplug_hourly_other_edata, sizeof(fplug_hourly_other_edata_t))) {
		LOG(LOG_ERR, "failed in initialize echonet lite frame of of hourly other");
		return 1;
	}
	if (enl_request_any_frame_get(&bluetooth_device->enl_request_any_frame_info, &request_frame, &request_frame_len, &request_tid)) {
		LOG(LOG_ERR, "failed in add data to echonet lite frame of of hourly other");
		return 1;
	}
	if (device_write_request(bluetooth_device->sd, request_frame, request_frame_len)) {
		LOG(LOG_ERR, "can not write echonet lite request frame of hourly other");
		return 1;	
	}
	/* レスポンスフレームの読み出し */
	if (enl_response_any_frame_init(&bluetooth_device->enl_response_any_frame_info, 122, &response_buffer, &response_buffer_len)) {
		LOG(LOG_ERR, "failed in initalize echonet lite frame of response of hourly other");
		return 1;
	}
	if (device_read_response(bluetooth_device->sd, response_buffer, response_buffer_len)) {
		LOG(LOG_ERR, "failed in read response of hourly other");
		return 1;
	}
	/* レスポンスの取得 */
	enl_response_any_frame_get_edata(&bluetooth_device->enl_response_any_frame_info, &response_edata_ptr, NULL);
	/* レスポンスチェック */
	if (*response_edata_ptr != 0x97) {
		LOG(LOG_ERR, "invalid response hourly other (%d)", *response_edata_ptr);
		return 1;
	}
	if (*(response_edata_ptr + 1) == 0x01) {
		LOG(LOG_ERR, "failed in get hourly other data");
		return 1;
	}
	fplug_hourly_other_data_edt = (fplug_hourly_other_data_edt_t *)(response_edata_ptr + 2);
	for (i = 0; i < 24; i++) {
		foreach_cb(((int)fplug_hourly_other_data_edt->temperature)/10, fplug_hourly_other_data_edt->humidity, fplug_hourly_other_data_edt->illuminance, cb_arg);
		fplug_hourly_other_data_edt++;
	}
	
	return 0;
}

static int
bluetooth_device_set_datetime(
    bluetooth_device_t *bluetooth_device)
{
	time_t now;
	struct tm now_tm;
	fplug_set_datetime_request_frame_t set_datetime_request_frame;
	fplug_set_datetime_response_frame_t set_datetime_response_frame;

	ASSERT(bluetooth_device != NULL);
	now = time(NULL);
	if(localtime_r(&now, &now_tm) == NULL) {
		LOG(LOG_ERR, "failed in get local time");
		return 1;
	}
	// リクエストフレーム作成
	set_datetime_request_frame.req_type = 0x07;
	set_datetime_request_frame.hour = now_tm.tm_hour;
	set_datetime_request_frame.min = now_tm.tm_min;
	set_datetime_request_frame.year = now_tm.tm_year + 1900;
	set_datetime_request_frame.month = now_tm.tm_mon + 1;
	set_datetime_request_frame.day = now_tm.tm_mday;
	// リクエストの書き込み
	if (device_write_request(bluetooth_device->sd, (unsigned char *)&set_datetime_request_frame, sizeof(fplug_set_datetime_request_frame_t))) {
		LOG(LOG_ERR, "can not write set datetime request frame of set datetime");
		return 1;
	}
	// レスポンスの読み込み
	if (device_read_response(bluetooth_device->sd, (unsigned char *)&set_datetime_response_frame, sizeof(fplug_set_datetime_response_frame_t))) {
		LOG(LOG_ERR, "failed in read response of set datetime");
		return 1;
	}
	// レスポンスチェック
	if (set_datetime_response_frame.result == 0x01) {
		LOG(LOG_ERR, "failed in set datetime");
		return 1;
	}
	
	return 0;
}

static int
fplug_device_get_bluetooth_device(
    fplug_device_t *fplug_device,
    const char *device_address,
    bluetooth_device_t **bluetooth_device)
{
        int i;
        char addr[ADDRESS_MAX_LEN];
	bluetooth_device_t *btdev;

	ASSERT(fplug_device != NULL);
	ASSERT(device_address != NULL);
	ASSERT(bluetooth_device != NULL);

        strlcpy(addr, device_address, sizeof(addr));
        for (i = 0; i < fplug_device->max_device; i++) {
                btdev = &fplug_device->bluetooth_device[i];
                if (btdev->connected && strcmp(btdev->device_address, addr) == 0) {
                        *bluetooth_device = btdev;
                        return 0;
                }
        }
        LOG(LOG_ERR, "not found device address (%s)", addr);

        return 1;
}

static int
connect_bluetooth_device(
    bluetooth_device_t *bluetooth_device)
{
	ASSERT(bluetooth_device != NULL);

        bluetooth_device->sd = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
        bluetooth_device->saddr.rc_family = AF_BLUETOOTH;
        bluetooth_device->saddr.rc_channel = (uint8_t)1;
        str2ba(bluetooth_device->device_address, &bluetooth_device->saddr.rc_bdaddr);
        if (connect(bluetooth_device->sd, (struct sockaddr *)&bluetooth_device->saddr, sizeof(bluetooth_device->saddr)) < 0) {
                LOG(LOG_ERR, "can not connect bluetooth (%s)\n", strerror(errno));
                goto error;
        }
        bluetooth_device->connected = 1;

        return 0;

error:
        if (bluetooth_device->sd != -1) {
                close(bluetooth_device->sd);
                bluetooth_device->sd = -1;
        }

        return 1;
}

static void
close_bluetooth_device(
    bluetooth_device_t *bluetooth_device)
{
	ASSERT(bluetooth_device != NULL);

        if (!bluetooth_device->connected) {
                return;
        }
        if (bluetooth_device->sd != -1) {
                close(bluetooth_device->sd);
                bluetooth_device->sd = -1;
        	bluetooth_device->connected = 0;
        }
}

static void
fplug_device_polling(
    evutil_socket_t fd,
    short events,
    void *arg)
{
	fplug_device_t *fplug_device = arg;
	bluetooth_device_t *bluetooth_device;
	int i;

	ASSERT(arg != NULL);

	LOG(LOG_DEBUG, "polling");

	if (evtimer_add(fplug_device->polling_event, &fplug_device->polling_interval)) {
                LOG(LOG_ERR, "faile in create fplug device");
	}

	for (i = 0; i < fplug_device->max_device; i++) {
                bluetooth_device = &fplug_device->bluetooth_device[i];
		if (bluetooth_device->connected) {
			if (bluetooth_device_realtime_stat(bluetooth_device)) {
				LOG(LOG_DEBUG, "failed in get realtime statistics");
			}
                }
        }
}

static int
bluetooth_device_realtime_stat(
    bluetooth_device_t *bluetooth_device)
{
	int error = 0;
	time_t now;
        double temperature = 0;
        unsigned int humidity = 0;
        unsigned int illuminance = 0;
        double rwatt = 0;
	stat_type_t stat_types[] = { TEMPERATURE, HUMIDITY, ILLUMINANCE, RWATT };
	int i;
	unsigned char esv;
	unsigned char opc;
	unsigned char epc;
	unsigned char pdc;
	unsigned char *edt_ptr;
	unsigned short v;
	
	ASSERT(bluetooth_device != NULL);

	now = time(NULL);
	for (i = 0; i < NELEMS(stat_types); i++) {
		// リクエストの書き込み
		if (bluetooth_device_write_request_frame(bluetooth_device, stat_types[i])) {
			LOG(LOG_ERR, "failed in make echonet lite request frame (%d)", i);
			error++;
			continue;
		}
		// 応答の読み込み
		if (bluetooth_device_read_response_frame(bluetooth_device, stat_types[i])) {
			LOG(LOG_ERR, "failed in make echonet lite response frame (%d)", stat_types[i]);
			error++;
			continue;
		}
		// 応答のチェック
		enl_response_frame_get_esv(&bluetooth_device->enl_response_frame_info, &esv);
		if (esv == 0x52) {
			LOG(LOG_ERR, "nack response (%d: service = %u)", stat_types[i], esv);
			error++;
			continue;
		}
		enl_response_frame_get_opc(&bluetooth_device->enl_response_frame_info, &opc);
		if (opc == 0) {
			LOG(LOG_ERR, "no data (%d: count = %u)", stat_types[i], opc);
			error++;
			continue;
		}
		LOG(LOG_DEBUG, "type %d: esv = %u, opc = %u", stat_types[i], esv, opc);
		enl_response_frame_get_data(&bluetooth_device->enl_response_frame_info, 1, &epc, &pdc, &edt_ptr);
		switch (stat_types[i]) {
		case TEMPERATURE:
			if (pdc < 2) {
				LOG(LOG_ERR, "unkown value length (%d: len = %d)", stat_types[i], pdc);
				error++;
				continue;
			}
			v = *((unsigned short *)edt_ptr);
			temperature = ((int)v)/10;
			break;
		case HUMIDITY:
			if (pdc < 1) {
				LOG(LOG_ERR, "unkown value length (%d: len = %d)", stat_types[i], pdc);
				error++;
				continue;
			}
			humidity = *edt_ptr;
			break;
		case ILLUMINANCE:
			if (pdc < 2) {
				LOG(LOG_ERR, "unkown value length (%d: len = %d)", stat_types[i], pdc);
				error++;
				continue;
			}
			illuminance = *((unsigned short *)edt_ptr);
			break;
		case RWATT:
			if (pdc < 2) {
				LOG(LOG_ERR, "unkown value length (%d: len = %d)", stat_types[i], pdc);
				error++;
				continue;
			}
			v = *((unsigned short *)edt_ptr);
			rwatt = ((int)v)/10;
			break;
		default:
			ABORT("not reached");
			/* NOT REACHED */
		}
	}
	LOG(LOG_DEBUG, "stat: temperature = %llf, humidity = %u, illuminance = %u, rwatt = %llf", temperature, humidity, illuminance, rwatt);
	if (stat_store_stat_add(bluetooth_device->stat_store, now, temperature, humidity, illuminance, rwatt)) {
		LOG(LOG_ERR, "failed in add storage of stat");
		error += 256;
	}

	return error;
}

static int
bluetooth_device_write_request_frame(
    bluetooth_device_t *bluetooth_device,
    int type)
{
	unsigned char *request_frame;
	size_t request_frame_len;
	unsigned short request_tid;

	// フレームの作成
	switch (type) {
	case TEMPERATURE:
		if (enl_request_frame_init(&bluetooth_device->enl_request_frame_info, 0x0e, 0xf0, 0x00, 0x00, 0x11, 0x00, 0x62)) {
			LOG(LOG_ERR, "failed in initialize echonet lite frame of temprature");
			return 1;
		}
		if (enl_request_frame_add(&bluetooth_device->enl_request_frame_info, 0xe0, 0x00, NULL)) {
			LOG(LOG_ERR, "failed in add data to echonet lite frame of temprature");
			return 1;
		}
		if (enl_request_frame_get(&bluetooth_device->enl_request_frame_info, &request_frame, &request_frame_len, &request_tid)) {
			LOG(LOG_ERR, "failed in add data to echonet lite frame of temprature");
			return 1;
		}
		break;
	case HUMIDITY:
		if (enl_request_frame_init(&bluetooth_device->enl_request_frame_info, 0x0e, 0xf0, 0x00, 0x00, 0x12, 0x00, 0x62)) {
			LOG(LOG_ERR, "failed in initialize echonet lite frame of temprature");
			return 1;
		}
		if (enl_request_frame_add(&bluetooth_device->enl_request_frame_info, 0xe0, 0x00, NULL)) {
			LOG(LOG_ERR, "failed in add data to echonet lite frame of temprature");
			return 1;
		}
		if (enl_request_frame_get(&bluetooth_device->enl_request_frame_info, &request_frame, &request_frame_len, &request_tid)) {
			LOG(LOG_ERR, "failed in add data to echonet lite frame of temprature");
			return 1;
		}
		break;
	case ILLUMINANCE:
		if (enl_request_frame_init(&bluetooth_device->enl_request_frame_info, 0x0e, 0xf0, 0x00, 0x00, 0x0d, 0x00, 0x62)) {
			LOG(LOG_ERR, "failed in initialize echonet lite frame of temprature");
			return 1;
		}
		if (enl_request_frame_add(&bluetooth_device->enl_request_frame_info, 0xe0, 0x00, NULL)) {
			LOG(LOG_ERR, "failed in add data to echonet lite frame of temprature");
			return 1;
		}
		if (enl_request_frame_get(&bluetooth_device->enl_request_frame_info, &request_frame, &request_frame_len, &request_tid)) {
			LOG(LOG_ERR, "failed in add data to echonet lite frame of temprature");
			return 1;
		}
		break;
	case RWATT:
		if (enl_request_frame_init(&bluetooth_device->enl_request_frame_info, 0x0e, 0xf0, 0x00, 0x00, 0x22, 0x00, 0x62)) {
			LOG(LOG_ERR, "failed in initialize echonet lite frame of temprature");
			return 1;
		}
		if (enl_request_frame_add(&bluetooth_device->enl_request_frame_info, 0xe2, 0x00, NULL)) {
			LOG(LOG_ERR, "failed in add data to echonet lite frame of temprature");
			return 1;
		}
		if (enl_request_frame_get(&bluetooth_device->enl_request_frame_info, &request_frame, &request_frame_len, &request_tid)) {
			LOG(LOG_ERR, "failed in add data to echonet lite frame of temprature");
			return 1;
		}
		break;
	default:
		ABORT("not reached");
		/* NOT REACHED */
	}
	// フレームを書き込む
	if (device_write_request(bluetooth_device->sd, request_frame, request_frame_len)) {
		LOG(LOG_ERR, "can not write echonet lite request frame (%d)", type);
		return 1;	
	}

	return 0;
}

static int
bluetooth_device_read_response_frame(
    bluetooth_device_t *bluetooth_device,
    int type)
{
	unsigned char *response_buffer;
	size_t response_buffer_len;

	if (enl_response_frame_init(&bluetooth_device->enl_response_frame_info, &response_buffer, &response_buffer_len)) {
		LOG(LOG_ERR, "failed in initalize echonet lite frame of response");
		return 1;
	}
	if (device_read_response(bluetooth_device->sd, response_buffer, response_buffer_len)) {
		LOG(LOG_ERR, "failed in read response");
		return 1;
	}
	while (1) {
		if (enl_response_frame_add(&bluetooth_device->enl_response_frame_info, &response_buffer, &response_buffer_len)) {
			LOG(LOG_ERR, "failed in add data to echonet lite frame of response");
			return 1;
		}
		if (response_buffer == NULL) {
			break;
		}	
		if (device_read_response(bluetooth_device->sd, response_buffer, response_buffer_len)) {
			LOG(LOG_ERR, "failed in read response");
			return 1;
		}
	}
	if (type == HUMIDITY) {
		/*
		 * device bug workaround
		 * pdc == 1, but response two byte
		 */ 
		LOG(LOG_DEBUG, "workaround read a byte");
		unsigned char dummy;
		if (device_read_response(bluetooth_device->sd, &dummy, 1)) {
			LOG(LOG_ERR, "failed in read response");
			return 1;
		}
	}

	return 0;
}

static int
device_write_request(
    int sd,
    unsigned char *frame,
    size_t frame_len)
{
        size_t wlen = 0;
	int tmp_wlen;

	ASSERT(frame != NULL);

	if (frame_len == 0) {
		return 0;
	}
	LOG_DUMP(LOG_DEBUG, frame, frame_len);
	wlen = 0;
	while (wlen != frame_len) {
		LOG(LOG_DEBUG, "trace: start write");
		tmp_wlen = write(sd, &frame[wlen], frame_len - wlen);
		LOG(LOG_DEBUG, "trace: end write");
		if (tmp_wlen < 0) {
			LOG(LOG_ERR, "can not write echonet lite request frame");
			return 1;
		}
		wlen += tmp_wlen;
	}
	/* debug */

        return 0;
}

static int
device_read_response(
    int sd,
    unsigned char *buffer,
    size_t buffer_len)
{
        size_t rlen = 0;
	int tmp_rlen;

	ASSERT(buffer != NULL);

	if (buffer_len == 0) {
		return 0;
	}
	rlen = 0;
	while (rlen != buffer_len) {
		LOG(LOG_DEBUG, "trace: start read");
		tmp_rlen = read(sd, &buffer[rlen], buffer_len - rlen);
		LOG(LOG_DEBUG, "trace: end read");
		if (tmp_rlen < 0) {
			LOG(LOG_ERR, "can not read echonet lite response");
			return 1;
		}
		rlen += tmp_rlen;
	}
	LOG_DUMP(LOG_DEBUG, buffer, buffer_len);

        return 0;
}

