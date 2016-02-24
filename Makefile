all:
	gcc -O2 -Wall  -levent -lbluetooth -o fplugstatd main.c fplug_device.c stat_store.c config.c string_util.c logger.c http.c
clean:
	rm -rf *.o fplugstatd
