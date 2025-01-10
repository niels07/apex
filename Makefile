CC = gcc
CFLAGS = -Wall -Wextra -Werror -Wno-implicit-fallthrough -std=c99 -g -rdynamic
BIN = apex
OBJ = main.o error.o lexer.o mem.o apexStr.o ast.o parser.o apexVal.o symbol.o vm.o compiler.o util.o lib.o
LIB_OBJ = lib/libio.so lib/libstd.so

all: $(OBJ) $(LIB_OBJ)
	$(CC) $(CFLAGS) -I . $(OBJ) $(LIB_OBJ) -o $(BIN) -lm

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

error.o: error.c error.h
	$(CC) $(CFLAGS) -c error.c

lexer.o: lexer.c lexer.h
	$(CC) $(CFLAGS) -c lexer.c

mem.o: mem.c mem.h
	$(CC) $(CFLAGS) -c mem.c

apexStr.o: apexStr.c apexStr.h	
	$(CC) $(CFLAGS) -c apexStr.c

ast.o: ast.c ast.h
	$(CC) $(CFLAGS) -c ast.c

apexVal.o: apexVal.c apexVal.h
	$(CC) $(CFLAGS) -c apexVal.c

symbol.o: symbol.c symbol.h
	$(CC) $(CFLAGS) -c symbol.c

vm.o: vm.c vm.h
	$(CC) $(CFLAGS) -c vm.c

compiler.o: compiler.c compiler.h
	$(CC) $(CFLAGS) -c compiler.c

parser.o: parser.c parser.h
	$(CC) $(CFLAGS) -c parser.c

lib.o: lib.c lib.h
	$(CC) $(CFLAGS) -c lib.c

util.o: util.c util.h
	$(CC) $(CFLAGS) -c util.c

lib/libio.so: lib/io.c lib/io.h
	$(CC) -shared -I . -o lib/libio.so -fPIC lib/io.c

lib/libstd.so: lib/std.c lib/std.h
	$(CC) -shared -I . -o lib/libstd.so -fPIC lib/std.c

clean:
	rm -f $(OBJ)
	rm -f $(LIB_OBJ)
	rm -f $(BIN)
