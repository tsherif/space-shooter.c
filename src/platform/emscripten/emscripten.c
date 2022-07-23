#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include "../../shared/constants.h"
#include "../../shared/data.h"
#include "../../shared/platform-interface.h"
#include "../../shared/debug.h"

double lastTime = 0.0;
EM_BOOL loop(double time, void *userData) {
    if (lastTime == 0.0) {
        lastTime = time;
    }

    float dt = time - lastTime;
    lastTime = time;

    game_update(dt);
    game_draw();

    return EM_TRUE;
}

int32_t main() {
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE gl = 0;
    EmscriptenWebGLContextAttributes attrs = {
        .majorVersion = 2,
        .minorVersion = 0
    };

    double windowWidth = 0.0;
    double windowHeight = 0.0;
    emscripten_get_element_css_size("body", &windowWidth, &windowHeight);
    emscripten_set_canvas_element_size("#canvas", windowWidth, windowHeight);

    gl = emscripten_webgl_create_context("#canvas", &attrs);
    emscripten_webgl_make_context_current(gl);

    if (!game_init()) {
        goto EXIT_GAME;
    }

     game_resize((int32_t) windowWidth, (int32_t) windowHeight);

    emscripten_request_animation_frame_loop(loop, NULL);

    EXIT_GAME:
    game_close();

    return 0;
}


void platform_getInput(Game_Input* input) {

}
void platform_playSound(Data_Buffer* sound, bool loop) {

}

void platform_debugMessage(const char* message) {
    int32_t length = 0;
    while(message[length]) {
        ++length;
    }

    write(STDERR_FILENO, message, length);
    write(STDERR_FILENO, "\n", 1);
}

void platform_userMessage(const char* message) {
    platform_debugMessage(message);
}


bool platform_loadFile(const char* fileName, Data_Buffer* buffer, bool nullTerminate) {
    int32_t fd = open(fileName, O_RDONLY);
    uint8_t* data = 0;

    if (fd == -1) {
        DEBUG_LOG("platform_loadFile: Failed to open file.");
        goto ERROR_NO_RESOURCES;
    }

    int32_t size = lseek(fd, 0, SEEK_END);
    int32_t allocation = size;

    if (nullTerminate) {
        allocation += 1;
    }

    if (size == -1) {
        DEBUG_LOG("platform_loadFile: Failed to get file size.");
        goto ERROR_FILE_OPENED;
    }

    if (lseek(fd, 0, SEEK_SET) == -1) {
        DEBUG_LOG("platform_loadFile: Failed to reset file cursor.");
        goto ERROR_FILE_OPENED;
    }

    data = (uint8_t*) malloc(allocation);

    if (!data) {
        DEBUG_LOG("platform_loadFile: Failed to allocate data.");
        goto ERROR_FILE_OPENED;
    }

    if (read(fd, data, size) == -1) {
        DEBUG_LOG("platform_loadFile: Failed to read data.");
        goto ERROR_DATA_ALLOCATED;
    }

    if (nullTerminate) {
        data[allocation - 1] = 0;
    }

    buffer->data = data;
    buffer->size = allocation;
    close(fd);

    return true;

    ERROR_DATA_ALLOCATED:
    free(data);
    
    ERROR_FILE_OPENED:
    close(fd);
    
    ERROR_NO_RESOURCES:
    return false;
} 