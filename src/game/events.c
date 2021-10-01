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

#include "events.h"

void events_update(EventsSequence* sequence, float dt) {
    if (!sequence->running) {
        return;
    }

    sequence->triggeredEvents = NULL;
    sequence->triggeredCount = 0;
    float lastTime = sequence->time;
    float currentTime = lastTime + dt;

    float start = 0;
    float end = 0;
    for (int32_t i = 0; i < sequence->count; ++i) {
        EventsEvent* event = sequence->events + i;
        start = end + event->delay;
        end = start + event->duration;
        if (start <= currentTime && end >= lastTime) {
            if (!sequence->triggeredEvents) {
                sequence->triggeredEvents = event;     
            }
            ++sequence->triggeredCount;
            if (event->duration > 0) {
                event->alpha =(currentTime - start) / event->duration;
                if (event->alpha > 1.0f) {
                    event->alpha = 1.0f;
                } 
            } else {
                event->alpha = 0.0f;
            }
        }
    }

    if (lastTime > end) {
        sequence->triggeredEvents = NULL;
        sequence->triggeredCount = 0;
        sequence->running = false;
        sequence->complete = true;    
    }

    sequence->time = currentTime;
}

void events_start(EventsSequence* sequence) {
    if (sequence->running) {
        return;
    }

    sequence->running = true;
    sequence->time = 0;
    sequence->triggeredEvents = NULL;
    sequence->triggeredCount = 0;
    sequence->complete = false;

    events_update(sequence, 0.0f);
}
