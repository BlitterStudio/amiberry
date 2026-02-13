# File Map — v43-clean Patch

Detailed breakdown of all modified files and what changed in each.

## 1. src/jit/arm/compemu_support_arm.cpp (Main DMA Guard)

This is the primary file for the page 0 DMA guard. All new code is inside `#ifdef JIT_DEBUG_MEM_CORRUPTION` blocks.

### New includes (top of file)
```c
#include <signal.h>
#include <sys/mman.h>
#include <ucontext.h>
#include <errno.h>
#include <dlfcn.h>
```

### Globals section (after `flush_icache_none` declaration, ~line 314)
- `jit_dbg_vec2_last` — Last-seen vec2 value (triggers arming on change)
- `jit_dbg_old_sigaction` / `jit_dbg_old_sigtrap_action` — Saved signal handlers for chaining
- `jit_dbg_vec2_page_protected` — Whether natmem page 0 is currently mprotect'd
- `jit_dbg_vec2_trap_armed` — Whether the DMA guard is active (set once, never cleared)
- `jit_dbg_saved_next_insn` / `jit_dbg_saved_next_insn_addr` — BRK single-step state
- `jit_dbg_brk_step_count` — Total BRK steps (for log throttling)
- `JIT_DBG_BRK_IMM` = `0xD42EEEE0` — ARM64 encoding of `BRK #0x7777`
- `jit_dbg_last_write_m68k_addr` / `jit_dbg_last_write_arm64_pc` — SIGSEGV→SIGTRAP communication
- `jit_dbg_vec2_sigsegv_count` — Total SIGSEGV count (for log throttling)
- `jit_page0_shadow[4096]` — Full-page shadow buffer
- `jit_page0_shadow_valid` — Whether shadow has been initialized
- `jit_dbg_page0_restore_count` / `jit_dbg_vec_restore_count` / `jit_dbg_vec2_write_count` — Counters
- `jit_vec_shadow[7]` — Vector shadow for Exception_normal safety net (M68k BE format, extern linkage)

### SIGTRAP handler: `jit_dbg_vec2_sigtrap_handler`
Handles BRK #0x7777 breakpoint after single-step. Core v43 logic:
1. Verify PC matches our BRK address
2. Restore original instruction at BRK location
3. Check if write was DMA or CPU (by comparing ARM64 PC against JIT code cache range)
4. DMA: restore from shadow. CPU: update shadow + vec_shadow
5. Re-protect page with `mprotect(PROT_READ)`
6. If not our BRK, chain to old SIGTRAP handler

### SIGSEGV handler: `jit_dbg_vec2_sigsegv_handler`
Handles page fault on protected natmem page 0:
1. Verify fault address is in our protected page (chain to old handler if not)
2. Save write M68k address and ARM64 PC for SIGTRAP handler
3. Unprotect page temporarily
4. Log vector writes (M68k 0x004-0x01B)
5. Insert BRK at PC+4 (making code page RWX first)
6. Return — faulting store re-executes, then BRK fires SIGTRAP

### Dispatch function: `jit_dbg_check_vec2_dispatch`
Called from 6 C dispatch points to detect when to arm protection:
1. Early return if already armed or natmem not ready
2. Read vec2 at M68k 0x008
3. Skip if zero (ROM hasn't initialized yet)
4. Track first non-zero value
5. On first CHANGE: initialize vec_shadow[1-6], install signal handlers, memcpy page 0 shadow, mprotect arm

### alloc_code fix (unrelated to DMA guard)
Replaced `assert((uintptr)ptr <= 0xffffffff)` with a proper check and warning for 64-bit systems.

### create_popalls fix (unrelated to DMA guard)
Added ARM64 I-cache flush (`flush_cpu_icache`) before `vm_protect`, fixing stale I-cache issues.

### Dispatch call sites (3 in this file)
- `recompile_block()` — before recompilation
- `cache_miss()` — on cache miss
- End of `compile_block()` — after compilation completes

## 2. src/newcpu.cpp

### Exception_normal guard (after `newpc = x_get_long(regs.vbr + 4 * vector_nr)`)
For vectors 1-6: if `newpc` differs from `jit_vec_shadow[vector_nr]`, override with shadow value. Uses `extern uae_u32 jit_vec_shadow[]` to access the shadow array from compemu_support_arm.cpp.

### Dispatch call sites (4 in this file)
Each calls `extern void jit_dbg_check_vec2_dispatch(const char*)`:
- `do_nothing()` — cycle budget expiration (MOST CRITICAL: only C function called during hash-table block chaining)
- `exec_nostats()` — interpreter loop entry
- `execute_normal()` ENTRY — before interpreter + compile
- `execute_normal()` PRE_COMPILE — after interpreter loop, before compile_block

## 3. src/osdep/sysconfig.h

Add `#define JIT_DEBUG_MEM_CORRUPTION` manually near lines 19-30 (where `#define JIT` and `#define USE_JIT_FPU` are). This is NOT in the patches — it's a manual step.

## 4. src/include/sysdeps.h

Added JITCALL definition for ARM/ARM64:
```c
#elif defined(CPU_arm) || defined(CPU_AARCH64)
/* ARM/ARM64: parameters passed in R0-R7 by default (AAPCS/AAPCS64). */
#define JITCALL
```

## 5. src/jit/arm/codegen_arm64.cpp

ARM64 register width fixes — changed 64-bit X-register instructions to 32-bit W-register equivalents to prevent dirty upper 32 bits:

| Function | Old | New | Why |
|----------|-----|-----|-----|
| `SIGNED8_IMM_2_REG` | `MOVN_xi` / `MOV_xi` | `MOVN_wi` / `MOV_wi` | 64-bit MOVN sign-extends to all 64 bits |
| `UNSIGNED16_IMM_2_REG` | `MOV_xi` | `MOV_wi` | Upper 32 bits left dirty |
| `SIGNED16_IMM_2_REG` | `MOVN_xi` / `MOV_xi` | `MOVN_wi` / `MOV_wi` | Same as SIGNED8 |
| `SIGNED8_REG_2_REG` | `SXTB_xx` | `SXTB_ww` | 64-bit sign-extend corrupts upper bits |
| `SIGNED16_REG_2_REG` | `SXTH_xx` | `SXTH_ww` | Same as SXTB |
| `LOAD_U32` | `MOVN_xi` / `MOV_xi` / `MOVK_xish` | `MOVN_wi` / `MOV_wi` / `MOVK_wish` | Values like 0xFFFFxxxx became 0xFFFFFFFFFFFFxxxx |
| `raw_flags_to_reg` | Removed dead `idx` variable | — | Cleanup |

Also added I-cache flush in `create_popalls`.

## 6. src/jit/arm/compemu_midfunc_arm64.cpp

Register width fixes in JIT mid-functions. Same pattern: 64-bit instructions changed to 32-bit to prevent dirty upper bits corrupting Amiga addresses used in register-indexed addressing `[Xn, X27]`.

## 7. src/jit/arm/compemu_midfunc_arm64_2.cpp

More register width fixes in JIT mid-functions (part 2). Same pattern as above.

## 8. src/jit/arm/compstbl_arm.cpp

Compiler table regeneration. This is mostly mechanical — regenerated from the opcode table. 7,474 lines changed but all automated.
