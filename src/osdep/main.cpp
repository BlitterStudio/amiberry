#include "sysdeps.h"
#include <SDL3/SDL_main.h>

#ifdef AMIBERRY_NATIVE_APPLE_UI
/* Implemented in apple/Shared/AmiberryNativeUI.swift via @_cdecl and linked
 * in from the amiberry_native_ui Swift static library. Calling it at startup
 * proves the mixed Swift/C++ link in both directions: C++ calls Swift, and
 * the Swift side calls the C settings API. */
extern "C" int amiberry_native_ui_startup_log(void);
#endif

int amiberry_main(int argc, char* argv[]);

int main(int argc, char* argv[])
{
#ifdef AMIBERRY_NATIVE_APPLE_UI
	amiberry_native_ui_startup_log();
#endif
	return amiberry_main(argc, argv);
}
