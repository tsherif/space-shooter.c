#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <stdint.h>
#include "../../shared/constants.h"
#include "../../shared/platform-interface.h"
#include "../../shared/debug.h"
#include "web-input.h"
#include "web-audio.h"

static struct {
    int32_t width;
    int32_t height;
} windowState = {};

static float lastTime = 0.0f;

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

    EMSCRIPTEN_RESULT result = emscripten_set_canvas_element_size("#canvas", (double) windowState.width, (double) windowState.height);
    DEBUG_ASSERT(result == EMSCRIPTEN_RESULT_SUCCESS, "Failed to set canvas size.");

    return EM_TRUE;
}

int32_t main() {
    EMSCRIPTEN_RESULT result = EMSCRIPTEN_RESULT_SUCCESS;
    
    double windowWidth = 0.0;
    double windowHeight = 0.0;
    result = emscripten_get_element_css_size("#canvas", &windowWidth, &windowHeight);
    DEBUG_ASSERT(result == EMSCRIPTEN_RESULT_SUCCESS, "Failed to get window size.");

    windowState.width = (int32_t) windowWidth;
    windowState.height = (int32_t) windowHeight;

    result = emscripten_set_canvas_element_size("#canvas", windowWidth, windowHeight);
    DEBUG_ASSERT(result == EMSCRIPTEN_RESULT_SUCCESS, "Failed to set canvas size.");

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE gl = emscripten_webgl_create_context("#canvas", & (EmscriptenWebGLContextAttributes) {
        .majorVersion = 2,
        .minorVersion = 0
    });

    if (gl == 0) {
        platform_userMessage("Failed to initialize WebGL.");
        return 1;
    }

    result = emscripten_webgl_make_context_current(gl);
    DEBUG_ASSERT(result == EMSCRIPTEN_RESULT_SUCCESS, "Failed to activate WebGL context.");

    web_initInputHandlers();
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

EM_JS(void, platform_userMessage, (const char* message), {
    alert(UTF8ToString(message));
})