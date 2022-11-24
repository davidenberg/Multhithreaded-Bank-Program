CC=gcc

CFLAGS= -g -Wall -pedantic

CLEANEXTS = o a

INSTALLDIR = .

all: connection server libqueuelib.a

connection: connection.c
	$(CC) $(CFLAGS) -o connection connection.c

server: server.c libqueuelib.a
	$(CC) $(CFLAGS) -o server server.c -pthread -L. -lqueuelib

queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -O -c queue.c

libqueuelib.a: queue.o
	ar rcs libqueuelib.a queue.o

clean:
	rm -rf *.o connection server