# Visual Corruption Investigation — ARM64 JIT

## Status: BLOCKED ON RUNTIME DIAGNOSTICS (as of 2026-02-12)

Static analysis is exhausted. Every individual piece of ARM64-specific codegen has been
verified correct through line-by-line comparison. Runtime diagnostic tools have been added
to isolate whether the bug is in compiled opcode handlers or JIT infrastructure.

## Symptoms
When JIT is enabled on ARM64 Linux, A1200 boots to Workbench but shows:
1. Window gadgets are black (should be blue/white Workbench 3.x style)
2. AmigaDOS window title text is missing
3. ROM version string during Kickstart splash is garbled ("ROM 2C.21..." instead of readable)

This is separate from the page 0 DMA crash (SOLVED via v43-clean).

## Key Paradox

Amiberry's 32-bit width fixes (276x BFI_xxii→BFI_wwii, 8x REV32_xx→REV_ww, 34x MOV_ww
cleanups, 32-bit LOAD_U32, etc.) should make it MORE correct than Lite's 64-bit operations
(defensively zeroing upper 32 bits of M68k values). Yet Amiberry shows corruption while
Lite doesn't. This means the root cause is NOT in the codegen width changes themselves.

## What Has Been Ruled Out (Exhaustive Comparison)

The ARM64 JIT was ported from Amiberry-Lite (where it works) to current Amiberry (where
it doesn't). An exhaustive line-by-line comparison found:

### 1. JIT Codegen Functions — VERIFIED CORRECT
- `compemu_midfunc_arm64.cpp` — all mid-level functions compared
  - Amiberry uses 32-bit ADD/SUB (ADD_www) vs Lite's 64-bit (ADD_xxx) — CORRECT
  - Range checks added for globals outside regs struct — CORRECT
  - fp_fscc_ri MOV_ww cleanup — CORRECT
  - flush_cpu_icache __builtin___clear_cache vs __clear_cache — EQUIVALENT
- `compemu_midfunc_arm64_2.cpp` — all handler functions compared
  - 276x BFI_xxii→BFI_wwii (32-bit byte/word insert) — CORRECT (zeroes upper bits defensively)
  - 8x REV32_xx→REV_ww (byte-swap) — EQUIVALENT (both correct when upper bits clean)
  - 34x MOV_ww(d,d) cleanup additions — CORRECT
  - All confirmed: 32-bit fixes are safe, no data corruption possible
- `codegen_arm64.cpp` — all raw functions identical (after NATMEM_OFFSET / countdown fixes)
  - LOAD_U64 — IDENTICAL (MOV_xi + MOVK_xish sequence)
  - LOAD_U32 — DIFFERENT but CORRECT: Amiberry uses MOV_wi (32-bit, zeroes upper bits),
    Lite uses MOV_xi (64-bit) — both produce correct 32-bit values
  - endblock functions (pc_inreg, pc_isconst) — CORRECT, branch offsets verified
  - compemu_raw_call, raw_dec_m, raw_maybe_recompile — all CORRECT
  - compemu_raw_set_pc_i — does NOT clobber flags, correct ordering
- `compemu_arm.cpp` — all compilation functions identical (54,000+ lines)

### 2. Memory Access Path — IDENTICAL
- `readbyte/readword/readlong` → `readmem_real` → `jnf_MEM_READ_OFF_*` — identical
- `writebyte/writeword/writelong` → `writemem_real` → `jnf_MEM_WRITE_OFF_*` — identical
- `readmem_special/writemem_special` → `jnf_MEM_READMEMBANK/WRITEMEMBANK` — identical
- Bank function lookup via `mem_banks` + address >> 16 — identical
- `SIZEOF_VOID_P * N` offsets for bank function pointers — correct
- ARM64 load/store instruction encodings all verified correct
- Byte-swap: REV_ww (32-bit), REV16_ww (16-bit), REV32_xx (64-bit) — all correct

### 3. Struct Offsets — CORRECT
- All offsets computed at compile time via `(uintptr)&regs.field - (uintptr)&regs`
- No hardcoded offsets anywhere
- Max offset ~768 bytes (well within ARM64 LDR/STR 16,380-byte limit)
- ARM64-specific struct extensions in newcpu.h handled by compile-time offset calculations

### 4. NATMEM_OFFSET Loading — FIXED
- Current Amiberry: global `natmem_offset` variable (not in regs struct)
- Lite: `regs.natmem_offset` field
- Fix: `compemu_raw_init_r_regstruct()` uses `LOAD_U64(&NATMEM_OFFSET) + LDR` (absolute address)

### 5. countdown/pissoff Access — FIXED
- Current Amiberry: global variable, not near regs
- Fix: All access via `LOAD_U64(&countdown) + LDR/STR` (absolute address)

### 6. regflags Access — CORRECT
- `raw_flags_to_reg()` uses `LOAD_U64(&regflags.nzcv) + STR_wXi` (absolute address)
- `live.state[FLAGTMP].mem = (uae_u32*)&(regflags.nzcv)` — correct pointer
- ARM64 flag_struct uses `nzcv` field (N=bit31, Z=bit30, C=bit29, V=bit28) — CORRECT

### 7. cpuemu_40.cpp Handlers — FUNCTIONALLY SAME
- Both use `get_byte_jit`/`put_byte_jit` etc. (accumulate `special_mem`)
- M68k emulation logic is bit-for-bit identical
- Only difference: return values (0 vs cycle counts) — ignored in execute_normal

### 8. compile_block() — THOROUGHLY COMPARED (16 differences found, all benign)
- Table selection: `nfcompfunctbl` vs `compfunctbl` based on `needed_flags` — same
- Failure path: `flush(1)` → `compemu_raw_call(cputbl[opcode])` — same
- optlev system differs: Amiberry has multi-level warmup (optcount = {4,0,0,0,0,0,-1,-1,-1,-1}),
  Lite immediately uses level 2 — BENIGN (affects when blocks get compiled, not HOW)
- Extra compemu_raw_mov_l_rm before maybe_do_nothing — redundant R0 load, HARMLESS
- NOFLAGS_SUPPORT_GENCOMP guard uses nfcpufunctbl in fallback — but nfcpufunctbl = handler_ff
  (same as cpufunctbl since NOFLAGS_SUPPORT_GENCPU is commented out) — NO EFFECT

### 9. build_comp() — STRUCTURAL FIXES APPLIED (NOT ROOT CAUSE)
- Added `cft_map()` to ALL opcode indices — but `cft_map()` is IDENTITY on ARM64
  (returns input unchanged because `HAVE_GET_WORD_UNSWAPPED` is not defined in sysconfig.h)
- Added inner j-loop for nfcpufunctbl population — correct structural fix, zero functional change
- Added NOFLAGS_SUPPORT_GENCOMP/GENCPU guards — correct but `NOFLAGS_SUPPORT_GENCPU` not defined
- **These changes make the code match x86 structurally but had ZERO functional effect**
- Lite's build_comp() also uses NO cft_map() at all — and works fine
- User confirmed: "The results seem the same (not fixed)"

### 10. Compilation Tables (compstbl_arm.cpp) — SEMANTICALLY IDENTICAL
- Same 1867 entries, same handler functions
- struct comptbl field order differs (opcode/specific swapped) but initializers match
- No missing or wrong opcode handlers

### 11. Special_mem Initialization — EQUIVALENT
- Both resolve to 0 in Direct mode

### 12. execute_normal() — DIFFERENT TIMING, SAME DATA PATH
- Current: `cpu_cycles = 4 * CYCLE_UNIT` (fixed), `adjust_cycles` commented out
- Lite: `cpu_cycles = (*cpufunctbl[opcode])()`, uses `adjust_cycles`
- Both correctly call `do_cycles` and accumulate `total_cycles` for compile_block
- **x86 JIT uses the same execute_normal and works** → NOT the cause

### 13. do_cycles Implementation — STRUCTURALLY DIFFERENT
- Current: `do_cycles` → `do_cycles_slow` → pissoff accumulation → `do_cycles_normal` or `do_cycles_cck`
- Lite: `do_cycles` is function pointer → `do_cycles_cpu_norm` or `do_cycles_cpu_fastest`
- `do_cycles_cck` calls `do_cck(true)` for chipset cycle-exact emulation (new in current)
- `custom_fastmode` can skip Copper/DMA generation (new in current)
- **x86 JIT uses the same do_cycles and works** → NOT the cause

### 14. Exception Handling Path — IDENTICAL
- `execute_exception` function identical
- `compemu_raw_handle_except` codegen identical
- `popall_execute_exception` registration identical

### 15. JIT Block Chaining — IDENTICAL
- `countdown` budget mechanism works the same
- `pissoff_value = 15000 * CYCLE_UNIT` in both
- Block exit via `popall_do_nothing` → `do_nothing()` → `do_cycles(0)` — same

### 16. JIT Cache Memory Setup — VERIFIED
- Separate vs contiguous allocations (popallspace/compiled_code) — implementation detail, both correct
- mmap flags — both MAP_PRIVATE | MAP_ANON
- popallspace I-cache flush lifecycle — correct
- mprotect RWX permissions — correct

### 17. USE_DATA_BUFFER — NOT DEFINED on ARM64
- Guard: `#if defined(CPU_arm) && !defined(ARMV6T2) && !defined(CPU_AARCH64)`
- FALSE on ARM64 in BOTH codebases — no difference

### 18. emit_quad vs emit_longlong — FUNCTIONALLY IDENTICAL
- Just naming differences, same implementation

### 19-33. Additional Areas Verified
- cft_map / DO_GET_OPCODE, MOVEM implementation, MOVE.B compilation, I-cache flush,
  register allocator, always_used/call_saved arrays, REG_PAR1/REG_PAR2,
  REGPARAM2/JITCALL, canbang/jit_direct_compatible_memory, specmem population flow,
  popall entry/exit stubs, check_for_cache_miss, SIGSEGV handler (ARM64 PC+4 skip vs
  x86 trampoline — functionally equivalent), COMP_DEBUG assertions — ALL CORRECT

## Known Differences (Not Proven Causative)

### 1. COMP_DEBUG = 1 (Amiberry) vs 0 (Lite)
- Enables `Dif(x)` debug assertions
- **Impact**: Performance only, no data change. Could cause jit_abort() on subtle issues,
  but jit_abort() → uae_reset(1,0) (full reset), NOT visual corruption.

### 2. Chipset Emulation Core — NEWER
- `do_cycles_cck` → `do_cck(true)` — cycle-exact custom chip emulation
- `custom_fastmode` — skips Copper/DMA generation when screen lines repeat
- More sophisticated event system (`ev_sync`, two-tier dispatch)
- These are standard WinUAE updates, not ARM64-JIT-specific
- **x86 JIT uses the same chipset code and works** → NOT ARM64-specific

### 3. Cycle Counting in execute_normal — DIFFERENT
- Current: fixed 4*CYCLE_UNIT per instruction
- Lite: variable per-instruction cycle counts + adjust_cycles
- **Impact**: Changes timing of when events fire during interpreter pass (before compilation)
- **x86 JIT uses same timing and works** → NOT the cause

## All Fix Attempts Applied (all INEFFECTIVE for visual corruption)
1. `MOV_ww(d, d)` in `jff_ASR_l_imm` (compemu_midfunc_arm64_2.cpp) — defensive hardening
2. 8x `REV32_xx` → `REV_ww` (compemu_midfunc_arm64_2.cpp) — defensive hardening
3. `cft_map()` + j-loop + NOFLAGS guards in `build_comp()` (compemu_support_arm.cpp) — structural fix (no-op)
- All are correct fixes for code quality but none resolved the visual corruption.
- User confirmed each time: "The results seem the same (not fixed)"

## Red Herrings Investigated
1. **cft_map() in build_comp()**: Identity function on ARM64, zero effect (see above)
2. **ROM overlay NATMEM mismatch**: x86 JIT has same behavior, works fine. Fix REVERTED.
3. **SIGSEGV at 0xF00000**: By design — fault handler falls back to bank-based access.
4. **SIGSEGV handler differences** (x86 trampoline vs ARM64 PC+4 skip): Functionally equivalent.
5. **Extra REG_PAR2 in compile_block() failure path**: Harmless — R1 is caller-saved, cpuop_func ignores it.
6. **get_target() vs get_target_noopt()**: Identical implementation.
7. **flush_reg_count() missing**: No-op (only active with RECORD_REGISTER_USAGE).
8. **cpuemu_40.cpp return values**: Timing only, not data — Lite returns cycles, Amiberry returns 0.
9. **USE_DATA_BUFFER on ARM64**: NOT defined in either codebase — initial agent analysis was wrong.
10. **struct regstruct layout differences**: Handled by compile-time offset calculations, not a bug.
11. **BFI_wwii vs BFI_xxii (276 instances)**: 32-bit BFI is CORRECT for M68k values; register allocator
    uses 32-bit STR_wXi/LDR_wXi consistently, so zeroing upper bits is safe.
12. **LOAD_U32 MOV_wi vs MOV_xi**: Amiberry's 32-bit version is correct (produces same 32-bit value).
13. **optlev warmup differences**: Affects WHEN blocks compile, not HOW. Not corruption-causing.

## Diagnostic Tools Available

### JIT_DEBUG_VISUAL (compemu_support_arm.cpp)
Behind `#ifdef JIT_DEBUG_VISUAL` in sysconfig.h (commented out by default):
- `jit_diag_verify_natmem()` — One-time: verifies natmem_offset, compares direct vs bank reads
- `jit_diag_log_tables()` — One-time: counts compiler table coverage
- `jit_diag_log_block()` — Logs first 20 blocks (PC, opcodes, specmem)
- `jit_diag_log_compile_decision()` — Logs each instruction's compile-vs-interpret decision
- `jit_diag_verify_specmem()` — Checks bank jit_read_flag consistency
How to enable: Uncomment `#define JIT_DEBUG_VISUAL` in sysconfig.h

### JIT_INTERPRET_ONLY (compemu_support_arm.cpp) — NEW
Forces ALL opcodes through the interpreter fallback path within compiled blocks.
The block structure (entry/exit stubs, countdown, block chaining) is still active —
only the individual opcode compilation is bypassed.

**Purpose**: Determine if the bug is in a compiled opcode handler (corruption disappears)
or in JIT infrastructure (corruption persists).

How to enable: Uncomment `#define JIT_INTERPRET_ONLY` in sysconfig.h

### JIT_BISECT_OPCODES (compemu_support_arm.cpp) — NEW
Forces specific opcode ranges through the interpreter for binary search.
Opcodes in the range [JIT_BISECT_MIN, JIT_BISECT_MAX] are forced interpreted;
all other opcodes are compiled normally.

**Purpose**: After JIT_INTERPRET_ONLY confirms the bug is in compiled handlers,
use this to binary-search which opcode class causes corruption.

**M68k opcode ranges for bisection**:
- `0x0000-0x0FFF`: Bit manipulation, immediate ops (ORI, ANDI, EORI, BTST, BCHG, BCLR, BSET)
- `0x1000-0x1FFF`: MOVE.B
- `0x2000-0x2FFF`: MOVE.L
- `0x3000-0x3FFF`: MOVE.W
- `0x4000-0x4FFF`: Miscellaneous (CLR, NEG, NOT, TST, LEA, PEA, MOVEM, JSR, JMP)
- `0x5000-0x5FFF`: ADDQ, SUBQ, Scc, DBcc
- `0x6000-0x6FFF`: Bcc, BSR, BRA
- `0x7000-0x7FFF`: MOVEQ
- `0x8000-0x8FFF`: OR, DIV, SBCD
- `0x9000-0x9FFF`: SUB, SUBX, SUBA
- `0xA000-0xAFFF`: A-line (unimplemented — Line-A traps)
- `0xB000-0xBFFF`: CMP, EOR, CMPM, CMPA
- `0xC000-0xCFFF`: AND, MUL, ABCD, EXG
- `0xD000-0xDFFF`: ADD, ADDX, ADDA
- `0xE000-0xEFFF`: Shift/Rotate (ASL, ASR, LSL, LSR, ROL, ROR, ROXL, ROXR)
- `0xF000-0xFFFF`: F-line (coprocessor — FPU)

How to enable: Uncomment `#define JIT_BISECT_OPCODES` and set MIN/MAX in sysconfig.h

## DISPROVEN: cft_map() in build_comp() (2026-02-11)

### What happened
A detailed comparison of `build_comp()` between ARM64 and x86 revealed structural differences:
missing `cft_map()` calls, missing inner j-loop, missing NOFLAGS guards. These were all fixed
to match x86 exactly.

### Why it was NOT the root cause
`cft_map()` on ARM64 is defined as an **identity function** — it returns the input unchanged.
This is because `HAVE_GET_WORD_UNSWAPPED` is commented out in `src/osdep/sysconfig.h`.
Therefore adding `cft_map()` calls had ZERO functional effect. Amiberry-Lite's build_comp()
also uses NO cft_map() at all, and it works fine.

The structural improvements (j-loop, NOFLAGS guards) are correct for code quality but
`NOFLAGS_SUPPORT_GENCPU` is also not defined, so the guards resolve to the same code path.

### Files Modified (kept for code quality)
- `src/jit/arm/compemu_support_arm.cpp`: `build_comp()` now matches x86 structurally.

## DISPROVEN: BFI_wwii 32-bit width changes (2026-02-12)

### What happened
Exhaustive comparison of compemu_midfunc_arm64_2.cpp found 276 instances where BFI_xxii
(64-bit) was changed to BFI_wwii (32-bit) in Amiberry. Initial concern: if the register
allocator depends on upper 32 bits being preserved, BFI_wwii (which zeroes them) could
cause corruption.

### Why it is NOT the root cause
The register allocator uses 32-bit STR_wXi/LDR_wXi for all M68k register spills and
fills. All M68k values are 32-bit. The upper 32 bits of ARM64 registers holding M68k
values are never meaningful. BFI_wwii correctly zeroes them as a defensive measure.
This change makes Amiberry MORE correct, not less.

**The paradox**: Lite's BFI_xxii preserves garbage in upper bits, yet works. Amiberry's
BFI_wwii cleans upper bits, yet shows corruption. This proves the bug is elsewhere.

## Next Steps: Runtime Diagnostics

### Step 1: Test JIT_INTERPRET_ONLY
1. Uncomment `#define JIT_INTERPRET_ONLY` in `src/osdep/sysconfig.h`
2. Build and run on Linux VM
3. Boot A1200 with JIT enabled
4. **If corruption DISAPPEARS**: Bug is in a compiled opcode handler → proceed to Step 2
5. **If corruption PERSISTS**: Bug is in JIT infrastructure (block management, popall stubs,
   event handling, or interaction with chipset code) → investigate those paths

### Step 2: Binary Search with JIT_BISECT_OPCODES (if Step 1 shows compiled handler bug)
1. Comment out JIT_INTERPRET_ONLY
2. Uncomment JIT_BISECT_OPCODES with initial range 0x0000-0x7FFF
3. If corruption disappears → bug is in 0x0000-0x7FFF, narrow further
4. If corruption persists → bug is in 0x8000-0xFFFF, narrow further
5. Continue halving the range until the guilty opcode class is identified

### Step 3: Individual Opcode Analysis (after bisection)
Once the opcode class is identified, compare the specific handler function in
compemu_midfunc_arm64_2.cpp against Lite's version instruction by instruction.

### Alternative: If JIT_INTERPRET_ONLY still shows corruption
The bug would be in JIT infrastructure that is common to both compiled and interpreted
paths within a JIT block:
- popall entry/exit stubs (register save/restore)
- Block chaining and countdown mechanism
- endblock codegen (pc_inreg, pc_isconst)
- Interaction between JIT-generated code and chipset event handling
- SIGSEGV handler behavior during compiled code execution
- I-cache coherency issues on the specific ARM64 platform

## Files Modified This Investigation
- `src/osdep/sysconfig.h` — Added JIT_INTERPRET_ONLY, JIT_BISECT_OPCODES defines
- `src/jit/arm/compemu_support_arm.cpp` — Added diagnostic guards in compile_block,
  build_comp structural fixes, JIT_DEBUG_VISUAL diagnostic functions
- `src/jit/arm/compemu_midfunc_arm64_2.cpp` — MOV_ww in jff_ASR_l_imm, REV32_xx→REV_ww
