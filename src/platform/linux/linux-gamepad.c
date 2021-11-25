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

/////////////////////////////////////////////////////////////////////
// Uses the evdev gamepad spec: 
// - https://www.kernel.org/doc/html/v4.13/input/gamepad.html
// - https://www.linuxjournal.com/article/6429
// - https://ourmachinery.com/post/gamepad-implementation-on-linux/
/////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/input.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <math.h>
#include "linux-gamepad.h"
#include "../../shared/debug.h"

#define PATH_MAX 512
#define INPUT_DIR "/dev/input/by-id"
#define GAMEPAD_LEFT_THUMB_DEADZONE  7849

static struct {
    int32_t fd;
    int16_t stickX;
    int16_t stickY;
    bool aButton;
    bool startButton;
    bool backButton;
    bool lastAButton;
    bool lastStartButton;
    bool lastBackButton;
} gamepad = {
    .fd = -1
};

// Test single bit in multi-byte array
static bool testBit(uint8_t* bitField, int32_t bit) {
    int32_t byte = bit / 8;
    int32_t innerBit = bit % 8;

    return( bitField[byte] & (1 << innerBit)) != 0;
}

// Check string suffix
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

void linux_detectGamepad(void) {
    DIR* inputDir = opendir(INPUT_DIR);

    if (!inputDir) {
        DEBUG_LOG("Failed to open input dir\n");
        return;
    }

    struct dirent* entry = readdir(inputDir);

    while (entry) {

        // evdev gamepad file names end with "-event-joystick"
        // See: https://wiki.archlinux.org/title/Gamepad#Gamepad_input_systems
        if (endsWith(entry->d_name, "-event-joystick")) {
            char path[PATH_MAX];
            snprintf(path, PATH_MAX, "%s/%s", INPUT_DIR, entry->d_name);
            gamepad.fd = open(path, O_RDONLY | O_NONBLOCK);

            if (gamepad.fd == -1) {
                continue;
            }

            // Get bitfields to test for gamepad capabilities
            uint8_t absBits[(ABS_CNT + 7) / 8] = { 0 };
            if (ioctl(gamepad.fd, EVIOCGBIT(EV_ABS, sizeof(absBits)), absBits) < 0) {
                goto ERROR_FILE_OPENED;
            }

            uint8_t keyBits[(KEY_CNT + 7) / 8] = { 0 };
            if (ioctl(gamepad.fd, EVIOCGBIT(EV_KEY, sizeof(keyBits)), keyBits) < 0) {
                goto ERROR_FILE_OPENED;  
            }

            // Test for thumbstick (ABS_X/ABS_Y) and A button (BTN_A)
            if (
                testBit(absBits, ABS_X) && 
                testBit(absBits, ABS_Y) && 
                testBit(keyBits, BTN_A) &&
                testBit(keyBits, BTN_START) &&
                testBit(keyBits, BTN_SELECT)
            ) {
                break;
            }
        
            ERROR_FILE_OPENED:
            close(gamepad.fd);
            gamepad.fd = -1;
        }
        entry = readdir(inputDir);
    }

    closedir(inputDir);
}

void linux_pingGamepad(void) {
    if (gamepad.fd == -1) {
        linux_detectGamepad();
    }
}

void linux_updateGamepad(void) {
    if (gamepad.fd == -1) {
        return;
    }

    struct input_event events[32];
    int32_t bytesRead = read(gamepad.fd, events, sizeof(events));

    if (bytesRead >= 0) {
        int32_t numEvents = bytesRead / sizeof(struct input_event);

        for (int32_t i = 0; i < numEvents; ++i) {
            struct input_event* event = events + i;
            switch (event->type) {
                case EV_ABS: {
                    switch (event->code) {
                        case ABS_X: gamepad.stickX = event->value; break;
                        case ABS_Y: gamepad.stickY = event->value; break;
                    }
                } break;
                case EV_KEY: {
                    switch (event->code) {
                        case BTN_A: gamepad.aButton = event->value; break;
                        case BTN_START: gamepad.startButton = event->value; break;
                        case BTN_SELECT: gamepad.backButton = event->value; break;
                    }
                } break;
            }
        }
    } else if (errno != EWOULDBLOCK && errno != EAGAIN) {
        gamepad.fd = -1;
    }
}

void linux_gamepadMenuButtons(LinuxGamepadMenuButtons* gamepadMenuButtons) {
    if (gamepad.fd == -1) {
        gamepadMenuButtons->start = false;
        gamepadMenuButtons->back = false;
        return;
    }

    gamepadMenuButtons->start = gamepad.startButton && gamepad.startButton != gamepad.lastStartButton;
    gamepad.lastStartButton = gamepad.startButton;


    gamepadMenuButtons->back = gamepad.backButton && gamepad.backButton != gamepad.lastBackButton;
    gamepad.lastBackButton = gamepad.backButton;
}

void linux_gamepadInput(GameInput* input) {
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

    input->shoot = gamepad.aButton && gamepad.aButton != gamepad.lastAButton;
    gamepad.lastAButton = gamepad.aButton;
}

void linux_closeGamepad(void) {
    if (gamepad.fd != -1) {
        close(gamepad.fd);
        gamepad.fd = -1;
    }
}
