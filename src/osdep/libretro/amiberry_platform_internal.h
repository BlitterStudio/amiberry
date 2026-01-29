#pragma once

static inline bool osdep_platform_require_window_for_mouse()
{
	return true;
}

static inline bool osdep_platform_use_event_pump()
{
	return false;
}

static inline bool osdep_platform_should_delay_on_pause()
{
	return false;
}

static inline bool osdep_platform_init_sdl()
{
	return true;
}

static inline void osdep_platform_shutdown_sdl()
{
}

static inline void osdep_platform_init_ui()
{
}

static inline void osdep_platform_sync_keyboard_leds()
{
}

static inline void osdep_platform_update_clipboard()
{
}

static inline void osdep_platform_call_real_main(int argc, char** argv)
{
	real_main(argc, argv);
}
