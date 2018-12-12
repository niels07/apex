PWD:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
CC=gcc
LIB=libapex.a
LIB_DIR=/usr/lib/apex/modules
INC_DIR=/usr/include/apex
CFLAGS=-fPIC -std=c89 -pedantic -Wall -Wno-switch -I/usr/include -c 

BIN=libio.so
OBJ=io.o

all: $(BIN)

libio.so: io.o
	$(CC) -rdynamic -o libio.so io.o -shared 

io.o: io.c
	$(CC) $(CFLAGS) io.c 

install:
	mkdir -p $(LIB_DIR)
	cp ./*.so $(LIB_DIR)

clean:
	rm -f io.o
	rm -f libio.so
