all:
	#gcc -O2 -Wall -o fplugstat fplugstat.c
	gcc -O2 -Wall -levent -lbluetooth -o fplugstatd fplugstatd.c tcp_server.c controller.c string_util.c
clean:
	rm -rf *.o fplugstat fplugstatd
