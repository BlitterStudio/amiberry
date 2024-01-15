
void casablanca_map_overlay(void);
void draco_map_overlay(void);
void draco_init(void);
void draco_free(void);
bool draco_mouse(int port, int x, int y, int z, int b);
void draco_bustimeout(uaecptr addr);
void draco_ext_interrupt(bool);
void draco_keycode(uae_u16 scancode, uae_u8 state);
