#pragma once

static inline bool osdep_platform_require_window_for_mouse()
{
	return false;
}

static inline bool osdep_platform_use_event_pump()
{
	return true;
}

static inline bool osdep_platform_should_delay_on_pause()
{
	return true;
}

static inline bool osdep_platform_init_sdl()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		write_log("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		int num = SDL_GetNumVideoDrivers();
		for (int i = 0; i < num; ++i) {
			write_log("Video Driver %d: %s\n", i, SDL_GetVideoDriver(i));
		}
		return false;
	}

	// Enable native IME for international text input (SDL 2.0.18+)
#ifdef SDL_HINT_IME_SHOW_UI
	SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif
	(void)atexit(SDL_Quit);
	return true;
}

static inline void osdep_platform_shutdown_sdl()
{
	SDL_Quit();
}

static inline void osdep_platform_init_ui()
{
	normalcursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	clipboard_init();
}

static inline void osdep_platform_sync_keyboard_leds()
{
#if defined(__linux__)
	ioctl(0, KDGKBLED, &kbd_flags);
	ioctl(0, KDGETLED, &kbd_led_status);
	if (kbd_flags & 07 & LED_CAP)
	{
		kbd_led_status |= LED_CAP;
		inputdevice_do_keyboard(AK_CAPSLOCK, 1);
	}
	else
	{
		kbd_led_status &= ~LED_CAP;
		inputdevice_do_keyboard(AK_CAPSLOCK, 0);
	}
	ioctl(0, KDSETLED, kbd_led_status);
#else
	int caps = SDL_GetModState();
	caps = caps & KMOD_CAPS;
	if (caps == KMOD_CAPS)
		kbd_led_status |= ~0x04;
	else
		kbd_led_status &= ~0x04;
#endif
}

static inline void osdep_platform_update_clipboard()
{
	auto* clipboard_uae = uae_clipboard_get_text();
	if (clipboard_uae) {
		SDL_SetClipboardText(clipboard_uae);
		uae_clipboard_free_text(clipboard_uae);
	}
}

static inline void osdep_platform_call_real_main(int argc, char** argv)
{
#ifdef __APPLE__
	SDL_PumpEvents();
	SDL_Event event;
	if (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_DROPFILE, SDL_DROPFILE) > 0)
	{
		write_log("Intercepted SDL_DROPFILE event: %s\n", event.drop.file);
		char** new_argv = new char*[argc + 2];
		for (int i = 0; i < argc; ++i)
		{
			new_argv[i] = argv[i];
		}
		new_argv[argc] = event.drop.file;
		new_argv[argc + 1] = nullptr;
		real_main(argc + 1, new_argv);
		SDL_free(event.drop.file);
		delete[] new_argv;
	}
	else
	{
		real_main(argc, argv);
	}
#else
	real_main(argc, argv);
#endif
}
