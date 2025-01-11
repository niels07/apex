CC = gcc
CFLAGS = -Wall -Wextra -Werror -Wno-implicit-fallthrough -std=c99 -g -rdynamic
BIN = apex
OBJ = main.o apexErr.o apexLex.o apexMem.o apexStr.o apexAST.o apexParse.o apexVal.o apexSym.o apexVM.o apexCode.o apexUtil.o apexLib.o
LIB_OBJ = lib/libio.so lib/libstd.so

all: $(OBJ) $(LIB_OBJ)
	$(CC) $(CFLAGS) -I . $(OBJ) $(LIB_OBJ) -o $(BIN) -lm

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

apexErr.o: apexErr.c apexErr.h
	$(CC) $(CFLAGS) -c apexErr.c

apexLex.o: apexLex.c apexLex.h
	$(CC) $(CFLAGS) -c apexLex.c

apexMem.o: apexMem.c apexMem.h
	$(CC) $(CFLAGS) -c apexMem.c

apexStr.o: apexStr.c apexStr.h	
	$(CC) $(CFLAGS) -c apexStr.c

apexAST.o: apexAST.c apexAST.h
	$(CC) $(CFLAGS) -c apexAST.c

apexVal.o: apexVal.c apexVal.h
	$(CC) $(CFLAGS) -c apexVal.c

apexSym.o: apexSym.c apexSym.h
	$(CC) $(CFLAGS) -c apexSym.c

apexVM.o: apexVM.c apexVM.h
	$(CC) $(CFLAGS) -c apexVM.c

apexCode.o: apexCode.c apexCode.h
	$(CC) $(CFLAGS) -c apexCode.c

apexParse.o: apexParse.c apexParse.h
	$(CC) $(CFLAGS) -c apexParse.c

apexLib.o: apexLib.c apexLib.h
	$(CC) $(CFLAGS) -c apexLib.c

apexUtil.o: apexUtil.c apexUtil.h
	$(CC) $(CFLAGS) -c apexUtil.c

lib/libio.so: lib/io.c lib/io.h
	$(CC) -shared -I . -o lib/libio.so -fPIC lib/io.c

lib/libstd.so: lib/std.c lib/std.h
	$(CC) -shared -I . -o lib/libstd.so -fPIC lib/std.c

clean:
	rm -f $(OBJ)
	rm -f $(LIB_OBJ)
	rm -f $(BIN)
