#include "sysconfig.h"
#include "sysdeps.h"

#include <string.h>

// UAE4ARM and fs-uae uses only chars / UTF-8 internally, so TCHAR is typedefed to
// char (WinUAE uses wchar_t internally).

char *ua (const TCHAR *s) 
{
	if (s == NULL) 
	  return NULL;
	return strdup (s);
}

char *au (const TCHAR *s) 
{
	if (s == NULL) 
	  return NULL;
	return strdup (s);
}

TCHAR* utf8u (const char *s)
{
	if (s == NULL) 
	  return NULL;
	return ua (s);
}

char* uutf8 (const TCHAR *s)
{
	if (s == NULL) 
	  return NULL;
	return ua (s);
}

TCHAR *au_copy (TCHAR *dst, int maxlen, const char *src)
{
	// this should match the WinUAE au_copy behavior, where either the
	// entire string is copied (and null-terminated), or the result is
	// an empty string
	if (uae_tcslcpy (dst, src, maxlen) >= maxlen) {
		dst[0] = '\0';
	}
	return dst;
}

char *ua_copy (char *dst, int maxlen, const TCHAR *src)
{
	return au_copy (dst, maxlen, src);
}

TCHAR *my_strdup_ansi (const char *src)
{
	return strdup (src);
}

TCHAR *au_fs (const char *src) 
{
  if (src == NULL) 
    return NULL;
  return strdup(src);
}

char *ua_fs (const TCHAR *s, int defchar) 
{
  if (s == NULL) 
    return NULL;
  return strdup(s);
}

TCHAR *au_fs_copy (TCHAR *dst, int maxlen, const char *src) 
{
  dst[0] = 0;
  strncpy(dst, src, maxlen);
  return dst;
}

char *ua_fs_copy (char *dst, int maxlen, const TCHAR *src, int defchar) 
{
  dst[0] = 0;
  strncpy(dst, src, maxlen);
  return dst;
}

void to_lower (TCHAR *s, int len)
{
  for (int i = 0; i < len; i++) {
    s[i] = tolower(s[i]);
  }
}

void to_upper (TCHAR *s, int len)
{
  for (int i = 0; i < len; i++) {
    s[i] = toupper(s[i]);
  }
}
