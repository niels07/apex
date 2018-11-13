PWD:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
CC=gcc
CFLAGS=-g -std=c89 -pedantic -I$(PWD) -Wall -Wno-switch 
LDFLAGS=-lm -ldl
OBJ=error.o malloc.o lexer.o parser.o io.o compiler.o strings.o vm.o ast.o value.o hash.o api.o
BIN=apex

all: $(OBJ) main.o
	$(CC) $(LDFLAGS) $(OBJ) main.o -o $(BIN)

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

value.o: value.c value.h
	$(CC) $(CFLAGS) -c value.c

hash.o: hash.c hash.h
	$(CC) $(CFLAGS) -c hash.c

api.o: api.c api.h
	$(CC) $(CFLAGS) -c api.c

tests: lexer_test parser_test

lexer_test: $(OBJ) tests/lexer_test.o
	$(CC) $(LDFLAGS) $(OBJ) tests/lexer_test.o -o lexer_test

tests/lexer_test.o: tests/lexer_test.c
	$(CC) $(CFLAGS) -c tests/lexer_test.c -o tests/lexer_test.o

parser_test: $(OBJ) tests/parser_test.o
	$(CC) $(LDFLAGS) $(OBJ) tests/parser_test.o -o parser_test

tests/parser_test.o: tests/parser_test.c
	$(CC) $(CFLAGS) -c tests/parser_test.c -o tests/parser_test.o

clean:
	rm -f $(OBJ) main.o
	rm -f $(BIN)

