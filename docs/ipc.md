# IPC controller interface

Amiberry exposes a line-oriented Unix-domain socket when it is built with
`USE_IPC_SOCKET=ON`. The interface is available on Linux and macOS. It is used
by controller software such as the
[Amiberry MCP server](https://github.com/BlitterStudio/amiberry-mcp-server),
which presents stable, action-level MCP tools and HTTP endpoints.

Application integrations should use those MCP or HTTP interfaces. The raw
commands described here are controller primitives: they expose window-logical
coordinates, complete button bitmasks, runtime identities, revisions, and
configuration compare-and-set operations. They are not intended as a direct
public automation API.

## Transport

The first instance listens on `$XDG_RUNTIME_DIR/amiberry.sock`, or
`/tmp/amiberry.sock` when `XDG_RUNTIME_DIR` is not set. Further instances use
`amiberry_1.sock` through `amiberry_9.sock` in the same directory.

Each request and response is one newline-terminated record. Request fields are
separated by tabs:

```text
COMMAND<TAB>ARGUMENT<TAB>ARGUMENT<NEWLINE>
```

A response starts with `OK` or `ERROR`; any following tab-separated fields are
command-specific. The `HELP` command lists all commands supported by the
running build.

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

## Validation status

The geometry mapping and controller primitives have automated coverage in
`tests/test_gfx_geometry.sh` and `tests/test_ipc_gui_automation.sh`, including
HiDPI conversion, revision invalidation, stale-guard rejection, button masks,
and unconditional release. Live end-to-end validation with packaged Amiberry
builds on Linux and macOS is a separate release check and is not implied by
those automated tests.
