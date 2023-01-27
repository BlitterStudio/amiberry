#ifndef VKBD_H
#define VKBD_H

#define VKBD_LEFT 1
#define VKBD_RIGHT 2
#define VKBD_UP 4
#define VKBD_DOWN 8
#define VKBD_BUTTON 16
 
enum VkbdLanguage {
    VKBD_LANGUAGE_UK,
    VKBD_LANGUAGE_GER,
    VKBD_LANGUAGE_FR,
    VKBD_LANGUAGE_US
};

enum VkbdStyle {
    VKBD_STYLE_WARM,
    VKBD_STYLE_COOL,
    VKBD_STYLE_DARK,
    VKBD_STYLE_ORIG
};

void vkbd_set_hires(bool hires);
void vkbd_set_language(VkbdLanguage language);
void vkbd_set_style(VkbdStyle style);
void vkbd_set_transparency(double transparency);

int vkbd_init(void);
void vkbd_quit(void);
void vkbd_redraw(void);
void vkbd_toggle(void);
bool vkbd_process(int state, int *keycode, int *pressed);
bool vkbd_is_active(void);

#endif // VKBD_H
