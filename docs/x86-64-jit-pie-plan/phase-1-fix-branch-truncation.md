# Phase 1: Fix Conditional Branch Offset Truncation

## Priority: HIGH (correctness bug)

## Problem

In `src/jit/x86/codegen_x86.cpp`, the conditional branch emitters (`raw_jl`,
`raw_jz`, `raw_jnz`, etc.) truncate the code pointer to 32 bits before
computing the relative displacement:

```c
static inline void raw_jl(uae_u32 t)
{
    raw_jcc_l_oponly(NATIVE_CC_LT);
    emit_long(t - (uae_u32)JITPTR target - 4);
}
```

The `(uae_u32)JITPTR target` cast truncates the current emit position to 32
bits. Under PIE+ASLR, JIT code could be above 4GB, producing wrong
displacements. The parameter `t` is also `uae_u32`, truncating the target.

**Note:** Under non-PIE this works because JIT memory is allocated in low 4GB
via MAP_32BIT. Under PIE, the anchor-based allocator places JIT memory near
`.data` which could be anywhere in the 47-bit VA space.

## Files to modify

- `src/jit/x86/codegen_x86.cpp` (lines ~1381-1397)

## Changes

Find all `raw_j*` functions that follow this pattern:

```c
static inline void raw_jl(uae_u32 t)
{
    raw_jcc_l_oponly(NATIVE_CC_LT);
    emit_long(t - (uae_u32)JITPTR target - 4);
}
```

Change each to use `uintptr` for the parameter type and pointer arithmetic:

```c
static inline void raw_jl(uintptr t)
{
    raw_jcc_l_oponly(NATIVE_CC_LT);
    emit_long((uae_u32)(t - (uintptr)target - 4));
}
```

**Key principle:** Do the subtraction at full pointer width (both operands are
in the same JIT block, so the difference is small), then truncate the *result*
to 32 bits for the x86 rel32 encoding.

### Functions to fix (search for `raw_jcc_l_oponly` and `raw_jmp_l_oponly` callers)

Grep for all functions matching the pattern. Expected functions include:
- `raw_jl`
- `raw_jz`
- `raw_jnz`
- `raw_jnl`  (if exists)
- `raw_jge`  (if exists)
- Any other `raw_j*` variants

Also check `raw_jmp` if it has a similar pattern.

## Verification

After this change:
- The code should compile cleanly with no warnings about integer truncation
- Grep for `(uae_u32)JITPTR target` — there should be no remaining instances
  in branch-related functions
- The relative displacement computation is now correct regardless of where in
  the address space the JIT code lives
