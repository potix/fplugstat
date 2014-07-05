#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// http://people.csail.mit.edu/albert/bluez-intro/x502.html
// http://code.metager.de/source/xref/linux/bluetooth/bluez/tools/rfcomm.c

#define MAX_FPLUG_DEVICE 7

struct fplug_device {
	const char *device_address;
        struct sockaddr_rc sddr;
	int sd;  
	int connected;
	struct termios oldtio, newtio;
};
typedef struct fplug_device fplug_device_t;

struct fplugd {
	int check_status;
	int foreground;
	fplug_device_t fplugd_device[MAX_FPLUG_DEVICE];
	int device_count;
};
typedef struct fplugd fplugd_t;

void
initialize_fplugd(fplugd_t *fplugd) {
	int i;

	memset(fplugd, 0, sizeof(fplugd_t));
	fplugd->device = DEFAULT_SERIAL_PORT;
	for (i = 0; i < MAX_FPLUG_DEVICE; i++) {
		fplugd->fplug_device[i].sd = -1;
	}
}

static int
parse_command_arguments(int argc, char *argv[], fplugd_t *fplugd) {
	while ((opt = getopt(argc, argv, "d:thiwF")) != -1) {
		switch (opt) {
		case 'd':
			fplugd->fplug_device[fplugd->device_count].device_address = optarg;
			fplugd->device_count += 1;
			break;
		case 't':
			fplugd->check_status |= 0x01;
                   	break;
		case 'h':
			fplugd->check_status |= 0x02;
                   	break;
		case 'i':
			fplugd->check_status |= 0x04;
                   	break;
		case 'w':
			fplugd->check_status |= 0x08;
                   	break;
		case 'F':
			fplugd->foreground = 1;
			break;
		default:
			fprintf(stderr, "Usage: %s [[-d <open_device>]...] [-thiw]\n", argv[0]);
			return 1;
		}
	}
	if (fplugd->check_status == 0) {
		fprintf(stderr, "Usage: %s [[-d <open_device>]...] [-thiw]\n", argv[0]);
		return 1;
	}

	return 0;
}

static int
connect_bluetooth(fplug_device_t *fplug_device)
{
	int s, status;

	fplug_device->sd = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	fplug_device->saddr.rc_family = AF_BLUETOOTH;
	fplug_device->saddr.rc_channel = (uint8_t)1;
	str2ba(fplug_device->device_address, &fplug_device->saddr.rc_bdaddr);
	if (connect(fplug_device->sd, (struct sockkaddr *)&fplug_device->saddr, sizeof(fplug_device->saddr)) < 0) {
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
close_bluetooth(fplug_device_t *fplug_device)
{
	fplug_device->connected = 0;
	if (fplug_device->sd != -1) {
		close(fplug_device->sd);
		fplug_device->sd = -1;
	}
	
}

int
main(int argc, char *argv[])
{
	struct fplugd_t fplugd;
	int opt;
	struct termios oldtio, newtio;
	int i;

	initialize_fplugd(&fplugd);
	if (parse_command_arguments(argc, argv, &fplugd)) {
		return 1;
	}
        if (!fplugd.foreground) {
                if (daemon(1,1)) {
                        fprintf(stderr, "faile in daemonaize");
                        return 1;
                }
                setsid();
        }
	for (i = 0: i < fplugd->device_count; i++) {
		if (connect_bluetooth(&fplugd->fplug_device[i])) {
			return 1;
		}
	}

	// XXX
	// XXX
	// XXX

	for (i = 0: i < fplugd->device_count; i++) {
		close_bluetooth(&fplugd->fplug_device[i]);
	}

	return 0;
}

