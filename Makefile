PWD:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
CC=gcc
CFLAGS=-g -std=c89 -pedantic -I$(PWD) -I$(PWD)/include -Wall -Wno-switch 
LDFLAGS=-lm -ldl -lapex -L.
OBJ=error.o malloc.o lexer.o parser.o io.o compiler.o strings.o vm.o ast.o value.o hash.o api.o
BIN=apex
LIB=libapex.a
TESTS=lexer_test parser_test

all: $(OBJ) main.o
	ar rcu $(LIB) $(OBJ)
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

strings.o: strings.c strings.h
	$(CC) $(CFLAGS) -c strings.c

ast.o: ast.c ast.h
	$(CC) $(CFLAGS) -c ast.c

value.o: value.c include/apex/value.h
	$(CC) $(CFLAGS) -c value.c

hash.o: hash.c hash.h
	$(CC) $(CFLAGS) -c hash.c

api.o: api.c 
	$(CC) $(CFLAGS) -c api.c

tests: $(TESTS)

lexer_test: $(OBJ) tests/lexer_test.o
	$(CC) $(LDFLAGS) $(OBJ) tests/lexer_test.o -o lexer_test

tests/lexer_test.o: tests/lexer_test.c
	$(CC) $(CFLAGS) -c tests/lexer_test.c -o tests/lexer_test.o

parser_test: $(OBJ) tests/parser_test.o
	$(CC) $(LDFLAGS) $(OBJ) tests/parser_test.o -o parser_test

tests/parser_test.o: tests/parser_test.c
	$(CC) $(CFLAGS) -c tests/parser_test.c -o tests/parser_test.o

install:
	mkdir -p /usr/include/apex
	mkdir -p /usr/lib/apex	
	cp -R $(PWD)/include/* /usr/include
	cp $(BIN) /usr/bin
	cp $(LIB) /usr/lib/apex

clean:
	rm -f ./*.o $(BIN) $(LIB)
	rm -f $(TESTS)


