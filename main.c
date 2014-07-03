#include <stdlib.h>
#include <stdio.h>
#include <string.h>


http://people.csail.mit.edu/albert/bluez-intro/x502.html
http://code.metager.de/source/xref/linux/bluetooth/bluez/tools/rfcomm.c

#define MAX_FPLUG_DEVICE 7

struct fplug_device {
	const char *device;
	struct termios oldtio, newtio;
	int fd;  
};
typedef struct fplug_device fplug_device_t;

struct fplugd {
	int check_status;
	int foreground;
	fplug_device_t fplugd_device[MAX_FPLUG_DEVICE];
	int device_count;
	int available_device_count;
};
typedef struct fplugd fplugd_t;


void
initialize_fplugd(fplugd_t *fplugd) {
	memset(fplugd, 0, sizeof(fplugd_t));
	fplugd->device = DEFAULT_SERIAL_PORT;
}

static int
parse_command_arguments(int argc, char *argv[], fplugd_t *fplugd) {
	while ((opt = getopt(argc, argv, "b:thiwF")) != -1) {
		switch (opt) {
		case 'b':
			fplugd->fplug_device[fplugd->device_count] =optarg;
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


int
main(int argc, char *argv[])
{

	struct fplugd_t fplugd;

	int opt;
	struct termios oldtio, newtio;

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

	return 0;
}

