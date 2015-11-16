 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Standard write_log that writes to the console
  *
  * Copyright 2001 Bernd Schmidt
  */
#include "sysconfig.h"
#include "sysdeps.h"
#include "uae.h"


#define WRITE_LOG_BUF_SIZE 4096
FILE *debugfile = NULL;

void console_out (const char *format,...)
{
    va_list parms;
    char buffer[WRITE_LOG_BUF_SIZE];

    va_start (parms, format);
    vsnprintf (buffer, WRITE_LOG_BUF_SIZE-1, format, parms);
    va_end (parms);
    printf(buffer);
}

#ifdef WITH_LOGGING

void write_log (const char *format,...)
{
  int count;
  int numwritten;
  char buffer[WRITE_LOG_BUF_SIZE];

  va_list parms;
  va_start (parms, format);
  count = vsnprintf( buffer, WRITE_LOG_BUF_SIZE-1, format, parms );
  if( debugfile ) {
	  fprintf( debugfile, buffer );
	  fflush (debugfile);
  }
  va_end (parms);
}

#endif

void jit_abort (const char *format,...)
{
    static int happened;
    int count;
    char buffer[WRITE_LOG_BUF_SIZE];
    va_list parms;
    va_start (parms, format);

    count = vsnprintf (buffer, WRITE_LOG_BUF_SIZE - 1, format, parms);
    write_log (buffer);
    va_end (parms);
    if (!happened)
	gui_message ("JIT: Serious error:\n%s", buffer);
    happened = 1;
    uae_reset (1);
}
