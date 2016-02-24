PROG = fplugstatd
OBJS = main.o fplug_device.o stat_store.o config.o string_util.o logger.o http.o
CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -L/lib -L/lib/x86_64-linux-gnu -L/lib64 -L/usr/lib -L/usr/lib/x86_64-linux-gnu -L/usr/lib64 -levent -lbluetooth -lpthread

.SUFFIXES: .c .o

all: clean $(PROG)

$(PROG): $(OBJS) 
	$(CC) -o $(PROG) $^ $(LDFLAGS) 
.c.o:
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -rf *.o fplugstatd
