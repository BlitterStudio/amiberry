#define SDL_MAIN_HANDLED
#include "SDL.h"
#include "SDL_ttf.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    SDL_SetMainReady();
    if (SDL_Init(0) < 0) {
        fprintf(stderr, "could not initialize sdl2: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
    }
    TTF_Quit();
    SDL_Quit();
    return 0;
}
