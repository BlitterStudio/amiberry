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
#include <clocale>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <conio.h>
#include <io.h>
#else
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <poll.h>
#endif

#if defined(__ANDROID__)
#include <android/log.h>
#endif

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "custom.h"
#include "events.h"
#include "debug.h"
#include "imgui_debugger_console.h"
#include "macos_debugger_console.h"
#include "uae.h"
#include "registry.h"
#include "threaddep/thread.h"

#define SHOW_CONSOLE 0

static int nodatestamps = 0;

int consoleopen = 1;
static int realconsole;
static int bootlogmode;

FILE* debugfile = nullptr;
int console_logging = 0;
static int debugger_type = -1;
BOOL debuggerinitializing = false;
extern bool lof_store;
static int console_input_linemode = -1;
int always_flush_log = 1;
TCHAR* conlogfile = nullptr;
static SDL_Window* previousactivewindow;

#define WRITE_LOG_BUF_SIZE 4096

static uae_sem_t log_sem;
static int log_sem_init;

#if defined(AMIBERRY_MACOS) || (!defined(__ANDROID__) && !defined(AMIBERRY_IOS))
static constexpr int CONSOLEOPEN_DEBUGGER_WINDOW = 2;

static bool console_debugger_available()
{
#if defined(_WIN32)
	return true;
#else
	return console_logging && isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
#endif
}

static bool debugger_window_supported()
{
#if defined(AMIBERRY_MACOS)
	return macos_debugger_console_supported();
#else
	return imgui_debugger_console_supported();
#endif
}

static bool use_debugger_window()
{
	return debugger_window_supported() && (!console_debugger_available() || debugger_type == 2);
}

static void open_debugger_window()
{
	if (!consoleopen) {
		previousactivewindow = SDL_GetKeyboardFocus();
	}
#if defined(AMIBERRY_MACOS)
	macos_debugger_console_open();
#else
	imgui_debugger_console_open();
#endif
	consoleopen = CONSOLEOPEN_DEBUGGER_WINDOW;
}

static void close_debugger_window()
{
#if defined(AMIBERRY_MACOS)
	macos_debugger_console_close();
#else
	imgui_debugger_console_close();
#endif
}

static void activate_debugger_window()
{
#if defined(AMIBERRY_MACOS)
	macos_debugger_console_activate();
#else
	imgui_debugger_console_activate();
#endif
}

static void write_debugger_window(const TCHAR* text)
{
#if defined(AMIBERRY_MACOS)
	macos_debugger_console_write(text);
#else
	imgui_debugger_console_write(text);
#endif
}

static bool debugger_window_has_input()
{
#if defined(AMIBERRY_MACOS)
	return macos_debugger_console_has_input();
#else
	return imgui_debugger_console_has_input();
#endif
}

static TCHAR debugger_window_getch()
{
#if defined(AMIBERRY_MACOS)
	return macos_debugger_console_getch();
#else
	return imgui_debugger_console_getch();
#endif
}

static int debugger_window_get(TCHAR* out, const int maxlen)
{
#if defined(AMIBERRY_MACOS)
	return macos_debugger_console_get(out, maxlen);
#else
	return imgui_debugger_console_get(out, maxlen);
#endif
}
#endif

/* console functions for debugger */


bool is_console_open()
{
	return consoleopen;
}

static void restore_console_settings()
{
	set_console_input_mode(1);
}

void set_console_input_mode(int line)
{
	if (console_input_linemode < 0)
		return;
	if (line == console_input_linemode)
		return;
#ifdef _WIN32
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	if (hStdin != INVALID_HANDLE_VALUE) {
		DWORD mode;
		GetConsoleMode(hStdin, &mode);
		if (line)
			mode |= (ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
		else
			mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
		SetConsoleMode(hStdin, mode);
		console_input_linemode = line;
	}
#else
	struct termios term;
	if (tcgetattr(STDIN_FILENO, &term) == 0) {
		if (line)
			term.c_lflag |= (ICANON | ECHO);
		else
			term.c_lflag &= ~(ICANON | ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &term);
		console_input_linemode = line;
	}
#endif
}

static void getconsole()
{
#ifdef _WIN32
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	if (hStdin != INVALID_HANDLE_VALUE) {
		DWORD mode;
		GetConsoleMode(hStdin, &mode);
		mode |= (ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
		SetConsoleMode(hStdin, mode);
		console_input_linemode = 1;
		std::atexit(restore_console_settings);
	}

	// Set the console code pages to UTF-8
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
#else
	struct termios term{};
	struct winsize ws{};

	// Get the terminal attributes
	if (tcgetattr(STDIN_FILENO, &term) == 0) {
		// Set the terminal attributes for line input and echo
		term.c_lflag |= (ICANON | ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &term);
		console_input_linemode = 1;
		std::atexit(restore_console_settings);
	}

	// Set the console input/output code page to UTF-8
	setlocale(LC_ALL, "en_US.UTF-8");

	// Get the console window size
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
		// No direct way to set terminal buffer size on Linux like Windows,
		// but we can log that we are initializing.
	}
#endif
}

void deactivate_console(void)
{
	if (previousactivewindow) {
		SDL_RaiseWindow(previousactivewindow);
		previousactivewindow = NULL;
	}
}

void activate_console()
{
	if (!consoleopen) {
		previousactivewindow = NULL;
		return;
	}
	previousactivewindow = SDL_GetKeyboardFocus();
#if defined(AMIBERRY_MACOS) || (!defined(__ANDROID__) && !defined(AMIBERRY_IOS))
	if (consoleopen == CONSOLEOPEN_DEBUGGER_WINDOW) {
		activate_debugger_window();
	}
#endif
}

static void open_console_window()
{
#ifdef _WIN32
	/* When built with -mwindows (Release), there is no console.
	 * Allocate one on demand so log output is visible.
	 * This mirrors WinUAE's open_console_window(). */
	if (AllocConsole()) {
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
		freopen("CONIN$", "r", stdin);
	}
#endif
	getconsole();
	consoleopen = -1;
	reopen_console();
}

static void openconsole ()
{
#if defined(AMIBERRY_MACOS) || (!defined(__ANDROID__) && !defined(AMIBERRY_IOS))
	if (debugger_type < 0) {
		regqueryint(NULL, _T("DebuggerType"), &debugger_type);
		if (debugger_type <= 0)
			debugger_type = console_debugger_available() ? 1 : 2;
	}
	if (debugger_type == 1 && !console_debugger_available()) {
		debugger_type = 2;
	}
	if (use_debugger_window()) {
		if (consoleopen == CONSOLEOPEN_DEBUGGER_WINDOW || debuggerinitializing)
			return;
		close_console();
		open_debugger_window();
		return;
	}
#ifdef _WIN32
	if (consoleopen >= 0) {
		if (!consoleopen)
			previousactivewindow = SDL_GetKeyboardFocus();
		close_console();
		open_console_window();
	}
#else
	if (consoleopen != 1) {
		if (!consoleopen)
			previousactivewindow = SDL_GetKeyboardFocus();
		close_console();
		getconsole();
		consoleopen = 1;
	}
#endif
#else
	if (realconsole) {
		if (debugger_type == 2) {
			consoleopen = 1;
		} else {
			consoleopen = -1;
		}
		return;
	}
	if (debugger_active && (debugger_type < 0 || debugger_type == 2)) {
		if (consoleopen > 0 || debuggerinitializing)
			return;
		if (debugger_type < 0) {
			regqueryint (NULL, _T("DebuggerType"), &debugger_type);
			if (debugger_type <= 0)
				debugger_type = 1;
			openconsole();
			return;
		}
		close_console ();
		open_console_window ();
	} else {
		if (consoleopen < 0)
			return;
		close_console ();
		open_console_window ();
	}
#endif
}

void debugger_change (int mode)
{
#if defined(AMIBERRY_MACOS) || (!defined(__ANDROID__) && !defined(AMIBERRY_IOS))
	const bool can_use_console_debugger = console_debugger_available();
	if (mode < 0) {
		if (debugger_type == 2 && can_use_console_debugger)
			debugger_type = 1;
		else
			debugger_type = 2;
	} else {
		debugger_type = mode;
	}
	if (debugger_type == 1 && !can_use_console_debugger)
		debugger_type = 2;
#else
	if (mode < 0)
		debugger_type = debugger_type == 2 ? 1 : 2;
	else
		debugger_type = mode;
#endif
	if (debugger_type != 1 && debugger_type != 2)
		debugger_type = 2;
	regsetint (NULL, _T("DebuggerType"), debugger_type);
	openconsole ();
}

void update_debug_info() {
	// used to update debug info in debugger UI.
}

void open_console()
{
	if (!log_sem_init) {
		uae_sem_init(&log_sem, 0, 1);
		log_sem_init = 1;
	}
#if defined(AMIBERRY_MACOS)
	openconsole();
	return;
#endif
	if (!consoleopen) {
		openconsole();
	}
}

bool is_interactive_console(void)
{
#if defined(AMIBERRY_MACOS) || (!defined(__ANDROID__) && !defined(AMIBERRY_IOS))
	return console_logging || debugger_window_supported();
#else
	return console_logging;
#endif
}

void reopen_console ()
{
#if defined(_WIN32) && !defined(AMIBERRY)
	HWND hwnd;

	if (realconsole)
		return;
	if (consoleopen >= 0)
		return;
	hwnd = myGetConsoleWindow ();
	if (hwnd) {
		int newpos = 1;
		int x, y, w, h;
		if (!regqueryint (NULL, _T("LoggerPosX"), &x))
			newpos = 0;
		if (!regqueryint (NULL, _T("LoggerPosY"), &y))
			newpos = 0;
		if (!regqueryint (NULL, _T("LoggerPosW"), &w))
			newpos = 0;
		if (!regqueryint (NULL, _T("LoggerPosH"), &h))
			newpos = 0;
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
#if defined(AMIBERRY_MACOS) || (!defined(__ANDROID__) && !defined(AMIBERRY_IOS))
	if (consoleopen == CONSOLEOPEN_DEBUGGER_WINDOW) {
		close_debugger_window();
	}
#endif
#ifdef _WIN32
	if (consoleopen < 0) {
		FreeConsole ();
	}
#endif
	consoleopen = 0;
}

int read_log()
{
	if (consoleopen >= 0)
		return -1;
	if (console_isch()) {
#ifdef _WIN32
		if (_kbhit())
			return _getch();
#else
		unsigned char ch;
		if (read(STDIN_FILENO, &ch, 1) == 1)
			return ch;
#endif
	}
	return -1;
}


static void writeconsole_2 (const TCHAR *buffer)
{
	if (!consoleopen)
		openconsole ();

#ifdef _WIN32
	/* In Release builds linked with -mwindows, stdout is disconnected.
	 * Allocate a console on demand so log output is actually visible. */
	static bool win_console_allocated = false;
	if (!win_console_allocated && (console_logging || SHOW_CONSOLE)) {
		if (AllocConsole()) {
			freopen("CONOUT$", "w", stdout);
			freopen("CONOUT$", "w", stderr);
			freopen("CONIN$", "r", stdin);
		}
		win_console_allocated = true;
	}
#endif

	if (
#if defined(AMIBERRY_MACOS) || (!defined(__ANDROID__) && !defined(AMIBERRY_IOS))
		consoleopen == CONSOLEOPEN_DEBUGGER_WINDOW
#else
		false
#endif
	) {
#if defined(AMIBERRY_MACOS) || (!defined(__ANDROID__) && !defined(AMIBERRY_IOS))
		write_debugger_window(buffer);
#endif
	}
	else if (consoleopen > 0) {
#ifdef _WIN32
		fprintf(stdout, "%s", buffer);
#else
		SDL_Log("%s", buffer);
#endif
	}
	else if (realconsole) {
		fprintf(stdout, "%s", buffer);
		fflush(stdout);
	}
	else if (consoleopen < 0) {
#ifdef _WIN32
		fprintf(stdout, "%s", buffer);
#else
		SDL_Log("%s", buffer);
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

static void flushconsole()
{
	if (consoleopen != 0) {
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
	TCHAR stack_buffer[WRITE_LOG_BUF_SIZE];
	TCHAR* bufp = stack_buffer;
	int bufsize = WRITE_LOG_BUF_SIZE;

	va_list arg_ptr;
	va_start(arg_ptr, format);
	for (;;) {
		va_list copy;
		va_copy(copy, arg_ptr);
		const int count = _vsntprintf(bufp, bufsize, format, copy);
		va_end(copy);
		if (count >= 0 && count < bufsize)
			break;
		// Either C99 truncation (count >= bufsize) or legacy MSVC truncation (count < 0).
		const int next = (count >= 0) ? (count + 1) : (bufsize * 4);
		if (next > 16 * 1024 * 1024) {
			// Give up and accept truncation rather than allocating unbounded memory.
			bufp[bufsize - 1] = 0;
			break;
		}
		if (bufp != stack_buffer)
			xfree(bufp);
		bufsize = next;
		bufp = xmalloc(TCHAR, bufsize);
	}
	va_end(arg_ptr);
	console_put(bufp);
	if (bufp != stack_buffer)
		xfree(bufp);
}

void console_out(const TCHAR* txt)
{
	console_put(txt);
}

bool console_isch ()
{
#if defined(_WIN32) && !defined(AMIBERRY)
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
#elif defined(_WIN32) && defined(AMIBERRY)
	if (console_buffer)
		return false;
	if (consoleopen == CONSOLEOPEN_DEBUGGER_WINDOW)
		return debugger_window_has_input();
	if (consoleopen < 0) {
		return _kbhit() != 0;
	}
	return false;
#else
	if (console_buffer)
		return false;
#if defined(AMIBERRY_MACOS) || (!defined(__ANDROID__) && !defined(AMIBERRY_IOS))
	if (consoleopen == CONSOLEOPEN_DEBUGGER_WINDOW)
		return debugger_window_has_input();
#endif
	struct pollfd fds;
	fds.fd = STDIN_FILENO;
	fds.events = POLLIN;
	return poll(&fds, 1, 0) == 1;
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
#if defined(AMIBERRY_MACOS) || (!defined(__ANDROID__) && !defined(AMIBERRY_IOS))
	if (consoleopen == CONSOLEOPEN_DEBUGGER_WINDOW)
	{
		return debugger_window_getch();
	}
#endif
	if (consoleopen < 0)
	{
		set_console_input_mode(0);
#ifdef _WIN32
		if (_kbhit())
			return _getch();
#else
		char ch;
		if (read(STDIN_FILENO, &ch, 1) == 1) {
			return ch;
		}
#endif
	}
	return 0;
}

int console_get (TCHAR *out, int maxlen)
{
#if defined(_WIN32) && !defined(AMIBERRY)
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
#if defined(AMIBERRY_MACOS) || (!defined(__ANDROID__) && !defined(AMIBERRY_IOS))
	if (consoleopen == CONSOLEOPEN_DEBUGGER_WINDOW)
		return debugger_window_get(out, maxlen);
#endif
	set_console_input_mode(1);
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


FILE *log_open (const TCHAR *name, int append, int bootlog, TCHAR *outpath)
{
	FILE *f = nullptr;

	if (!log_sem_init) {
		uae_sem_init(&log_sem, 0, 1);
		log_sem_init = 1;
	}

	if (name != nullptr) {
		if (bootlog >= 0) {
			_tcscpy (outpath, name);
			f = fopen(name, append ? _T("a") : _T("w"));
			if (!f && bootlog) {
				TCHAR tmp[MAX_DPATH];
	#ifdef _WIN32
				const char* tmpdir = getenv("TEMP");
				if (!tmpdir) tmpdir = ".";
				_sntprintf(tmp, MAX_DPATH, _T("%s\\amiberry_log.txt"), tmpdir);
#else
				_tcscpy(tmp, _T("/tmp/amiberry_log.txt"));
#endif
				_tcscpy (outpath, tmp);
				f = fopen(tmp, append ? _T("a") : _T("w"));
			}
		}
	} else {
		// Check for console flag? In Amiberry we usually just check console_logging
	}
	bootlogmode = bootlog;
	return f;
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

	uint64_t ticks = SDL_GetTicks();
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
	_sntprintf(p, 10, "%03d", milliseconds);
	p += strlen(p);
	if (vsync_counter != 0xffffffff)
		_sntprintf(p, 50, " [%u %03d%s%03d/%03d]", vsync_counter, current_hpos_safe(), lof_store ? "-" : "=", vpos, linear_display_vpos);
	strcat(p, ": ");
	return out;
}


void write_log(const char* format, ...)
{
	int count;
	TCHAR buffer[WRITE_LOG_BUF_SIZE], * ts;
	int bufsize = WRITE_LOG_BUF_SIZE;
	TCHAR* bufp;
	va_list parms;
	const bool has_libretro_log = false;

#ifndef __ANDROID__
	if (!has_libretro_log && !amiberry_options.write_logfile && !console_logging && !debugfile)
		return;
#endif

	if (log_sem_init) uae_sem_wait(&log_sem);

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
#if defined(__ANDROID__)
		__android_log_print(ANDROID_LOG_INFO, "Amiberry", "%s", bufp);
#endif
	if (SHOW_CONSOLE || console_logging) {
		if (lfdetected && ts)
			writeconsole(ts);
#if !defined(__ANDROID__)
		writeconsole(bufp);
#endif
	}
	if (debugfile) {
		if (lfdetected && ts)
			fprintf(debugfile, "%s", ts);
		fprintf(debugfile, "%s", bufp);
	}
	lfdetected = 0;
	if (bufp[0] != '\0' && bufp[_tcslen(bufp) - 1] == '\n')
		lfdetected = 1;
	va_end(parms);
	if (bufp != buffer)
		xfree(bufp);
	if (always_flush_log)
		flush_log();

	if (log_sem_init) uae_sem_post(&log_sem);
}

void write_dlog(const TCHAR* format, ...)
{
	va_list parms;
	va_start(parms, format);
	TCHAR buffer[WRITE_LOG_BUF_SIZE];
	_vsntprintf(buffer, WRITE_LOG_BUF_SIZE - 1, format, parms);
	write_log("%s", buffer);
	va_end(parms);
}

void write_logx(const TCHAR* format, ...)
{
	va_list parms;
	va_start(parms, format);
	TCHAR buffer[WRITE_LOG_BUF_SIZE];
	_vsntprintf(buffer, WRITE_LOG_BUF_SIZE - 1, format, parms);
	write_log("%s", buffer);
	va_end(parms);
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
