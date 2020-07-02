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
#include "options.h"

#define WRITE_LOG_BUF_SIZE 4096
FILE *debugfile = NULL;

int consoleopen = 0;
static int realconsole;
static TCHAR* console_buffer;

void console_out (const TCHAR *format,...)
{
    va_list parms;
    TCHAR buffer[WRITE_LOG_BUF_SIZE];

    va_start (parms, format);
    vsnprintf (buffer, WRITE_LOG_BUF_SIZE-1, format, parms);
    va_end (parms);
    cout << buffer << endl;
}

void f_out(FILE* f, const TCHAR* format, ...)
{
    if (f == NULL) {
        return;
    }
    va_list arg_ptr;
    va_start(arg_ptr, format);
    vfprintf(f, format, arg_ptr);
    va_end(arg_ptr);
}

TCHAR console_getch(void)
{
    //flushmsgpump();
    if (console_buffer) {
        return 0;
    }
    else if (realconsole) {
        return getwc(stdin);
    }
    else if (consoleopen < 0) {
        unsigned long len;

        for (;;) {
	        const auto out = getchar();
            putchar(out);
            return out;
        }
    }
    return 0;
}

void write_log(const char* format, ...)
{
    if (amiberry_options.write_logfile)
    {
        // Redirect logging to Android's logcat
#ifdef ANDROID
        va_list parms;
        va_start(parms, format);
        SDL_Log(format, parms);
        va_end(parms);
#else

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

#endif
    }
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
