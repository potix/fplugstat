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
	unsigned char buf[32];
	size_t buf_size;
	int i;

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
