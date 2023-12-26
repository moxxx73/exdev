.PHONY: all install static build_dir

all: libexdev.so static

libexdev.so: build_dir
	gcc -Wall -Wextra -fPIC -shared ./src/exdev.c ./src/utils.c ./src/shell.c -o ./build/libexdev.so 

install:
	ln -sf $(shell pwd)/include/exdev.h /usr/local/include/exdev.h
	ln -sf $(shell pwd)/build/libexdev.so /usr/local/lib/libexdev.so

static: build_dir
	gcc -Wall -Wextra -c ./src/exdev.c -o ./build/exdev.o
	gcc -Wall -Wextra -c ./src/utils.c -o ./build/utils.o
	gcc -Wall -Wextra -c ./src/shell.c -o ./build/shell.o
	ar cr ./build/libexdev.a ./build/exdev.o ./build/shell.o ./build/utils.o 
	rm ./build/exdev.o ./build/shell.o ./build/utils.o

build_dir:
	mkdir -p ./build