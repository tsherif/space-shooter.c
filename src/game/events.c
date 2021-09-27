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

void events_start(EventsSequence* sequence) {
    if (sequence->running) {
        return;
    }

    sequence->running = true;
    sequence->ticks = 0;
    sequence->current = 0;
    sequence->complete = false;
    sequence->events[0].t = 0.0f;
}

void events_update(EventsSequence* sequence) {
    if (!sequence->running) {
        return;
    }

    ++sequence->ticks;

    EventsEvent* event = sequence->events + sequence->current;
    int32_t duration = event->duration;

    // duration of 0 or 1 will last for 1 tick
    if (duration == 0) {
        duration = 1;
    }

    if (sequence->ticks >= event->delay + duration) {
        sequence->ticks = 0;
        ++sequence->current;
        if (sequence->current == sequence->count || sequence->current == EVENTS_MAX_SEQUENCE) {
            sequence->running = false;
            sequence->complete = true;
            return;
        }
        event = sequence->events + sequence->current;
    }

    if (duration > 1 && sequence->ticks > event->delay) {
        event->t = (float) (sequence->ticks - event->delay) / (duration - 1);
    } else {
        event->t = 0.0f;
    }
}

bool events_on(EventsSequence* sequence, int32_t id) {
    if (!sequence->running) {
        return false;
    }

    EventsEvent* event = sequence->events + sequence->current;

    return event->id == id && sequence->ticks >= event->delay;
}