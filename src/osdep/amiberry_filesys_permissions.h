#pragma once

#include <sys/stat.h>
#include <sys/types.h>

inline mode_t amiberry_filesys_apply_protection_bits(
	const mode_t host_mode,
	const bool read_protected,
	const bool write_protected,
	const bool execute_protected)
{
	mode_t mode = host_mode;
	const bool is_directory = S_ISDIR(host_mode);

	mode = read_protected ? (mode & ~S_IRUSR) : (mode | S_IRUSR);
	mode = write_protected ? (mode & ~S_IWUSR) : (mode | S_IWUSR);

	if (is_directory) {
		const mode_t execute_bits = S_IXUSR | S_IXGRP | S_IXOTH;
		mode = execute_protected ? (mode & ~execute_bits) : (mode | execute_bits);
	} else if (execute_protected) {
		// Amiga executability is metadata, not a request to make every host file
		// a native executable. Preserve existing execute bits unless protection
		// explicitly removes them.
		mode &= ~(S_IXUSR | S_IXGRP | S_IXOTH);
	}

	if (mode & S_IRUSR) {
		mode |= S_IRGRP | S_IROTH;
	}

	return mode;
}
