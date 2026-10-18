#!/bin/sh
set -eu

cxx="${CXX:-c++}"
out="${TMPDIR:-/tmp}/curl_ca_bundle_test.$$"
trap 'rm -f "$out"' EXIT

curl_flags="$(pkg-config --cflags --libs libcurl)"
"$cxx" -std=c++17 -Wall -Wextra -Werror -DAMIBERRY_HAS_CURL -Isrc/osdep \
	-o "$out" \
	tests/curl_ca_bundle_test.cpp \
	src/osdep/amiberry_curl.cpp \
	$curl_flags
"$out"
