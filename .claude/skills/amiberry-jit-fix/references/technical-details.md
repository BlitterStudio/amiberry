# Technical Details

## Table of Contents
1. BRK Single-Step Mechanism
2. DMA vs CPU Write Distinction
3. Vec2 Change Detection
4. Byte Order Handling
5. Signal Handler Chain
6. ARM64 Register Width Bug Pattern
7. Key Memory Addresses
8. Log Throttling Pattern
9. Testing Configuration

## 1. BRK Single-Step Mechanism

The core innovation allowing per-write protection without performance collapse.

**Problem**: `mprotect(PROT_READ)` catches ALL writes to page 0. Simply unprotecting on each SIGSEGV leaves a window where other DMA writes slip through before re-protection.

**Solution**: ARM64 `BRK` instruction as a software single-step:

```
SIGSEGV fires (write to protected page)
  ↓
Handler: save fault info, unprotect page, insert BRK at PC+4
  ↓
Return from handler → faulting store re-executes (succeeds)
  ↓
CPU hits BRK at PC+4 → SIGTRAP fires
  ↓
SIGTRAP handler: restore original insn, restore/update shadow, re-protect page
```

Key details:
- BRK encoding: `0xD42EEEE0` = `BRK #0x7777`
- JIT code cache pages must be made RWX temporarily to insert BRK: `mprotect(code_page, 4096, PROT_READ | PROT_WRITE | PROT_EXEC)`
- I-cache flush required after BRK insertion AND after restoration: `__builtin___clear_cache()`
- Safety: if a previous BRK is still pending (`jit_dbg_saved_next_insn_addr != NULL`), fall back to unprotect-and-return

## 2. DMA vs CPU Write Distinction

After each write completes (in SIGTRAP handler), determine origin:

```c
int from_jit = (compiled_code != NULL &&
    wr_pc >= (uae_u64)(uintptr_t)compiled_code &&
    wr_pc < (uae_u64)(uintptr_t)current_compile_p);
```

- `compiled_code` = start of JIT code cache (set during init)
- `current_compile_p` = current compilation pointer (advances as blocks compile)
- ARM64 PC in this range = write from JIT-compiled M68k code = legitimate CPU write
- ARM64 PC outside this range = write from C code = blitter DMA or other host-side write

**DMA write**: Restore 4-byte-aligned value from `jit_page0_shadow`. This undoes the corruption.

**CPU write**: Update `jit_page0_shadow` with the new value. Also update `jit_vec_shadow[1-6]` if the write targeted a vector address (using `do_byteswap_32()` to convert to M68k big-endian).

## 3. Vec2 Change Detection

The DMA guard arms when exec library init starts replacing ROM exception handlers:

1. `jit_dbg_check_vec2_dispatch()` reads vec2 at M68k 0x008 (Bus Error vector)
2. First non-zero value: remember as `jit_dbg_vec2_last` (this is the ROM handler address)
3. First CHANGE from this value: exec library is installing its own handlers → arm protection
4. After arming (`jit_dbg_vec2_trap_armed = 1`), the dispatch function returns immediately on all future calls

Dispatch call sites (6 total, called from C functions that JIT blocks transition through):
- `do_nothing()` — MOST CRITICAL: called on cycle budget expiration during hash-table block chaining
- `exec_nostats()` — interpreter loop entry
- `execute_normal()` ENTRY — before interpreter + compile
- `execute_normal()` PRE_COMPILE — after interpreter loop, before compile_block
- `recompile_block()` — before recompilation
- `cache_miss()` — on cache miss
- End of `compile_block()` — after compilation completes

## 4. Byte Order Handling

Amiga M68k is big-endian. ARM64 host is little-endian. natmem stores values in host byte order.

- `x_get_long()` in newcpu.cpp returns M68k big-endian values
- Raw natmem reads return host little-endian values
- `jit_vec_shadow[]` stores M68k big-endian format (matching `x_get_long()` output)
- Conversion: `uae_u32 m68k_val = do_byteswap_32(raw_natmem_val)`
- The v40 bug was comparing LE natmem values against BE shadow values → never matched

## 5. Signal Handler Chain

Both handlers chain to previous handlers for non-matching signals:

**SIGSEGV**: Check if fault address is in our protected page (`page_base` to `page_base + 4096`). If not, call `jit_dbg_old_sigaction` (SA_SIGINFO path or simple handler path). If old handler is SIG_DFL, re-raise SIGSEGV with default handler.

**SIGTRAP**: Check if PC matches `jit_dbg_saved_next_insn_addr`. If not, call `jit_dbg_old_sigtrap_action`. If no old handler, log warning and return (don't terminate).

## 6. ARM64 Register Width Bug Pattern

The JIT was using 64-bit X-register instructions where 32-bit W-register instructions were needed. This caused sign-extension to pollute the upper 32 bits.

**Example**: Loading the M68k value `0xFFFF0001`:
- `MOV_xi(r, 0x0001); MOVK_xish(r, 0xFFFF, 16)` → `r = 0x00000000FFFF0001` ✓ looks OK
- But `MOVN_xi(r, ~0xFFFFxxxx)` → `r = 0xFFFFFFFFFFFFxxxx` ✗ upper 32 bits dirty

When this register is later used as an index in `[X27, Xn]` (where X27 = natmem base), the dirty upper bits create a completely wrong address, crashing or corrupting memory.

**Fix pattern**: Replace all `_xi`, `_xx` suffixed instructions with `_wi`, `_ww` equivalents:
- `MOVN_xi` → `MOVN_wi` (32-bit move-not)
- `MOV_xi` → `MOV_wi` (32-bit move)
- `MOVK_xish` → `MOVK_wish` (32-bit move-keep with shift)
- `SXTB_xx` → `SXTB_ww` (32-bit sign-extend byte)
- `SXTH_xx` → `SXTH_ww` (32-bit sign-extend halfword)

The W-register variants automatically zero the upper 32 bits of the corresponding X-register on ARM64.

## 7. Key Memory Addresses

| M68k Address | Purpose | DMA affected? |
|-------------|---------|---------------|
| 0x000 | Reset SSP | No |
| 0x004 | Reset PC (Vector 1) | Yes |
| 0x008 | Bus Error (Vector 2) | Yes — primary trigger |
| 0x00C | Address Error (Vector 3) | Yes |
| 0x010 | Illegal Instruction (Vector 4) | Yes |
| 0x014 | Division by Zero (Vector 5) | Yes |
| 0x018 | CHK Instruction (Vector 6) | Yes |
| 0x01C-0x03C | Other exception vectors | Possibly |
| 0x0C0-0x0D8 | FPU vectors | Yes (found by v43) |
| 0x5D0+ | OS data area | Yes (found by v43) |
| 0xBFC-0xC2C | OS data area | Yes (found by v43) |

## 8. Log Throttling Pattern

All counters use the same throttling pattern to avoid log spam:
```c
if (count <= 30 ||
    (count <= 300 && count % 10 == 0) ||
    (count % 500 == 0))
```
This logs the first 30 events individually, then every 10th up to 300, then every 500th.

## 9. Testing Configuration

The fix was tested on:
- A1200 configuration with Kickstart ROM
- JIT enabled with `JIT_DEBUG_MEM_CORRUPTION` defined
- ARM64 (aarch64) Linux host
- Boot sequence: Kickstart init → Workbench → SysInfo (full boot test)
- v43 test showed 57 DMA writes restored across 25 unique addresses with zero crashes
