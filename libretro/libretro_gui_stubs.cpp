#include "sdl_compat.h"
#include "vkbd/vkbd.h"

int TTF_Init(void)
{
	return 0;
}

int TTF_SetFontSizeDPI(TTF_Font* font, int ptsize, unsigned int hdpi, unsigned int vdpi)
{
	(void)font;
	(void)ptsize;
	(void)hdpi;
	(void)vdpi;
	return 0;
}

void vkbd_set_hires(bool hires)
{
	(void)hires;
}

void vkbd_set_language(VkbdLanguage language)
{
	(void)language;
}

void vkbd_set_style(VkbdStyle style)
{
	(void)style;
}

void vkbd_set_language(const std::string& language)
{
	(void)language;
}

void vkbd_set_style(const std::string& style)
{
	(void)style;
}

void vkbd_set_transparency(double transparency)
{
	(void)transparency;
}

void vkbd_set_keyboard_has_exit_button(bool keyboardHasExitButton)
{
	(void)keyboardHasExitButton;
}

void vkbd_init(void)
{
}

void vkbd_quit(void)
{
}

void vkbd_redraw(void)
{
}

void vkbd_toggle(void)
{
}

bool vkbd_process(int state, int* keycode, int* pressed)
{
	(void)state;
	if (keycode)
		*keycode = 0;
	if (pressed)
		*pressed = 0;
	return false;
}

bool vkbd_is_active(void)
{
	return false;
}

void vkbd_update_position_from_texture()
{
}
