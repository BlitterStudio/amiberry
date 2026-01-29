#include "sysdeps.h"
#include "input_platform.h"

#include <SDL.h>

#include <string>

#include "amiberry_input.h"
#include "fsdb.h"
#include "target.h"

void osdep_init_keyboard(int* keyboard_german, int* retroarch_inited, int* num_retroarch_kbdjoy)
{
	if (!keyboard_german || !retroarch_inited || !num_retroarch_kbdjoy)
		return;

	*keyboard_german = 0;
	if (SDL_GetKeyFromScancode(SDL_SCANCODE_Y) == SDLK_z)
		*keyboard_german = 1;

	if (*retroarch_inited)
		return;

	std::string retroarch_file = get_retroarch_file();
	if (my_existsfile2(retroarch_file.c_str()))
	{
		bool valid = true;
		for (int kb = 0; kb < 4 && valid; ++kb)
		{
			valid = init_kb_from_retroarch(kb, retroarch_file);
			if (valid)
				(*num_retroarch_kbdjoy)++;
		}
	}
	*retroarch_inited = 1;
}
