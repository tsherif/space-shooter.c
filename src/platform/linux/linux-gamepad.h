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

//////////////////////////////////////////////////////////////
// Linux gamepad detection, querying and cleanup functions.
//////////////////////////////////////////////////////////////

#ifndef _LINUX_GAMEPAD_H_
#define _LINUX_GAMEPAD_H_

#include <stdbool.h>
#include "../../shared/platform-interface.h"

//////////////////////////////////////////////////////////////////////
// Linux_Gamepad contains data about the current input state. Note
// that "gamepad" here is an abstraction which can also represent
// input from the keyboard.
//
// Members:
// - stickX: Horizontal input from the left thumb stick, normalized
//      a value between -1.0 and 1.0.
// - stickY: Vertical input from the left thumb stick, normalized
//      a value between -1.0 and 1.0.
// - aButton: Whether the A button being pressed.
// - startButton: Whether the Start button being pressed.
// - backButton: Whether the Back button being pressed.
// - keyboard: Whether this input came from the keyboard.
//////////////////////////////////////////////////////////////////////

typedef struct {
    float stickX;
    float stickY;
    bool aButton;
    bool startButton;
    bool backButton;
    bool keyboard;
    bool fast_shoot;
} Linux_Gamepad;

//////////////////////////////////////////////////////////////////
// Linux gamepad controls functions.
//
// - linux_detectGamepad(): Attempt to connect to a gamepad.
// - linux_updateGamepad(): Capture current gamepad state.
// - linux_closeGamepad(): Release gamepad resources.
//////////////////////////////////////////////////////////////////

void linux_detectGamepad(void);
void linux_updateGamepad(Linux_Gamepad* gamepad);
void linux_closeGamepad(void);

#endif
