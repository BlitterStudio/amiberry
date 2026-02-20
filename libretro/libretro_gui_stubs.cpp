#include <string>
#include <vector>

#include "sdl_compat.h"
#include "vkbd/vkbd.h"
#include "on_screen_joystick.h"

// serial_ports is declared in parser.h and defined in amiberry_gui.cpp (standalone).
// The libretro build doesn't include amiberry_gui.cpp, so provide the definition here.
std::vector<std::string> serial_ports;

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

// On-screen joystick stubs (not used in libretro builds)
void on_screen_joystick_init(SDL_Renderer* renderer) { (void)renderer; }
void on_screen_joystick_quit() {}
void on_screen_joystick_redraw(SDL_Renderer* renderer) { (void)renderer; }
#ifdef USE_OPENGL
void on_screen_joystick_redraw_gl(int drawable_w, int drawable_h, const SDL_Rect& game_rect)
{
	(void)drawable_w; (void)drawable_h; (void)game_rect;
}
#endif
bool on_screen_joystick_handle_finger_down(const SDL_Event& event, int window_w, int window_h)
{
	(void)event; (void)window_w; (void)window_h; return false;
}
bool on_screen_joystick_handle_finger_up(const SDL_Event& event, int window_w, int window_h)
{
	(void)event; (void)window_w; (void)window_h; return false;
}
bool on_screen_joystick_handle_finger_motion(const SDL_Event& event, int window_w, int window_h)
{
	(void)event; (void)window_w; (void)window_h; return false;
}
bool on_screen_joystick_is_enabled() { return false; }
void on_screen_joystick_set_enabled(bool enabled) { (void)enabled; }
bool on_screen_joystick_keyboard_tapped() { return false; }
void on_screen_joystick_update_layout(int screen_w, int screen_h, const SDL_Rect& game_rect)
{
	(void)screen_w; (void)screen_h; (void)game_rect;
}
