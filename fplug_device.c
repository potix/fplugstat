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
#include "fplug_device.h"

static int connect_bluetooth_device(fplug_device_t *fplug_device);
static void close_bluetooth_device(fplug_device_t *fplug_device);

int
initialize_fplug_devicies(
    fplug_devicies_t *fplug_devicies) {
	int i;

	if (fplug_devicies == NULL) {
		return EINVAL;
	}

	memset(fplug_devicies, 0, sizeof(fplug_devicies_t));
        for (i = 0; i < MAX_FPLUG_DEVICE; i++) {
                fplug_devicies->fplug_device[i].sd = -1;
        }

	return 0;
}

int
connect_bluetooth_devicies(
    fplug_devicies_t *fplug_devicies) {
	int i;

	if (fplug_devicies == NULL) {
		return EINVAL;
	}
        for (i = 0; i < MAX_FPLUG_DEVICE; i++) {
		if (fplug_devicies->fplug_device[i].device_name[0] != '\0' &&
		    fplug_devicies->fplug_device[i].device_address[0] != '\0') {
			if (connect_bluetooth_device(&fplug_devicies->fplug_device[i])) {
				continue;
			}
			fplug_devicies->available_count++;
		}
        }
	if (fplug_devicies->available_count == 0) {
                fprintf(stderr, "no connected bluetooth devicies\n");
		return 1;
	}

	return 0;
}

int
close_bluetooth_devicies(
    fplug_devicies_t *fplug_devicies)
{
	int i;

	if (fplug_devicies == NULL) {
		return EINVAL;
	}
        for (i = 0; i < MAX_FPLUG_DEVICE; i++) {
		close_bluetooth_device(&fplug_devicies->fplug_device[i]);
        }

	return 0;
}

static int
connect_bluetooth_device(
    fplug_device_t *fplug_device)
{
        fplug_device->sd = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
        fplug_device->saddr.rc_family = AF_BLUETOOTH;
        fplug_device->saddr.rc_channel = (uint8_t)1;
        str2ba(fplug_device->device_address, &fplug_device->saddr.rc_bdaddr);
        if (connect(fplug_device->sd, (struct sockaddr *)&fplug_device->saddr, sizeof(fplug_device->saddr)) < 0) {
                fprintf(stderr, "can not connect bluetooth (%s)\n", strerror(errno));
                goto error;
        }
        fplug_device->connected = 1;

        return 0;

error:
        if (fplug_device->sd != -1) {
                close(fplug_device->sd);
                fplug_device->sd = -1;
        }

        return 1;
}

static void
close_bluetooth_device(
    fplug_device_t *fplug_device)
{
        if (!fplug_device->connected) {
                return;
        }
        fplug_device->connected = 0;
        if (fplug_device->sd != -1) {
                close(fplug_device->sd);
                fplug_device->sd = -1;
        }
}
