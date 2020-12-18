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

#include "fsdb_host.h"
#include "uae.h"

int my_errno = 0;

struct my_opendir_s {
	DIR* dir{};
};

struct my_openfile_s {
	int fd;
	char* path;
};

#ifdef AMIBERRY
int GetUtf8CharacterLength(unsigned char utf8Char)
{
	if (utf8Char < 0x80) return 1;
	if ((utf8Char & 0x20) == 0) return 2;
	if ((utf8Char & 0x10) == 0) return 3;
	if ((utf8Char & 0x08) == 0) return 4;
	if ((utf8Char & 0x04) == 0) return 5;

	return 6;
}

char Utf8ToLatin1Character(char* s, int* readIndex)
{
	int len = GetUtf8CharacterLength(static_cast<unsigned char>(s[*readIndex]));
	if (len == 1)
	{
		char c = s[*readIndex];
		(*readIndex)++;

		return c;
	}

	unsigned int v = (s[*readIndex] & (0xff >> (len + 1))) << ((len - 1) * 6);
	(*readIndex)++;
	for (len--; len > 0; len--)
	{
		v |= (static_cast<unsigned char>(s[*readIndex]) - 0x80) << ((len - 1) * 6);
		(*readIndex)++;
	}

	return (v > 0xff) ? 0 : (char)v;
}

// overwrites s in place
char* Utf8ToLatin1String(char* s)
{
	for (int readIndex = 0, writeIndex = 0; ; writeIndex++)
	{
		if (s[readIndex] == 0)
		{
			s[writeIndex] = 0;
			break;
		}

		char c = Utf8ToLatin1Character(s, &readIndex);
		if (c == 0)
		{
			c = '_';
		}

		s[writeIndex] = c;
	}

	return s;
}

std::string iso_8859_1_to_utf8(std::string& str)
{
	string strOut;
	for (std::string::iterator it = str.begin(); it != str.end(); ++it)
	{
		uint8_t ch = *it;
		if (ch < 0x80) {
			strOut.push_back(ch);
		}
		else {
			strOut.push_back(0xc0 | ch >> 6);
			strOut.push_back(0x80 | (ch & 0x3f));
		}
}
	return strOut;
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
	struct stat64 st{};
	if (stat64(name, &st) == -1) {
		write_log("my_chmod: stat on file %s failed\n", name);
		return false;
	}

	if (mode & FILEFLAG_WRITE)
		// set write permission
		st.st_mode |= S_IWUSR;
	else
		// clear write permission
		st.st_mode &= ~(S_IWUSR);
	chmod(name, st.st_mode);

	stat64(name, &st);
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
	struct stat64 st{};

	if (stat64(name, &st) == -1) {
		write_log("my_stat: stat on file %s failed\n", name);
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
	// mask is ignored
	if (mask && strcmp(mask, "*.*") != 0) {
		write_log("WARNING: directory mask was not *.*");
	}

	auto* mod = xmalloc(struct my_opendir_s, 1);
	if (!mod)
		return NULL;
	mod->dir = opendir(name);
	if (!mod->dir) {
		my_errno = errno;
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
	my_errno = 0;
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
		auto* entry = readdir64(mod->dir);
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

		Utf8ToLatin1String(result);
		_tcscpy(name, result);
		return 1;
	}
}

int my_existsfile(const char* name)
{
	struct stat st {};
	if (lstat(name, &st) == -1)
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
	if (lstat(name, &st) == -1)
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
	return (stat(s.c_str(), &buffer) == 0);
}

struct my_openfile_s* my_open(const TCHAR* name, const int flags)
{
	int open_flags = O_BINARY;
	if (flags & O_TRUNC) {
		open_flags = open_flags | O_TRUNC; //write_log("  O_TRUNC\n");
	}
	if (flags & O_CREAT) {
		open_flags = open_flags | O_CREAT; //write_log("  O_CREAT\n");
	}
	if (flags & O_RDWR) {
		open_flags = open_flags | O_RDWR; //write_log("  O_RDRW\n");
	}
	else if (flags & O_RDONLY) {
		open_flags = open_flags | O_RDONLY; //write_log("  O_RDONLY\n");
	}
	else if (flags & O_WRONLY) {
		open_flags = open_flags | O_WRONLY; //write_log("  O_WRONLY\n");
	}
	char* path = my_strdup(name);

	int file_existed = fs_path_exists(path);
	int file = open(path, open_flags, 0644);
	if (file == -1) {
		my_errno = errno;
		write_log("WARNING: my_open could not open (%s, %d)\n", name,
			open_flags);
		if (open_flags & O_TRUNC) {
			write_log("  O_TRUNC\n");
		}
		if (open_flags & O_CREAT) {
			write_log("  O_CREAT\n");
		}
		if (open_flags & O_RDWR) {
			write_log("  O_RDWR\n");
		}
		else if (open_flags & O_RDONLY) {
			write_log("  O_RDONLY\n");
		}
		else if (open_flags & O_WRONLY) {
			write_log("  O_WRONLY\n");
		}
		free(path);
		return NULL;
	}
	if (!file_existed) {
		fsdb_file_info info;
		fsdb_init_file_info(&info);
		int error = fsdb_set_file_info(path, &info);
	}
	xfree(path);

	auto* mos = new struct my_openfile_s;
	mos->fd = file;
	mos->path = my_strdup(name);
	my_errno = 0;
	return mos;
}

void my_close(struct my_openfile_s* mos)
{
	free(mos->path);
	int result = close(mos->fd);
	if (result != 0)
		write_log("error closing file\n");
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
	int error = mkdir(path, 0755);
	if (error) {
		my_errno = errno;
		return -1;
	}
	int file_existed = 0;
	if (!file_existed) {
		fsdb_file_info info;
		fsdb_init_file_info(&info);
		int error = fsdb_set_file_info(path, &info);
	}
	my_errno = 0;
	return 0;
}

int my_truncate(const TCHAR* name, uae_u64 len)
{
	int int_len = (int)len;
	struct my_openfile_s* mos = my_open(name, O_WRONLY);
	if (mos == NULL) {
		my_errno = errno;
		write_log("WARNING: opening file for truncation failed\n");
		return -1;
	}
#ifdef WINDOWS
	int result = _chsize(mos->fd, int_len);
#else
	int result = ftruncate(mos->fd, int_len);
#endif
	my_close(mos);
	my_errno = 0;
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
	int result = rmdir(path);
	my_errno = errno;

	auto* meta_name = (TCHAR*)malloc(MAX_DPATH);
	strcpy(meta_name, path);
	strcat(meta_name, ".uaem");
	
	unlink(meta_name);
	xfree(meta_name);

	return result;
}

int my_unlink(const TCHAR* path)
{
	errno = 0;
	int result = unlink(path);
	my_errno = errno;

	auto* meta_name = (TCHAR*)malloc(MAX_DPATH);
	strcpy(meta_name, path);
	strcat(meta_name, ".uaem");
	
	unlink(meta_name);
	xfree(meta_name);

	return result;
}

static int rename_file(const char* oldname, const char* newname) {
	int result = 0;
	for (int i = 0; i < 10; i++) {
		result = rename(oldname, newname);
		my_errno = errno;
		if (result == 0) {
			break;
		}
#ifdef WINDOWS
		write_log("Could not rename \"%s\" to \"%s\": Windows error"
			"code %lu\n", oldname, newname, GetLastError());
#endif
		sleep_millis(10);
	}
	return result;
}

int my_rename(const TCHAR* oldname, const TCHAR* newname)
{
	errno = 0;
	int result = rename_file(oldname, newname);
	if (result != 0) {
		// could not rename file
		return result;
	}

	auto* oldname2 = (TCHAR*)malloc(MAX_DPATH);
	strcpy(oldname2, oldname);
	strcat(oldname2, ".uaem");
	
	if (fs_path_exists(oldname2)) {
		auto* newname2 = (TCHAR*)malloc(MAX_DPATH);
		strcpy(newname2, newname);
		strcat(newname2, ".uaem");
		if (rename_file(oldname2, newname2) != 0) {
			// could not rename meta file, revert changes
			int saved_errno = my_errno;
			rename_file(newname, oldname);
			my_errno = saved_errno;
			result = -1;
		}
		xfree(newname2);
	}
	xfree(oldname2);
	return result;
}

uae_s64 my_lseek(struct my_openfile_s* mos, uae_s64 offset, int whence)
{
	errno = 0;
	off_t result = lseek(mos->fd, offset, whence);
	my_errno = errno;
	return result;
}

FILE* my_opentext(const TCHAR* name)
{
	// FIXME: WinUAE's version does some content checking related to unicode.
	// see fsdb_mywin32.cpp
	return fopen(name, "rb");
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

int host_errno_to_dos_errno(int err) {
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
	case ENOTEMPTY:
		return ERROR_DIRECTORY_NOT_EMPTY;
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
	return host_errno_to_dos_errno(my_errno);
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
