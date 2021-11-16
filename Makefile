CFLAGS=-std=c11 -Wall -Wno-unused-result -DSOGL_MAJOR_VERSION=3 -DSOGL_MINOR_VERSION=3 -D_POSIX_C_SOURCE=199309L -o build/space-shooter
DEBUG_FLAGS=-g -DSPACE_SHOOTER_DEBUG
RELEASE_FLAGS=-O3 -Wno-unused-variable -Wno-unused-but-set-variable
SOURCE_FILES=src/shared/*.c src/game/*.c src/platform/linux/*.c
CC=gcc
LDLIBS=-lX11 -ldl -lGL -lm -lpthread -lasound

debug: assets
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(SOURCE_FILES) $(LDLIBS)

release: assets 
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) $(SOURCE_FILES) $(LDLIBS)

assets:
	rm -rf build
	mkdir build
	cp -r assets build/assets
	
.PHONY: debug release assets
