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
#include "emscripten-audio.h"

static struct {
    int32_t width;
    int32_t height;
    bool fullscreen;
} windowState;

double lastTime = 0.0;

static struct {
    float stickX;
    float stickY;
    bool aButton;
    bool startButton;
    bool backButton;
    bool keyboard;
} gamepad;

static struct {
    bool left;
    bool right;
    bool up;
    bool down;
} keyboardDirections;

static bool strEquals(const char* s1, const char* s2, int32_t n) {
    for (int32_t i = 0; i < n; ++i) {
        if (s1[i] != s2[i]) {
            return false;
        }

        if (s1[i] == '\0' || s2[i] == '\0') {
            return s1[i] == '\0' && s2[i] == '\0';
        }
    }

    return true;
}

static EM_BOOL loop(double time, void *userData) {
    if (lastTime == 0.0) {
        lastTime = time;
    }

    float dt = time - lastTime;
    lastTime = time;

    emscripten_updateAudio();
    game_update(dt);
    game_draw();

    return EM_TRUE;
}

static EM_BOOL onKeyDown(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData) {
    bool keyProcessed = EM_FALSE;

    if (strEquals(keyEvent->code, "ArrowLeft", 32)) {
        keyboardDirections.left = true;
        keyProcessed = EM_TRUE;
    }

    if (strEquals(keyEvent->code, "ArrowRight", 32)) {
        keyboardDirections.right = true;
        keyProcessed = EM_TRUE;
    }

    if (strEquals(keyEvent->code, "ArrowUp", 32)) {
        keyboardDirections.up = true;
        keyProcessed = EM_TRUE;
    }

    if (strEquals(keyEvent->code, "ArrowDown", 32)) {
        keyboardDirections.down = true;
        keyProcessed = EM_TRUE;
    }

    if (strEquals(keyEvent->code, "Space", 32)) {
        gamepad.aButton = true;
        keyProcessed = EM_TRUE;
    }

    if (strEquals(keyEvent->code, "Escape", 32)) {
        gamepad.backButton = true;
        keyProcessed = EM_TRUE;
    }

    if (strEquals(keyEvent->code, "KeyF", 32)) {
        gamepad.startButton = true;
        keyProcessed = EM_TRUE;
    }

    if (keyboardDirections.left) {
        gamepad.stickX = -1.0f;
    } else if (keyboardDirections.right) {
        gamepad.stickX = 1.0f;
    } else {
        gamepad.stickX = 0.0f;
    }

    if (keyboardDirections.down) {
        gamepad.stickY = -1.0f;
    } else if (keyboardDirections.up) {
        gamepad.stickY = 1.0f;
    } else {
        gamepad.stickY = 0.0f;
    }

    gamepad.keyboard = true;

    return keyProcessed;
}

static EM_BOOL onKeyUp(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData) {
    bool keyProcessed = EM_FALSE;

    if (strEquals(keyEvent->code, "ArrowLeft", 32)) {
        keyboardDirections.left = false;
        keyProcessed = EM_TRUE;
    }
    
    if (strEquals(keyEvent->code, "ArrowRight", 32)) {
        keyboardDirections.right = false;
        keyProcessed = EM_TRUE;
    }
    
    if (strEquals(keyEvent->code, "ArrowUp", 32)) {
        keyboardDirections.up = false;
        keyProcessed = EM_TRUE;
    }
    
    if (strEquals(keyEvent->code, "ArrowDown", 32)) {
        keyboardDirections.down = false;
        keyProcessed = EM_TRUE;
    }
    
    if (strEquals(keyEvent->code, "Space", 32)) {
        gamepad.aButton = false;
        keyProcessed = EM_TRUE;
    }
    
    if (strEquals(keyEvent->code, "Escape", 32)) {
        gamepad.backButton = false;
        keyProcessed = EM_TRUE;
    }
    
    if (strEquals(keyEvent->code, "KeyF", 32)) {
        gamepad.startButton = false;
        keyProcessed = EM_TRUE;
    }

    if (keyboardDirections.left) {
        gamepad.stickX = -1.0f;
    } else if (keyboardDirections.right) {
        gamepad.stickX = 1.0f;
    } else {
        gamepad.stickX = 0.0f;
    }

    if (keyboardDirections.down) {
        gamepad.stickY = -1.0f;
    } else if (keyboardDirections.up) {
        gamepad.stickY = 1.0f;
    } else {
        gamepad.stickY = 0.0f;
    }

    gamepad.keyboard = true;

    return keyProcessed;
}

static EM_BOOL onInitialKeyDown(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData) {
    emscripten_initAudio();

    if (!game_init()) {
        goto EXIT_GAME;
    }

    game_resize(windowState.width, windowState.height);
    emscripten_request_animation_frame_loop(loop, NULL);

    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, EM_FALSE, NULL);
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, EM_FALSE, onKeyDown);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, EM_FALSE, onKeyUp); 

    emscripten_set_element_css_size("#start-text", 0.0, 0.0);

    EXIT_GAME:
    return EM_TRUE;
}

#include <stdio.h>
int32_t main() {
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE gl = 0;
    EmscriptenWebGLContextAttributes attrs = {
        .majorVersion = 2,
        .minorVersion = 0
    };

    double windowWidth = 0.0;
    double windowHeight = 0.0;
    emscripten_get_element_css_size("#canvas", &windowWidth, &windowHeight);
    windowState.width = (int32_t) windowWidth;
    windowState.height = (int32_t) windowHeight;

    printf("%f %f\n", windowWidth, windowHeight);

    emscripten_set_canvas_element_size("#canvas", windowWidth, windowHeight);
    gl = emscripten_webgl_create_context("#canvas", &attrs);
    emscripten_webgl_make_context_current(gl);

    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, EM_FALSE, onInitialKeyDown);

    return 0;
}


void platform_getInput(Game_Input* input) {
    input->lastShoot = input->shoot;
    input->velocity[0] = gamepad.stickX;
    input->velocity[1] = gamepad.stickY;
    input->shoot = gamepad.aButton;
    input->keyboard = gamepad.keyboard;
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