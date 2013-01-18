CC=gcc $(CFLAGS) 
CFLAGS=--std=c99 -Wall

all: exe

lib: setup	
	$(CC) -c src/latticenoise.c -o build/latticenoise.o
	ar rs bin/liblatticenoise.a build/latticenoise.o

exe: lib
	$(CC) -c src/mknoise.c -o build/mknoise.o
	$(CC) -static build/mknoise.o -o bin/mknoise -Lbin/ -llatticenoise

setup:
	@mkdir -p build
	@mkdir -p bin

clean:
	rm -rf bin
	rm -rf build