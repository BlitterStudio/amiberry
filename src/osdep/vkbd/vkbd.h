#ifndef VKBD_H
#define VKBD_H

#define VKBD_LEFT 1
#define VKBD_RIGHT 2
#define VKBD_UP 4
#define VKBD_DOWN 8
#define VKBD_BUTTON 16

// special return codes for vkbd_process
#define KEYCODE_NOTHING (-1234567)

int vkbd_init(void);
void vkbd_quit(void);
void vkbd_redraw(void);
void vkbd_toggle(void);
bool vkbd_process(int state, int *keycode, int *pressed);
void vkbd_displace_up(void);
void vkbd_displace_down(void);
void vkbd_transparency_up(void);
void vkbd_transparency_down(void);
bool vkbd_is_active(void);

#endif // VKBD_H
