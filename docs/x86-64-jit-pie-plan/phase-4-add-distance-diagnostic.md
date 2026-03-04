# Phase 4: Add Runtime Distance Diagnostic

## Priority: LOW (diagnostic / safety net)

## Problem

If the JIT cache is accidentally placed too far from globals, the result is
silent corruption (wrong RIP-relative offsets). A runtime diagnostic provides
early detection.

## Files to modify

- `src/jit/x86/compemu_support_x86.cpp` (in `alloc_cache` or `build_comp`,
  after `compiled_code` / JIT cache is allocated)

## Changes

After the JIT cache allocation succeeds, add a proximity check:

```c
// Verify JIT cache is within RIP-relative reach of globals
{
    intptr_t dist = (intptr_t)compiled_code - (intptr_t)&regs;
    write_log("JIT: code cache at %p, regs at %p, distance=%+lld bytes\n",
              compiled_code, &regs, (long long)dist);
    if (llabs(dist) > (intptr_t)0x70000000) {
        write_log("JIT: WARNING: code cache is %lld bytes from globals — "
                  "RIP-relative addressing may fail! Disabling JIT.\n",
                  (long long)dist);
        // Disable JIT to prevent silent corruption
        changed_prefs.cachesize = 0;
        currprefs.cachesize = 0;
    }
}
```

This uses `regs` as the reference point because it's the most frequently
accessed global via `_r_X` RIP-relative addressing.

## Verification

- Run with `--log` and verify the distance is printed
- On a normal PIE build, distance should be well under 1.75GB
- If you want to test the warning path, you could temporarily hack the
  threshold to a very small value
