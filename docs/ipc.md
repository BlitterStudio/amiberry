# IPC controller interface

Amiberry exposes a line-oriented local control socket when it is built with
`USE_IPC_SOCKET=ON`. On Linux and macOS it is a Unix-domain socket; on Windows
it is a named pipe. It is used by controller software such as the
[Amiberry MCP server](https://github.com/BlitterStudio/amiberry-mcp-server),
which presents stable, action-level MCP tools and HTTP endpoints.

Application integrations should use those MCP or HTTP interfaces. The raw
commands described here are controller primitives: they expose window-logical
coordinates, complete button bitmasks, runtime identities, revisions, and
configuration compare-and-set operations. They are not intended as a direct
public automation API.

## Transport

On Linux and macOS the first instance listens on
`$XDG_RUNTIME_DIR/amiberry.sock`, or `/tmp/amiberry.sock` when
`XDG_RUNTIME_DIR` is not set. Further instances use `amiberry_1.sock` through
`amiberry_9.sock` in the same directory.

On Windows the first instance listens on the named pipe `\\.\pipe\amiberry`,
and further instances use `\\.\pipe\amiberry_1` through `\\.\pipe\amiberry_9`.
The pipe is byte-mode and only the account Amiberry runs as can open it, from
the local machine. Open it like a file, for example with
`System.IO.Pipes.NamedPipeClientStream` in .NET or `CreateFile` in C. Amiberry
keeps one pipe instance waiting for the next client and creates a new one
within a frame of each connection, so a client that gets `ERROR_PIPE_BUSY`
(231) should retry for a short time. Python's `open()` reports that error as
`EINVAL`; `tools/ipc_client.py` calls `_winapi.CreateFile` to see it. A pipe
cannot be half-closed: read the replies before closing
the handle. `tools/ipc_client.py` and the Amiberry MCP server work on both
transports.

Each request and response is one newline-terminated record. Request fields are
separated by tabs:

```text
COMMAND<TAB>ARGUMENT<TAB>ARGUMENT<NEWLINE>
```

A response starts with `OK` or `ERROR`; any following tab-separated fields are
command-specific. The `HELP` command lists all commands supported by the
running build.

A connection stays open until the client closes it. A controller can send many
requests over one connection or open a new connection per request, and up to
16 controllers can be connected at once. Requests on one connection are
answered in order. Amiberry closes a connection whose request grows past
64 KiB without a newline.

Commands run on the emulation thread between frames. Amiberry answers waiting
requests for a few milliseconds per frame, at least one per connection, and
keeps listening briefly so a controller that sends its next request as soon as
it reads the reply is not limited to one request per frame. Requests beyond
that budget wait for the next frame. Several requests can therefore take effect
in the same emulated frame. A controller that needs the Amiga to observe a
state, such as a key or mouse button being held down, must wait between the
press and the release instead of relying on one request per frame.

## Screenshot-driven GUI automation

The safe controller sequence is:

1. Capture a coherent screenshot and geometry snapshot with
   `SCREENSHOT <path> ACTIONABLE`.
2. Observe the current focus, input configuration, monitor, and revisions with
   `GET_GUI_AUTOMATION_STATE`.
3. If required, use `SET_GUI_AUTOMATION_CONFIG` as a guarded compare-and-set.
4. Convert the selected screenshot pixel into the window-logical coordinate
   space described by the capture, then send it with
   `SEND_MOUSE_ABS_GUARDED`.
5. Use `RELEASE_MOUSE_BUTTONS` for coordinate-free cleanup after any pressed
   button, including when geometry or revisions have changed.

Controllers must reject stale captures. Do not replay guarded input after a
runtime restart, geometry change, monitor change, or input-configuration
revision change. Capture a new actionable screenshot instead.

### `SCREENSHOT <path> ACTIONABLE`

Creates a PNG and atomically returns the geometry which made that exact image
actionable. Success has this fixed schema-version-1 field set:

```text
OK
schema_version=1
path=<path>
runtime_id=<process identity>
capture_nonce=<capture identity>
geometry_revision=<unsigned revision>
monitor_id=<active input monitor>
display_mode=<mode>
renderer=<renderer>
image_width=<PNG width>
image_height=<PNG height>
source_x=<source rectangle x>
source_y=<source rectangle y>
source_width=<source rectangle width>
source_height=<source rectangle height>
viewport_x=<window-logical viewport x>
viewport_y=<window-logical viewport y>
viewport_width=<window-logical viewport width>
viewport_height=<window-logical viewport height>
window_width=<window-logical width>
window_height=<window-logical height>
```

The fields are tab-separated on the wire; they are shown one per line above
for readability. `image_*` and `source_*` use screenshot pixels.
`viewport_*` and `window_*` use Amiberry window-logical coordinates. A
controller maps a point from the saved image through the source rectangle and
viewport exactly once. Raw absolute IPC commands do not accept screenshot
pixels.

Without `ACTIONABLE`, `SCREENSHOT <path>` remains the legacy screenshot
operation and returns only the path. It has no runtime, revision, or geometry
guard and must not be used as a relative-coordinate fallback for automation.

Actionable capture is currently supported by the SDL and OpenGL renderers.
The Vulkan renderer rejects `SCREENSHOT <path> ACTIONABLE` because its
threaded presentation path cannot yet pair captured surface pixels with the
geometry of the same presented frame. Legacy `SCREENSHOT <path>` remains
available with Vulkan, but it does not make the image safe for coordinate
automation.

### `GET_GUI_AUTOMATION_STATE`

Takes no arguments. A successful schema-version-1 response contains:

- `runtime_id`, `geometry_revision`, `monitor_id`, and `geometry_valid`
- `pending_tablet_mode` and `effective_tablet_mode`
- `pending_mouse_untrap` and `effective_mouse_untrap`
- `pending_effective_diverged` and `input_config_revision`
- `focus_ready`, `supported_button_mask`, and the current `button_mask`

Tablet modes are `off`, `mousehack`, and `real`. Mouse-untrap modes are `off`,
`middle`, `magic`, and `both`. A controller must not send guarded input unless
geometry is valid, focus is ready, and the effective input settings are
compatible.

### `SET_GUI_AUTOMATION_CONFIG`

```text
SET_GUI_AUTOMATION_CONFIG <expected_tablet_mode> <expected_mouse_untrap> <expected_input_config_revision> <tablet_mode> <mouse_untrap>
```

This command compares the expected pending settings and revision with the live
state before changing the pending settings. Success returns `reason=none` and
the resulting pending/effective values and `input_config_revision`. A stale or
externally changed state returns `ERROR`, `reason=input_config_conflict`, and
the current state. `reason=malformed_request` reports invalid arguments.

This compare-and-set is for a controller that temporarily owns settings and
later restores only the state it still owns. Public MCP and HTTP users should
not invoke it directly.

### `SEND_MOUSE_ABS_GUARDED`

```text
SEND_MOUSE_ABS_GUARDED <x> <y> <button_mask> <runtime_id> <geometry_revision> <monitor_id> <input_config_revision>
```

`x` and `y` are window-logical coordinates derived from the actionable
screenshot metadata. `button_mask` is the complete desired button mask: bit 0 is
left, bit 1 is right, and bit 2 is middle, so valid masks are 0 through 7. It
is a state assignment, not a named click action.

Amiberry applies the coordinate and button mask together only when all guards
still match. Every response includes `schema_version=1`, `applied`, `reason`,
the current `runtime_id`, `geometry_revision`, `monitor_id`,
`input_config_revision`, and `button_mask`; success also returns `x` and `y`.
Stable rejection reasons are:

- `geometry_invalid`
- `runtime_mismatch`
- `geometry_revision_mismatch`
- `monitor_mismatch`
- `input_config_revision_mismatch`
- `settings_incompatible`
- `focus_not_ready`
- `unsupported_button_mask`
- `coordinate_out_of_bounds`
- `input_rejected`
- `malformed_request`

On rejection, the guarded mutation is not applied. Controllers should use the
returned state to decide whether to retry setup or recapture; they must not
blindly replay the same mouse operation.

### `RELEASE_MOUSE_BUTTONS`

Takes no arguments and releases left, right, and middle without requiring a
coordinate or a valid capture. It is unconditional and idempotent. Success
returns `schema_version=1` and `button_mask=0`; a non-zero returned mask is an
error and cleanup remains unconfirmed.

This is the recovery primitive after a partial click or drag. Controller-level
click and drag implementations may use short internal dwell intervals (the
Amiberry MCP controller currently uses 50 ms between press/move/release
phases), but that timing and the raw button-mask sequence are private
implementation details rather than public API parameters.

## Settings edit session

The `SETTINGS_*` commands expose one edit session of the shared settings
layer (`src/osdep/settings/amiberry_settings.h`), so a controller can edit
the pending configuration end to end with validation and commit semantics
instead of the handful of `SET_CONFIG` options. Like every command they run
on the emulation thread. The session belongs to the IPC server as a whole
and is shared by every connected controller: one controller's COMMIT
commits every controller's staged edits, and one's RESET_SESSION discards
them all. Coordinate controllers accordingly.

- `SETTINGS_GET <id>` replies `OK <id>=<value>` with the value in `.uae`
  spelling. Unknown option IDs reply `ERROR Unknown option: <id>`.
- `SETTINGS_SET <id> <value>` stores a checked value in the session and
  applies the dependent changes the GUI applies. Invalid or unavailable
  values reply `ERROR` with the reason.
- `SETTINGS_VALIDATE` runs the core's configuration fixups as a preview and
  replies `OK` with one field per adjustment they would make
  (`<id>=<requested>-><adjusted> (<reason>)`), or `OK none`. The session is
  not modified and nothing is logged.
- `SETTINGS_COMMIT` stores the session into the pending configuration and
  replies `OK changed=<true|false> apply=<live|reset|restart>`, the
  strongest apply class among the changed options. When something changed it
  also signals the running emulation like `SET_CONFIG` does, so live options
  take effect at once. The session is closed afterwards.
- `SETTINGS_RESET_SESSION` discards the session's uncommitted edits.
- `SETTINGS_MOUNTS` replies `OK` with one field per mounted drive
  (directory, hardfile, CD or tape unit) in the order a `.uae` file lists
  them, each spelled as the value of its `uaehfN` line, for example
  `dir,rw,DH0:Work:/home/amiga/work,0`; `OK none` when there are none.
- `SETTINGS_MOUNT_ADD <value>` appends a drive given in that spelling. It is
  checked like the hard drive panel's additions: a malformed entry, a device
  name in use, a controller without a free unit or an entry too long for a
  configuration line replies `ERROR Invalid mount entry: <value>`, a full
  list (50 drives) `ERROR The mount list is full`.
- `SETTINGS_MOUNT_REPLACE <index> <value>` replaces drive `index` (from 0),
  checked the same way; the drive keeps its place in the list.
- `SETTINGS_MOUNT_REMOVE <index>` removes drive `index`; later drives move up.

Mount edits are staged in the session like `SETTINGS_SET` edits.
`SETTINGS_COMMIT` stores the whole list and reports `apply=reset` when it
changed: drives attach when the emulation resets.

A session that holds no uncommitted edits is reopened from
the current pending configuration at the next `SETTINGS_*` command, so a
configuration loaded or changed any other way (`LOAD_CONFIG`, `SET_CONFIG`,
the GUI) is what subsequent `SETTINGS_GET` and `SETTINGS_SET` see. Staged
edits are never dropped behind the controller's back: while any uncommitted
`SETTINGS_SET` or mount edit exists, the session stays open and isolated.

Option IDs and values are exactly the `.uae` keys and spellings the layer
defines; `amiberry_settings_count()`/`amiberry_settings_at()` list them.

## Debugger monitor

`MONITOR <text>` runs the same command text as GDB `monitor <text>`. The
command list and argument syntax are in `docs/gdb-debugging.md`. IPC joins the
tab-separated arguments with spaces. A path or name that contains spaces must
be sent as one argument. The reply is `OK` and one field per result line.
JSON replies are a single field. `disasm` returns one field per instruction
line. A tab inside a result line is replaced with a space so the field count
matches the line count. Failure is `ERROR` and a message.

The handler runs on the emulation thread, either between frames or while the
native debugger is stopped. Read-only commands do not require a stopped CPU.
Between frames the 68k core is not executing, so a guest memory read is stable
for the duration of the call. `cpu_threaded=true` is the exception: the CPU
thread can still be executing, so guest-memory reads and state changes are
refused. `capabilities`, `execution-status`, `input`, `screenshot`, and the
list/status commands that do not read guest RAM are still accepted.

These commands change debugger tables or remap memory banks, and require
`PAUSE` or a stopped native debugger:

- `watch add`, `watch range`, `watch remove`, `watch clear-events`
- `dma-watch add`, `dma-watch remove`
- `condition add`, `condition remove`
- `exception-mask`
- `history on`, `history off`
- `guest-output on`, `guest-output off`, `guest-output clear`
- `process-break name`, `process-break address`, `process-break clear`

`memwatch_setup()` remaps the watched banks. `map_banks` flushes the JIT
cache as part of that remap. The console debugger installs watches only while
the CPU is stopped; IPC uses the same rule. Host pause is a stop between
frames, so it is also accepted when the CPU is not threaded.

`step-over` is refused. It completes inside the GDB stop loop. Use
`DEBUG_STEP_OVER`, which requires the native debugger to be stopped.
`checkpoint save` and `checkpoint restore` are refused for the same reason.
Use `SAVESTATE` and `LOADSTATE`. Note that `SAVESTATE` and `QUICKSAVE` are
refused while the CPU is stopped by the debugger (native or GDB); resume
execution or `PAUSE` first so the snapshot is taken at a clean frame boundary.
`input sequence` is refused. Its steps advance from the frame hook only while
a GDB client is connected. Use `input event`.

A watch hit, exception-mask hit, condition, or process-break entry with no GDB
client stops in the native debugger. `DEBUG_STATUS` reports `stopped=1` and
`reason=` set to the stop text when one was produced. A read/write watch is
`T05awatch:`, a write watch is `T05watch:`, and a read watch is `T05rwatch:`.
Exception stops use `Tnnwinuae-exception:...`, process-break stops use
`T05winuae-entry:...`, and conditions use `T05breakpoint:N;`. `reason=` is
empty unless `stopped=1`, however the debugger was left. `execution-status`
reports the GDB halt flag, not this native stop.

`SET_BREAKPOINT`, `CLEAR_BREAKPOINT`, and `LIST_BREAKPOINTS` manage standard PC
breakpoints and leave monitor condition slots (`MONITOR condition add`) alone:
auto-allocation skips slots that hold conditions, requesting an explicit slot
that holds a condition is refused, and `CLEAR_BREAKPOINT ALL` clears only
standard breakpoints. Conditions must be removed through `MONITOR condition remove`.

IPC and a connected GDB client share one monitor object. IPC keeps answering
while GDB has the CPU stopped, but state-changing `MONITOR` commands,
`input event` and `input release` are refused while a GDB client is
connected. A GDB detach clears everything in that object, so a new
GDB connection is refused while IPC holds any of it: watches, DMA watches,
conditions, an exception mask, a
process-break, history recording, guest-output capture or captured records,
captured watch events, or input held by `input event`. Clear it before
attaching GDB with `watch remove`, `dma-watch remove`, `condition remove`,
`exception-mask 0`, `process-break clear`, `history off`, `guest-output off`
followed by `guest-output clear`, `watch clear-events`, and `input release`.
The last captured exception (`exception`) does not block GDB; it is dropped
when that GDB client detaches.

## Validation status

The geometry mapping and controller primitives have automated coverage in
`tests/test_gfx_geometry.sh` and `tests/test_ipc_gui_automation.sh`, including
HiDPI conversion, revision invalidation, stale-guard rejection, button masks,
and unconditional release. Both scripts also compile and pass on Windows with
the llvm-mingw toolchain. The Windows named-pipe transport and the
screenshot-driven automation commands have had a live end-to-end pass against
an installed release build: actionable capture, the guarded configuration
compare-and-set, guarded clicks and drags with held buttons read back from
the emulated CIA registers, focus gating, and stale-guard rejection after a
window resize and after a restart. Live end-to-end validation with packaged
Amiberry builds on Linux and macOS remains a separate release check and is
not implied by those automated tests.
