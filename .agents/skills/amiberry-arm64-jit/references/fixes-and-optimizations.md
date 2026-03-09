# Fixes and Optimizations — Code-Level Reference

## Fix 1: Inter-block dont_care_flags()

**File**: `src/jit/arm/compemu_support_arm.cpp` in `compile_block()`

**Location**: Near end of compile_block(), after the main instruction loop, in the inter-block optimization block (search for `dont_care_flags` near `if (bi1)` block).

**The fix** (2 lines added):
```c
// Before dont_care_flags(), flush flags to memory for the slow path
flush_flags();       // NEW: saves NZCV → regflags.nzcv if flags are in register
dont_care_flags();   // existing: marks flags unimportant for successor fast path
```

**Why it works**: The slow path (countdown < 0 → interpreter) reads flags from `regflags.nzcv` in memory. Without `flush_flags()`, `dont_care_flags()` causes `flush(1)` to skip the MRS+STR, leaving stale values in memory.

## Fix 2: Two-pass Liveflags

**File**: `src/jit/arm/compemu_support_arm.cpp` in `compile_block()`

**Location**: Before `remove_deps(bi)`, after block compilation is complete.

```c
// Collect successor flags from dependent blocks
uae_u8 successor_flags = 0;
for (int i = 0; i < 2; i++) {
    if (bi->dep[i].target) {
        successor_flags |= bi->dep[i].target->needed_flags;
    }
}
// Use as starting point instead of FLAG_ALL
liveflags[blocklen] = successor_flags;
```

## Fix 3: FBcc Branch Target Truncation

**File**: `src/jit/arm/compemu_fpp_arm.cpp` in `comp_fbcc_opp()`

**Before** (buggy — passes 64-bit host pointer through 32-bit set_const):
```c
uintptr target = (uintptr)comp_pc_p + displacement;
mov_l_ri(tmp, target);  // calls set_const → truncates to 32 bits!
```

**After** (uses displacement + arm_ADD_l_ri for safe 64-bit arithmetic):
```c
// Get current PC_P into a temp register
int tmp = writereg(S1);
mov_l_rr(tmp, PC_P_REG);        // copy PC_P (64-bit safe)
arm_ADD_l_ri(tmp, displacement); // sign-extends displacement correctly
```

**Pattern to watch for**: ANY call to `set_const()` or `mov_l_ri()` with a host pointer value on a non-PC_P register is a potential truncation bug.

## Fix 4: BSR.L/Bcc.L Sign Extension

**Files**: `src/jit/arm/compemu_arm.cpp`, `src/jit/arm/gencomp_arm.c`

**Before** (zero-extends):
```c
mov_l_ri(src, comp_get_ilong((m68k_pc_offset += 4) - 4));
```

**After** (sign-extends):
```c
mov_l_ri(src, (uae_s32)comp_get_ilong((m68k_pc_offset += 4) - 4));
```

Applied to 32 handlers: BSR.L (`0x61ff`), BRA.L (`0x60ff`), and 14 Bcc.L conditions × `_ff`/`_nf` variants.

Generator fix in `gencomp_arm.c`: Added `(uae_s32)` cast for `i_BSR` (sz_long) and `i_Bcc` (sz_long case).

## Fix 5: ADDX/SUBX Z-flag

**File**: `src/jit/arm/compemu_midfunc_arm64_2.cpp`

**Functions**: `jff_ADDX_b`, `jff_ADDX_w`, `jff_ADDX_l`, `jff_SUBX_b`, `jff_SUBX_w`, `jff_SUBX_l`

All Z-flag related instructions wrapped in `if (needed_flags & FLAG_Z)`:
- Before operation: MOVN+MOVN+CSEL to save old Z state
- After MRS: UBFX+CMP+SET_xxZflag+CSEL+AND to implement "only clears" semantics
- When `!(needed_flags & FLAG_Z)`: just MRS, saving 4-8 instructions

## Fix 5b: ADDX byte/word MOVN_xi Dual-Purpose Initialization

**File**: `src/jit/arm/compemu_midfunc_arm64_2.cpp`

**Functions**: `jff_ADDX_b`, `jff_ADDX_w`

**Before** (buggy — MOVN_xi inside Z guard):
```c
if (needed_flags & FLAG_Z) {
    MOVN_xi(REG_WORK1, 0);              // skipped when Z not needed!
    MOVN_xish(REG_WORK2, 0x4000, 16);
    CSEL_xxxc(REG_WORK2, REG_WORK2, REG_WORK1, NATIVE_CC_NE);
}
BFI_xxii(REG_WORK1, s, 24, 8);  // only sets bits 24-31, bits 0-23 = garbage
ADCS_www(REG_WORK1, REG_WORK1, REG_WORK3);  // carry corrupted by garbage
```

**After** (MOVN_xi unconditional):
```c
// REG_WORK1 must be all-ones: bits 0-23 are padding for the byte BFI+ADCS
// carry chain, and also serves as the all-ones value for sticky-Z CSEL.
MOVN_xi(REG_WORK1, 0);  // ALWAYS execute
if (needed_flags & FLAG_Z) {
    MOVN_xish(REG_WORK2, 0x4000, 16);
    CSEL_xxxc(REG_WORK2, REG_WORK2, REG_WORK1, NATIVE_CC_NE);
}
BFI_xxii(REG_WORK1, s, 24, 8);  // bits 0-23 are all-ones, correct padding
ADCS_www(REG_WORK1, REG_WORK1, REG_WORK3);  // carry computed correctly
```

**Not affected**: `jff_SUBX_b/w` (uses `LSL_wwi` which fully initializes both operands, zeroing lower bits), `jff_ADDX_l/jff_SUBX_l` (full 32-bit operations, no byte packing).

**Symptom**: Vertical teal/cyan corruption lines on A4000 AGA red gradient wallpaper. Only manifests when two-pass liveflags optimization eliminates FLAG_Z from ADDX_b/w instructions used in AGA rendering.
## Fix 6: arm_ADD_l / arm_ADD_l_ri Sign Extension

**File**: `src/jit/arm/compemu_midfunc_arm64.cpp`

**arm_ADD_l** — when destination is PC_P:
```c
if (isPC_P(d)) {
    ADD_xxwEX(d, d, s, EX_SXTW, 0);  // sign-extend W→X before adding
} else {
    ADD_www(d, d, s);
}
```

**arm_ADD_l_ri** — when destination is PC_P and value doesn't fit immediate:
Uses `LOAD_U64` for the full 64-bit sign-extended value, then `ADD_xxx`.

**arm_ADD_l_ri8 / arm_SUB_l_ri8** — PC_P check added:
```c
if (isPC_P(d)) {
    // Use sign-extending path
} else {
    ADD_wwi(d, d, imm);  // normal 32-bit
}
```

## Fix 7: set_const 32-bit Masking

**File**: `src/jit/arm/compemu_midfunc_arm64.cpp` (also in compemu_support_arm.cpp)

```c
STATIC_INLINE void set_const(int r, uae_u32 val)
{
    if (r != PC_P) {
        val &= 0xFFFFFFFF;  // mask to 32 bits for M68K registers
    }
    // ... rest of set_const
}
```

## Fix 10: Bus Error Recovery

**File**: `src/newcpu.cpp` in `m68k_run_jit()`

```c
// OUTSIDE the inner loop (once per dispatch entry):
static jmp_buf bus_error_jmp;
if (setjmp(bus_error_jmp) != 0) {
    jit_in_compiled_code = false;
    // ... reset state, continue
}

// Around JIT execution:
jit_in_compiled_code = true;
r->handler();           // execute compiled block
jit_in_compiled_code = false;
```

**File**: `src/osdep/sigsegv_handler.cpp`
```c
if (jit_in_compiled_code) {
    longjmp(bus_error_jmp, 1);
}
```

## R_REGSTRUCT Range Optimization

**File**: `src/jit/arm/codegen_arm64.cpp`

**Functions**: `compemu_raw_mov_l_mr`, `compemu_raw_mov_l_mi`, `compemu_raw_mov_l_rm`

**Before** (strict struct membership check):
```c
if (d >= (uae_u32*)&regs && d < (uae_u32*)((char*)&regs + sizeof(struct regstruct))) {
    int idx = (uae_u8*)d - (uae_u8*)&regs;
    STR_wXi(s, R_REGSTRUCT, idx);
```

**After** (range-based offset check):
```c
uintptr idx = (uintptr)d - (uintptr)&regs;
if (idx <= 16380 && (idx & 3) == 0) {
    STR_wXi(s, R_REGSTRUCT, (int)idx);
```

**Why**: `regflags` is a separate global near `regs` in BSS. The old check excluded it (not inside struct), forcing the slow LOAD_U64 path (4-5 instructions) for EVERY flag store at EVERY block exit. The new check accepts anything within STR_wXi range of R_REGSTRUCT.

## Modified Files Summary (17 files)

| File | Lines Changed | Category |
|---|---|---|
| `src/jit/arm/compemu_support_arm.cpp` | +526/-709 | Inter-block fix, liveflags, set_const, guards removed, diagnostics removed |
| `src/jit/arm/codegen_arm64.cpp` | +36/-1 | R_REGSTRUCT range, 32-bit width fixes, I-cache flush |
| `src/jit/arm/compemu_midfunc_arm64.cpp` | +82/-5 | sign-extension, set_const masking, PC_P checks |
| `src/jit/arm/compemu_midfunc_arm64_2.cpp` | +98/-6 | ADDX/SUBX Z-flag fix + skip, MOVE16 |
| `src/jit/arm/compemu_fpp_arm.cpp` | +19/-1 | FBcc truncation fix |
| `src/newcpu.cpp` | +101/-7 | setjmp/bus error, dispatch loop, diagnostics removed |
| `src/jit/compemu.h` | +5/0 | jit_in_compiled_code extern |
| `src/machdep/m68k.h` | +8/-1 | FLAGBIT_X=0 |
| `src/osdep/sigsegv_handler.cpp` | +34/-1 | SIGBUS/longjmp, quarantine |
| `src/expansion.cpp` | +39/-3 | Removed quarantine warning stubs |
| `src/memory.cpp` | +82/-5 | commit_natmem_gaps call, size_t |
| `src/include/memory.h` | +4/-1 | size_t declarations |
| `src/include/uae/vm.h` | +16/-1 | size_t, UAE_VM_32BIT removed |
| `src/vm.cpp` | +74/-7 | size_t throughout |
| `src/osdep/amiberry_mem.cpp` | +94/-6 | commit_natmem_gaps(), size_t |
| `src/osdep/imgui/ram.cpp` | +6/-1 | size_t for display |
| `src/main.cpp` | +4/-1 | size_t |

## Investigation History (Key Discoveries)

These disproved hypotheses are preserved to prevent re-investigation:

1. **BFI_wwii (32-bit) vs BFI_xxii (64-bit)**: BFI_wwii is CORRECT. 32-bit defensively zeroes upper bits. Reverting to 64-bit CAUSED H3 crashes.
2. **Infrastructure 32→64-bit width changes**: 32-bit is CORRECT. Reverting CAUSED H3 crashes.
3. **Individual opcode handler bugs**: All handlers verified correct via exhaustive line-by-line comparison with Amiberry-Lite.
4. **cft_map() in build_comp()**: Identity function on ARM64, zero effect.
5. **ROM overlay NATMEM mismatch**: x86 JIT has same behavior and works fine.
6. **COMP_DEBUG = 1**: Performance only, no data corruption. Assertions cause reset, not corruption.
7. **Chipset emulation differences**: x86 JIT uses same chipset code and works — NOT ARM64-specific.
8. **Cycle counting in execute_normal**: Affects timing, not data — x86 uses same and works.
