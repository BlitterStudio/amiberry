#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/zz9000_sdk_media_test.$$"
video="${TMPDIR:-/tmp}/zz9000_sdk_media_test.$$.mpg"
audio_only="${TMPDIR:-/tmp}/zz9000_sdk_media_test.$$.audio.mpg"
vp8="${TMPDIR:-/tmp}/zz9000_sdk_media_test.$$.vp8.webm"
vp9="${TMPDIR:-/tmp}/zz9000_sdk_media_test.$$.vp9.webm"
mkv="${TMPDIR:-/tmp}/zz9000_sdk_media_test.$$.mkv"
ps="${TMPDIR:-/tmp}/zz9000_sdk_media_test.$$.vob"
lpcm="${TMPDIR:-/tmp}/zz9000_sdk_media_test.$$.lpcm.vob"
audio_ac3="${TMPDIR:-/tmp}/zz9000_sdk_media_test.$$.audio.ac3.mpg"
interlaced="${TMPDIR:-/tmp}/zz9000_sdk_media_test.$$.interlaced.vob"
large="${TMPDIR:-/tmp}/zz9000_sdk_media_test.$$.large.vob"
large_odd_half="${TMPDIR:-/tmp}/zz9000_sdk_media_test.$$.large_odd_half.vob"
truncated="${TMPDIR:-/tmp}/zz9000_sdk_media_test.$$.truncated.webm"
trap 'rm -f "$out" "$video" "$audio_only" "$vp8" "$vp9" "$mkv" "$ps" "$lpcm" "$audio_ac3" "$interlaced" "$large" "$large_odd_half" "$truncated"' EXIT
ffmpeg -nostdin -loglevel error -y -f lavfi -i 'testsrc2=size=160x120:rate=25:duration=1' -f lavfi -i 'sine=frequency=440:sample_rate=44100:duration=1' -c:v mpeg1video -q:v 4 -c:a mp2 -b:a 128k -f mpeg "$video"
ffmpeg -nostdin -loglevel error -y -f lavfi -i 'sine=frequency=440:sample_rate=44100:duration=1' -c:a mp2 -b:a 128k -f mpeg "$audio_only"
ffmpeg -nostdin -loglevel error -y -f lavfi -i 'testsrc2=size=160x120:rate=25:duration=3' -f lavfi -i 'sine=frequency=440:sample_rate=48000:duration=3' -c:v libvpx -deadline realtime -cpu-used 8 -c:a libvorbis "$vp8"
ffmpeg -nostdin -loglevel error -y -f lavfi -i 'testsrc2=size=160x120:rate=25:duration=3' -f lavfi -i 'sine=frequency=440:sample_rate=48000:duration=3' -c:v libvpx-vp9 -row-mt 1 -cpu-used 8 -c:a libopus "$vp9"
ffmpeg -nostdin -loglevel error -y -f lavfi -i 'testsrc2=size=160x120:rate=25:duration=1' -f lavfi -i 'sine=frequency=44100:duration=1' -c:v mpeg2video -pix_fmt yuv420p -c:a mp2 "$mkv"
ffmpeg -nostdin -loglevel error -y -f lavfi -i 'testsrc2=size=160x120:rate=25:duration=1' -f lavfi -i 'sine=frequency=440:sample_rate=48000:duration=1' -c:v mpeg2video -pix_fmt yuv420p -c:a ac3 -f vob "$ps"
ffmpeg -nostdin -loglevel error -y -f lavfi -i 'sine=frequency=440:sample_rate=48000:duration=1' -c:a ac3 -f vob "$audio_ac3"
ffmpeg -nostdin -loglevel error -y -f lavfi -i 'testsrc2=size=160x120:rate=25:duration=1' -f lavfi -i 'sine=frequency=440:sample_rate=48000:duration=1' -c:v mpeg2video -pix_fmt yuv420p -c:a pcm_dvd -f vob "$lpcm"
ffmpeg -nostdin -loglevel error -y -f lavfi -i 'testsrc2=size=160x120:rate=25:duration=1' -c:v mpeg2video -flags +ilme+ildct -pix_fmt yuv420p -an -f vob "$interlaced"
ffmpeg -nostdin -loglevel error -y -f lavfi -i 'testsrc2=size=1280x720:rate=10:duration=1' -c:v mpeg2video -pix_fmt yuv420p -an -f vob "$large"
# 1282 / 2 = 641: the half-size row must still end on a whole YUY2 pair.
ffmpeg -nostdin -loglevel error -y -f lavfi -i 'testsrc2=size=1282x720:rate=10:duration=1' -c:v mpeg2video -pix_fmt yuv420p -an -f vob "$large_odd_half"
dd if="$vp8" of="$truncated" bs=1 count=512 status=none
. tests/zz9000_sdk_build.sh
zz9000_sdk_compile "$out" tests/zz9000_sdk_media_test.cpp
"$out" "$video" "$audio_only"
"$out" --av "$vp8" 3 2 5 0 1 160 120
"$out" --av "$vp9" 4 2 4 0 1 160 120
"$out" --av "$mkv" 2 2 1 0 1 160 120
"$out" --av "$ps" 2 1 3 0 1 160 120
"$out" --av "$lpcm" 2 1 2 0 1 160 120
"$out" --av "$audio_ac3" 2 1 3 0 0 160 120
"$out" --av "$interlaced" 2 1 0 0 1 160 120
"$out" --av "$large" 2 1 0 0 1 1280 720
"$out" --av "$large_odd_half" 2 1 0 0 1 1282 720
"$out" --av "$truncated" 3 2 5 1 1 160 120
