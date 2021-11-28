space-shooter.c
===============
A cross-platform, top-down 2D space shooter written in C using only system libraries.

![gif](./space-shooter.c.gif)

Dependencies
------------
- Windowing: Win32 (Windows), Xlib (Linux)
- Rendering: OpenGL
- Audio: XAudio2 (Windows), ALSA/pthread (Linux)
- Gamepad: Xinput (Windows), evdev (Linux)

Description
-----------
`space-shooter.c` is a top-down 2D space shooter developed as a personal challenge to create a cross-platform game in standard C using only system libraries (with system libraries defined as anything included in the C standard library or supported operating systems). `space-shooter.c` has been tested on Windows 10 and Ubuntu Linux 16.04. Why not Mac? I don't currently have a Mac machine to develop on. That may come later depending on how much free time I have. This project drew heavy inspiration from [Handmade Hero](https://handmadehero.org/) and [pacman.c](https://github.com/floooh/pacman.c), and many implementation details were inspired by open-source projects like [sokol](https://github.com/floooh/sokol), [glfw](https://github.com/glfw/glfw) and [SDL](https://github.com/libsdl-org/SDL).

Building
--------

Windows
- In a shell with [cl](https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-160) set up, run `build.bat` for a debug build or `build.bat release` for an optimized build.
- Run `space-shooter.exe` from the `build/` directory.

Linux
- Run `make` for a debug build or `build.bat release` for an optimized build.
- Run `./space-shooter` from the `build/` directory.

Asset Credits
-------------
- Sprites: https://ansimuz.itch.io/spaceship-shooter-environment
- BG Music: https://chiphead64.itch.io/free-game-soundtrack-by-cactusdude-hurry-up
- SFX: https://fallenblood.itch.io/50-sfx
- Font: https://gpway.itch.io/white-pixel-sprite-font
