all:
	gcc -O2 -Wall -levent -lbluetooth -o fplugstatd main.c fplug_device.c config.c string_util.c logger.c
clean:
	rm -rf *.o fplugstatd
