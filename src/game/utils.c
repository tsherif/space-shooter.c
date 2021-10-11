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

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../shared/platform-interface.h"
#include "utils.h"

void utils_init(void) {
    srand((uint32_t) time(NULL));
}

float utils_lerp(float min, float max, float t) {
    return min + t * (max - min);
}

float utils_randomRange(float min, float max) {
    return utils_lerp(min, max, (float) rand() / ((uint64_t)RAND_MAX + 1));
}

bool utils_boxCollision(float min1[2], float max1[2], float min2[2], float max2[2], float scale) {
     float correctionFactor = (1.0f - scale) * 0.5f;
     float xCorrection1 = (max1[0] - min1[0]) * correctionFactor;
     float yCorrection1 = (max1[1] - min1[1]) * correctionFactor;
     float xCorrection2 = (max2[0] - min2[0]) * correctionFactor;
     float yCorrection2 = (max2[1] - min2[1]) * correctionFactor;
 
    if (min1[0] + xCorrection1 > max2[0] - xCorrection2) {
        return false;
    }

    if (min2[0] + xCorrection2 > max1[0] - xCorrection1) {
        return false;
    }

    if (min1[1] + yCorrection1 > max2[1] - yCorrection2) {
        return false;
    }

    if (min2[1] + yCorrection2 > max1[1] - yCorrection1) {
        return false;
    }

    return true;
}

void utils_uintToString(uint32_t n, char* buffer, int32_t bufferLength) {
    buffer[bufferLength - 1] = '\0';
    int32_t i = bufferLength - 2;

    while (i >= 0) {
        buffer[i] = '0' + (n % 10);
        n /= 10;
        --i;
    }
}

// NOTE(Tarek): Hardcoded to load 32bpp BGRA  
bool utils_bmpToImage(DataBuffer* imageBuffer, DataImage* image) {
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
    image->width = width;
    image->height = height;

    return true;
}

// TODO(Tarek): Check format. Should be 2-channel 44.1kHz
bool utils_wavToSound(DataBuffer* soundData, DataBuffer* sound) {
    int32_t offset = 0;
    uint32_t chunkType = 0;
    uint32_t chunkSize = 0;
    uint32_t fileFormat = 0;

    chunkType = *(uint32_t *) soundData->data;
    offset +=  2 * sizeof(uint32_t);

    if (chunkType != 0x46464952) { // "RIFF" little-endian
        return false;
    }

    fileFormat = *(uint32_t *) (soundData->data + offset);
    offset += sizeof(uint32_t);

    if (fileFormat != 0x45564157) { // "WAVE" little-endian
        return false;
    }

    while (offset + 2 * sizeof(uint32_t) < soundData->size) {
        chunkType = *(uint32_t *) (soundData->data + offset);
        offset +=  sizeof(uint32_t);

        chunkSize = *(uint32_t *) (soundData->data + offset);
        offset +=  sizeof(uint32_t);

        if (offset + chunkSize > soundData->size) {
            return false;
        }

        if (chunkType == 0x61746164) { // "data" little-endian
            sound->size = chunkSize;
            sound->data = (uint8_t *) malloc(chunkSize);
            memcpy(sound->data, soundData->data + offset, chunkSize);
            break;
        }

        offset += chunkSize;
    }

    return true;
}
