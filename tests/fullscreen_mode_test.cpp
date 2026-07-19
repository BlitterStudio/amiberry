#include <iostream>

#include "amiberry_gfx_mode.h"

static int failures;

static void expect_eq(const int actual, const int expected, const char* message)
{
	if (actual != expected) {
		std::cerr << message << ": expected " << expected << ", got " << actual << '\n';
		failures++;
	}
}

int main()
{
	expect_eq(amiberry_normalize_gfx_fullscreen_mode(GFX_WINDOW), GFX_WINDOW,
		"Windowed mode must remain Windowed");
	expect_eq(amiberry_normalize_gfx_fullscreen_mode(GFX_FULLSCREEN), GFX_FULLWINDOW,
		"Legacy exclusive Fullscreen mode must migrate to Full-window");
	expect_eq(amiberry_normalize_gfx_fullscreen_mode(GFX_FULLWINDOW), GFX_FULLWINDOW,
		"Full-window mode must remain Full-window");
	expect_eq(amiberry_normalize_gfx_fullscreen_mode(-1), -1,
		"Unknown mode values must remain available for validation");

	return failures == 0 ? 0 : 1;
}
