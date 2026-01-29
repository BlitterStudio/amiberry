#include "input_platform.h"

void osdep_init_keyboard(int* keyboard_german, int* retroarch_inited, int* num_retroarch_kbdjoy)
{
	if (keyboard_german)
		*keyboard_german = 0;
	if (retroarch_inited)
		*retroarch_inited = 1;
	if (num_retroarch_kbdjoy)
		*num_retroarch_kbdjoy = 0;
}
