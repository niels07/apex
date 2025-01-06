CC = gcc
CFLAGS = -Wall -Wextra -Werror -Wno-implicit-fallthrough -std=c99 -g
BIN = apex
OBJ = main.o error.o lexer.o mem.o string.o ast.o parser.o value.o symbol.o vm.o compiler.o stdlib.o util.o

all: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(BIN) -lm

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

error.o: error.c error.h
	$(CC) $(CFLAGS) -c error.c

lexer.o: lexer.c lexer.h
	$(CC) $(CFLAGS) -c lexer.c

mem.o: mem.c mem.h
	$(CC) $(CFLAGS) -c mem.c

string.o: string.c string.h	
	$(CC) $(CFLAGS) -c string.c

ast.o: ast.c ast.h
	$(CC) $(CFLAGS) -c ast.c

value.o: value.c value.h
	$(CC) $(CFLAGS) -c value.c

symbol.o: symbol.c symbol.h
	$(CC) $(CFLAGS) -c symbol.c

vm.o: vm.c vm.h
	$(CC) $(CFLAGS) -c vm.c

compiler.o: compiler.c compiler.h
	$(CC) $(CFLAGS) -c compiler.c

parser.o: parser.c parser.h
	$(CC) $(CFLAGS) -c parser.c

stdlib.o: stdlib.c stdlib.h
	$(CC) $(CFLAGS) -c stdlib.c

util.o: util.c util.h
	$(CC) $(CFLAGS) -c util.c

clean:
	rm -f $(OBJ)
	rm -f $(BIN)
