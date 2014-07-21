all:
	gcc -O2 -Wall -levent -lbluetooth -ljansson -o fplugstatd main.c tcp_server.c controller.c string_util.c request.c stat.c
clean:
	rm -rf *.o fplugstatd
