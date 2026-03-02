# Step 04: Threading and Synchronization Migration

## Objective

Migrate all SDL threading primitives (threads, mutexes, semaphores, condition variables) from SDL2 to SDL3 naming and semantics. This step is critical because `src/threaddep/thread.h` defines macros used throughout the entire emulator codebase.

## Prerequisites

- Step 03 complete (timer and utilities migrated)

## Files to Modify

| File | Role | Changes |
|------|------|---------|
| `src/threaddep/thread.h` | Central threading macros | Type renames, function renames in macros |
| `src/threaddep/threading.cpp` | Thread creation/management | `SDL_CreateThread`, `SDL_DetachThread`, `SDL_WaitThread` |
| `src/osdep/amiberry.cpp` | Condition variables, mutexes | `SDL_CreateCond` → `SDL_CreateCondition`, etc. |
| `src/osdep/clipboard.cpp` | Mutex for clipboard access | `SDL_CreateMutex`, `SDL_LockMutex`, `SDL_UnlockMutex` |
| `src/osdep/bsdsocket_host.cpp` | Extensive mutex usage (~20 sites) | All mutex operations |

## Detailed Changes

### 1. Semaphore API (Critical — `thread.h`)

The file `src/threaddep/thread.h` contains `#define` macros that wrap SDL semaphore operations. These macros are used throughout the entire UAE emulator core.

**Type rename**:
```cpp
// Before:
typedef SDL_sem *uae_sem_t;

// After:
typedef SDL_Semaphore *uae_sem_t;
```

**Function renames** in the macros:
| SDL2 Macro Body | SDL3 Macro Body |
|----------------|----------------|
| `SDL_CreateSemaphore(n)` | `SDL_CreateSemaphore(n)` (unchanged) |
| `SDL_DestroySemaphore(s)` | `SDL_DestroySemaphore(s)` (unchanged) |
| `SDL_SemPost(s)` | `SDL_SignalSemaphore(s)` |
| `SDL_SemWait(s)` | `SDL_WaitSemaphore(s)` |
| `SDL_SemTryWait(s)` | `SDL_TryWaitSemaphore(s)` |
| `SDL_SemWaitTimeout(s, ms)` | `SDL_WaitSemaphoreTimeout(s, ms)` |
| `SDL_SemValue(s)` | `SDL_GetSemaphoreValue(s)` |

**Example macro update**:
```cpp
// Before:
#define uae_sem_post(PSEM) SDL_SemPost(*(PSEM))
#define uae_sem_wait(PSEM) SDL_SemWait(*(PSEM))
#define uae_sem_trywait(PSEM) SDL_SemTryWait(*(PSEM))
#define uae_sem_trywait_delay(PSEM, MS) SDL_SemWaitTimeout(*(PSEM), MS)
#define uae_sem_getvalue(PSEM) SDL_SemValue(*(PSEM))

// After:
#define uae_sem_post(PSEM) SDL_SignalSemaphore(*(PSEM))
#define uae_sem_wait(PSEM) SDL_WaitSemaphore(*(PSEM))
#define uae_sem_trywait(PSEM) SDL_TryWaitSemaphore(*(PSEM))
#define uae_sem_trywait_delay(PSEM, MS) SDL_WaitSemaphoreTimeout(*(PSEM), MS)
#define uae_sem_getvalue(PSEM) SDL_GetSemaphoreValue(*(PSEM))
```

**Return type change**: SDL3 `SDL_WaitSemaphore()` and similar return `bool` (true on success) instead of `int` (0 on success). Update any code that checks `== 0` for success to check `== true` or just use the bool directly.

### 2. Mutex API

**Type rename**:
```cpp
// Before:
SDL_mutex*

// After:
SDL_Mutex*
```

**Function changes**:
| SDL2 | SDL3 | Return Type Change |
|------|------|--------------------|
| `SDL_CreateMutex()` | `SDL_CreateMutex()` | Same (returns pointer) |
| `SDL_LockMutex(m)` | `SDL_LockMutex(m)` | `int` → `void` |
| `SDL_TryLockMutex(m)` | `SDL_TryLockMutex(m)` | `int` → `bool` (true = locked) |
| `SDL_UnlockMutex(m)` | `SDL_UnlockMutex(m)` | `int` → `void` |
| `SDL_DestroyMutex(m)` | `SDL_DestroyMutex(m)` | Same |

**Critical**: `SDL_LockMutex()` and `SDL_UnlockMutex()` return `void` in SDL3 (they were `int` in SDL2). Remove any return value checks:
```cpp
// Before (if checking return):
if (SDL_LockMutex(m) == 0) { ... }

// After:
SDL_LockMutex(m);
// Always succeeds (asserts on invalid mutex in debug builds)
```

**Key locations** for mutex changes:
- `bsdsocket_host.cpp`: ~20 call sites (heaviest user)
- `clipboard.cpp`: clipboard mutex
- `amiberry.cpp`: various synchronization points

### 3. Condition Variable API

**Type rename**:
```cpp
// Before:
SDL_cond*

// After:
SDL_Condition*
```

**Function renames**:
| SDL2 | SDL3 |
|------|------|
| `SDL_CreateCond()` | `SDL_CreateCondition()` |
| `SDL_DestroyCond(c)` | `SDL_DestroyCondition(c)` |
| `SDL_CondSignal(c)` | `SDL_SignalCondition(c)` |
| `SDL_CondBroadcast(c)` | `SDL_BroadcastCondition(c)` |
| `SDL_CondWait(c, m)` | `SDL_WaitCondition(c, m)` |
| `SDL_CondWaitTimeout(c, m, ms)` | `SDL_WaitConditionTimeout(c, m, ms)` |

**Return type changes**: `SDL_WaitCondition()` returns `void` (was `int`). `SDL_WaitConditionTimeout()` returns `bool` (was `int`).

**Key locations**:
- `amiberry.cpp`: GUI synchronization with emulation thread

### 4. Thread API

**Function changes**:
| SDL2 | SDL3 | Notes |
|------|------|-------|
| `SDL_CreateThread(fn, name, data)` | `SDL_CreateThread(fn, name, data)` | Same signature |
| `SDL_WaitThread(t, &status)` | `SDL_WaitThread(t, &status)` | Same |
| `SDL_DetachThread(t)` | `SDL_DetachThread(t)` | Same |
| `SDL_GetThreadID(t)` | `SDL_GetThreadID(t)` | Same |
| `SDL_threadID` | `SDL_ThreadID` | Type rename |
| `SDL_SetThreadPriority(p)` | `SDL_SetCurrentThreadPriority(p)` | Renamed |

**Thread priority enum**:
| SDL2 | SDL3 |
|------|------|
| `SDL_THREAD_PRIORITY_LOW` | `SDL_THREAD_PRIORITY_LOW` |
| `SDL_THREAD_PRIORITY_NORMAL` | `SDL_THREAD_PRIORITY_NORMAL` |
| `SDL_THREAD_PRIORITY_HIGH` | `SDL_THREAD_PRIORITY_HIGH` |
| `SDL_THREAD_PRIORITY_TIME_CRITICAL` | `SDL_THREAD_PRIORITY_TIME_CRITICAL` |

**Key locations**:
- `threading.cpp`: thread creation for CPU, sound, and other subsystems

## Search Patterns

```bash
# Semaphores
grep -rn 'SDL_sem\b' src/
grep -rn 'SDL_SemPost\|SDL_SemWait\|SDL_SemTryWait\|SDL_SemValue\|SDL_SemWaitTimeout' src/

# Mutexes
grep -rn 'SDL_mutex\b' src/
grep -rn 'SDL_CreateMutex\|SDL_LockMutex\|SDL_UnlockMutex\|SDL_TryLockMutex\|SDL_DestroyMutex' src/

# Conditions
grep -rn 'SDL_cond\b' src/
grep -rn 'SDL_CreateCond\|SDL_CondSignal\|SDL_CondWait\|SDL_CondBroadcast\|SDL_DestroyCond' src/

# Threads
grep -rn 'SDL_threadID\b' src/
grep -rn 'SDL_SetThreadPriority' src/
```

## Acceptance Criteria

1. `thread.h` macros compile and use SDL3 semaphore/mutex/condition APIs
2. All `SDL_sem` → `SDL_Semaphore`, `SDL_mutex` → `SDL_Mutex`, `SDL_cond` → `SDL_Condition`
3. All renamed functions updated (`SDL_SemPost` → `SDL_SignalSemaphore`, etc.)
4. Return type changes handled (remove checks for `int` return from `SDL_LockMutex`/`SDL_UnlockMutex`)
5. Emulator starts without deadlocks or thread-related crashes
6. CPU emulation, sound, and GUI threads all function correctly
7. `bsdsocket_host.cpp` mutex operations work (network emulation functional)
