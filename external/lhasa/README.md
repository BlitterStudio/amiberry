# Lhasa

Vendored copy of [Lhasa](https://github.com/fragglet/lhasa), a pure-C
library for reading LHA (.lha/.lzh) archives.

- Upstream: https://github.com/fragglet/lhasa
- Release tag: **v0.6.0** (commit `75ed83559f23e9538e0045c62f53f77ab03d03d6`)
- Licence: ISC — see `COPYING.md`

Only the library (`lib/`, including `lib/public/`) is vendored; upstream's
command-line tools, tests and autotools build are not. The upstream tree is
otherwise unmodified.

Lhasa replaces the old LHa for UNIX 1.14i code that used to live in
`src/archivers/lha/`: LHa for UNIX is non-free (its licence forbids
binary-only distribution and imposes commercial-use conditions), while the
ISC licence permits use in the App Store and direct-download builds alike.
The UAE glue that turns it into an Amiberry archiver is
`src/archivers/lha/uae_lha.cpp`.
