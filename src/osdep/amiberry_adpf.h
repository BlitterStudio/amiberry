#ifndef AMIBERRY_ADPF_H
#define AMIBERRY_ADPF_H

#include <cstdint>

// Android Dynamic Performance Framework (ADPF) Performance Hint integration.
// Every function is a safe no-op when ADPF is unavailable: non-Android builds,
// Android API < 31, or the hint API could not be resolved at runtime.

// Initialise an ADPF hint session bound to the given emulation-thread kernel
// tid (gettid()).  MUST be called on, or with the tid of, the thread that runs
// the emulation.  Returns true iff a session is now active.  Idempotent: any
// prior session is closed first.
bool adpf_init(int32_t emu_tid);

// Report one frame: updates the session's target work duration when it changes,
// then reports the actual work duration.  No-op when no session is active.
// Both values are nanoseconds; each is clamped to >= 1 (the API rejects <= 0).
void adpf_report_frame(int64_t actual_work_ns, int64_t target_ns);

// Close the active session, if any.  Safe to call when inactive.
void adpf_cleanup(void);

#endif // AMIBERRY_ADPF_H
