// Lists and extracts LZX archives through the real zfile archive code
// (src/zfile.cpp, src/zfile_archive.cpp, src/archivers/lzx/lzx.cpp) and
// compares every member with a manifest of the original files.
//
// Usage: lzx_archive_test MANIFEST "YYYY-MM-DD HH:MM:SS" ARCHIVE...
// MANIFEST lines are "<sha256> <size> <relative path>". Every archive must
// hold exactly those files, all dated with the given local time and with
// the default protection bits (flags 0) and no comments. Hand-built
// archives then cover stored groups, attributes, comments, odd entries and
// the DSQ path.
//
// Only the LZX path runs for real; the other formats zfile.cpp links
// against are stubbed below.

#include "sysconfig.h"
#include "sysdeps.h"
#include "zfile.h"
#include "zarchive.h"
#include "fsdb.h"
#include "disk.h"
#include "diskutil.h"
#include "fdi2raw.h"
#include "uae/caps.h"
#include "archivers/dms/pfile.h"
#include "archivers/wrp/warp.h"
#include "7z/Sha256.h"

#include <dirent.h>
#include <unistd.h>
#include <zlib.h>
#include <sys/stat.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

// ---------------------------------------------------------------------
// Stubs for what zfile.cpp links against outside the archive code.

static std::string g_log;

void write_log(const char *format, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, format);
	vsnprintf(buf, sizeof buf, format, ap);
	va_end(ap);
	g_log += buf;
}

std::string home_dir;

struct my_opendir_s {
	DIR *dir;
};

my_opendir_s *my_opendir(const TCHAR *name)
{
	DIR *d = opendir(name);
	if (!d)
		return nullptr;
	return new my_opendir_s{ d };
}
my_opendir_s *my_opendir(const TCHAR *name, const TCHAR *)
{
	return my_opendir(name);
}
void my_closedir(my_opendir_s *d)
{
	if (d) {
		closedir(d->dir);
		delete d;
	}
}
int my_readdir(my_opendir_s *d, TCHAR *name)
{
	struct dirent *e = readdir(d->dir);
	if (!e)
		return 0;
	strcpy(name, e->d_name);
	return 1;
}
bool my_stat(const TCHAR *name, struct mystat *st) noexcept
{
	struct stat s;
	if (stat(name, &s) != 0)
		return false;
	st->size = s.st_size;
	st->mode = S_ISDIR(s.st_mode) ? FILEFLAG_DIR : 0;
	st->mtime.tv_sec = s.st_mtime;
	st->mtime.tv_usec = 0;
	return true;
}
bool my_existsfile2(const TCHAR *name)
{
	struct stat s;
	return stat(name, &s) == 0 && S_ISREG(s.st_mode);
}
bool my_existsdir(const TCHAR *name)
{
	struct stat s;
	return stat(name, &s) == 0 && S_ISDIR(s.st_mode);
}
FILE *my_opentext(const TCHAR *name) noexcept
{
	return fopen(name, "r");
}
void amiga_to_timeval(struct mytimeval *tv, int, int, int, int)
{
	tv->tv_sec = 0;
	tv->tv_usec = 0;
}
int DISK_history_add(const TCHAR *, int, int, int) { return 0; }
int isamigatrack(uae_u16 *, uae_u8 *, int, uae_u8 *, uae_u8 *, int, int *) { return 0; }
int ispctrack(uae_u16 *, uae_u8 *, int, uae_u8 *, uae_u8 *, int, int *) { return 0; }
int caps_loadimage(struct zfile *, int, int *) { return 0; }
int caps_loadtrack(uae_u16 *, uae_u16 *, int, int, int *, int *, int *, int *, bool) { return 0; }
void caps_unloadimage(int) {}
USHORT DMS_Process_File(struct zfile *, struct zfile *, USHORT, USHORT, USHORT, USHORT, int, struct zfile **) { return ERR_NOTDMS; }
FDI *fdi2raw_header(struct zfile *) { return nullptr; }
void fdi2raw_header_free(FDI *) {}
int fdi2raw_loadtrack(FDI *, uae_u16 *, uae_u16 *, int, int *, int *, int *, int) { return 0; }
int fdi2raw_get_last_track(FDI *) { return 0; }
struct zfile *unwarp(struct zfile *) { return nullptr; }
struct zvolume *archive_directory_lha(struct zfile *) { return nullptr; }
struct zfile *archive_access_lha(struct znode *) { return nullptr; }
void archive_close_lha(void *) {}

// ---------------------------------------------------------------------

static int failures;

#define CHECK(cond, ...) \
	do { \
		if (!(cond)) { \
			failures++; \
			fprintf(stderr, "FAIL (%s:%d): ", __FILE__, __LINE__); \
			fprintf(stderr, __VA_ARGS__); \
			fputc('\n', stderr); \
		} \
	} while (0)

struct original {
	std::string sha;
	uae_s64 size;
};

static std::map<std::string, original> manifest;
static uae_s64 expected_mtime;

static std::string sha256_hex(const uae_u8 *data, size_t size)
{
	CSha256 ctx;
	Byte digest[SHA256_DIGEST_SIZE];
	Sha256_Init(&ctx);
	Sha256_Update(&ctx, data, size);
	Sha256_Final(&ctx, digest);
	std::string hex;
	char b[3];
	for (Byte d : digest) {
		snprintf(b, sizeof b, "%02x", d);
		hex += b;
	}
	return hex;
}

// Reads the zfile from its current position, which must be 0, to the end.
static std::string zfile_sha(struct zfile *f, uae_s64 *size)
{
	*size = zfile_size(f);
	if (zfile_ftell(f) != 0)
		return "position not 0";
	std::vector<uae_u8> data((size_t)*size + 1);
	size_t got = zfile_fread(data.data(), 1, (size_t)*size + 1, f);
	if ((uae_s64)got != *size)
		return "short read";
	return sha256_hex(data.data(), got);
}

static std::string base_name(const std::string &path)
{
	size_t p = path.find_last_of("/\\");
	return p == std::string::npos ? path : path.substr(p + 1);
}

static std::vector<uae_u8> read_file(const char *path)
{
	std::ifstream in(path, std::ios::binary);
	return std::vector<uae_u8>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

// The volume's file nodes in archive order, with paths relative to it.
static std::vector<std::pair<std::string, znode *>> file_nodes(zvolume *zv)
{
	std::vector<std::pair<std::string, znode *>> files;
	const size_t root_len = strlen(zv->root.name);
	for (znode *zn = zv->root.next; zn; zn = zn->next) {
		if (zn->type != ZNODE_FILE)
			continue;
		std::string rel = zn->fullname + root_len + 1;
		for (char &c : rel)
			if (c == FSDB_DIR_SEPARATOR)
				c = '/';
		files.emplace_back(rel, zn);
	}
	return files;
}

// Listing, metadata and every member through archive_directory_lzx and
// archive_access_lzx, twice (the second time from the group cache).
static void check_volume(const char *path)
{
	struct zfile *zf = zfile_fopen(path, _T("rb"), 0);
	CHECK(zf, "%s: cannot open", path);
	if (!zf)
		return;
	zvolume *zv = archive_directory_lzx(zf);
	CHECK(zv, "%s: not listed as an LZX archive", path);
	if (!zv) {
		zfile_fclose(zf);
		return;
	}
	auto files = file_nodes(zv);
	CHECK(files.size() == manifest.size(), "%s: %zu members listed, %zu expected", path, files.size(), manifest.size());
	CHECK(archive_access_lzx(&zv->root) == nullptr, "%s: the volume root opened as a file", path);
	for (int pass = 0; pass < 2; pass++) {
		std::set<std::string> seen;
		for (auto &[rel, zn] : files) {
			auto it = manifest.find(rel);
			CHECK(it != manifest.end(), "%s: unexpected member '%s'", path, rel.c_str());
			if (it == manifest.end())
				continue;
			CHECK(seen.insert(rel).second, "%s: '%s' listed twice", path, rel.c_str());
			CHECK(zn->size == it->second.size, "%s: '%s' listed with size %lld", path, rel.c_str(), (long long)zn->size);
			CHECK(zn->flags == 0, "%s: '%s' has flags %d", path, rel.c_str(), zn->flags);
			CHECK(zn->comment == nullptr, "%s: '%s' has a comment", path, rel.c_str());
			CHECK(zn->mtime.tv_sec == expected_mtime, "%s: '%s' dated %lld, expected %lld", path, rel.c_str(),
				(long long)zn->mtime.tv_sec, (long long)expected_mtime);
			struct zfile *f = archive_access_lzx(zn);
			CHECK(f, "%s: '%s' did not extract (pass %d)", path, rel.c_str(), pass);
			if (!f)
				continue;
			CHECK(!strcmp(zfile_getname(f), zn->name), "%s: '%s' extracted as '%s'", path, rel.c_str(), zfile_getname(f));
			uae_s64 size;
			std::string sha = zfile_sha(f, &size);
			CHECK(size == it->second.size && sha == it->second.sha, "%s: '%s' extracted wrong (%lld bytes, %s, pass %d)",
				path, rel.c_str(), (long long)size, sha.c_str(), pass);
			zfile_fclose(f);
		}
	}
	// Directory nodes are not files.
	for (znode *zn = zv->root.next; zn; zn = zn->next) {
		if (zn->type != ZNODE_FILE)
			CHECK(archive_access_lzx(zn) == nullptr, "%s: directory '%s' opened as a file", path, zn->name);
	}
	zfile_fclose_archive(zv);
	zfile_fclose(zf);
}

// The archive scan the ROM and disk scanners use (zfile_zopen ->
// archive_access_scan -> archive_getzfile on every file node).
struct scan_state {
	const char *path;
	std::map<std::string, int> found;
};

static int scan_member(struct zfile *f, void *user)
{
	auto *st = static_cast<scan_state *>(user);
	std::string name = zfile_getname(f);
	uae_s64 size;
	std::string sha = zfile_sha(f, &size);
	bool match = false;
	for (auto &[rel, orig] : manifest) {
		if (base_name(rel) == name && orig.size == size && orig.sha == sha)
			match = true;
	}
	CHECK(match, "%s: scanned member '%s' (%lld bytes) does not match an original", st->path, name.c_str(), (long long)size);
	st->found[name]++;
	return 0;
}

static void check_scan(const char *path)
{
	scan_state st{ path, {} };
	CHECK(zfile_zopen(path, scan_member, &st), "%s: zfile_zopen failed", path);
	for (auto &[rel, orig] : manifest)
		CHECK(st.found[base_name(rel)] == 1, "%s: scan returned '%s' %d times", path, rel.c_str(), st.found[base_name(rel)]);
}

// Members opened by path on a mounted volume, as archive filesystems and
// WHDLoad boot do. zfile_open_archive keeps what it returns in zn->f, so
// the volume owns it.
static void check_open_archive(const char *path)
{
	zvolume *zv = zfile_fopen_archive(path);
	CHECK(zv, "%s: zfile_fopen_archive failed", path);
	if (!zv)
		return;
	for (int pass = 0; pass < 2; pass++) {
		for (auto &[rel, orig] : manifest) {
			std::string member = std::string(path) + FSDB_DIR_SEPARATOR_S + rel;
			g_log.clear();
			struct zfile *f = zfile_open_archive(member.c_str(), 0);
			CHECK(f, "%s: zfile_open_archive failed: %s", member.c_str(), g_log.c_str());
			if (!f)
				continue;
			uae_s64 size;
			std::string sha = zfile_sha(f, &size);
			CHECK(size == orig.size && sha == orig.sha, "%s: opened wrong (%lld bytes, pass %d)", member.c_str(),
				(long long)size, pass);
		}
	}
	zfile_fclose_archive(zv);
}

// Opens every member of an in-memory copy. Returns the number of members
// listed. Data that comes back must be right, unless the decoder logged a
// CRC mismatch for that member (when its group was decoded, possibly for
// an earlier member).
static size_t access_all(const std::vector<uae_u8> &bytes, const char *what)
{
	struct zfile *zf = zfile_fopen_data(_T("damaged.lzx"), bytes.size(), bytes.data());
	zvolume *zv = archive_directory_lzx(zf);
	size_t listed = 0;
	if (bytes.size() < 10 || memcmp(bytes.data(), "LZX", 3) != 0)
		CHECK(!zv, "%s: listed a file that is not an LZX archive", what);
	else
		CHECK(zv, "%s: an archive with the LZX magic was not listed", what);
	if (zv) {
		g_log.clear();
		for (auto &[rel, zn] : file_nodes(zv)) {
			listed++;
			struct zfile *f = archive_access_lzx(zn);
			if (!f)
				continue;
			auto it = manifest.find(rel);
			uae_s64 size;
			std::string sha = zfile_sha(f, &size);
			bool right = it != manifest.end() && size == it->second.size && sha == it->second.sha;
			std::string warning = std::string("CRC mismatch ") + zn->name + ":";
			CHECK(right || g_log.find(warning) != std::string::npos,
				"%s: '%s' came back wrong without a CRC warning", what, rel.c_str());
			zfile_fclose(f);
		}
		zfile_fclose_archive(zv);
	}
	zfile_fclose(zf);
	return listed;
}

// Truncated and corrupted copies: no crash, no hang, a clean failure.
static void check_damage(const char *path)
{
	const std::vector<uae_u8> good = read_file(path);
	char what[256];

	snprintf(what, sizeof what, "%s complete in memory", path);
	CHECK(access_all(good, what) == manifest.size(), "%s: incomplete listing", what);

	std::vector<size_t> cuts;
	for (size_t n = 0; n <= 80 && n < good.size(); n++)
		cuts.push_back(n);
	for (size_t n = 81; n < good.size(); n += 4099)
		cuts.push_back(n);
	cuts.push_back(good.size() - 1);
	for (size_t n : cuts) {
		std::vector<uae_u8> cut(good.begin(), good.begin() + n);
		snprintf(what, sizeof what, "%s cut at %zu", path, n);
		size_t listed = access_all(cut, what);
		if (n == 10)
			CHECK(listed == 0, "%s: listed %zu members", what, listed);
	}

	unsigned int seed = 12345;
	for (int i = 0; i < 300; i++) {
		std::vector<uae_u8> bad = good;
		seed = seed * 1103515245 + 12345;
		size_t at = 10 + (seed >> 8) % (bad.size() - 10);
		seed = seed * 1103515245 + 12345;
		bad[at] ^= (uae_u8)(1 + (seed >> 16) % 255);
		snprintf(what, sizeof what, "%s byte %zu corrupted", path, at);
		access_all(bad, what);
	}
}

// Hand-built archives for what the LZX 1.21 archives do not show: stored
// (pack mode 0) groups, attributes, comments, entries without a node,
// unsupported pack modes, entries after the last group and DSQ images.
static void put_le32(uae_u8 *p, uae_u32 v)
{
	for (int i = 0; i < 4; i++)
		p[i] = (uae_u8)(v >> (8 * i));
}

static std::vector<uae_u8> built_archive()
{
	return { 'L', 'Z', 'X', 0, 0x0c, 0, 0x0a, 4, 0, 0 };
}

// An entry header; packed != 0 ends a group, whose data the caller appends.
static void add_entry(std::vector<uae_u8> &a, const std::string &name, const std::string &data, uae_u32 packed,
	int mode = 0, uae_u8 attr = 0x0f, const std::string &comment = "")
{
	uae_u8 h[31] = {};
	h[0] = attr;
	put_le32(h + 2, (uae_u32)data.size());
	put_le32(h + 6, packed);
	h[10] = 0x0a;
	h[11] = (uae_u8)mode;
	h[14] = (uae_u8)comment.size();
	h[15] = 0x0a;
	h[18] = 0xdc; h[19] = 0xa9; h[20] = 0x20; h[21] = 0xc9; // 27 Oct 1990 18:03:09
	put_le32(h + 22, (uae_u32)crc32(0L, (const Bytef *)data.data(), (uInt)data.size()));
	h[30] = (uae_u8)name.size();
	uLong crc = crc32(0L, h, sizeof h);
	crc = crc32(crc, (const Bytef *)name.data(), (uInt)name.size());
	crc = crc32(crc, (const Bytef *)comment.data(), (uInt)comment.size());
	put_le32(h + 26, (uae_u32)crc);
	a.insert(a.end(), h, h + sizeof h);
	a.insert(a.end(), name.begin(), name.end());
	a.insert(a.end(), comment.begin(), comment.end());
}

static void add_data(std::vector<uae_u8> &a, const std::string &data)
{
	a.insert(a.end(), data.begin(), data.end());
}

// The member's bytes, or "<null>" if it does not open.
static std::string member_text(znode *zn)
{
	struct zfile *f = archive_access_lzx(zn);
	if (!f)
		return "<null>";
	std::string s((size_t)zfile_size(f), 0);
	if (zfile_ftell(f) != 0 || zfile_fread(&s[0], 1, s.size(), f) != s.size())
		s = "<bad read>";
	zfile_fclose(f);
	return s;
}

static void check_built_archives()
{
	std::vector<uae_u8> a = built_archive();
	add_entry(a, "info.info", "abc", 0, 0, 0x07, "hello");
	add_entry(a, "h", "defg", 7, 0, 0x2f, std::string("\0x", 2));
	add_data(a, "abcdefg");
	// Group members without a node (a directory-like name, an empty name)
	// still take their place in the group.
	add_entry(a, "x", "12", 0);
	add_entry(a, "y/", "56", 0);
	add_entry(a, "", "34", 6);
	add_data(a, "125634");
	add_entry(a, "mode5", "q", 1, 5);
	add_data(a, "q");
	add_entry(a, "short", "qq", 1);
	add_data(a, "q");
	add_entry(a, "tail0", "", 0);
	add_entry(a, "tail1", "abc", 0);

	struct zfile *zf = zfile_fopen_data(_T("built.lzx"), a.size(), a.data());
	zvolume *zv = archive_directory_lzx(zf);
	CHECK(zv, "hand-built archive not listed");
	if (!zv) {
		zfile_fclose(zf);
		return;
	}
	std::map<std::string, znode *> nodes;
	std::string order;
	for (auto &[rel, zn] : file_nodes(zv)) {
		nodes[rel] = zn;
		order += rel + " ";
	}
	CHECK(order == "info.info h x mode5 short tail0 tail1 ", "hand-built archive listed as '%s'", order.c_str());
	if (nodes.size() == 7) {
		znode *info = nodes["info.info"], *h = nodes["h"];
		CHECK(info->flags == 0x02 && h->flags == 0x80, "attributes 0x07, 0x2f gave flags 0x%02x, 0x%02x", info->flags, h->flags);
		CHECK(info->comment && !strcmp(info->comment, "hello"), "comment 'hello' not kept");
		CHECK(!h->comment, "a comment starting with NUL was kept");
		CHECK(member_text(info) == "abc" && member_text(h) == "defg", "stored group extracted wrong");
		CHECK(member_text(nodes["x"]) == "12", "group with nodeless members extracted wrong");
		g_log.clear();
		CHECK(member_text(nodes["mode5"]) == "<null>" && g_log.find("unsupported pack mode 5") != std::string::npos,
			"pack mode 5 not rejected with a log line");
		CHECK(member_text(nodes["short"]) == "<null>", "stored group with packed size != total opened");
		CHECK(member_text(nodes["tail0"]).empty(), "empty entry after the last group did not open empty");
		CHECK(member_text(nodes["tail1"]) == "<null>", "non-empty entry after the last group opened");
	}
	zfile_fclose_archive(zv);
	zfile_fclose(zf);

	// DSQ (zfile.cpp): a .dsq that is an LZX archive whose first member is a
	// PKD image becomes an ADF. One whose first member is empty, inside a
	// directory, or a PKD image that is malformed or shorter than its header
	// says comes back unchanged.
	std::string pkd("PKD\x11", 4);
	pkd.resize(32);
	pkd[7] = 2;  // blocks
	pkd[10] = 2; // block size 512
	pkd[18] = 22; // 11 sectors per track, times 2
	for (int i = 0; i < 1024; i++)
		pkd += (char)(i & 255);
	std::string pkd13_cut("PKD\x13", 4); // header cut before its 52 bytes
	pkd13_cut.resize(40);
	pkd13_cut[7] = 2;
	pkd13_cut[10] = 2;
	pkd13_cut[18] = 22;
	std::string pkd13_bitmap = pkd13_cut; // 1760-block bitmap, no data
	pkd13_bitmap.resize(62);
	pkd13_bitmap[6] = (char)(1760 >> 8);
	pkd13_bitmap[7] = (char)(1760 & 255);
	pkd13_bitmap[51] = 1;
	std::string pkd_short = pkd.substr(0, 32 + 600); // 2 sectors declared, 600 bytes
	std::string pkd_nosectors = pkd;
	pkd_nosectors[18] = 0;
	struct dsq_case {
		const char *what;
		const char *first;
		std::string data;
	} cases[] = {
		{ "PKD image", "disk", pkd },
		{ "empty first member", "empty", "" },
		{ "first member in a directory", "dir/disk", pkd },
		{ "truncated PKD header", "disk", pkd13_cut },
		{ "PKD with a bitmap and no sectors", "disk", pkd13_bitmap },
		{ "PKD sectors past the data", "disk", pkd_short },
		{ "PKD with 0 sectors per track", "disk", pkd_nosectors },
	};
	const char *tmp = getenv("TMPDIR");
	std::string dsq_path = std::string(tmp && *tmp ? tmp : "/tmp") + "/lzx_archive_test." + std::to_string(getpid()) + ".dsq";
	for (const dsq_case &c : cases) {
		const bool converts = &c == &cases[0];
		std::vector<uae_u8> d = built_archive();
		add_entry(d, c.first, c.data, 0);
		add_entry(d, "other", "x", (uae_u32)c.data.size() + 1);
		add_data(d, c.data + "x");
		FILE *out = fopen(dsq_path.c_str(), "wb");
		CHECK(out && fwrite(d.data(), 1, d.size(), out) == d.size(), "cannot write %s", dsq_path.c_str());
		if (out)
			fclose(out);
		struct zfile *z = zfile_fopen(dsq_path.c_str(), _T("rb"), ZFD_UNPACK);
		CHECK(z, "DSQ %s: did not open", c.what);
		if (z) {
			uae_u8 head[4] = {};
			zfile_fseek(z, 0, SEEK_SET);
			zfile_fread(head, 1, sizeof head, z);
			if (converts)
				CHECK(!strcmp(zfile_getname(z), "disk.adf") && zfile_size(z) == 1760 * 512 && head[0] == 0 && head[3] == 3,
					"DSQ not converted (%s, %lld bytes)", zfile_getname(z), (long long)zfile_size(z));
			else
				CHECK(zfile_size(z) == (uae_s64)d.size() && !memcmp(head, "LZX", 3), "DSQ %s: file changed", c.what);
			zfile_fclose(z);
		}
	}
	remove(dsq_path.c_str());
}

int main(int argc, char **argv)
{
	if (argc < 4) {
		fprintf(stderr, "usage: %s MANIFEST \"YYYY-MM-DD HH:MM:SS\" ARCHIVE...\n", argv[0]);
		return 2;
	}
	std::ifstream in(argv[1]);
	std::string sha, rel;
	long long size;
	while (in >> sha >> size >> rel)
		manifest[rel] = original{ sha, size };
	if (manifest.empty()) {
		fprintf(stderr, "%s: empty manifest\n", argv[1]);
		return 2;
	}
	struct tm tm;
	memset(&tm, 0, sizeof tm);
	if (sscanf(argv[2], "%d-%d-%d %d:%d:%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec) != 6) {
		fprintf(stderr, "bad date '%s'\n", argv[2]);
		return 2;
	}
	tm.tm_year -= 1900;
	tm.tm_mon -= 1;
	expected_mtime = mktime(&tm);

	check_built_archives();
	for (int i = 3; i < argc; i++) {
		check_volume(argv[i]);
		check_scan(argv[i]);
		check_open_archive(argv[i]);
		check_damage(argv[i]);
	}
	if (failures) {
		fprintf(stderr, "lzx archive test: %d failure(s)\n", failures);
		return 1;
	}
	printf("lzx archive test: %zu members in %d archive(s) match\n", manifest.size(), argc - 3);
	return 0;
}
