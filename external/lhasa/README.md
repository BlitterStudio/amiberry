# Lhasa

Vendored copy of [Lhasa](https://github.com/fragglet/lhasa), a pure-C
library for reading LHA (.lha/.lzh) archives.

- Upstream: https://github.com/fragglet/lhasa
- Release tag: **v0.6.0** (commit `75ed83559f23e9538e0045c62f53f77ab03d03d6`)
- Licence: ISC — see `COPYING.md`

Only the library (`lib/`, including `lib/public/`) is vendored; upstream's
command-line tools, tests and autotools build are not. Local changes, each
marked "Amiberry change" in the source:

- `process_level0_path()` in `lib/lha_file_header.c` also converts the
  `0xff` path separator that LHa for UNIX writes in level 0/1 names
  (upstream only converts `\`), so directory entries ending in it are
  recognised.
- `decode_level0_header()` in the same file accepts the oldest level 0
  headers, which omit the CRC after the name (two bytes shorter than
  upstream's minimum); their CRC reads as 0 and the glue doesn't check it.
- `lha_input_stream_skip()` in `lib/lha_input_stream.c` consumes bytes
  still in the lead-in buffer before skipping in the stream: a first
  header shorter than the lead-in otherwise misaligns the next one.
- MacLHA names use `:` as the path separator and allow `/`: Lhasa swaps
  the two, as LHa for UNIX does, while decoding level 0/1 names
  (`process_level0_path()`, with the Mac OS byte of a level 0 extended
  area recognised in `decode_level0_header()`) and in level 2+ filenames
  (`convert_macos_name()`), before headers are validated.

The tree is otherwise unmodified.

Lhasa replaces the old LHa for UNIX 1.14i code that used to live in
`src/archivers/lha/`: LHa for UNIX is non-free (its licence forbids
binary-only distribution and imposes commercial-use conditions), while the
ISC licence permits use in the App Store and direct-download builds alike.
The UAE glue that turns it into an Amiberry archiver is
`src/archivers/lha/uae_lha.cpp`.
