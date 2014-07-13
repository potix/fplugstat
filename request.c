#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <event.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#define MAX_FPLUG_DEVICE 7

/* F-PLUG request */
const char Temp[]={0x10, 0x81, 0x00, 0x00, 0x0e, 0xf0, 0x00, 0x00, 0x11, 0x00, 0x62, 0x01, 0xe0, 0x00};
const char Humid[]={0x10, 0x81, 0x00, 0x00, 0x0e, 0xf0, 0x00, 0x00, 0x12, 0x00, 0x62, 0x01, 0xe0, 0x00};
const char Illum[]={0x10, 0x81, 0x00, 0x00, 0x0e, 0xf0, 0x00, 0x00, 0x0D, 0x00, 0x62, 0x01, 0xe0, 0x00};
const char RWatt[]={0x10, 0x81, 0x00, 0x00, 0x0e, 0xf0, 0x00, 0x00, 0x22, 0x00, 0x62, 0x01, 0xe2, 0x00};

struct fplug_device {
	const char *device_address;
        struct sockaddr_rc saddr;
	int sd;  
	int connected;
};
typedef struct fplug_device fplug_device_t;

struct fplugd {
	int check_status;
	int foreground;
	fplug_device_t fplug_device[MAX_FPLUG_DEVICE];
	int device_count;
	int available_count;
        struct event_base *event_base;
        sigset_t sigmask;
        struct event sigterm_event;
        struct event sigint_event;
	struct timeval timer_tv;
        struct event timer_event; // test
};
typedef struct fplugd fplugd_t;

void
initialize_fplugd(
    fplugd_t *fplugd)
{
	int i;

	memset(fplugd, 0, sizeof(fplugd_t));
	for (i = 0; i < MAX_FPLUG_DEVICE; i++) {
		fplugd->fplug_device[i].sd = -1;
	}
}

static int
parse_command_arguments(
    int argc,
    char *argv[],
    fplugd_t *fplugd)
{
	int opt;

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
			fprintf(stderr, "Usage: %s [[-d <device address>]...] [-thiw]\n", argv[0]);
			return 1;
		}
	}
	if (fplugd->check_status == 0) {
		fprintf(stderr, "Usage: %s [[-d <device address>]...] [-thiw]\n", argv[0]);
		return 1;
	}

	return 0;
}

static void
terminate(
    int fd,
    short event,
    void *args)
{
        fplugd_t *fplugd = args;

        event_del(&fplugd->sigterm_event);
        event_del(&fplugd->sigint_event);
        event_del(&fplugd->timer_event);
}

static int
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

static void
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

static int
do_request(int fd, const char *request, size_t request_size, unsigned char *response, size_t response_size)
{
	int wlen = 0, tmp_wlen;
	int rlen = 0, tmp_rlen;

	while (wlen != request_size) {
		tmp_wlen = write(fd, &request[wlen], request_size - wlen);
		if (tmp_wlen < 0) {
			fprintf(stderr, "can not write request\n");
			return 1;
		}
		wlen += tmp_wlen;
	}
	while (rlen != response_size) {
		tmp_rlen = read(fd, &response[rlen], response_size - rlen);
		if (rlen < 0) {
			fprintf(stderr, "can not read request\n");
			return 1;
		}
		rlen += tmp_rlen;
	}

	return 0;
}

static int
get_statistics(
    fplug_device_t *fplug_device,
    int check_status)
{
	unsigned char buf[32];
	size_t buf_size;
	int i;
	int fd = fplug_device->sd;

	if (!fplug_device->connected) {
		return 0;
	}

	if (check_status & 0x01) {
		i = 0;
		while (1) {
			buf_size = 15;
			if (do_request(fd, Temp, sizeof(Temp), buf, buf_size)) {
				printf("Temp 0 failed in request\n");
				return 1;
			}
			if (buf[5] != 0x00) {
				read(fd, &buf[buf_size], sizeof(buf) - buf_size);
			}
			if (buf[5] == 0x11 && buf[10] == 0x72 && buf[13] == 2) {
				break;
			}
			i++;
			if (i > 10) {
				printf("Temp 0 give up to get statistics\n");
				return 1;
			}
		}
		printf("Temp %.1lf success\n", (double)((int)buf[14] + ((int)buf[15] << 8)) / 10.0);
	}
	if (check_status & 0x02) {
		i = 0;
		while (1) {
			buf_size = 15;
			if (do_request(fd, Humid, sizeof(Humid), buf, buf_size)) {
				printf("Humid 0 failed in request\n");
				return 1;
			}
			if (buf[5] != 0x00) {
				read(fd, &buf[buf_size], sizeof(buf) - buf_size);
			}
			if (buf[5] == 0x12 && buf[10] == 0x72 && buf[13] == 1) {
				break;
			}
			i++;
			if (i > 10) {
				printf("Humid 0 give up to get statistics\n");
				return 1;
			}
		}
		printf("Humid %d success\n", (int)buf[14]);
	}
	if (check_status & 0x04) {
		i = 0;
		while (1) {
			buf_size = 15;
			if (do_request(fd, Illum, sizeof(Illum), buf, buf_size)) {
				printf("Illum 0 failed in request\n");
				return 1;
			}
			if (buf[5] != 0x00) {
				read(fd, &buf[buf_size], sizeof(buf) - buf_size);
			}
			if (buf[5] == 0x0d && buf[10] == 0x72 && buf[13] == 2) {
				break;
			}
			i++;
			if (i > 10) {
				printf("Illum 0 give up to get statistics\n");
				return 1;
			}
		}
                printf("Illum %d success\n", (int)buf[14] + ((int)buf[15] << 8));
	}
	if (check_status & 0x08) {
		i = 0;
		while (1) {
			buf_size = 15;
			if (do_request(fd, RWatt, sizeof(RWatt), buf, buf_size)) {
				printf("Watt 0 failed in request\n");
				return 1;
			}
			if (buf[5] != 0x00) {
				read(fd, &buf[buf_size], sizeof(buf) - buf_size);
			}
                        if (buf[5] == 0x22 && buf[10] == 0x72 && buf[13] == 2) {
				break;
			}
			i++;
			if (i > 10) {
				printf("Watt 0 give up to get statistics\n");
				return 1;
			}
		}
		printf("Watt %.1lf success\n", (double)((int)buf[14] + ((int)buf[15] << 8)) / 10);
	}

	return 0;
}

void
statistics_main(
    int fd,
    short event,
    void *args)
{
        fplugd_t *fplugd = args;
	int i;

	for (i = 0; i < fplugd->device_count; i++) {
		get_statistics(&fplugd->fplug_device[i], fplugd->check_status);
	}
}

int
main(
    int argc,
    char *argv[])
{
	int error = 0;
	fplugd_t fplugd;
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
	fplugd.event_base = event_init();
	sigaddset(&fplugd.sigmask, SIGPIPE);
	pthread_sigmask(SIG_SETMASK, &fplugd.sigmask, NULL);
	signal_set(&fplugd.sigterm_event, SIGTERM, terminate, &fplugd);
	event_base_set(fplugd.event_base, &fplugd.sigterm_event);
	signal_add(&fplugd.sigterm_event, NULL);
	signal_set(&fplugd.sigint_event, SIGINT, terminate, &fplugd);
	event_base_set(fplugd.event_base, &fplugd.sigint_event);
	signal_add(&fplugd.sigint_event, NULL);
	for (i = 0; i < fplugd.device_count; i++) {
		if (connect_bluetooth(&fplugd.fplug_device[i])) {
			continue;
		}
		fplugd.available_count += 1;
	}
	if (fplugd.available_count == 0) {
		fprintf(stderr, "no there available device.");
		error = 1;
		goto last;
	}
	fplugd.timer_tv.tv_sec = 5;
	fplugd.timer_tv.tv_usec = 0;
	//evtimer_set(&fplugd.timer_event, statistics_main, &fplugd);
	event_set(&fplugd.timer_event, -1, EV_PERSIST, statistics_main, &fplugd);
	event_base_set(fplugd.event_base, &fplugd.timer_event);
	evtimer_add(&fplugd.timer_event, &fplugd.timer_tv);
	if (event_base_dispatch(fplugd.event_base) == -1) {
		fprintf(stderr, "failed in event base dispatch");
		error = 1;
		goto last;
	}
last:
	for (i = 0; i < fplugd.device_count; i++) {
		close_bluetooth(&fplugd.fplug_device[i]);
	}

	return error;
}

