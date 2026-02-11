---
name: amiberry-jit-fix
description: >
  Continue development of the ARM64 JIT compiler crash fix for Amiberry (GitHub issue #1766).
  This skill captures the complete technical knowledge from 15+ debugging sessions that identified
  and fixed a page 0 DMA corruption bug during A1200 Kickstart boot. Use this skill when working
  on: (1) the JIT_DEBUG_MEM_CORRUPTION code in compemu_support_arm.cpp or newcpu.cpp,
  (2) the page 0 DMA guard mechanism (mprotect/SIGSEGV/BRK single-step),
  (3) the visual corruption issue (black gadgets, garbled ROM version text) that remains unsolved,
  (4) any ARM64 JIT register width bugs (64-bit vs 32-bit instruction selection),
  (5) any crash or exception cascade during A1200 boot with JIT enabled.
  MANDATORY TRIGGERS: JIT crash, page 0, DMA guard, vec2, exception vector corruption,
  blitter DMA, mprotect, SIGSEGV handler, BRK single-step, ARM64 JIT, Kickstart boot,
  visual corruption, black gadgets, garbled ROM version, compemu_support_arm, JIT_DEBUG_MEM_CORRUPTION.
---

# Amiberry ARM64 JIT Crash Fix — Session Knowledge

## Status Summary

**Crash fix: SOLVED (v43-clean).** System boots to Workbench and runs SysInfo without crash.
**Visual corruption: UNSOLVED.** Black window gadgets and garbled ROM version text persist.

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

## Visual Corruption (UNSOLVED)

Black window gadgets (should be blue/white) and garbled ROM version text ("ROM 2C.21..." instead of "ROM 2C.2ft") appear during boot. Present across ALL versions, including before the DMA guard. This is a separate JIT issue, NOT caused by page 0 DMA corruption.

See [references/visual-corruption-investigation.md](references/visual-corruption-investigation.md) for the exhaustive comparison between Amiberry and Amiberry-Lite that has ruled out essentially ALL JIT code differences.

### Key Finding: All JIT Code is Functionally Identical
An exhaustive line-by-line comparison of 12 major subsystems (codegen, midfuncs, memory access, compile_block, build_comp, popall, compemu, cpuemu handlers, struct offsets, regflags, special_mem, bank lookup) found the ARM64 JIT code to be functionally identical between Amiberry-Lite (where it works) and current Amiberry (where it doesn't).

### What Differs (Not Yet Proven Causative)
1. **cpuemu_40.cpp returns 0** instead of actual cycle counts (by design in newer WinUAE gencpu)
2. **execute_normal() hardcodes 4*CYCLE_UNIT** and has adjust_cycles() commented out
3. **Newer WinUAE emulator core** may have behavioral differences in chipset emulation, event system, or memory bank initialization that interact differently with JIT-compiled code

### Next Steps
Runtime diagnostics needed — code comparison has been exhausted. Need to instrument JIT at runtime to catch wrong values being read/written.

## Key Technical Details

See [references/technical-details.md](references/technical-details.md) for signal handler implementation, BRK mechanism, DMA vs CPU write distinction, byte order handling, and ARM64 register width fix patterns.

## Known Separate Issues

- "Error in compiled code" at M68k 0x00F00000 (unmapped region) — occurs early in every boot, system recovers. Separate JIT bug, not addressed.
