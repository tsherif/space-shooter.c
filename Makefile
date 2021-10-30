CFLAGS=-g -std=c11 -Wall -DSOGL_MAJOR_VERSION=4 -DSOGL_MINOR_VERSION=5 -D_POSIX_C_SOURCE=199309L
CC=gcc
LDLIBS=-lX11 -ldl -lGL -lm -lpthread -lasound

all:
	rm -rf build
	mkdir build
	cp -r assets build/assets   
	$(CC) $(CFLAGS) -o build/space-shooter src/shared/*.c src/game/*.c src/platform/linux/*.c $(LDLIBS)
