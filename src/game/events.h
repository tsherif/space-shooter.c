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

#ifndef _GAME_EVENTS_H_
#define _GAME_EVENTS_H_
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    EVENTS_NONE,
    EVENTS_TITLE,
    EVENTS_SUBTITLE,
    EVENTS_DISPLAY,
    EVENTS_FADE,
    EVENTS_RESTART
} Events_Id;

typedef struct {
    float delay;
    float duration;
    int32_t id;
} Events_Event;

typedef struct {
    Events_Event* events;
    int32_t count;
    float time;
    Events_Id triggeredEvent; // NOTE(Tarek): Only one triggered event at a time! Do we ever need simultaneous events?
    float alpha; // "Factor" between 0-1 of how far into the the current triggered event we are. Can be used for blending.
    bool running;
    bool complete;
    bool loop;
} Events_Sequence;

void events_start(Events_Sequence* sequence);
void events_stop(Events_Sequence* sequence);
void events_beforeFrame(Events_Sequence* sequence, float dt);
bool events_on(Events_Sequence* sequence, int32_t id);

Events_Sequence events_titleControlSequence;
Events_Sequence events_titleSequence;
Events_Sequence events_subtitleSequence;
Events_Sequence events_instructionSequence;
Events_Sequence events_gameOverSequence;
Events_Sequence events_gameOverRestartSequence;
Events_Sequence events_levelTransitionSequence;

#endif
