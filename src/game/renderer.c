#define STB_IMAGE_IMPLEMENTATION
#include "renderer.h"
#include "../../lib/stb_image.h"

static GLuint pixelSizeLocation;
static GLuint panelPixelSizeLocation;
static GLuint spriteSheetLocation;
static GLuint spriteSheetDimensionsLocation;
static GLuint panelIndexLocation;
static GLuint pixelOffsetLocation;
static GLuint whiteOutLocation;
static GLuint scaleLocation;

static uint16_t windowWidth;
static uint16_t windowHeight;
static uint16_t gameWidth;
static uint16_t gameHeight;
static uint16_t gameScreenOffsetX;
static uint16_t gameScreenOffsetY;
static uint16_t gameScreenWidth;
static uint16_t gameScreenHeight;

void renderer_init(int width, int height) {
    gameWidth = width;
    gameHeight = height;

    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);

    const char* vsSource = "#version 450\n"
    "layout (location=0) in vec2 position;\n"
    "uniform vec2 pixelOffset;\n"
    "uniform vec2 pixelSize;\n"
    "uniform vec2 panelPixelSize;\n"
    "uniform vec2 panelIndex;\n"
    "uniform vec2 spriteSheetDimensions;\n"
    "uniform float scale;\n"
    "out vec2 vUV;\n"
    "void main() {\n"
    "    vec2 uv = position;\n"
    "    vUV = (uv + panelIndex) / spriteSheetDimensions;\n"
    "    vec2 clipOffset = pixelOffset * pixelSize - 1.0;\n"
    "    gl_Position = vec4((position * panelPixelSize * pixelSize * scale + clipOffset) * vec2(1.0, -1.0), 0.0, 1.0);\n"
    "}\n";

    const char* fsSource = "#version 450\n"
    "in vec2 vUV;\n"
    "uniform sampler2D spriteSheet;\n"
    "uniform bool whiteOut;"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "    fragColor = texture(spriteSheet, vUV);\n"
    "    if (whiteOut) fragColor.rgb = vec3(1.0);\n"
    "    fragColor.rgb *= fragColor.a;\n"
    "}\n";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vsSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fsSource, NULL);
    glCompileShader(fragmentShader);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint result;
    glGetProgramiv(program, GL_LINK_STATUS, &result);

    // TODO(Tarek): Get rid of these.
    if (result != GL_TRUE) {
        MessageBoxA(NULL, "Program failed to link!", "FAILURE", MB_OK);
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
        if (result != GL_TRUE) {
            MessageBoxA(NULL, "Vertex shader failed to compile!", "FAILURE", MB_OK);
        }
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
        if (result != GL_TRUE) {
            MessageBoxA(NULL, "Fragment shader failed to compile!", "FAILURE", MB_OK);
        }
    }


    glUseProgram(program);
    pixelSizeLocation = glGetUniformLocation(program, "pixelSize");
    panelPixelSizeLocation = glGetUniformLocation(program, "panelPixelSize");
    spriteSheetLocation = glGetUniformLocation(program, "spriteSheet");
    spriteSheetDimensionsLocation = glGetUniformLocation(program, "spriteSheetDimensions");
    panelIndexLocation = glGetUniformLocation(program, "panelIndex");
    pixelOffsetLocation = glGetUniformLocation(program, "pixelOffset");
    whiteOutLocation = glGetUniformLocation(program, "whiteOut");
    scaleLocation = glGetUniformLocation(program, "scale");

    glUniform2f(pixelSizeLocation, 2.0f / width, 2.0f / height);

    float positions[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f,  1.0f
    };

    GLuint spriteArray;
    glGenVertexArrays(1, &spriteArray);
    glBindVertexArray(spriteArray);

    GLuint positionBuffer;
    glGenBuffers(1, &positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);
    glUniform1i(spriteSheetLocation, 0);
}

bool renderer_loadTexture(const char* fileName, GLuint* texture) {
    int imageWidth, imageHeight, imageChannels;
    uint8_t *imageData = stbi_load(fileName, &imageWidth, &imageHeight, &imageChannels, 4);

    if (!imageData) {
        return false;
    }

    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    stbi_image_free(imageData);

    return true;
}

void renderer_resize(int width, int height) {
    windowWidth = width;
    windowHeight = height;

    float aspect = (float) gameWidth / gameHeight;
    gameScreenWidth = width;
    gameScreenHeight = (uint16_t) (width / aspect);

    if (gameScreenHeight > height) {
        gameScreenHeight = height;
        gameScreenWidth = (uint16_t) (aspect * gameScreenHeight);
    }

    gameScreenOffsetX = (width - gameScreenWidth) / 2;
    gameScreenOffsetY = (height - gameScreenHeight) / 2;

    glViewport(gameScreenOffsetX, gameScreenOffsetY, gameScreenWidth, gameScreenHeight);
}

void renderer_beforeFrame(void) {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glScissor(0, 0, windowWidth, windowHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glScissor(gameScreenOffsetX, gameScreenOffsetY, gameScreenWidth, gameScreenHeight);
    glClear(GL_COLOR_BUFFER_BIT);
}

void renderer_draw(Renderer_RenderList* list, uint8_t count) {
    if (count == 0) {
        return;
    }


    glBindTexture(GL_TEXTURE_2D, list->texture);
    glUniform2fv(panelPixelSizeLocation, 1, list->panelDims);
    glUniform2fv(spriteSheetDimensionsLocation, 1, list->sheetDims);

    for (uint8_t i = 0; i < count; ++i) {
        glUniform2fv(pixelOffsetLocation, 1, list->positions + i * 2);
        glUniform2fv(panelIndexLocation, 1, list->currentSpritePanels + i * 2);
        glUniform1f(whiteOutLocation, list->whiteOut[i]);
        float scale = 1.0f;
        if (list->scale[0]) {
            scale = list->scale[0];
        }
        glUniform1f(scaleLocation, scale);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
}
