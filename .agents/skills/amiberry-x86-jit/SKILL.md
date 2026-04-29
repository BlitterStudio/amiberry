---
name: amiberry-x86-jit
description: >
  Reference workflow for Amiberry x86-64 JIT debugging and fixes. Use for x86 or x86-64 JIT
  allocator failures, RIP-relative `±2GB` distance bugs, PIE or ASLR allocation issues,
  high-natmem/direct-mode bugs, `PC_P` pointer-width regressions, `MAP_32BIT` or `VirtualAlloc`
  fallback bugs, runtime JIT disable or interpreter fallback bugs,
  and regressions in files such as `src/jit/x86/compemu_support_x86.cpp` and
  `src/jit/x86/compemu_x86.h`.
---

# Amiberry x86-64 JIT — Allocation And RIP-Relative Reference

## Scope

- Use this skill for x86/x86-64 JIT issues in `src/jit/x86/`.
- Use `amiberry-arm64-jit` instead for `src/jit/arm/` codegen and ARM64 flag handling.
- Load [references/address-space-and-failures.md](references/address-space-and-failures.md) when you need the recent allocator fixes, invariants, or symbol names.

## Core Problem Shape

The main x86-64 failure mode is not "allocation failed" in the generic sense. It is "allocation succeeded at an address that the JIT cannot safely reference" or "a pointer-width value was routed through a 32-bit virtual-register path."

x86-64 JIT code uses RIP-relative addressing to reach globals and nearby code/data. That means:

- Low address does not automatically mean safe address.
- The correct check is distance from the actual base used by the JIT path, not distance from an arbitrary `.data` symbol.
- Pool allocations are dangerous because they can bypass later code-cache range checks.

High natmem above 4GB is supported. Do not treat it as a reason to disable JIT or broadly fall back to interpreter mode; the goal is to keep direct compiled performance unless a real special memory bank requires helper paths.

## Non-Negotiable Invariants

- Any allocation used by RIP-relative helpers must be within safe range of the effective JIT anchor.
- Under PIE or high ASLR, validate against the real base chosen by the allocator path:
  - `vm_acquire_anchor` when already established
  - otherwise the first cache base or fallback anchor used by the current code path
- Linux and Windows last-resort allocation paths both need distance validation.
- If allocation failure disables JIT at runtime, keep `cache_enabled`, `currprefs.cachesize`, and `changed_prefs.cachesize` in sync.
- If block compilation fails after partial setup, clean the block state with `invalidate_block(bi)` and clear stale tags with `flush_icache_hard(3)`.
- `PC_P` (vreg 16) is the only pointer-width virtual register. All Dn/An/temp non-`PC_P` vregs are 32-bit M68K values.
- `set_const()` must mask non-`PC_P` values to 32 bits; any host pointer copied into a non-`PC_P` vreg must be materialized with a 64-bit register path or represented as a 32-bit displacement plus a pointer-width `PC_P`/`R_MEMSTART` add.
- Constant `PC_P` copies in `mov_l_rr()` must not use the generic `mov_l_ri()` shortcut, because that truncates when copied into a non-`PC_P` destination.
- `R_MEMSTART` holds `natmem_offset` on x86-64 and is the base for direct high-natmem loads/stores.
- `jit_n_addr_unsafe` can mean "high natmem needs pointer-clean codegen"; `jit_n_addr_bank_unsafe` means "a real bank requires helper paths." Do not collapse these two cases.

## Investigation Workflow

1. Identify whether the failure is in code-cache allocation, pool allocation, or a later block-compilation path.
2. Inspect the active anchor used by `jit_vm_acquire()`.
3. Check whether the allocator is validating distance after every fallback, not just the primary path.
4. Verify whether a helper or fallback returns success on an address that is valid to the OS but invalid for RIP-relative codegen.
5. If JIT is disabled on failure, trace whether later prefs-apply paths can silently re-enable it.
6. For high-natmem regressions, verify whether the failing path is truncating `PC_P`, `natmem_offset`, `regs.pc_p`, or cache/tag pointers through 32-bit stores, loads, adds, or `isconst` shortcuts.
7. Separate correctness fallbacks from performance fallbacks: real `S_N_ADDR` banks can use helpers, but high natmem alone should still compile normal blocks.

## Linux x86-64 Allocator Rules

- Prefer a VMA-aware near-address search such as `find_nearest_gap()` before blind probing.
- Adaptive probe stride is safer than a hardcoded large stride; tiny pool allocations can fit into holes that a 2MB walk will skip.
- `UAE_VM_32BIT` / `MAP_32BIT` is a strategy, not a proof. A returned pointer can still be out of safe range under PIE layouts.
- If a `MAP_32BIT` allocation is returned but rejected by range checks, consider the next fallback instead of disabling JIT immediately.

## macOS x86-64 Allocator Rules

- Darwin can ignore advisory `mmap()` address hints and return a valid mapping far from the requested anchor.
- Use exact near-address allocation for x86-64 JIT buffers that need RIP-relative reach; do not accept a hinted result unless it is range-checked.
- `veccode`, `popallspace`, block pools, and the JIT cache all need to stay within the same ±2GB anchor discipline.

## Windows x86-64 Allocator Rules

- Treat the Windows path the same way conceptually: search near the active base first, then validate every fallback.
- Unanchored `VirtualAlloc(NULL, ...)` is not safe unless the result is range-checked and freed on failure.
- Remember that lazy block or pool allocators may bypass any post-hoc code-cache validation done elsewhere.

## Common Review Traps

- Range-checking against `&data_anchor` even though the effective base is `vm_acquire_anchor` or the compiled cache.
- Trying unanchored allocation only when `MAP_32BIT` fails, but not when it succeeds and is then rejected as too far away.
- Clearing only `currprefs.cachesize` when disabling JIT, allowing a later prefs apply to re-enable the broken path.
- Forgetting to move new helper definitions above first use, breaking x86 CI with simple declaration-order errors.
- Returning early from `compile_block()` without invalidating partially initialized state.
- Routing constant `PC_P` through `mov_l_ri()` or `set_const()` for a non-`PC_P` temp.
- Using `readreg_offset(PC_P, ...)` when the logical value, including deferred offsets, is needed.
- Treating `jit_n_addr_unsafe` as a blanket interpreter/helper fallback when `jit_n_addr_bank_unsafe` is false.
- Leaving one-off investigation logs such as H3 diagnostics in generic memory paths.

## Verification

- Test x86-64 Linux with PIE or aggressive ASLR and sustained Workbench usage.
- Test x86-64 Windows fallback paths if code changed under `_WIN32`.
- Confirm normal JIT behavior when allocations succeed.
- Confirm graceful interpreter fallback when allocations fail.
- Change an unrelated preference after runtime JIT disable and verify the failing JIT path does not come back.
- Run a high-natmem SysInfo/Workbench smoke test and check for no H3/software failure/CPU-halt markers.
- Confirm performance is at JIT levels, not interpreter-level MIPS, after high-natmem fixes.
