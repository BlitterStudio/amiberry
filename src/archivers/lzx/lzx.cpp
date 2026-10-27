/*
 * LZX archive support for zfile.
 *
 * Copyright (C) 2026 Dimitris Panokostas and the Amiberry contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This code was written from a functional specification of the LZX
 * archive format, without reference to any other LZX decoder.
 *
 * An LZX archive is a list of entries; runs of entries ("merged groups")
 * are compressed together as one stream, whose packed data follows the
 * group's last entry (the terminator, the one with a non-zero packed size).
 * Each file node remembers its group in the decoder-specific znode fields:
 *   offset      absolute offset of the group terminator's entry header,
 *               0 if the entry has no group (a truncated archive)
 *   packedsize  the group's unpacked total, UINT_MAX if it overflows
 *   offset2     the member's offset within the group's unpacked output
 *   method      the member's stored CRC-32
 * The terminator's header is read again at access time for the packed size,
 * pack mode and data offset, so a group is found without walking the node
 * chain, and entries that got no node still count towards the layout.
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <new>
#include <vector>

#include <zlib.h>

#include "sysconfig.h"
#include "sysdeps.h"
#include "zfile.h"
#include "zarchive.h"

namespace {

constexpr int ARCHIVE_HEADER_SIZE = 10;
constexpr int ENTRY_HEADER_SIZE = 31;
constexpr int MODE_STORED = 0;
constexpr int MODE_COMPRESSED = 2;
constexpr int MAX_MATCH = 258;

uae_u32 get_le32 (const uae_u8 *p)
{
	return (uae_u32)p[0] | ((uae_u32)p[1] << 8) | ((uae_u32)p[2] << 16) | ((uae_u32)p[3] << 24);
}

uae_u32 get_be32 (const uae_u8 *p)
{
	return ((uae_u32)p[0] << 24) | ((uae_u32)p[1] << 16) | ((uae_u32)p[2] << 8) | (uae_u32)p[3];
}

unsigned int clamp_u32 (uae_u64 v)
{
	return v > UINT_MAX ? UINT_MAX : (unsigned int)v;
}

/* ------------------------------------------------------------------ */
/* Entry headers                                                       */
/* ------------------------------------------------------------------ */

struct lzx_entry {
	uae_u8 header[ENTRY_HEADER_SIZE];
	char name[256];      /* NUL-terminated */
	char comment[256];   /* NUL-terminated */
	int comment_len;
	uae_u32 unpacked;
	uae_u32 packed;
	uae_u32 data_crc;
	int mode;
	uae_u64 next;        /* offset of the byte after the comment */
};

enum class entry_read { ok, end, truncated, bad_crc };

entry_read read_entry (struct zfile *zf, uae_u64 pos, lzx_entry &e)
{
	zfile_fseek (zf, (uae_s64)pos, SEEK_SET);
	size_t got = zfile_fread (e.header, 1, ENTRY_HEADER_SIZE, zf);
	if (got == 0)
		return entry_read::end;
	if (got < ENTRY_HEADER_SIZE)
		return entry_read::truncated;

	const size_t name_len = e.header[30];
	const size_t comment_len = e.header[14];
	if (zfile_fread (e.name, 1, name_len, zf) != name_len)
		return entry_read::truncated;
	if (zfile_fread (e.comment, 1, comment_len, zf) != comment_len)
		return entry_read::truncated;
	e.name[name_len] = 0;
	e.comment[comment_len] = 0;

	uae_u8 h[ENTRY_HEADER_SIZE];
	memcpy (h, e.header, sizeof h);
	memset (h + 26, 0, 4);
	uLong crc = crc32 (0L, h, sizeof h);
	crc = crc32 (crc, (const Bytef *)e.name, (uInt)name_len);
	crc = crc32 (crc, (const Bytef *)e.comment, (uInt)comment_len);
	if ((uae_u32)crc != get_le32 (e.header + 26))
		return entry_read::bad_crc;

	e.comment_len = (int)comment_len;
	e.unpacked = get_le32 (e.header + 2);
	e.packed = get_le32 (e.header + 6);
	e.mode = e.header[11];
	e.data_crc = get_le32 (e.header + 22);
	e.next = pos + ENTRY_HEADER_SIZE + name_len + comment_len;
	return entry_read::ok;
}

/* LZX stores "allowed" RWED bits; Amiga protection bits mean "denied". */
int lzx_flags (uae_u8 attributes)
{
	static const uae_u8 amiga_bit[8] = { 0x08, 0x04, 0x01, 0x02, 0x10, 0x80, 0x40, 0x20 };
	int flags = 0;
	for (int i = 0; i < 8; i++) {
		if (attributes & (1 << i))
			flags |= amiga_bit[i];
	}
	return flags ^ 0x0f;
}

/* Host-local time, as Amiberry has always shown LZX dates. */
uae_s64 lzx_mtime (uae_u32 w)
{
	struct tm tm;
	memset (&tm, 0, sizeof tm);
	tm.tm_mday = (int)(w >> 27);
	tm.tm_mon = (int)((w >> 23) & 15);
	tm.tm_year = (int)((w >> 17) & 63) + 70;
	tm.tm_hour = (int)((w >> 12) & 31);
	tm.tm_min = (int)((w >> 6) & 63);
	tm.tm_sec = (int)(w & 63);
	tm.tm_isdst = 0;
	return (uae_s64)mktime (&tm);
}

/* ------------------------------------------------------------------ */
/* Bit reader                                                          */
/* ------------------------------------------------------------------ */

/* Big-endian 16-bit words, each consumed least-significant bit first.
 * Bits past the data read as 0; using them is an error, checked by the
 * caller through overrun(). An odd final byte is the high half of the
 * last word, whose (missing) low half reads as 0. */
struct bit_reader {
	const uae_u8 *data;
	uae_u64 size;
	uae_u64 next = 0;
	uae_u64 buf = 0;
	unsigned int count = 0;
	uae_u64 used = 0;
	uae_u64 limit;

	bit_reader (const uae_u8 *d, uae_u64 s) : data (d), size (s), limit ((s + 1) / 2 * 16) {}

	void refill ()
	{
		while (count <= 48) {
			uae_u32 hi = next < size ? data[next] : 0;
			uae_u32 lo = next + 1 < size ? data[next + 1] : 0;
			next += 2;
			buf |= (uae_u64)((hi << 8) | lo) << count;
			count += 16;
		}
	}

	void consume (unsigned int n)
	{
		buf >>= n;
		count -= n;
		used += n;
	}

	/* An n-bit field, n <= 16, first bit read is the least significant. */
	unsigned int read (unsigned int n)
	{
		refill ();
		unsigned int v = (unsigned int)(buf & ((1u << n) - 1));
		consume (n);
		return v;
	}

	bool overrun () const
	{
		return used > limit;
	}
};

/* ------------------------------------------------------------------ */
/* Canonical prefix codes                                              */
/* ------------------------------------------------------------------ */

constexpr int MAX_CODE_LEN = 16;
constexpr int FAST_BITS = 10;

template <int NSYMS>
struct prefix_code {
	uae_u32 first[MAX_CODE_LEN + 1];   /* first canonical code of each length */
	uae_u16 count[MAX_CODE_LEN + 1];   /* codes of each length */
	uae_u16 index[MAX_CODE_LEN + 1];   /* position in sorted[] of each length's first symbol */
	uae_u16 sorted[NSYMS];             /* symbols by (length, symbol) */
	/* Indexed by the next FAST_BITS stream bits: (symbol << 5) | length for
	 * codes up to FAST_BITS long, 0 where a longer code starts. */
	uae_u16 fast[1 << FAST_BITS];

	/* Builds the code; false unless the lengths form a complete code. */
	bool build (const uae_u8 *lengths)
	{
		memset (count, 0, sizeof count);
		for (int s = 0; s < NSYMS; s++) {
			if (lengths[s] > MAX_CODE_LEN)
				return false;
			count[lengths[s]]++;
		}
		int left = 1;
		for (int len = 1; len <= MAX_CODE_LEN; len++) {
			left <<= 1;
			left -= count[len];
			if (left < 0)
				return false;
		}
		if (left != 0)
			return false;

		uae_u32 code = 0;
		int idx = 0;
		uae_u16 pos[MAX_CODE_LEN + 1];
		for (int len = 1; len <= MAX_CODE_LEN; len++) {
			first[len] = code;
			index[len] = (uae_u16)idx;
			pos[len] = (uae_u16)idx;
			idx += count[len];
			code = (code + count[len]) << 1;
		}
		for (int s = 0; s < NSYMS; s++) {
			if (lengths[s])
				sorted[pos[lengths[s]]++] = (uae_u16)s;
		}

		memset (fast, 0, sizeof fast);
		for (int len = 1; len <= FAST_BITS; len++) {
			for (int k = 0; k < count[len]; k++) {
				const unsigned int c = first[len] + k;
				unsigned int rev = 0;
				for (int b = 0; b < len; b++)
					rev |= ((c >> b) & 1) << (len - 1 - b);
				const uae_u16 entry = (uae_u16)((sorted[index[len] + k] << 5) | len);
				for (unsigned int f = rev; f < (1u << FAST_BITS); f += 1u << len)
					fast[f] = entry;
			}
		}
		return true;
	}

	/* The first bit read is the code's most significant bit. */
	int decode (bit_reader &br) const
	{
		br.refill ();
		const uae_u16 e = fast[br.buf & ((1u << FAST_BITS) - 1)];
		if (e) {
			br.consume (e & 31);
			return e >> 5;
		}
		uae_u32 c = 0;
		for (int len = 1; len <= MAX_CODE_LEN; len++) {
			c = (c << 1) | (uae_u32)((br.buf >> (len - 1)) & 1);
			if (c >= first[len] && c - first[len] < count[len]) {
				br.consume (len);
				return sorted[index[len] + (c - first[len])];
			}
		}
		return -1; /* not reached for a complete code */
	}
};

/* ------------------------------------------------------------------ */
/* Pack mode 2 decompressor                                            */
/* ------------------------------------------------------------------ */

constexpr int MAIN_SYMBOLS = 768;
constexpr int PRETREE_SYMBOLS = 20;
constexpr int ALIGNED_SYMBOLS = 8;

const uae_u8 slot_extra[32] = {
	0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
	7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14
};
const uae_u32 slot_base[32] = {
	0, 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192,
	256, 384, 512, 768, 1024, 1536, 2048, 3072, 4096, 6144, 8192, 12288, 16384, 24576, 32768, 49152
};

struct lzx_decoder {
	bit_reader br;
	uae_u8 main_len[MAIN_SYMBOLS] = {};
	bool main_present = false;
	prefix_code<MAIN_SYMBOLS> main_code;
	prefix_code<ALIGNED_SYMBOLS> aligned_code;
	prefix_code<PRETREE_SYMBOLS> pretree;

	lzx_decoder (const uae_u8 *in, uae_u64 in_size) : br (in, in_size) {}

	/* One pass of main-code lengths (pass A: 0..255, pass B: 256..767);
	 * the lengths are differences mod 17 against the previous ones. */
	bool read_lengths (int start, int end)
	{
		const bool pass_a = start == 0;
		uae_u8 pl[PRETREE_SYMBOLS];
		for (int i = 0; i < PRETREE_SYMBOLS; i++)
			pl[i] = (uae_u8)br.read (4);
		if (!pretree.build (pl))
			return false;
		int i = start;
		while (i < end) {
			int c = pretree.decode (br);
			if (c < 0 || br.overrun ())
				return false;
			if (c <= 16) {
				main_len[i] = (uae_u8)((main_len[i] + 17 - c) % 17);
				i++;
				continue;
			}
			int run;
			uae_u8 value = 0;
			if (c == 17) {
				run = (pass_a ? 4 : 3) + (int)br.read (4);
			} else if (c == 18) {
				run = (pass_a ? 20 : 19) + (int)br.read (pass_a ? 5 : 6);
			} else {
				run = (pass_a ? 4 : 3) + (int)br.read (1);
				int c2 = pretree.decode (br);
				if (c2 < 0 || c2 > 16)
					return false;
				value = (uae_u8)((main_len[i] + 17 - c2) % 17);
			}
			if (br.overrun ())
				return false;
			if (run > end - i)
				run = end - i;
			memset (main_len + i, value, (size_t)run);
			i += run;
		}
		return true;
	}

	bool run (uae_u8 *out, uae_u64 total)
	{
		uae_u64 produced = 0;
		uae_u32 repeat = 1;
		while (produced < total) {
			const unsigned int type = br.read (3);
			if (type == 3) {
				uae_u8 al[ALIGNED_SYMBOLS];
				for (int i = 0; i < ALIGNED_SYMBOLS; i++)
					al[i] = (uae_u8)br.read (3);
				if (!aligned_code.build (al))
					return false;
			} else if (type == 1) {
				if (!main_present)
					return false;
			} else if (type != 2) {
				return false;
			}
			uae_u64 n = (uae_u64)br.read (8) << 16;
			n |= (uae_u64)br.read (8) << 8;
			n |= br.read (8);
			if (n == 0 || n > total - produced)
				return false;
			if (type != 1) {
				if (!read_lengths (0, 256) || !read_lengths (256, MAIN_SYMBOLS))
					return false;
				if (!main_code.build (main_len))
					return false;
				main_present = true;
			}
			if (br.overrun ())
				return false;

			const uae_u64 end = produced + n;
			while (produced < end) {
				const int s = main_code.decode (br);
				if (s < 0)
					return false;
				if (s < 256) {
					out[produced++] = (uae_u8)s;
				} else {
					const int k = s - 256;
					const int ps = k & 31;
					const int ls = k >> 5;
					uae_u32 d;
					if (type == 3 && slot_extra[ps] >= 3) {
						d = slot_base[ps] + 8 * br.read (slot_extra[ps] - 3u);
						const int a = aligned_code.decode (br);
						if (a < 0)
							return false;
						d += (uae_u32)a;
					} else {
						d = slot_base[ps] + br.read (slot_extra[ps]);
						if (d == 0)
							d = repeat;
					}
					repeat = d;
					const uae_u32 len = slot_base[ls] + 3 + br.read (slot_extra[ls]);
					if (len > end - produced)
						return false;
					/* Bytes before the group's start read as 0. */
					for (uae_u32 j = 0; j < len; j++, produced++)
						out[produced] = produced >= d ? out[produced - d] : 0;
				}
				if (br.overrun ())
					return false;
			}
		}
		return true;
	}
};

/* Decodes the group whose terminator is term into out (total bytes). */
bool decode_group (struct zfile *zf, const lzx_entry &term, uae_u8 *out, uae_u64 total)
{
	const uae_u64 packed = term.packed;
	/* zfile_size() is not set on every kind of zfile (archives opened by
	 * zfile_fopen_archive, for one); the end position is. */
	zfile_fseek (zf, 0, SEEK_END);
	const uae_s64 length = zfile_ftell (zf);
	if (length < 0 || term.next + packed > (uae_u64)length)
		return false;
	if (term.mode == MODE_STORED) {
		if (packed != total)
			return false;
		zfile_fseek (zf, (uae_s64)term.next, SEEK_SET);
		return zfile_fread (out, 1, (size_t)packed, zf) == packed;
	}
	/* Every symbol uses at least one bit and yields at most MAX_MATCH bytes. */
	if (total > ((packed + 1) / 2 * 16) * MAX_MATCH)
		return false;
	uae_u8 *in = xmalloc (uae_u8, (size_t)packed);
	if (!in)
		return false;
	zfile_fseek (zf, (uae_s64)term.next, SEEK_SET);
	bool ok = zfile_fread (in, 1, (size_t)packed, zf) == packed;
	if (ok) {
		lzx_decoder *dec = new (std::nothrow) lzx_decoder (in, packed);
		ok = dec && dec->run (out, total);
		delete dec;
	}
	xfree (in);
	return ok;
}

/* A new in-memory zfile with zn's bytes from the group output. */
struct zfile *member_zfile (struct zfile *archive, struct znode *zn, const uae_u8 *group, uae_u64 total)
{
	const uae_u64 size = (uae_u64)zn->size;
	if ((uae_u64)zn->offset2 + size > total)
		return nullptr;
	const uae_u8 *data = group + zn->offset2;
	const uae_u32 crc = (uae_u32)crc32 (0L, data, (uInt)size);
	if (crc != zn->method)
		write_log (_T("LZX CRC mismatch %s: expected %08x, got %08x\n"), zn->name, zn->method, crc);
	struct zfile *f = zfile_fopen_empty (archive, zn->name, size);
	if (!f)
		return nullptr;
	if (size && zfile_fwrite (data, (size_t)size, 1, f) != 1) {
		zfile_fclose (f);
		return nullptr;
	}
	zfile_fseek (f, 0, SEEK_SET);
	return f;
}

} // namespace

struct zvolume *archive_directory_lzx (struct zfile *in_file)
{
	uae_u8 header[ARCHIVE_HEADER_SIZE];

	zfile_fseek (in_file, 0, SEEK_SET);
	if (zfile_fread (header, 1, sizeof header, in_file) != sizeof header || memcmp (header, "LZX", 3) != 0)
		return nullptr;

	struct zvolume *zv = zvolume_alloc (in_file, ArchiveFormatLZX, nullptr, nullptr);
	if (!zv)
		return nullptr;

	const size_t volume_path_len = zv->root.fullname ? uaetcslen (zv->root.fullname) : 0;
	std::vector<struct znode *> group;
	uae_u64 group_total = 0;
	uae_u64 pos = ARCHIVE_HEADER_SIZE;
	lzx_entry e;

	for (;;) {
		if (pos > UINT_MAX) {
			write_log (_T("LZX: archive too large, listing stopped at offset %llu\n"), (unsigned long long)pos);
			break;
		}
		const entry_read r = read_entry (in_file, pos, e);
		if (r == entry_read::end)
			break;
		if (r != entry_read::ok) {
			write_log (_T("LZX: %s entry header at offset %llu, listing stopped\n"),
				r == entry_read::truncated ? _T("truncated") : _T("bad CRC in"), (unsigned long long)pos);
			break;
		}

		struct znode *zn = nullptr;
		/* The znode layer builds full paths in MAX_DPATH buffers. */
		if (strlen (e.name) + volume_path_len + 16 >= MAX_DPATH) {
			write_log (_T("LZX: skipping a member with a %zu-byte name\n"), strlen (e.name));
		} else {
			struct zarchive_info zai;
			memset (&zai, 0, sizeof zai);
			zai.name = e.name;
			zai.size = e.unpacked;
			zai.flags = lzx_flags (e.header[0]);
			if (e.comment_len > 0 && e.comment[0])
				zai.comment = e.comment;
			zai.tv.tv_sec = lzx_mtime (get_be32 (e.header + 18));
			zai.tv.tv_usec = 0;
			zn = zvolume_addfile_abs (zv, &zai);
		}
		if (zn) {
			zn->offset = 0;
			zn->packedsize = 0;
			zn->offset2 = clamp_u32 (group_total);
			zn->method = e.data_crc;
			group.push_back (zn);
		}
		group_total += e.unpacked;

		if (e.packed == 0) {
			pos = e.next;
			continue;
		}
		for (struct znode *member : group) {
			member->offset = (unsigned int)pos;
			member->packedsize = clamp_u32 (group_total);
		}
		group.clear ();
		group_total = 0;
		pos = e.next + e.packed;
		if (zfile_fseek (in_file, (uae_s64)pos, SEEK_SET)) {
			write_log (_T("LZX: packed data past the end of the archive, listing stopped\n"));
			break;
		}
	}
	return zv;
}

struct zfile *archive_access_lzx (struct znode *zn)
{
	if (!zn || zn->type != ZNODE_FILE)
		return nullptr;
	if (zn->f)
		return zfile_dup (zn->f);

	struct zvolume *zv = zn->volume;
	struct zfile *zf = zv->archive;

	if (zn->size == 0)
		return zfile_fopen_empty (zf, zn->name, 0);
	if (zn->offset == 0) {
		write_log (_T("LZX: no packed data for %s (truncated archive)\n"), zn->name);
		return nullptr;
	}

	const uae_u64 total = zn->packedsize;
	lzx_entry term;
	if (total > INT_MAX || (uae_u64)zn->size > total
		|| read_entry (zf, zn->offset, term) != entry_read::ok || term.packed == 0) {
		write_log (_T("LZX corrupt compressed data %s\n"), zn->name);
		return nullptr;
	}
	if (term.mode != MODE_COMPRESSED && term.mode != MODE_STORED) {
		write_log (_T("LZX unsupported pack mode %d for %s\n"), term.mode, zn->name);
		return nullptr;
	}

	uae_u8 *out = xmalloc (uae_u8, (size_t)total);
	if (!out || !decode_group (zf, term, out, total)) {
		write_log (_T("LZX corrupt compressed data %s\n"), zn->name);
		xfree (out);
		return nullptr;
	}

	struct zfile *result = member_zfile (zf, zn, out, total);
	if (!result) {
		write_log (_T("LZX corrupt compressed data %s\n"), zn->name);
	} else {
		/* Callers open every member in turn: keep the rest of the group.
		 * Nodes are created in archive order, so a group's file nodes are
		 * adjacent in the volume's node chain (only directory nodes can sit
		 * between them): walk just this group, not the whole volume. */
		struct znode *first = zn;
		for (struct znode *p = zn->prev; p && p != &zv->root; p = p->prev) {
			if (p->type != ZNODE_FILE)
				continue;
			if (p->offset != zn->offset)
				break;
			first = p;
		}
		for (struct znode *other = first; other; other = other->next) {
			if (other->type != ZNODE_FILE)
				continue;
			if (other->offset != zn->offset)
				break;
			if (other != zn && !other->f)
				other->f = member_zfile (zf, other, out, total);
		}
	}
	xfree (out);
	return result;
}
