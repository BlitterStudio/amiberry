#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "custom.h"
#include "xwin.h"

int64_t g_uae_epoch = 0;

void uae_time_calibrate(void)
{
	// Initialize timebase
	g_uae_epoch = read_processor_time();
	syncbase = 1000000; // Microseconds
}