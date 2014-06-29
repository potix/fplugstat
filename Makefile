all:
	gcc -O2 -o fplugstat fplugstat.c
clean:
	rm -rf *.o fplugstat
