# Visual Corruption Investigation — ARM64 JIT

## Status: UNSOLVED (as of 2026-02-11)

## Symptoms
When JIT is enabled on ARM64 Linux, A1200 boots to Workbench but shows:
1. Window gadgets are black (should be blue/white Workbench 3.x style)
2. AmigaDOS window title text is missing
3. ROM version string during Kickstart splash is garbled ("ROM 2C.21..." instead of readable)

This is separate from the page 0 DMA crash (SOLVED via v43-clean).

## What Has Been Ruled Out (Exhaustive Comparison)

The ARM64 JIT was ported from Amiberry-Lite (where it works) to current Amiberry (where it doesn't). An exhaustive line-by-line comparison found:

### 1. JIT Codegen Functions — IDENTICAL
- `compemu_midfunc_arm64.cpp` — all mid-level functions identical
- `compemu_midfunc_arm64_2.cpp` — all mid-level functions identical
- `codegen_arm64.cpp` — all raw functions identical (after NATMEM_OFFSET / countdown fixes)
- `compemu_arm.cpp` — all compilation functions identical (54,000+ lines)

### 2. Memory Access Path — IDENTICAL
- `readbyte/readword/readlong` → `readmem_real` → `jnf_MEM_READ_OFF_*` — identical
- `writebyte/writeword/writelong` → `writemem_real` → `jnf_MEM_WRITE_OFF_*` — identical
- `readmem_special/writemem_special` → `jnf_MEM_READMEMBANK/WRITEMEMBANK` — identical
- Bank function lookup via `mem_banks` + address >> 16 — identical
- `SIZEOF_VOID_P * N` offsets for bank function pointers — correct

### 3. Struct Offsets — CORRECT
- All offsets computed at compile time via `(uintptr)&regs.field - (uintptr)&regs`
- No hardcoded offsets anywhere
- Max offset ~768 bytes (well within ARM64 LDR/STR 16,380-byte limit)
- Boundary checking at 32,760 bytes with absolute-address fallback exists

### 4. NATMEM_OFFSET Loading — FIXED
- Current Amiberry: global `natmem_offset` variable (not in regs struct)
- Lite: `regs.natmem_offset` field
- Fix: `compemu_raw_init_r_regstruct()` uses `LOAD_U64(&NATMEM_OFFSET) + LDR` (absolute address)

### 5. countdown/pissoff Access — FIXED
- Current Amiberry: global variable, not near regs
- Fix: All access via `LOAD_U64(&countdown) + LDR/STR` (absolute address)

### 6. regflags Access — CORRECT
- Current Amiberry: `regflags` is global (`extern struct flag_struct regflags __asm__("regflags")`)
- Lite: `regs.ccrflags` is in regstruct
- `raw_flags_to_reg()` correctly uses `LOAD_U64(&regflags.nzcv) + STR_wXi` (absolute address)
- `live.state[FLAGTMP].mem = (uae_u32*)&(regflags.nzcv)` — correct pointer

### 7. cpuemu_40.cpp Handlers — FUNCTIONALLY SAME
- Both use `get_byte_jit`/`put_byte_jit` etc. (accumulate `special_mem`)
- `_40_ff` handlers correctly in `op_smalltbl_43` used by JIT
- `_46_ff`/`_49_ff` handlers NOT used by JIT (different tables)

### 8. compile_block() — FUNCTIONALLY EQUIVALENT
- Table selection: `nfcompfunctbl` vs `compfunctbl` based on `needed_flags` — same
- Failure path: `flush(1)` → `compemu_raw_call(cputbl[opcode])` — same
- `specmem` save/restore — same

### 9. build_comp() — MINOR BUGS, NOT CAUSATIVE
- ARM version missing inner loop for nfcpufunctbl matching (line 2947-2962)
- Uses uninitialized `j` variable (line 2953) — only affects logging
- nfcpufunctbl IS correctly populated by blanket loop at lines 2964-2966
- Line 3007 bug (`tbl[i].opcode` instead of `nfctbl[i].opcode`) — same bug exists in x86

### 10. Special_mem Initialization — EQUIVALENT
- Current: `special_mem = special_mem_default` where `special_mem_default = 0` for Direct mode
- Lite: `special_mem = DISTRUST_CONSISTENT_MEM` where `DISTRUST_CONSISTENT_MEM = 0`
- Both resolve to 0

### 11. get_jit_opcode() — EQUIVALENT
- Current: complex conditional, but resolves to `get_diword(0)` when JIT Direct + !cpu_compatible
- Lite: directly calls `get_diword(0)`

### 12. addrbank Layout — IDENTICAL
- Same struct field order in both codebases
- `jit_read_flag`/`jit_write_flag` at same position
- `custom_bank` correctly sets `S_READ`/`S_WRITE`

## Known Differences (Not Yet Proven Causative)

### 1. cpuemu_40.cpp Returns 0 Instead of Cycle Counts
- Current Amiberry: `using_nocycles = 1` in gencpu → all handlers `return 0;`
- Lite: handlers return actual cycles (e.g., `return 8 * CYCLE_UNIT / 2;`)
- `execute_normal()` hardcodes `cpu_cycles = 4 * CYCLE_UNIT`
- `adjust_cycles()` call is COMMENTED OUT
- **Impact**: Affects JIT timing/recompilation frequency, NOT data correctness

### 2. comptbl Struct Field Order
- Current Amiberry: `{ handler, opcode, specific }`
- Lite: `{ handler, specific, opcode }`
- **Impact**: None — code accesses by field NAME, not position

### 3. COMP_DEBUG Changed from 0 to 1 in compemu_arm.h
- Enables `Dif(x)` macros for debug assertions
- **Impact**: Performance only, no data change

## Previous Fix Attempts (INEFFECTIVE)
1. Added `MOV_ww(d, d)` in `jff_ASR_l_imm` — no improvement
2. Changed 8x `REV32_xx` → `REV_ww` — no improvement
3. These were applied based on theoretical analysis but corruption persists

## Likely Root Cause Candidates

Given that ALL code is functionally identical, the issue is likely:

1. **Something in the newer WinUAE emulator core** that interacts differently with JIT-compiled code — possibly memory bank initialization, chipset register timing, or event system differences

2. **A subtle difference in how the newer cpuemu_40.cpp handlers interact with the JIT** — the `real_opcode = opcode` pattern is new (Lite just uses `opcode` directly) but shouldn't matter

3. **Thread safety or memory ordering** — current Amiberry uses atomics for thread state; Lite uses volatile. If the JIT is accessing shared state without proper barriers, data could be stale

4. **Cache coherency on ARM64** — JIT writes native code and reads it back. I-cache flush is critical. The `flush_cpu_icache` call in `create_popalls` was added, but there may be other paths where I-cache is stale

## Diagnostic Approach (Next Steps)

Since code comparison has been exhausted, runtime diagnostics are needed:
1. Add logging in `jnf_MEM_READ_OFF_*` to capture what the JIT reads from specific Amiga addresses (e.g., custom chip area, font data)
2. Compare with what the interpreter reads from the same addresses
3. Check if `R_MEMSTART` (register 27) holds the correct value throughout block execution
4. Verify `special_mem` is being correctly accumulated during block building for chip register accesses
