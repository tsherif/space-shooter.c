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

#ifndef _PLATFORM_INTERFACE_H_
#define _PLATFORM_INTERFACE_H_
#include <stdbool.h>
#include <stdint.h>
#include "data.h"

// Must be implemented by game, to be used by platform layer.
typedef struct {
	float velocity[2];
	bool shoot;
} GameInput;

bool game_init(void);
void game_update(float elapsedTime); // In milliseconds
void game_draw(void);
void game_resize(int width, int height);

// Must be implemented by platform layer, to be used by game.
void platform_getInput(GameInput* input);
void platform_playSound(DataBuffer* sound, bool loop);
void platform_debugLog(const char* message);
void platform_userMessage(const char* message);
bool platform_loadFile(const char* fileName, DataBuffer* buffer, bool nullTerminate);

#endif
