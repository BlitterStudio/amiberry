#pragma once

#include <cstddef>
#include <cstdint>

#include "utilfwd.h"

#define CHD_FLAC_USE_DRFLAC 1

using FLAC__StreamEncoderState = int;
using FLAC__StreamDecoderState = int;
using FLAC__byte = uint8_t;

struct drflac_memory_stream {
	const uint8_t* data{};
	size_t size{};
	size_t cursor{};
};
struct drflac_file_stream {
	util::read_stream* file{};
	util::random_access* random{};
	uint64_t size{};
	uint64_t cursor{};
};
