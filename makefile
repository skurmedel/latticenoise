CFLAGS=--std=c99 -Wall 


all: lib exe

lib:
	setup
	CC -c src/latticenoise.c -o build/latticenoise.o
	ar rcs bin/liblatticenoise.lib build/latticenoise.o

exe: lib
	CFLAGS+= -Lbin/ -llatticenoise
	CC -static src/mknoise.c -o bin/mknoise

setup:
	@mkdir -p build
	@mkdir -p bin

clean:
	rm -rf bin
	rm -rf build