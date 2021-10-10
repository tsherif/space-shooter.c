CFLAGS=-g -std=c11 -Wall -fms-extensions -DSOGL_MAJOR_VERSION=4 -DSOGL_MINOR_VERSION=5
CC=gcc
LDLIBS=-lX11 -ldl -lGL -lm

all:
	rm -rf build
	mkdir build
	cp -r assets build/assets   
	$(CC) $(CFLAGS) -o build/space-shooter src/shared/*.c src/game/*.c src/platform/x11/*.c $(LDLIBS)
