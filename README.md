space-shooter.c
===============
A cross-platform, top-down 2D space shooter written in C using only system libraries.

![gif](./space-shooter.c.gif)

Dependencies
------------
- Window management: Win32 (Windows), Xlib (Linux)
- Rendering: OpenGL
- Audio: XAudio2 (Windows), ALSA/pthread (Linux)
- Gamepad: Xinput (Windows), evdev (Linux)

Description
-----------
`space-shooter.c` is a cross-platform, top-down 2D space shooter written in standard C11 using only system libraries (with system libraries defined as anything included in the C standard library or supported operating systems). `space-shooter.c` has been tested on Windows 10 and Ubuntu Linux 16.04. This project drew heavy inspiration from [Handmade Hero](https://handmadehero.org/) and [pacman.c](https://github.com/floooh/pacman.c).

The design and architecture of `space-shooter.c` is described [here](./ARCHITECTURE.md) [WIP].

### Caveat
I am not a professional game developer, nor a professional C programmer, so the design may be unconventional or sub-optimal in many ways. Happy to take feedback from any pros out there!

Building
--------
Windows
- In a shell with [cl](https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-160) set up, run `build.bat` for a debug build or `build.bat release` for an optimized build.
- Run `space-shooter.exe` from the `build/` directory.

Linux
- Make sure development headers for the Linux kernel, Xlib, glx and ALSA are installed.
    - E.g. on Ubuntu, run the following: `sudo apt install linux-libc-dev libx11-dev mesa-common-dev libasound2-dev`  
- Run `make` for a debug build or `make release` for an optimized build.
- Run `./space-shooter` from the `build/` directory.

Asset Credits
-------------
- Sprites: https://ansimuz.itch.io/spaceship-shooter-environment
- BG Music: https://chiphead64.itch.io/free-game-soundtrack-by-cactusdude-hurry-up
- SFX: https://fallenblood.itch.io/50-sfx
- Font: https://gpway.itch.io/white-pixel-sprite-font
