
#include "sysconfig.h"
#include "sysdeps.h"

#include "ini.h"

#ifdef AMIBERRY
#define _tfopen fopen
#define fgetws fgets
#define fputws fputs
#endif

static TCHAR *initrim(TCHAR *s)
{
	while (*s != 0 && *s <= 32)
		s++;
	TCHAR *s2 = s;
	while (*s2)
		s2++;
	while (s2 > s) {
		s2--;
		if (*s2 > 32)
			break;
		*s2 = 0;
	}
	return s;
}

void ini_free(struct ini_data *ini)
{
	if (!ini)
		return;
	for(int c = 0; c < ini->inilines; c++) {
		struct ini_line *il = ini->inidata[c];
		if (il) {
			xfree(il->section);
			xfree(il->key);
			xfree(il->value);
			xfree(il);
		}
		ini->inidata[c] = NULL;
	}
	xfree(ini);
}

static void ini_sort(struct ini_data *ini)
{
	for(int c1 = 0; c1 < ini->inilines; c1++) {
		struct ini_line *il1 = ini->inidata[c1];
		if (il1 == NULL)
			continue;
		for (int c2 = c1 + 1; c2 < ini->inilines; c2++) {
			struct ini_line *il2 = ini->inidata[c2];
			if (il2 == NULL)
				continue;
			int order = 0;
			int sec = _tcsicmp(il1->section, il2->section);
			if (sec) {
				if (!il1->section_order && !il2->section_order)
					order = sec;
				else
					order = il2->section_order - il1->section_order;
			} else {
				order = _tcsicmp(il1->key, il2->key);
			}
			if (order > 0) {
				struct ini_line il;
				memcpy(&il, il1, sizeof(struct ini_line));
				memcpy(il1, il2, sizeof(struct ini_line));
				memcpy(il2, &il, sizeof(struct ini_line));
			}
		}
	}
#if 0
	for(int c1 = 0; c1 < inilines; c1++) {
		struct ini_line *il1 = inidata[c1];
		if (il1)
			write_log(_T("[%s] %s %s\n"), il1->section, il1->key, il1->value);
	}
	write_log(_T("\n"));
#endif
}

void ini_addnewcomment(struct ini_data *ini, const TCHAR *section, const TCHAR *val)
{
	ini_addnewstring(ini, section, _T(""), val);
}

void ini_addnewstring(struct ini_data *ini, const TCHAR *section, const TCHAR *key, const TCHAR *val)
{
	struct ini_line *il = xcalloc(struct ini_line, 1);
	if (!il)
		return;
	il->section = my_strdup(section);
	if (!_tcsicmp(section, _T("Amiberry")))
		il->section_order = 1;
	if (key == NULL) {
		il->key = my_strdup(_T(""));
		il->value = my_strdup(_T(""));
	} else {
		il->key = my_strdup(key);
		il->value = my_strdup(val);
	}
	int cnt = 0;
	while (cnt < ini->inilines && ini->inidata[cnt])
		cnt++;
	if (cnt == ini->inilines) {
		ini->inilines += 10;
		ini->inidata = xrealloc(struct ini_line*, ini->inidata, ini->inilines);
		if (!ini->inidata) {
			xfree(il->key);
			xfree(il->value);
			xfree(il);
			return;
		}
		int cnt2 = cnt;
		while (cnt2 < ini->inilines) {
			ini->inidata[cnt2++] = NULL;
		}
	}
	ini->inidata[cnt] = il;
	ini->modified = true;
}

void ini_addnewval(struct ini_data *ini, const TCHAR *section, const TCHAR *key, uae_u32 v)
{
	TCHAR tmp[MAX_DPATH];
	_sntprintf(tmp, sizeof tmp, _T("%08X ; %u"), v, v);
	ini_addnewstring(ini, section, key, tmp);
}

void ini_addnewval64(struct ini_data *ini, const TCHAR *section, const TCHAR *key, uae_u64 v)
{
	TCHAR tmp[MAX_DPATH];
	_sntprintf(tmp, sizeof tmp, _T("%016llX ; %llu"), v, v);
	ini_addnewstring(ini, section, key, tmp);
}

void ini_addnewdata(struct ini_data *ini, const TCHAR *section, const TCHAR *key, const uae_u8 *data, int len)
{
	TCHAR *s = xcalloc(TCHAR, len * 3);
	if (!s)
		return;
	_tcscpy(s, _T("\\\n"));
	int w = 32;
	for (int i = 0; i < len; i += w) {
		if (i > 0)
			_tcscat(s, _T(" \\\n"));
		TCHAR *p = s + _tcslen(s);
		for (int j = 0; j < w && j + i < len; j++) {
			_sntprintf (p, sizeof p, _T("%02X"), data[i + j]);
			p += 2;
		}
		*p = 0;
	}
	ini_addnewstring(ini, section, key, s);
	xfree(s);
}

static const uae_u8 bom[3] = { 0xef, 0xbb, 0xbf };

struct ini_data *ini_new(void)
{
	struct ini_data *iniout = xcalloc(ini_data, 1);
	return iniout;
}

struct ini_data *ini_load(const TCHAR *path, bool sort)
{
	bool utf8 = false;
	TCHAR section[MAX_DPATH];
	uae_u8 tmp[3];
	struct ini_data ini = { 0 };
	int section_id = 1;

	if (path == NULL || path[0] == 0)
		return NULL;
	FILE *f = _tfopen(path, _T("rb"));
	if (!f)
		return NULL;
	size_t v = fread(tmp, 1, sizeof tmp, f);
	fclose (f);
	if (v == 3 && tmp[0] == 0xef && tmp[1] == 0xbb && tmp[2] == 0xbf) {
		f = _tfopen (path, _T("rt, ccs=UTF-8"));
	} else {
		f = _tfopen (path, _T("rt"));
	}
	section[0] = 0;
	for (;;) {
		TCHAR tbuffer[MAX_DPATH];
		tbuffer[0] = 0;
		if (!fgetws(tbuffer, MAX_DPATH, f))
			break;
		TCHAR *s = initrim(tbuffer);
		if (_tcslen(s) < 3)
			continue;
		if (s[0] == ';')
			continue;
		if (s[0] == '[' && s[_tcslen(s) - 1] == ']') {
			s[_tcslen(s) - 1] = 0;
			_tcscpy(section, s + 1);
			for (int c = 0; c < ini.inilines; c++) {
				struct ini_line *il = ini.inidata[c];
				if (il && !_tcscmp(il->section, section)) {
					section_id++;
					_sntprintf(section + _tcslen(section), sizeof section + _tcslen(section), _T("|%d"), section_id);
					break;
				}
			}
			continue;
		}
		if (section[0] == 0)
			continue;
		TCHAR *s1 = _tcschr(s, '=');
		if (s1) {
			*s1++ = 0;
			TCHAR *s2 = my_strdup(initrim(tbuffer));
			TCHAR *s3 = initrim(s1);
			if (s3[0] == '\\' && s3[1] == 0) {
				// multiline
				xfree(s3);
				s3 = NULL;
				int len = MAX_DPATH;
				TCHAR *otxt = xcalloc(TCHAR, len);
				for (;;) {
					tbuffer[0] = 0;
					if (!fgetws(tbuffer, MAX_DPATH, f))
						break;
					s3 = initrim(tbuffer);
					if (s3[0] == 0)
						break;
					bool nl = s3[_tcslen(s3) - 1] == '\\';
					if (nl) {
						s3[_tcslen(s3) - 1] = 0;
						initrim(s3);
					}
					if (_tcslen(otxt) + _tcslen(s3) + 1 >= len) {
						len += MAX_DPATH;
						otxt = xrealloc(TCHAR, otxt, len);
					}
					_tcscat(otxt, s3);
					s3 = NULL;
					if (!nl)
						break;
				}
				xfree(s3);
				ini_addnewstring(&ini, section, s2, otxt);
			} else {
				ini_addnewstring(&ini, section, s2, s3);
			}
			xfree(s2);
		}
	}
	fclose(f);
	if (sort)
		ini_sort(&ini);
	struct ini_data *iniout = xcalloc(ini_data, 1);
	if (iniout) {
		memcpy(iniout, &ini, sizeof(struct ini_data));
		iniout->modified = false;
	}
	return iniout;
}

bool ini_save(struct ini_data *ini, const TCHAR *path)
{
	TCHAR section[MAX_DPATH];
	TCHAR sep[2] = { '=', 0 };
	TCHAR com[3] = { ';', ' ', 0 };
	TCHAR lf[2] = {  10, 0 };
	TCHAR left[2] = { '[', 0 };
	TCHAR right[2] = { ']', 0 };

	if (!ini)
		return false;
	ini_sort(ini);
	FILE *f = _tfopen(path, _T("wt, ccs=UTF-8"));
	if (!f)
		return false;
	section[0] = 0;
	for (int c = 0; c < ini->inilines; c++) {
		TCHAR out[MAX_DPATH];
		struct ini_line *il = ini->inidata[c];
		if (!il)
			continue;
		if (_tcscmp(il->section, section)) {
			_tcscpy(out, lf);
			_tcscat(out, left);
			_tcscat(out, il->section);
			_tcscat(out, right);
			_tcscat(out, lf);
			fputws(out, f);
			_tcscpy(section, il->section);
		}
		if (il->key[0] != 0 || il->value[0] != 0) {
			if (il->key[0] == 0) {
				fputws(com, f);
			} else {
				fputws(il->key, f);
				fputws(sep, f);
			}
			fputws(il->value, f);
			fputws(lf, f);
		}
	}
	fclose(f);
	ini->modified = false;
	return true;
}

bool ini_nextsection(struct ini_data *ini, TCHAR *section)
{
	if (!ini)
		return false;
	TCHAR nextsect[256];
	_tcscpy(nextsect, section);
	const TCHAR *s = _tcschr(nextsect, '|');
	if (s) {
		int sectionid = _tstol(s + 1);
		_sntprintf(nextsect + (s - nextsect) + 1, sizeof nextsect + (s - nextsect) + 1, _T("%d"), sectionid + 1);
	} else {
		_tcscpy(nextsect + _tcslen(nextsect), _T("|2"));
	}
	for (int c = 0; c < ini->inilines; c++) {
		struct ini_line *il = ini->inidata[c];
		if (il && !_tcsicmp(section, il->section)) {
			for (int c2 = c + 1; c2 < ini->inilines; c2++) {
				il = ini->inidata[c2];
				if (il && !_tcsicmp(nextsect, il->section)) {
					_tcscpy(section, nextsect);
					return true;
				}
			}
			return false;
		}
	}
	return false;
}

bool ini_getstring_multi(struct ini_data *ini, const TCHAR *section, const TCHAR *key, TCHAR **out, struct ini_context *ctx)
{
	if (!ini)
		return false;
	int start = ctx ? ctx->start : 0;
	int end = ctx ? (ini->inilines > ctx->end ? ctx->end : ini->inilines) : ini->inilines;
	for (int c = start; c < end; c++) {
		struct ini_line *il = ini->inidata[c];
		if (il && !_tcsicmp(section, il->section) && (key == NULL || !_tcsicmp(key, il->key))) {
			if (out) {
				*out = my_strdup(il->value);
			}
			if (ctx)
				ctx->lastpos = c;
			return true;
		}
	}
	return false;
}

bool ini_getstring(struct ini_data *ini, const TCHAR *section, const TCHAR *key, TCHAR **out)
{
	return ini_getstring_multi(ini, section, key, out, NULL);
}


bool ini_getval_multi(struct ini_data *ini, const TCHAR *section, const TCHAR *key, int *v, struct ini_context *ctx)
{
	TCHAR *out2 = NULL;
	if (!ini_getstring_multi(ini, section, key, &out2, ctx))
		return false;
	if (out2[0] == 0)
		return false;
	if (_tcslen(out2) > 2 && out2[0] == '0' && _totupper(out2[1]) == 'X') {
		TCHAR *endptr;
		*v = _tcstol(out2 + 2, &endptr, 16);
	} else {
		*v = _tstol(out2);
	}
	xfree(out2);
	return true;
}

bool ini_getval(struct ini_data *ini, const TCHAR *section, const TCHAR *key, int *v)
{
	return ini_getval_multi(ini, section, key, v, NULL);
}

bool ini_getbool(struct ini_data *ini, const TCHAR *section, const TCHAR *key, bool *v)
{
	TCHAR *s = NULL;
	if (!ini_getstring(ini, section, key, &s))
		return false;
	if (!_tcsicmp(s, _T("true")) || !_tcsicmp(s, _T("1"))) {
		xfree(s);
		*v = true;
		return true;
	}
	if (!_tcsicmp(s, _T("false")) || !_tcsicmp(s, _T("0"))) {
		xfree(s);
		*v = false;
		return true;
	}
	return false;
}

bool ini_getdata_multi(struct ini_data *ini, const TCHAR *section, const TCHAR *key, uae_u8 **out, int *size, struct ini_context *ctx)
{
	TCHAR *out2 = NULL;
	uae_u8 *outp = NULL;
	int len;
	bool quoted = false;
	int j = 0;

	if (!ini_getstring_multi(ini, section, key, &out2, ctx))
		return false;

	len = uaetcslen(out2);
	outp = xcalloc(uae_u8, len);
	if (!outp)
		goto err;

	for (int i = 0; i < len; ) {
		TCHAR c1 = _totupper(out2[i + 0]);
		if (c1 == '\"') {
			quoted = !quoted;
			i++;
		} else {
			if (quoted) {
				outp[j++] = (uae_u8)c1;
				i++;
			} else {
				TCHAR c2 = _totupper(out2[i + 1]);
				if (c1 > 0 && c1 <= ' ') {
					i++;
					continue;
				}
				if (i + 1 >= len)
					goto err;
				if (c1 >= 'A')
					c1 -= 'A' - 10;
				else if (c1 >= '0')
					c1 -= '0';
				if (c1 > 15)
					goto err;
				if (c2 >= 'A')
					c2 -= 'A' - 10;
				else if (c2 >= '0')
					c2 -= '0';
				if (c2 > 15)
					goto err;
				outp[j++] = c1 * 16 + c2;
				i += 2;
			}
		}
	}
	if (quoted)
		goto err;
	*out = outp;
	*size = j;
	return true;
err:
	xfree(out2);
	xfree(outp);
	return false;
}

bool ini_getdata(struct ini_data *ini, const TCHAR *section, const TCHAR *key, uae_u8 **out, int *size)
{
	return ini_getdata_multi(ini, section, key, out, size, NULL);
}

bool ini_getsection(struct ini_data *ini, int idx, TCHAR **section)
{
	const TCHAR *sptr = NULL;
	for (int c = 0; c < ini->inilines; c++) {
		struct ini_line *il = ini->inidata[c];
		if (il) {
			if (!sptr) {
				sptr = il->section;
			}
			if (!sptr)
				continue;
			if (_tcsicmp(sptr, il->section)) {
				idx--;
				if (idx < 0) {
					*section = my_strdup(il->section);
					return true;
				}
				sptr = il->section;
			}
		}
	}
	return false;
}

bool ini_getsectionstring(struct ini_data *ini, const TCHAR *section, int idx, TCHAR **keyout, TCHAR **valout)
{
	for (int c = 0; c < ini->inilines; c++) {
		struct ini_line *il = ini->inidata[c];
		if (il && !_tcsicmp(section, il->section)) {
			if (idx == 0) {
				if (keyout) {
					*keyout = my_strdup(il->key);
				}
				if (valout) {
					*valout = my_strdup(il->value);
				}
				return true;
			}
			idx--;
		}
	}
	return false;
}

void ini_setcurrentasstart(struct ini_data *ini, struct ini_context *ctx)
{
	ctx->start = ctx->lastpos;
}

void ini_setnextasstart(struct ini_data *ini, struct ini_context *ctx)
{
	ctx->start = ctx->lastpos + 1;
}

void ini_setlast(struct ini_data *ini, const TCHAR *section, const TCHAR *key, struct ini_context *ctx)
{
	for (int c = ctx->start + 1; c < ini->inilines; c++) {
		struct ini_line *il = ini->inidata[c];
		if (il && !_tcsicmp(section, il->section) && (key == NULL || !_tcsicmp(key, il->key))) {
			ctx->end = c;
			return;
		}
	}
}

void ini_setlastasstart(struct ini_data *ini, struct ini_context *ctx)
{
	ctx->start = ctx->end;
	ctx->end = 0x7fffffff;
}

void ini_initcontext(struct ini_data *ini, struct ini_context *ctx)
{
	memset(ctx, 0, sizeof(struct ini_context));
	ctx->end = 0x7fffffff;
}

bool ini_addstring(struct ini_data *ini, const TCHAR *section, const TCHAR *key, const TCHAR *val)
{
	for (int c = 0; c < ini->inilines; c++) {
		struct ini_line *il = ini->inidata[c];
		if (il && !_tcsicmp(section, il->section)) {
			if (il->key == NULL)
				return true;
			if (!_tcsicmp(key, il->key)) {
				const TCHAR *v = val ? val : _T("");
				if (il->value && !_tcscmp(il->value, v))
					return true;
				xfree(il->value);
				il->value = my_strdup(v);
				ini->modified = true;
				return true;
			}
		}
	}
	ini_addnewstring(ini, section, key, val);
	ini->modified = true;
	return true;
}

bool ini_delete(struct ini_data *ini, const TCHAR *section, const TCHAR *key)
{
	bool deleted = false;
	for (int c = 0; c < ini->inilines; c++) {
		struct ini_line *il = ini->inidata[c];
		if (il && !_tcsicmp(section, il->section) && (key == NULL || !_tcsicmp(key, il->key))) {
			xfree(il->section);
			xfree(il->key);
			xfree(il->value);
			xfree(il);
			ini->inidata[c] = NULL;
			ini->modified = true;
			deleted = true;
		}
	}
	return deleted;
}
