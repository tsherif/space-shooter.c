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

// NOTE(Tarek): Hardcoded to load 32bpp BGRA  
static bool bmpToImage(DataBuffer* imageBuffer, DataImage* image) {
    uint16_t type = *(uint16_t *) imageBuffer->data;

    if (type != 0x4d42) {
        platform_debugLog("utils_bmpToRgba: Invalid BMP data.");
        return false;
    }
    uint32_t imageOffset   = *(uint32_t *) (imageBuffer->data + 10);
    
    uint32_t dibHeaderSize = *(uint32_t *) (imageBuffer->data + 14);
    if (dibHeaderSize < 70) {
        platform_debugLog("utils_bmpToRgba: Unsupported DIB header.");
        return false;
    }

    int32_t width          = *(int32_t *)  (imageBuffer->data + 18);
    int32_t height         = *(int32_t *)  (imageBuffer->data + 22);

    uint16_t bpp           = *(uint16_t *) (imageBuffer->data + 28);
    if (bpp != 32) {
        platform_debugLog("utils_bmpToRgba: Unsupported bpp, must be 32.");
        return false;
    }

    uint32_t compression   = *(uint32_t *) (imageBuffer->data + 30);
    if (compression != 3) {
        platform_debugLog("utils_bmpToRgba: Unsupported compression, must be BI_BITFIELDS (3).");
        return false;
    }

    uint32_t redMask       = *(uint32_t *) (imageBuffer->data + 54);
    uint32_t greenMask     = *(uint32_t *) (imageBuffer->data + 58);
    uint32_t blueMask      = *(uint32_t *) (imageBuffer->data + 62);
    uint32_t alphaMask     = *(uint32_t *) (imageBuffer->data + 66);

    if (redMask != 0x00ff0000 || greenMask != 0x0000ff00 || blueMask != 0x000000ff || alphaMask != 0xff000000) {
        platform_debugLog("utils_bmpToRgba: Unsupported pixel layout, must be BGRA.");
        return false;
    }


    uint8_t* bmpImage = imageBuffer->data + imageOffset;
    
    int32_t numPixels = width * height;
    uint8_t* imageData = (uint8_t *) malloc(numPixels * 4);

    for (int32_t i = 0; i < numPixels;  ++i) {
        int32_t row = i / width;
        int32_t col = i % width;
        int32_t mirrorRow = height - row - 1;
        int32_t mirrorI = mirrorRow * width + col;

        int32_t byteI = i * 4;
        uint8_t b = bmpImage[byteI];
        uint8_t g = bmpImage[byteI + 1];
        uint8_t r = bmpImage[byteI + 2];
        uint8_t a = bmpImage[byteI + 3];

        int32_t mirrorByteI = mirrorI * 4;
        imageData[mirrorByteI]     = r;
        imageData[mirrorByteI + 1] = g;
        imageData[mirrorByteI + 2] = b;
        imageData[mirrorByteI + 3] = a;
    }

    image->data = imageData;
    image->size = numPixels * 4;
    image->width = width;
    image->height = height;

    return true;
}

void renderer_init(int width, int height) {
    gameWidth = width;
    gameHeight = height;

    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);

    const char* vsSource = "#version 450\n"
    "layout (location=0) in vec2 position;\n"
    "layout (location=1) in vec2 pixelOffset;\n"
    "layout (location=2) in vec2 panelIndex;\n"
    "layout (location=3) in float scale;\n"
    "layout (location=4) in float alpha;\n"
    "layout (location=5) in float whiteOut;\n"
    "uniform vec2 panelPixelSize;\n"
    "uniform vec2 spriteSheetDimensions;\n"
    "uniform vec2 pixelSize;\n"
    "out vec2 vUV;\n"
    "out float vAlpha;\n"
    "out float vWhiteOut;"
    "void main() {\n"
    "    vec2 uv = position;\n"
    "    vUV = (uv + panelIndex) / spriteSheetDimensions;\n"
    "    vWhiteOut = whiteOut;\n"
    "    vAlpha = alpha;\n"
    "    vec2 clipOffset = pixelOffset * pixelSize - 1.0;\n"
    "    gl_Position = vec4((position * panelPixelSize * pixelSize * scale + clipOffset) * vec2(1.0, -1.0), 0.0, 1.0);\n"
    "}\n";

    const char* fsSource = "#version 450\n"
    "in vec2 vUV;\n"
    "in float vAlpha;"
    "in float vWhiteOut;"
    "uniform sampler2D spriteSheet;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "    fragColor = texture(spriteSheet, vUV);\n"
    "    if (vWhiteOut != 0.0) fragColor.rgb = vec3(1.0);\n"
    "    fragColor.a *= vAlpha;\n"
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

    if (result != GL_TRUE) {
        platform_debugLog("Program failed to link!");
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
        char buffer[1024];
        if (result != GL_TRUE) {
            platform_debugLog("Vertex shader failed to compile!");
            glGetShaderInfoLog(vertexShader, 1024, NULL, buffer);
            platform_debugLog(buffer);
        }
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
        if (result != GL_TRUE) {
            platform_debugLog("Fragment shader failed to compile!");
            glGetShaderInfoLog(fragmentShader, 1024, NULL, buffer);
            platform_debugLog(buffer);
        }
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
    glVertexAttribPointer(5, 1, GL_UNSIGNED_BYTE, GL_FALSE, 0, NULL);
    glVertexAttribDivisor(5, 1);
    glEnableVertexAttribArray(5);
}

void renderer_initDataTexture(uint8_t* data, int32_t width, int32_t height, GLuint* texture) {
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

bool renderer_initBmpTexture(const char* fileName, GLuint* texture) {
    DataBuffer imageBuffer = { 0 };
    DataImage image = { 0 };

    if (!platform_loadBinFile(fileName, &imageBuffer)) {
        return false;
    }

    if (!bmpToImage(&imageBuffer, &image)) {
        data_freeBuffer(&imageBuffer);
        return false;
    }

    renderer_initDataTexture(image.data, image.width, image.height, texture);

    data_freeImage(&image);
    data_freeBuffer(&imageBuffer);

    return true;
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

void renderer_draw(Renderer_RenderList* list) {
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
    glBufferData(GL_ARRAY_BUFFER, list->count * sizeof(uint8_t), list->whiteOut, GL_DYNAMIC_DRAW);

    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, list->count);
}
