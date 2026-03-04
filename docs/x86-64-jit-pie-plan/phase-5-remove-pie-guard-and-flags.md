# Phase 5: Remove #error Guard and -fno-pie Flags

## Priority: HIGH (the actual user-facing change)

## Prerequisites

Phases 1-4 must be completed first. This phase removes the safety guards that
prevent PIE builds, so the fixes from earlier phases must be in place.

## Files to modify

1. `src/jit/x86/compemu_support_x86.cpp` (lines 106-108)
2. `cmake/SourceFiles.cmake` (line ~563)

## Changes

### 5a: Remove compile-time #error guard

In `src/jit/x86/compemu_support_x86.cpp`, find and remove:

```c
#if defined(__pie__) || defined(__PIE__)
#error Position-independent code (PIE) cannot be used with JIT
#endif
```

### 5b: Update CMake flags

In `cmake/SourceFiles.cmake`, find the block (around line 563):

```cmake
if(NOT ANDROID AND NOT WIN32)
    target_compile_options(${PROJECT_NAME} PRIVATE -fno-pie)
    target_link_options(${PROJECT_NAME} PRIVATE -no-pie)
```

Replace with:

```cmake
if(NOT ANDROID AND NOT WIN32
        AND CMAKE_SYSTEM_NAME STREQUAL "FreeBSD"
        AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")
    # FreeBSD x86-64 JIT uses ADDR32 prefix + MAP_32BIT strategy,
    # requiring all code/data in the low 2GB. PIE would break this.
    # Linux/macOS x86-64 use RIP-relative + anchor-based allocation,
    # which is PIE-compatible.
    target_compile_options(${PROJECT_NAME} PRIVATE -fno-pie)
    target_link_options(${PROJECT_NAME} PRIVATE -no-pie)
```

This keeps `-fno-pie` only for FreeBSD x86-64 (which uses the fundamentally
different ADDR32+MAP_32BIT approach that is incompatible with PIE).

All other platforms — Linux x86-64, macOS x86-64, ARM64 everywhere — will
now build as PIE by default.

## Verification

1. **Linux x86-64:** Build and check binary type:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build -j$(nproc)
   readelf -h build/amiberry | grep Type
   # Should show: Type: DYN (Position-Independent Executable)
   ```

2. **Non-PIE regression:** Verify explicit non-PIE still works:
   ```bash
   cmake -B build-nopie -DCMAKE_BUILD_TYPE=Release \
     -DCMAKE_C_FLAGS="-fno-pie" -DCMAKE_CXX_FLAGS="-fno-pie" \
     -DCMAKE_EXE_LINKER_FLAGS="-no-pie"
   cmake --build build-nopie -j$(nproc)
   ```

3. **ARM64:** Verify ARM64 build is unaffected (no `-fno-pie` was needed)

4. **JIT functional:** Boot A1200 with JIT enabled, run software
