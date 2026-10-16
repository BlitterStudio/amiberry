#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/zz9000_sdk_audio_test.$$"
mp3="${TMPDIR:-/tmp}/zz9000_sdk_audio_test.$$.mp3"
mono_mp3="${TMPDIR:-/tmp}/zz9000_sdk_audio_mono_test.$$.mp3"
low_mp3="${TMPDIR:-/tmp}/zz9000_sdk_audio_low_test.$$.mp3"
flac16="${TMPDIR:-/tmp}/zz9000_sdk_audio_16_test.$$.flac"
flac24="${TMPDIR:-/tmp}/zz9000_sdk_audio_24_test.$$.flac"
vorbis="${TMPDIR:-/tmp}/zz9000_sdk_audio_vorbis_test.$$.ogg"
flac16_ref="${TMPDIR:-/tmp}/zz9000_sdk_audio_16_test.$$.s16be"
flac24_ref="${TMPDIR:-/tmp}/zz9000_sdk_audio_24_test.$$.s32be"
vorbis_ref="${TMPDIR:-/tmp}/zz9000_sdk_audio_vorbis_test.$$.s16le"
picture_png="${TMPDIR:-/tmp}/zz9000_sdk_audio_picture_test.$$.png"
flac_picture="${TMPDIR:-/tmp}/zz9000_sdk_audio_picture_test.$$.flac"
vorbis_long="${TMPDIR:-/tmp}/zz9000_sdk_audio_vorbis_long.$$.ogg"
vorbis_long_ref="${TMPDIR:-/tmp}/zz9000_sdk_audio_vorbis_long.$$.s16le"
flac_long="${TMPDIR:-/tmp}/zz9000_sdk_audio_flac_long.$$.flac"
flac_long_ref="${TMPDIR:-/tmp}/zz9000_sdk_audio_flac_long.$$.s16le"
trap 'rm -f "$out" "$mp3" "$mono_mp3" "$low_mp3" "$flac16" "$flac24" "$vorbis" "$flac16_ref" "$flac24_ref" "$vorbis_ref" "$picture_png" "$flac_picture" "$vorbis_long" "$vorbis_long_ref" "$flac_long" "$flac_long_ref"' EXIT
ffmpeg -nostdin -loglevel error -f lavfi -i 'sine=frequency=440:duration=0.35' \
  -ar 48000 -ac 2 -b:a 128k "$mp3"
ffmpeg -nostdin -loglevel error -f lavfi -i 'sine=frequency=440:duration=0.2' \
  -ar 44100 -ac 1 -b:a 64k "$mono_mp3"
ffmpeg -nostdin -loglevel error -f lavfi -i 'sine=frequency=440:duration=0.2' \
  -ar 22050 -ac 1 -b:a 48k "$low_mp3"
ffmpeg -nostdin -loglevel error -f lavfi -i 'sine=frequency=330:duration=0.15' \
  -ar 44100 -ac 1 -c:a flac -sample_fmt s16 "$flac16"
ffmpeg -nostdin -loglevel error -f lavfi -i 'sine=frequency=550:duration=0.15' \
  -ar 48000 -ac 2 -c:a flac -sample_fmt s32 -bits_per_raw_sample 24 "$flac24"
ffmpeg -nostdin -loglevel error -f lavfi -i 'sine=frequency=660:duration=0.15' \
  -ar 32000 -ac 2 -c:a libvorbis "$vorbis"
ffmpeg -nostdin -loglevel error -i "$flac16" -f s16be -acodec pcm_s16be "$flac16_ref"
ffmpeg -nostdin -loglevel error -i "$flac24" -f s32be -acodec pcm_s32be "$flac24_ref"
ffmpeg -nostdin -loglevel error -i "$vorbis" -f s16le -acodec pcm_s16le "$vorbis_ref"
# Noise keeps the Vorbis pages large, so PCM backpressure stops parsing
# before the EOS page of a single EOF-marked feed.
ffmpeg -nostdin -loglevel error -f lavfi -i 'anoisesrc=duration=2:color=pink:sample_rate=44100:amplitude=0.5' \
  -ac 2 -c:a libvorbis "$vorbis_long"
ffmpeg -nostdin -loglevel error -i "$vorbis_long" -f s16le -acodec pcm_s16le "$vorbis_long_ref"
# White noise barely compresses, so one feed carries well over 128 KiB.
ffmpeg -nostdin -loglevel error -f lavfi -i 'anoisesrc=duration=3:color=white:sample_rate=44100:amplitude=0.5' \
  -ac 1 -c:a flac -sample_fmt s16 "$flac_long"
ffmpeg -nostdin -loglevel error -i "$flac_long" -f s16le -acodec pcm_s16le "$flac_long_ref"
ffmpeg -nostdin -loglevel error -f lavfi -i 'testsrc2=size=1024x1024:rate=1' -frames:v 1 "$picture_png"
ffmpeg -nostdin -loglevel error -i "$flac16" -i "$picture_png" -map 0:a -map 1:v \
  -c:a copy -c:v png -disposition:v attached_pic "$flac_picture"
. tests/lib/zz9000_sdk_build.sh
zz9000_sdk_compile "$out" tests/zz9000_sdk_test.cpp libmpg123 -DHAVE_MPG123
SDL_AUDIODRIVER=dummy "$out" "$mp3" "$mono_mp3" "$low_mp3" "$flac16" "$flac24" "$vorbis" "$flac16_ref" "$flac24_ref" "$vorbis_ref" "$flac_picture" "$vorbis_long" "$vorbis_long_ref" "$flac_long" "$flac_long_ref"
