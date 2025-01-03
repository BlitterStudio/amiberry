#include "sysconfig.h"
#include "sysdeps.h"

#include <stdlib.h>
#include <stdarg.h>

#ifdef AMIBERRY
#else
#include <windows.h>
#endif

#include "options.h"
#include "traps.h"
#ifdef AMIBERRY
#ifndef _WIN32
typedef int HDC;
typedef int HGLOBAL;
typedef unsigned int UINT;
#endif
#endif
#include "clipboard.h"
#include "keybuf.h"
#include "memory.h"
#include "autoconf.h"

#include "threaddep/thread.h"
#include "memory.h"
#include "native2amiga_api.h"

#ifdef AMIBERRY

#include "uae/uae.h"

/** clipboard_read is called from both the main thread and uae thread
 * (mousehack_done), so clipboard_from_host_text/changed and is protected with
 * a mutex. */
static SDL_mutex* clipboard_from_host_mutex;
static char* clipboard_from_host_text;
static bool clipboard_from_host_changed;

static SDL_mutex* clipboard_to_host_mutex;
static char* clipboard_to_host_text;

#endif // AMIBERRY

#define DEBUG_CLIP 0

int clipboard_debug;

static int chwnd;
static HDC hdc;
static uaecptr clipboard_data;
static int vdelay, vdelay2;
static int signaling, initialized;
static uae_u8* to_amiga;
static uae_u32 to_amiga_size;
static int to_amiga_phase;
static int clipopen;
static int clipactive;
static int clipboard_change;
static void* clipboard_delayed_data;
static int clipboard_delayed_size;
static bool clip_disabled;

static void debugwrite(TrapContext* ctx, const TCHAR* name, uaecptr p, int size)
{
#ifdef AMIBERRY

#else
	FILE *f;
	int cnt;

	if (!p || !size)
		return;
	cnt = 0;
	for (;;) {
		TCHAR tmp[MAX_DPATH];
		_stprintf (tmp, _T("%s.%03d.dat"), name, cnt);
		f = _tfopen (tmp, _T("rb"));
		if (f) {
			fclose (f);
			cnt++;
			continue;
		}
		f = _tfopen (tmp, _T("wb"));
		if (f) {
			uae_u8 *pd = xmalloc(uae_u8, size);
			trap_get_bytes(ctx, pd, p, size);
			fwrite (pd, size, 1, f);
			fclose (f);
			xfree(pd);
		}
		return;
	}
#endif
}

static uae_u32 to_amiga_start_cb(TrapContext* ctx, void* ud)
{
	if (trap_get_long(ctx, clipboard_data) != 0)
		return 0;
	if (clipboard_debug)
	{
		debugwrite(ctx, _T("clipboard_p2a"), clipboard_data, to_amiga_size);
	}
#if DEBUG_CLIP > 0
	write_log(_T("clipboard: to_amiga %08x %d\n"), clipboard_data, to_amiga_size);
#endif
	trap_put_long(ctx, clipboard_data, to_amiga_size);
	uae_Signal(trap_get_long(ctx, clipboard_data + 8), 1 << 13);
	to_amiga_phase = 2;
	return 1;
}

static void to_amiga_start(TrapContext* ctx)
{
#ifdef AMIBERRY
	write_log("clipboard: to_amiga_start initialized=%d clipboard_data=%d\n",
	          initialized, clipboard_data);
#endif
	to_amiga_phase = 0;
	if (!initialized)
		return;
	if (!clipboard_data)
		return;
	to_amiga_phase = 1;
}

static uae_char* pctoamiga(const uae_char* txt)
{
	const auto len = strlen(txt) + 1;
	auto* txt2 = xmalloc(uae_char, len);
	auto j = 0;
	for (unsigned int i = 0; i < len; i++)
	{
		const auto c = txt[i];
#ifdef _WIN32
		if (c == 13)
			continue;
#endif
		txt2[j++] = c;
	}
	return txt2;
}

static int parsecsi(const char* txt, int off, int len)
{
	while (off < len)
	{
		if (txt[off] >= 0x40)
			break;
		off++;
	}
	return off;
}

static void to_keyboard(const TCHAR* pctxt)
{
	uae_char* txt = ua(pctxt);
	keybuf_inject(txt);
	xfree(txt);
}

static TCHAR* amigatopc(const char* txt)
{
	auto pc = 0;
	auto cnt = 0;
	const auto len = strlen(txt) + 1;
	for (unsigned int i = 0; i < len; i++)
	{
		const auto c = txt[i];
		if (c == 13)
			pc = 1;
		if (c == 10)
			cnt++;
	}
	if (pc)
		return my_strdup_ansi(txt);
	auto* const txt2 = xcalloc(char, len + cnt);
	auto j = 0;
	for (unsigned int i = 0; i < len; i++)
	{
		const auto c = txt[i];
		if (c == 0 && i + 1 < len)
			continue;
#ifdef _WIN32
		if (c == 10)
			txt2[j++] = 13;
#endif
		if (c == 0x9b)
		{
			i = parsecsi(txt, static_cast<int>(i) + 1, static_cast<int>(len));
			continue;
		}
		if (c == 0x1b && i + 1 < len && txt[i + 1] == '[')
		{
			i = parsecsi(txt, static_cast<int>(i) + 2, static_cast<int>(len));
			continue;
		}
		txt2[j++] = c;
	}
	auto* const s = my_strdup_ansi(txt2);
	xfree(txt2);
	return s;
}

static void to_iff_text(TrapContext* ctx, const TCHAR* pctxt)
{
	uae_u8 b[] = {'F', 'O', 'R', 'M', 0, 0, 0, 0, 'F', 'T', 'X', 'T', 'C', 'H', 'R', 'S', 0, 0, 0, 0};

	auto s = ua(pctxt);
	uae_char* txt = pctoamiga(s);
	int txtlen = strlen(txt);
	xfree(to_amiga);
	uae_u32 size = txtlen + sizeof b + (txtlen & 1) - 8;
	b[4] = size >> 24;
	b[5] = size >> 16;
	b[6] = size >> 8;
	b[7] = size >> 0;
	size = txtlen;
	b[16] = size >> 24;
	b[17] = size >> 16;
	b[18] = size >> 8;
	b[19] = size >> 0;
	to_amiga_size = sizeof b + txtlen + (txtlen & 1);
	to_amiga = xcalloc(uae_u8, to_amiga_size);
	memcpy(to_amiga, b, sizeof b);
	memcpy(to_amiga + sizeof b, txt, txtlen);
	to_amiga_start(ctx);
	xfree(txt);
	xfree(s);
}

static int clipboard_put_text(const TCHAR* txt);

static void from_iff_text(uae_u8* addr, uae_u32 len)
{
	char* txt = nullptr;
	auto txtsize = 0;

	auto* const eaddr = addr + len;
	if (memcmp("FTXT", addr + 8, 4) != 0)
		return;
	addr += 12;
	while (addr < eaddr)
	{
		const uae_u32 csize = (addr[4] << 24) | (addr[5] << 16) | (addr[6] << 8) | (addr[7] << 0);
		if (addr + 8 + csize > eaddr)
			break;
		if (!memcmp(addr, "CHRS", 4) && csize)
		{
			const auto prevsize = txtsize;
			txtsize += csize;
			char* new_txt = xrealloc(char, txt, txtsize + 1);
			if (!new_txt)
			{
				xfree(txt);
				return;
			}
			txt = new_txt;
			memcpy(txt + prevsize, addr + 8, csize);
			txt[txtsize] = 0;
		}
		addr += 8 + csize + (csize & 1);
		if (csize >= 1 && addr[-2] == 0x0d && addr[-1] == 0x0a && addr[0] == 0)
			addr++;
		else if (csize >= 1 && addr[-1] == 0x0d && addr[0] == 0x0a)
			addr++;
	}
	if (txt == nullptr)
	{
		clipboard_put_text(_T(""));
	}
	else
	{
		auto* const pctxt = amigatopc(txt);
		clipboard_put_text(pctxt);
		xfree(pctxt);
	}
	xfree(txt);
}

#ifdef AMIBERRY // NL
#else

static void to_iff_ilbm(TrapContext *ctx, HBITMAP hbmp)
{
	BITMAP bmp;
	int bmpw, w, h, bpp, iffbpp, tsize, size, x, y, i;
	int iffsize, bodysize;
	uae_u32 colors[256] = { 0 };
	int cnt;
	uae_u8 *iff, *p;
	uae_u8 iffilbm[] = {
		'F','O','R','M',0,0,0,0,'I','L','B','M',
		'B','M','H','D',0,0,0,20, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		'C','A','M','G',0,0,0, 4,  0,0,0,0,
	};

	if (!GetObject (hbmp, sizeof bmp, &bmp))
		return;
	w = bmp.bmWidth;
	h = bmp.bmHeight;
	bpp = bmp.bmBitsPixel;

	if (bpp != 8 && bpp != 32)
		return;
	bmpw = (w * bpp / 8 + 3) & ~3;
	size = bmpw * h;
	bmp.bmBits = xmalloc (uae_u8, size);
	if (!GetBitmapBits (hbmp, size, bmp.bmBits)) {
		xfree (bmp.bmBits);
		return;
	}
#if DEBUG_CLIP > 0
	write_log (_T("BMP2IFF: W=%d H=%d bpp=%d\n"), w, h, bpp);
#endif
	iffbpp = bpp > 8 ? 24 : bpp;
	cnt = 0;
	for (y = 0; y < h && cnt < 256; y++) {
		uae_u32 *s = (uae_u32*)(((uae_u8*)bmp.bmBits) + y * bmpw);
		for (x = 0; x < w && cnt < 256; x++) {
			uae_u32 v = s[x];
			for (i = 0; i < cnt; i++) {
				if (colors[i] == v)
					break;
			}
			if (i == 256)
				break;
			if (i == cnt)
				colors[cnt++] = v;
		}
	}
	if (cnt < 256) {
		i = cnt;
		iffbpp = 0;
		while (i > 0) {
			i >>= 1;
			iffbpp++;
		}
#if DEBUG_CLIP > 0
		write_log (_T("BMP2IFF: Colors=%d BPP=%d\n"), cnt, iffbpp);
#endif
	}

	bodysize = (((w + 15) & ~15) / 8) * h * iffbpp;

	iffsize = sizeof (iffilbm) + (8 + 256 * 3 + 1) + (4 + 4) + bodysize;
	iff = xcalloc (uae_u8, iffsize);
	memcpy (iff, iffilbm, sizeof iffilbm);
	p = iff + 5 * 4;
	// BMHD
	p[0] = w >> 8;
	p[1] = w;
	p[2] = h >> 8;
	p[3] = h;
	p[8] = iffbpp;
	p[14] = 1;
	p[15] = 1;
	p[16] = w >> 8;
	p[17] = w;
	p[18] = h >> 8;
	p[19] = h;
	p = iff + sizeof iffilbm - 4;
	// CAMG
	if (w > 400)
		p[2] |= 0x80; // HIRES
	if (h > 300)
		p[3] |= 0x04; // LACE
	p += 4;
	if (iffbpp <= 8) {
		int cols = 1 << iffbpp;
		int cnt = 0;
		memcpy (p, "CMAP", 4);
		p[4] = 0;
		p[5] = 0;
		p[6] = (cols * 3) >> 8;
		p[7] = (cols * 3);
		p += 8;
		for (i = 0; i < cols; i++) {
			*p++ = colors[i] >> 16;
			*p++ = colors[i] >> 8;
			*p++ = colors[i] >> 0;
			cnt += 3;
		}
		if (cnt & 1)
			*p++ = 0;
	}
	memcpy (p, "BODY", 4);
	p[4] = bodysize >> 24;
	p[5] = bodysize >> 16;
	p[6] = bodysize >>  8;
	p[7] = bodysize >>  0;
	p += 8;

	if (bpp > 8 && iffbpp <= 8) {
		for (y = 0; y < h && i < 256; y++) {
			uae_u32 *s = (uae_u32*)(((uae_u8*)bmp.bmBits) + y * bmpw);
			int b;
			for (b = 0; b < iffbpp; b++) {
				int mask2 = 1 << b;
				for (x = 0; x < w; x++) {
					uae_u32 v = s[x];
					int off = x / 8;
					int mask = 1 << (7 - (x & 7));
					for (i = 0; i < (1 << iffbpp); i++) {
						if (colors[i] == v)
							break;
					}
					if (i & mask2)
						p[off] |= mask;
				}
				p += ((w + 15) & ~15) / 8;
			}
		}
	} else if (bpp <= 8) {
		for (y = 0; y < h; y++) {
			uae_u8 *s = (uae_u8*)(((uae_u8*)bmp.bmBits) + y * bmpw);
			int b;
			for (b = 0; b < 8; b++) {
				int mask2 = 1 << b;
				for (x = 0; x < w; x++) {
					int off = x / 8;
					int mask = 1 << (7 - (x & 7));
					uae_u8 v = s[x];
					if (v & mask2)
						p[off] |= mask;
				}
				p += ((w + 15) & ~15) / 8;
			}
		}
	} else if (bpp == 32) {
		for (y = 0; y < h; y++) {
			uae_u32 *s = (uae_u32*)(((uae_u8*)bmp.bmBits) + y * bmpw);
			int b, bb;
			for (bb = 0; bb < 3; bb++) {
				for (b = 0; b < 8; b++) {
					int mask2 = 1 << (((2 - bb) * 8) + b);
					for (x = 0; x < w; x++) {
						int off = x / 8;
						int mask = 1 << (7 - (x & 7));
						uae_u32 v = s[x];
						if (v & mask2)
							p[off] |= mask;
					}
					p += ((w + 15) & ~15) / 8;
				}
			}
		}
	}

	tsize = p - iff - 8;
	p = iff + 4;
	p[0] = tsize >> 24;
	p[1] = tsize >> 16;
	p[2] = tsize >>  8;
	p[3] = tsize >>  0;

	to_amiga_size = 8 + tsize + (tsize & 1);
	to_amiga = iff;

	to_amiga_start (ctx);

	xfree (bmp.bmBits);
}

static uae_u8 *iff_decomp(const uae_u8 *addr, const uae_u8 *eaddr, int w, int h, int planes)
{
	int y, i, w2;
	uae_u8 *dst;

	w2 = (w + 15) & ~15;
	dst = xcalloc (uae_u8, w2 * h * planes);
	for (y = 0; y < h * planes; y++) {
		uae_u8 *p = dst + w2 * y;
		uae_u8 *end = p + w2;
		while (p < end) {
			if (addr >= eaddr)
				return dst;
			uae_s8 c = *addr++;
			if (c >= 0 && c <= 127) {
				uae_u8 cnt = c + 1;
				if (addr + cnt > eaddr)
					return dst;
				for (i = 0; i < cnt && p < end; i++) {
					*p++= *addr++;
				}
			} else if (c <= -1 && c >= -127) {
				uae_u8 cnt = -c + 1;
				if (addr >= eaddr)
					return dst;
				uae_u8 v = *addr++;
				for (i = 0; i < cnt && p < end; i++) {
					*p++= v;
				}
			}
		}
	}
	return dst;
}

static int clipboard_put_bmp (HBITMAP hbmp);
static void from_iff_ilbm(uae_u8 *saddr, uae_u32 len)
{
	HBITMAP hbm = NULL;
	BITMAPINFO *bmih;
	uae_u32 size, bmsize, camg;
	uae_u8 *addr, *eaddr, *bmptr;
	int i;
	int w, bmpw, iffw, h, planes, compr, masking;
	int bmhd, body;
	RGBQUAD rgbx[256];

	if (len < 12)
		return;
	bmih = NULL;
	bmhd = 0, body = 0;
	bmsize = 0;
	bmptr = NULL;
	planes = 0; compr = 0;
	addr = saddr;
	eaddr = addr + len;
	size = (addr[4] << 24) | (addr[5] << 16) | (addr[6] << 8) | (addr[7] << 0);
	if (size > 0xffffff)
		return;
	if (memcmp ("ILBM", addr + 8, 4))
		return;
	camg = 0;
	for (i = 0; i < 256; i++) {
		rgbx[i].rgbRed = i;
		rgbx[i].rgbGreen = i;
		rgbx[i].rgbBlue = i;
		rgbx[i].rgbReserved = 0;
	}

	addr += 12;
	for (;;) {
		int csize;
		uae_u8 chunk[4];
		uae_u8 *paddr, *ceaddr;

		paddr = addr;
		if (paddr + 8 > eaddr)
			return;
		memcpy (chunk, addr, 4);
		csize = (addr[4] << 24) | (addr[5] << 16) | (addr[6] << 8) | (addr[7] << 0);
		addr += 8;
		ceaddr = addr + csize;
		// chunk end larger than end of data?
		if (ceaddr > eaddr)
			return;
		if (!memcmp (chunk, "BMHD" ,4)) {
			bmhd = 1;
			w = (addr[0] << 8) | addr[1];
			h = (addr[2] << 8) | addr[3];
			planes = addr[8];
			masking = addr[9];
			compr = addr[10];

		} else if (!memcmp (chunk, "CAMG" ,4)) {
			camg = (addr[0] << 24) | (addr[1] << 16) | (addr[2] << 8) | (addr[3] << 0);
			if ((camg & 0xFFFF0000) && !(camg & 0x1000))
				camg = 0;
		} else if (!memcmp (chunk, "CMAP" ,4)) {
			if (planes <= 8) {
				int zero4 = 1;
				for (i = 0; i < (1 << planes) && addr < ceaddr; i++, addr += 3) {
					rgbx[i].rgbRed = addr[0];
					rgbx[i].rgbGreen = addr[1];
					rgbx[i].rgbBlue = addr[2];
					if ((addr[0] & 0x0f) || (addr[1] & 0x0f) || (addr[2] & 0x0f))
						zero4 = 0;
				}
				if (zero4) {
					for (i = 0; i < (1 << planes); i++) {
						rgbx[i].rgbRed |= rgbx[i].rgbRed >> 4;
						rgbx[i].rgbGreen |= rgbx[i].rgbGreen >> 4;
						rgbx[i].rgbBlue |= rgbx[i].rgbBlue >> 4;
					}
				}
			}
		} else if (!memcmp (chunk, "BODY" ,4) && bmhd) {
			int x, y;
			int ham, ehb, bmpdepth;
			uae_u8 *caddr = NULL, *dptr;
			body = 1;

#if DEBUG_CLIP > 0
			write_log (_T("W=%d H=%d planes=%d mask=%d comp=%d CAMG=%08x\n"), w, h, planes, masking, compr, camg);
#endif

			ham = 0; ehb = 0;
			if ((camg & 0x0800) && planes > 4)
				ham = planes >= 7 ? 8 : 6;
			if (!(camg & (0x0800 | 0x0400 | 0x8000 | 0x0040)) && (camg & 0x0080) && planes == 6)
				ehb = 1;

			if (planes <= 8 && !ham)
				bmpdepth = 8;
			else
				bmpdepth = 32;
			iffw = (w + 15) & ~15;
			bmpw = (w * (bmpdepth / 8) + 3) & ~3;

			bmsize = sizeof (BITMAPINFO);
			if (bmpdepth <= 8)
				bmsize += (1 << planes) * sizeof (RGBQUAD);
			bmih = (BITMAPINFO*)xcalloc (uae_u8, bmsize);
			bmih->bmiHeader.biSize = sizeof (bmih->bmiHeader);
			bmih->bmiHeader.biWidth = w;
			bmih->bmiHeader.biHeight = -h;
			bmih->bmiHeader.biPlanes = 1;
			bmih->bmiHeader.biBitCount = bmpdepth;
			bmih->bmiHeader.biCompression = BI_RGB;
			if (bmpdepth <= 8) {
				RGBQUAD *rgb = bmih->bmiColors;
				bmih->bmiHeader.biClrImportant = 0;
				bmih->bmiHeader.biClrUsed = 1 << (planes + ehb);
				for (i = 0; i < (1 << planes); i++) {
					rgb->rgbRed = rgbx[i].rgbRed;
					rgb->rgbGreen = rgbx[i].rgbGreen;
					rgb->rgbBlue = rgbx[i].rgbBlue;
					rgb++;
				}
				if (ehb) {
					for (i = 0; i < (1 << planes); i++) {
						rgb->rgbRed = rgbx[i].rgbRed >> 1;
						rgb->rgbGreen = rgbx[i].rgbGreen >> 1;
						rgb->rgbBlue = rgbx[i].rgbBlue >> 1;
						rgb++;
					}
				}
			}
			bmptr = xcalloc (uae_u8, bmpw * h);

			if (compr)
				addr = caddr = iff_decomp(addr, addr + csize, w, h, planes + (masking == 1 ? 1 : 0));
			dptr = bmptr;

			if (planes <= 8 && !ham) {
				// paletted
				for (y = 0; y < h; y++) {
					for (x = 0; x < w; x++) {
						int b;
						int off = x / 8;
						int mask = 1 << (7 - (x & 7));
						uae_u8 c = 0;
						for (b = 0; b < planes; b++)
							c |= (addr[b * iffw / 8 + off] & mask) ? (1 << b) : 0;
						dptr[x] = c;
					}
					dptr += bmpw;
					addr += planes * iffw / 8;
					if (masking == 1)
						addr += iffw / 8;
				}
			} else if (ham) {
				// HAM6/8 -> 32
				for (y = 0; y < h; y++) {
					DWORD ham_lastcolor = 0;
					for (x = 0; x < w; x++) {
						int b;
						int off = x / 8;
						int mask = 1 << (7 - (x & 7));
						uae_u8 c = 0;
						for (b = 0; b < planes; b++)
							c |= (addr[b * iffw / 8 + off] & mask) ? (1 << b) : 0;
						if (ham > 6) {
							DWORD c2 = c & 0x3f;
							switch (c >> 6)
							{
							case 0: ham_lastcolor = *((DWORD*)(&rgbx[c])); break;
							case 1: ham_lastcolor &= 0x00FFFF03; ham_lastcolor |= c2 << 2; break;
							case 2: ham_lastcolor &= 0x0003FFFF; ham_lastcolor |= c2 << 18; break;
							case 3: ham_lastcolor &= 0x00FF03FF; ham_lastcolor |= c2 << 10; break;
							}
						} else {
							uae_u32 c2 = c & 0x0f;
							switch (c >> 4)
							{
							case 0: ham_lastcolor = *((DWORD*)(&rgbx[c])); break;
							case 1: ham_lastcolor &= 0xFFFF00; ham_lastcolor |= c2 << 4; break;
							case 2: ham_lastcolor &= 0x00FFFF; ham_lastcolor |= c2 << 20; break;
							case 3: ham_lastcolor &= 0xFF00FF; ham_lastcolor |= c2 << 12; break;
							}
						}
						*((DWORD*)(&dptr[x * 4])) = ham_lastcolor;
					}
					dptr += bmpw;
					addr += planes * iffw / 8;
					if (masking == 1)
						addr += iffw / 8;
				}
			} else {
				// 24bit RGB
				for (y = 0; y < h; y++) {
					for (x = 0; x < w; x++) {
						uae_u8 c;
						int b;
						int off = x / 8;
						int mask = 1 << (7 - (x & 7));
						c = 0;
						for (b = 0; b < planes / 3; b++)
							c |= (addr[((0 * planes / 3) + b) * iffw / 8 + off] & mask) ? (1 << b) : 0;
						dptr[x * 4 + 2] = c;
						c = 0;
						for (b = 0; b < planes / 3; b++)
							c |= (addr[((1 * planes / 3) + b) * iffw / 8 + off] & mask) ? (1 << b) : 0;
						dptr[x * 4 + 1] = c;
						c = 0;
						for (b = 0; b < planes / 3; b++)
							c |= (addr[((2 * planes / 3) + b) * iffw / 8 + off] & mask) ? (1 << b) : 0;
						dptr[x * 4 + 0] = c;
					}
					dptr += bmpw;
					addr += planes * iffw / 8;
					if (masking == 1)
						addr += iffw / 8;
				}
			}
			if (caddr)
				xfree (caddr);
		}
		addr = paddr + csize + (csize & 1) + 8;
		if (addr >= eaddr)
			break;
	}
	if (body) {
		hbm = CreateDIBitmap (hdc, &bmih->bmiHeader, CBM_INIT, bmptr, bmih, DIB_RGB_COLORS);
		if (hbm)
			clipboard_put_bmp (hbm);
	}
	xfree (bmih);
	xfree (bmptr);
}
#endif

static void from_iff(TrapContext* ctx, uaecptr data, uae_u32 len)
{
	if (len < 18)
		return;
	if (!trap_valid_address(ctx, data, len))
		return;
	auto* const buf = xmalloc(uae_u8, (len + 3) & ~3);
	trap_get_bytes(ctx, buf, data, int(len + 3) & ~3);

	if (clipboard_debug)
		debugwrite(ctx, _T("clipboard_a2p"), data, static_cast<int>(len));
	if (!memcmp("FORM", buf, 4))
	{
		if (!memcmp("FTXT", buf + 8, 4))
			from_iff_text(buf, len);
#ifdef AMIBERRY
		/* Bitmaps are not supported in FS-UAE yet */
#else
		if (!memcmp ("ILBM", buf + 8, 4))
			from_iff_ilbm(buf, len);
#endif
	}
	xfree(buf);
}

void clipboard_disable(bool disabled)
{
	clip_disabled = disabled;
}

static void clipboard_read(TrapContext* ctx, int hwnd, bool keyboardinject)
{
	HGLOBAL hglb;
	UINT f;
	int text = FALSE, bmp = FALSE;

#ifdef AMIBERRY
	write_log("clipboard clip_disabled=%d hwnd=%d to_amiga=%d "
	          "heartbeat=%d\n",
	          clip_disabled, hwnd, to_amiga != nullptr, filesys_heartbeat());
#endif
	if (clip_disabled || !hwnd)
		return;
	if (to_amiga)
	{
#if DEBUG_CLIP > 0
		write_log (_T("clipboard: read windows clipboard but ignored because previous clip transfer still active\n"));
#endif
		return;
	}
	clipboard_change = 0;
#if DEBUG_CLIP > 0
	write_log (_T("clipboard: read windows clipboard\n"));
#endif
	if (!filesys_heartbeat())
		return;
#ifdef AMIBERRY
#else
	if (!OpenClipboard (hwnd))
		return;
#endif
#ifdef AMIBERRY
	char* lptstr = nullptr;
	SDL_mutexP(clipboard_from_host_mutex);
	if (clipboard_from_host_changed)
	{
		if (clipboard_from_host_text)
		{
			lptstr = strdup(clipboard_from_host_text);
		}
		else
		{
			lptstr = nullptr;
		}
		// lptstr = strdup(
		//	clipboard_from_host_text ? clipboard_from_host_text : "");
		text = true;
		clipboard_from_host_changed = false;
	}
	SDL_mutexV(clipboard_from_host_mutex);
#else
	f = 0;
	while (f = EnumClipboardFormats (f)) {
		if (f == CF_UNICODETEXT)
			text = TRUE;
		if (f == CF_BITMAP && !keyboardinject)
			bmp = TRUE;
	}
#endif
	if (text)
	{
#ifdef AMIBERRY
		if (true)
		{
#else
		hglb = GetClipboardData (CF_UNICODETEXT); 
		if (hglb != NULL) { 
			TCHAR *lptstr = (TCHAR*)GlobalLock (hglb); 
#endif
			if (lptstr != nullptr)
			{
#if DEBUG_CLIP > 0
				write_log (_T("clipboard: CF_UNICODETEXT '%s'\n"), lptstr);
#endif
				if (keyboardinject)
				{
					to_keyboard(lptstr);
				}
				else
				{
					to_iff_text(ctx, lptstr);
				}
#ifdef AMIBERRY
				free(lptstr);
#else
				GlobalUnlock (hglb);
#endif
			}
		}
	}
	else if (bmp)
	{
#ifdef AMIBERRY
		write_log(_T("[CLIPBOARD] Bitmap clipboard sharing not implemented\n"));
#else
		HBITMAP hbmp = (HBITMAP)GetClipboardData (CF_BITMAP);
		if (hbmp != NULL) {
#if DEBUG_CLIP > 0
			write_log (_T("clipboard: CF_BITMAP\n"));
#endif
			to_iff_ilbm(ctx, hbmp);
		}
#endif
	}
#ifdef AMIBERRY
#else
	CloseClipboard ();
#endif
}

static void clipboard_free_delayed()
{
	if (clipboard_delayed_data == nullptr)
		return;
	if (clipboard_delayed_size < 0)
		xfree(clipboard_delayed_data);
#ifdef AMIBERRY
#else
	else
		DeleteObject (clipboard_delayed_data);
#endif
	clipboard_delayed_data = nullptr;
	clipboard_delayed_size = 0;
}

void clipboard_changed(int hwnd)
{
#if DEBUG_CLIP > 0
	write_log (_T("clipboard: windows clipboard changed message\n"));
#endif
	if (!initialized)
		return;
	if (!clipboard_data)
		return;
	if (clipopen)
		return;
	if (!clipactive)
	{
		clipboard_change = 1;
		return;
	}
	clipboard_read(nullptr, hwnd, false);
}

#ifdef AMIBERRY
#else
static int clipboard_put_bmp_real (HBITMAP hbmp)
{
	int ret = FALSE;

	if (!OpenClipboard (chwnd)) 
		return ret;
	clipopen++;
	EmptyClipboard ();
	SetClipboardData (CF_BITMAP, hbmp); 
	ret = TRUE;
	CloseClipboard ();
	clipopen--;
#if DEBUG_CLIP > 0
	write_log (_T("clipboard: BMP written to windows clipboard\n"));
#endif
	return ret;
}
#endif

static int clipboard_put_text_real(const TCHAR* txt)
{
#ifdef AMIBERRY
	SDL_mutexP(clipboard_to_host_mutex);
	if (clipboard_to_host_text != nullptr)
	{
		free(clipboard_to_host_text);
		clipboard_to_host_text = nullptr;
	}
	clipboard_to_host_text = strdup(txt);
	SDL_mutexV(clipboard_to_host_mutex);
	return true;
#else
	HGLOBAL hglb;
	int ret = FALSE;

	if (!OpenClipboard (chwnd)) 
		return ret;
	clipopen++;
	EmptyClipboard (); 
	hglb = GlobalAlloc (GMEM_MOVEABLE, (_tcslen (txt) + 1) * sizeof (TCHAR));
	if (hglb) {
		TCHAR *lptstr = (TCHAR*)GlobalLock (hglb);
		_tcscpy (lptstr, txt);
		GlobalUnlock (hglb);
		SetClipboardData (CF_UNICODETEXT, hglb); 
		ret = TRUE;
	}
	CloseClipboard ();
	clipopen--;
#if DEBUG_CLIP > 0
	write_log (_T("clipboard: text written to windows clipboard\n"));
#endif
	return ret;
#endif
}

static int clipboard_put_text(const TCHAR* txt)
{
#ifdef AMIBERRY
	return clipboard_put_text_real(txt);
#else
	if (!clipactive)
		return clipboard_put_text_real (txt);
	clipboard_free_delayed ();
	clipboard_delayed_data = my_strdup (txt);
	clipboard_delayed_size = -1;
	return 1;
#endif
}

#ifdef AMIBERRY
#else
static int clipboard_put_bmp (HBITMAP hbmp)
{
	if (!clipactive)
		return clipboard_put_bmp_real (hbmp);
	clipboard_delayed_data = (void*)hbmp;
	clipboard_delayed_size = 1;
	return 1;
}
#endif

void amiga_clipboard_die(TrapContext* ctx)
{
	signaling = 0;
	write_log(_T("clipboard not initialized\n"));
}

void amiga_clipboard_init(TrapContext* ctx)
{
	signaling = 0;
	write_log(_T("clipboard initialized\n"));
	initialized = 1;
	clipboard_read(ctx, chwnd, false);
}

void amiga_clipboard_task_start(TrapContext* ctx, uaecptr data)
{
	clipboard_data = data;
	signaling = 1;
	write_log(_T("clipboard task init: %08x\n"), clipboard_data);
}

uae_u32 amiga_clipboard_proc_start(TrapContext* ctx)
{
	write_log(_T("clipboard process init: %08x\n"), clipboard_data);
	signaling = 1;
	return clipboard_data;
}

void amiga_clipboard_got_data(TrapContext* ctx, uaecptr data, uae_u32 size, uae_u32 actual)
{
	if (!initialized)
	{
		write_log(_T("clipboard: got_data() before initialized!?\n"));
		return;
	}
#if DEBUG_CLIP > 0
	write_log (_T("clipboard: <-amiga, %08x, %08x %d %d\n"), clipboard_data, data, size, actual);
#endif
	from_iff(ctx, data, actual);
}

int amiga_clipboard_want_data(TrapContext* ctx)
{
#ifdef AMIBERRY
	write_log("amiga_clipboard_want_data\n");
#endif

	const auto addr = trap_get_long(ctx, clipboard_data + 4);
	const auto size = trap_get_long(ctx, clipboard_data);
	if (!initialized)
	{
		write_log(_T("clipboard: want_data() before initialized!? (%08x %08x %d)\n"), clipboard_data, addr, size);
		to_amiga = nullptr;
		return 0;
	}
	if (size != to_amiga_size)
	{
		write_log(_T("clipboard: size %d <> %d mismatch!?\n"), size, to_amiga_size);
		to_amiga = nullptr;
		return 0;
	}
	if (addr && size)
	{
		trap_put_bytes(ctx, to_amiga, addr, size);
	}
	xfree(to_amiga);
#if DEBUG_CLIP > 0
	write_log (_T("clipboard: ->amiga, %08x, %08x %d (%d) bytes\n"), clipboard_data, addr, size, to_amiga_size);
#endif
	to_amiga = nullptr;
	to_amiga_size = 0;
	return 1;
}

void clipboard_active(int hwnd, int active)
{
	clipactive = active;
	if (!initialized || !hwnd)
		return;
	if (clipactive && clipboard_change)
	{
		clipboard_read(nullptr, hwnd, false);
	}
	if (!clipactive && clipboard_delayed_data)
	{
		if (clipboard_delayed_size < 0)
		{
			clipboard_put_text_real(static_cast<TCHAR*>(clipboard_delayed_data));
			xfree(clipboard_delayed_data);
		}
		else
		{
			//clipboard_put_bmp_real ((HBITMAP)clipboard_delayed_data);
		}
		clipboard_delayed_data = nullptr;
		clipboard_delayed_size = 0;
	}
}

static uae_u32 clipboard_vsync_cb(TrapContext* ctx, void* ud)
{
	const auto task = trap_get_long(ctx, clipboard_data + 8);
	if (task && native2amiga_isfree())
	{
		uae_Signal(task, 1 << 13);
#if DEBUG_CLIP > 0
		write_log(_T("clipboard: signal %08x\n"), clipboard_data);
#endif
	}
	return 0;
}

void clipboard_vsync()
{
	if (!filesys_heartbeat())
		return;
	if (!clipboard_data)
		return;

	if (signaling)
	{
		vdelay--;
		if (vdelay > 0)
			return;

		trap_callback(clipboard_vsync_cb, nullptr);
		vdelay = 50;
	}

	if (vdelay2 > 0)
	{
		vdelay2--;
		//write_log(_T("vdelay2 = %d\n"), vdelay2);
	}

	if (to_amiga_phase == 1 && vdelay2 <= 0)
	{
		trap_callback(to_amiga_start_cb, nullptr);
	}
}

void clipboard_reset()
{
	write_log(_T("clipboard: reset (%08x)\n"), clipboard_data);
	clipboard_unsafeperiod();
	clipboard_free_delayed();
	clipboard_data = 0;
	signaling = 0;
	initialized = 0;
	xfree(to_amiga);
	to_amiga = nullptr;
	to_amiga_size = 0;
	to_amiga_phase = 0;
	clip_disabled = false;
#ifdef AMIBERRY
#else
	ReleaseDC (chwnd, hdc);
#endif
}

#ifdef AMIBERRY
void clipboard_init()
{
	chwnd = 1; // fake window handle
	write_log(_T("clipboard_init\n"));
	clipboard_from_host_mutex = SDL_CreateMutex();
	clipboard_from_host_text = strdup("");
	clipboard_to_host_mutex = SDL_CreateMutex();
	// Activate clipboard functionality (let UAE think we are in the
	// foreground).
	clipactive = 1;
#else
void clipboard_init (HWND hwnd)
{
	chwnd = hwnd;
	hdc = GetDC (chwnd);
#endif
}

void target_paste_to_keyboard()
{
#ifdef AMIBERRY
	write_log("target_paste_to_keyboard (clipboard)\n");
#endif
	clipboard_read(nullptr, chwnd, true);
}

// force 2-second delay before accepting new data
void clipboard_unsafeperiod()
{
	vdelay2 = 100;
	if (vdelay < 60)
		vdelay = 60;
}

#ifdef AMIBERRY // NL

char* uae_clipboard_get_text()
{
	SDL_mutexP(clipboard_to_host_mutex);
	char* text = clipboard_to_host_text;
	if (text)
	{
		clipboard_to_host_text = nullptr;
	}
	SDL_mutexV(clipboard_to_host_mutex);
	return text;
}

void uae_clipboard_free_text(const char* text)
{
	if (text)
	{
		free(clipboard_to_host_text);
	}
	else
	{
		write_log("WARNING: uae_clipboard_free_text called with NULL pointer\n");
	}
}

void uae_clipboard_put_text(const char* text)
{
	if (!clipboard_from_host_text)
	{
		// clipboard_init not called yet
		return;
	}
	if (!text)
	{
		text = "";
	}
	if (strcmp(clipboard_from_host_text, text) == 0)
	{
		return;
	}
	SDL_mutexP(clipboard_from_host_mutex);
	if (clipboard_from_host_text)
	{
		free(clipboard_from_host_text);
	}
	clipboard_from_host_text = strdup(text);
	clipboard_from_host_changed = true;
	SDL_mutexV(clipboard_from_host_mutex);

	// FIXME: We can call clipboard_changed directly, and so, we do not
	// really need the locks (all access to clipboard_from_host* is from
	// main thread.

	// FIXME: Replace last with global clipboard_from_host_text data, this
	// can hold the last clipboard data indefinitively, in case we want to
	// read again, we just use clipboard_from_host_changed to signal new data
	// anyway

	// Edit, actually, clipboard_read is called from filesys handler / trap
	// mousehack thinghy? (UAE thread)
#if 1
	//if (clipboard_from_host_changed) {
	write_log("clipboard new data from host, calling clipboard_changed\n");
	// clipboard_read(NULL, chwnd, false);
	clipboard_changed(chwnd);
	//}
#endif
}

#endif  // AMIBERRY
