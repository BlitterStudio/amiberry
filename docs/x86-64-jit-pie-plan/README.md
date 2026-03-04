# x86-64 JIT PIE Compatibility Plan

## Problem

The x86-64 JIT currently requires `-fno-pie` because it uses RIP-relative
`[RIP+disp32]` addressing (via the `_r_X` macro in `codegen_x86.h`) for global
variable access. This requires JIT code to be within ±2GB of `.data`. Non-PIE
builds satisfy this by placing `.data` at fixed low addresses, but PIE+ASLR
randomizes `.data` placement.

**Key insight:** RIP-relative addressing is the *standard* x86-64 mode — it's
not anti-PIE. We just need to guarantee JIT code allocation stays within ±2GB
of `.data`. The Windows port already does this via a VirtualQuery-based anchor
allocator. We extend this to Linux.

## Phases (execute in order)

| Phase | File | Summary |
|-------|------|---------|
| 1 | [phase-1-fix-branch-truncation.md](phase-1-fix-branch-truncation.md) | Fix 32-bit pointer truncation in conditional branch emitters |
| 2 | [phase-2-fix-veccode-allocation.md](phase-2-fix-veccode-allocation.md) | Change veccode allocation from MAP_32BIT to anchor-based |
| 3 | [phase-3-harden-probe-loop.md](phase-3-harden-probe-loop.md) | Harden vm_acquire probe loop for PIE+ASLR reliability |
| 4 | [phase-4-add-distance-diagnostic.md](phase-4-add-distance-diagnostic.md) | Add runtime proximity check between JIT cache and globals |
| 5 | [phase-5-remove-pie-guard-and-flags.md](phase-5-remove-pie-guard-and-flags.md) | Remove #error guard and -fno-pie flags |
| 6 | [phase-6-update-comments.md](phase-6-update-comments.md) | Update comments explaining constraints |

## What stays unchanged (and why)

| Component | Reason |
|-----------|--------|
| `_r_X` RIP-relative macro | Works fine — anchor strategy guarantees proximity |
| R_MEMSTART (R15) register-indirect | Already PIE-safe |
| PC_P 64-bit handling | Already fully 64-bit in register allocator |
| natmem UAE_VM_32BIT | comp_pc_p and other 32-bit arithmetic depend on low-4GB natmem |
| isconst guards | Already correctly disabled on x86-64 |
| Function calls/jumps | Already use movabs+indirect (PIE-safe) |
| FreeBSD ADDR32 path | Keeps -fno-pie; PIE would require removing ADDR32 (separate project) |

## Verification (after all phases)

1. **Build as PIE:** `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc)`, then `readelf -h build/amiberry | grep Type` → should show `DYN`
2. **JIT functional test:** Boot A1200 with JIT (cachesize=8192), run WorkBench
3. **Check diagnostics:** Run with `--log`, verify no distance warnings
4. **FPU test:** Run FPU-heavy software (triggers comp_pc_p assert path)
5. **Non-PIE regression:** Build with explicit `-fno-pie` to verify it still works
6. **FreeBSD:** Verify FreeBSD build still gets `-fno-pie` from CMake

## Key files reference

| File | Role |
|------|------|
| `src/jit/x86/codegen_x86.cpp` | Instruction emitters (branch functions) |
| `src/jit/x86/codegen_x86.h` | `_r_X` macro (RIP-relative addressing) |
| `src/jit/x86/compemu_support_x86.cpp` | JIT cache allocation, anchor probe loop, #error guard |
| `src/jit/x86/exception_handler.cpp` | veccode trampoline allocation |
| `src/jit/x86/compemu_fpp_x86.cpp` | FPU JIT, comp_pc_p assert |
| `cmake/SourceFiles.cmake` | -fno-pie/-no-pie flags |
