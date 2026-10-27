/*
 * LHA archive support for zfile, built on Lhasa (ISC licence, vendored in
 * external/lhasa). This replaces the LHa for UNIX 1.14i decoder, whose
 * licence was non-free and not usable in distributed binaries.
 *
 * Listing walks the archive headers with Lhasa's basic reader and stores
 * each member's ordinal position in znode::offset; extraction re-walks the
 * headers to that member and decodes it through Lhasa, verifying the
 * header's CRC-16 over the decompressed data (a mismatch fails the
 * extraction instead of returning corrupt data).
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
}

/* ------------------------------------------------------------------ */
/* LHAInputStream over a struct zfile                                  */
/* ------------------------------------------------------------------ */

/* Lhasa's contract: the number of bytes read (0 at the end of the file),
 * or -1 for an error, which zfile does not report separately. */
static int lha_stream_read (void *handle, void *buf, size_t buf_len)
{
	struct zfile *zf = (struct zfile *) handle;
	return (int) zfile_fread (buf, 1, buf_len, zf);
}

static int lha_stream_skip (void *handle, size_t bytes)
{
	struct zfile *zf = (struct zfile *) handle;
	return zfile_fseek (zf, (uae_s64) bytes, SEEK_CUR) == 0;
}

static const LHAInputStreamType lha_stream_type = {
	lha_stream_read,
	lha_stream_skip,
	NULL /* close: the zfile belongs to the caller */
};

/* A reader that returns the archive's entries one to one, in order:
 * members are found again by their position. */
static LHAReader *lha_open_reader (struct zfile *zf, LHAInputStream **streamp)
{
	LHAInputStream *stream;
	LHAReader *reader;

	*streamp = NULL;
	zfile_fseek (zf, 0, SEEK_SET);
	stream = lha_input_stream_new (&lha_stream_type, zf);
	if (!stream)
		return NULL;
	reader = lha_reader_new (stream);
	if (!reader) {
		lha_input_stream_free (stream);
		return NULL;
	}
	lha_reader_set_dir_policy (reader, LHA_READER_DIR_PLAIN);
	*streamp = stream;
	return reader;
}

/* ------------------------------------------------------------------ */
/* Extended-header scan                                               */
/* ------------------------------------------------------------------ */

/* What the old LHa glue exposed beyond Lhasa's header fields: the
 * operating-system attribute (0x40) and unix timestamp (0x54) extensions,
 * and the file comment, which Amiga LhA stores after a NUL in the name
 * (in the level 0/1 name field, or in the filename extension 0x01 of
 * level 2+ headers). Lhasa keeps the complete raw header in raw_data. */
struct lha_extra_info {
	bool has_attribute;
	unsigned int attribute;
	bool has_unix_time;
	unsigned long unix_time;
	const unsigned char *comment;
	size_t comment_len;
};

/* A stored name of len bytes: the comment is whatever follows its NUL. */
static void lha_name_comment (const unsigned char *name, size_t len, struct lha_extra_info *info)
{
	const unsigned char *nul = (const unsigned char *) memchr (name, 0, len);
	if (nul && nul + 1 < name + len) {
		info->comment = nul + 1;
		info->comment_len = (size_t) (name + len - (nul + 1));
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
	case 1:
		/* The name field: its length at offset 21, the bytes from 22. */
		if (raw_len > 21 && 22 + (size_t) raw[21] <= raw_len)
			lha_name_comment (raw + 22, raw[21], info);
		if (hdr->header_level == 0)
			return; /* level 0 headers have no extension headers */
		off = raw[0]; /* base header size byte: extensions follow */
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
			size = raw[off] | (raw[off + 1] << 8)
				| (raw[off + 2] << 16) | ((size_t) raw[off + 3] << 24);
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
				info->unix_time = content[0] | (content[1] << 8)
					| (content[2] << 16) | ((unsigned long) content[3] << 24);
			}
			break;
		default:
			break;
		}

		off += size;
	}
}

/* ------------------------------------------------------------------ */
/* Header field conversion                                            */
/* ------------------------------------------------------------------ */

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

static time_t lha_mtime (const LHAFileHeader *hdr, const struct lha_extra_info *info)
{
	time_t t = (time_t) hdr->timestamp;

	/* Lhasa sets the unix timestamp directly when the header carries one
	 * (level 2+, or a level 0 OS-extended area); the level 1+ 0x54
	 * extension has to be picked up here. */
	if (info->has_unix_time
		&& (hdr->os_type == LHA_OS_TYPE_UNIX || hdr->os_type == LHA_OS_TYPE_AMIGA))
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

/* The CRC follows the stored path in level 0/1 headers; the oldest headers
 * could omit it (the same rule the WHDBooter's own header walker applies).
 * Level 2+ headers always carry one. */
static bool lha_has_crc (const LHAFileHeader *hdr)
{
	if (hdr->header_level >= 2)
		return true;
	return hdr->raw_data_len > 21
		&& (size_t) hdr->raw_data[0] >= 24 + (size_t) hdr->raw_data[21];
}

/* "path/filename" of a member with '/' separators; directories have a
 * NULL filename and a path ending in '/'. Lhasa converts '\' in level 0/1
 * names but not the 0xff separator LHa writes there, so convert it too. */
static char *lha_member_name (const LHAFileHeader *hdr)
{
	const char *path = hdr->path ? hdr->path : "";
	const char *name = hdr->filename ? hdr->filename : "";
	size_t pathlen = strlen (path), namelen = strlen (name);
	char *out, *p;

	out = xmalloc (char, pathlen + namelen + 1);
	p = out;
	memcpy (p, path, pathlen);
	p += pathlen;
	memcpy (p, name, namelen);
	p[namelen] = 0;
	for (p = out; *p; p++) {
		if ((unsigned char) *p == 0xff)
			*p = '/';
	}
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
	LHAInputStream *stream = NULL;
	LHAReader *reader;
	LHAFileHeader *hdr;
	unsigned int index = 0;

	_tzset ();
	zv = zvolume_alloc (zf, ArchiveFormatLHA, NULL, NULL);
	reader = lha_open_reader (zf, &stream);
	if (!reader)
		return zv;

	while ((hdr = lha_reader_next_file (reader)) != NULL) {
		bool isdir = strcmp (hdr->compress_method, LHA_COMPRESS_TYPE_DIR) == 0;
		char *member = lha_member_name (hdr);

		memset (&extra, 0, sizeof extra);
		lha_scan_extensions (hdr, &extra);
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
				zn->offset = index; /* member ordinal, for archive_access_lha */
		}
		index++;

		xfree (zai.comment);
		xfree (zai.name);
		xfree (member);
	}

	lha_reader_free (reader);
	lha_input_stream_free (stream);
	return zv;
}

/* Decodes the reader's current member into out (pre-allocated at the
 * member's size) and checks the header's CRC over the decompressed data. */
static bool lha_extract_current (LHAReader *reader, LHAFileHeader *hdr, struct zfile *out)
{
	uae_u8 *p = out->data;
	size_t left = (size_t) out->size;
	unsigned int crc = 0;
	bool ok = true;

	while (left > 0) {
		size_t chunk = left > 65536 ? 65536 : left;
		size_t got = lha_reader_read (reader, p, chunk);
		if (got == 0) {
			/* unsupported method, truncated archive or corrupt data */
			ok = false;
			break;
		}
		crc = lha_crc16_update (crc, p, got);
		p += got;
		left -= got;
	}

	if (ok && lha_has_crc (hdr) && out->size > 0 && crc != hdr->crc) {
		write_log (_T("LHA: CRC mismatch for '%s' (got %04x, expected %04x)\n"),
			out->name, crc, hdr->crc);
		ok = false;
	} else if (!ok) {
		TCHAR *method = au (hdr->compress_method);
		write_log (_T("LHA: could not decode '%s' (method %s: unsupported, truncated or corrupt)\n"),
			out->name, method);
		xfree (method);
	}
	return ok;
}

struct zfile *archive_access_lha (struct znode *zn)
{
	struct zfile *zf = zn->volume->archive;
	struct zfile *out = NULL;
	LHAInputStream *stream = NULL;
	LHAReader *reader;
	LHAFileHeader *hdr = NULL;
	unsigned int index = 0;

	reader = lha_open_reader (zf, &stream);
	if (!reader)
		return NULL;

	while (index <= zn->offset && (hdr = lha_reader_next_file (reader)) != NULL) {
		if (index == zn->offset)
			break;
		index++;
	}

	if (hdr && strcmp (hdr->compress_method, LHA_COMPRESS_TYPE_DIR) != 0
		&& (uae_u64) hdr->length == (uae_u64) zn->size) {
		out = zfile_fopen_empty (zf, zn->name, zn->size);
		if (out && !lha_extract_current (reader, hdr, out)) {
			zfile_fclose (out);
			out = NULL;
		}
	} else {
		write_log (_T("LHA: member '%s' not found in archive\n"), zn->name);
	}

	lha_reader_free (reader);
	lha_input_stream_free (stream);
	return out;
}
