# ZZ9000 SDK mailbox and MHI audio

The emulated ZZ9000 exposes the SDK v2 mailbox discovery registers and a
polling completion ring at board offset `0xD000`. It implements core
discovery, bounded shared-buffer allocation, and the audio-stream operations
used by the current `zz9k.library` and installed `mhizz9000.library`:
begin, feed, read, play, stop, and close. The older direct-register MP3 decoder
is no longer used by that MHI driver. The separate SDK-backed `mpega.library`
client uses the same stream service for decoded PCM reads. Its S16BE sessions
retain the MP3's sample rate and channel count; playback remains available
only for S16LE sessions, matching the card's stream contract.

Audio-stream capability bits are present only in builds with mpg123, AHI, and
SDL audio support. MHI's S16LE stream decodes to 48 kHz stereo 16-bit PCM and
plays through the selected Amiberry playback device. MHI has a separate host
output stream from the board's AHI path. The SDK audio fabric is not emulated or
advertised; the installed guest drivers therefore exclude simultaneous active
AHI and MHI.
The SDK's other media, image, archive, crypto,
module, and physical-card operations remain unadvertised and return
`UNSUPPORTED`.

The Zorro III model reserves a bounded shared-buffer heap near the top of its
128 MB board memory. The Zorro II model provides a 64 KiB host-visible staging
window plus card-only buffers for the compressed and PCM rings. Both models
reset the mailbox and release stream resources on board reset or removal.

The focused tests are `tests/test_zz9000_sdk.sh` and
`tests/test_zz9000_sdk_audio.sh`. They exercise mailbox completion, allocation,
reset, MHI playback, and MPEGA-style big-endian PCM reads with short input,
native-rate mono output at 44.1 and 22.05 kHz, EOF, and compact Zorro II
buffers. ZZPlay and AmigaAMP MHI playback have been confirmed with the
installed libraries on an A4000 system drive; ZZ9000AX AHI still plays
afterward. AmigaAMP pause/resume, seek, track completion, and starting another
track also work. On that same drive, `mpega.library` 2.125 passed the SDK smoke
tool's null API check, decoded a full 246.8-second 44.1 kHz stereo MP3 through
EOF, and decoded 100 frames after seeking to 30 seconds. MHI drain and
backpressure edge cases, warm reset, and Zorro II behavior still need guest
checks before claiming full compatibility with a particular system drive.
