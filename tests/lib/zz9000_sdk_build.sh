# Sourced by the ZZ9000 SDK test runners.
# zz9000_sdk_compile OUT TEST_SOURCE [EXTRA_PKG_CONFIG_MODULES] [EXTRA_FLAGS...]
# Builds the test against every SDK translation unit. Optional host codecs
# are enabled when their development packages are installed, as in CMake.
zz9000_sdk_compile() {
	local out="$1" test_source="$2" modules="sdl3 ${3:-}"
	shift 3 || shift $#
	local defines=()
	if pkg-config --exists libavformat libavcodec libavutil libswresample; then
		modules="$modules libavformat libavcodec libavutil libswresample"
		defines+=(-DAMIBERRY_WITH_FFMPEG)
	fi
	if pkg-config --exists libwebp libwebpdemux; then
		modules="$modules libwebp libwebpdemux"
		defines+=(-DAMIBERRY_WITH_WEBP)
	fi
	# shellcheck disable=SC2086
	${CXX:-c++} -std=c++17 -Isrc -Isrc/include -Isrc/osdep "${defines[@]}" "$@" \
		$(pkg-config --cflags $modules) \
		-o "$out" "$test_source" src/zz9000_sdk*.cpp \
		$(pkg-config --libs $modules) -lz -lpthread
}
