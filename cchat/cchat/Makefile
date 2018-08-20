CC = gcc
CFLAGS = -g -Wall -pthread

all: server

server: server.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	-rm -f *.o *~ *core* server
