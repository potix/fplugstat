#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define DEFAULT_SERIAL_PORT "/dev/rfcomm0"

/* F-PLUG request */
const char Temp[]={0x10, 0x81, 0x00, 0x00, 0x0e, 0xf0, 0x00, 0x00, 0x11, 0x00, 0x62, 0x01, 0xe0, 0x00};
const char Humid[]={0x10, 0x81, 0x00, 0x00, 0x0e, 0xf0, 0x00, 0x00, 0x12, 0x00, 0x62, 0x01, 0xe0, 0x00};
const char Illum[]={0x10, 0x81, 0x00, 0x00, 0x0e, 0xf0, 0x00, 0x00, 0x0D, 0x00, 0x62, 0x01, 0xe0, 0x00};
const char RWatt[]={0x10, 0x81, 0x00, 0x00, 0x0e, 0xf0, 0x00, 0x00, 0x22, 0x00, 0x62, 0x01, 0xe2, 0x00};

static int do_request(int fd, const char *request, size_t request_size, unsigned char *response, size_t response_size);
static int get_status(int fd, int check_status);

int
main(int argc, char *argv[])
{
	int fd;  
	struct termios oldtio, newtio, tmptio;
	int opt;
	const char *open_dev = DEFAULT_SERIAL_PORT;
	int check_status = 0;

	while ((opt = getopt(argc, argv, "d:thiw")) != -1) {
		switch (opt) {
		case 'd':
			open_dev=optarg;
			break;
		case 't':
			check_status |= 0x01;
                   	break;
		case 'h':
			check_status |= 0x02;
                   	break;
		case 'i':
			check_status |= 0x04;
                   	break;
		case 'w':
			check_status |= 0x08;
                   	break;
		default:
			fprintf(stderr, "Usage: %s [-d <open_device>] [-thiw]\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}
	if (check_status == 0) {
		fprintf(stderr, "Usage: %s [-d <open_device>] [-thiw]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	if ((fd = open(open_dev, O_RDWR )) < 0) {
		fprintf(stderr, "can not open device %s\n", open_dev);
		exit(EXIT_FAILURE);
	}
    
	if (tcgetattr( fd, &oldtio )) {
		fprintf(stderr, "can not get terminal attribute %s\n", open_dev);
		exit(EXIT_FAILURE);
	}
	newtio = oldtio;
	newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
	if (tcsetattr(fd, TCSANOW, &newtio)) {
		fprintf(stderr, "can not set terminal attribute %s\n", open_dev);
		exit(EXIT_FAILURE);
	}
	if (tcgetattr( fd, &tmptio )) {
		fprintf(stderr, "can not get terminal attribute %s\n", open_dev);
		exit(EXIT_FAILURE);
	}
	if (newtio.c_cflag != tmptio.c_cflag) {
		fprintf(stderr, "terminal attribute is mismatch %x != %x\n", newtio.c_cflag, tmptio.c_cflag);
		exit(EXIT_FAILURE);
	}
	if (get_status(fd, check_status)) {
		fprintf(stderr, "can not get status\n");
	}
	tcsetattr(fd, TCSANOW, &oldtio);
	close(fd);    

	return 0;
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
get_status(int fd, int check_status)
{
	unsigned char buf[16];

	if (check_status & 0x01) {
		while (1) {
			if (do_request(fd, Temp, sizeof(Temp), buf, sizeof(buf))) {
				printf("0 failed in request\n");
				return 1;
			}
			if (buf[5] == 0x11 && buf[10] == 0x72 && buf[13] == 2) break;
		}
		printf( "%.1lf success\n", (double)(buf[14] + (buf[15] << 8)) / 10.0);
	}
	if (check_status & 0x02) {
		while (1) {
			if (do_request(fd, Humid, sizeof(Humid), buf, sizeof(buf))) {
				printf("0 failed in request\n");
				return 1;
			}
			if (buf[5] == 0x12 && buf[10] == 0x72 && buf[13] == 1) break;
		}
		printf( "%d success\n", buf[14]);
	}
	if (check_status & 0x04) {
		while (1) {
			if (do_request(fd, Illum, sizeof(Illum), buf, sizeof(buf))) {
				printf("0 failed in request\n");
				return 1;
			}
                        if (buf[5] == 0x0d && buf[10] == 0x72 && buf[13] == 2) break;
		}
                printf("%d success\n", buf[14] + (buf[15] << 8));
	}
	if (check_status & 0x08) {
		while (1) {
			if (do_request(fd, RWatt, sizeof(RWatt), buf, sizeof(buf))) {
				printf("0 failed in request\n");
				return 1;
			}
                        if (buf[5] == 0x22 && buf[10] == 0x72 && buf[13] == 2) break;
		}
		printf("%.1lf success\n", (double)(buf[14] + (buf[15] << 8)) / 10);
	}

	return 0;
}
