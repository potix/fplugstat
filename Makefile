all:
	gcc -O2 -Wall -o fplugstat fplugstat.c
clean:
	rm -rf *.o fplugstat
