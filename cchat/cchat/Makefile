CC = gcc
CFLAGS = -g -Wall

all: server

server: server.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	-rm -f *.o *~ *core* server
