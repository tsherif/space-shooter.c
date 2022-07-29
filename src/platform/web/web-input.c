#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <stdint.h>
#include "../../shared/platform-interface.h"

#define GAMEPAD_HORIZONTAL_AXIS 0
#define GAMEPAD_VERTICAL_AXIS 1
#define GAMEPAD_A_BUTTON 0
#define GAMEPAD_START_BUTTON 9

static struct {
    int32_t index;
    float stickX;
    float stickY;
    bool aButton;
    bool fKey;     // Fullscreen via gamepad not widely supported, see: https://bugzilla.mozilla.org/show_bug.cgi?id=1740573
    bool keyboard;
} gamepad = {
    .index = -1
};

static struct {
    bool left;
    bool right;
    bool up;
    bool down;
} keyboardDirections;

bool fullscreen;

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

EM_BOOL web_onGamepadConnected(int eventType, const EmscriptenGamepadEvent *gamepadEvent, void *userData) {
    if (gamepad.index == -1) {
        gamepad.index = gamepadEvent->index;
    }

    return EM_TRUE;
}

EM_BOOL web_onGamepadDisconnected(int eventType, const EmscriptenGamepadEvent *gamepadEvent, void *userData) {
    if (gamepad.index == gamepadEvent->index) {
        gamepad.index = -1;
        emscripten_sample_gamepad_data();
        int32_t numGamepads = emscripten_get_num_gamepads();
        EmscriptenGamepadEvent gamepadState = { 0 };
        for (int32_t i = 0; i < numGamepads; ++i) {
            emscripten_get_gamepad_status(i, &gamepadState);
            if (gamepadState.connected) {
                gamepad.index = i;
                break;
            }
        } 
    }

    return EM_TRUE;
}

EM_BOOL web_onKeyDown(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData) {
    EM_BOOL keyProcessed = EM_FALSE;
    bool fKey = false; // Don't set gamepad to keyboard if the f key was hit.

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

    if (strEquals(keyEvent->code, "KeyF", 32)) {
        if (fullscreen) {
            emscripten_exit_fullscreen();
            fullscreen = false;
        } else {
            emscripten_request_fullscreen_strategy("#canvas", EM_FALSE, & (EmscriptenFullscreenStrategy) {
                .scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH,
                .canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_HIDEF,
                .filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_DEFAULT
            });
            fullscreen = true;
        }

        gamepad.fKey = true;
        fKey = true;
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

    if (keyProcessed == EM_TRUE && !fKey) {
        gamepad.keyboard = true;
    }

    return keyProcessed;
}

EM_BOOL web_onKeyUp(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData) {
    EM_BOOL keyProcessed = EM_FALSE;
    bool fKey = false; // Don't set gamepad to keyboard if the f key was hit.

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
    
    if (strEquals(keyEvent->code, "KeyF", 32)) {
        gamepad.fKey = false;
        fKey = true;
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

    if (keyProcessed == EM_TRUE && !fKey) {
        gamepad.keyboard = true;
    }

    return keyProcessed;
}

void platform_getInput(Game_Input* input) {
    if (gamepad.index > -1) {
        emscripten_sample_gamepad_data();
        EmscriptenGamepadEvent gamepadState = { 0 };
        emscripten_get_gamepad_status(gamepad.index, &gamepadState);
        if (gamepadState.connected) {
            float stickX = (float) gamepadState.axis[GAMEPAD_HORIZONTAL_AXIS];
            float stickY = (float) -gamepadState.axis[GAMEPAD_VERTICAL_AXIS];

            if (stickX > -0.1f && stickX < 0.1f) {
                stickX = 0.0f;
            }

            if (stickY > -0.1f && stickY < 0.1f) {
                stickY = 0.0f;
            }

            bool aButton = gamepadState.digitalButton[GAMEPAD_A_BUTTON];

            if (!gamepad.keyboard || stickX != 0.0f || stickY != 0.0f || aButton) {
                gamepad.stickX = stickX;
                gamepad.stickY = stickY;
                gamepad.aButton = aButton;
                gamepad.keyboard = false;
            }
        }
    }

    input->lastShoot = input->shoot;
    input->velocity[0] = gamepad.stickX;
    input->velocity[1] = gamepad.stickY;
    input->shoot = gamepad.aButton;
    input->keyboard = gamepad.keyboard;
}