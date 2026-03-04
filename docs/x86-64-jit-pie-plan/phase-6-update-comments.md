# Phase 6: Update Comments

## Priority: LOW (documentation only)

## Files to modify

- `src/jit/x86/compemu_fpp_x86.cpp` (line ~738)
- `src/jit/x86/compemu_support_x86.cpp` (near the removed #error, and near
  the anchor allocator)

## Changes

### 6a: comp_pc_p assert comment

In `src/jit/x86/compemu_fpp_x86.cpp`, find the assert around line 738-739:

```c
assert((uintptr) comp_pc_p <= 0xffffffffUL);
```

Add or update the comment above it to clarify the constraint:

```c
// comp_pc_p points into natmem, which is allocated with UAE_VM_32BIT
// (low 4GB) on x86-64. This is independent of PIE/ASLR — natmem
// placement uses explicit mmap hints, not .text/.data ASLR.
assert((uintptr) comp_pc_p <= 0xffffffffUL);
```

### 6b: Anchor allocator documentation

In `src/jit/x86/compemu_support_x86.cpp`, near the `data_anchor` variable and
`vm_acquire` function, add a block comment explaining the PIE strategy:

```c
/*
 * JIT cache allocation strategy (PIE-compatible):
 *
 * x86-64 JIT uses RIP-relative [RIP+disp32] addressing to access globals
 * (regs, regflags, etc.) via the _r_X() macro. This requires the JIT code
 * cache to be within ±2GB of .data.
 *
 * Under non-PIE, .data is at a fixed low address and MAP_32BIT suffices.
 * Under PIE+ASLR, .data can be anywhere in the 47-bit VA space.
 *
 * We use an anchor-based probe: &data_anchor is a reference point in .data,
 * and we search outward from it for a free VA range. This mirrors the
 * Windows strategy (VirtualQuery walk from data_anchor).
 *
 * The probe covers ±1.75GB at 2MB intervals, well within the ±2GB
 * RIP-relative limit.
 */
```

### 6c: FreeBSD comment in CMake

If not already added in Phase 5, ensure the CMake condition for FreeBSD
has a clear comment explaining why FreeBSD is the exception.

## Verification

- Review comments make sense and accurately describe the design
- No functional changes in this phase — build should be identical
