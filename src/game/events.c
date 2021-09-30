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

void events_update(EventsSequence* sequence, int32_t time) {
    if (!sequence->running) {
        return;
    }

    sequence->currentCount = 0;
    int32_t lastTime = sequence->time;
    int32_t currentTime = lastTime + time;

    int32_t start = 0;
    int32_t end = 0;
    for (int32_t i = 0; i < sequence->count; ++i) {
        EventsEvent* event = sequence->events + i;
        start = end + event->delay;
        end = start + event->duration;
        if (start <= currentTime && end >= lastTime) {
            sequence->current[sequence->currentCount] = i;
            ++sequence->currentCount;
            if (event->duration > 0) {
                event->alpha = (float) (currentTime - start) / event->duration;
                if (event->alpha > 1.0f) {
                    event->alpha = 1.0f;
                } 
            } else {
                event->alpha = 0.0f;
            }
        }
    }

    if (lastTime > end) {
        sequence->currentCount = false;
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
    sequence->currentCount = 0;
    sequence->complete = false;

    events_update(sequence, 0);
}

bool events_on(EventsSequence* sequence, int32_t id, float* alpha) {
    if (!sequence->running) {
        return false;
    }

    for (int32_t i = 0; i < sequence->currentCount; ++i) {
        EventsEvent* event = sequence->events + sequence->current[i];
        if (event->id == id) {
            if (alpha) {
                *alpha = event->alpha;
            }
            return true;
        }
    }

    return false;
}