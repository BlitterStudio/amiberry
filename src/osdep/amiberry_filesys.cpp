/*
 * UAE - The Un*x Amiga Emulator
 *
 * Amiberry: Amiga Emulator
 * Filesystem functions
 *
 * Copyright 2025 Dimitris Panokostas
 *
 */

#include "config.h"
#include "sysconfig.h"
#include "sysdeps.h"

#include "fsdb.h"
#include "options.h"
#include "filesys.h"
#include "zfile.h"
#include <unistd.h>
#include <algorithm>
#include <list>
#include <dirent.h>
#include <iconv.h>
#include <iostream>
#include <mutex>

#ifdef USE_OLDGCC
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
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
static bool has_logged_iconv_fail = false;
[[nodiscard]] bool utf8_to_latin1_string(const std::string_view input, std::string& output)
{
    // Fast path for empty input
    if (input.empty()) {
        output.clear();
        return true;
    }

    // Pre-allocate output buffer - maximum size needed is the input length
    output.clear();
    output.reserve(input.length());

    // Fast path for ASCII-only content (bytes < 128)
    bool ascii_only = true;
    for (unsigned char c : input) {
        if (c >= 128) {
            ascii_only = false;
            break;
        }
    }

    // If ASCII only, we can just copy directly without conversion
    if (ascii_only) {
        output.assign(input);
        return true;
    }

    // Thread-local RAII wrapper for iconv to ensure thread safety
    static thread_local class IconvWrapper {
        iconv_t handle_;
        std::once_flag init_flag_;
        bool valid_ = false;

    public:
        IconvWrapper() : handle_(iconv_t(-1)) {}

        ~IconvWrapper() {
            if (handle_ != iconv_t(-1))
                iconv_close(handle_);
        }

        iconv_t get() {
            std::call_once(init_flag_, [this]() {
                handle_ = iconv_open("ISO-8859-1//TRANSLIT", "UTF-8");
                valid_ = (handle_ != iconv_t(-1));
            });
            return handle_;
        }

        bool is_valid() const { return valid_; }

        void reset() {
            if (valid_)
                iconv(handle_, nullptr, nullptr, nullptr, nullptr);
        }
    } iconv_wrapper;

    const iconv_t iconv_handle = iconv_wrapper.get();
    if (iconv_handle == iconv_t(-1)) {
        if (!has_logged_iconv_fail) {
            write_log(_T("utf8_to_latin1_string: iconv_open failed: %s\n"), strerror(errno));
            has_logged_iconv_fail = true;
        }
        output = input;
        return false;
    }

    // Use a single allocation for input buffer
    std::vector<char> in_buf(input.begin(), input.end());

    // Fixed buffer for output with reasonable size
    std::array<char, 2048> buf{};
    char* src_ptr = in_buf.data();
    size_t src_size = input.size();
    size_t invalid_chars = 0;

    // Reset conversion state before starting
    iconv_wrapper.reset();

    // Perform conversion in chunks
    while (src_size > 0) {
        char* dst_ptr = buf.data();
        size_t dst_size = buf.size();

        size_t res = iconv(iconv_handle, &src_ptr, &src_size, &dst_ptr, &dst_size);

        if (res == size_t(-1)) {
            switch (errno) {
                case E2BIG:
                    // Buffer full - append what we have and continue
                    output.append(buf.data(), buf.size() - dst_size);
                    continue;

                case EILSEQ:
                    // Invalid sequence - skip byte and continue
                    ++src_ptr;
                    --src_size;
                    ++invalid_chars;
                    break;

                case EINVAL:
                    // Incomplete sequence at end - skip remaining bytes
                    src_size = 0;
                    ++invalid_chars;
                    break;

                default:
                    // Unexpected error
                    write_log(_T("utf8_to_latin1_string: conversion error: %s\n"), strerror(errno));
                    src_size = 0;
                    ++invalid_chars;
            }
        }

        // Append successfully converted data
        if (buf.size() > dst_size) {
            output.append(buf.data(), buf.size() - dst_size);
        }
    }

    // Log if significant number of characters were invalid
    if (invalid_chars > 0 && invalid_chars > input.size() / 10) {
        write_log(_T("utf8_to_latin1_string: %zu invalid characters found in string of length %zu\n"),
                 invalid_chars, input.size());
    }

    return invalid_chars == 0;
}

[[nodiscard]] std::string iso_8859_1_to_utf8(std::string_view str) noexcept
{
	if (str.empty()) {
		return {};
	}

	// Calculate exact size needed to avoid reallocations
	const auto utf8_size = std::count_if(str.begin(), str.end(),
		[](unsigned char c) { return c >= 0x80; }) + str.size();

	std::string str_out;
	str_out.reserve(utf8_size);

	// Convert each character
	for (const auto ch : str)
	{
		const auto byte = static_cast<uint8_t>(ch);
		if (byte < 0x80) {
			// ASCII characters (0-127) are copied as-is
			str_out += byte;
		}
		else {
			// Convert ISO-8859-1 to UTF-8 (two bytes)
			str_out += static_cast<char>(0xC0 | (byte >> 6));     // First byte: 110xxxxx
			str_out += static_cast<char>(0x80 | (byte & 0x3F));   // Second byte: 10xxxxxx
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

[[nodiscard]] std::string prefix_with_application_directory_path(std::string_view currentpath)
{
	if (currentpath.empty()) {
		write_log("prefix_with_application_directory_path: Empty path provided\n");
		return {};
	}

#ifdef __MACH__
	try {
		// Get the main bundle
		CFBundleRef mainBundle = CFBundleGetMainBundle();
		if (!mainBundle) {
			write_log("prefix_with_application_directory_path: Cannot fetch main bundle\n");
			return std::string(currentpath);
		}

		// Create URL for resource
		CFStringRef cfPath = CFStringCreateWithCString(nullptr,
			currentpath.data(),
			kCFStringEncodingUTF8);
		if (!cfPath) {
			write_log("prefix_with_application_directory_path: Failed to create CFString\n");
			return std::string(currentpath);
		}

		// Use RAII for CFString cleanup
		struct CFStringReleaser {
			CFStringRef ref;
			~CFStringReleaser() { if (ref) CFRelease(ref); }
		} pathGuard{ cfPath };

		CFURLRef appUrlRef = CFBundleCopyResourceURL(mainBundle, cfPath, nullptr, nullptr);
		if (!appUrlRef) {
			write_log("prefix_with_application_directory_path: Failed to get resource URL\n");
			return std::string(currentpath);
		}

		// Use RAII for CFURL cleanup
		struct CFURLReleaser {
			CFURLRef ref;
			~CFURLReleaser() { if (ref) CFRelease(ref); }
		} urlGuard{ appUrlRef };

		CFStringRef pathRef = nullptr;
		if (!CFURLCopyResourcePropertyForKey(appUrlRef, kCFURLPathKey, &pathRef, nullptr) || !pathRef) {
			write_log("prefix_with_application_directory_path: Unable to fetch App bundle file path\n");
			return std::string(currentpath);
		}

		// Use RAII for path cleanup
		struct CFPathReleaser {
			CFStringRef ref;
			~CFPathReleaser() { if (ref) CFRelease(ref); }
		} resultGuard{ pathRef };

		return CFStringCopyUTF8String(pathRef);
	}
	catch (const std::exception& e) {
		write_log("prefix_with_application_directory_path: Exception: %s\n", e.what());
		return std::string(currentpath);
	}
#else
	try {
		if (const auto env_dir = std::getenv("EXTERNAL_FILES_DIR")) {
			std::string result;
			result.reserve(std::strlen(env_dir) + currentpath.length() + 1);
			result = env_dir;
			result += '/';
			result += currentpath;
			return result;
		}
		return std::string(currentpath);
	}
	catch (const std::exception& e) {
		write_log("prefix_with_application_directory_path: Exception: %s\n", e.what());
		return std::string(currentpath);
	}
#endif
}

std::string prefix_with_data_path(const std::string& filename)
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

[[nodiscard]] int my_setcurrentdir(const TCHAR* curdir, TCHAR* oldcur)
{
	try {
		// Store current directory if requested
		if (oldcur != nullptr) {
			if (!getcwd(oldcur, MAX_DPATH)) {
				write_log("my_setcurrentdir: Failed to get current directory: %s\n", strerror(errno));
				return -1;
			}
		}

		// Change directory if requested
		if (curdir != nullptr) {
			if (chdir(curdir) != 0) {
				write_log("my_setcurrentdir: Failed to change directory to '%s': %s\n",
					curdir, strerror(errno));
				return -1;
			}
		}

		return 0;
	}
	catch (const std::exception& e) {
		write_log("my_setcurrentdir: Exception occurred: %s\n", e.what());
		return -1;
	}
}

[[nodiscard]] bool my_chmod(const TCHAR* name, uae_u32 mode) noexcept
{
	// Input validation
	if (!name) {
		write_log("my_chmod: null filename provided\n");
		return false;
	}

	try {
		// Use std::filesystem for path handling
		const auto output = iso_8859_1_to_utf8(std::string_view(name));
		fs::path filepath(output);

		// Get current file status
		std::error_code ec;
		auto perms = fs::status(filepath, ec).permissions();
		if (ec) {
			write_log("my_chmod: stat on file %s failed: %s\n",
				output.c_str(), ec.message().c_str());
			return false;
		}

		// Calculate new permissions
		// Clear write permission bits first
		perms &= ~(fs::perms::owner_write |
			fs::perms::group_write |
			fs::perms::others_write);

		// Set new write permissions if requested
		if (mode & FILEFLAG_WRITE) {
			perms |= fs::perms::owner_write;
		}

		// Apply new permissions
		fs::permissions(filepath, perms, ec);
		if (ec) {
			write_log("my_chmod: chmod on file %s failed: %s\n",
				output.c_str(), ec.message().c_str());
			return false;
		}

		return true;
	}
	catch (const std::exception& e) {
		write_log("my_chmod: Exception while processing %s: %s\n",
			name, e.what());
		return false;
	}
}

[[nodiscard]] bool my_stat(const TCHAR* name, struct mystat* statbuf) noexcept
{
	try {
		// Input validation
		if (!name || !statbuf) {
			write_log("my_stat: null argument(s) provided\n");
			return false;
		}

		// Use filesystem for better path handling
		const auto output = iso_8859_1_to_utf8(std::string_view(name));
		fs::path filepath(output);

		// Get file status using std::filesystem
		std::error_code ec;
		auto file_status = fs::status(filepath, ec);
		if (ec) {
			write_log("my_stat: status check failed for %s: %s\n",
				output.c_str(), ec.message().c_str());
			return false;
		}

		// Get detailed file information
		struct stat st {};
		if (stat(output.c_str(), &st) == -1) {
			write_log("my_stat: stat failed for %s: %s\n",
				output.c_str(), strerror(errno));
			return false;
		}

		// Fill in the stat buffer
		statbuf->size = st.st_size;
		statbuf->mode = ((file_status.permissions() & fs::perms::owner_read) != fs::perms::none ? FILEFLAG_READ : 0) |
			((file_status.permissions() & fs::perms::owner_write) != fs::perms::none ? FILEFLAG_WRITE : 0);
		statbuf->mtime.tv_sec = st.st_mtime;
		statbuf->mtime.tv_usec = 0;

		return true;
	}
	catch (const std::exception& e) {
		write_log("my_stat: Exception while processing %s: %s\n",
			name, e.what());
		return false;
	}
}

bool compare_nocase(const std::string& first, const std::string& second)
{
	return std::lexicographical_compare(
		first.begin(), first.end(),
		second.begin(), second.end(),
		[](const unsigned char a, const unsigned char b) {
			return std::tolower(a) < std::tolower(b);
		}
	);
}

struct my_opendir_s* my_opendir(const TCHAR* name, const TCHAR* mask)
{
	if (!name) {
		write_log("my_opendir: null directory name provided\n");
		return nullptr;
	}

	auto mod = std::make_unique<my_opendir_s>();
	if (!mod) {
		write_log("my_opendir: memory allocation failed\n");
		return nullptr;
	}

	const auto output = iso_8859_1_to_utf8(std::string(name));
	mod->dir = opendir(output.c_str());

	if (!mod->dir) {
		write_log("my_opendir: opendir on directory %s failed\n", name);
		return nullptr;
	}

	return mod.release();
}

struct my_opendir_s* my_opendir(const TCHAR* name) {
	return my_opendir(name, _T("*.*"));
}

void my_closedir(struct my_opendir_s* mod)
{
	if (!mod) {
		return;
	}

	if (mod->dir && closedir(mod->dir) != 0) {
		write_log("my_closedir: closedir on directory failed: %s\n", strerror(errno));
	}

	delete mod;
}

int my_readdir(struct my_opendir_s* mod, TCHAR* name)
{
	if (!mod || !name) {
		write_log("my_readdir: null argument(s) provided\n");
		return 0;
	}

	static const std::set<std::string> ignoreList = { "_UAEFSDB.___", "Thumbs.db", ".DS_Store", "UAEFS.ini" };

	while (true) {
		auto* entry = readdir(mod->dir);
		if (!entry) {
			return 0;
		}

		const std::string result(entry->d_name);
		const int len = result.length();

		if (ignoreList.find(result) != ignoreList.end() ||
			(len > 5 && result.compare(len - 5, 5, ".uaem") == 0)) {
			continue;
		}

		std::string string_output;
		if (!utf8_to_latin1_string(result, string_output)) {
			write_log("my_readdir: utf8_to_latin1_string conversion failed for %s\n", result.c_str());
			continue;
		}

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

	struct stat st{};
	if (lstat(name, &st) != 0) {
		write_log("my_existslink: lstat on file %s failed: %s\n", name, strerror(errno));
		return false;
	}

	return S_ISLNK(st.st_mode);
}

bool my_existsfile2(const char* name)
{
	if (!name) return false;
	return fs::exists(name) && fs::is_regular_file(name);
}

bool my_existsfile(const char* name)
{
	if (!name) return false;
	const auto output = iso_8859_1_to_utf8(string(name));
	return fs::exists(output) && fs::is_regular_file(output);
}

bool my_existsdir(const char* name)
{
	if (!name) return false;
	const auto output = iso_8859_1_to_utf8(string(name));
	return fs::exists(output) && fs::is_directory(output);
}

uae_s64 my_fsize(struct my_openfile_s* mos)
{
	if (!mos) {
		write_log("my_fsize: null pointer provided\n");
		return -1;
	}

	struct stat st{};
	if (fstat(mos->fd, &st) != 0) {
		write_log("my_fsize: fstat on file %s failed: %s\n", mos->path, strerror(errno));
		return -1;
	}

	return static_cast<uae_s64>(st.st_size);
}

int my_getvolumeinfo(const char* root)
{
	if (!root) {
		write_log("my_getvolumeinfo: null pointer provided\n");
		return -1;
	}

	struct stat st{};
	if (lstat(root, &st) != 0) {
		write_log("my_getvolumeinfo: lstat on file %s failed: %s\n", root, strerror(errno));
		return -1;
	}

	if (!S_ISDIR(st.st_mode)) {
		write_log("my_getvolumeinfo: %s is not a directory\n", root);
		return -2;
	}

	return MYVOLUMEINFO_STREAMS;
}

bool fs_path_exists(const std::string& s)
{
	if (s.empty()) {
		write_log("fs_path_exists: empty string provided\n");
		return false;
	}

	const auto output = iso_8859_1_to_utf8(s);
	struct stat buffer{};
	return stat(output.c_str(), &buffer) == 0;
}

struct my_openfile_s* my_open(const TCHAR* name, int flags)
{
	if (!name) {
		write_log("my_open: null pointer provided\n");
		return nullptr;
	}

	auto mos = std::make_unique<my_openfile_s>();
	if (!mos) {
		write_log("my_open: memory allocation failed\n");
		return nullptr;
	}

	const auto output = iso_8859_1_to_utf8(std::string(name));
	mos->fd = (flags & O_CREAT) ? open(output.c_str(), flags, 0660) : open(output.c_str(), flags);

	if (mos->fd == -1) {
		write_log("my_open: open on file %s failed: %s\n", name, strerror(errno));
		return nullptr;
	}

	mos->path = strdup(name);
	return mos.release();
}

void my_close(struct my_openfile_s* mos)
{
	if (!mos) {
		return;
	}

	if (close(mos->fd) != 0) {
		write_log("my_close: close on file %s failed: %s\n", mos->path, strerror(errno));
	}

	free(mos->path);
	delete mos;
}

unsigned int my_read(struct my_openfile_s* mos, void* b, unsigned int size)
{
	if (!mos) {
		write_log("my_read: null file pointer provided\n");
		return 0;
	}

	if (!b) {
		write_log("my_read: null buffer pointer provided\n");
		return 0;
	}

	ssize_t bytes_read = read(mos->fd, b, size);
	if (bytes_read == -1) {
		write_log("my_read: read on file %s failed with error %s\n", mos->path, strerror(errno));
		return 0;
	}

	if (bytes_read < size) {
		write_log("my_read: read on file %s returned less bytes than requested\n", mos->path);
	}

	return static_cast<unsigned int>(bytes_read);
}

[[nodiscard]] unsigned int my_write(struct my_openfile_s* mos, void* b, unsigned int size)
{
	// Early validation with combined null check message
	if (mos == nullptr || b == nullptr) {
		write_log("my_write: %s\n",
			mos == nullptr ? "null file pointer provided" : "null buffer pointer provided");
		return 0;
	}

	const char* buffer = static_cast<const char*>(b);
	unsigned int total_written = 0;

	// Handle partial writes with retry logic
	while (total_written < size) {
		const ssize_t bytes_written = write(mos->fd,
			buffer + total_written,
			size - total_written);

		if (bytes_written == -1) {
			// Check for interruption and retry
			if (errno == EINTR) {
				continue;
			}

			write_log("my_write: write on file %s failed with error %s after %u bytes\n",
				mos->path, strerror(errno), total_written);
			return total_written; // Return partial progress on error
		}

		if (bytes_written == 0) {
			// No more data can be written
			write_log("my_write: write on file %s stopped after %u of %u bytes\n",
				mos->path, total_written, size);
			break;
		}

		total_written += static_cast<unsigned int>(bytes_written);
	}

	// Only log if we didn't write everything but didn't hit an error
	if (total_written < size) {
		write_log("my_write: write on file %s wrote %u of %u bytes requested\n",
			mos->path, total_written, size);
	}

	return total_written;
}

[[nodiscard]] int my_mkdir(const TCHAR* path)
{
	if (path == nullptr) {
		write_log("my_mkdir: null path provided\n");
		return -1;
	}

	try {
		// Convert input path to UTF-8 for filesystem operations
		const auto utf8_path = iso_8859_1_to_utf8(std::string_view(path));

		// Check if directory already exists to provide better error reporting
		std::error_code ec;
		if (fs::exists(utf8_path, ec)) {
			if (fs::is_directory(utf8_path, ec)) {
				write_log("my_mkdir: directory %s already exists\n", path);
				return -1;
			}
			write_log("my_mkdir: path %s exists but is not a directory\n", path);
			return -1;
		}

		// Create all parent directories if they don't exist
		fs::path parent = fs::path(utf8_path).parent_path();
		if (!parent.empty() && !fs::exists(parent, ec)) {
			if (!fs::create_directories(parent, ec)) {
				write_log("my_mkdir: failed to create parent directories for %s: %s\n",
					path, ec.message().c_str());
				return -1;
			}
		}

		// Create the actual directory with proper permissions
		if (mkdir(utf8_path.c_str(), 0755) != 0) {
			write_log("my_mkdir: mkdir on path %s failed: %s\n",
				path, strerror(errno));
			return -1;
		}

		return 0;
	}
	catch (const std::exception& e) {
		write_log("my_mkdir: exception while creating directory %s: %s\n",
			path, e.what());
		return -1;
	}
}

[[nodiscard]] int my_truncate(const TCHAR* name, uae_u64 len)
{
	if (name == nullptr) {
		write_log("my_truncate: null path provided\n");
		return -1;
	}

	try {
		// Convert path to UTF-8 for filesystem operations
		const auto utf8_path = iso_8859_1_to_utf8(std::string_view(name));

		// Check if file exists and is writable
		std::error_code ec;
		if (!fs::exists(utf8_path, ec)) {
			write_log("my_truncate: file %s does not exist\n", name);
			return -1;
		}

		if (!fs::is_regular_file(utf8_path, ec)) {
			write_log("my_truncate: %s is not a regular file\n", name);
			return -1;
		}

		// Use RAII to ensure file handle is properly closed
		std::unique_ptr<my_openfile_s, decltype(&my_close)> mos(
			my_open(name, O_WRONLY),
			my_close
		);

		if (!mos) {
			write_log("my_truncate: failed to open file %s for truncation\n", name);
			return -1;
		}

#ifdef WINDOWS
		if (len > static_cast<uae_u64>(INT_MAX)) {
			write_log("my_truncate: length %llu exceeds maximum file size on Windows\n", len);
			return -1;
		}
		const int result = _chsize(mos->fd, static_cast<int>(len));
#else
		const int result = ftruncate(mos->fd, static_cast<off_t>(len));
#endif

		if (result == -1) {
			write_log("my_truncate: truncating file %s to size %llu failed: %s\n",
				name, len, strerror(errno));
			return -1;
		}

		return 0;
	}
	catch (const std::exception& e) {
		write_log("my_truncate: exception while truncating %s: %s\n",
			name, e.what());
		return -1;
	}
}

[[nodiscard]] static bool remove_extra_file(const char* path, const char* name) noexcept
{
	if (!path || !name) {
		write_log("remove_extra_file: null argument(s) provided\n");
		return false;
	}

	try {
		// Create filesystem path and handle path concatenation properly
		fs::path filepath = fs::path(path);
		if (filepath.empty()) {
			write_log("remove_extra_file: empty path provided\n");
			return false;
		}

		// Make sure path ends with exactly one separator
		if (!filepath.string().empty() && filepath.string().back() != '/') {
			filepath /= "";  // This adds a proper path separator
		}

		// Append filename using proper path concatenation
		filepath /= name;

		// Check if file exists before attempting removal
		std::error_code ec;
		if (!fs::exists(filepath, ec)) {
			// Not considering it an error if file doesn't exist
			return true;
		}

		if (!fs::is_regular_file(filepath, ec)) {
			write_log("remove_extra_file: %s is not a regular file\n", filepath.c_str());
			return false;
		}

		// Attempt to remove the file
		if (!fs::remove(filepath, ec)) {
			write_log("remove_extra_file: failed to remove %s: %s\n",
				filepath.c_str(), ec.message().c_str());
			return false;
		}

		return true;
	}
	catch (const std::exception& e) {
		write_log("remove_extra_file: exception while removing %s/%s: %s\n",
			path, name, e.what());
		return false;
	}
}

[[nodiscard]] bool my_isfilehidden(const char* path) noexcept
{
	if (!path) {
		write_log("my_isfilehidden: null path provided\n");
		return false;
	}

	try {
		const fs::path filepath(path);

		if (filepath.empty()) {
			write_log("my_isfilehidden: empty path provided\n");
			return false;
		}

#ifdef _WIN32
		// On Windows, check the file attributes
		std::error_code ec;
		const auto attrs = fs::status(filepath, ec).permissions();
		if (ec) {
			write_log("my_isfilehidden: failed to get attributes for %s: %s\n",
				path, ec.message().c_str());
			return false;
		}
		return (attrs & fs::perms::hidden) != fs::perms::none;
#else
		// On Unix-like systems, check if filename starts with '.'
		const std::string filename = filepath.filename().string();
		return !filename.empty() && filename[0] == '.';
#endif
	}
	catch (const std::exception& e) {
		write_log("my_isfilehidden: exception while checking %s: %s\n",
			path, e.what());
		return false;
	}
}

[[nodiscard]] bool my_setfilehidden(const TCHAR* path, bool hidden) noexcept
{
	if (!path) {
		write_log("my_setfilehidden: null path provided\n");
		return false;
	}

	try {
		// Convert to filesystem path and validate
		const auto utf8_path = iso_8859_1_to_utf8(std::string_view(path));
		fs::path filepath(utf8_path);

		if (filepath.empty()) {
			write_log("my_setfilehidden: empty path provided\n");
			return false;
		}

		std::error_code ec;
		if (!fs::exists(filepath, ec)) {
			write_log("my_setfilehidden: file %s does not exist\n", path);
			return false;
		}

#ifdef _WIN32
		// Windows implementation using file attributes
		const DWORD attrs = GetFileAttributes(path);
		if (attrs == INVALID_FILE_ATTRIBUTES) {
			write_log("my_setfilehidden: GetFileAttributes failed for %s\n", path);
			return false;
		}

		const DWORD new_attrs = hidden ?
			(attrs | FILE_ATTRIBUTE_HIDDEN) :
			(attrs & ~FILE_ATTRIBUTE_HIDDEN);

		if (!SetFileAttributes(path, new_attrs)) {
			write_log("my_setfilehidden: SetFileAttributes failed for %s\n", path);
			return false;
		}
#else
		// Unix implementation using dot prefix
		const fs::path filename = filepath.filename();
		const std::string name = filename.string();

		if (name.empty() || name == "." || name == "..") {
			write_log("my_setfilehidden: invalid filename %s\n", path);
			return false;
		}

		const bool is_currently_hidden = !name.empty() && name[0] == '.';
		if (is_currently_hidden == hidden) {
			// Already in desired state
			return true;
		}

		fs::path new_path = filepath.parent_path();
		if (hidden && !is_currently_hidden) {
			new_path /= "." + name;
		}
		else if (!hidden && is_currently_hidden) {
			new_path /= name.substr(1);
		}

		fs::rename(filepath, new_path, ec);
		if (ec) {
			write_log("my_setfilehidden: rename failed for %s: %s\n",
				path, ec.message().c_str());
			return false;
		}
#endif
		return true;
	}
	catch (const std::exception& e) {
		write_log("my_setfilehidden: exception while processing %s: %s\n",
			path, e.what());
		return false;
	}
}

[[nodiscard]] int my_rmdir(const TCHAR* path) noexcept
{
	if (!path) {
		write_log("my_rmdir: null directory path provided\n");
		return -1;
	}

	try {
		// Convert path to UTF-8 and create filesystem path
		const auto utf8_path = iso_8859_1_to_utf8(std::string_view(path));
		fs::path dirpath(utf8_path);

		// Validate path
		std::error_code ec;
		if (!fs::exists(dirpath, ec)) {
			write_log("my_rmdir: directory %s does not exist\n", path);
			return -1;
		}

		if (!fs::is_directory(dirpath, ec)) {
			write_log("my_rmdir: path %s is not a directory\n", path);
			return -1;
		}

		// First try to remove common system files that might prevent directory deletion
		const std::array<const char*, 2> extra_files = { "Thumbs.db", ".DS_Store" };
		for (const auto& file : extra_files) {
			remove_extra_file(path, file);
		}

		// Check if directory is empty (excluding . and ..)
		bool is_empty = true;
		for (const auto& entry : fs::directory_iterator(dirpath, ec)) {
			is_empty = false;
			break;
		}

		if (!is_empty) {
			write_log("my_rmdir: directory %s is not empty\n", path);
			return -1;
		}

		// Remove the directory
		errno = 0;
		int result = rmdir(utf8_path.c_str());

		if (result != 0) {
			write_log("my_rmdir: rmdir on directory %s failed: %s\n",
				path, strerror(errno));
			return -1;
		}

		return 0;
	}
	catch (const std::exception& e) {
		write_log("my_rmdir: exception while removing directory %s: %s\n",
			path, e.what());
		return -1;
	}
}

[[nodiscard]] int my_unlink(const TCHAR* path) noexcept
{
	if (!path) {
		write_log("my_unlink: null file path provided\n");
		return -1;
	}

	try {
		// Convert to filesystem path and validate
		const auto utf8_path = iso_8859_1_to_utf8(std::string_view(path));
		fs::path filepath(utf8_path);

		if (filepath.empty()) {
			write_log("my_unlink: empty path provided\n");
			return -1;
		}

		// Check file existence and type
		std::error_code ec;
		if (!fs::exists(filepath, ec)) {
			write_log("my_unlink: file %s does not exist\n", path);
			return -1;
		}

		if (!fs::is_regular_file(filepath, ec)) {
			write_log("my_unlink: %s is not a regular file\n", path);
			return -1;
		}

		// Ensure we have write permission to delete
		if (!fs::is_regular_file(filepath) || (fs::status(filepath, ec).permissions() & fs::perms::owner_all) == fs::perms::none) {
			write_log("my_unlink: insufficient permissions to remove %s\n", path);
			return -1;
		}

		// Attempt to remove the file
		errno = 0;
		if (!fs::remove(filepath, ec)) {
			write_log("my_unlink: remove failed for %s: %s\n",
				path, ec.message().c_str());
			return -1;
		}

		return 0;
	}
	catch (const std::exception& e) {
		write_log("my_unlink: exception while removing %s: %s\n",
			path, e.what());
		return -1;
	}
}

[[nodiscard]] int my_rename(const TCHAR* oldname, const TCHAR* newname) noexcept
{
	// Input validation with descriptive error message
	if (oldname == nullptr || newname == nullptr) {
		write_log("my_rename: invalid arguments - %s\n",
			oldname == nullptr ? "old filename is null" : "new filename is null");
		return -1;
	}

	if (*oldname == '\0' || *newname == '\0') {
		write_log("my_rename: empty filename provided\n");
		return -1;
	}

	try {
		// Convert paths to UTF-8 format
		const auto old_output = iso_8859_1_to_utf8(std::string_view(oldname));
		const auto new_output = iso_8859_1_to_utf8(std::string_view(newname));

		// Check if paths are valid after conversion
		if (old_output.empty() || new_output.empty()) {
			write_log("my_rename: path conversion failed\n");
			return -1;
		}

		// Reset errno before operation
		errno = 0;

		// Perform rename operation
		const int result = rename(old_output.c_str(), new_output.c_str());

		// Enhanced error reporting with specific error codes
		if (result != 0) {
			const char* error_msg;
			switch (errno) {
			case EACCES:
				error_msg = "Permission denied";
				break;
			case ENOENT:
				error_msg = "Source file doesn't exist";
				break;
			case EEXIST:
				error_msg = "Destination file already exists";
				break;
			case EINVAL:
				error_msg = "Invalid argument";
				break;
			default:
				error_msg = strerror(errno);
			}
			write_log("my_rename: rename from '%s' to '%s' failed: %s (errno=%d)\n",
				oldname, newname, error_msg, errno);
		}

		return result;
	}
	catch (const std::exception& e) {
		write_log("my_rename: unexpected error while renaming '%s' to '%s': %s\n",
			oldname, newname, e.what());
		return -1;
	}
}

[[nodiscard]] uae_s64 my_lseek(struct my_openfile_s* mos, uae_s64 offset, int whence) noexcept
{
	// Input validation with detailed error reporting
	if (mos == nullptr) {
		write_log("my_lseek: null file descriptor provided\n");
		return -1;
	}

	if (mos->fd < 0 || mos->path == nullptr) {
		write_log("my_lseek: invalid file descriptor state (fd=%d, path=%s)\n",
			mos->fd, mos->path ? mos->path : "null");
		return -1;
	}

	// Validate whence parameter
	if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END) {
		write_log("my_lseek: invalid whence parameter %d for file %s\n",
			whence, mos->path);
		return -1;
	}

	// Reset errno before operation
	errno = 0;

	// Perform seek operation with proper type casting
	const auto result = static_cast<uae_s64>(
		lseek(mos->fd, static_cast<off_t>(offset), whence)
		);

	// Enhanced error reporting with specific error conditions
	if (result == -1) {
		const char* whence_str = whence == SEEK_SET ? "SEEK_SET" :
			whence == SEEK_CUR ? "SEEK_CUR" :
			whence == SEEK_END ? "SEEK_END" : "unknown";

		const char* error_msg;
		switch (errno) {
		case EINVAL:
			error_msg = "Invalid offset or whence parameter";
			break;
		case EBADF:
			error_msg = "Bad file descriptor";
			break;
		case ESPIPE:
			error_msg = "Resource is a pipe or FIFO";
			break;
		case ENXIO:
			error_msg = "Device does not support seek";
			break;
		default:
			error_msg = strerror(errno);
		}

		write_log("my_lseek: seek failed on file %s (offset=%lld, whence=%s): %s\n",
			mos->path, static_cast<long long>(offset), whence_str, error_msg);
	}

	return result;
}

[[nodiscard]] FILE* my_opentext(const TCHAR* name) noexcept
{
	// Input validation with descriptive error message
	if (name == nullptr || *name == '\0') {
		write_log("my_opentext: %s\n",
			name == nullptr ? "null filename provided" : "empty filename provided");
		return nullptr;
	}

	try {
		// Convert path to UTF-8 using string_view for efficiency
		const auto utf8_path = iso_8859_1_to_utf8(std::string_view(name));
		if (utf8_path.empty()) {
			write_log("my_opentext: path conversion failed for '%s'\n", name);
			return nullptr;
		}

		// Check if file exists and is readable before attempting to open
		std::error_code ec;
		if (!fs::exists(utf8_path, ec)) {
			write_log("my_opentext: file '%s' does not exist\n", name);
			return nullptr;
		}

		if (!fs::is_regular_file(utf8_path, ec)) {
			write_log("my_opentext: '%s' is not a regular file\n", name);
			return nullptr;
		}

		// Reset errno before operation
		errno = 0;

		// Open file with binary mode and read-only access
		FILE* file = fopen(utf8_path.c_str(), "rb");

		// Enhanced error reporting
		if (file == nullptr) {
			const char* error_msg;
			switch (errno) {
			case EACCES:
				error_msg = "Permission denied";
				break;
			case EMFILE:
				error_msg = "Too many open files";
				break;
			case ENOMEM:
				error_msg = "Out of memory";
				break;
			default:
				error_msg = strerror(errno);
			}
			write_log("my_opentext: failed to open '%s': %s (errno=%d)\n",
				name, error_msg, errno);
		}

		return file;
	}
	catch (const std::exception& e) {
		write_log("my_opentext: exception while opening '%s': %s\n",
			name, e.what());
		return nullptr;
	}
}

bool my_createshortcut(const char* source, const char* target, const char* description)
{
	return false;
}

[[nodiscard]] bool my_resolvesoftlink(TCHAR* linkfile, int size, bool linkonly) noexcept
{
	// Input validation
	if (linkfile == nullptr || size <= 0) {
		write_log("my_resolvesoftlink: invalid arguments (linkfile=%p, size=%d)\n",
			static_cast<void*>(linkfile), size);
		return false;
	}

	try {
		// Convert path to UTF-8 using string_view for efficiency
		const auto utf8_path = iso_8859_1_to_utf8(std::string_view(linkfile));
		if (utf8_path.empty()) {
			write_log("my_resolvesoftlink: path conversion failed for '%s'\n", linkfile);
			return false;
		}

		// Create filesystem path
		fs::path link_path(utf8_path);

		// Check if path exists and is a symlink
		std::error_code ec;
		if (!fs::exists(link_path, ec)) {
			write_log("my_resolvesoftlink: '%s' does not exist\n", linkfile);
			return false;
		}

		if (!fs::is_symlink(link_path, ec)) {
			if (linkonly) {
				write_log("my_resolvesoftlink: '%s' is not a symbolic link\n", linkfile);
				return false;
			}
			// Not a symlink but linkonly=false, so return success without modification
			return true;
		}

		// Read the symlink target
		fs::path target_path = fs::read_symlink(link_path, ec);
		if (ec) {
			write_log("my_resolvesoftlink: failed to read symlink '%s': %s\n",
				linkfile, ec.message().c_str());
			return false;
		}

		// Convert target path to absolute if it's relative
		if (target_path.is_relative()) {
			target_path = fs::absolute(link_path.parent_path() / target_path, ec);
			if (ec) {
				write_log("my_resolvesoftlink: failed to resolve absolute path for '%s': %s\n",
					linkfile, ec.message().c_str());
				return false;
			}
		}

		// Convert target path back to Latin1
		std::string target_latin1;
		if (!utf8_to_latin1_string(target_path.string(), target_latin1)) {
			write_log("my_resolvesoftlink: character conversion failed for target path\n");
			return false;
		}

		// Check buffer size
		if (static_cast<int>(target_latin1.length()) >= size) {
			write_log("my_resolvesoftlink: target path too long for buffer (need %zu, have %d)\n",
				target_latin1.length() + 1, size);
			return false;
		}

		// Copy resolved path to output buffer
		_tcsncpy(linkfile, target_latin1.c_str(), size - 1);
		linkfile[size - 1] = '\0';

		return true;
	}
	catch (const std::exception& e) {
		write_log("my_resolvesoftlink: exception while resolving '%s': %s\n",
			linkfile, e.what());
		return false;
	}
}

[[nodiscard]] const TCHAR* my_getfilepart(const TCHAR* filename) noexcept
{
	if (!filename) {
		return nullptr;
	}

	const TCHAR* last_separator = nullptr;
	const TCHAR* p = filename;

	// Single pass through the string to find the last separator
	while (*p) {
		if (*p == '/' || *p == '\\') {
			last_separator = p;
		}
		++p;
	}

	return last_separator ? last_separator + 1 : filename;
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

void my_canonicalize_path(const char* path, char* out, int size)
{
	if (!path || !out) {
		write_log("my_canonicalize_path: null arguments provided\n");
		return;
	}

	if (size <= 0) {
		write_log("my_canonicalize_path: invalid size provided\n");
		return;
	}

	// Ensure the path is not longer than the destination buffer
	_tcsncpy(out, path, size - 1);

	// Always ensure the output is null-terminated
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

	std::string p1(path1);
	std::string p2(path2);

	my_canonicalize_path(p1.c_str(), p1.data(), p1.size());
	my_canonicalize_path(p2.c_str(), p2.data(), p2.size());

	const auto len = std::min(p1.size(), p2.size());

	if (!std::equal(p1.begin(), p1.begin() + len, p2.begin(), p2.begin() + len,
		[](const char a, const char b) { return std::tolower(a) == std::tolower(b); })) {
		return 0;
	}

	_tcscpy(path, (p2.c_str() + len));

	unsigned int cnt = 0;
	for (unsigned int i = 0; i < std::strlen(path); i++) {
		if (path[i] == '\\' || path[i] == '/') {
			path[i] = '/';
			cnt++;
		}
	}

	write_log("'%s' (%s) matched with '%s' (%s), extra = '%s'\n", path1, p1.c_str(), path2, p2.c_str(), path);
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
	_sntprintf(ci->volname, sizeof ci->volname, "DH_%c", ci->rootdir[0]);
	return 2;
}

// If replace is false, copyfile will fail if file already exists
bool copyfile(const char* target, const char* source, const bool replace)
{
    // Input validation
    if (!target || !source) {
        write_log("copyfile: null path provided\n");
        return false;
    }

    try {
        // Convert paths to UTF-8 for filesystem operations
        const auto source_path = iso_8859_1_to_utf8(std::string_view(source));
        const auto target_path = iso_8859_1_to_utf8(std::string_view(target));

        fs::path src_fs(source_path);
        fs::path dst_fs(target_path);

        // Check if source file exists
        std::error_code ec;
        if (!fs::exists(src_fs, ec)) {
            write_log("copyfile: source file '%s' does not exist\n", source);
            return false;
        }

        // Check if source is a regular file
        if (!fs::is_regular_file(src_fs, ec)) {
            write_log("copyfile: source '%s' is not a regular file\n", source);
            return false;
        }

        // Avoid unnecessary copy if source and destination are the same file
        if (fs::equivalent(src_fs, dst_fs, ec)) {
            return true;
        }

        // Ensure target directory exists
        fs::path parent = dst_fs.parent_path();
        if (!parent.empty() && !fs::exists(parent, ec)) {
            if (!fs::create_directories(parent, ec)) {
                write_log("copyfile: failed to create parent directory for '%s': %s\n",
                         target, ec.message().c_str());
                return false;
            }
        }

        // Set copy options
        const fs::copy_options options = replace
            ? fs::copy_options::overwrite_existing
            : fs::copy_options::none;

        // Copy file with proper error reporting
        if (!fs::copy_file(src_fs, dst_fs, options, ec)) {
            write_log("copyfile: failed to copy from '%s' to '%s': %s\n",
                     source, target, ec.message().c_str());
            return false;
        }

        return true;
    }
    catch (const std::exception& e) {
        write_log("copyfile: exception while copying from '%s' to '%s': %s\n",
                 source, target, e.what());
        return false;
    }
}

void filesys_addexternals(void)
{
	if (!currprefs.automount_cddrives)
		return;

	const auto cd_drives = get_cd_drives();
	if (!cd_drives.empty())
	{
		int drvnum = 0;
		for (auto& drive : cd_drives)
		{
			struct uaedev_config_info ci = {};
			_tcscpy(ci.rootdir, drive.c_str());
			ci.readonly = true;
			ci.bootpri = -20 - drvnum;
			add_filesys_unit(&ci, true);
			drvnum++;
		}
	}
}

std::string my_get_sha1_of_file(const char* filepath)
{
    // Input validation
    if (!filepath) {
        write_log("my_get_sha1_of_file: null file path provided\n");
        return "";
    }

    try {
        // Convert path to UTF-8 for filesystem operations
        const auto utf8_path = iso_8859_1_to_utf8(std::string_view(filepath));

        // Check file existence before attempting operations
        std::error_code ec;
        fs::path file_path(utf8_path);
        if (!fs::exists(file_path, ec) || !fs::is_regular_file(file_path, ec)) {
            write_log("my_get_sha1_of_file: '%s' does not exist or is not a regular file\n", filepath);
            return "";
        }

        // Use RAII for file descriptor
        struct FileDescriptor {
            int fd = -1;
            FileDescriptor(const char* path) : fd(open(path, O_RDONLY)) {}
            ~FileDescriptor() { if (fd >= 0) close(fd); }
            bool valid() const { return fd >= 0; }
        } file(utf8_path.c_str());

        if (!file.valid()) {
            write_log("my_get_sha1_of_file: open on file '%s' failed: %s\n",
                     filepath, strerror(errno));
            return "";
        }

        // Get file size
        struct stat sb;
        if (fstat(file.fd, &sb) < 0) {
            write_log("my_get_sha1_of_file: fstat on file '%s' failed: %s\n",
                     filepath, strerror(errno));
            return "";
        }

        // Handle empty files specially
        if (sb.st_size == 0) {
            write_log("my_get_sha1_of_file: '%s' is an empty file\n", filepath);
            return get_sha1_txt(nullptr, 0);
        }

        // Use RAII for memory mapping
        struct MappedMemory {
            void* mem = MAP_FAILED;
            size_t size = 0;

            MappedMemory(int fd, size_t sz) : size(sz) {
                mem = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
            }

            ~MappedMemory() {
                if (mem != MAP_FAILED) munmap(mem, size);
            }

            bool valid() const { return mem != MAP_FAILED; }
        } mapping(file.fd, static_cast<size_t>(sb.st_size));

        if (!mapping.valid()) {
            write_log("my_get_sha1_of_file: mmap on file '%s' failed: %s\n",
                     filepath, strerror(errno));
            return "";
        }

        // Calculate SHA1
        const TCHAR* sha1 = get_sha1_txt(mapping.mem, sb.st_size);
        if (!sha1) {
            write_log("my_get_sha1_of_file: SHA1 calculation failed for '%s'\n", filepath);
            return "";
        }

        return std::string(sha1);
    }
    catch (const std::exception& e) {
        write_log("my_get_sha1_of_file: exception processing '%s': %s\n",
                 filepath, e.what());
        return "";
    }
}
