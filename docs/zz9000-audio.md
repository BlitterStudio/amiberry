# ZZ9000AX AHI audio emulation

Amiberry's ZZ9000 board model also exposes the AX audio registers and the
legacy TX and RX rings. A real-system `zz9000ax.audio` subdriver uses the same
ConfigDev and register protocol in Amiberry as it does with the card. The
driver does not need an emulator build or an installation change.

## Implemented interface

The card window begins at the Zorro board address. Register accesses occupy
the first 64 KiB. Card memory begins at board offset `0x10000`. The legacy
audio TX and RX rings are at board offsets `card_size - 0x10000` and
`card_size - 0x8000` respectively. Each holds eight 3840-byte periods.

| Offset | Meaning |
|---|---|
| `0x04` | Read pending interrupt flags (audio = bit 1); write bits 3 and 5 to acknowledge audio |
| `0x70` | Write a TX period offset divided by 256 to convert the guest's BE samples; read the last collision flag |
| `0x74` | Audio frames per 20 ms period; playback and capture run at `frames × 50` Hz |
| `0xF4` | Read AX presence and TX status capability (`0x0003`); write playback and capture interrupt enables |
| `0xF6` | Packed RX completed period and sequence |
| `0xF8` | Packed TX completed period and sequence |

The status words have bit 15 set, the completed period in bits 12–14, and a
12-bit sequence in bits 0–11. `0xE6` still reads zero, keeping the installed
driver on its legacy aperture and register path.

The emulator advances the formatter at a host-time 20 ms cadence during
emulation. TX slots contain guest big-endian stereo S16 samples; the `0x70`
kick swaps the submitted period and SDL plays it at the guest's selected rate.
Capture converts host little-endian stereo S16 into the guest's big-endian RX
ring. Missing host capture data becomes silence while period status continues
to advance. Selecting no sampler keeps the host microphone closed and supplies
silence to the RX ring. Playback follows the Master and AHI volume controls,
mute, and AHI channel-swap preference; capture follows the same channel-swap
preference. Libretro does not expose AX audio yet because its SDL audio stream
stub does not forward samples to the frontend.

All card memory, register, status, and interrupt updates run from the
emulation callback. A completed period asserts Paula's `INTB_EXTER` request
(bit 13, level 6). The stock driver's `AddIntServer(INTB_EXTER, ...)` handler
reads `0x04`, acknowledges it, and wakes its AHI worker. The card model clears
its pending flag on that acknowledgement, stop, and reset; Exec clears the
shared Paula request after running the interrupt-server chain. This supports
the driver's default EXTER wiring; a system configured with `ENV:ZZ9K_INT2`
uses the physical PORTS line and is not modeled yet.

## Validation

Build Amiberry, then boot a ZZ9000 system drive using a configuration with a
ZZ9000 board. Start an AHI application that selects ZZ9000AX. Useful
`Amiberry.log` milestones are:

1. `ZZ9000AX: probed` — the installed subdriver found the AX card.
2. `ZZ9000AX: intreq mask` — playback or capture started.
3. `ZZ9000AX: audio interrupt pending` followed by `interrupt acked` — the
   installed ISR serviced a completed period.
4. Repeated `period` lines with advancing TX sequence and nonzero `peak` while
   playing audible content — the worker is refilling the ring.

Playback was verified with a real A4000 system image and its installed AHI
subdriver: the guest booted, audio was heard, interrupt acknowledgements and
SWAB kicks continued, and the TX sequence advanced beyond 1,250 with nonzero
sample peaks. Capture has not been tested. The emulator currently models the
legacy AHI register path, without the SDK audio fabric or DSP parameter
operations.
