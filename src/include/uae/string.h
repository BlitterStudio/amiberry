#ifndef UAE_STRING_H
#define UAE_STRING_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "uae/types.h"
#include <string.h>

#ifdef _WIN32_
/* Make sure the real _tcs* functions are already declared before we
 * re-define them below. */
#include <tchar.h>
#include <wchar.h>
#include <stdlib.h>
#endif

#ifdef _WIN32_
/* Using the real _tcs* functions */
#else
#define _istdigit isdigit
#define _istspace isspace
#define _istupper isupper
#define _sntprintf snprintf
#define _stprintf sprintf
#define _strtoui64 strtoll
#define _tcscat strcat
#define _tcschr strchr
#define _tcscmp strcmp
#define _tcscpy strcpy
#define _tcscspn strcspn
#define _tcsdup strdup
#define _tcsftime strftime
#define _tcsftime strftime
#define _tcsicmp strcasecmp
#define _tcslen strlen
#define _tcsncat strncat
#define _tcsncmp strncmp
#define _tcsncpy strncpy
#define _tcsnicmp strncasecmp
#define _tcsrchr strrchr
#define _tcsspn strspn
#define _tcsstr strstr
#define _tcstod strtod
#define _tcstok strtok
#define _tcstol strtol
#define _totlower tolower
#define _totupper toupper
#define _tprintf printf
#define _tstof atof
#define _tstoi64 atoll
#define _tstoi atoi
#define _tstol atol
#define _vsnprintf vsnprintf
#define _vsntprintf vsnprintf
#endif

static inline size_t uae_tcslcpy(TCHAR *dst, const TCHAR *src, size_t size)
{
	if (size == 0) {
		return 0;
	}
	size_t src_len = _tcslen(src);
	size_t cpy_len = src_len;
	if (cpy_len >= size) {
		cpy_len = size - 1;
	}
	memcpy(dst, src, cpy_len * sizeof(TCHAR));
	dst[cpy_len] = _T('\0');
	return src_len;
}

static inline size_t uae_strlcpy(char *dst, const char *src, size_t size)
{
	if (size == 0) {
		return 0;
	}
	size_t src_len = strlen(src);
	size_t cpy_len = src_len;
	if (cpy_len >= size) {
		cpy_len = size - 1;
	}
	memcpy(dst, src, cpy_len);
	dst[cpy_len] = '\0';
	return src_len;
}

#endif /* UAE_STRING_H */
