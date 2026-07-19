#!/usr/bin/env bash
set -euo pipefail

source_file="src/osdep/amiberry_rp9.cpp"

if ! grep -F -q 'register_extracted_directories(files, directory, std::filesystem::path(normalized))' "$source_file"; then
	echo "RP9 extraction must register explicit embedded directory entries" >&2
	exit 1
fi

if ! grep -F -q 'std::filesystem::path(normalized).parent_path()))' "$source_file"; then
	echo "RP9 extraction must register directories synthesized for nested files" >&2
	exit 1
fi

if ! grep -F -q 'register_extracted_path(files, normalized, destination)' "$source_file"; then
	echo "RP9 extraction must reject portable file/directory path collisions" >&2
	exit 1
fi
