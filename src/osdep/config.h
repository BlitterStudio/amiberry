 /* 
  * UAE - The Un*x Amiga Emulator
  * 
  * User configuration options
  *
  * Copyright 1995 - 1998 Bernd Schmidt
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
#define VK_Green SDLK_HOME

/*
 * Virtual Key for CD32 Blue button
 * default: END (279)
 */
#define VK_Blue SDLK_PAGEDOWN

/*
 * Virtual Key for CD32 Red button
 * default: PAGEDOWN (281)
 */
#define VK_Red SDLK_END

/*
 * Virtual Key for (Y) button
 * default: PAGEUP (280)
 */
#define VK_Yellow SDLK_PAGEUP

/*
 * Virtual Key for (Left shoulder) button
 * default: RSHIFT (303)
 */
#ifdef ANDROID
#define VK_L SDLK_F13
#else
#define VK_LShoulder SDLK_RSHIFT
#endif

/*
 * Virtual Key for (Right shoulder) button
 * default: RCTRL (305)
 */
#define VK_RShoulder SDLK_RCTRL

/*
 * Virtual Key for CD32 Start button
 * default: Pause/Break
 */
#define VK_Play SDLK_RETURN

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