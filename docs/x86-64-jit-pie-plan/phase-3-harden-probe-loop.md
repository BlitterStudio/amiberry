# Phase 3: Harden vm_acquire Probe Loop

## Priority: MEDIUM (reliability under hardened kernels)

## Problem

The existing probe loop in `compemu_support_x86.cpp` (lines ~210-269) uses
`mmap` with address hints to place JIT memory within ±1.75GB of `.data`. Under
standard kernels this works well, but:

1. Some hardened kernels may ignore mmap hints more aggressively
2. Without `MAP_FIXED_NOREPLACE`, mmap can silently place the allocation at a
   completely different address (the hint is just a hint)
3. The current code checks distance after the fact, but the fallback at line
   265-268 uses `uae_vm_alloc(size, 0, ...)` which gets OS-chosen placement
   that could be far away

## Files to modify

- `src/jit/x86/compemu_support_x86.cpp` (~lines 210-269)

## Changes

### 3a: Use MAP_FIXED_NOREPLACE when available

In the probe loop's `mmap` call, add `MAP_FIXED_NOREPLACE` (Linux 4.17+).
With this flag, `mmap` either places at the exact requested address or returns
`MAP_FAILED` (never silently relocates):

```c
#ifdef MAP_FIXED_NOREPLACE
    int extra_flags = MAP_FIXED_NOREPLACE;
#else
    int extra_flags = 0;
#endif

    result = mmap((void *)try_addr, size, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | extra_flags, -1, 0);
```

When `MAP_FIXED_NOREPLACE` is available, the distance check after mmap is
technically redundant but should be kept as a safety net.

When `MAP_FIXED_NOREPLACE` is not available (older kernels, macOS, FreeBSD),
the existing behavior (hint + distance check) remains.

### 3b: Improve fallback error handling

The fallback at the end of the probe loop currently does:

```c
result = uae_vm_alloc(size, 0, UAE_VM_READ_WRITE);
```

This can place memory anywhere. Add a distance check after this fallback:

```c
result = uae_vm_alloc(size, 0, UAE_VM_READ_WRITE);
if (result) {
    intptr_t dist = (intptr_t)result - (intptr_t)&data_anchor;
    if (llabs(dist) >= 0x70000000) {
        write_log("JIT: FATAL: could not allocate cache within 2GB of globals "
                   "(cache=%p anchor=%p dist=%lld)\n",
                   result, &data_anchor, (long long)dist);
        uae_vm_free(result, size);
        result = NULL;
        // JIT will be disabled (cachesize = 0)
    }
}
```

### 3c: Log the successful allocation

After a successful allocation with acceptable distance, log it:

```c
write_log("JIT: cache allocated at %p (anchor=%p, dist=%+lld)\n",
          result, &data_anchor, (long long)((intptr_t)result - (intptr_t)&data_anchor));
```

## Verification

- Build and run on a stock Linux kernel: probe loop should succeed on first or
  second attempt
- Check `--log` output for the allocation address and distance
- Distance should be well under 1.75GB (0x70000000)
- On older kernels without `MAP_FIXED_NOREPLACE`, behavior should be unchanged
