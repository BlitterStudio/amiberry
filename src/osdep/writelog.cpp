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

FILE* debugfile = nullptr;
static int debugger_type = -1;

#define WRITE_LOG_BUF_SIZE 4096

int consoleopen = 0;
static int realconsole;
static TCHAR *console_buffer;
static int console_buffer_size;

void flush_log(void) {

}

static void openconsole (void)
{
#ifdef _WIN32
	if (realconsole) {
		if (debugger_type == 2) {
			open_debug_window ();
			consoleopen = 1;
		} else {
			close_debug_window ();
			consoleopen = -1;
		}
		return;
	}
	if (debugger_active && (debugger_type < 0 || debugger_type == 2)) {
		if (consoleopen > 0 || debuggerinitializing)
			return;
		if (debugger_type < 0) {
#ifdef FSUAE
#else
			regqueryint (NULL, _T("DebuggerType"), &debugger_type);
#endif
			if (debugger_type <= 0)
				debugger_type = 1;
			openconsole();
			return;
		}
		close_console ();
		if (open_debug_window ()) {
			consoleopen = 1;
			return;
		}
		open_console_window ();
	} else {
		if (consoleopen < 0)
			return;
		close_console ();
		open_console_window ();
	}
#endif
}

void activate_console (void)
{
	if (!consoleopen)
		return;
#ifdef _WIN32
	SetForegroundWindow (GetConsoleWindow ());
#else

#endif
}

void debugger_change (int mode)
{
	if (mode < 0)
		debugger_type = debugger_type == 2 ? 1 : 2;
	else
		debugger_type = mode;
	if (debugger_type != 1 && debugger_type != 2)
		debugger_type = 2;
#ifdef AMIBERRY
	// Not using the registry
#else
	regsetint (NULL, _T("DebuggerType"), debugger_type);
#endif
	openconsole ();
}

void update_debug_info(void) {
	// used to update debug info in debugger UI , currently Amiberry only supports
	// using console debugging on Linux/Mac OS X.
}

void reopen_console (void)
{
#ifdef _WIN32
	HWND hwnd;

	if (realconsole)
		return;
	if (consoleopen >= 0)
		return;
	hwnd = myGetConsoleWindow ();
	if (hwnd) {
		int newpos = 1;
		int x, y, w, h;
#ifdef FSUAE
		newpos = 0;
#else
		if (!regqueryint (NULL, _T("LoggerPosX"), &x))
			newpos = 0;
		if (!regqueryint (NULL, _T("LoggerPosY"), &y))
			newpos = 0;
		if (!regqueryint (NULL, _T("LoggerPosW"), &w))
			newpos = 0;
		if (!regqueryint (NULL, _T("LoggerPosH"), &h))
			newpos = 0;
#endif
		if (newpos) {
			RECT rc;
			rc.left = x;
			rc.top = y;
			rc.right = x + w;
			rc.bottom = y + h;
			if (MonitorFromRect (&rc, MONITOR_DEFAULTTONULL) != NULL) {
				SetForegroundWindow (hwnd);
				SetWindowPos (hwnd, HWND_TOP, x, y, w, h, SWP_NOACTIVATE);

			}
		}
	}
#endif
}

void close_console (void)
{
#ifdef _WIN32
	if (realconsole)
		return;
	if (consoleopen > 0) {
		close_debug_window ();
	} else if (consoleopen < 0) {
		HWND hwnd = myGetConsoleWindow ();
		if (hwnd && !IsIconic (hwnd)) {
			RECT r;
			if (GetWindowRect (hwnd, &r)) {
				r.bottom -= r.top;
				r.right -= r.left;
#ifdef FSUAE
#else
				regsetint (NULL, _T("LoggerPosX"), r.left);
				regsetint (NULL, _T("LoggerPosY"), r.top);
				regsetint (NULL, _T("LoggerPosW"), r.right);
				regsetint (NULL, _T("LoggerPosH"), r.bottom);
#endif
			}
		}
		FreeConsole ();
	}
#else

#endif
	consoleopen = 0;
}

static void writeconsole_2 (const TCHAR *buffer)
{
	unsigned int temp;

	if (!consoleopen)
		openconsole ();

	if (consoleopen > 0) {
#ifdef _WIN32
		WriteOutput (buffer, _tcslen (buffer));
#endif
	} else if (realconsole) {
#ifdef AMIBERRY
		fputs (buffer, stdout);
#else
		fputws (buffer, stdout);
#endif
		fflush (stdout);
	} else if (consoleopen < 0) {
#ifdef _WIN32
		WriteConsole (stdoutput, buffer, _tcslen (buffer), &temp, 0);
#endif
	}
}

static void writeconsole (const TCHAR *buffer)
{
	if (_tcslen (buffer) > 256) {
		TCHAR *p = my_strdup (buffer);
		TCHAR *p2 = p;
		while (_tcslen (p) > 256) {
			TCHAR tmp = p[256];
			p[256] = 0;
			writeconsole_2 (p);
			p[256] = tmp;
			p += 256;
		}
		writeconsole_2 (p);
		xfree (p2);
	} else {
		writeconsole_2 (buffer);
	}
}

TCHAR *setconsolemode (TCHAR *buffer, int maxlen)
{
	TCHAR *ret = NULL;
	if (buffer) {
		console_buffer = buffer;
		console_buffer_size = maxlen;
	} else {
		ret = console_buffer;
		console_buffer = NULL;
	}
	return ret;
}

static void console_put (const TCHAR *buffer)
{
	if (console_buffer) {
		if (_tcslen (console_buffer) + _tcslen (buffer) < console_buffer_size)
			_tcscat (console_buffer, buffer);
	} else {
		openconsole ();
		writeconsole (buffer);
	}
}

static int console_buf_len = 100000;

void console_out_f (const TCHAR *format,...)
{
	va_list arg_ptr;
	va_start(arg_ptr, format);
	vprintf(format, arg_ptr);
	va_end(arg_ptr);
}

void console_out(const TCHAR* format, ...)
{
	va_list parms;
	TCHAR buffer[WRITE_LOG_BUF_SIZE];

	va_start(parms, format);
	_vsntprintf(buffer, WRITE_LOG_BUF_SIZE - 1, format, parms);
	va_end(parms);
	cout << buffer << endl;
}

bool console_isch (void)
{
#ifdef _WIN32
	flushmsgpump();
	if (console_buffer) {
		return 0;
	} else if (realconsole) {
		return false;
	} else if (consoleopen < 0) {
		DWORD events = 0;
		GetNumberOfConsoleInputEvents (stdinput, &events);
		return events > 0;
	}
	return false;
#else
	return false;
#endif
}

void f_out(FILE* f, const TCHAR* format, ...)
{
	if (f == nullptr)
	{
		return;
	}
	TCHAR buffer[WRITE_LOG_BUF_SIZE];
	va_list parms;
	va_start(parms, format);
	_vsntprintf(buffer, WRITE_LOG_BUF_SIZE - 1, format, parms);
	va_end(parms);
	cout << buffer << endl;
}

TCHAR* buf_out (TCHAR *buffer, int *bufsize, const TCHAR *format, ...) {
	if (buffer == NULL) {
		return 0;
	}
	va_list parms;
	va_start (parms, format);
	vsnprintf (buffer, (*bufsize) - 1, format, parms);
	va_end (parms);
	*bufsize -= _tcslen (buffer);
	return buffer + _tcslen (buffer);
}

void console_out (const TCHAR *txt)
{
	printf("%s", txt);
}

TCHAR console_getch(void)
{
	//flushmsgpump();
	if (console_buffer)
	{
		return 0;
	}
	if (realconsole)
	{
		return getwc(stdin);
	}
	if (consoleopen < 0)
	{
		unsigned long len;

		for (;;)
		{
			const auto out = getchar();
			putchar(out);
			return out;
		}
	}
	return 0;
}

int console_get (TCHAR *out, int maxlen)
{
#ifdef _WIN32
	*out = 0;

	flushmsgpump();
	set_console_input_mode(1);
	if (consoleopen > 0) {
		return console_get_gui (out, maxlen);
	} else if (realconsole) {
		DWORD totallen;

		*out = 0;
		totallen = 0;
		while (maxlen > 0) {
			*out = getwc (stdin);
			if (*out == 13)
				break;
			out++;
			maxlen--;
			totallen++;
		}
		*out = 0;
		return totallen;
	} else if (consoleopen < 0) {
		DWORD len, totallen;

		*out = 0;
		totallen = 0;
		while(maxlen > 0) {
			ReadConsole (stdinput, out, 1, &len, 0);
			if(*out == 13)
				break;
			out++;
			maxlen--;
			totallen++;
		}
		*out = 0;
		return totallen;
	}
	return 0;
#else
	TCHAR *res = fgets(out, maxlen, stdin);
	if (res == NULL) {
		return -1;
	}
	int len = strlen(out);
	return len - 1;
#endif
}

void console_flush (void)
{
	fflush(stdout);
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

		va_list parms{};
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

void jit_abort(const TCHAR* format, ...)
{
	static int happened;
	TCHAR buffer[WRITE_LOG_BUF_SIZE];
	va_list parms{};
	va_start(parms, format);

	auto count = vsnprintf(buffer, WRITE_LOG_BUF_SIZE - 1, format, parms);
	write_log(buffer);
	va_end(parms);
	if (!happened)
		gui_message("JIT: Serious error:\n%s", buffer);
	happened = 1;
	uae_reset(1, 0);
}
