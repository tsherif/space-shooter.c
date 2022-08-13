#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <stdint.h>
#include "../../shared/constants.h"
#include "../../shared/platform-interface.h"
#include "../../shared/debug.h"
#include "web-input.h"
#include "web-audio.h"

static struct {
    int32_t windowWidth;
    int32_t windowHeight;
    float lastFrameTime;
} state = {};

static EM_BOOL gameLoop(double time, void *userData) {
    if (state.lastFrameTime == 0.0f) {
        state.lastFrameTime = (float) time;
    }

    float dt = time - state.lastFrameTime;
    state.lastFrameTime = time;

    web_updateAudio();
    game_update(dt);
    game_draw();

    return EM_TRUE;
}

static EM_BOOL onResize(int eventType, const EmscriptenUiEvent *uiEvent, void *userData) {
    state.windowWidth = uiEvent->windowInnerWidth;
    state.windowHeight = uiEvent->windowInnerHeight;
    game_resize(state.windowWidth, state.windowHeight);

    EMSCRIPTEN_RESULT result = emscripten_set_canvas_element_size("#canvas", (double) state.windowWidth, (double) state.windowHeight);
    DEBUG_ASSERT(result == EMSCRIPTEN_RESULT_SUCCESS, "Failed to set canvas size.");

    return EM_TRUE;
}

int32_t main() {
    EMSCRIPTEN_RESULT result = EMSCRIPTEN_RESULT_SUCCESS;
    
    double windowWidth = 0.0;
    double windowHeight = 0.0;
    result = emscripten_get_element_css_size("#canvas", &windowWidth, &windowHeight);
    DEBUG_ASSERT(result == EMSCRIPTEN_RESULT_SUCCESS, "Failed to get window size.");

    state.windowWidth = (int32_t) windowWidth;
    state.windowHeight = (int32_t) windowHeight;

    result = emscripten_set_canvas_element_size("#canvas", windowWidth, windowHeight);
    DEBUG_ASSERT(result == EMSCRIPTEN_RESULT_SUCCESS, "Failed to set canvas size.");

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE gl = emscripten_webgl_create_context("#canvas", &(EmscriptenWebGLContextAttributes) {
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

    if (!game_init(&(Game_InitOptions) {
        .showInputToStartScreen = true,
        .hideSystemInstructions = true,
        .noAudio = true
    })) {
        return 1;
    }

    game_resize(state.windowWidth, state.windowHeight);
    emscripten_request_animation_frame_loop(gameLoop, NULL);

    return 0;
}

EM_JS(void, platform_userMessage, (const char* message), {
    alert(UTF8ToString(message));
})