#ifndef IMGUI_OSK_H
#define IMGUI_OSK_H

// D-pad / button state bitmask for imgui_osk_process() and osk_control()
#define OSK_UP     0x01
#define OSK_DOWN   0x02
#define OSK_LEFT   0x04
#define OSK_RIGHT  0x08
#define OSK_BUTTON 0x10

// Initialize the on-screen keyboard. Call after imgui_overlay_init().
void imgui_osk_init();

// Shut down and free resources.
void imgui_osk_shutdown();

// Toggle the keyboard visibility (slide in/out).
void imgui_osk_toggle();

// Returns true if the keyboard is visible or animating.
bool imgui_osk_is_active();

// Returns true if the keyboard should be rendered this frame.
bool imgui_osk_should_render();

// Render the keyboard. Called between imgui_overlay_begin_frame/end_frame.
// dw, dh = drawable (viewport) dimensions in pixels.
void imgui_osk_render(int dw, int dh);

// Process joystick/D-pad input for keyboard navigation.
// state: bitmask of VKBD_UP/DOWN/LEFT/RIGHT/BUTTON
// Returns: keycode in *keycode, pressed state in *pressed (0 or 1).
// Return value: true if a key event was generated.
bool imgui_osk_process(int state, int* keycode, int* pressed);

// Handle touch/mouse input at screen coordinates.
// Returns true if the event was consumed by the keyboard.
bool imgui_osk_handle_finger_down(float screen_x, float screen_y, int finger_id);
bool imgui_osk_handle_finger_up(float screen_x, float screen_y, int finger_id);
bool imgui_osk_handle_finger_motion(float screen_x, float screen_y, int finger_id);

// Check if a screen coordinate falls within the keyboard area.
bool imgui_osk_hit_test(float screen_x, float screen_y);

// Configuration
void imgui_osk_set_transparency(float alpha); // 0.0 = fully transparent, 1.0 = opaque
void imgui_osk_set_language(const char* lang); // "US", "UK", "DE", "FR"

// Direct gamepad control (called from SDL event handler).
// x/y: direction (-1,0,1). button: bit index. buttonstate: 0/1.
void osk_control(int x, int y, int button, int buttonstate);

#endif // IMGUI_OSK_H
