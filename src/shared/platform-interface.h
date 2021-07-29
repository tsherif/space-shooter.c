#ifndef _PLATFORM_interface_H_
#define _PLATFORM_interface_H_
#include <stdbool.h>

typedef struct {
	bool left;
	bool right;
	bool up;
	bool down;
	bool space;
	bool ctrl;
	bool changed;
} GameKeyboard;

typedef struct {
	float leftStickX;
	float leftStickY;
	bool aButton;
} GameController;

// Must be implemented by game, to be used by platform layer.
void game_init(void);
void game_update(void);
void game_draw(void);
void game_resize(int width, int height);
void game_keyboard(GameKeyboard* inputKeys);
void game_controller(GameController* controllerInput);

// Must be implemented by platform layer, to be used by platform game.
typedef struct PlatformSound PlatformSound;
bool platform_initAudio(void);
PlatformSound* platform_loadSound(const char* fileName);
void platform_playSound(PlatformSound* sound);

#endif