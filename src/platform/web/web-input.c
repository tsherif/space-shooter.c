#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <stdint.h>
#include <math.h>
#include "../../shared/constants.h"
#include "../../shared/platform-interface.h"
#include "../../shared/debug.h"
#include "web-audio.h"

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

static bool audioInitialized = false;
static bool gameStarted = false;

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

static void initializeAudio(void) {
    if (!web_initAudio()) {
        platform_userMessage("Failed to initialize audio.");
    }
    audioInitialized = true; // Don't want to keep trying if it fails.
}

static void findGamepad(void) {
    gamepad.index = -1;
    
    if (emscripten_sample_gamepad_data() != EMSCRIPTEN_RESULT_SUCCESS) {
        gamepad.keyboard = true;
        return;
    }
    
    int32_t numGamepads = emscripten_get_num_gamepads();
    for (int32_t i = 0; i < numGamepads; ++i) {
        EmscriptenGamepadEvent gamepadState = { 0 };
        if (
            emscripten_get_gamepad_status(i, &gamepadState) == EMSCRIPTEN_RESULT_SUCCESS && 
            gamepadState.connected
        ) {
            gamepad.index = i;
            break;
        }
    }

    gamepad.keyboard = gamepad.index = -1;
}

static EM_BOOL onGamepadConnected(int eventType, const EmscriptenGamepadEvent *gamepadEvent, void *userData) {
    if (gamepad.index == -1) {
        gamepad.index = gamepadEvent->index;
    }

    return EM_TRUE;
}

static EM_BOOL onGamepadDisconnected(int eventType, const EmscriptenGamepadEvent *gamepadEvent, void *userData) {
    if (gamepad.index == gamepadEvent->index) {
        findGamepad();   
    }

    return EM_TRUE;
}

static EM_BOOL onKeyDown(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData) {
    EM_BOOL keyProcessed = EM_FALSE;

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
        if (!audioInitialized) {
            initializeAudio();
        }
        gamepad.aButton = true;
        keyProcessed = EM_TRUE;
    }

    if (strEquals(keyEvent->code, "KeyF", 32)) {
        EMSCRIPTEN_RESULT result = EMSCRIPTEN_RESULT_SUCCESS;
        EmscriptenFullscreenChangeEvent status = { 0 };
        result = emscripten_get_fullscreen_status(&status);
        if (result == EMSCRIPTEN_RESULT_SUCCESS) {
            if (status.isFullscreen) {
                emscripten_exit_fullscreen();
            } else {
                result = emscripten_request_fullscreen_strategy("#canvas", EM_FALSE, & (EmscriptenFullscreenStrategy) {
                    .scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH,
                    .canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_HIDEF,
                    .filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_NEAREST
                }); 
            }
        }

        if (result != EMSCRIPTEN_RESULT_SUCCESS) {
            platform_userMessage("Fullscreen not supported.");
        }

        gamepad.fKey = true;
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

    if (keyProcessed == EM_TRUE && !gamepad.fKey && gameStarted) {
        gamepad.keyboard = true;
    }

    return keyProcessed;
}

static EM_BOOL onKeyUp(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData) {
    EM_BOOL keyProcessed = EM_FALSE;

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

    return keyProcessed;
}

void web_initInputHandlers(void) {
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, EM_FALSE, onKeyDown);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, EM_FALSE, onKeyUp);
    
    if (
        emscripten_set_gamepadconnected_callback(NULL, EM_FALSE, onGamepadConnected) != EMSCRIPTEN_RESULT_SUCCESS || 
        emscripten_set_gamepaddisconnected_callback(NULL, EM_FALSE, onGamepadDisconnected)  != EMSCRIPTEN_RESULT_SUCCESS
    ) {
        DEBUG_LOG("Gamepad not supported.");
        gamepad.keyboard = true;
    }
}

void platform_getInput(Game_Input* input) {
    if (!gameStarted) {
        input->shoot = gamepad.aButton;

        if (input->shoot) {
            game_startMusic();
            gameStarted = true;
        }

        return;
    }

    if (gamepad.index > -1) {
        EmscriptenGamepadEvent gamepadState = { 0 };
        if (
            emscripten_sample_gamepad_data() == EMSCRIPTEN_RESULT_SUCCESS &&
            emscripten_get_gamepad_status(gamepad.index, &gamepadState) == EMSCRIPTEN_RESULT_SUCCESS &&
            gamepadState.connected
        ) {
            float stickX = 0.0f;
            float stickY = 0.0f;
            float x = (float) gamepadState.axis[GAMEPAD_HORIZONTAL_AXIS];
            float y = (float) gamepadState.axis[GAMEPAD_VERTICAL_AXIS];

            float mag = (float) sqrt(x * x + y * y);
            x /= mag;
            y /= mag;

            if (mag > SPACE_SHOOTER_GAMEPAD_STICK_DEADZONE_FLOAT) {
                mag -= SPACE_SHOOTER_GAMEPAD_STICK_DEADZONE_FLOAT;
                mag /= 1.0f - SPACE_SHOOTER_GAMEPAD_STICK_DEADZONE_FLOAT;
                stickX = x * mag;
                stickY = -y * mag;
            } else {
                stickX = 0.0f;
                stickY = 0.0f;
            }

            bool aButton = gamepadState.digitalButton[GAMEPAD_A_BUTTON];

            if (!gamepad.keyboard || stickX != 0.0f || stickY != 0.0f || aButton) {
                gamepad.stickX = stickX;
                gamepad.stickY = stickY;
                gamepad.aButton = aButton;
                gamepad.keyboard = false;
            }
        } else {
            findGamepad();
        }
    }

    input->lastShoot = input->shoot;
    input->velocity[0] = gamepad.stickX;
    input->velocity[1] = gamepad.stickY;
    input->shoot = gamepad.aButton;
    input->keyboard = gamepad.keyboard;
}