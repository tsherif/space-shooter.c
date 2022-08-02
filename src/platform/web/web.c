#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <stdint.h>
#include "../../shared/constants.h"
#include "../../shared/platform-interface.h"
#include "web-input.h"
#include "web-audio.h"

static struct {
    int32_t width;
    int32_t height;
} windowState = {};

float lastTime = 0.0f;


static EM_BOOL loop(double time, void *userData) {
    if (lastTime == 0.0f) {
        lastTime = (float) time;
    }

    float dt = time - lastTime;
    lastTime = time;

    web_updateAudio();
    game_update(dt);
    game_draw();

    return EM_TRUE;
}

static EM_BOOL onResize(int eventType, const EmscriptenUiEvent *uiEvent, void *userData) {
    windowState.width = uiEvent->windowInnerWidth;
    windowState.height = uiEvent->windowInnerHeight;
    game_resize(windowState.width, windowState.height);
    emscripten_set_canvas_element_size("#canvas", (double) windowState.width, (double) windowState.height);

    return EM_TRUE;
}

int32_t main() {
    double windowWidth = 0.0;
    double windowHeight = 0.0;
    emscripten_get_element_css_size("#canvas", &windowWidth, &windowHeight);
    windowState.width = (int32_t) windowWidth;
    windowState.height = (int32_t) windowHeight;

    emscripten_set_canvas_element_size("#canvas", windowWidth, windowHeight);
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE gl = emscripten_webgl_create_context("#canvas", & (EmscriptenWebGLContextAttributes) {
        .majorVersion = 2,
        .minorVersion = 0
    });
    emscripten_webgl_make_context_current(gl);

    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, EM_FALSE, web_onKeyDown);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, EM_FALSE, web_onKeyUp); 
    emscripten_set_gamepadconnected_callback(NULL, EM_FALSE, web_onGamepadConnected);
    emscripten_set_gamepaddisconnected_callback(NULL, EM_FALSE, web_onGamepadDisconnected);
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_FALSE, onResize);

    if (!game_init(& (Game_InitOptions) {
        .showInputToStartScreen = true,
        .hideSystemInstructions = true,
        .noMusic = true
    })) {
        return 1;
    }
    game_resize(windowState.width, windowState.height);
    emscripten_request_animation_frame_loop(loop, NULL);

    return 0;
}