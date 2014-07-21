all:
	gcc -O2 -Wall -levent -lbluetooth -ljansson -o fplugstatd main.c fplug_device.c config.c
clean:
	rm -rf *.o fplugstatd
