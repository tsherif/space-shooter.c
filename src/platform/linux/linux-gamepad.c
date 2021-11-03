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

#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <linux/input.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <math.h>
#include "../../shared/platform-interface.h"

#define INPUT_DIR "/dev/input/by-id/"
#define GAMEPAD_LEFT_THUMB_DEADZONE  7849

static struct {
    int32_t fd;
    int16_t stickX;
    int16_t stickY;
    bool aButton;
} gamepad = {
    .fd = -1
};

static bool testBit(uint8_t* bitField, int32_t bit) {
    int32_t byte = bit / 8;
    int32_t innerBit = bit % 8;

    return( bitField[byte] & (1 << innerBit)) != 0;
}

static bool endsWith(const char* s, const char* suffix) {
    const char* sp = s;
    int32_t sl = 0;
    const char* suffixP = suffix;
    int32_t suffixL = 0;

    while (*sp) {
        ++sp;
        ++sl;
    }

    while (*suffixP) {
        ++suffixP;
        ++suffixL;
    }

    if (sl < suffixL) {
        return false;
    }

    while (suffixL > 0) {
        if (*sp != *suffixP) {
            return false;
        }

        --sp;
        --suffixP;
        --suffixL;
    }

    return true;
}

static char* concatStrings(char* dst, int32_t dstLen, const char* s1, const char* s2) {
    int32_t charsWritten = 0;
    while (*s1 && charsWritten < dstLen - 1) {
        dst[charsWritten] = *s1;
        ++s1;
        ++charsWritten;
    }

    while (*s2 && charsWritten < dstLen - 1) {
        dst[charsWritten] = *s2;
        ++s2;
        ++charsWritten;
    }

    dst[charsWritten] = '\0';

    return dst;
}

void linux_initGamepad(void) {
    DIR* inputDir = opendir(INPUT_DIR);

    if (!inputDir) {
        platform_debugLog("Failed to open input dir\n");
        return;
    }

    struct dirent* entry = readdir(inputDir);

    while (entry) {
        if (endsWith(entry->d_name, "-event-joystick")) {
            char path[512];
            concatStrings(path, sizeof(path), INPUT_DIR,  entry->d_name);
            gamepad.fd = open(path, O_RDONLY | O_NONBLOCK);

            uint8_t absBits[(ABS_CNT + 7) / 8];
            ioctl(gamepad.fd, EVIOCGBIT(EV_ABS, sizeof(absBits)), absBits);

            uint8_t keyBits[(KEY_CNT + 7) / 8];
            ioctl(gamepad.fd, EVIOCGBIT(EV_KEY, sizeof(keyBits)), keyBits);

            if (testBit(absBits, ABS_X) && testBit(absBits, ABS_Y) && testBit(keyBits, BTN_A)) {
                break;
            } else {
                close(gamepad.fd);
                gamepad.fd = -1;
            }
        }
        entry = readdir(inputDir);
    }

    closedir(inputDir);
}

#include <stdio.h>

void linux_gamepadInput(GameInput* input) {
    if (gamepad.fd == -1) {
        return;
    }

    struct input_event events[16];
    int32_t bytesRead = read(gamepad.fd, events, sizeof(events));
    bool aButtonPressed = false;

    if (bytesRead >= 0) {
        int32_t numEvents = bytesRead / sizeof(struct input_event);

        for (int32_t i = 0; i < numEvents; ++i) {
            struct input_event* event = events + i;
            switch (event->type) {
                case EV_ABS: {
                    switch (event->code) {
                        case ABS_X: {
                            gamepad.stickX = event->value;
                        } break;
                        case ABS_Y: {
                            gamepad.stickY = event->value;
                        } break;
                    }
                } break;
                case EV_KEY: {
                    switch (event->code) {
                        case BTN_A: {
                            aButtonPressed = event->value && event->value != gamepad.aButton;
                            gamepad.aButton = event->value;
                        } break;
                    }
                } break;
            }
        }
    } else if (errno != EWOULDBLOCK && errno != EAGAIN) {
        gamepad.fd = -1;
    }

    float x = gamepad.stickX;
    float y = gamepad.stickY;

    float mag = (float) sqrt(x * x + y * y);
    x /= mag;
    y /= mag;

    if (mag > GAMEPAD_LEFT_THUMB_DEADZONE) {
        mag -= GAMEPAD_LEFT_THUMB_DEADZONE;
        mag /= 32767.0f - GAMEPAD_LEFT_THUMB_DEADZONE;
        input->velocity[0] = x * mag;
        input->velocity[1] = -y * mag;
    } else {
        input->velocity[0] = 0.0f;
        input->velocity[1] = 0.0f;
    }

    input->shoot = aButtonPressed;

    // fprintf(stderr, "%f, %f, %d\n", input->velocity[0], input->velocity[1], input->shoot);
}
