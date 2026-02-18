---
name: amiberry-jit-fix
description: >
  ARM64 JIT compiler fixes for Amiberry (GitHub issue #1766). This skill captures the complete
  technical knowledge from 20+ debugging sessions that identified and fixed: (a) a page 0 DMA
  corruption crash during A1200 Kickstart boot, (b) visual corruption (black gadgets, garbled
  text) caused by incorrect inter-block flag optimization, and (c) SIGSEGV crashes on unmapped
  natmem gaps (e.g., 0x00F10000) in complex configs with hardfiles/RTG. Use this skill when working on:
  (1) the JIT_DEBUG_MEM_CORRUPTION code in compemu_support_arm.cpp or newcpu.cpp,
  (2) the page 0 DMA guard mechanism (mprotect/SIGSEGV/BRK single-step),
  (3) any ARM64 JIT register width bugs (64-bit vs 32-bit instruction selection),
  (4) any crash or exception cascade during A1200 boot with JIT enabled,
  (5) 68040 JIT mode issues on ARM64,
  (6) the inter-block flag optimization bug and its fix (dont_care_flags at block boundaries),
  (7) natmem gap crashes and the commit_natmem_gaps() fix in amiberry_mem.cpp.
  MANDATORY TRIGGERS: JIT crash, page 0, DMA guard, vec2, exception vector corruption,
  blitter DMA, mprotect, SIGSEGV handler, BRK single-step, ARM64 JIT, Kickstart boot,
  visual corruption, black gadgets, garbled ROM version, compemu_support_arm,
  JIT_DEBUG_MEM_CORRUPTION, inter-block flag, dont_care_flags, 68040 JIT,
  natmem gap, unmapped region, commit_natmem_gaps, 0x00F10000, canbang, PROT_NONE.
---

# Amiberry ARM64 JIT Crash Fix — Session Knowledge

## Status Summary

**Crash fix: SOLVED (v43-clean).** System boots to Workbench and runs SysInfo without crash.
**Visual corruption: SOLVED.** Caused by inter-block flag optimization in compile_block() — disabled with `#if 0`.
**Natmem gap crash: SOLVED.** Complex configs (hardfiles, RTG, etc.) crashed at unmapped natmem gaps — fixed by `commit_natmem_gaps()`.
**ARM64 JIT runtime status (2026-02): STABLE with narrow fallback.**
- SysInfo + A4000 configs pass with JIT enabled.
- Lightwave boots with JIT enabled using a narrow ARM64 hotspot fallback.
- Broad ARM64 Z3 fallback was removed (performance parity improved).
- `jit_n_addr_unsafe` now starts at `0` again (direct mode), with normal unsafe-bank escalation.

## Root Cause (Crash)

Kickstart ROM programs the blitter to DMA-clear M68k addresses 0x004-0x01B (exception vectors 1-6) during init. With JIT's asynchronous blitter, DMA fires BETWEEN compiled blocks, zeroing vectors. If an exception fires during this window, the CPU jumps to address 0 → illegal instruction cascade → crash.

DMA also corrupts FPU vectors (0x0c0-0x0d8), OS data (0x5d0+), and other page 0 locations. The v42 fix only protected vectors 1-6; v43's full-page shadow fixed all addresses.

## Fix Mechanism (v43-clean)

Three cooperating components, all behind `#ifdef JIT_DEBUG_MEM_CORRUPTION`:

1. **Vec2 dispatch detection** (`jit_dbg_check_vec2_dispatch`): Called from 6 C dispatch points. Watches vec2 (Bus Error vector at M68k 0x008). When it changes (exec library replacing ROM handlers), snapshots page 0 into 4KB shadow, installs signal handlers, and arms `mprotect(PROT_READ)` on natmem page 0.

2. **SIGSEGV→BRK single-step**: Write to protected page triggers SIGSEGV handler. Handler saves write address/PC, unprotects page, inserts ARM64 `BRK #0x7777` at PC+4, returns. Faulting store re-executes, then hits BRK.

3. **SIGTRAP shadow logic**: BRK fires SIGTRAP. Handler distinguishes DMA vs CPU writes by checking if ARM64 PC is inside JIT code cache (`compiled_code` to `current_compile_p`):
   - **DMA write** (PC outside cache): Restore 4-byte-aligned value from shadow → undo corruption
   - **CPU write** (PC inside cache): Update shadow with new value; also update `jit_vec_shadow[]` for vectors 1-6
   - Re-protects page immediately. Page unprotected for exactly ONE instruction.

4. **Exception_normal safety net** (newcpu.cpp): Before dispatching vectors 1-6, compares `x_get_long()` result against `jit_vec_shadow[]`. If mismatch, uses shadow value instead.

## Files Modified (8 total)

See [references/file-map.md](references/file-map.md) for detailed file-by-file breakdown with line numbers.

| File | Changes |
|------|---------|
| `src/jit/arm/compemu_support_arm.cpp` | DMA guard: globals, signal handlers, dispatch function, arming code (~345 lines added) |
| `src/newcpu.cpp` | Exception_normal guard + 4 dispatch call sites (~50 lines added) |
| `src/osdep/sysconfig.h` | `#define JIT_DEBUG_MEM_CORRUPTION` compile flag (add manually) |
| `src/include/sysdeps.h` | JITCALL define for ARM/ARM64 |
| `src/jit/arm/codegen_arm64.cpp` | 64→32-bit instruction width fixes (MOVN_xi→MOVN_wi etc.) |
| `src/jit/arm/compemu_midfunc_arm64.cpp` | Register width fixes for mid-functions |
| `src/jit/arm/compemu_midfunc_arm64_2.cpp` | Register width fixes for mid-functions (part 2) |
| `src/jit/arm/compstbl_arm.cpp` | Compiler table regeneration (mechanical) |

## Patches

Two clean patches were generated:
- `amiberry-arm64-jit-v43-clean.patch` — 2-file patch (compemu_support_arm.cpp + newcpu.cpp only, 560 lines)
- `amiberry-arm64-jit-v43-clean-full.patch` — All 8 files (9,276 lines)

Note: `JIT_DEBUG_MEM_CORRUPTION` must be defined in sysconfig.h manually (not in the patches).

## Compile Flag

Add to `src/osdep/sysconfig.h`:
```c
#define JIT_DEBUG_MEM_CORRUPTION
```
Place it near the other JIT defines (around line 19-30 where `#define JIT` is).

## Crash Progression Through Versions

- **v40**: Fixed H3 error → revealed Guru Meditation
- **v41**: Fixed Guru (multi-vector shadow) → revealed green screen
- **v42**: Fixed green screen (BRK single-step for vectors 1-6) → revealed it missed non-vector DMA
- **v43**: Full-page 4KB shadow → **clean boot to SysInfo** (57 DMA writes across 25 addresses restored)
- **v43-clean**: Removed ~1,300 lines of diagnostic scaffolding, kept ~300 lines of core fix

## Visual Corruption (SOLVED)

### Root Cause
Inter-block flag optimization in `compile_block()` (`src/jit/arm/compemu_support_arm.cpp`)
incorrectly discards CPU flags at block boundaries. The optimization uses a single-instruction
lookahead to decide if flags can be dropped, but this misses cases where the second (or later)
instruction in the next block needs those flags.

### Fix
Changed `#if 1` to `#if 0` on the inter-block flag optimization block (~line 3528).
This matches Amiberry-Lite's ARM64 JIT which never had this optimization.

### Symptoms (now fixed)
- Black window gadgets (should be blue/white)
- Missing AmigaDOS window title text
- Garbled ROM version string during Kickstart splash
- H3 CPU crash when loading SysInfo

### Key Insight
The bug was NOT in any individual opcode handler — it was in the compile_block()
infrastructure that manages flag state between blocks. This is why exhaustive comparison
of all codegen functions found them correct: they ARE correct. The corruption came from
`dont_care_flags()` being called at block boundaries when it shouldn't have been.

### Disproved Hypotheses
- BFI_wwii (32-bit) vs BFI_xxii (64-bit): BFI_wwii is CORRECT; reverting to xxii caused H3 crashes
- Infrastructure 32→64-bit width changes: 32-bit is CORRECT; reverting caused H3 crashes
- Individual opcode handler bugs: All handlers verified correct via line-by-line comparison

See [references/visual-corruption-investigation.md](references/visual-corruption-investigation.md) for the full investigation history, bisection methodology, and diagnostic tools.

## Key Technical Details

See [references/technical-details.md](references/technical-details.md) for signal handler implementation, BRK mechanism, DMA vs CPU write distinction, byte order handling, and ARM64 register width fix patterns.

## Natmem Gap Crash (SOLVED)

### Root Cause
Complex configs (A4000, 68060/040/020, JIT+FPU, hardfiles, RTG) crashed with SIGSEGV at M68k addresses
in unmapped natmem gaps (e.g., 0x00F10000 — a 448K gap between UAE Boot ROM at 0x00F00000 and
Kickstart ROM at 0x00F80000). The natmem block is reserved with `PROT_NONE` (2GB), and only actual
memory banks get committed. Gaps between banks remain uncommitted, causing faults when both JIT-compiled
code and the interpreter (in `canbang`/direct mode) access natmem directly.

The existing SIGSEGV handler (`sigsegv_handler.cpp`) only handles faults inside JIT code range. Faults
from interpreter code (outside JIT range) are not recovered — they trigger `max_signals` countdown and crash.

### Fix
`commit_natmem_gaps()` in `src/osdep/amiberry_mem.cpp` — called at end of `memory_reset()` in `src/memory.cpp`.
After all memory banks are mapped, walks the natmem space and commits any remaining gaps via `uae_vm_commit()`
with the correct fill value (zero or 0xFF based on `cs_unmapped_space`). Zero runtime cost — direct memory
access just works with no signals or bank lookups needed.

### Files Modified (3 total)
| File | Changes |
|------|---------|
| `src/osdep/amiberry_mem.cpp` | Added `commit_natmem_gaps()` function (~110 lines) |
| `src/include/uae/mman.h` | Added declaration |
| `src/memory.cpp` | Added call site in `memory_reset()` behind `#ifdef NATMEM_OFFSET` |

### Key Technical Details
- Uses `shmids[]` array (MAX_SHMID=256) to collect committed regions
- Calculates offsets relative to `natmem_reserved` (not `natmem_offset`, which differs with RTG offset)
- Page-aligns gap boundaries to host page size
- Fill byte: `0x00` for `cs_unmapped_space` 0/1, `0xFF` for `cs_unmapped_space` 2
- Cross-platform: works on Linux (`mprotect`), macOS, and Windows (`VirtualAlloc(MEM_COMMIT)`)
- Idempotent — safe to call on config changes/resets

## Current ARM64 Guard Policy (2026-02)

Implemented in `src/jit/arm/compemu_support_arm.cpp`:

- ROM and UAE Boot ROM (`rtarea`) blocks run interpreter-only on ARM64.
- Narrow fixed-safety interpreter fallback for known unstable Lightwave startup range:
  - `PC 0x4003df00` to `0x4003e1ff`
  - log marker: `JIT: ARM64 guard active, running known unstable ARM64 block range without JIT`
- Dynamic unstable-block quarantine is learned at runtime from:
  - SIGSEGV/JIT fault recovery (`sigsegv_handler.cpp`)
  - Autoconfig warning paths (`expansion.cpp`)
  - Selected startup illegal-op probes (`newcpu.cpp`, `op_illg`)
- Dynamic unstable-key tracking now uses a bitmap (O(1) lookup) and resets on `compemu_reset()`.
- `AMIBERRY_ARM64_DISABLE_HOTSPOT_GUARD=1` only disables optional hotspot logic and does not bypass the fixed safety hotspot guard.
- `AMIBERRY_ARM64_GUARD_VERBOSE=1` enables per-key/per-window dynamic guard learning logs for diagnostics.

## Known Separate Issues

- `pull overflow` log spam can still occur under heavy workloads, even when emulation is stable.
- One narrow ARM64 hotspot fallback remains; full parity goal is to eliminate it without regressions.
