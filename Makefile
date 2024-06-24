# Variables for flexibility
CC = gcc
CFLAGS = -Wall -g
LDFLAGS = 
TARGETS = server subscriber

all: $(TARGETS) 

server: server.c
	$(CC) $(CFLAGS) -o server server.c $(LDFLAGS)

subscriber: subscriber.c
	$(CC) $(CFLAGS) -o subscriber subscriber.c $(LDFLAGS)

clean:
	rm -f $(TARGETS)
