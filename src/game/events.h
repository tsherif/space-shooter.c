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

//////////////////////////////////////////////////////////////////////
// Simple event system inspired by the one used in pacman.c
// (https://github.com/floooh/pacman.c) with the primary difference
// being this one is time-based instead of tick-based. 
//
// Events sequences are used to structure logic that needs to 
// be orchestrated in time, e.g. the title intro sequence or 
// transitioning between levels. Rather than using callbacks,
// event sequences are polled for currently active events
// and logic can condionally execute based on those events.
//
// Note that only one event can be active at a time per sequence.
//////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////
// Events_Event contains data about a single event.
//
// Members:
//  - delay: how long to wait in ms after the previous event before
//      starting.
//  - duration: how long the event lasts.
//  - id: an Events_Id enum.
//
// Note that either delay or duration must be at least 50ms. This
// is to enforce the constraint that only one event be active
// at a time per sequence.
//////////////////////////////////////////////////////////////////////

typedef struct {
    float delay;
    float duration;
    int32_t id;
} Events_Event;

//////////////////////////////////////////////////////////////////////
// Events_Sequence is essentially an array of Events_Events with
// meta data about its running state.
//
// Members:
//  - events: array of Events_Events.
//  - count: length of events array.
//  - time: current time within the sequence.
//  - activeEvent: currently active event, or NULL if none are active.
//  - alpha: proportion between 0-1 indicating how far we are into the 
//      currently active event (useful for things like fading out).
//  - running: is this sequence currently active?
//  - complete: has this sequence finished running?
//  - loop: should this sequence loop when it runs through all
//      its events?
//
//////////////////////////////////////////////////////////////////////

typedef struct {
    Events_Event* events;
    int32_t count;
    float time;
    Events_Id activeEvent; // NOTE(Tarek): Only one triggered event at a time! Do we ever need simultaneous events?
    float alpha; // "Factor" between 0-1 of how far into the the current triggered event we are. Can be used for blending.
    bool running;
    bool complete;
    bool loop;
} Events_Sequence;

/////////////////////////////////////////////////////////////////////////
// Event manipulation functions.
//
// - events_start(): Mark sequence as running.
// - events_stop(): Reset sequence and mark as not running.
// - events_beforeFrame(): Update a sequence based on the elapsed time
//      since the last frame.
// - events_on(): Check if the provided event ID is currently 
//      active in a sequence.
//
// Basic usage:
//
// Events_sequence s = { ... };
// events_start(&s);
//
// // On each frame:
// events_beforeFrame(&s, elapsedTime);
//
// if (events_on(&s, EVENTS_DISPLAY)) {
//     // ... Display something   
// }
//
/////////////////////////////////////////////////////////////////////////

void events_start(Events_Sequence* sequence);
void events_stop(Events_Sequence* sequence);
void events_beforeFrame(Events_Sequence* sequence, float elapsedTime);
bool events_on(Events_Sequence* sequence, int32_t id);

//////////////////////////////////////////////////////////////////////
// The sequences used by the game
//////////////////////////////////////////////////////////////////////

extern Events_Sequence events_titleControlSequence;
extern Events_Sequence events_titleSequence;
extern Events_Sequence events_subtitleSequence;
extern Events_Sequence events_instructionSequence;
extern Events_Sequence events_gameOverSequence;
extern Events_Sequence events_gameOverRestartSequence;
extern Events_Sequence events_levelTransitionSequence;

#endif
