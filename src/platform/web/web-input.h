////////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
// 
// Copyright (c) 2022 Tarek Sherif
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

#ifndef _WEB_INPUT_H_
#define _WEB_INPUT_H_

//////////////////////////////////////////////////////////////////////////////
// Callbacks for input handling.
//
// - web_onGamepadConnected(): Process gamepad connection.
// - web_onGamepadDisconnected(): Process gamepad disconnection.
// - web_onKeyDown(): Process key down event.
// - web_onKeyUp(): Process key up event.
//////////////////////////////////////////////////////////////////////////////

EM_BOOL web_onGamepadConnected(int eventType, const EmscriptenGamepadEvent *gamepadEvent, void *userData);
EM_BOOL web_onGamepadDisconnected(int eventType, const EmscriptenGamepadEvent *gamepadEvent, void *userData);
EM_BOOL web_onKeyDown(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData);
EM_BOOL web_onKeyUp(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData);

#endif
