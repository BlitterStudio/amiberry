# ARM64 JIT Architecture — Deep Reference

## Compilation Pipeline

### Block Lifecycle
1. `execute_normal()` runs interpreter for `MAXRUN` instructions, building `pc_hist[]` trace
2. `compile_block()` compiles the trace into native ARM64 code
3. Compiled block stored in JIT code cache (`compiled_code` → `current_compile_p`)
4. Block chaining: blocks link directly via hash table for fast transitions
5. On `countdown < 0`: exit to C via `popall_do_nothing` → `do_nothing()` → interpreter

### compile_block() Structure
```
1. Look up/create blockinfo for PC
2. For each instruction in pc_hist[]:
   a. Compute needed_flags via liveflags[] backward propagation
   b. Select handler: comptbl[opcode] (flag-setting) or compnfctbl[opcode] (no-flags)
   c. Call handler → emits ARM64 instructions
3. Inter-block optimization:
   a. flush_flags()     ← CRITICAL: spill NZCV to memory FIRST
   b. dont_care_flags() ← then mark flags unimportant for fast path
4. Flush registers, emit endblock code
5. Link to successor blocks via deps
```

### Two-pass Liveflags
Before `remove_deps(bi)`, the code iterates `bi->dep[]` successor blocks and OR's their `needed_flags` into `successor_flags`. This becomes `liveflags[blocklen]` — the starting point for backward flag propagation. Without this, `liveflags[blocklen] = FLAG_ALL` (pessimistic).

### Inter-block Flag Fix Detail
The inter-block optimization exists at the end of `compile_block()`:
```c
// BEFORE the fix (buggy):
dont_care_flags();   // sets live.flags_are_important = 0
flush(1);            // flush(1) calls flags_to_stack() which checks flags_are_important
                     // → flags NOT saved to memory
                     // → interpreter reads stale regflags.nzcv → CRASH

// AFTER the fix:
flush_flags();       // MRS + STR: saves NZCV to regflags.nzcv (for slow path / interpreter)
dont_care_flags();   // marks flags unimportant (successor block can skip loading them)
flush(1);            // flags already saved, this is now safe
```

## Register Allocator

### Virtual → Physical Mapping
- 16 virtual data registers (D0-D7, A0-A7 mapped to 0-15)
- PC_P = 16, FLAGX = 17, FLAGTMP = 18
- Physical: ARM64 W0-W15 (32-bit) for M68K data, X-register for PC_P only
- `live.state[r]` tracks each virtual register's status (in-register, on-stack, const)

### Const Propagation
- `set_const(r, val)`: marks register as holding a known constant
- For non-PC_P: `val &= 0xFFFFFFFF` (32-bit mask)
- For PC_P: NO mask (preserves full 64-bit host pointer)
- `isconst(r)`: checks if register holds a known constant
- Const values used for compile-time optimization (fold address calculations, skip loads)

### Flag State Machine
- `live.flags_on_stack`: NZCV is in `regflags.nzcv` memory
- `live.flags_in_flags`: NZCV is in ARM64 NZCV register
- `live.flags_are_important`: whether flags must be preserved
- `flags_to_stack()` / `flags_to_reg()`: transition between states
- `flush_flags()`: unconditionally saves NZCV to memory if in register
- `dont_care_flags()`: sets `flags_are_important = 0`

## ARM64 Codegen Patterns

### R_REGSTRUCT-relative Access (Fast Path)
```
// When offset ≤ 16380 and 4-aligned:
STR_wXi(src, R_REGSTRUCT, offset)    // 1 instruction

// When offset out of range:
LOAD_U64(tmp, absolute_addr)          // 1-4 instructions
STR_wXx(src, tmp, 0)                  // 1 instruction
```

The range check in `compemu_raw_mov_l_mr/mi/rm` uses:
```c
uintptr idx = (uintptr)d - (uintptr)&regs;
if (idx <= 16380 && (idx & 3) == 0) {
    // Fast: STR_wXi relative to R_REGSTRUCT
} else {
    // Slow: LOAD_U64 + STR
}
```
This covers `regflags` (separate global near `regs` in BSS) via range, not struct membership.

### 64-bit Pointer Arithmetic (PC_P)
```c
// CORRECT: sign-extend 32-bit displacement when adding to PC_P
if (isPC_P(d)) {
    // Use SXTW extension: ADD Xd, Xd, Ws, SXTW
    ADD_xxwEX(d, d, s, EX_SXTW, 0);
} else {
    ADD_www(d, d, s);  // normal 32-bit add
}
```

### Flag Handling in ADDX/SUBX
M68K ADDX/SUBX: Z flag only CLEARS (if result is non-zero), never SETS.
```c
if (needed_flags & FLAG_Z) {
    // Before operation: save old Z in scratch
    MOVN_wi(REG_WORK4, 0);              // REG_WORK4 = 0xFFFFFFFF
    MOVN_wi(REG_WORK3, 0);              // REG_WORK3 = 0xFFFFFFFF
    CSEL_xxxc(REG_WORK4, REG_WORK4, REG_WORK3, NATIVE_CC_NE);  // save old Z

    // ... perform ADDX/SUBX ...
    MRS_NZCV_x(REG_WORK1);

    // After: if new result is zero, keep old Z; if non-zero, clear Z
    UBFX_xxii(REG_WORK2, REG_WORK1, 30, 1);  // extract new Z bit
    CMP_wi(REG_WORK2, 1);                      // new Z set?
    SET_xxZflag(REG_WORK3, REG_WORK1);         // REG_WORK3 = flags with Z forced set
    CSEL_xxxc(REG_WORK1, REG_WORK3, REG_WORK1, NATIVE_CC_NE);  // keep old Z if result!=0
    AND_xxCflag(REG_WORK1, REG_WORK1, REG_WORK4);  // AND with saved old Z
} else {
    MRS_NZCV_x(REG_WORK1);  // just get flags, skip Z handling
}
```

## Page 0 DMA Guard (Debug Only)

Behind `#ifdef JIT_DEBUG_MEM_CORRUPTION` — NOT compiled in release builds.

### Mechanism
1. `jit_dbg_check_vec2_dispatch()`: Monitors Bus Error vector (M68K 0x008). When exec library replaces ROM handlers, arms `mprotect(PROT_READ)` on natmem page 0.
2. Write to page 0 → SIGSEGV → handler saves info, unprotects, inserts `BRK #0x7777` at PC+4
3. Faulting store re-executes, then BRK fires SIGTRAP
4. SIGTRAP handler: DMA write (PC outside JIT cache) → restore from shadow; CPU write (PC inside JIT cache) → update shadow. Re-protect page.

### DMA vs CPU Distinction
```c
int from_jit = (compiled_code != NULL &&
    wr_pc >= (uae_u64)(uintptr_t)compiled_code &&
    wr_pc < (uae_u64)(uintptr_t)current_compile_p);
```

## Bus Error Recovery

### Mechanism
```c
// In m68k_run_jit() — OUTSIDE inner loop:
jmp_buf bus_error_jmp;
if (setjmp(bus_error_jmp) != 0) {
    // longjmp landed here — bus error during JIT execution
    // Reset state, continue with interpreter
}

// In SIGBUS handler:
if (jit_in_compiled_code) {
    longjmp(bus_error_jmp, 1);
}
```

### Cost
- setjmp saves ~20 registers on ARM64 (X19-X28 callee-saved + D8-D15 FP callee-saved)
- ~144 bytes of register state per call
- ~2% MIPS cost even outside inner loop

## Natmem Gap Management

### commit_natmem_gaps() (amiberry_mem.cpp)
Called at end of `memory_reset()`. Walks natmem space, finds unmapped regions between banks, commits them via `uae_vm_commit()` with read-only permissions.

Key details:
- Uses `shmids[]` array (MAX_SHMID=256) to find committed regions
- Offsets relative to `natmem_reserved` (NOT `natmem_offset` — differs with RTG offset)
- Page-aligns gap boundaries to host page size
- Fill: `0x00` for `cs_unmapped_space` 0/1, `0xFF` for `cs_unmapped_space` 2
- On POSIX: skips `memset` when fill=0 (kernel zero-fills, avoids OOM on Android)
- Cross-platform: works on Linux, macOS, Windows

### SIGSEGV Quarantine (sigsegv_handler.cpp)
`arm64_quarantine_candidate`: When JIT code faults on a `dummy_bank` address (unmapped natmem), the handler quarantines that JIT block to prevent repeated faults.
