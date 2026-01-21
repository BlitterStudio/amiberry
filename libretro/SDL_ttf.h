#ifndef LIBRETRO_SDL_TTF_H
#define LIBRETRO_SDL_TTF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _TTF_Font TTF_Font;

int TTF_Init(void);
int TTF_SetFontSizeDPI(TTF_Font* font, int ptsize, unsigned int hdpi, unsigned int vdpi);

#ifdef __cplusplus
}
#endif

#endif
