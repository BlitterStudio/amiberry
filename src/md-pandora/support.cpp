#include "config.h"
#include "sysconfig.h"

#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>

#include "sysdeps.h"
#include "options.h"
#include "events.h"
#include "custom.h"


extern int screen_is_picasso;

int64_t g_uae_epoch = 0;


int machdep_init (void)
{
  picasso_requested_on = 0;
  picasso_on = 0;
  screen_is_picasso = 0;

  // Initialize timebase
  g_uae_epoch = read_processor_time();
  syncbase = 1000000; // Microseconds

  return 1;
}


void machdep_free (void)
{
}
