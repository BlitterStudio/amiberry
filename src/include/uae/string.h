#ifndef UAE_STRING_H
#define UAE_STRING_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <SDL.h>

#include "uae/types.h"
#include <cstring>

#if defined(_WIN32) && !defined(AMIBERRY)
/* WinUAE: use the real _tcs* wide/narrow functions from tchar.h */
#include <tchar.h>
#include <wchar.h>
#include <stdlib.h>
#elif defined(_WIN32) && defined(AMIBERRY)
/* Amiberry on Windows: tchar.h maps _tcs* to narrow-char functions (strlen, etc.)
   since UNICODE is not defined. We include tchar.h for those definitions and only
   add the extras that tchar.h doesn't provide. */
#include <tchar.h>
#include <wchar.h>
#include <stdlib.h>
/* These aren't in tchar.h — provide our own */
#ifndef _istxdigit
#define _istxdigit isxdigit
#endif
#ifndef _istdigit
#define _istdigit SDL_isdigit
#endif
#ifndef _istspace
#define _istspace SDL_isspace
#endif
#ifndef _istupper
#define _istupper isupper
#endif
#ifndef _sntprintf
#define _sntprintf SDL_snprintf
#endif
#ifndef _stprintf
#define _stprintf sprintf
#endif
#ifndef _strtoui64
#define _strtoui64 SDL_strtoll
#endif
#ifndef _tcsicmp
#define _tcsicmp SDL_strcasecmp
#endif
#ifndef _tcsnicmp
#define _tcsnicmp SDL_strncasecmp
#endif
#ifndef _totlower
#define _totlower SDL_tolower
#endif
#ifndef _totupper
#define _totupper SDL_toupper
#endif
#ifndef _tprintf
#define _tprintf printf
#endif
#ifndef _tstof
#define _tstof SDL_atof
#endif
#ifndef _tstoi64
#define _tstoi64 atoll
#endif
#ifndef _tstoi
#define _tstoi SDL_atoi
#endif
#ifndef _tstol
#define _tstol atol
#endif
#ifndef _vsntprintf
#define _vsntprintf SDL_vsnprintf
#endif
#else
/* Non-Windows (Linux, macOS, etc.): TCHAR is char, define all _tcs* macros */
#define _istxdigit isxdigit
#define _istdigit SDL_isdigit
#define _istspace SDL_isspace
#define _istupper isupper
#define _sntprintf SDL_snprintf
#define _stprintf sprintf
#define _strtoui64 SDL_strtoll
#define _tcscat strcat
#define _tcschr SDL_strchr
#define _tcscmp SDL_strcmp
#define _tcscpy strcpy
#define _tcscspn strcspn
#define _tcsdup SDL_strdup
#define _tcsftime strftime
#define _tcsicmp SDL_strcasecmp
#define _tcslen SDL_strlen
#define _tcsncat strncat
#define _tcsncmp SDL_strncmp
#define _tcsncpy strncpy
#define _tcsnicmp SDL_strncasecmp
#define _tcsrchr SDL_strrchr
#define _tcsspn strspn
#define _tcsstr SDL_strstr
#define _tcstod SDL_strtod
#define _tcstok strtok
#define _tcstol SDL_strtol
#define _tcstoul SDL_strtoul
#define _totlower SDL_tolower
#define _totupper SDL_toupper
#define _tprintf printf
#define _tstof SDL_atof
#define _tstoi64 atoll
#define _tstoi SDL_atoi
#define _tstol atol
#define _vsnprintf SDL_vsnprintf
#define _vsntprintf SDL_vsnprintf
#endif

/* Amiberry-specific: always define these regardless of platform.
   On Windows, _tcslen is already defined by tchar.h (as strlen).
   These are used in Amiberry code but not defined by tchar.h or WinUAE. */
#ifndef uaestrlen
#define uaestrlen SDL_strlen
#endif
#ifndef uaetcslen
#define uaetcslen SDL_strlen
#endif

static size_t uae_tcslcpy(char *dst, const TCHAR *src, size_t size)
{
	if (size == 0) {
		return 0;
	}
	const auto src_len = _tcslen(src);
	auto cpy_len = src_len;
	if (cpy_len >= size) {
		cpy_len = size - 1;
	}
	memcpy(dst, src, cpy_len * sizeof(TCHAR));
	dst[cpy_len] = _T('\0');
	return src_len;
}

static size_t uae_strlcpy(char *dst, const char *src, size_t size)
{
	if (size == 0) {
		return 0;
	}
	const auto src_len = strlen(src);
	auto cpy_len = src_len;
	if (cpy_len >= size) {
		cpy_len = size - 1;
	}
	memcpy(dst, src, cpy_len);
	dst[cpy_len] = '\0';
	return src_len;
}

#endif /* UAE_STRING_H */
