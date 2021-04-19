CC=gcc
CFLAGS=-Wall -g

all: rex rexd

cmd.o: cmd.c cmd.h
	$(CC) $(CFLAGS) -c cmd.c

proto.o: proto.c proto.h
	$(CC) $(CFLAGS) -c proto.c

rex: rex.c proto.o cmd.o
	$(CC) $(CFLAGS) rex.c proto.o cmd.o -o rex

rexd: rexd.c proto.o cmd.o
	$(CC) $(CFLAGS) rexd.c proto.o cmd.o -o rexd -pthread

clean:
	rm -rf rex rexd *.o
