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

void
initialize_fplug_devicies(
    fplug_devicies_t *fplug_devicies) {
	int i;

	memset(fplug_devicies, 0, sizeof(fplug_devicies_t));
        for (i = 0; i < MAX_FPLUG_DEVICE; i++) {
                fplug_devicies->fplug_device[i].sd = -1;
        }
}

int
connect_bluetooth(
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

void
close_bluetooth(
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
