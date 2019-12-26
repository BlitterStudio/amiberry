/*
 * UAE - The Un*x Amiga Emulator
 *
 * Standard write_log that writes to the console
 *
 * Copyright 2001 Bernd Schmidt
 */
#include <cstdarg>
#include <cstdio>
#include <iostream>

#include "sysdeps.h"
#include "uae.h"

#define WRITE_LOG_BUF_SIZE 4096
FILE *debugfile = NULL;
extern bool write_logfile;

void console_out (const TCHAR *format,...)
{
    va_list parms;
    TCHAR buffer[WRITE_LOG_BUF_SIZE];

    va_start (parms, format);
    vsnprintf (buffer, WRITE_LOG_BUF_SIZE-1, format, parms);
    va_end (parms);
    cout << buffer << endl;
}

void write_log (const char *format,...)
{
    // Redirect logging to Android's logcat
#ifdef ANDROID
    va_list parms;
    va_start(parms, format);
    SDL_Log(format, parms);
    va_end(parms);
#else
	if (write_logfile)
	{
		TCHAR buffer[WRITE_LOG_BUF_SIZE];

		va_list parms;
		va_start(parms, format);
		auto count = vsnprintf(buffer, WRITE_LOG_BUF_SIZE - 1, format, parms);
		if (debugfile)
		{
			fprintf(debugfile, "%s", buffer);
			fflush(debugfile);
		}
		va_end(parms);
	}
#endif
}

void jit_abort (const TCHAR *format,...)
{
    static int happened;
    TCHAR buffer[WRITE_LOG_BUF_SIZE];
    va_list parms;
    va_start (parms, format);

    auto count = vsnprintf(buffer, WRITE_LOG_BUF_SIZE - 1, format, parms);
    write_log (buffer);
    va_end (parms);
    if (!happened)
        gui_message ("JIT: Serious error:\n%s", buffer);
    happened = 1;
    uae_reset (1, 0);
}
