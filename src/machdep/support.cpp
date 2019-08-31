#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "custom.h"
#include "xwin.h"

extern int screen_is_picasso;

int64_t g_uae_epoch = 0;


int machdep_init (void)
{
	struct amigadisplay *ad = &adisplays;

  ad->picasso_requested_on = 0;
  ad->picasso_on = 0;
  screen_is_picasso = 0;

  // Initialize timebase
  g_uae_epoch = read_processor_time();
  syncbase = 1000000; // Microseconds
  
  return 1;
}


void machdep_free (void)
{
}
