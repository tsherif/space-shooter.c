space-shooter.c [WIP]
=====================
A top-down space shooter written in C with minimal dependencies.

Uses:
- Windowing: Win32
- Rendering: OpenGL
- Audio: XAudio2
- Image Loading: stb_image

Asset Credits
-------------
- Sprites: https://ansimuz.itch.io/spaceship-shooter-environment
- BG Music: https://chiphead64.itch.io/free-game-soundtrack-by-cactusdude-hurry-up
- SFX: https://fallenblood.itch.io/50-sfx

Current State
-------------

- [x] Sprite rendering
- [x] Audio engine
- [x] Controller support
- [x] Keyboard support
- [x] Player ship
- [x] Fire bullets
- [x] Enemy ships
- [ ] Multiple enemy ship types
- [x] Enemy bullets
- [x] Enemy bullet sound
- [x] Bullets kill enemies
- [ ] Bullets kill player
- [ ] Explosions
- [ ] Multiple levels
- [ ] Title screen
- [ ] Keep score
- [x] Scale using a framebuffer
- [x] Get rid of copying into render list
- [ ] Instanced draws
- [ ] Clean up globals 