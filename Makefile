CFLAGS=-std=c11 -Wall -Wno-unused-result -fno-common 
SOURCE_FILES=src/shared/*.c src/platform/posix/*.c src/game/*.c
DEBUG_FLAGS=-g -DSPACE_SHOOTER_DEBUG
RELEASE_FLAGS=-O3

LINUX_CC=gcc
LINUX_CFLAGS=-DSOGL_MAJOR_VERSION=3 -DSOGL_MINOR_VERSION=3 -D_POSIX_C_SOURCE=199309L -o build/space-shooter
LINUX_SOURCE_FILES=src/platform/linux/*.c
LINUX_LDLIBS=-lX11 -ldl -lGL -lm -lpthread -lasound

WEB_CC=emcc
WEB_CFLAGS=-DSPACE_SHOOTER_OPENGLES -sMAX_WEBGL_VERSION=2 -sMIN_WEBGL_VERSION=2 --preload-file "./assets" -sINITIAL_MEMORY=59965440 -o build/space-shooter.js
WEB_SOURCE_FILES=src/platform/web/*.c
WEB_LDLIBS=-lopenal

linux-debug: assets
	$(LINUX_CC) $(DEBUG_FLAGS) $(CFLAGS) $(LINUX_CFLAGS) $(SOURCE_FILES) $(LINUX_SOURCE_FILES) $(LINUX_LDLIBS)

linux-release: assets 
	$(LINUX_CC) $(RELEASE_FLAGS) $(CFLAGS) $(LINUX_CFLAGS) $(SOURCE_FILES) $(LINUX_SOURCE_FILES) $(LINUX_LDLIBS)

web-debug: clean
	$(WEB_CC) $(DEBUG_FLAGS) $(CFLAGS) $(WEB_CFLAGS) $(SOURCE_FILES) $(WEB_SOURCE_FILES) $(WEB_LDLIBS)
	cp src/platform/web/index.html build/index.html

web-release: clean
	$(WEB_CC) $(RELEASE_FLAGS) $(CFLAGS) $(WEB_CFLAGS) $(SOURCE_FILES) $(WEB_SOURCE_FILES) $(WEB_LDLIBS)
	cp src/platform/web/index.html build/index.html

assets: clean
	cp -r assets build/assets

clean:
	rm -rf build
	mkdir build
	
.PHONY: debug release assets
