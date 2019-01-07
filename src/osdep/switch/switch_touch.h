//
// Created by rsn8887 on 02/05/18.
#ifndef SWITCH_TOUCH_H
#define SWITCH_TOUCH_H

#include <SDL2/SDL.h>
#if defined(__vita__)
#include <psp2/touch.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

void SWITCH_HandleTouch(SDL_Event *event);
void SWITCH_FinishSimulatedMouseClicks();

#ifdef __cplusplus
}
#endif
#endif /* SWITCH_TOUCH_H */
