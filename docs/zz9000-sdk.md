# ZZ9000 SDK mailbox, images, audio, and video

The emulated ZZ9000 exposes the SDK v2 mailbox discovery registers and a
polling completion ring at board offset `0xD000`. The protocol follows the
ZZ9000 firmware's SDK (`zz9000-firmware` master, which now advertises WebP,
FLAC, Vorbis, WebM and MPEG-2). Amiberry advertises a capability bit or
service flag only when the host code behind it is compiled in, so the guest
clients fall back exactly as they would on older firmware.

## Build options

| Feature | Needs |
|---|---|
| MP3 audio streams (MHI, MPEGA, ZZPlay MP3) | mpg123 and AHI support |
| FLAC and Ogg Vorbis audio streams | FFmpeg (`USE_FFMPEG`, default on) |
| MPEG-2, WebM and Matroska media sessions | FFmpeg, Zorro III |
| WebP stills and animations | libwebp with libwebpdemux (`USE_WEBP`, default on) |
| JPEG/PNG, MPEG-1 media, surfaces, console encoder | always built |

FFmpeg 5.1 or newer is required. It is no longer tied to `USE_VIDEOGRAB`;
genlock video input still uses it when both are enabled. Android, iOS and
the libretro core build without FFmpeg and libwebp. Windows ARM64 builds
include libwebp but not FFmpeg, because vcpkg's FFmpeg port does not build
for `arm64-mingw-dynamic`.

## Discovery and memory

Discovery, service queries, and bounded shared-buffer allocation work on both
board models. Zorro II additionally answers the aperture-info handshake
(`0x111C`/`0x111E`) and the aperture layout query that current
`zz9k.library` builds use. The codec, crypto and diagnostics services are not
advertised; current clients treat them as optional.

The Zorro III model reserves a shared-buffer heap near the top of its 128 MB
board memory and also accepts card-only media allocations of up to 64 MiB
(`ZZ9K_ALLOC_MEDIA`), which ZZPlay's `--preload` option copies the file into
with `MEM_COPY` before playback. The Zorro II model provides a 64 KiB
host-visible staging window plus card-only buffers for the compressed and PCM
rings. Both models reset the mailbox and release every session, worker thread
and buffer on board reset or removal.

## Audio streams

The audio-stream service implements begin, codec-aware begin (`BeginEx`),
feed, read, play, stop, close and per-stream gain. MP3 sessions use mpg123;
the legacy begin opcode and `BeginEx` with the MP3 codec produce identical
PCM. FLAC (8 to 192 kHz, up to 24 bits, mono or stereo) and Ogg Vorbis (8 to
96 kHz, mono or stereo) are decoded incrementally with FFmpeg as the guest
feeds them, so large files are never held in host memory. As on the card,
their input waits in the session's input ring until PCM space lets the
decoder take it: a feed is refused only when it does not fit that ring, and
`bytes_consumed` counts decoded input. FLAC sessions can return S16LE,
S16BE or S32BE; Vorbis sessions return S16LE or S16BE.

MHI's S16LE stream plays through the selected Amiberry playback device on a
host output stream separate from the board's AHI path; the per-stream gain
applies to that output. The SDK audio fabric is not emulated, so the
installed guest drivers do not run AHI and MHI at the same time.
`mpega.library` keeps the MP3's sample rate and channel count for its S16BE
reads.

## Images

The image service decodes JPEG, PNG and WebP streams into RGB or 32-bit tiles
for the installed picture DataTypes, or into an ARM-local surface for SDK
viewers. Surface mapping, fill, and clipped bilinear scaling target the live
32-bit or 16-bit RTG framebuffer. WebP inputs follow the firmware limits:
8 MiB of input, 8192 pixels per side, 6 megapixels per canvas, and for
animations 256 frames. Animated WebP sessions step frames with next, present,
retire and restart, report frame durations and loop counts, and present
YUV422CGX frames to the Picasso96 video window. CPU-visible SDK surface
allocations still return `UNSUPPORTED`.

## Media sessions

On Zorro III the video service plays:

- MPEG-1 Program Stream with MP2 audio, decoded by pl_mpeg (always built);
- MPEG-2 Program Stream and VOB files with AC-3, DVD LPCM or MP2 audio,
  including audio-only and video-only sessions;
- WebM with VP8 or VP9 video and Opus or Vorbis audio;
- Matroska with MPEG-1, MPEG-2, VP8 or VP9 video and the same audio codecs.

Everything except MPEG-1 is decoded by FFmpeg on one worker thread per
session. ZZPlay streams the file through a bounded input queue; the media
write reply reports the cumulative number of accepted bytes, and decoding
begins as soon as the stream headers arrive rather than at end of file.
Like the card's video FIFO, the worker demuxes up to 1 MiB of video ahead
of the decoded-frame queue, so audio that a Program Stream carries behind
its pictures keeps decoding while the guest holds a frame. Once all input
and EOF have arrived, DECODE answers BUSY rather than NEED_INPUT until the
worker has caught up.
Decoded frames are packed to YUY2 for the Picasso96 video window. Interlaced
frames are blended, and frames too large for the window are packed at half
size. Audio goes to the session's S16BE PCM ring. Card-local AX media binding
is not advertised, so ZZPlay plays the sound through guest AHI. Zorro II video
sessions remain unadvertised because its video window aperture is not mapped
for this path.

The media status op (`0x0b0a`) answers all six card pages: timing (with the
timeline origin), audio, counters (decoded, presented and discarded frames
counted separately), audio output (zero underruns — there is no card-side
DMA pump here to starve), the overlay presentation path, and per-stage
profiling. The presentation page reports the PIP's flags and source,
destination and screen geometry packed into 16-bit pairs; because the
emulated PIP *is* the FPGA overlay plane, an active overlay always reports
the native path, and ZZPlay names it "native 1:1" or "native scaled" rather
than "card-local compositor". Profiling packs each stage as
`(microseconds << 32) | calls` measured around the real FFmpeg/pl_mpeg
decode, YUY2 pack, present copy and audio decode work; the NEON flag never
appears because the host packs in scalar C++. Pages above five and nonzero
flags are rejected exactly as on the card.

## Console encoder

The console service (`0x8200`) encodes the live RTG framebuffer into RFB tile
updates for remote-console clients. It converts BGRA8888 or RGB565
framebuffers to big-endian RGB565 tiles, refuses RGB555, and compresses tiles
with zlib when that makes them smaller.

## Not emulated

The SDK's archive, crypto, module, and physical-card operations remain
unadvertised and return `UNSUPPORTED`.

## Tests

`tests/test_zz9000_sdk.sh`, `tests/test_zz9000_sdk_audio.sh`,
`tests/test_zz9000_sdk_image.sh`, `tests/test_zz9000_sdk_media.sh` and
`tests/test_zz9000_sdk_console.sh` build the SDK against whichever optional
libraries `pkg-config` finds and generate their fixtures with the `ffmpeg`
command-line tool. They cover mailbox completion, allocation, reset and
preload buffers; MP3, FLAC and Vorbis PCM against FFmpeg-decoded references
and chunked feeding; JPEG, PNG and WebP pixels, limits and animation
stepping; streaming playback of every supported container and codec pair,
truncated input, and close/reopen; and console tile output.

## Guest qualification

On the user's A4000 system drive, ZZPlay and AmigaAMP MHI playback, MPEGA
decoding, the JPEG and PNG picture DataTypes, the SDK viewer, and ZZPlay
MPEG-1/MP2 playback (pause, resume, resize, loop, longer clips) work with the
installed libraries. ZZPlay also played MPEG-2 Program Stream and VOB files
with MP2, AC-3 and LPCM audio (up to 1280x720), WebM VP8/Vorbis and
VP9/Opus, Matroska with MPEG-1, MPEG-2 and VP8 video, a 1920x1080 VP9 clip,
FLAC and Ogg Vorbis through AHI and through on-card output, animated WebP,
and preloaded playback; `zz9k-view` showed lossy, lossless and animated WebP.
The WebP picture DataType and the sound DataType, the console encoder,
Zorro II and warm reset still need guest checks.
