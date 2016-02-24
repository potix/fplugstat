#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include "common_macros.h"
#include "common_define.h"
#include "config.h"
#include "fplug_device.h"
#include "stat_store.h"

#define DEFAULT_MAX_DEVICE 3
#define DEFAULT_POLLING_INTERVAL 5

#define NAME_MAX_LEN 64
#define ADDRESS_MAX_LEN 32

struct bluetooth_device {
        char device_name[NAME_MAX_LEN];
        char device_address[ADDRESS_MAX_LEN];
        struct sockaddr_rc saddr;
        int sd;
        int connected;
        stat_store_t *stat_store;
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

static int connect_bluetooth_device(bluetooth_device_t *bluetooth_device);
static void close_bluetooth_device(bluetooth_device_t *bluetooth_device);

int
fplug_device_create(
    fplug_device_t **fplug_device,
    config_t *config,
    struct event_base *event_base) {
	fplug_device_t *new = NULL;
	bluetooth_device_t *bluetooth_device_new = NULL;
	int i;
        char dev_section[CONFIG_LINE_BUF];

	if (fplug_device == NULL ||
	    config == NULL ||
	    event_base == NULL) {
		return EINVAL;
	}

	new = malloc(sizeof(fplug_device_t));
	if (new == NULL) {
                LOG(LOG_ERR, "faile in create fplug device");
		goto fail;
	}
	memset(fplug_devicies, 0, sizeof(fplug_devicies_t));
	new->event_base = event_base;
	new->polling_event = evtimer_new(event_base, fplug_device_polling, new)
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

	bluetooth_device_new = malloc(sizeof(fplug_device_t) * new->max_device);
	if (bluetooth_device_new = NULL) {
                LOG(LOG_ERR, "faile in create bluetooth device");
		goto fail;
	}
	memset(bluetooth_device_new, 0, sizeof(fplug_device_t) * new->max_device);
	new->bluetooth_device = bluetooth_device_new;
 
	for (i = 0; i < new->max_device; i++) {
		bluetooth_device_t *btdev = new->bluetooth_device[i];
		btdev->sd = -1;
		snprintf(dev_section, sizeof(dev_section), "%s%d", "device", i + 1);
		if (config_get_string(config, btdev->device_name, sizeof(btdev->device_name), dev_section, "name",  NULL, sizeof(btdev->device_name) - 1)) {
			continue;
		}
                if (config_get_string(config, btdev->device_address, sizeof(btdev->device_address), dev_section, "address",  NULL, sizeof(device->device_address) - 1)) {
                        continue;
                }
		if (stat_store_create(&btdev->stat_store, config)) {
                	LOG(LOG_ERR, "faile in create stat store");
			goto fail;
		} 
        }
	*fplug_device = new;

	return 0;

fail:

	if (bluetooth_device_new) {
		for (i = 0; i < new->max_device; i++) {
			bluetooth_device_t *btdev = bluetooth_device_new[i];
			if (btdev->stat_store) {
				stat_store_destroy(btdev->stat_store);
				btdev->stat_store = NULL;
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
		return EINVAL;
	}
        for (i = 0; i < fplug_device->max_device; i++) {
		if (fplug_device->bluetooth_device[i].device_name[0] != '\0' &&
		    fplug_device->bluetooth_device[i].device_address[0] != '\0') {
			if (connect_bluetooth_device(&fplug_device->bluetooth_device[i])) {
				continue;
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
		return EINVAL;
	}
	if (evtimer_add(fplug_device->polling_event, &fplug_device.polling_tv)) {
                LOG(LOG_ERR, "faile in create fplug device");
	}

	return 0;
}

int
fplug_device_polling_stop(
    fplug_device_t *fplug_device) {

	if (fplug_device == NULL) {
		return EINVAL;
	}

	evtimer_del(fplug_device->polling_event);

	return 0;
}

int
fplug_device_destroy(
    fplug_device_t *fplug_device)
{
	int i;

	if (fplug_device == NULL) {
		return EINVAL;
	}
        for (i = 0; i < fplug_device->max_device; i++) {
		bluetooth_device_t *btdev = new->bluetooth_device[i];
		close_bluetooth_device(btdev);
		if (btdev->stat_store) {
			stat_store_destroy(btdev->stat_store);
			btdev->stat_store = NULL;
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
    void (*foreach_cb)(char *device_name, char *device_address, void *cb_arg),
    void *cb_arg)
{

	if (fplug_device == NULL ||
	    foreach_cb == NULL) {
		return EINVAL;
	}

	for (i = 0; i < fplug_device->max_device; i++) {
                bluetooth_device_t *btdev = new->bluetooth_device[i];
		if (btdev->connected) {
			foreach_cb(btdev->device_name, btdev->device_address, cb_arg);
                }
        }

	return 0;
}

int
fplug_device_get_stat_store(
    fplug_device_t *fplug_device,
    stat_store_t **stat_store,
    char *device_address)
{
	int addr[ADDRESS_MAX_LEN];

	if (fplug_device == NULL ||
	    stat_store == NULL ||
	    device_address == NULL) {
		return EINVAL;
	}

	strlcpy(addr, device_address, sizeof(addr));
	for (i = 0; i < fplug_device->max_device; i++) {
                bluetooth_device_t *btdev = new->bluetooth_device[i];
		if (strcmp(btdev->device_address, addr) == 0) {
			*stat_store = btdev->stat_store;
			return 0;
		}
        }
	LOG(LOG_ERR, "not found device (%s)", addr); 

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
