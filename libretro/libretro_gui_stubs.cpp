#include <string>
#include <vector>

#include "sdl_compat.h"
#include "imgui_overlay.h"
#include "imgui_osk.h"
#include "on_screen_joystick.h"

// serial_ports is declared in parser.h and defined in amiberry_gui.cpp (standalone).
// The libretro build doesn't include amiberry_gui.cpp, so provide the definition here.
std::vector<std::string> serial_ports;

// get_json_timestamp / get_xml_timestamp are defined in main_window.cpp (standalone GUI).
// The libretro build doesn't include main_window.cpp, so provide stubs here.
std::string get_json_timestamp(const std::string& json_filename)
{
	(void)json_filename;
	return {};
}

std::string get_xml_timestamp(const std::string& xml_filename)
{
	(void)xml_filename;
	return {};
}

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

// ImGui overlay stubs (not used in libretro builds)
void imgui_overlay_init(SDL_Window* window, SDL_Renderer* sdl_renderer, void* gl_context)
{
	(void)window; (void)sdl_renderer; (void)gl_context;
}
void imgui_overlay_shutdown() {}
bool imgui_overlay_is_initialized() { return false; }
void imgui_overlay_begin_frame() {}
void imgui_overlay_end_frame() {}
struct ImDrawData;
ImDrawData* imgui_overlay_get_draw_data() { return nullptr; }
void imgui_overlay_restore_context() {}
bool imgui_overlay_is_vulkan() { return false; }

// ImGui on-screen keyboard stubs
void imgui_osk_init() {}
void imgui_osk_shutdown() {}
void imgui_osk_toggle() {}
bool imgui_osk_is_active() { return false; }
bool imgui_osk_should_render() { return false; }
void imgui_osk_render(int dw, int dh) { (void)dw; (void)dh; }
bool imgui_osk_process(int state, int* keycode, int* pressed)
{
	(void)state;
	if (keycode) *keycode = 0;
	if (pressed) *pressed = 0;
	return false;
}
bool imgui_osk_handle_finger_down(float x, float y, int id) { (void)x; (void)y; (void)id; return false; }
bool imgui_osk_handle_finger_up(float x, float y, int id) { (void)x; (void)y; (void)id; return false; }
bool imgui_osk_handle_finger_motion(float x, float y, int id) { (void)x; (void)y; (void)id; return false; }
bool imgui_osk_hit_test(float x, float y) { (void)x; (void)y; return false; }
void imgui_osk_set_transparency(float alpha) { (void)alpha; }
void imgui_osk_set_language(const char* lang) { (void)lang; }

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
