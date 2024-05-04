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
#include <iostream>

#ifdef USE_OLDGCC
#include <experimental/filesystem>
#else
#include <filesystem>
#endif

#include <set>
#include <sys/mman.h>

#include "crc32.h"
#include "fsdb_host.h"
#include "uae.h"

#ifdef __MACH__
#include <CoreFoundation/CoreFoundation.h>
#endif

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
	char* src_ptr = in_buf.data();
	size_t src_size = input.size();
	std::vector<char> buf(1024);
	std::string dst;

	auto* iconv_ = iconv_open("ISO-8859-1//TRANSLIT", "UTF-8");

	while (src_size > 0) {
		char* dst_ptr = buf.data();
		size_t dst_size = buf.size();
		size_t res = ::iconv(iconv_, &src_ptr, &src_size, &dst_ptr, &dst_size);
		if (res == (size_t)-1) {
			if (errno != E2BIG) {
				// skip character
				++src_ptr;
				--src_size;
			}
		}
		dst.append(buf.data(), buf.size() - dst_size);
	}
	output = std::move(dst);
	iconv_close(iconv_);
}

std::string iso_8859_1_to_utf8(const std::string& str)
{
	std::string str_out;
	str_out.reserve(str.size() * 2); // Reserve space to avoid reallocations

	for (const auto& ch : str)
	{
		uint8_t byte = static_cast<uint8_t>(ch);
		if (byte < 0x80) {
			str_out.push_back(byte);
		}
		else {
			str_out.push_back(0xc0 | byte >> 6);
			str_out.push_back(0x80 | (byte & 0x3f));
		}
	}
	return str_out;
}
#endif

// Helper function to copy UTF 8 string to C string
#ifdef __MACH__
std::string CFStringCopyUTF8String(CFStringRef aString) {
	if (aString == NULL) {
		return NULL;
	}

	CFIndex length = CFStringGetLength(aString);
	CFIndex maxSize =
	CFStringGetMaximumSizeForEncoding(length,
									  kCFStringEncodingUTF8);
	char *temp = (char *)malloc(maxSize);
	std::string buffer; 
	if (CFStringGetCString(aString, temp, maxSize,
						   kCFStringEncodingUTF8)) {
	buffer = std::string(temp);
		return buffer;
	}
	return NULL;
}
#endif

std::string prefix_with_application_directory_path(std::string currentpath)
{
#ifdef __MACH__
// On OS X we return the path of the bundle
	// Build full path to Resources directory where the data folder is located
	CFURLRef appUrlRef;
	// Get full path to current bundle + filename being fetched
	appUrlRef = CFBundleCopyResourceURL(CFBundleGetMainBundle(), CFStringCreateWithCString(NULL,currentpath.c_str(),kCFStringEncodingASCII), NULL, NULL);
	CFStringRef path;
	if( !CFURLCopyResourcePropertyForKey(appUrlRef, kCFURLPathKey, &path, NULL))
	{
		printf("Unable to fetch App bundle file path\n");
	}
	std::string filePath = CFStringCopyUTF8String(path);
	// Return correct filepath to application
	return filePath;
#else
	if (const auto env_dir = getenv("EXTERNAL_FILES_DIR"); env_dir != nullptr)
	{
		return getenv("EXTERNAL_FILES_DIR") + ("/" + currentpath);
	}
	return currentpath;
#endif
}


std::string prefix_with_data_path(std::string filename)
{
#ifdef __MACH__
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	if (mainBundle == NULL)
	{
		printf("Can't fetch main bundle\n");
		return filename;
	}
	std::string filePath = "Data/" + filename;
	CFURLRef dataFileURL = CFBundleCopyResourceURL(mainBundle, CFStringCreateWithCString(NULL, filePath.c_str(), kCFStringEncodingASCII), NULL, NULL);
	if (dataFileURL == NULL)
	{
		printf("Can't fetch dataFileUrl\n");
		return filename;
	}
	CFStringRef path;
	if (!CFURLCopyResourcePropertyForKey(dataFileURL, kCFURLPathKey, &path, NULL))
	{
		printf("Can't find file path\n");
		return filename;
	}
	filePath = CFStringCopyUTF8String(path);
	return filePath;
#else
	auto result = get_data_path();
	return result.append(filename);
#endif
}

std::string prefix_with_whdboot_path(std::string filename)
{
#ifdef __MACH__
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	if (mainBundle == NULL)
	{
		printf("Can't fetch main bundle\n");
		return filename;
	}

	std::string filePath = "Whdboot/" + filename;
	CFURLRef whdbootUrl = CFBundleCopyResourceURL(mainBundle, CFStringCreateWithCString(NULL, filePath.c_str(), kCFStringEncodingASCII), NULL, NULL);
	if (whdbootUrl == NULL)
	{
		printf("Can't fetch WhdbootUrl (CFBundleCopyResourceURL returned NULL)\n");
		return filename;
	}
	CFStringRef path;
	if (!CFURLCopyResourcePropertyForKey(whdbootUrl, kCFURLPathKey, &path, NULL))
	{
		printf("Can't find file path (CFURLCopyResourcePropertyForKey failed)\n");
		return filename;
	}
	filePath = CFStringCopyUTF8String(path);
	return filePath;
#else
	auto result = get_whdbootpath();
	return result.append(filename);
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

	// Convert input to UTF-8
	auto output = iso_8859_1_to_utf8(string(name));

	// Get current state
	struct stat st {};
	if (stat(output.c_str(), &st) == -1) {
		write_log("my_chmod: stat on file %s failed\n", output.c_str());
		return false;
	}

	// Prepare new mode
	auto newmode = st.st_mode;
	if (mode & FILEFLAG_WRITE)
		newmode |= S_IWUSR;  // set write permission
	else
		newmode &= ~S_IWUSR; // clear write permission

	// Change mode
	if (chmod(output.c_str(), newmode) == -1) {
		write_log("my_chmod: chmod on file %s failed\n", output.c_str());
		return false;
	}

	return true;
}

bool my_stat(const TCHAR* name, struct mystat* statbuf)
{
	if (!name || !statbuf) {
		write_log("my_stat: null arguments provided\n");
		return false;
	}

	struct stat st {};
	auto output = iso_8859_1_to_utf8(string(name));

	if (stat(output.c_str(), &st) == -1) {
		write_log("my_stat: stat on file %s failed\n", output.c_str());
		return false;
	}

	statbuf->size = st.st_size;
	statbuf->mode = ((st.st_mode & S_IRUSR) ? FILEFLAG_READ : 0) |
		((st.st_mode & S_IWUSR) ? FILEFLAG_WRITE : 0);
	statbuf->mtime.tv_sec = st.st_mtime;
	statbuf->mtime.tv_usec = 0;

	return true;
}

bool compare_nocase(const std::string& first, const std::string& second)
{
	const auto len = std::min(first.length(), second.length());

	for (unsigned int i = 0; i < len; ++i)
	{
		const auto first_ch = std::tolower(first[i]);
		const auto second_ch = std::tolower(second[i]);

		if (first_ch != second_ch)
			return first_ch < second_ch;
	}

	return first.length() < second.length();
}

struct my_opendir_s* my_opendir(const TCHAR* name, const TCHAR* mask)
{
	if (!name) {
		write_log("my_opendir: null directory name provided\n");
		return NULL;
	}

	auto* mod = new my_opendir_s;
	if (!mod) {
		write_log("my_opendir: memory allocation failed\n");
		return NULL;
	}

	auto output = iso_8859_1_to_utf8(string(name));
	mod->dir = opendir(output.c_str());

	if (!mod->dir) {
		write_log("my_opendir: opendir on directory %s failed\n", name);
		delete mod;
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
	if (!mod || !name)
		return 0;

	const set<std::string> ignoreList = { "_UAEFSDB.___", "Thumbs.db", ".DS_Store", "UAEFS.ini" };

	while (true)
	{
		auto* entry = readdir(mod->dir);
		if (entry == NULL)
			return 0;

		auto* result = entry->d_name;
		const int len = strlen(result);

		if (ignoreList.find(result) != ignoreList.end() ||
			(len > 5 && strncmp(result + len - 5, ".uaem", 5) == 0)) {
			continue;
		}

		auto string_input = string(result);
		string string_output;
		utf8_to_latin1_string(string_input, string_output);
		_tcscpy(name, string_output.c_str());

		return 1;
	}
}

bool my_existslink(const char* name)
{
	if (!name) {
		write_log("my_existslink: null file name provided\n");
		return false;
	}

	struct stat st {};
	if (lstat(name, &st) == -1) {
		write_log("my_existslink: lstat on file %s failed\n", name);
		return false;
	}

	return S_ISLNK(st.st_mode);
}

bool my_existsfile2(const char* name)
{
	struct stat st {};
	return stat(name, &st) != -1 && S_ISREG(st.st_mode);
}

bool my_existsfile(const char* name)
{
	struct stat st {};
	auto output = iso_8859_1_to_utf8(string(name));
	return stat(output.c_str(), &st) != -1 && S_ISREG(st.st_mode);
}

bool my_existsdir(const char* name)
{
	struct stat st {};
	auto output = iso_8859_1_to_utf8(string(name));
	return stat(output.c_str(), &st) != -1 && S_ISDIR(st.st_mode);
}

uae_s64 my_fsize(struct my_openfile_s* mos)
{
	if (mos == nullptr) {
		write_log("my_fsize: null pointer provided\n");
		return -1;
	}

	struct stat st {};
	if (fstat(mos->fd, &st) == -1) {
		write_log("my_fsize: fstat on file %s failed\n", mos->path);
		return -1;
	}
	return st.st_size;
}

int my_getvolumeinfo(const char* root)
{
	if (root == nullptr) {
		write_log("my_getvolumeinfo: null pointer provided\n");
		return -1;
	}

	struct stat st {};
	auto ret = 0;

	if (lstat(root, &st) == -1) {
		write_log("my_getvolumeinfo: lstat on file %s failed\n", root);
		return -1;
	}
	if (!S_ISDIR(st.st_mode)) {
		write_log("my_getvolumeinfo: %s is not a directory\n", root);
		return -2;
	}

	ret |= MYVOLUMEINFO_STREAMS;

	return ret;
}

bool fs_path_exists(const std::string& s)
{
	if (s.empty()) {
		write_log("fs_path_exists: empty string provided\n");
		return false;
	}

	struct stat buffer {};
	auto output = iso_8859_1_to_utf8(s);
	return stat(output.c_str(), &buffer) == 0;
}

struct my_openfile_s* my_open(const TCHAR* name, int flags)
{
	if (name == nullptr) {
		write_log("my_open: null pointer provided\n");
		return NULL;
	}

	auto* mos = xmalloc(struct my_openfile_s, 1);
	if (!mos) {
		write_log("my_open: memory allocation failed\n");
		return NULL;
	}

	auto output = iso_8859_1_to_utf8(string(name));
	if (flags & O_CREAT) {
		mos->fd = open(output.c_str(), flags, 0660);
	}
	else {
		mos->fd = open(output.c_str(), flags);
	}

	if (mos->fd == -1) {
		write_log("my_open: open on file %s failed\n", name);
		xfree(mos);
		return NULL;
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
	if (mos == nullptr) {
		write_log("my_read: null file pointer provided\n");
		return 0;
	}

	if (b == nullptr) {
		write_log("my_read: null buffer pointer provided\n");
		return 0;
	}

	const auto bytes_read = read(mos->fd, b, size);
	if (bytes_read == -1) {
		write_log("my_read: read on file %s failed with error %s\n", mos->path, strerror(errno));
		return 0;
	}
	else if (bytes_read < size) {
		write_log("my_read: read on file %s returned less bytes than requested\n", mos->path);
	}

	return static_cast<unsigned int>(bytes_read);
}

unsigned int my_write(struct my_openfile_s* mos, void* b, unsigned int size)
{
	if (mos == nullptr) {
		write_log("my_write: null file pointer provided\n");
		return 0;
	}

	if (b == nullptr) {
		write_log("my_write: null buffer pointer provided\n");
		return 0;
	}

	const auto bytes_written = write(mos->fd, b, size);
	if (bytes_written == -1) {
		write_log("my_write: write on file %s failed with error %s\n", mos->path, strerror(errno));
		return 0;
	}
	else if (bytes_written < size) {
		write_log("my_write: write on file %s wrote less bytes than requested\n", mos->path);
	}

	return static_cast<unsigned int>(bytes_written);
}

int my_mkdir(const TCHAR* path)
{
	if (path == nullptr) {
		write_log("my_mkdir: null pointer provided\n");
		return -1;
	}

	auto output = iso_8859_1_to_utf8(string(path));
	int error = mkdir(output.c_str(), 0755);
	if (error) {
		write_log("my_mkdir: mkdir on path %s failed with error %s\n", path, strerror(errno));
		return -1;
	}
	return 0;
}

int my_truncate(const TCHAR* name, uae_u64 len)
{
	if (name == nullptr) {
		write_log("my_truncate: null pointer provided\n");
		return -1;
	}

	struct my_openfile_s* mos = my_open(name, O_WRONLY);
	if (mos == NULL) {
		write_log("my_truncate: opening file %s for truncation failed\n", name);
		return -1;
}

	int result;
#ifdef WINDOWS
	if (len > INT_MAX) {
		write_log("my_truncate: length %llu is too large for _chsize\n", len);
		result = -1;
	}
	else {
		result = _chsize(mos->fd, static_cast<int>(len));
	}
#else
	result = ftruncate(mos->fd, len);
#endif

	if (result == -1) {
		write_log("my_truncate: truncating file %s failed with error %s\n", name, strerror(errno));
	}

	my_close(mos);
	return result;
}

static void remove_extra_file(const char* path, const char* name)
{
	std::string p(path);
	p = fix_trailing(p);
	p += name;
	unlink(p.c_str());
}

bool my_isfilehidden(const char* path)
{
	if (path == nullptr)
	{
		return false;
	}

	std::string filename(path);
	return filename[0] == '.';
}

void my_setfilehidden(const TCHAR* path, bool hidden)
{
	if (path == nullptr)
	{
		return;
	}

	std::string filename(path);

#ifdef _WIN32
	DWORD attrs = GetFileAttributes(path);

	if (hidden)
	{
		attrs |= FILE_ATTRIBUTE_HIDDEN;
	}
	else
	{
		attrs &= ~FILE_ATTRIBUTE_HIDDEN;
	}

	SetFileAttributes(path, attrs);
#else
	std::string newname;

	if (hidden)
	{
		if (filename[0] != '.')
		{
			newname = "." + filename;
			rename(filename.c_str(), newname.c_str());
		}
	}
	else
	{
		if (filename[0] == '.')
		{
			newname = filename.substr(1);
			rename(filename.c_str(), newname.c_str());
		}
	}
#endif
}

int my_rmdir(const TCHAR* path)
{
	if (path == nullptr) {
		write_log("my_rmdir: null directory path provided\n");
		return -1;
	}

	remove_extra_file(path, "Thumbs.db");
	remove_extra_file(path, ".DS_Store");

	errno = 0;
	auto output = iso_8859_1_to_utf8(string(path));
	int result = rmdir(output.c_str());

	if (result != 0) {
		write_log("my_rmdir: rmdir on directory %s failed with error %d\n", path, errno);
	}

	return result;
}

int my_unlink(const TCHAR* path)
{
	if (path == nullptr) {
		write_log("my_unlink: null file path provided\n");
		return -1;
	}

	errno = 0;
	auto output = iso_8859_1_to_utf8(string(path));
	int result = unlink(output.c_str());

	if (result != 0) {
		write_log("my_unlink: unlink on file %s failed with error %d\n", path, errno);
	}

	return result;
}

int my_rename(const TCHAR* oldname, const TCHAR* newname)
{
	if (oldname == nullptr || newname == nullptr) {
		write_log("my_rename: null file name provided\n");
		return -1;
	}

	errno = 0;
	auto old_output = iso_8859_1_to_utf8(string(oldname));
	auto new_output = iso_8859_1_to_utf8(string(newname));

	int result = rename(old_output.c_str(), new_output.c_str());

	if (result != 0) {
		write_log("my_rename: rename from %s to %s failed with error %d\n", oldname, newname, errno);
	}

	return result;
}

uae_s64 my_lseek(struct my_openfile_s* mos, uae_s64 offset, int whence)
{
	if (mos == nullptr) {
		write_log("my_lseek: null file descriptor provided\n");
		return -1;
	}

	errno = 0;
	auto result = lseek(mos->fd, offset, whence);

	if (result == -1) {
		write_log("my_lseek: lseek on file %s failed with error %d\n", mos->path, errno);
	}

	return result;
}

FILE* my_opentext(const TCHAR* name)
{
	if (name == nullptr) {
		write_log("my_opentext: null file name provided\n");
		return nullptr;
	}

	auto output = iso_8859_1_to_utf8(string(name));
	FILE* file = fopen(output.c_str(), "rb");

	if (file == nullptr) {
		write_log("my_opentext: fopen on file %s failed\n", name);
	}

	return file;
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
	return filename;
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
	if (path == nullptr || out == nullptr) {
		write_log("my_canonicalize_path: null arguments provided\n");
		return;
	}

	_tcsncpy(out, path, size - 1);
	out[size - 1] = '\0';
}

bool my_issamepath(const TCHAR* path1, const TCHAR* path2)
{
	if (path1 == nullptr || path2 == nullptr) {
		write_log("my_issamepath: null path provided\n");
		return false;
	}

	return _tcsicmp(path1, path2) == 0;
}

int my_issamevolume(const TCHAR* path1, const TCHAR* path2, TCHAR* path)
{
	if (path1 == nullptr || path2 == nullptr || path == nullptr) {
		write_log("my_issamevolume: null arguments provided\n");
		return 0;
	}

	TCHAR p1[MAX_DPATH];
	TCHAR p2[MAX_DPATH];

	my_canonicalize_path(path1, p1, sizeof p1 / sizeof(TCHAR));
	my_canonicalize_path(path2, p2, sizeof p2 / sizeof(TCHAR));

	unsigned int len = _tcslen(p1);
	if (len > _tcslen(p2))
		len = _tcslen(p2);

	if (_tcsnicmp(p1, p2, len))
		return 0;

	_tcscpy(path, p2 + len);

	unsigned int cnt = 0;
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
		write_log(_T("Unimplemented error %s\n"), strerror(e));
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

// If replace is false, copyfile will fail if file already exists
bool copyfile(const char* target, const char* source, const bool replace)
{
#ifdef USE_OLDGCC
	std::experimental::filesystem::copy_options options = {};
	options = replace ? experimental::filesystem::copy_options::overwrite_existing : experimental::filesystem::copy_options::none;
#else
	std::filesystem::copy_options options = {};
	options = replace ? filesystem::copy_options::overwrite_existing : filesystem::copy_options::none;
#endif
	return copy_file(source, target, options);
}

void filesys_addexternals(void)
{
	// this would mount system drives on Windows
}

std::string my_get_sha1_of_file(const char* filepath)
{
	if (filepath == nullptr) {
		write_log("my_get_sha1_of_file: null file path provided\n");
		return "";
	}

	int fd = open(filepath, O_RDONLY);
	if (fd < 0) {
		write_log("my_get_sha1_of_file: open on file %s failed\n", filepath);
		return "";
	}

	struct stat sb;
	if (fstat(fd, &sb) < 0) {
		write_log("my_get_sha1_of_file: fstat on file %s failed\n", filepath);
		close(fd);
		return "";
	}

	void* mem = mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (mem == MAP_FAILED) {
		write_log("my_get_sha1_of_file: mmap on file %s failed\n", filepath);
		close(fd);
		return "";
	}

	const TCHAR* sha1 = get_sha1_txt(mem, sb.st_size);

	munmap(mem, sb.st_size);
	close(fd);

	return string(sha1);
}
