#ifndef UAE_STRING_H
#define UAE_STRING_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <SDL.h>

#include "uae/types.h"
#include <cstring>

#ifdef _WIN32
/* Make sure the real _tcs* functions are already declared before we
 * re-define them below. */
#include <tchar.h>
#include <wchar.h>
#include <stdlib.h>
#endif

#ifdef _WIN32
/* Using the real _tcs* functions */
#else
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
#define uaestrlen SDL_strlen
#define uaetcslen SDL_strlen
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
