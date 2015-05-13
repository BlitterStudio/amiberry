#include "config.h"
#include "sysconfig.h"

#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>

#include "sysdeps.h"
#include "options.h"
#include "events.h"


int64_t g_uae_epoch = 0;


void machdep_init (void)
{
  // Initialize timebase
  g_uae_epoch = read_processor_time();
  syncbase = 1000000; // Microseconds
}


void machdep_free (void)
{
}
