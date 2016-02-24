PROG = fplugstatd
OBJS = main.o fplug_device.o stat_store.o config.o string_util.o logger.o http.o
CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -levent -lbluetooth

.SUFFIXES: .c .o

all: clean $(PROG)

$(PROG): $(OBJS) 
	$(CC) $(LDFLAGS) -o $(PROG) $^
.c.o:
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -rf *.o fplugstatd
