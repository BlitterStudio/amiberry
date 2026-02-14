#ifndef ON_SCREEN_JOYSTICK_H
#define ON_SCREEN_JOYSTICK_H

#include <SDL.h>

// Initialize on-screen joystick textures and state.
// Call after the renderer is created.
void on_screen_joystick_init(SDL_Renderer* renderer);

// Clean up textures and state.
void on_screen_joystick_quit();

// Render the on-screen joystick controls.
// Call during the rendering frame, after the game screen is drawn.
void on_screen_joystick_redraw(SDL_Renderer* renderer);

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
