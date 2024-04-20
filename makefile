CFLAGS=-std=c11 -pedantic -Wvla -Wall -Werror -D_DEFAULT_SOURCE -g

all: encrypt decrypt encryptPolybe decryptPolybe

# EDITION DES LIENS

# exécutables ROT13


# exécutables 

# COMPILATION

utils_v1.o: utils_v1.c utils_v1.h
	cc $(CFLAGS) -c utils_v1.c

# NETTOYAGE
clean: 
	rm -f *.o
	rm -f encrypt decrypt encryptPolybe decryptPolybe
	
