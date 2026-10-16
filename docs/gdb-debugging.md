# GDB remote debugging

Amiberry can expose the emulated 68k CPU to a GDB client through the GDB
remote serial protocol. The server is part of the built-in debugger, is off by
default, and is controlled only by configuration options. It comes from
WinUAE.

## Enabling the server

Add these lines to a `.uae` configuration:

```ini
debugging_features=gdbserver
gdb_port=2345
```

or pass them on the command line:

```sh
amiberry -s debugging_features=gdbserver -s gdb_port=2345
```

`gdb_port` defaults to 2345. Amiberry logs
`WinUAE GDB: listening on 127.0.0.1:<port>` when the server starts.

The standalone application builds the server when both `DEBUGGER` and
`GDBSERVER` are defined. The libretro core does not expose the host event loop
needed to poll the server, so it intentionally compiles the server out.

The server is not available with the threaded CPU option
(`cpu_threaded=true`). In that mode Amiberry logs
`WinUAE GDB: not available with the threaded CPU (cpu_threaded=true)` and does
not listen.


`gdb_server_poll()` runs from Amiberry's `handle_events()` loop. It never
accesses emulated state from a socket thread.
The server listens on `127.0.0.1` only and accepts one client at a time. It is
unauthenticated: any local process that connects gets full control of the
guest. When it is enabled but no client is attached, the only cost is a
non-blocking `accept()` once per frame; nothing traces the CPU.

## Connecting

Use any GDB that supports m68k, for example `m68k-elf-gdb`, `m68k-amigaos-gdb`
or `gdb-multiarch`:

```text
(gdb) set endian big
(gdb) set architecture m68k
(gdb) target remote 127.0.0.1:2345
```

`gdb-multiarch` needs `set endian big` when no m68k executable is loaded;
otherwise it shows every register byte-swapped.

The CPU stops at the next debugger boundary when a client attaches. If the
emulator is paused in Amiberry, unpause it first, because the CPU has to run to
reach that boundary. While the CPU is stopped, the Amiberry window does not
update.

## What is supported

- The 18 integer registers (D0-D7, A0-A7, SR, PC), read and write. GDB also
  lists the FPU registers for m68k; they read as `<unavailable>`.
- Reads of RAM and ROM, and writes to RAM. ROM writes fail.
- Custom chip registers at `$DFF000-$DFF1FF`: reads return Amiberry's saved
  register snapshot, and aligned word writes use the debugger's hardware write
  path. Other I/O areas are not accessible.
- Single step, PC breakpoints and CPU data watchpoints (`watch`, `rwatch`,
  `awatch`), using the built-in debugger's breakpoint and memwatch tables.
- Range stepping (`vCont;r`): execution continues until the PC leaves the
  range. At least one instruction runs; an empty range single-steps.
- The memory map (`info mem`), listing RAM and ROM regions.

Memory addresses are physical; guest MMU translation is not applied.
Watchpoints must lie below `$7FFF0000`, can be at most 64 KiB long, and are
unavailable while the debugger's MMU mode is enabled.

Because the target provides a memory map, GDB refuses to access addresses
outside the listed RAM and ROM regions. To read custom chip registers, allow
access to unlisted regions first:

```text
(gdb) set mem inaccessible-by-default off
(gdb) x/2xh 0xdff004
```

## Monitor commands

Amiberry-specific operations use GDB `monitor` (`qRcmd`) commands. Replies are
hex-encoded text, usually JSON. Numeric arguments are hexadecimal unless a
command says otherwise. Commands require a stopped CPU except `capabilities`
and `execution-status`.

- `monitor capabilities` lists commands available in the current build and
  configuration. `monitor execution-status` reports CPU, host-pause, frame,
  beam and cycle state without stopping the guest.
- `monitor disasm ADDRESS DECIMAL-COUNT` disassembles 1–100 instructions.
  `monitor screenshot NATIVE-HOST-PATH` writes a PNG without changing
  screenshot preferences or showing a status-line message.
- `monitor memory-map` returns the debugger memory map as JSON. `monitor
  memory-check ADDRESS LENGTH` checks that a physical RAM range is writable
  and reports whether it is chip or fast RAM.
- `monitor process-break name NAME` or `process-break address TASK` arms a
  one-shot stop at an AmigaDOS program entry. `process-break status` and
  `process-break clear` inspect or cancel it. `monitor segments [PROCESS]`
  reports loaded AmigaDOS segments.
- `monitor tasks` lists current, ready and waiting Exec tasks. `monitor
  history on|off` and `history read COUNT` use the existing debugger history.
  `monitor condition add`, `condition list`, `condition remove`, and
  `step-over` use the existing breakpoint and trace controls.
- `monitor exception-mask MASK` chooses CPU exception vectors; `monitor
  exception` reports the selected exception with its pre-frame register
  snapshot.
- `monitor dma-watch add ADDRESS LENGTH MODE MASK`, `dma-watch list`, and
  `dma-watch remove ID` manage DMA watches. `monitor watch add` and `watch
  range` create CPU/DMA watch predicates; `watch list`, `watch remove`,
  `watch events`, `watch last`, and `watch clear-events` inspect them.
- `monitor guest-output on|off|read|clear` captures uaelib function 86 debug
  messages. It retains bounded payload and record counts.
- `monitor checkpoint save NATIVE-HOST-PATH` saves at the next frame start,
  then stops. `monitor checkpoint restore NATIVE-HOST-PATH` follows the
  normal reset/restore lifecycle and keeps the client connection. Restore
  clears remote breakpoints, watches, exception selection, entry/range state,
  and captured output.
- `monitor input event KIND PORT CODE VALUE` injects keyboard, relative mouse,
  or joystick input through the shared input handlers. `monitor input
  sequence` accepts frame-delayed events; `input status` and `input release`
  inspect or cancel the sequence.

`monitor capabilities` is the authoritative command list. It omits commands
that are unavailable in the active build or configuration.

## Detaching

`detach`, or closing the client, resumes the emulation and removes the
breakpoints and watchpoints that the client set. Breakpoints and watchpoints
set from Amiberry's own debugger stay in place. A hard reset of the emulated
machine closes the connection.
