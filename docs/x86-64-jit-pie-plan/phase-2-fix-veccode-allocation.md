# Phase 2: Fix veccode Allocation Strategy

## Priority: HIGH (required for PIE)

## Problem

In `src/jit/x86/exception_handler.cpp`, the veccode trampoline is allocated
with `UAE_VM_32BIT` (maps to `MAP_32BIT` on Linux), forcing it into the low
2GB:

```c
veccode = (uae_u8 *) uae_vm_alloc(256, UAE_VM_32BIT, UAE_VM_READ_WRITE_EXECUTE);
```

The veccode trampoline generates RIP-relative references to globals (e.g.,
`regs`, exception handler pointers). Under PIE, globals are not in the low 2GB,
so veccode must be placed near `.data` instead of in the low 2GB.

## Files to modify

- `src/jit/x86/exception_handler.cpp` (~line 890)

## Approach

Replace the `UAE_VM_32BIT` allocation with the anchor-based allocation strategy
already used by the JIT cache allocator in `compemu_support_x86.cpp`.

### Option A: Use vm_acquire from compemu_support_x86.cpp

The function `vm_acquire` in `compemu_support_x86.cpp` (lines ~210-269)
already implements a probe loop that allocates memory within ±1.75GB of a
`.data` anchor. If this function can be exposed (declare in a header or make
non-static), veccode can use it:

```c
// In exception_handler.cpp
veccode = (uae_u8 *) vm_acquire(256, UAE_VM_READ_WRITE_EXECUTE);
```

### Option B: Allocate veccode from the JIT cache region

Since veccode is only 256 bytes, it could be carved from the beginning or end
of the JIT cache allocation (which is already correctly placed by the anchor
allocator). This avoids needing a separate allocation.

### Option C: Inline the probe logic

If neither Option A nor B is feasible, inline a small version of the probe loop:

```c
static uae_u8 data_anchor_eh;

static void *alloc_near_data(size_t size, int prot)
{
    uintptr_t anchor = (uintptr_t)&data_anchor_eh;
    for (intptr_t offset = 0; offset < 0x70000000; offset += 0x200000) {
        for (int sign = 1; sign >= -1; sign -= 2) {
            uintptr_t try_addr = anchor + sign * offset;
            try_addr &= ~(uintptr_t)0xFFF; // page-align
            void *p = mmap((void *)try_addr, size, prot,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (p != MAP_FAILED) {
                intptr_t dist = (intptr_t)p - (intptr_t)anchor;
                if (llabs(dist) < 0x70000000)
                    return p;
                munmap(p, size);
            }
        }
    }
    return NULL;
}
```

### Recommended: Option A

Option A is cleanest. Steps:
1. In `compemu_support_x86.cpp`, remove `static` from `vm_acquire` (or rename
   to `jit_vm_acquire` to avoid naming conflicts)
2. Declare it in a header (e.g., `compemu_support_x86.h` or a new
   `jit_alloc.h`)
3. In `exception_handler.cpp`, call it instead of `uae_vm_alloc`

## Timing concern

`veccode` is allocated in `exception_handler.cpp` (likely during early JIT
init). The anchor allocator in `compemu_support_x86.cpp` is also used during
JIT init. Verify that veccode allocation happens at a point where the anchor
strategy is available. If veccode is allocated before the JIT cache, the anchor
variable still works (it's a static global — always at its final address).

## Verification

- Verify veccode is allocated near globals: add a log line:
  ```c
  write_log("veccode=%p regs=%p dist=%lld\n", veccode, &regs,
            (long long)((intptr_t)veccode - (intptr_t)&regs));
  ```
- Distance should be well under 2GB
- Run with JIT enabled, trigger exception paths (access unmapped Amiga memory)
