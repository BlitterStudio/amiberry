# ZZ9000Net.device Ethernet emulation

Amiberry's ZZ9000 board exposes the Ethernet register and frame windows used
by the installed SANA-II `ZZ9000Net.device`. Select a ZZ9000 board in the
Expansions panel, then choose its Ethernet backend. The default is SLIRP user
mode NAT. The backend is separate from `bsdsocket.library` emulation and from
other emulated network cards. The settings are saved in `.uae` files:

```ini
zz9000_net=slirp
zz9000_int2=false
```

For a test of the installed SANA-II stack, set `bsdsocket_emu=false` in the
Amiberry configuration. Otherwise a guest application can use Amiberry's
host `bsdsocket.library` emulation without sending frames through the ZZ9000.

`zz9000_net=none` disconnects the card. TAP and pcap adapters appear in the
selector when those backends are built and available. An unavailable backend
leaves the card visible to the guest but causes transmit to return an error;
Amiberry logs the failed host connection. SLIRP does not provide a bridge to
other devices on the host's physical LAN.

SLIRP supports one emulated network card at a time. If another card already
uses it, the ZZ9000 host backend stays disconnected. Choose TAP or pcap for
one card, or disable the other card, to use ZZ9000Net through SLIRP.

The interface uses board offsets `0x2000` for the current RX frame and
`0x8000` for TX staging. Writing a frame length to register `0x80` transmits
the staged frame and reading it returns the send status. The RX window starts
with a big-endian size and serial header. Writing that serial to `0x82`
accepts the frame; the previous driver's constant `1` acknowledgement also
works. An empty window reads as zero. Frames are bounded to 1518 bytes and
the receive backlog holds up to 128 frames. The MAC address uses registers
`0x84` through `0x88`; status and drop counters use `0x8c` and `0x8e`.

The Ethernet interrupt is bit 0 of register `0x04`. It shares Paula's EXTER
line (INT6) with ZZ9000AX by default. The guest's `ENV:ZZ9K_INT2` or
`ZZ9000.CFG` INT2 setting requires `zz9000_int2=true` in Amiberry, selecting
the PORTS line (INT2) for both functions. Change the setting before booting.

The A4000 test boot used writable copies of both real-machine hardfiles,
`zz9000_net=slirp`, and `bsdsocket_emu=false`. The installed guest network
stack carried traffic while ZZ9000AX audio played. The originals were not
mounted by that test. The focused frame protocol test and Windows debug build
passed. Longer bidirectional transfers, reset during traffic, TAP/pcap, Zorro
II, and INT2 remain to be qualified.
