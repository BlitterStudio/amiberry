 /* 
  * UAE - The Un*x Amiga Emulator
  * 
  * User configuration options
  *
  * Copyright 1995 - 1998 Bernd Schmidt
  */

/*
 * Please note: Many things are configurable with command line parameters,
 * and you can put anything you can pass on the command line into a 
 * configuration file ~/.uaerc. Please read the documentation for more
 * information.
 * 
 * NOTE NOTE NOTE
 * Whenever you change something in this file, you have to "make clean"
 * afterwards.
 * Don't remove the '#' signs. If you want to enable something, move it out
 * of the C comment block, if you want to disable something, move it inside
 * the block.
 */

/*
 * When USE_COMPILER is defined, a m68k->i386 instruction compiler will be
 * used. This is experimental. It has only been tested on a Linux/i386 ELF
 * machine, although it might work on other i386 Unices.
 * This is supposed to speed up application programs. It will not work very
 * well for hardware bangers like games and demos, in fact it will be much
 * slower. It can also be slower for some applications and/or benchmarks.
 * It needs a lot of tuning. Please let me know your results with this.
 * The second define, RELY_ON_LOADSEG_DETECTION, decides how the compiler 
 * tries to detect self-modifying code. If it is not set, the first bytes
 * of every compiled routine are used as checksum before executing the
 * routine. If it is set, the UAE filesystem will perform some checks to 
 * detect whether an executable is being loaded. This is less reliable
 * (it won't work if you don't use the harddisk emulation, so don't try to
 * use floppies or even the RAM disk), but much faster.
 *
 * @@@ NOTE: This option is unfortunately broken in this version. Don't
 * try to use it. @@@
 *
#define USE_COMPILER
#define RELY_ON_LOADSEG_DETECTION
 */

/***************************************************************************
 * Operating system/machine specific options
 * Configure these for your CPU. The default settings should work on any
 * machine, but may not give optimal performance everywhere.
 * (These don't do very much yet, except HAVE_RDTSC
 */

/*
 * [pismy] defines virtual keys
 * Still hard-coded but can be easily changed by recompiling the project...
 * See codes here: https://www.libsdl.org/release/SDL-1.2.15/include/SDL_keysym.h
 */

/*
 * Virtual Key for CD32 Green button
 * default: HOME (278)
 */
#pragma once
#ifdef USE_SDL1
#define VK_Green SDLK_HOME
#elif USE_SDL2
#define VK_Green SDL_SCANCODE_HOME
#endif

/*
 * Virtual Key for CD32 Blue button
 * default: END (279)
 */
#ifdef USE_SDL1
#define VK_Blue SDLK_PAGEDOWN
#elif USE_SDL2
#define VK_Blue SDL_SCANCODE_PAGEDOWN
#endif

/*
 * Virtual Key for CD32 Red button
 * default: PAGEDOWN (281)
 */
#ifdef USE_SDL1
#define VK_Red SDLK_END
#elif USE_SDL2
#define VK_Red SDL_SCANCODE_END
#endif

/*
 * Virtual Key for (Y) button
 * default: PAGEUP (280)
 */
#ifdef USE_SDL1
#define VK_Yellow SDLK_PAGEUP
#elif USE_SDL2
#define VK_Yellow SDL_SCANCODE_PAGEUP
#endif

/*
 * Virtual Key for (Left shoulder) button
 * default: RSHIFT (303)
 */
#ifdef ANDROID
#define VK_L SDLK_F13
#else
#ifdef USE_SDL1
#define VK_LShoulder SDLK_RSHIFT
#elif USE_SDL2
#define VK_LShoulder SDL_SCANCODE_RSHIFT
#endif
#endif

/*
 * Virtual Key for (Right shoulder) button
 * default: RCTRL (305)
 */
#ifdef USE_SDL1
#define VK_RShoulder SDLK_RCTRL
#elif USE_SDL2
#define VK_RShoulder SDL_SCANCODE_RCTRL
#endif

/*
 * Virtual Key for CD32 Start button
 * default: Pause/Break
 */
#ifdef USE_SDL1
#define VK_Play SDLK_RETURN
#elif USE_SDL2
#define VK_Play SDL_SCANCODE_RETURN
#endif

/*
 * Virtual Key for (up) button
 * default: UP (273)
 */
#ifdef USE_SDL1
#define VK_UP SDLK_UP
#elif USE_SDL2
#define VK_UP SDL_SCANCODE_UP
#endif

/*
 * Virtual Key for (down) button
 * default: DOWN (274)
 */
#ifdef USE_SDL1
#define VK_DOWN SDLK_DOWN
#elif USE_SDL2
#define VK_DOWN SDL_SCANCODE_DOWN
#endif

/*
 * Virtual Key for (right) button
 * default: RIGHT (275)
 */
#ifdef USE_SDL1
#define VK_RIGHT SDLK_RIGHT
#elif USE_SDL2
#define VK_RIGHT SDL_SCANCODE_RIGHT
#endif

/*
 * Virtual Key for (left) button
 * default: LEFT (276)
 */
#ifdef USE_SDL1
#define VK_LEFT SDLK_LEFT
#elif USE_SDL2
#define VK_LEFT SDL_SCANCODE_LEFT
#endif

/*
 * Virtual Key for (ESC) button
 * default: ESC (27)
 */
#ifdef USE_SDL1
#define VK_ESCAPE SDLK_ESCAPE
#elif USE_SDL2
#define VK_ESCAPE SDL_SCANCODE_ESCAPE
#endif