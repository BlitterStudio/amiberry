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

void console_out (const TCHAR *format,...)
{
    //va_list parms;
    //TCHAR buffer[WRITE_LOG_BUF_SIZE];

    //va_start (parms, format);
    //vsnprintf (buffer, WRITE_LOG_BUF_SIZE-1, format, parms);
    //va_end (parms);
    //printf(buffer);
}

void writeconsole(const TCHAR *buffer)
{
	
}

void write_log (const char *format,...)
{
    //int count;
    //int numwritten;
    //TCHAR buffer[WRITE_LOG_BUF_SIZE];

    //va_list parms;
    //va_start (parms, format);
    //count = vsnprintf( buffer, WRITE_LOG_BUF_SIZE-1, format, parms );
    //if( debugfile )
    //{
    //    fprintf( debugfile, buffer );
    //    fflush (debugfile);
    //}
    //va_end (parms);
}

void jit_abort (const TCHAR *format,...)
{
    static int happened;
    int count;
    TCHAR buffer[WRITE_LOG_BUF_SIZE];
    va_list parms;
    va_start (parms, format);

    count = vsnprintf (buffer, WRITE_LOG_BUF_SIZE - 1, format, parms);
	writeconsole(buffer);
    va_end (parms);
    if (!happened)
        gui_message ("JIT: Serious error:\n%s", buffer);
    happened = 1;
    uae_reset (1, 0);
}
