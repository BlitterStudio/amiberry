 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Standard write_log that writes to the console
  *
  * Copyright 2001 Bernd Schmidt
  */
#include "sysconfig.h"
#include "sysdeps.h"

#define WRITE_LOG_BUF_SIZE 4096
FILE *debugfile = NULL;

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

