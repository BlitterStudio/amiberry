# OS4 Filesystem Packet Troubleshooting

Use this reference for `src/filesys.cpp`, AmigaOS 4 DOS packet compatibility, and unknown packet fallback behavior.

## Core Invariants

- Unsupported OS4 filesystem packets should fail explicitly with `DOS_FALSE` and `ERROR_ACTION_NOT_KNOWN`.
- Explicit fallback is better than falling through into unrelated packet handling or returning ambiguous success.
- Add packet constants near the existing OS4 64-bit filesystem packet definitions.
- Keep behavior narrow unless the packet semantics are implemented fully.

## Important Packet Cases

- `ACTION_FILESYSTEM_ATTR` is OS4 filesystem metadata support.
- `ACTION_CHANGE_FILE_POSITION64`, `ACTION_GET_FILE_POSITION64`, `ACTION_CHANGE_FILE_SIZE64`, and `ACTION_GET_FILE_SIZE64` are implemented 64-bit file operations.
- `ACTION_EXAMINEDATA`, `ACTION_EXAMINEDATA_FH`, `ACTION_EXAMINEDATA_OBJ`, and `ACTION_EXAMINEDATA_DIR` should currently return `ERROR_ACTION_NOT_KNOWN` unless full examinedata semantics are implemented.

## Debugging Checklist

1. Identify the numeric packet action from logs or debugger state.
2. Check whether WinUAE implements the packet or only rejects it explicitly.
3. If unsupported, add a named constant and explicit `handle_packet()` case returning `DOS_FALSE` and `ERROR_ACTION_NOT_KNOWN`.
4. If implementing support, verify lock/file-handle/object semantics across OS3, OS4, and MorphOS packet paths.
5. Test software fallback behavior after `ERROR_ACTION_NOT_KNOWN`.

## Common Traps

- Returning success with incomplete result fields.
- Letting unknown OS4 packets fall through to MOS packet cases.
- Implementing a packet for one object type while ignoring file-handle and directory variants.
- Treating OS4 packet compatibility as a CLI parsing or mount setup issue before checking packet dispatch.
