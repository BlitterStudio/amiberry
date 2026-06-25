#ifndef AMIBERRY_ADPF_H
#define AMIBERRY_ADPF_H

#include <cstdint>

// Android Dynamic Performance Framework (ADPF) Performance Hint integration.
// Every function is a safe no-op when ADPF is unavailable: non-Android builds,
// Android API < 31, or the hint API could not be resolved at runtime.

// Register a frame-critical thread (its kernel tid, gettid()) with the ADPF
// hint session.  The first registered thread creates the session; later threads
// are added to it (the session is rebuilt covering the union of tids).  This
// lets every thread doing per-frame work - the main thread and, when
// cpu_threaded is enabled, the dedicated CPU thread - be boosted together.
// Returns true iff the calling thread is now covered by an active session, so
// the caller can fall back to static tuning when it is not.  Thread-safe.
bool adpf_register_thread(int32_t tid);

// Report one frame: updates the session's target work duration when it changes,
// then reports the actual work duration.  No-op when no session is active.
// Both values are nanoseconds; each is clamped to >= 1 (the API rejects <= 0).
void adpf_report_frame(int64_t actual_work_ns, int64_t target_ns);

// Close the active session, if any.  Safe to call when inactive.
void adpf_cleanup(void);

#endif // AMIBERRY_ADPF_H
