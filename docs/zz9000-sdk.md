# ZZ9000 SDK mailbox, images, audio, and video

The emulated ZZ9000 exposes the SDK v2 mailbox discovery registers and a
polling completion ring. The protocol follows the ZZ9000 firmware's SDK
(`zz9000-firmware` master, which now advertises WebP, FLAC, Vorbis, WebM and
MPEG-2). Amiberry advertises a capability bit or service flag only when the
host code behind it is compiled in, so the guest clients fall back exactly as
they would on older firmware.

## Build options

| Feature | Needs |
|---|---|
| MP3 streams and one-shot `DECODE_MP3` | mpg123 and AHI support |
| FLAC and Ogg Vorbis audio streams | FFmpeg (`USE_FFMPEG`, default on) |
| MPEG-2, WebM and Matroska media sessions | FFmpeg, Zorro III |
| WebP stills and animations | libwebp with libwebpdemux (`USE_WEBP`, default on) |
| JPEG/PNG sessions, MPEG-1 media, surfaces, console encoder | always built |

FFmpeg 5.1 or newer is required. It is no longer tied to `USE_VIDEOGRAB`;
genlock video input still uses it when both are enabled. Android, iOS and
the libretro core build without FFmpeg and libwebp. Windows ARM64 builds
include libwebp but not FFmpeg, because vcpkg's FFmpeg port does not build
for `arm64-mingw-dynamic`.

One-shot MP3 decode (`0x0500`) uses the same mpg123 gate as MP3 streams.
Android forces `USE_MPG123` off. iOS does not build mpg123
(`cmake/Dependencies.cmake`). Windows ARM64 forces it off because the
vcpkg port does not build for `arm64-mingw-dynamic`. The libretro makefile
does not link mpg123. Those builds do not advertise
`ZZ9K_CAP_AUDIO_DECODE` (bit 7) on any path, and `0x0500` answers
`UNSUPPORTED`. The SDK audio path is compiled only when `HAVE_MPG123`,
`AHI` and not `LIBRETRO` are all set, so linking mpg123 into the libretro
core would still not serve `0x0500`.

## Discovery and memory

Discovery, service queries, and bounded shared-buffer allocation work on both
board models. Zorro II additionally answers the aperture-info handshake
(`0x111C`/`0x111E`) and the aperture layout query that current
`zz9k.library` builds use. The codec, crypto, diagnostics, firmware-module
and ARM app (ZZRun) services are not advertised, and the audio output clock
op (`0x0517`) answers `UNSUPPORTED`; current clients treat all of them as
optional.

The mailbox sits where the firmware places it, in the main board aperture:
ARM `0x08100000` with 64-entry rings on Zorro III, and ARM `0x005DF480` with
22-entry rings (ending at board offset `0x3F0000`) on Zorro II. Every SDK
client maps that range the same way. The older board-`0xD000` placement went
through the mapped-I/O window, whose ARM address the SDK moved from
`0x3FE40000` to `0x3FF70000` in October 2026, so it only worked with one
generation of clients.

The Zorro III model reserves a shared-buffer heap near the top of its 128 MB
board memory and also accepts card-only media allocations of up to 64 MiB
(`ZZ9K_ALLOC_MEDIA`), which ZZPlay's `--preload` option copies the file into
with `MEM_COPY` before playback. The Zorro II model provides a 61 KiB
(`0xF480`) host-visible staging window below the mailbox, or 16 KiB once the
generation-2 aperture layout is acknowledged, plus card-only buffers for the
compressed and PCM rings. Both models reset the mailbox and release every
session, worker thread and buffer on board reset or removal.

`zz9k.library` revision 38 and later tags every mailbox entry with its
opener's client ID (flags bits 5-15) and sends `CLIENT_RELEASE` (`0x0006`)
when that opener's last close comes. Amiberry then closes the client's
image, media and audio sessions and frees the buffers and surfaces it
allocated, so a program that exits without closing its session no longer
keeps the card busy until a reset. An untagged entry (client ID 0) or one
with a payload is `BAD_REQUEST`, as on the card; older libraries send only
untagged entries and never use the op. Unlike the card, which frees
everything the client allocated, Amiberry keeps a buffer or surface that
another client's live session still uses: an image tile buffer or
destination surface, or a media PCM ring, the cases where `FREE_SHARED` and
`FREE_SURFACE` answer `BUSY`. It passes to the client that owns that
session and is freed when that client is released. Audio stream rings are
freed with their owner, since a stream keeps the ring it resolved at begin.
The op never answers `BUSY`, because `zz9k.library` sends it again on
`BUSY`.

## Audio streams

The audio-stream service implements begin, codec-aware begin (`BeginEx`),
feed, read, play, stop, close and per-stream gain, and keeps up to four
concurrent sessions like the card's four stream slots; a begin against a
full table answers `NO_MEMORY`. MP3 sessions use mpg123; the legacy begin
opcode and `BeginEx` with the MP3 codec produce identical PCM. FLAC (8 to
192 kHz, up to 24 bits, mono or stereo) and Ogg Vorbis (8 to 96 kHz, mono
or stereo) are decoded incrementally with FFmpeg as the guest feeds them,
so large files are never held in host memory. As on the card, input waits
in the session's input ring until PCM space lets the decoder take it: a
feed is refused only when it does not fit that ring (still `OK`, with the
BACKPRESSURE result flag), and `bytes_consumed` counts decoded input.
Undecodable MP3 data never faults a session — the card's minimp3 consumes
nothing and waits — and the input ring of any open session can be freed,
because the session keeps the ring it resolved at begin. FLAC sessions can
return S16LE, S16BE or S32BE; Vorbis sessions return S16LE or S16BE.
Playback (`Play`) binds one session at a time; a `Play` for a different
session while one is bound answers `BUSY`, exactly as the card's single
output pump does.

MHI's S16LE stream plays through the selected Amiberry playback device on a
host output stream separate from the board's AHI path; the per-stream gain
applies to that output. The SDK audio fabric is not emulated, so the
installed guest drivers do not run AHI and MHI at the same time.
`mpega.library` keeps the MP3's sample rate and channel count for its S16BE
reads.

`DECODE_MP3` (`0x0500`) is the card's one-shot path (`handle_decode_mp3`,
`mp3_decode_compute`), and only when mpg123 is compiled in. A payload
shorter than 48 bytes, a zero source, a source above 16 MiB, a destination
shorter than 2 bytes, or a flag other than `EXPECT_END` is `BAD_REQUEST`.
A missing buffer is `BAD_HANDLE`. An output format other than S16LE or
S16BE is `UNSUPPORTED`. A requested rate or channel count that is nonzero
and differs from the stream is `UNSUPPORTED`: there is no resampling.
`EXPECT_END` is accepted and does not change the decode. The destination
capacity is rounded down to an even length. The reply is the 48-byte
`SDKAudioDecodeResultPayload` the SDK decoder requires: bytes consumed,
bytes written, sample rate, channels, the requested sample format, frames
written, and the END flag when the input was consumed. mpg123 fills the
destination until it is full or the input ends. It has no minimp3 parse
cursor, so a truncated trailing frame can be counted differently, and an
ID3v1 tag the card leaves unconsumed may be skipped. Bit 7 is set in the
mailbox capability word, the audio service word, and the Zorro III video
service word only in a build that can serve this op.

## Surfaces

`ALLOC_SURFACE` (`0x0200`) follows `handle_alloc_surface`. A zero width or
height, an unknown format, `PLANAR`, a pitch shorter than one row, or a
surface larger than 32 MiB is `BAD_REQUEST`. A full table (four surfaces)
or no board space is `NO_MEMORY`. `ARM_LOCAL` (flag 16) keeps the pixels
in host memory and clears `CPU_VISIBLE`. Any other allocation is
CPU-visible: the pixels live in board memory, the guest reads and writes
them through the Zorro window, and the reply sets `CPU_VISIBLE` (flag 1).
`FRAMEBUFFER`, `DISPLAYED` and `SHARED_BUFFER` are stored as given. A
pitch of 0 means the minimum row size. Supported formats are RGB565,
ARGB8888, RGBA8888, INDEX8, RGB555, BGRA8888, RGB888 and YUV422CGX.

`FREE_SURFACE` refuses a missing handle (`BAD_HANDLE`) and a surface an
image session still uses (`BUSY`). `MAP_FRAMEBUFFER_SURFACE` returns
handle `0x80000000` for the live RTG framebuffer. `FILL_SURFACE` rejects
any flag (`UNSUPPORTED`) and a rectangle outside the surface
(`BAD_REQUEST`). The framebuffer handle is valid for fill, copy and scale.

`COPY_SURFACE` (`0x0204`) follows `handle_copy_surface` and
`sdk_surface_copy_rect`. The payload is 48 bytes. A missing source or
destination, including the framebuffer handle when no mode is set, is
`BAD_HANDLE`. Any flag, or a format mismatch, is `UNSUPPORTED`. A
rectangle outside either surface, a short pitch, or a format with no byte
size (YUV422CGX) is `BAD_REQUEST`. Each row is copied with `memmove`,
bottom-up when the destination is the same storage and starts on a lower
row, so an overlapping same-surface copy keeps the source bytes. A
successful copy into the framebuffer is a redraw from `poll`, as a fill
or a scale into the framebuffer is.

`QUERY_PALETTE` (`0x0205`) follows `handle_query_palette` and
`sdk_palette_pack_be`. The payload is at least 24 bytes. Flag bits other
than `SECONDARY` are `BAD_REQUEST`; `SECONDARY` is `UNSUPPORTED`, because
the secondary CLUT is not shadowed. `count` must be 1 to 256 and
`start + count` must not pass 256, or the answer is `BAD_REQUEST`. A
missing destination buffer, or a window that does not fit it, is
`BAD_HANDLE`. The `surface` field is accepted and ignored: the palette is
display-global. Entries are consecutive big-endian `0x00RRGGBB` words in
the caller's buffer. The shadow has 256 entries and is updated whenever
the RTG driver writes a primary CLUT entry (`set_palette_entry`). The
card's shadow is a static table that keeps its contents over an Amiga
reset, so after `reset()` clears it the board refills it from its CLUT.
The card fills its shadow from the RTG driver's DMA `SET_PALETTE`
operation (`dma_rtg.c`); Amiberry also fills it from the register-based
palette writes, which the card does not shadow.
The surface service advertises `ZZ9K_SERVICE_FLAG_SURFACE_PALETTE_QUERY`
and an opcode count of 6.

## Images

`SCALE_IMAGE` (`0x0400`) is the clipped scaler with the destination
rectangle as the clip, and it ignores its flags word. Same-format scaling
is nearest for every format that has a byte size, and bilinear for the
4-byte formats. Across formats, only BGRA8888 to RGB565 or RGB555 is
accepted. A same-surface scale, any other format pair, or a filter above
bilinear is `UNSUPPORTED`. `SCALE_IMAGE_CLIPPED` (`0x0407`) is the same
scaler with an explicit clip; a nonzero flags word is `UNSUPPORTED`. An
empty clip intersection succeeds and writes nothing. A scale that changes
the framebuffer is a redraw.

`DECODE_JPEG` (`0x0401`) follows `handle_decode_jpeg`: the picture at its
own size, never scaled, into the destination rectangle. A destination
width or height of 0 runs to the surface edge. The output format must be
the surface's, and must be ARGB8888, RGBA8888, BGRA8888 or RGB888. Any
flag is `UNSUPPORTED`. A picture that is not a JPEG, or that does not fit
the rectangle, is `IO_ERROR`. The reply is the 48-byte
`SDKImageDecodeResultPayload`. A decode into the framebuffer is a redraw.
`DECODE_PNG` (`0x0402`) and `DECODE_GIF` (`0x0403`) answer `UNSUPPORTED`,
as the card does.

Streaming JPEG, PNG and WebP sessions still feed tiles or an SDK surface
for the picture DataTypes and SDK viewers. WebP inputs follow the firmware
limits: 8 MiB of input, 8192 pixels per side, 6 megapixels per canvas, and
for animations 256 frames. Animated WebP sessions step frames with next,
present, retire and restart, report frame durations and loop counts, and
present YUV422CGX frames to the Picasso96 video window.

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
framebuffers to big-endian RGB565 tiles, encodes 8-bit screens (reported as
INDEX8 surfaces) as CLUT8 tiles, refuses RGB555, and compresses tiles with
zlib when that makes them smaller. Tiles are always raw; the card's optional
XOR and copy-rectangle layers are not produced.

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
and chunked feeding; one-shot MP3 decode (result fields, S16BE against
S16LE, rate and channel mismatch, a short destination); JPEG, PNG and WebP
pixels, limits and animation stepping; surface copy (including overlap,
bounds and a framebuffer redraw) and palette packing; streaming playback
of every supported container and codec pair, truncated input, and
close/reopen; and console tile output.

## Guest qualification

On the user's A4000 system drive, ZZPlay and AmigaAMP MHI playback, MPEGA
decoding, the JPEG and PNG picture DataTypes, the SDK viewer, and ZZPlay
MPEG-1/MP2 playback (pause, resume, resize, loop, longer clips) work with the
installed libraries. ZZPlay also played MPEG-2 Program Stream and VOB files
with MP2, AC-3 and LPCM audio (up to 1280x720), WebM VP8/Vorbis and
VP9/Opus, Matroska with MPEG-1, MPEG-2 and VP8 video, a 1920x1080 VP9 clip,
FLAC and Ogg Vorbis through AHI and through on-card output, animated WebP,
and preloaded playback; `zz9k-view` showed lossy, lossless and animated WebP.
A later run on a disposable copy of the same system, with drivers master
(`zz9k.library` 2.33, ZZPlay 0.6), passed the remaining checks: the WebP
picture DataType and the FLAC and Ogg Vorbis sound DataType decode correctly,
the console encoder's decoded tiles match the screen pixel for pixel, ZZPlay
reports the presentation path and card profile from status pages 4 and 5, and
a warm reset during media, audio and network sessions recovers all of them.
On a 4 MB Zorro II board with a 1024x768 16-bit Workbench, `zz9k-archive`,
`zz9k-view`, MHI, AHI and on-card audio work, and ZZPlay refuses video files
because Zorro II video sessions are not advertised.
