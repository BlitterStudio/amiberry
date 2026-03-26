#pragma once

#include <string>

// Darwin-only helper API for raw-disk authentication handle creation.
// Returns a file descriptor on success, or -1 on failure with error populated.
int macos_authopen_fd(const char* path, int open_flags, std::string& error);
