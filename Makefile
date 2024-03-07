PWD:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
CC=gcc
CFLAGS=-g -std=c89 -pedantic -I$(PWD) -I$(PWD)/include -Wall -Wno-switch 
LDFLAGS=-lm -ldl -lapex -L.
OBJ=hash.o error.o malloc.o lexer.o parser.o vm.o value.o compiler.o api.o  
LIB=libapex.a
BIN=apex
TESTS=tests/lexer_test

all: $(OBJ) main.o
	$(MAKE) modules/Makefile
	ar rcD $(LIB) $(OBJ)
	ranlib $(LIB)
	$(CC) -o $(BIN) main.o $(LDFLAGS) 

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

error.o: error.c error.h
	$(CC) $(CFLAGS) -c error.c

lexer.o: lexer.c lexer.h
	$(CC) $(CFLAGS) -c lexer.c

malloc.o: malloc.c malloc.h
	$(CC) $(CFLAGS) -c malloc.c

io.o: io.c io.h
	$(CC) $(CFLAGS) -c io.c

parser.o: parser.c parser.h
	$(CC) $(CFLAGS) -c parser.c

compiler.o: compiler.c compiler.h
	$(CC) $(CFLAGS) -c compiler.c

vm.o: vm.c vm.h
	$(CC) $(CFLAGS) -c vm.c

value.o: value.c value.h
	$(CC) $(CFLAGS) -c value.c

hash.o: hash.c hash.h
	$(CC) $(CFLAGS) -c hash.c

api.o: api.c 
	$(CC) $(CFLAGS) -c api.c

tests: $(TESTS)

tests/lexer_test: $(OBJ) tests/lexer_test.o
	$(CC) -o tests/lexer_test $(OBJ) tests/lexer_test.o 

tests/lexer_test.o: tests/lexer_test.c
	$(CC) $(CFLAGS) -c tests/lexer_test.c -o tests/lexer_test.o

install:
	mkdir -p /usr/include/apex
	mkdir -p /usr/lib/apex	
	cp apex.h /usr/include/apex
	cp value.h /user/include/apex
	cp $(BIN) /usr/bin
	cp $(LIB) /usr/lib/apex

clean:
	rm -f ./*.o $(BIN) $(LIB)
	rm -f tests/*.o $(TESTS)

