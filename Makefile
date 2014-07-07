all:
	#gcc -O2 -Wall -o fplugstat fplugstat.c
	gcc -O2 -Wall -levent -lbluetooth -o fplugstatd fplugstatd.c
clean:
	rm -rf *.o fplugstat
