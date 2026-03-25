# x86-64 JIT Address Space And Failure Reference

## Current Local Symbols

- `jit_vm_acquire(uae_u32 size, int options)`
- `vm_acquire_anchor`
- `find_nearest_gap(uintptr base, uae_u32 size, uintptr range)`
- `disable_jit_on_runtime_alloc_failure(const char *what)`
- `invalidate_block(blockinfo* bi)`
- `flush_icache_hard(int)`

## Recent Root-Cause Fixes

### 1. Linux near-address allocation must be VMA-aware

Blind probing in large steps can miss usable holes close to the anchor. The local code now has `find_nearest_gap()` on Linux to walk `/proc/self/maps` and claim the closest gap within range.

### 2. `MAP_32BIT` is not sufficient by itself

`UAE_VM_32BIT` / `MAP_32BIT` can return a low address that is still too far from the actual JIT base under PIE or high-ASLR layouts. Always validate the distance after allocation.

### 3. Retry logic must cover range rejection, not only OS failure

If the `MAP_32BIT` attempt returns a pointer that later fails the distance check, the allocator should still try the next fallback path before giving up.

### 4. Validate Windows last-resort `VirtualAlloc`

The Windows fallback can return an address that the OS accepts but the JIT cannot safely reference. Free and reject out-of-range results.

### 5. Runtime JIT disable must persist

When an allocation failure disables JIT, update both:

- `currprefs.cachesize`
- `changed_prefs.cachesize`

Otherwise a later prefs-apply path can silently re-enable JIT and hit the same failure again.

### 6. Partial compilation failure needs cleanup

If block compilation fails after `bi` has been partially initialized:

- call `invalidate_block(bi)`
- call `flush_icache_hard(3)` before returning

This prevents stale `cache_tags[]` or half-built blocks from executing after fallback.

## Practical Review Checklist

- What is the effective base for the distance calculation on this path?
- Does every fallback path validate the final pointer against that base?
- Can a pool allocator bypass a check that exists only in the main cache path?
- If JIT disables itself, can any later preference sync undo that?
- Does every early return after partial setup clean up block state?
