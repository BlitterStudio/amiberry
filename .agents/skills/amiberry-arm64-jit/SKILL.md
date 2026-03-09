---
name: amiberry-arm64-jit
description: >
  Complete ARM64 JIT compiler knowledge for Amiberry (GitHub issue #1766). Captures architecture,
  12 root-cause bug fixes, performance optimizations, and testing methodology from 30+ debugging
  sessions. Key fixes: (a) inter-block flag optimization (flush_flags before dont_care_flags +
  two-pass liveflags), (b) FBcc 64-bit branch target truncation, (c) BSR.L/Bcc.L sign extension,
  (d) ADDX/SUBX Z-flag M68K semantics, (e) ADDX byte/word register initialization (MOVN_xi dual-purpose),
  (g) bus error recovery via setjmp/longjmp. Performance: R_REGSTRUCT range-based offset,
  ADDX/SUBX Z-flag skip, guard infrastructure removal. Use this skill for ANY ARM64 JIT work:
  crashes, stalls, incorrect behavior, performance, codegen, inter-block flags, natmem/memory,
  FPU JIT, or the page 0 DMA guard.
  MANDATORY TRIGGERS: ARM64 JIT, JIT crash, JIT stall, JIT performance, MIPS, SysInfo,
  compile_block, dont_care_flags, flush_flags, inter-block, liveflags, FBcc, comp_fbcc_opp,
  BSR, Bcc, sign extension, comp_pc_p, set_const, PC_P, 64-bit pointer, truncation,
  ADDX, SUBX, Z flag, FLAG_Z, natmem gap, commit_natmem_gaps, PROT_NONE, canbang,
  bus error, setjmp, SIGBUS, R_REGSTRUCT, regflags, codegen_arm64, compemu_support_arm,
  compemu_fpp_arm, compemu_midfunc_arm64, page 0, DMA guard, JIT_DEBUG_MEM_CORRUPTION,
  SIGSEGV handler, gencomp_arm, Lightwave, Workbench boot, block chaining.
---

# Amiberry ARM64 JIT — Complete Knowledge Base

## Current Status (2026-03)

**All known bugs: FIXED.** All fixes are root-cause, not guards/bandaids.

| Test Config | Result |
|---|---|
| Lightwave-NoRAM.uae (no Z3 RAM, A1200+68040+JIT) | ✅ Boots to Workbench |
| Lightwave.uae (256MB Z3, boots into LightWave 3D app) | ✅ App runs stable |
| SysInfo.uae (MIPS benchmark) | ✅ ~7391 MIPS (baseline ~7800) |
| A4000.uae (68060/AGA, red wallpaper) | ✅ Clean wallpaper, no corruption |
The ~5.7% MIPS gap is from bus error recovery (setjmp ~2%) and accumulated compile-time safety checks. Accepted tradeoff.

## Architecture Quick Reference

### Virtual Registers
| Register | Host | Purpose | Width |
|---|---|---|---|
| PC_P | — | Virtual reg 16 — host pointer to current M68K PC in natmem | **64-bit** (ONLY 64-bit virt reg) |
| FLAGX | — | Virtual reg 17 — M68K X flag | 32-bit (bit 0) |
| FLAGTMP | — | Virtual reg 18 — scratch for flag operations | 32-bit |
| R_REGSTRUCT | X28 | `&regs` — regstruct base pointer | 64-bit (host ptr) |
| R_MEMSTART | X27 | `natmem_offset` — Amiga memory base | 64-bit (host ptr) |

**Critical rule**: PC_P is the ONLY virtual register holding a 64-bit host pointer. All other virtual regs hold 32-bit M68K values. Any code that puts a 64-bit value into a non-PC_P register MUST be treated as a bug.

### Flag System
- ARM64 NZCV maps to M68K flags: N=bit31, Z=bit30, C=bit29, V=bit28
- `regflags` is a **separate global** from `regs` (NOT inside regstruct)
- `regflags.nzcv` — NZCV flags stored as ARM64 format
- `regflags.x` — X flag stored at **bit 0** (not bit 29)
- `FLAG_Z = 0x0004`, `FLAG_C = 0x0002`, `FLAG_N = 0x0008`, `FLAG_V = 0x0001`
- `FLAG_CZNV = FLAG_C|FLAG_Z|FLAG_N|FLAG_V`, `FLAG_ALL = FLAG_CZNV|FLAG_X`
- `needed_flags` — per-instruction bitmask of which flags successor instructions need
- `liveflags[]` — array tracking live flag state through a block (backward propagation)

### Memory Layout
- `natmem_reserved` — base of reserved VM (up to 4GB on ARM64)
- `natmem_offset` — base for Amiga address space (may differ from `natmem_reserved` with RTG offset)
- Direct access: `natmem_offset + M68K_addr` → host pointer
- `canbang` — true when JIT direct memory access is active
- Gaps between memory banks filled by `commit_natmem_gaps()` (read-only pages)

### Key Offset Facts
- `STR_wXi` max offset: 16380 bytes (12-bit field × 4 scaling)
- `STR_xXi` max offset: 32760 bytes (12-bit field × 8 scaling)
- `LOAD_U64` emits 1–4 ARM64 instructions (MOVZ + up to 3 MOVK)
- Offset from `regs` to `regflags` depends on linker but is typically within 16KB
- R_REGSTRUCT-relative stores use fast 1-instruction `STR_wXi` when offset ≤ 16380 and 4-aligned

See [references/architecture.md](references/architecture.md) for deep technical details.

## All Fixes (12 Root Causes)

### 1. Inter-block `dont_care_flags()` — Lightwave-NoRAM crash
**Root cause**: `compile_block()` calls `dont_care_flags()` at block boundaries, which sets `live.flags_are_important = 0`. This causes the subsequent `flush(1)` → `flags_to_stack()` to skip saving NZCV to memory. When the slow path fires (countdown < 0 → interpreter), the interpreter reads stale flags → wrong conditional branches → crash.

**Fix** (compemu_support_arm.cpp): Call `flush_flags()` BEFORE `dont_care_flags()` in the inter-block optimization. This saves NZCV to memory for the slow path, then marks flags unimportant so the fast path (successor block) skips redundant flag loads.

**Note**: The x86 JIT has this same optimization as `#if 0` disabled — same bug was never fixed upstream in WinUAE.

### 2. Two-pass liveflags optimization
**Enhancement to fix #1**: Before `remove_deps(bi)` in `compile_block()`, save successor blocks' `needed_flags` from deps. Use `successor_flags` instead of `FLAG_ALL` as `liveflags[blocklen]` starting point. This gives the backward liveflags propagation real information about what the successor actually needs, producing better flag elimination.

### 3. FBcc branch target truncation — Lightwave app crash
**Root cause**: In `compemu_fpp_arm.cpp` `comp_fbcc_opp()`, the FPU conditional branch handler computed a 64-bit host target address and passed it through `set_const()` for a non-PC_P register. Since `set_const()` masks non-PC_P values to 32 bits, the upper 32 bits were lost → branch to wrong address → crash.

**Fix**: Split into displacement + `arm_ADD_l_ri()` which properly handles 64-bit for PC_P-derived values.

### 4. BSR.L/Bcc.L sign extension — backward branch crash
**Root cause**: 32-bit displacement from `comp_get_ilong()` (returns `uae_u32`) zero-extends to 64-bit. Backward branches like -4096 (`0xFFFFF000`) become `0x00000000FFFFF000` instead of `0xFFFFFFFFFFFFF000`. Adding to PC_P corrupts `comp_pc_p`.

**Fix**: Cast to `(uae_s32)` in 32 handlers (BSR.L + BRA.L + 14 Bcc.L × `_ff`/`_nf` variants) + generator.

### 5. ADDX/SUBX Z-flag semantics
**Root cause**: M68K ADDX/SUBX Z flag only CLEARS, never SETS. The JIT was unconditionally setting Z from the result.

**Fix**: Extra instructions (UBFX/CMP/SET_xxZflag/CSEL/AND) after MRS in `jff_ADDX_b/w/l` and `jff_SUBX_b/w/l`. Wrapped in `if (needed_flags & FLAG_Z)` for performance.

### 5b. ADDX byte/word MOVN_xi dual-purpose initialization — A4000 wallpaper corruption
**Root cause**: In `jff_ADDX_b` and `jff_ADDX_w`, `MOVN_xi(REG_WORK1, 0)` (set all-ones) was inside `if (needed_flags & FLAG_Z)`. But REG_WORK1 serves TWO purposes: (1) Z-flag sticky CSEL base value, AND (2) padding for the byte/word BFI + ADCS carry chain. When Z isn't needed, MOVN is skipped, leaving garbage in bits 0-23 (byte) or 0-15 (word) of REG_WORK1. The subsequent `BFI_xxii` only sets the upper bits. `ADCS_www` then computes carry from garbage padding → wrong carry flag → visual corruption.

**Fix**: Move `MOVN_xi(REG_WORK1, 0)` OUTSIDE the `if (needed_flags & FLAG_Z)` block. Keep MOVN_xish and CSEL inside the guard.

**Not affected**: SUBX_b/w (uses LSL which fully initializes both operands), ADDX_l/SUBX_l (full 32-bit ops, no byte packing).

**Symptom**: Vertical teal/cyan lines on the A4000 AGA red gradient wallpaper. Only visible with JIT + two-pass liveflags (which enables the FLAG_Z skip path).

### 6. ARM64 sign-extension in `arm_ADD_l` / `arm_ADD_l_ri`
**Root cause**: 32-bit M68K displacements added to 64-bit PC_P host pointers without sign extension.

**Fix**: Both functions now sign-extend 32-bit values when the destination is PC_P, using `ADD_xxwEX(d, d, s, EX_SXTW, 0)` or splitting large immediates.

### 7. `set_const` 32-bit masking
**Fix**: Non-PC_P registers masked to 32 bits in `set_const()`. PC_P explicitly excluded to preserve 64-bit host pointers.

### 8. 64-bit pc_p store/load paths
**Fix**: All code paths that store/load PC_P use 64-bit operations (`STR_xXi`/`LDR_xXi` or `LOAD_U64`).

### 9. MOVE16 64-bit add
**Fix**: `ADD_xxx` instead of `ADD_www` in MOVE16 handler (needs 64-bit for natmem pointer arithmetic).

### 10. Bus error recovery
**Fix**: `setjmp`/`longjmp` in `m68k_run_jit()`, `jit_in_compiled_code` flag, SIGBUS handler integration. The `setjmp` is placed OUTSIDE the inner loop (performance: ~2% cost). Handles bus errors from JIT-compiled code accessing unmapped memory.

### 11. VM/memory fixes
- `natmem_reserved_size` → `size_t` (was `uae_u32`, overflowed at 4GB)
- ARM64 gets 4GB natmem reservation (was 2GB)
- `UAE_VM_32BIT` dropped (JIT is fully 64-bit clean)
- `commit_natmem_gaps()` fills unmapped regions with read-only pages

See [references/fixes-and-optimizations.md](references/fixes-and-optimizations.md) for code-level details.

## Performance Knowledge

### Benchmark: SysInfo MIPS (A1200/68040/JIT, macOS ARM64)
| State | MIPS | Delta |
|---|---|---|
| Before all changes (baseline) | ~7800 | — |
| After all fixes, before optimization | ~6950 | -10.9% |
| + diagnostic code cleanup | ~6950 | 0% |
| + guard infrastructure removal | ~7193 | +3.5% |
| + R_REGSTRUCT range + ADDX/SUBX Z-skip | ~7356 | +2.3% |
| **Final** | **~7356** | **-5.7%** |

### Cost Breakdown of Remaining Gap
- **setjmp bus error recovery**: ~2% (~156 MIPS) — fixed cost, can't remove without losing crash protection
- **Compile-time overhead**: `set_const` masking, sign-extension checks — runs during block compilation only
- **I-cache effects**: Slightly different generated code sizes

### Performance Optimizations Applied
1. **Guard infrastructure removal** (+240 MIPS): Removed dynamic unstable bitmap, guard stats counters, per-block guard checks from `compile_block()`
2. **R_REGSTRUCT range-based offset** (+100 MIPS): Changed `compemu_raw_mov_l_mr/mi/rm` from strict `sizeof(struct regstruct)` bounds to `idx <= 16380 && (idx & 3) == 0`. Lets `regflags` use fast 1-instruction `STR_wXi` instead of 4-5 instruction `LOAD_U64 + STR`. Flag stores happen at every block exit.
3. **ADDX/SUBX Z-flag skip** (+63 MIPS): Wrapped Z-flag instructions in `if (needed_flags & FLAG_Z)`. Saves 4-8 instructions per ADDX/SUBX when Z not needed.
4. **setjmp outside inner loop** (+2400 MIPS from initial): Moved setjmp from per-block to per-dispatch-call.

### Performance Anti-patterns
- `LOAD_U64` for frequently-accessed globals → use R_REGSTRUCT-relative when offset allows
- Unconditional flag handling → check `needed_flags` first
- Guards/bitmaps in hot compilation path → remove or make branch-free
- setjmp in inner loop → move to outer scope

### SysInfo Benchmark Variance
~3-5% between runs depending on system load/thermals. Always use multiple measurements.

## Testing Methodology

### Test Configs (in `~/Library/Application Support/Amiberry/Configurations/`)
| Config | Purpose | Expected |
|---|---|---|
| Lightwave-NoRAM.uae | No Z3 RAM, A1200+68040+JIT | Boots to Workbench in 1-2s |
| Lightwave.uae | 256MB Z3, auto-boots Lightwave app | LightWave 3D 5.0 loads, runs stable |
| SysInfo.uae | MIPS benchmarking | Boot → click SPEED button → read MIPS |
| A4000.uae | 68060/AGA regression test | Clean red gradient wallpaper, no vertical lines |

### Build & Deploy (macOS)
```bash
# Touch files (ninja doesn't always detect edits)
touch src/jit/arm/compemu_support_arm.cpp src/jit/arm/codegen_arm64.cpp

# Build
cmake --build cmake-build-release --target Amiberry -j$(sysctl -n hw.ncpu)

# Deploy
cp -f cmake-build-release/Amiberry.app/Contents/MacOS/Amiberry \
  /Applications/Amiberry.app/Contents/MacOS/Amiberry
codesign --force --deep --sign - /Applications/Amiberry.app/Contents/MacOS/Amiberry
```

### SysInfo SPEED Button Procedure (via Amiberry MCP)
1. Launch with SysInfo.uae config, wait ~7s for boot
2. Park mouse top-left: `dx=-700, dy=-600`
3. Navigate to SPEED button area: `dx=395, dy=485` → `dx=0, dy=400` → `dx=150, dy=120` → `dx=45, dy=20`
4. Click: `buttons=1` then `buttons=0`
5. Wait ~14s for benchmark
6. Screenshot to read results

### Key Regression Tests
After ANY JIT change, test ALL FOUR configs. Lightwave-NoRAM tests inter-block flags. Lightwave.uae tests FBcc/FPU paths and Z3 memory. SysInfo tests overall performance. A4000.uae tests ADDX/SUBX carry correctness (AGA rendering triggers byte-width ADDX with Z not needed).

## Key File Map

| File | What's There |
|---|---|
| `src/jit/arm/compemu_support_arm.cpp` | **Main JIT file**: compile_block, inter-block fix, two-pass liveflags, set_const masking, flush_flags, dont_care_flags, page 0 DMA guard |
| `src/jit/arm/codegen_arm64.cpp` | ARM64 instruction emitters, R_REGSTRUCT range optimization, popall stubs, endblock code, 64-bit pc_p store/load |
| `src/jit/arm/compemu_midfunc_arm64.cpp` | Mid-level JIT ops: dont_care_flags(), arm_ADD_l/arm_ADD_l_ri sign-extension, set_const masking, arm_ADD_l_ri8/arm_SUB_l_ri8 PC_P checks |
| `src/jit/arm/compemu_midfunc_arm64_2.cpp` | Handler functions: ADDX/SUBX Z-flag fix + skip optimization, MOVE16 64-bit add |
| `src/jit/arm/compemu_fpp_arm.cpp` | FPU JIT: FBcc branch target truncation fix in comp_fbcc_opp() |
| `src/jit/arm/compemu_arm.h` | Constants: PC_P=16, FLAGX=17, FLAGTMP=18, FLAG_Z=0x0004, FLAG_ALL |
| `src/jit/arm/codegen_arm64.h` | ARM64 encoding macros: STR_wXi, LOAD_U64, SET_xxZflag, etc. |
| `src/newcpu.cpp` | JIT dispatch loop, setjmp/bus error recovery, jit_in_compiled_code flag |
| `src/osdep/sigsegv_handler.cpp` | SIGBUS/longjmp recovery, natmem gap quarantine (arm64_quarantine_candidate) |
| `src/osdep/amiberry_mem.cpp` | commit_natmem_gaps() for read-only gap pages |
| `src/memory.cpp` | memory_reset() calls commit_natmem_gaps() |
| `src/machdep/m68k.h` | FLAGBIT_X=0, SET_XFLG/GET_XFLG use bit 0, regflags struct |
| `src/jit/compemu.h` | extern declarations (jit_in_compiled_code) |
| `src/vm.cpp` | VM abstraction (reserve/commit/decommit), size_t fixes |

## Key Patterns & Gotchas

1. **Any new `set_const()` call with a host pointer on non-PC_P register → BUG.** Must use displacement + `arm_ADD_l_ri()` pattern instead.
2. **`dont_care_flags()` must always be preceded by `flush_flags()`** when flags might be in NZCV but not yet spilled to memory.
3. **M68K branch displacements are SIGNED** — always cast `comp_get_ilong()` result to `(uae_s32)` before use in address arithmetic.
4. **ADDX/SUBX Z flag only clears, never sets** — different from normal ADD/SUB. ADDX_b/w `MOVN_xi(REG_WORK1, 0)` MUST be unconditional (serves dual purpose: Z-flag base AND byte-packing padding).
5. **`regflags` is NOT inside `regs`** — codegen offset checks must account for nearby globals, not just `sizeof(struct regstruct)`.
6. **The x86 JIT inter-block optimization is also broken** (`#if 0` in compemu_support_x86.cpp) — same bug, never fixed in WinUAE.
7. **setjmp on ARM64 saves ~20 registers** (X19-X28 + D8-D15, ~144 bytes) — ~2% cost even outside inner loop.
8. **`flush_flags()` at inter-block is mostly first-compilation cost** — two-pass liveflags makes subsequent passes set `flags_are_important = 0` via per-instruction `dont_care_flags()`, so the inter-block `flush_flags()` becomes a no-op.
9. **ADDX_b/w BFI+ADCS carry chain requires all-ones padding** — `MOVN_xi` initializes REG_WORK1 to `0xFFFFFFFF`, then BFI inserts the source byte/word into the upper bits. The lower bits MUST be all-ones for ADCS to produce the correct carry. SUBX_b/w doesn't have this issue because it uses LSL (which zeros the lower bits) on both operands.
