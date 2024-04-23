CFLAGS=-std=c11 -pedantic -Wvla -Wall -Werror -D_DEFAULT_SOURCE -g

CC=gcc

ALL= server client

all: $(ALL)

# Server

server : server.o utils_v1.o
	$(CC) $(CCFLAGS) -o server server.o utils_v1.o
server.o: server.c jeu.h
	$(CC) $(CCFLAGS) -c server.c

# Client

client : client.o utils_v1.o
	$(CC) $(CCFLAGS) -o client client.o utils_v1.o
client.o: client.c jeu.h
	$(CC) $(CCFLAGS) -c client.c

# UTILS

utils_v1.o: utils_v1.c utils_v1.h
	cc $(CFLAGS) -c utils_v1.c

# NETTOYAGE
clean: 
	rm -f *.o
	rm -f $(ALL)