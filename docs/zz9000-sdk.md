# ZZ9000 SDK mailbox and MHI audio

The emulated ZZ9000 exposes the SDK v2 mailbox discovery registers and a
polling completion ring at board offset `0xD000`. It implements core
discovery, bounded shared-buffer allocation, and the audio-stream operations
used by the current `zz9k.library` and installed `mhizz9000.library`:
begin, feed, read, play, stop, and close. The older direct-register MP3 decoder
is no longer used by that MHI driver.

Audio-stream capability bits are present only in builds with mpg123, AHI, and
SDL audio support. MP3 is decoded to 48 kHz stereo 16-bit PCM and plays through
the selected Amiberry playback device. MHI has a separate host output stream
from the board's AHI path. The SDK audio fabric is not emulated or advertised;
the installed guest drivers therefore exclude simultaneous active AHI and MHI.
The SDK's other media, image, archive, crypto,
module, and physical-card operations remain unadvertised and return
`UNSUPPORTED`.

The Zorro III model reserves a bounded shared-buffer heap near the top of its
128 MB board memory. The Zorro II model provides a 64 KiB host-visible staging
window plus card-only buffers for the compressed and PCM rings. Both models
reset the mailbox and release stream resources on board reset or removal.

The focused tests are `tests/test_zz9000_sdk.sh` and
`tests/test_zz9000_sdk_audio.sh`. They exercise mailbox completion, allocation,
reset, and a generated MP3 stream. ZZPlay and AmigaAMP MHI playback have been
confirmed with the installed libraries on an A4000 system drive; ZZ9000AX AHI
still plays afterward. Pause/resume, seek, drain, warm reset, and Zorro II
behavior still need guest checks before claiming full compatibility with a
particular system drive.
