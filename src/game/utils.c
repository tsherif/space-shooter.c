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
#include "../shared/constants.h"
#include "../shared/platform-interface.h"
#include "../shared/debug.h"
#include "utils.h"

#define BMP_SIGNATURE 0x4d42
#define BMP_BPP 32
#define BMP_BITFIELD_COMPRESSION 3

#define WAVE_RIFF_SIGNATURE 0x46464952
#define WAVE_TYPE_SIGNATURE 0x45564157
#define WAVE_FMT_SIGNATURE 0x20746d66
#define WAVE_DATA_SIGNATURE 0x61746164
#define WAVE_PCM_FORMAT 1


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
bool utils_bmpToImage(Data_Buffer* imageData, Data_Image* image) {
    uint32_t imageOffset   = *(uint32_t *) (imageData->data + 10);
    int32_t width          = *(int32_t *)  (imageData->data + 18);
    int32_t height         = *(int32_t *)  (imageData->data + 22);

#ifdef SPACE_SHOOTER_DEBUG
    uint16_t type = *(uint16_t *) imageData->data;
    DEBUG_ASSERT(type == BMP_SIGNATURE, "utils_bmpToRgba: Invalid BMP data.");

    uint32_t dibHeaderSize = *(uint32_t *) (imageData->data + 14);
    DEBUG_ASSERT(dibHeaderSize >= 70, "utils_bmpToRgba: Unsupported DIB header.");

    uint16_t bpp           = *(uint16_t *) (imageData->data + 28);
    DEBUG_ASSERT(bpp == BMP_BPP, "utils_bmpToRgba: Unsupported bpp, must be 32.");

    uint32_t compression   = *(uint32_t *) (imageData->data + 30);
    DEBUG_ASSERT(compression == BMP_BITFIELD_COMPRESSION, "utils_bmpToRgba: Unsupported compression, must be BI_BITFIELDS (3).");

    uint32_t redMask       = *(uint32_t *) (imageData->data + 54);
    uint32_t greenMask     = *(uint32_t *) (imageData->data + 58);
    uint32_t blueMask      = *(uint32_t *) (imageData->data + 62);
    uint32_t alphaMask     = *(uint32_t *) (imageData->data + 66);
    DEBUG_ASSERT(redMask == 0x00ff0000 && greenMask == 0x0000ff00 && blueMask == 0x000000ff && alphaMask == 0xff000000, "utils_bmpToRgba: Unsupported pixel layout, must be BGRA.");
#endif

    uint8_t* bmpImage = imageData->data + imageOffset;
    
    int32_t numPixels = width * height;
    uint8_t* data = (uint8_t *) malloc(numPixels * 4);

    if (!data) {
        DEBUG_LOG("utils_bmpToRgba: Unable to allocate image data.");
        return false; 
    }

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
        data[mirrorByteI]     = r;
        data[mirrorByteI + 1] = g;
        data[mirrorByteI + 2] = b;
        data[mirrorByteI + 3] = a;
    }

    image->data = data;
    image->width = width;
    image->height = height;

    return true;
}

// NOTE(Tarek): Hardcoded to load 2-channel 44.1kHz 16-bit data, with RIFF, fmt and data chunks sequential.
bool utils_wavToSound(Data_Buffer* soundData, Data_Buffer* sound) {
    uint32_t fmtSize = *(uint32_t *) (soundData->data + 16);
    int32_t dataOffset = fmtSize + 20;
        
#ifdef SPACE_SHOOTER_DEBUG
    // "RIFF" little-endian
    uint32_t riffType = *(uint32_t *) soundData->data;
    DEBUG_ASSERT(riffType == WAVE_RIFF_SIGNATURE, "utils_wavToSound: Invalid WAVE file. Missing RIFF chunk.");
    
    // fileSize == size of file - 4 bytes each for this and the previous field.
    uint32_t fileSize = *(uint32_t *) (soundData->data + 4);
    DEBUG_ASSERT(fileSize == soundData->size - 8, "utils_wavToSound: Invalid WAVE file. File size incorrect.");

    // "WAVE" little-endian
    uint32_t fileFormat = *(uint32_t *) (soundData->data + 8);
    DEBUG_ASSERT(fileFormat == WAVE_TYPE_SIGNATURE, "utils_wavToSound: Invalid WAVE file. Missing WAVE chunk.");

    // "fmt " little-endian
    uint32_t fmtType = *(uint32_t *) (soundData->data + 12);
    DEBUG_ASSERT(fmtType == WAVE_FMT_SIGNATURE, "utils_wavToSound: Invalid WAVE file. Missing fmt chunk.");
    

    uint16_t formatCode = *(uint16_t *) (soundData->data + 20);            
    uint16_t channels   = *(uint16_t *) (soundData->data + 22);            
    uint32_t rate       = *(uint32_t *) (soundData->data + 24);            
    uint16_t bps        = *(uint16_t *) (soundData->data + 34);
    DEBUG_ASSERT(formatCode == 1, "utils_wavToSound: Invalid audio data. Audio must be uncompressed.");
    DEBUG_ASSERT(channels == SPACE_SHOOTER_AUDIO_CHANNELS, "utils_wavToSound: Invalid audio data. Audio must be stereo.");
    DEBUG_ASSERT(rate == SPACE_SHOOTER_AUDIO_SAMPLE_RATE, "utils_wavToSound: Invalid audio data. Audio must be 44.1k samples per second.");
    DEBUG_ASSERT(bps == SPACE_SHOOTER_AUDIO_BPS, "utils_wavToSound: Invalid audio data. Audio must be 16bps.");

    // "data" little-endian
    uint32_t dataType = *(uint32_t *) (soundData->data + dataOffset);
    DEBUG_ASSERT(dataType == WAVE_DATA_SIGNATURE, "utils_wavToSound: Invalid WAVE file. Missing data chunk.");
#endif

    uint32_t dataSize = *(uint32_t *) (soundData->data + dataOffset + 4);
    sound->size = dataSize;
    sound->data = (uint8_t *) malloc(dataSize);

    if (!sound->data) {
        DEBUG_LOG("utils_wavToSound: Unable to allocate sound data.");
        return false; 
    }

    memcpy(sound->data, soundData->data + dataOffset, dataSize);

    return true;
}
