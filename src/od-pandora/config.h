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
 * Virtual Key for (A) button
 * default: HOME (278)
 */
#define VK_A SDLK_HOME

/*
 * Virtual Key for (B) button
 * default: END (279)
 */
#define VK_B SDLK_END

/*
 * Virtual Key for (X) button
 * default: PAGEDOWN (281)
 */
#define VK_X SDLK_PAGEDOWN

/*
 * Virtual Key for (Y) button
 * default: PAGEUP (280)
 */
#define VK_Y SDLK_PAGEUP

/*
 * Virtual Key for (Left shoulder) button
 * default: RSHIFT (303)
 */
#define VK_L SDLK_RSHIFT

/*
 * Virtual Key for (Right shoulder) button
 * default: RCTRL (305)
 */
#define VK_R SDLK_RCTRL

/*
 * Virtual Key for (up) button
 * default: UP (273)
 */
#define VK_UP SDLK_UP

/*
 * Virtual Key for (down) button
 * default: DOWN (274)
 */
#define VK_DOWN SDLK_DOWN

/*
 * Virtual Key for (right) button
 * default: RIGHT (275)
 */
#define VK_RIGHT SDLK_RIGHT

/*
 * Virtual Key for (left) button
 * default: LEFT (276)
 */
#define VK_LEFT SDLK_LEFT

/*
 * Virtual Key for (ESC) button
 * default: ESC (27)
 */
#define VK_ESCAPE SDLK_ESCAPE
