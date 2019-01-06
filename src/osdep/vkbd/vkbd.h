#ifndef VKBD_H
#define VKBD_H
#include<SDL.h>

#ifdef USE_SDL2
#include "sdl2_to_sdl1.h"
#endif

#define VKBD_X 20
#define VKBD_Y 200

#define VKBD_LEFT 1
#define VKBD_RIGHT 2	
#define VKBD_UP 4
#define VKBD_DOWN 8
#define VKBD_BUTTON 16
#define VKBD_BUTTON_BACKSPACE 32
#define VKBD_BUTTON_SHIFT 64
#define VKBD_BUTTON_RESET_STICKY 128
#define VLBD_BUTTON2 128

// special return codes for vkbd_process
#define KEYCODE_NOTHING -1234567
#define KEYCODE_STICKY_RESET -100

#define NUM_STICKY 7 // number of sticky keys (shift, alt etc)

int vkbd_init(void);
void vkbd_quit(void);
void vkbd_redraw(void);
int vkbd_process(void);
void vkbd_init_button2(void);
void vkbd_displace_up(void);
void vkbd_displace_down(void);
void vkbd_transparency_up(void);
void vkbd_transparency_down(void);
void vkbd_reset_sticky_keys(void);

extern int vkbd_mode;
extern int vkbd_move;
typedef struct
{
	int code; // amiga-side keycode
	bool stuck; // is it currently stuck pressed?
	bool can_switch; // de-bounce
	unsigned char index; // index in vkbd_rect[]
} t_vkbd_sticky_key;
extern t_vkbd_sticky_key vkbd_sticky_key[NUM_STICKY];
extern int vkbd_key;
extern int vkbd_keysave;
extern SDLKey vkbd_button2;
extern int keymappings[10][3];
#endif // VKBD_H