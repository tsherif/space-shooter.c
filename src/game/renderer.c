////////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
// 
// Copyright (c) 2021 Tarek Sherif
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
////////////////////////////////////////////////////////////////////////////////////

#include <malloc.h>
#include "renderer.h"
#include "../shared/data.h"
#include "../shared/platform-interface.h"
#include "../shared/debug.h"

static GLuint pixelSizeUniform;
static GLuint spriteSheetUniform;
static GLuint panelPixelSizeUniform;
static GLuint spriteSheetDimensionsUniform;

static GLuint panelIndexBuffer;
static GLuint pixelOffsetBuffer;
static GLuint scaleBuffer;
static GLuint whiteOutBuffer;
static GLuint alphaBuffer;

static int32_t windowWidth;
static int32_t windowHeight;
static int32_t gameWidth;
static int32_t gameHeight;
static int32_t gameScreenOffsetX;
static int32_t gameScreenOffsetY;
static int32_t gameScreenWidth;
static int32_t gameScreenHeight;

bool renderer_init(int width, int height) {
    gameWidth = width;
    gameHeight = height;

    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);

    DataBuffer vsSource = { 0 };
    DataBuffer fsSource = { 0 };

    if (!platform_loadFile("assets/shaders/vs.glsl", &vsSource, true) ||
        !platform_loadFile("assets/shaders/fs.glsl", &fsSource, true)) {
        DEBUG_LOG("renderer_init: Unable to load shaders.");
        return false;
    }

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vsSource.data, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fsSource.data, NULL);
    glCompileShader(fragmentShader);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    data_freeBuffer(&vsSource);
    data_freeBuffer(&fsSource);

    GLint result;
    glGetProgramiv(program, GL_LINK_STATUS, &result);

    if (result != GL_TRUE) {
        DEBUG_LOG("Program failed to link!");
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
        char buffer[1024];
        if (result != GL_TRUE) {
            DEBUG_LOG("Vertex shader failed to compile!");
            glGetShaderInfoLog(vertexShader, 1024, NULL, buffer);
            DEBUG_LOG(buffer);
        }
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
        if (result != GL_TRUE) {
            DEBUG_LOG("Fragment shader failed to compile!");
            glGetShaderInfoLog(fragmentShader, 1024, NULL, buffer);
            DEBUG_LOG(buffer);
        }

        return false;
    }

    glUseProgram(program);

    pixelSizeUniform = glGetUniformLocation(program, "pixelSize");
    panelPixelSizeUniform = glGetUniformLocation(program, "panelPixelSize");
    spriteSheetDimensionsUniform = glGetUniformLocation(program, "spriteSheetDimensions");
    spriteSheetUniform = glGetUniformLocation(program, "spriteSheet");

    glUniform2f(pixelSizeUniform, 2.0f / width, 2.0f / height);
    glUniform1i(spriteSheetUniform, 0);

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

    // Instanced attributes

    glGenBuffers(1, &pixelOffsetBuffer);
    glGenBuffers(1, &pixelOffsetBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, pixelOffsetBuffer);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glVertexAttribDivisor(1, 1);
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &panelIndexBuffer);
    glGenBuffers(1, &panelIndexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, panelIndexBuffer);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glVertexAttribDivisor(2, 1);
    glEnableVertexAttribArray(2);

    glGenBuffers(1, &scaleBuffer);
    glGenBuffers(1, &scaleBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, scaleBuffer);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, NULL);
    glVertexAttribDivisor(3, 1);
    glEnableVertexAttribArray(3);

    glGenBuffers(1, &alphaBuffer);
    glGenBuffers(1, &alphaBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, alphaBuffer);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 0, NULL);
    glVertexAttribDivisor(4, 1);
    glEnableVertexAttribArray(4);

    glGenBuffers(1, &whiteOutBuffer);
    glGenBuffers(1, &whiteOutBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, whiteOutBuffer);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, 0, NULL);
    glVertexAttribDivisor(5, 1);
    glEnableVertexAttribArray(5);

    return true;
}

void renderer_initTexture(GLuint* texture, uint8_t* data, int32_t width, int32_t height) {
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void renderer_resize(int32_t width, int32_t height) {
    windowWidth = width;
    windowHeight = height;

    float aspect = (float) gameWidth / gameHeight;
    gameScreenWidth = width;
    gameScreenHeight = (int32_t) (width / aspect);

    if (gameScreenHeight > height) {
        gameScreenHeight = height;
        gameScreenWidth = (int32_t) (aspect * gameScreenHeight);
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

void renderer_draw(RendererList* list) {
    if (list->count == 0) {
        return;
    }

    glBindTexture(GL_TEXTURE_2D, list->texture);
    glUniform2fv(panelPixelSizeUniform, 1, list->sprite->panelDims);
    glUniform2fv(spriteSheetDimensionsUniform, 1, list->sprite->sheetDims);

    glBindBuffer(GL_ARRAY_BUFFER, pixelOffsetBuffer);
    glBufferData(GL_ARRAY_BUFFER, list->count * 2 * sizeof(float), list->position, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, panelIndexBuffer);
    glBufferData(GL_ARRAY_BUFFER, list->count * 2 * sizeof(float), list->currentSpritePanel, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, scaleBuffer);
    glBufferData(GL_ARRAY_BUFFER, list->count * sizeof(float), list->scale, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, alphaBuffer);
    glBufferData(GL_ARRAY_BUFFER, list->count * sizeof(float), list->alpha, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, whiteOutBuffer);
    glBufferData(GL_ARRAY_BUFFER, list->count * sizeof(float), list->whiteOut, GL_DYNAMIC_DRAW);

    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, list->count);
}
