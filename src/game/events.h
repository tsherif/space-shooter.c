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
} EventsId;

typedef struct {
    float delay;
    float duration;
    int32_t id;
} EventsEvent;

typedef struct {
    EventsEvent* events;
    int32_t count;
    float time;
    EventsId triggeredEvent; // NOTE(Tarek): Only one triggered event at a time! Do we ever need simultaneous events?
    float alpha; // "Factor" between 0-1 of how far into the the current triggered event we are. Can be used for blending.
    bool running;
    bool complete;
    bool loop;
} EventsSequence;

void events_start(EventsSequence* sequence);
void events_stop(EventsSequence* sequence);
void events_beforeFrame(EventsSequence* sequence, float dt);
bool events_on(EventsSequence* sequence, int32_t id);

EventsSequence events_titleControlSequence;
EventsSequence events_titleSequence;
EventsSequence events_subtitleSequence;
EventsSequence events_gameOverSequence;
EventsSequence events_gameOverRestartSequence;
EventsSequence events_levelTransitionSequence;

#endif
