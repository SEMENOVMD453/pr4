GCC = gcc
FLAGS = -Wall -pthread

all: server client

server: server.c
	$(GCC) $(FLAGS) server.c -o server

client: client.c
	$(GCC) $(FLAGS) client.c -o client

clean:
	rm -f server client *.o
