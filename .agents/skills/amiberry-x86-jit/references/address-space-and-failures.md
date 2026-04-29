# x86-64 JIT Address Space And Failure Reference

## Current Local Symbols

- `jit_vm_acquire(uae_u32 size, int options)`
- `vm_acquire_anchor`
- `find_nearest_gap(uintptr base, uae_u32 size, uintptr range)`
- `disable_jit_on_runtime_alloc_failure(const char *what)`
- `invalidate_block(blockinfo* bi)`
- `flush_icache_hard(int)`
- `jit_n_addr_unsafe`
- `jit_n_addr_bank_unsafe`
- `jit_use_memory_helpers()`
- `jit_use_compile_fallbacks()`

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

## High-Natmem Direct JIT Support (issue #1987)

x86-64 high natmem is supported. Natmem may live above 4GB and the JIT should keep
direct compiled performance unless a real special memory bank requires helper
paths.

Current behavior:

- `preinit_shm()` may reserve 4GB natmem on 64-bit JIT builds and drops the low-address
  requirement for x86-64.
- `memory_reset()` sets `jit_n_addr_unsafe` when natmem exceeds the 32-bit range.
- `map_banks()` sets `jit_n_addr_bank_unsafe` only when a real bank advertises
  `S_N_ADDR`; this is the signal for broad helper/fallback behavior.
- `R_MEMSTART` holds `natmem_offset` on x86-64, and direct loads/stores use it as the
  64-bit base register.
- `PC_P` uses 64-bit loads, stores, cmovs, and adds. Non-`PC_P` virtual registers remain
  32-bit M68K values.

Key fixes that must not regress:

- `set_const()` masks non-`PC_P` constants to 32 bits, so host pointers must not be
  stored as generic constants in scratch virtual registers.
- `mov_l_rr()` special-cases constant `PC_P` copies with a 64-bit materialized register;
  the generic `mov_l_ri()` shortcut would truncate when the destination is not `PC_P`.
- Non-constant `PC_P` copies must use `readreg()` rather than `readreg_offset()` when the
  logical value, including deferred offset, is required.
- FBcc target handling keeps signed displacement separate from the 64-bit host pointer.
- Helper fallback paths update `regs.pc_p`, `regs.pc_oldp`, `regs.pc`, and
  `regs.instruction_pc` with pointer-width and M68K-width values respectively.
- Generic H3/default memory-fault logging must not carry x86 JIT investigation tags.

Historical reproducer:

- macOS x86-64 or Linux/WSL with natmem above 4GB, A1200 68040/FPU, Z3, JIT direct.
- Older code truncated `natmem_offset` or `PC_P`, causing H3/software failures, yellow
  reset loops, or interpreter-level performance after fallback.

Verification checklist:

- Build Windows and Linux x86-64 release targets.
- Run high-natmem SysInfo/Workbench smoke with logging; expect timeout/normal run, not
  H3, software failure, CPU halted, or `Register 16 should be constant`.
- Confirm JIT-level performance, not interpreter-level MIPS.
