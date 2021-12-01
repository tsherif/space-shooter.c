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

#ifndef _GAME_UTILS_H_
#define _GAME_UTILS_H_
#include <stdint.h>
#include <stdbool.h>
#include "../shared/data.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Collection of smaller utility functions.
//
// - utils_init(): initialization (for random number generator).
// - utils_lerp(): linear interpolation between min and max
// - utils_randomRange(): random float between min and max
// - utils_boxCollision(): detect collision between boxes defined by min1/max1 and min2/max2,
//		scaled by `scale` multiplicative factor (used to make collisions more forgiving).
// - utils_uintToString(uint32_t n, char* buffer, int32_t bufferLength): convert a unsigned
// 		integer to a string. 
// - utils_bmpToImage(): Parse the image data out of a BMP file. Note this function is hardcoded to 
//		load 32bpp, uncompressed BGRA data (the format output by gimp).
// - utils_wavToSound(): Parse audio data out of a WAVE file. Note this function is hardcoded
//		to load 2-channel, 44.1kHz, 16-bit data, with the RIFF, fmt and data chunks in sequential 
//		order.
//////////////////////////////////////////////////////////////////////////////////////////////////////

void utils_init(void);
float utils_lerp(float min, float max, float t);
float utils_randomRange(float min, float max);
bool utils_boxCollision(float min1[2], float max1[2], float min2[2], float max2[2], float scale);
void utils_uintToString(uint32_t n, char* buffer, int32_t bufferLength); 
bool utils_bmpToImage(Data_Buffer* imageData, Data_Image* image);
bool utils_wavToSound(Data_Buffer* soundData, Data_Buffer* sound);

#endif