# RTG And Picasso96 Troubleshooting

Use this reference for RTG/Picasso96 color-channel, BGRA/RGBA, zero-copy, and presentation regressions.

## Core Invariants

- Keep guest-visible RTG mode flags intact. Do not "fix" rendering by migrating a config from `RGBFF_B8G8R8A8` to `RGBFF_R8G8B8A8`.
- Treat guest `RGBFTYPE` and host `SDL_PixelFormat` as separate concepts.
- Pick the host SDL surface format that can present the current guest RTG format directly.
- Check per-monitor state. Do not assume monitor 0 when the path receives `monid`.
- Keep 15/16-bit RTG modes on the mature 32-bit host presentation path unless every backend involved supports native 16-bit presentation.

## Important Code Paths

- `src/cfgfile.cpp`: compatibility helpers must preserve guest-visible RTG pixel-format intent.
- `src/osdep/gfx_colors.cpp` and `.h`: host pixel-format selection helpers live here.
- `src/osdep/amiberry_gfx.cpp`: calls `update_pixel_format(monid)` and decides zero-copy eligibility.
- `src/osdep/picasso96.cpp`: sets `host_mode`, conversion tables, and copy/scale conversion cases.

## Debugging Checklist

1. Identify the guest RTG `RGBFormat` in `picasso96_state[monid]`.
2. Identify whether the RTG board is hardware-backed through `currprefs.rtgboards[i]`.
3. Resolve the host SDL format from guest format plus hardware RTG state.
4. Allow zero-copy only when guest pixels match the host SDL format.
5. Verify both direct copy and scaled copy paths for the affected format.
6. Test both native UAE RTG and hardware RTG where relevant.

## Common Traps

- Changing config compatibility to rewrite guest mode flags instead of fixing presentation.
- Assuming Amiberry is always RGBA for all RTG boards.
- Testing only the normal copy path and missing `copyrow_scale()`.
- Fixing monitor 0 while multi-monitor state still uses stale or mismatched formats.
- Treating `rtg_nocustom` as equivalent to any active RTG screen; it is a preference gate.
