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

