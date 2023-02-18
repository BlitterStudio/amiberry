#ifndef VKBD_H
#define VKBD_H

#define VKBD_LEFT 1
#define VKBD_RIGHT 2
#define VKBD_UP 4
#define VKBD_DOWN 8
#define VKBD_BUTTON 16
#include <string>

enum VkbdLanguage
{
	VKBD_LANGUAGE_UK,
	VKBD_LANGUAGE_GER,
	VKBD_LANGUAGE_FR,
	VKBD_LANGUAGE_US
};

enum VkbdStyle
{
	VKBD_STYLE_WARM,
	VKBD_STYLE_COOL,
	VKBD_STYLE_DARK,
	VKBD_STYLE_ORIG
};

extern void vkbd_set_hires(bool hires);
extern void vkbd_set_language(VkbdLanguage language);
extern void vkbd_set_style(VkbdStyle style);
extern void vkbd_set_language(std::string language);
extern void vkbd_set_style(std::string style);
extern void vkbd_set_transparency(double transparency);
extern void vkbd_set_keyboard_has_exit_button(bool keyboardHasExitButton);

extern void vkbd_init(void);
extern void vkbd_quit(void);
extern void vkbd_redraw(void);
extern void vkbd_toggle(void);
extern bool vkbd_process(int state, int* keycode, int* pressed);
extern bool vkbd_is_active(void);

#endif // VKBD_H
