#include "config.h"
#include "sysconfig.h"
#include "sysdeps.h"

#include "fsdb.h"
#include "options.h"
#include "filesys.h"
#include "zfile.h"
#include <unistd.h>
#include <list>
#include <dirent.h>
#include <iconv.h>

#include "fsdb_host.h"
#include "uae.h"

struct my_opendir_s {
	DIR* dir{};
};

struct my_openfile_s {
	int fd;
	char* path;
};

#ifdef AMIBERRY
void utf8_to_latin1_string(std::string& input, std::string& output)
{
	std::vector<char> in_buf(input.begin(), input.end());
	char* src_ptr = &in_buf[0];
	size_t src_size = input.size();
	std::vector<char> buf(1024);
	std::string dst;
	
	auto* iconv_ = iconv_open("ISO-8859-1", "UTF-8");
	
	while (0 < src_size) {
		char* dst_ptr = &buf[0];
		size_t dst_size = buf.size();
		size_t res = ::iconv(iconv_, &src_ptr, &src_size, &dst_ptr, &dst_size);
		if (res == (size_t)-1) {
			if (errno == E2BIG) {
				// ignore this error
			}
			else {
				// skip character
				++src_ptr;
				--src_size;
			}
		}
		dst.append(&buf[0], buf.size() - dst_size);
	}
	dst.swap(output);
	iconv_close(iconv_);
}

std::string iso_8859_1_to_utf8(std::string& str)
{
	string str_out;
	for (auto it = str.begin(); it != str.end(); ++it)
	{
		uint8_t ch = *it;
		if (ch < 0x80) {
			str_out.push_back(ch);
		}
		else {
			str_out.push_back(0xc0 | ch >> 6);
			str_out.push_back(0x80 | (ch & 0x3f));
		}
	}
	return str_out;
}
#endif

string prefix_with_application_directory_path(string currentpath)
{
#ifdef ANDROID
	return getenv("EXTERNAL_FILES_DIR") + ("/" + currentpath);
#else
	return currentpath;
#endif
}

int my_setcurrentdir(const TCHAR* curdir, TCHAR* oldcur)
{
	const auto ret = 0;
	if (oldcur)
		getcwd(oldcur, MAX_DPATH);
	if (curdir)
		chdir(curdir);
	return ret;
}

bool my_chmod(const TCHAR* name, uae_u32 mode)
{
	// Note: only used to set or clear write protect on disk file

	// get current state
	struct stat st{};
	auto input = string(name);
	auto output = iso_8859_1_to_utf8(input);
	if (stat(output.c_str(), &st) == -1) {
		write_log("my_chmod: stat on file %s failed\n", output.c_str());
		return false;
	}

	if (mode & FILEFLAG_WRITE)
		// set write permission
		st.st_mode |= S_IWUSR;
	else
		// clear write permission
		st.st_mode &= ~(S_IWUSR);
	chmod(output.c_str(), st.st_mode);

	stat(output.c_str(), &st);
	auto newmode = 0;
	if (st.st_mode & S_IRUSR) {
		newmode |= FILEFLAG_READ;
	}
	if (st.st_mode & S_IWUSR) {
		newmode |= FILEFLAG_WRITE;
	}

	return (mode == newmode);
}

bool my_stat(const TCHAR* name, struct mystat* statbuf)
{
	struct stat st{};

	auto input = string(name);
	auto output = iso_8859_1_to_utf8(input);
	if (stat(output.c_str(), &st) == -1) {
		write_log("my_stat: stat on file %s failed\n", output.c_str());
		return false;
	}

	statbuf->size = st.st_size;
	statbuf->mode = 0;
	if (st.st_mode & S_IRUSR) {
		statbuf->mode |= FILEFLAG_READ;
	}
	if (st.st_mode & S_IWUSR) {
		statbuf->mode |= FILEFLAG_WRITE;
	}
	statbuf->mtime.tv_sec = st.st_mtime;
	statbuf->mtime.tv_usec = 0;

	return true;
}

bool compare_nocase(const std::string& first, const std::string& second)
{
	unsigned int i = 0;
	while ((i < first.length()) && (i < second.length()))
	{
		if (tolower(first[i]) < tolower(second[i])) return true;
		else if (tolower(first[i]) > tolower(second[i])) return false;
		++i;
	}
	return (first.length() < second.length());
}

struct my_opendir_s* my_opendir(const TCHAR* name, const TCHAR* mask)
{
	auto* mod = xmalloc(struct my_opendir_s, 1);
	if (!mod)
		return NULL;
	mod->dir = opendir(name);
	if (!mod->dir) {
		write_log("my_opendir %s failed\n", name);
		xfree(mod);
		return NULL;
	}
	return mod;
}

struct my_opendir_s* my_opendir(const TCHAR* name) {
	return my_opendir(name, _T("*.*"));
}

void my_closedir(struct my_opendir_s* mod)
{
	if (mod) {
		closedir(mod->dir);
		xfree(mod);
	}
}

int my_readdir(struct my_opendir_s* mod, TCHAR* name)
{
	if (!mod)
		return 0;
	
	while (true)
	{
		auto* entry = readdir(mod->dir);
		if (entry == NULL)
			return 0;
		
		auto* result = entry->d_name;
		const int len = strlen(result);

		if (strcasecmp(result, "_UAEFSDB.___") == 0) {
			continue;
		}
		if (strcasecmp(result, "Thumbs.db") == 0) {
			continue;
		}
		if (strcasecmp(result, ".DS_Store") == 0) {
			continue;
		}
		if (strcasecmp(result, "UAEFS.ini") == 0) {
			continue;
		}
		if (len > 5 && strncmp(result + len - 5, ".uaem", 5) == 0) {
			// ignore metadata / attribute files, obviously
			continue;
		}

		auto string_input = string(result);
		string string_output;
		utf8_to_latin1_string(string_input, string_output);
		//write_log("Original: %s - Converted: %s\n", string_input.c_str(), string_output.c_str());
		_tcscpy(name, string_output.c_str());

		return 1;
	}
}

int my_existsfile(const char* name)
{
	struct stat st {};
	auto input = string(name);
	auto output = iso_8859_1_to_utf8(input);
	if (lstat(output.c_str(), &st) == -1)
	{
		return 0;
	}
	if (!S_ISDIR(st.st_mode))
		return 1;
	return 0;
}

int my_existsdir(const char* name)
{
	struct stat st {};
	auto input = string(name);
	auto output = iso_8859_1_to_utf8(input);	
	if (lstat(output.c_str(), &st) == -1)
	{
		return 0;
	}
	if (S_ISDIR(st.st_mode))
		return 1;
	return 0;
}

uae_s64 my_fsize(struct my_openfile_s* mos)
{
	struct stat st {};
	if (fstat(mos->fd, &st) == -1) {
		write_log("my_fsize: fstat on file %s failed\n", mos->path);
		return -1;
	}
	return st.st_size;
}

int my_getvolumeinfo(const char* root)
{
	struct stat st {};
	auto ret = 0;

	if (lstat(root, &st) == -1)
		return -1;
	if (!S_ISDIR(st.st_mode))
		return -2;

	ret |= MYVOLUMEINFO_STREAMS;
	
	return ret;
}

bool fs_path_exists(const std::string& s)
{
	struct stat buffer{};
	auto input = string(s);
	auto output = iso_8859_1_to_utf8(input);
	return stat(output.c_str(), &buffer) == 0;
}

struct my_openfile_s* my_open(const TCHAR* name, int flags)
{
	struct my_openfile_s* mos;

	mos = xmalloc(struct my_openfile_s, 1);
	if (!mos)
		return NULL;
	auto input = string(name);
	auto output = iso_8859_1_to_utf8(input);
	if (flags & O_CREAT)
	{		
		mos->fd = open(output.c_str(), flags, 0660);
	}
	else
	{
		mos->fd = open(output.c_str(), flags);
	}
	if (!mos->fd) {
		xfree(mos);
		mos = NULL;
	}
	return mos;
}

void my_close(struct my_openfile_s* mos)
{
	if (mos)
		close(mos->fd);
	xfree(mos);
}

unsigned int my_read(struct my_openfile_s* mos, void* b, unsigned int size)
{
	const auto bytes_read = read(mos->fd, b, size);
	if (bytes_read == -1)
	{
		write_log("WARNING: my_read failed (-1)\n");
		return 0;
	}
	return static_cast<unsigned int>(bytes_read);
}

unsigned int my_write(struct my_openfile_s* mos, void* b, unsigned int size)
{
	const auto bytes_written = write(mos->fd, b, size);
	if (bytes_written == -1)
	{
		write_log("WARNING: my_write failed (-1) fd=%d buffer=%p size=%d\n",
			mos->fd, b, size);
		write_log("errno %d\n", errno);
		write_log("  mos %p -> h=%d\n", mos, mos->fd);
		return 0;
	}
	return static_cast<unsigned int>(bytes_written);
}

int my_mkdir(const TCHAR* path)
{
	auto input = string(path);
	auto output = iso_8859_1_to_utf8(input);
	int error = mkdir(output.c_str(), 0755);
	if (error) {
		return -1;
	}
	return 0;
}

int my_truncate(const TCHAR* name, uae_u64 len)
{
	int int_len = (int)len;
	struct my_openfile_s* mos = my_open(name, O_WRONLY);
	if (mos == NULL) {
		write_log("WARNING: opening file for truncation failed\n");
		return -1;
	}
#ifdef WINDOWS
	int result = _chsize(mos->fd, int_len);
#else
	int result = ftruncate(mos->fd, int_len);
#endif
	my_close(mos);
	return result;
}

static void remove_extra_file(const char* path, const char* name)
{
	auto* p = (TCHAR*)malloc(MAX_DPATH);
	strcpy(p, path);
	fix_trailing(p);
	strcat(p, name);
	
	unlink(p);
	xfree(p);
}

int my_rmdir(const TCHAR* path)
{
	remove_extra_file(path, "Thumbs.db");
	remove_extra_file(path, ".DS_Store");

	errno = 0;
	auto input = string(path);
	auto output = iso_8859_1_to_utf8(input);
	int result = rmdir(output.c_str());

	return result;
}

int my_unlink(const TCHAR* path)
{
	errno = 0;
	auto input = string(path);
	auto output = iso_8859_1_to_utf8(input);
	int result = unlink(output.c_str());

	return result;
}

int my_rename(const TCHAR* oldname, const TCHAR* newname)
{
	errno = 0;
	auto old_input = string(oldname);
	auto old_output = iso_8859_1_to_utf8(old_input);
	auto new_input = string(newname);
	auto new_output = iso_8859_1_to_utf8(new_input);

	return rename(old_output.c_str(), new_output.c_str());
}

uae_s64 my_lseek(struct my_openfile_s* mos, uae_s64 offset, int whence)
{
	errno = 0;
	return lseek(mos->fd, offset, whence);
}

FILE* my_opentext(const TCHAR* name)
{
	auto input = string(name);
	auto output = iso_8859_1_to_utf8(input);
	return fopen(output.c_str(), "rb");
}

bool my_createshortcut(const char* source, const char* target, const char* description)
{
	return false;
}

bool my_resolvesoftlink(TCHAR* linkfile, int size, bool linkonly)
{
	return false;
}

const TCHAR* my_getfilepart(const TCHAR* filename)
{
	const TCHAR* p;

	p = _tcsrchr(filename, '\\');
	if (p)
		return p + 1;
	p = _tcsrchr(filename, '/');
	if (p)
		return p + 1;
	return p;
}

int host_errno_to_dos_errno(int err)
{
	static int warned = 0;

	switch (err) {
	case ENOMEM:
		return ERROR_NO_FREE_STORE;
	case EEXIST:
		return ERROR_OBJECT_EXISTS;
	case EACCES:
	case EROFS:
		return ERROR_WRITE_PROTECTED;
		//case ENOMEDIUM:
	case EINVAL:
	case ENOENT:
	case EBADF:
		return ERROR_OBJECT_NOT_AROUND;
	case ENOSPC:
		return ERROR_DISK_IS_FULL;
	case EBUSY:
		return ERROR_OBJECT_IN_USE;
	case EISDIR:	
		return ERROR_OBJECT_WRONG_TYPE;
#if defined(ETXTBSY)
	case ETXTBSY:	return ERROR_OBJECT_IN_USE;
#endif	
#if defined(ENOTEMPTY)
#if ENOTEMPTY != EEXIST
	case ENOTEMPTY:	return ERROR_DIRECTORY_NOT_EMPTY;
#endif
#endif
	case ESPIPE:
		return ERROR_SEEK_ERROR;
	default:
		if (!warned) {
			gui_message(_T("Unimplemented error %d - Contact author!"), err);
			warned = 1;
		}
		return ERROR_NOT_IMPLEMENTED;
	}
}

void my_canonicalize_path(const TCHAR* path, TCHAR* out, int size)
{
	_tcsncpy(out, path, size);
	out[size - 1] = 0;
	return;
}

bool my_issamepath(const TCHAR* path1, const TCHAR* path2)
{
	return _tcsicmp(path1, path2) == 0;
}

int my_issamevolume(const TCHAR* path1, const TCHAR* path2, TCHAR* path)
{
	TCHAR p1[MAX_DPATH];
	TCHAR p2[MAX_DPATH];
	unsigned int len, cnt;

	my_canonicalize_path(path1, p1, sizeof p1 / sizeof(TCHAR));
	my_canonicalize_path(path2, p2, sizeof p2 / sizeof(TCHAR));
	len = _tcslen(p1);
	if (len > _tcslen(p2))
		len = _tcslen(p2);
	if (_tcsnicmp(p1, p2, len))
		return 0;
	_tcscpy(path, p2 + len);
	cnt = 0;
	for (unsigned int i = 0; i < _tcslen(path); i++) {
		if (path[i] == '\\' || path[i] == '/') {
			path[i] = '/';
			cnt++;
		}
	}
	write_log(_T("'%s' (%s) matched with '%s' (%s), extra = '%s'\n"), path1, p1, path2, p2, path);
	return cnt;
}

int dos_errno(void)
{
	const auto e = errno;

	switch (e) {
	case ENOMEM:	return ERROR_NO_FREE_STORE;
	case EEXIST:	return ERROR_OBJECT_EXISTS;
	case EACCES:	return ERROR_WRITE_PROTECTED;
	case ENOENT:	return ERROR_OBJECT_NOT_AROUND;
	case ENOTDIR:	return ERROR_OBJECT_WRONG_TYPE;
	case ENOSPC:	return ERROR_DISK_IS_FULL;
	case EBUSY:       	return ERROR_OBJECT_IN_USE;
	case EISDIR:	return ERROR_OBJECT_WRONG_TYPE;
#if defined(ETXTBSY)
	case ETXTBSY:	return ERROR_OBJECT_IN_USE;
#endif
#if defined(EROFS)
	case EROFS:       	return ERROR_DISK_WRITE_PROTECTED;
#endif
#if defined(ENOTEMPTY)
#if ENOTEMPTY != EEXIST
	case ENOTEMPTY:	return ERROR_DIRECTORY_NOT_EMPTY;
#endif
#endif

	default:
		write_log(("Unimplemented error %s\n", strerror(e)));
		return ERROR_NOT_IMPLEMENTED;
	}
}

void filesys_host_init()
{
}

/* Returns 1 if an actual volume-name was found, 2 if no volume-name (so uses some defaults) */
int target_get_volume_name(struct uaedev_mount_info* mtinf, struct uaedev_config_info* ci, bool inserted,
                           bool fullcheck, int cnt)
{
	sprintf(ci->volname, "DH_%c", ci->rootdir[0]);
	return 2;
}
