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
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "custom.h"
#include "events.h"
#include "debug.h"
#include "uae.h"

#define SHOW_CONSOLE 0

static int nodatestamps = 0;

int consoleopen = 1;
static int realconsole;
static int bootlogmode;

FILE* debugfile = nullptr;
int console_logging = 0;
static int debugger_type = -1;
//extern BOOL debuggerinitializing;
BOOL debuggerinitializing = false;
extern int lof_store;
static int console_input_linemode = -1;
int always_flush_log = 1;
TCHAR* conlogfile = nullptr;

#define WRITE_LOG_BUF_SIZE 4096

/* console functions for debugger */

bool is_console_open()
{
	return consoleopen;
}

static void getconsole()
{
	struct termios term{};
	struct winsize ws{};

	// Get the terminal attributes
	tcgetattr(STDIN_FILENO, &term);

	// Set the terminal attributes for line input and echo
	term.c_lflag |= (ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &term);

	// Set the console input/output code page to UTF-8
	setlocale(LC_ALL, "en_US.UTF-8");

	// Get the console window size
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

	// Set the console window size to a maximum of 5000 lines
	if (ws.ws_row < 5000)
	{
		ws.ws_row = 5000;
		ioctl(STDOUT_FILENO, TIOCSWINSZ, &ws);
	}
}

void activate_console()
{
	if (!consoleopen)
		return;
	//SetForegroundWindow(GetConsoleWindow());
}

static void open_console_window()
{
	//AllocConsole();
	getconsole();
	consoleopen = -1;
	reopen_console();
}

static void openconsole ()
{
	if (realconsole) {
		if (debugger_type == 2) {
			//open_debug_window ();
			consoleopen = 1;
		} else {
			//close_debug_window ();
			consoleopen = -1;
		}
		return;
	}
	if (debugger_active && (debugger_type < 0 || debugger_type == 2)) {
		if (consoleopen > 0 || debuggerinitializing)
			return;
		if (debugger_type < 0) {
#ifdef AMIBERRY
#else
			regqueryint (NULL, _T("DebuggerType"), &debugger_type);
#endif
			if (debugger_type <= 0)
				debugger_type = 1;
			openconsole();
			return;
		}
		close_console ();
		//if (open_debug_window ()) {
		//	consoleopen = 1;
		//	return;
		//}
		open_console_window ();
	} else {
		if (consoleopen < 0)
			return;
		close_console ();
		open_console_window ();
	}
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

void update_debug_info() {
	// used to update debug info in debugger UI , currently Amiberry only supports
	// using console debugging on Linux/Mac OS X.
}

void open_console()
{
	if (!consoleopen) {
		openconsole();
	}
}

void reopen_console ()
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

void close_console ()
{
	if (realconsole)
		return;
#ifdef _WIN32
	if (consoleopen > 0) {
		close_debug_window ();
	} else if (consoleopen < 0) {
		HWND hwnd = myGetConsoleWindow ();
		if (hwnd && !IsIconic (hwnd)) {
			RECT r;
			if (GetWindowRect (hwnd, &r)) {
				r.bottom -= r.top;
				r.right -= r.left;
				int dpi = getdpiforwindow(hwnd);
				r.left = r.left * 96 / dpi;
				r.right = r.right * 96 / dpi;
				r.top = r.top * 96 / dpi;
				r.bottom = r.bottom * 96 / dpi;
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

int read_log()
{
	if (consoleopen >= 0)
		return -1;
	//TODO needs implementation
	return -1;
}

static void writeconsole_2 (const TCHAR *buffer)
{
	if (!consoleopen)
		openconsole ();

	if (consoleopen > 0) {
		SDL_Log("%s", buffer);
	}
	else if (realconsole) {
		fprintf(stdout, "%s", buffer);
		fflush(stdout);
	}
	else if (consoleopen < 0) {
		SDL_Log("%s", buffer);
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

static void flushconsole()
{
	if (consoleopen > 0) {
		fflush(stdout);
	}
	else if (realconsole) {
		fflush(stdout);
	}
}

static TCHAR* console_buffer;
static int console_buffer_size;

TCHAR *setconsolemode (TCHAR *buffer, const int maxlen)
{
	TCHAR *ret = nullptr;
	if (buffer) {
		console_buffer = buffer;
		console_buffer_size = maxlen;
	} else {
		ret = console_buffer;
		console_buffer = nullptr;
	}
	return ret;
}

static void console_put (const TCHAR *buffer)
{
	if (console_buffer) {
		if (_tcslen (console_buffer) + _tcslen (buffer) < console_buffer_size)
			_tcscat (console_buffer, buffer);
	} else if (consoleopen) {
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

void console_out(const TCHAR* txt)
{
	console_put(txt);
}

bool console_isch ()
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

TCHAR console_getch()
{
	fflush(stdout);
	if (console_buffer)
	{
		return 0;
	}
	if (realconsole)
	{
		return getchar();
	}
	if (consoleopen < 0)
	{
		char out[2];
		out[0] = getchar();
		out[1] = '\0';
		return out[0];
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
	if (res == nullptr) {
		return -1;
	}
	int len = strlen(out);
	while (len > 0 && (out[len - 1] == '\r' || out[len - 1] == '\n')) {
		out[len - 1] = 0;
		len--;
	}
	return len;
#endif
}

void console_flush (void)
{
	flushconsole();
}

static int lfdetected = 1;

TCHAR* write_log_get_ts(void)
{
	static char out[100];
	static char lastts[100];
	char curts[100];

	if (bootlogmode)
		return nullptr;
	if (nodatestamps)
		return nullptr;
	if (!vsync_counter)
		return nullptr;

	Uint32 ticks = SDL_GetTicks();
	time_t seconds = ticks / 1000;
	int milliseconds = ticks % 1000;

	struct tm* t = localtime(&seconds);
	strftime(curts, sizeof(curts), "%Y-%m-%d %H:%M:%S\n", t);

	char* p = out;
	*p = 0;
	if (strncmp(curts, lastts, strlen(curts) - 3)) { // "xx\n"
		strcat(p, curts);
		p += strlen(p);
		strcpy(lastts, curts);
	}
	strftime(p, sizeof(out) - (p - out), "%S-", t);
	p += strlen(p);
	_sntprintf(p, sizeof p, "%03d", milliseconds);
	p += strlen(p);
	if (vsync_counter != 0xffffffff)
		_sntprintf(p, sizeof p, " [%d %03d%s%03d]", vsync_counter, current_hpos_safe(), lof_store ? "-" : "=", vpos);
	strcat(p, ": ");
	return out;
}

void write_log(const char* format, ...)
{
	int count;
	TCHAR buffer[WRITE_LOG_BUF_SIZE], *ts;
	int bufsize = WRITE_LOG_BUF_SIZE;
	TCHAR* bufp;
	va_list parms;

	if (!amiberry_options.write_logfile && !console_logging && !debugfile)
		return;

	if (!_tcsicmp(format, _T("*")))
		count = 0;

	va_start(parms, format);
	bufp = buffer;
	for (;;) {
		count = _vsntprintf(bufp, bufsize - 1, format, parms);
		if (count < 0) {
			bufsize *= 10;
			if (bufp != buffer)
				xfree(bufp);
			bufp = xmalloc(TCHAR, bufsize);
			continue;
		}
		break;
	}
	bufp[bufsize - 1] = 0;
	if (!_tcsncmp(bufp, _T("write "), 6))
		bufsize--;
	ts = write_log_get_ts();
	if (bufp[0] == '*')
		count++;
	if (SHOW_CONSOLE || console_logging) {
		if (lfdetected && ts)
			writeconsole(ts);
		writeconsole(bufp);
	}
	if (debugfile) {
		if (lfdetected && ts)
			fprintf(debugfile, _T("%s"), ts);
		fprintf(debugfile, _T("%s"), bufp);
	}
	lfdetected = 0;
	if (bufp[0] != '\0' && bufp[_tcslen(bufp) - 1] == '\n')
		lfdetected = 1;
	va_end(parms);
	if (bufp != buffer)
		xfree(bufp);
	if (always_flush_log)
		flush_log();
}

void flush_log()
{
	if (debugfile)
		fflush(debugfile);
	flushconsole();
}

void f_out(FILE* f, const TCHAR* format, ...)
{
	TCHAR buffer[WRITE_LOG_BUF_SIZE];
	va_list parms;
	va_start(parms, format);

	if (f == nullptr || !consoleopen)
		return;
	_vsntprintf(buffer, WRITE_LOG_BUF_SIZE - 1, format, parms);
	openconsole();
	writeconsole(buffer);
	va_end(parms);
}

TCHAR* buf_out(TCHAR* buffer, int* bufsize, const TCHAR* format, ...)
{
	va_list parms;
	va_start(parms, format);

	if (buffer == nullptr)
		return nullptr;
	_vsntprintf(buffer, (*bufsize) - 1, format, parms);
	va_end(parms);
	*bufsize -= _tcslen(buffer);
	return buffer + _tcslen(buffer);
}

void log_close(FILE* f)
{
	if (f)
		fclose(f);
}

void jit_abort(const TCHAR* format, ...)
{
	static bool happened = false;
	TCHAR buffer[WRITE_LOG_BUF_SIZE];
	va_list parms;
	va_start(parms, format);

	auto count = vsnprintf(buffer, WRITE_LOG_BUF_SIZE - 1, format, parms);
	buffer[count] = '\0'; // Ensure null termination
	va_end(parms);

	if (!happened)
	{
		gui_message("JIT: Serious error:\n%s", buffer);
		happened = true;
	}

	write_log("%s", buffer); // Use format specifier to prevent potential format string vulnerabilities
	uae_reset(1, 0);
}
