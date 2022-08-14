CFLAGS=-std=c11 -Wall -Werror -Wno-unused-result -fno-common 
SOURCE_FILES=src/shared/*.c src/platform/posix/*.c src/game/*.c
DEBUG_FLAGS=-g -DSPACE_SHOOTER_DEBUG
RELEASE_FLAGS=-O3

LINUX_CC=gcc
LINUX_CFLAGS=-DSOGL_MAJOR_VERSION=3 -DSOGL_MINOR_VERSION=3 -D_POSIX_C_SOURCE=199309L -o build/space-shooter
LINUX_SOURCE_FILES=src/platform/linux/*.c
LINUX_LDLIBS=-lX11 -ldl -lGL -lm -lpthread -lasound

WEB_CC=emcc
WEB_CFLAGS=-DSPACE_SHOOTER_OPENGLES -sMAX_WEBGL_VERSION=2 -sMIN_WEBGL_VERSION=2 --preload-file "./assets" -sINITIAL_MEMORY=59179008
WEB_DEBUG_FLAGS=-fdebug-compilation-dir=".."
WEB_SOURCE_FILES=src/platform/web/*.c
WEB_LDLIBS=-lopenal
WEB_DEBUG_DIR=build
WEB_RELEASE_DIR=site

linux-debug: assets
	$(LINUX_CC) $(DEBUG_FLAGS) $(CFLAGS) $(LINUX_CFLAGS) $(SOURCE_FILES) $(LINUX_SOURCE_FILES) $(LINUX_LDLIBS)

linux-release: assets 
	$(LINUX_CC) $(RELEASE_FLAGS) $(CFLAGS) $(LINUX_CFLAGS) $(SOURCE_FILES) $(LINUX_SOURCE_FILES) $(LINUX_LDLIBS)

web-debug: clean
	$(WEB_CC) $(DEBUG_FLAGS) $(WEB_DEBUG_FLAGS) $(CFLAGS) $(WEB_CFLAGS) $(SOURCE_FILES) $(WEB_SOURCE_FILES) $(WEB_LDLIBS) -o $(WEB_DEBUG_DIR)/space-shooter.js
	cp src/platform/web/index.html $(WEB_DEBUG_DIR)/index.html

web-release:
	rm -rf $(WEB_RELEASE_DIR)
	mkdir $(WEB_RELEASE_DIR)
	$(WEB_CC) $(RELEASE_FLAGS) $(CFLAGS) $(WEB_CFLAGS) $(SOURCE_FILES) $(WEB_SOURCE_FILES) $(WEB_LDLIBS) -o $(WEB_RELEASE_DIR)/space-shooter.js
	cp src/platform/web/index.html $(WEB_RELEASE_DIR)/index.html

assets: clean
	cp -r assets build/assets

clean:
	rm -rf build
	mkdir build
	
.PHONY: debug release assets
