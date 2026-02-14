#ifndef ON_SCREEN_JOYSTICK_H
#define ON_SCREEN_JOYSTICK_H

#include <SDL.h>

// Initialize on-screen joystick surfaces and state.
// When USE_OPENGL is defined, renderer can be NULL (GL textures are created lazily).
void on_screen_joystick_init(SDL_Renderer* renderer);

// Clean up textures/surfaces and state.
void on_screen_joystick_quit();

// Render the on-screen joystick controls using SDL2 renderer.
// Call during the rendering frame, after the game screen is drawn.
void on_screen_joystick_redraw(SDL_Renderer* renderer);

#ifdef USE_OPENGL
// Render the on-screen joystick controls using OpenGL.
// drawable_w/h = GL drawable size, game_rect = the Amiga display rect within it.
// Call from show_screen() before SwapWindow.
void on_screen_joystick_redraw_gl(int drawable_w, int drawable_h, const SDL_Rect& game_rect);
#endif

// Touch event handlers. Return true if the event was consumed by the on-screen controls.
bool on_screen_joystick_handle_finger_down(const SDL_Event& event, int window_w, int window_h);
bool on_screen_joystick_handle_finger_up(const SDL_Event& event, int window_w, int window_h);
bool on_screen_joystick_handle_finger_motion(const SDL_Event& event, int window_w, int window_h);

// Query / set enabled state
bool on_screen_joystick_is_enabled();
void on_screen_joystick_set_enabled(bool enabled);

// Update the control layout when screen geometry changes.
// game_rect is the destination rectangle of the Amiga screen on the display.
void on_screen_joystick_update_layout(int screen_w, int screen_h, const SDL_Rect& game_rect);

#endif // ON_SCREEN_JOYSTICK_H
