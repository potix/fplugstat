all:
	#gcc -O2 -Wall -o fplugstat fplugstat.c
	gcc -O2 -Wall -levent -lbluetooth -o fplugstatd main.c tcp_server.c controller.c string_util.c request.c stat.c
clean:
	rm -rf *.o fplugstat fplugstatd
