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

#include "common_macros.h"
#include "common_define.h"
#include "string_util.h"

struct fplug_device {
	int type;
	const char *device_name[MAX_DEV_NAMELEN];
	const char *device_address[MAX_DEV_ADDRLEN];
};
typedef struct fplug_device fplug_device_t;

struct fplugc {
	int check_stat;
	fplug_device_t fplug_device[MAX_FPLUG_DEVICE];
	int device_count;
	const char *addr[NI_MAXHOST];
	const char *port[NI_MAXSERV];
	int sd;
        sigset_t sigmask;
	char request_buff[MAX_CNTRL_REQ_BUFF];
	size_t request_len;
	char response_buff[MAX_CNTRL_RES_BUFF];
};
typedef struct fplugc fplugc_t;

void
initialize_fplugc(
    fplugc_t *fplugc)
{
	memset(fplugc, 0, sizeof(fplugc_t));
	fplugc->sd = -1;
	fplugc->addr = DEFAULT_CNTRL_ADDR;
	fplugc->port = DEFAULT_CNTRL_PORT;
}

static int
parse_command_arguments(
    int argc,
    char *argv[],
    fplugc_t *fplugc)
{
	int opt;

	while ((opt = getopt(argc, argv, "d:n:s:p:thiwF")) != -1) {
		switch (opt) {
		case 'd':
			fplugc->fplug_device[fplugc->device_count].type = TYPE_DEV_ADDR;
			strlcpy(fplugc->fplug_device[fplugc->device_count].device_address, optarg, sizeof(fplugc->fplug_device[0].device_address));
			fplugc->device_count += 1;
			break;
		case 'n':
			fplugc->fplug_device[fplugc->device_count].type = TYPE_DEV_NAME;
			strlcpy(fplugc->fplug_device[fplugc->device_count].device_name, optarg, sizeof(fplugc->fplug_device[0].device_name));
			fplugc->device_count += 1;
			break;
		case 's':
			strlcpy(fplugc->addr, optarg, sizeof(fplugc->addr));
			break;
		case 'p':
			strlcpy(fplugc->port, optarg, sizeof(fplugc->port));
			break;
		case 't':
			fplugc->check_stat |= 0x01;
                   	break;
		case 'h':
			fplugc->check_stat |= 0x02;
                   	break;
		case 'i':
			fplugc->check_stat |= 0x04;
                   	break;
		case 'w':
			fplugc->check_stat |= 0x08;
                   	break;
		default:
			fprintf(stderr, "Usage: %s [[-d <device address>]...] [[-n <device name>]...] [-s port] [-p port] [-thiw]\n", argv[0]);
			return 1;
		}
	}
	if (fplugc->check_stat == 0) {
		fprintf(stderr, "Usage: %s [[-d <device address>]...] [[-n <device name>]...] [-s port] [-p port] [-thiw]\n", argv[0]);
		return 1;
	}

	return 0;
}

static void 
make_request(
    fplugc_t *fplugc)
{
	fplug_device_t *fplug_device;
	int tmp_len;
	int req_len = 0;

	if ((tmp_len = snprintf(fplugc->request, sizeof(fplugc->request), "stat -c %d", fplugc->check_stat)) < 0) {
		return 1;
	}
	req_len += tmp_len;
	for (i = 0; i < fplugc->device_count; i++) {
		fplug_device = &fplugc->fplug_device[i];
		tmp_len = 0;
		if (fplug_device->type == TYPE_DEV_NAME) {
			tmp_len = snprintf(&fplugc->request[req_len], sizeof(fplugc->request) - req_len, " -n %s", fplug_device->name);
		} else if (fplug_device->type == TYPE_DEV_ADDR) {
			tmp_len = snprintf(&fplugc->request[req_len], sizeof(fplugc->request) - req_len, " -d %s", fplug_device->addr);
		}
		if (tmp_len < 0) {
			return 1;
		}
		req_len += tmp_len;
	}
	if ((tmp_len = snprintf(&fplugc->request[req_len], sizeof(fplugc->request) - req_len, "\r\n")) < 0) {
		return 1;
	}
	fplugc->request_len = req_len;

	return 0;
}

static int
connect_daemon(
    int *sock,
    const char *addr,
    const char *port)
{
        int s = -1;
        struct addrinfo hints, *res, *res0 = NULL;
        int reuse_addr = 1;
        int nodelay = 1;
        char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

        ASSERT(sock != NULL);
        ASSERT(addr != NULL);
        ASSERT(port != NULL);
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = PF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(addr, port, &hints, &res0) != 0) {
                fprintf(stderr, "failed in getaddrinfo\n");
                goto last;
        }
        for (res = res0; res; res = res->ai_next) {
                if ((s = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
                        fprintf(stderr, "failed in create socket\n");
                        s = -1;
                        continue;
                }
                if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr))) {
                        fprintf(stderr, "failed in set REUSEADDR\n");
                        close(s);
                        s = -1;
                        continue;
                }
                if (setsockopt(s, SOL_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay))) {
                        fprintf(stderr, "failed in set NODELAY\n");
                        close(s);
                        s = -1;
                        continue;
                }
                if (getnameinfo(
                    res->ai_addr,
                    res->ai_addrlen,
                    hbuf, sizeof(hbuf),
                    sbuf, sizeof(sbuf),
                    NI_NUMERICHOST|NI_NUMERICSERV)) {
                        fprintf(stderr, "failed in getnameinfo %m\n");
                } else {
                        fprintf(stderr, "connect address = %s, port = %s\n", hbuf, sbuf);
                }
                if (connect(s, res->ai_addr, res->ai_addrlen) < 0) {
                        fprintf(stderr, "failed in connect\n");
                        close(s);
                        s = -1;
                        continue;
                }
                break;
        }
last:
        if (res0) {
                freeaddrinfo(res0);
        }
        if (s == -1) {
                return 1;
        }
        *sock = s;

        return 0;
}

static int
do_request(
    int sd,
    const char *request,
    size_t request_size,
    unsigned char *response,
    size_t response_size)
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
                        fprintf(stderr, "can not read response\n");
                        return 1;
                }
                rlen += tmp_rlen;
        }

        return 0;
}

int
main(
    int argc,
    char *argv[])
{
	int error = 0;
	fplugc_t fplugc;
	int req_len;
	int i;

	initialize_fplugc(&fplugc);
	if (parse_command_arguments(argc, argv, &fplugc)) {
		return 1;
	}
	sigemptyset(&fplugc.sigmask);
	sigaddset(&fplugc.sigmask, SIGPIPE);
	pthread_sigmask(SIG_BLOCK, &fplugc.sigmask, NULL);
	if (make_request(&fplugc)) {
		fprintf(stderr, "faile in create request");
		error = 1
		goto last;
	}
	if (connect_daemon(&fplugc.sd, fplugc.addr, fplugc.port)) {
		fprintf(stderr, "faile in connect to daemon");
		error = 1
		goto last;
	}
	if (do_request(fplugc.sd, fplugc.request, fplugc.request_len, fplugc.response, sizeof(fplugc.response)) {
		fprintf(stderr, "faile in connect to daemon");
		error = 1;
		goto last;
	}

last:
	if (fplugc.sd != -1) {
		close(fplugc.sd);
	}

	return error;
}

