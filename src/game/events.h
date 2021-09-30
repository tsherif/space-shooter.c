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

#define EVENTS_MAX_SEQUENCE 32

typedef struct {
    int32_t delay;
    int32_t duration;
    int32_t id;
    float alpha;
} EventsEvent;

typedef struct {
    EventsEvent events[EVENTS_MAX_SEQUENCE];
    int32_t time;
    int32_t count;
    int32_t current[EVENTS_MAX_SEQUENCE];
    int32_t currentCount;
    bool running;
    bool complete;
} EventsSequence;

void events_start(EventsSequence* sequence);
void events_update(EventsSequence* sequence, int32_t time);
bool events_on(EventsSequence* sequence, int32_t id, float* alpha);

#endif
