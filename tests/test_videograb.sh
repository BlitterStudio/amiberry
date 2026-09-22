#!/usr/bin/env bash
set -euo pipefail

if command -v python3 >/dev/null 2>&1; then
	PYTHON=python3
elif command -v python >/dev/null 2>&1; then
	PYTHON=python
elif command -v py >/dev/null 2>&1; then
	PYTHON="py -3"
else
	echo "python3, python, or py is required" >&2
	exit 1
fi

$PYTHON - <<'PY'
from pathlib import Path
import sys

videograb = Path("src/osdep/videograb.cpp").read_text()
arcadia = Path("src/arcadia.cpp").read_text()
drawing = Path("src/drawing.cpp").read_text()
specialmonitors = Path("src/specialmonitors.cpp").read_text()


def fail(message: str) -> None:
	print(message, file=sys.stderr)
	sys.exit(1)


def region_between(text: str, start_marker: str, end_marker: str) -> str:
	try:
		start = text.index(start_marker)
		end = text.index(end_marker, start)
	except ValueError as exc:
		fail(f"Could not find source marker: {exc}")
	return text[start:end]


read_frame = region_between(
	videograb,
	"static bool read_ffmpeg_frame(",
	"static void uninit_ffmpeg_videograb(",
)
eof = region_between(read_frame, "if (err == AVERROR_EOF)", "if (err < 0)")
video_drain = eof.find("ffmpeg_decode_video_packet(nullptr, target_frame)")
audio_drain = eof.find("ffmpeg_decode_audio_packet(nullptr)")
loop_seek = eof.find("ffmpeg_seek_frame(0)")
if not (0 <= audio_drain < loop_seek and 0 <= video_drain < loop_seek):
	fail("FFmpeg decoders must drain delayed frames before the EOF loop seek")
if "if (!ffmpeg_discard_audio_packet(ffmpeg_packet))" not in read_frame:
	fail("FFmpeg seeks must discard audio packets before the requested frame")

audio_discard = region_between(
	videograb,
	"static bool ffmpeg_discard_audio_packet(",
	"static uae_s64 ffmpeg_current_frame(",
)
if "av_compare_ts(timestamp - start_time" not in audio_discard:
	fail("FFmpeg audio seek filtering must compare stream-relative timestamps")

seek_frame = region_between(
	videograb,
	"static bool ffmpeg_seek_frame(",
	"static bool read_ffmpeg_frame(",
)
if "ffmpeg_audio_discard_until_frame" not in seek_frame:
	fail("FFmpeg seeks must record the requested audio start frame")

frame_from_pts = region_between(
	videograb,
	"static uae_s64 ffmpeg_frame_from_pts(",
	"static uae_s64 ffmpeg_timestamp_from_frame(",
)
timestamp_from_frame = region_between(
	videograb,
	"static uae_s64 ffmpeg_timestamp_from_frame(",
	"static uae_s64 ffmpeg_current_frame(",
)
if "pts - start_time" not in frame_from_pts:
	fail("FFmpeg frame numbers must be relative to the stream start time")
if "start_time + av_rescale_q" not in timestamp_from_frame:
	fail("FFmpeg seeks must restore the stream start-time offset")

init_video = region_between(
	videograb,
	"bool initvideograb(",
	"bool getvideograb(",
)
if "audio_volume = 100 - currprefs.sound_volume_genlock;" not in init_video:
	fail("FFmpeg playback must start at the configured genlock volume")
if "audio_chflags = 3;" not in init_video:
	fail("Ordinary FFmpeg video playback must enable both audio channels")

mute_on = region_between(arcadia, "case 0x24: // Audio mute", "case 0x25: // Audio mute off")
mute_off = region_between(arcadia, "case 0x25: // Audio mute off", "case 0x26: // Video off")
for command in (mute_on, mute_off):
	if "setchflagsvideograb(ld_audio, ld_audio_mute);" not in command:
		fail("Laserdisc mute commands must update the video audio backend")
if "setchflagsvideograb(ld_audio, false);" in arcadia:
	fail("Laserdisc channel and restore updates must preserve mute state")

status = videograb[videograb.index("void isvideograb_status("):]
if "setchflagsvideograb(audio_chflags, audio_muted);" not in status:
	fail("Genlock volume refreshes must preserve backend mute state")

genlock_update = region_between(
	specialmonitors,
	"void specialmonitor_update_genlock(",
	"static uae_u32 quickrand(",
)
if "close_genlock_video();" not in genlock_update:
	fail("Inactive genlock must close its video or camera backend")
if "currprefs.genlock || currprefs.genlock_effects" not in genlock_update:
	fail("Genlock cleanup must follow both connector and effects state")

specialmonitor_reset = region_between(
	specialmonitors,
	"void specialmonitor_reset(",
	"bool specialmonitor_need_genlock(",
)
close_on_reset = specialmonitor_reset.find("close_genlock_video();")
early_return = specialmonitor_reset.find("if (!currprefs.monitoremu)")
if not (0 <= close_on_reset < early_return):
	fail("Reset must close genlock capture even without monitor emulation")

genlock_render = region_between(drawing, "// genlock", "#ifdef CD32")
if "specialmonitor_update_genlock();" not in genlock_render:
	fail("Rendering must update capture lifecycle even when genlock is inactive")
PY
