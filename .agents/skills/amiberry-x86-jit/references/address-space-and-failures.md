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

## Outstanding Work: make x86-64 JIT 64-bit clean (tracked in #1987, root cause #1983)

ARM64 is fully 64-bit pointer-clean (PC_P is the only 64-bit vreg, all others hold
32-bit M68k values; 19 fix points documented in the arm64 skill). x86-64 still has
32-bit arithmetic paths that truncate `natmem_offset` to 32 bits when it lives
above 4GB:

- `add_l_ri` / `adjust_nreg` / LEA sequences that fold natmem-relative offsets.
- PC_P helpers that emit 32-bit adds against `natmem_offset`.
- `comp_pc_p` arithmetic assumed to fit in 32 bits.

Current behavior (mitigation, not fix): `preinit_shm()` in `src/osdep/amiberry_mem.cpp`
detects `natmem_reserved + size > 4GB` on non-`CPU_AARCH64` 64-bit builds and:

- logs `MMAN: WARNING: natmem at ... exceeds 32-bit range on x86-64 - JIT not 64-bit clean`;
- sets `canbang = false`;
- forces `currprefs.cachesize = changed_prefs.cachesize = 0` so JIT is off.

This is safe but kills JIT on macOS x86-64 hosts where the kernel refuses every
low-address hint (e.g. heavy ASLR, 40 GB RAM).

### Reproducer (issue #1983)

- macOS 26.3 x86-64, 40 GB RAM, A1200 config with 68040/FPU, 128 MB Z3, JIT 16384 direct.
- `uae_vm_reserve(2GB, UAE_VM_32BIT)` loop 0x80000000 → 0x20000000 all return `0x11b0dc000`
  and get rejected as > 32-bit; fallback `VirtualAlloc(nullptr, ...)` shim drops
  `UAE_VM_32BIT` (`amiberry_mem.cpp:117`) and accepts the high address.
- Without the guard, JIT-emitted helper computes
  `(uae_u32)natmem_offset + amiga_pc = 0x1b0dc000 + 0xf80e5a = 0x1c05ce5a`
  and dereferences that garbage pointer → SIGSEGV in a C helper (PC outside JIT cache)
  → `exception_handler.cpp:950 Caught SIGSEGV outside JIT code` → abort.

### Path to a real fix

1. Audit and promote to 64-bit all sites that read `natmem_offset` into a register
   before indexing. Candidates (look for `NATMEM_OFFSET`, `natmem_offset`,
   `MEMBASEADDR` uses, `(uae_u32)...` casts near natmem arithmetic):
   - `src/jit/x86/compemu_support_x86.cpp` — `add_l_ri`, `adjust_nreg`, LEA emit paths.
   - `src/jit/x86/compemu_fpp_x86.cpp` — FPU natmem loads.
   - PC_P emission sites (search for `comp_pc_p`).
2. Remove or condition-out any `ADDR32` prefix emission (already done in some paths;
   confirm none remain in the hot loop).
3. Once clean, drop the `preinit_shm` guard above and switch `uae_vm_reserve` in
   `amiberry_mem.cpp:391` to also drop `UAE_VM_32BIT` on x86-64 (mirror the ARM64
   branch).
4. Verify on macOS x86-64 where the kernel always places natmem above 4GB, and on
   Linux x86-64 with PIE + aggressive ASLR.

### Secondary bug to fix alongside

The `VirtualAlloc` POSIX shim in `src/osdep/amiberry_mem.cpp:76-129` calls
`uae_vm_reserve(dwSize, 0)` unconditionally, dropping any 32-bit flag the caller
intended. If `UAE_VM_32BIT` handling is preserved long-term, the shim should take
a flag arg or the caller should go through `uae_vm_reserve` directly.
