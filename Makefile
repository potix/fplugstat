PROG = fplugstatd
OBJS = main.o fplug_device.o stat_store.o echonet_lite.o config.o string_util.o logger.o http.o  file_util.o
CC = gcc
#CFLAGS = -Wall -g3 -ggdb3 -O0
CFLAGS = -Wall -O2
LDFLAGS = -L/lib -L/lib/x86_64-linux-gnu -L/lib64 -L/usr/lib -L/usr/lib/x86_64-linux-gnu -L/usr/lib64 -levent -lbluetooth -lpthread

.SUFFIXES: .c .o

.PHONY: all
all: $(PROG)

$(PROG): $(OBJS) 
	$(CC) -o $(PROG) $^  $(LDFLAGS) 

.c.o:
	$(CC) $(CFLAGS) -c $< 

.PHONY: clean
clean:
	rm -rf *.o fplugstatd
