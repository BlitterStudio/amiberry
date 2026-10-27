/*
 * LHA archive support for zfile, built on Lhasa (ISC licence, vendored in
 * external/lhasa). This replaces the LHa for UNIX 1.14i decoder, whose
 * licence was non-free and not usable in distributed binaries.
 *
 * Listing walks the archive headers with Lhasa's basic reader and stores
 * each member's ordinal position in znode::offset. Extraction decodes the
 * member's raw stream (no MacBinary or other filtering) and verifies the
 * header's CRC-16 over the decompressed data, so a mismatch fails the
 * extraction instead of returning corrupt data. Each archive volume keeps a
 * reader positioned after the last member it returned, so extracting the
 * members in order reads the archive once.
 *
 * Copyright (C) 2026 Dimitris Panokostas
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "zfile.h"
#include "zarchive.h"

extern "C" {
#include "public/lhasa.h"
#include "lha_basic_reader.h"
}

/* ------------------------------------------------------------------ */
/* LHAInputStream over a struct zfile                                  */
/* ------------------------------------------------------------------ */

/* The stream keeps its own position and seeks before every read, so other
 * users of the archive's zfile between two calls can't disturb it. */
struct lha_zstream {
	struct zfile *zf;
	uae_s64 pos;
};

/* Lhasa's contract: the number of bytes read (0 at the end of the file),
 * or -1 for an error. */
static int lha_stream_read (void *handle, void *buf, size_t buf_len)
{
	struct lha_zstream *s = (struct lha_zstream *) handle;
	size_t got;

	if (zfile_fseek (s->zf, s->pos, SEEK_SET) != 0)
		return -1;
	got = zfile_fread (buf, 1, buf_len, s->zf);
	s->pos += got;
	return (int) got;
}

static int lha_stream_skip (void *handle, size_t bytes)
{
	struct lha_zstream *s = (struct lha_zstream *) handle;
	s->pos += bytes;
	return 1;
}

static const LHAInputStreamType lha_stream_type = {
	lha_stream_read,
	lha_stream_skip,
	NULL /* close: the zfile belongs to the caller */
};

/* A basic reader over an archive, from its start. */
struct lha_cursor {
	struct lha_zstream zs;
	LHAInputStream *stream;
	LHABasicReader *reader;
	LHAFileHeader *hdr;	/* current member, owned by the reader */
	int index;		/* ordinal of hdr, -1 before the first */
	bool decoded;		/* hdr's data has been read */
};

static void lha_cursor_close (struct lha_cursor *c)
{
	if (c->reader)
		lha_basic_reader_free (c->reader);
	if (c->stream)
		lha_input_stream_free (c->stream);
	c->reader = NULL;
	c->stream = NULL;
	c->hdr = NULL;
	c->index = -1;
	c->decoded = false;
}

static bool lha_cursor_open (struct lha_cursor *c, struct zfile *zf)
{
	lha_cursor_close (c);
	c->zs.zf = zf;
	c->zs.pos = 0;
	c->stream = lha_input_stream_new (&lha_stream_type, &c->zs);
	if (!c->stream)
		return false;
	c->reader = lha_basic_reader_new (c->stream);
	if (!c->reader) {
		lha_input_stream_free (c->stream);
		c->stream = NULL;
		return false;
	}
	return true;
}

static LHAFileHeader *lha_cursor_next (struct lha_cursor *c)
{
	c->hdr = lha_basic_reader_next_file (c->reader);
	c->decoded = false;
	if (c->hdr)
		c->index++;
	return c->hdr;
}

/* ------------------------------------------------------------------ */
/* Header fields the old glue exposed                                 */
/* ------------------------------------------------------------------ */

/* Beyond Lhasa's header fields: the operating-system attribute (0x40) and
 * unix timestamp (0x54) extensions, the unix timestamp in a level 0
 * OS-extended area, and the file comment, which Amiga LhA stores after a
 * NUL in the name (in the level 0/1 name field, or in the filename
 * extension 0x01 of level 2+ headers). Lhasa keeps the complete raw header
 * in raw_data. */
struct lha_extra_info {
	bool has_attribute;
	unsigned int attribute;
	bool has_unix_time;
	unsigned long unix_time;
	const unsigned char *comment;
	size_t comment_len;
};

static unsigned long lha_le32 (const unsigned char *p)
{
	return p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned long) p[3] << 24);
}

/* A stored name of len bytes: the comment is whatever follows its NUL. */
static void lha_name_comment (const unsigned char *name, size_t len, struct lha_extra_info *info)
{
	const unsigned char *nul = (const unsigned char *) memchr (name, 0, len);
	if (nul && nul + 1 < name + len) {
		info->comment = nul + 1;
		info->comment_len = (size_t) (name + len - (nul + 1));
	}
}

/* Level 0: the size byte at offset 0 counts the header from offset 2; the
 * name length is at 21 and the name from 22. A size of at least 22 + name
 * length means a CRC follows the name; an OS-extended area may follow the
 * CRC: an OS byte, then for Unix ('U') and Amiga ('A') a minor version and
 * the 32-bit unix timestamp (with mode, uid and gid after it). */
static void lha_scan_level0 (const unsigned char *raw, size_t raw_len, struct lha_extra_info *info)
{
	size_t namelen, header_size;
	long extend;

	if (raw_len < 22)
		return;
	namelen = raw[21];
	header_size = raw[0];
	if (22 + namelen <= raw_len)
		lha_name_comment (raw + 22, namelen, info);
	extend = (long) header_size - (long) namelen - 22;
	if (extend >= 1 + 11 && 24 + namelen + 6 <= raw_len) {
		const unsigned char os = raw[24 + namelen];
		if (os == 'U' || os == 'A') {
			info->has_unix_time = true;
			info->unix_time = lha_le32 (raw + 24 + namelen + 2);
		}
	}
}

static void lha_scan_extensions (const LHAFileHeader *hdr, struct lha_extra_info *info)
{
	const unsigned char *raw = hdr->raw_data;
	const size_t raw_len = hdr->raw_data_len;
	const size_t field_size = hdr->header_level == 3 ? 4 : 2;
	size_t off;

	switch (hdr->header_level) {
	case 0:
		lha_scan_level0 (raw, raw_len, info);
		return;
	case 1:
		/* The name field, as in level 0; extensions follow the base header. */
		if (raw_len > 21 && 22 + (size_t) raw[21] <= raw_len)
			lha_name_comment (raw + 22, raw[21], info);
		off = raw[0];
		break;
	case 2:
		off = 24;
		break;
	case 3:
		off = 28;
		break;
	default:
		return;
	}

	while (off + field_size <= raw_len) {
		size_t size, content_len;
		const unsigned char *content;

		if (field_size == 4)
			size = lha_le32 (raw + off);
		else
			size = raw[off] | (raw[off + 1] << 8);

		if (size == 0)
			break; /* end of the extension chain */
		if (size < field_size + 1 || off + size > raw_len)
			break; /* corrupt: stop trusting the rest */

		content = raw + off + field_size + 1;
		content_len = size - field_size - 1;

		switch (raw[off + field_size]) {
		case 0x01: /* filename */
			lha_name_comment (content, content_len, info);
			break;
		case 0x40: /* operating-system file attribute */
			if (content_len >= 2) {
				info->has_attribute = true;
				info->attribute = content[0] | (content[1] << 8);
			}
			break;
		case 0x54: /* unix last-modified time */
			if (content_len >= 4) {
				info->has_unix_time = true;
				info->unix_time = lha_le32 (content);
			}
			break;
		default:
			break;
		}

		off += size;
	}
}

/* The archive stores wall-clock times; report them on a UTC scale, like
 * the previous glue (and the TAR archiver) did. */
static time_t lha_to_utc (time_t t)
{
#if defined(__linux__) || defined(_WIN32)
	return t - _timezone;
#else
	struct tm *lt = localtime (&t);
	return lt ? t - lt->tm_gmtoff : t;
#endif
}

/* The unix timestamp where the header carries one (the level 0 Unix/Amiga
 * extended area, or the level 1+ 0x54 extension on Unix and Amiga
 * archives), else Lhasa's timestamp. */
static time_t lha_mtime (const LHAFileHeader *hdr, const struct lha_extra_info *info)
{
	time_t t = (time_t) hdr->timestamp;

	if (info->has_unix_time
		&& (hdr->header_level == 0
			|| hdr->os_type == LHA_OS_TYPE_UNIX || hdr->os_type == LHA_OS_TYPE_AMIGA))
		t = (time_t) info->unix_time;

	return lha_to_utc (t);
}

/* The attribute exposed to the filesystem layer: the attribute byte at
 * offset 19, present in every header level (the Amiga protection bits on
 * Amiga archives), which a level 1+ operating-system attribute (0x40)
 * extension overrides with its low byte - as the previous glue did. */
static int lha_attribute (const LHAFileHeader *hdr, const struct lha_extra_info *info)
{
	int attr = 0;

	if (hdr->raw_data_len > 19)
		attr = hdr->raw_data[19];

	if (info->has_attribute
		&& (hdr->os_type == LHA_OS_TYPE_MSDOS
			|| hdr->os_type == LHA_OS_TYPE_HUMAN68K
			|| hdr->os_type == LHA_OS_TYPE_AMIGA
			|| hdr->os_type == LHA_OS_TYPE_UNKNOWN))
		attr = (int) (info->attribute & 0xff);

	return attr;
}

/* Level 1+ headers always carry a CRC; a level 0 one when its size byte is
 * at least 22 + the name length (the oldest headers omitted it). */
static bool lha_has_crc (const LHAFileHeader *hdr)
{
	if (hdr->header_level >= 1)
		return true;
	return hdr->raw_data_len > 21
		&& (size_t) hdr->raw_data[0] >= 22 + (size_t) hdr->raw_data[21];
}

/* "path/filename" of a member with '/' separators; directories have a
 * NULL filename and a path ending in '/'. The vendored Lhasa has already
 * converted MacLHA's ':' separators (convert_macos_name()). */
static char *lha_member_name (const LHAFileHeader *hdr)
{
	const char *path = hdr->path ? hdr->path : "";
	const char *name = hdr->filename ? hdr->filename : "";
	size_t pathlen = strlen (path), namelen = strlen (name);
	char *out = xmalloc (char, pathlen + namelen + 1);

	memcpy (out, path, pathlen);
	memcpy (out + pathlen, name, namelen);
	out[pathlen + namelen] = 0;
	return out;
}

/* The LhA CRC-16 (the value the archive headers store), over the
 * decompressed data. */
static unsigned int lha_crc16_update (unsigned int crc, const uae_u8 *data, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		crc ^= data[i];
		for (int bit = 0; bit < 8; bit++)
			crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
	}
	return crc & 0xffff;
}

/* ------------------------------------------------------------------ */
/* zfile archiver interface                                           */
/* ------------------------------------------------------------------ */

struct zvolume *archive_directory_lha (struct zfile *zf)
{
	struct zvolume *zv;
	struct znode *zn;
	struct zarchive_info zai;
	struct lha_extra_info extra;
	struct lha_cursor c = {};
	LHAFileHeader *hdr;

	_tzset ();
	zv = zvolume_alloc (zf, ArchiveFormatLHA, NULL, NULL);
	c.index = -1;
	if (!lha_cursor_open (&c, zf))
		return zv;

	while ((hdr = lha_cursor_next (&c)) != NULL) {
		bool isdir = strcmp (hdr->compress_method, LHA_COMPRESS_TYPE_DIR) == 0;
		char *member;

		memset (&extra, 0, sizeof extra);
		lha_scan_extensions (hdr, &extra);
		member = lha_member_name (hdr);
		/* The znode layer copies names into MAX_DPATH buffers after the
		 * volume path (and may append ".1" to duplicates): skip a member
		 * whose name can't fit, as the old parser rejected long names. */
		if (strlen (member) + (zv->root.fullname ? uaetcslen (zv->root.fullname) : 0) + 16 >= MAX_DPATH) {
			write_log (_T("LHA: skipping a member with a %zu-byte name\n"), strlen (member));
			xfree (member);
			continue;
		}
		memset (&zai, 0, sizeof zai);
		zai.name = au (member);
		zai.size = (uae_s64) hdr->length;
		zai.flags = lha_attribute (hdr, &extra);
		zai.tv.tv_sec = lha_mtime (hdr, &extra);
		if (extra.comment_len > 0) {
			char *comment = xmalloc (char, extra.comment_len + 1);
			memcpy (comment, extra.comment, extra.comment_len);
			comment[extra.comment_len] = 0;
			while (extra.comment_len > 0 && comment[extra.comment_len - 1] == 0)
				extra.comment_len--;
			if (extra.comment_len > 0)
				zai.comment = au (comment);
			xfree (comment);
		}

		if (isdir) {
			zvolume_adddir_abs (zv, &zai);
		} else {
			zn = zvolume_addfile_abs (zv, &zai);
			if (zn)
				zn->offset = (unsigned int) c.index; /* member ordinal */
		}

		xfree (zai.comment);
		xfree (zai.name);
		xfree (member);
	}

	lha_cursor_close (&c);
	return zv;
}

/* Decodes the cursor's current member (its raw stream) into out,
 * pre-allocated at the member's size, and checks the header's CRC. */
static bool lha_extract_current (struct lha_cursor *c, struct zfile *out)
{
	LHAFileHeader *hdr = c->hdr;
	LHADecoder *dec = lha_basic_reader_decode (c->reader);
	uae_u8 *p = out->data;
	size_t left = (size_t) out->size;
	unsigned int crc = 0;
	bool ok = true;

	c->decoded = true;
	if (!dec) {
		TCHAR *method = au (hdr->compress_method);
		write_log (_T("LHA: unsupported compression method %s ('%s')\n"), method, out->name);
		xfree (method);
		return false;
	}

	while (left > 0) {
		size_t chunk = left > 65536 ? 65536 : left;
		size_t got = lha_decoder_read (dec, p, chunk);
		if (got == 0) {
			ok = false; /* truncated archive or corrupt data */
			break;
		}
		crc = lha_crc16_update (crc, p, got);
		p += got;
		left -= got;
	}
	lha_decoder_free (dec);

	if (ok && lha_has_crc (hdr) && out->size > 0 && crc != hdr->crc) {
		write_log (_T("LHA: CRC mismatch for '%s' (got %04x, expected %04x)\n"),
			out->name, crc, hdr->crc);
		ok = false;
	} else if (!ok) {
		write_log (_T("LHA: could not decode '%s' (truncated or corrupt)\n"), out->name);
	}
	return ok;
}

struct zfile *archive_access_lha (struct znode *zn)
{
	struct zvolume *zv = zn->volume;
	struct zfile *zf = zv->archive;
	struct lha_cursor *c = (struct lha_cursor *) zv->handle;
	struct zfile *out = NULL;
	const int want = (int) zn->offset;

	if (!c) {
		c = xcalloc (struct lha_cursor, 1);
		c->index = -1;
		zv->handle = c;
	}
	/* Continue from the last member returned; an earlier member (or the
	 * same one again, its data already read) needs a fresh walk. */
	if (!c->reader || c->index > want || (c->index == want && c->decoded)) {
		if (!lha_cursor_open (c, zf))
			return NULL;
	}
	while (c->index < want && lha_cursor_next (c) != NULL)
		;

	if (c->index == want && c->hdr
		&& strcmp (c->hdr->compress_method, LHA_COMPRESS_TYPE_DIR) != 0
		&& (uae_u64) c->hdr->length == (uae_u64) zn->size) {
		out = zfile_fopen_empty (zf, zn->name, zn->size);
		if (out && !lha_extract_current (c, out)) {
			zfile_fclose (out);
			out = NULL;
		}
	} else {
		write_log (_T("LHA: member '%s' not found in archive\n"), zn->name);
		lha_cursor_close (c);
	}
	return out;
}

void archive_close_lha (void *handle)
{
	struct lha_cursor *c = (struct lha_cursor *) handle;
	if (!c)
		return;
	lha_cursor_close (c);
	xfree (c);
}
