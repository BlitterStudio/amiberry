# Threaded Device Shutdown Troubleshooting

Use this reference for shutdown hangs, blocked worker threads, PCAP/uaenet teardown, and PCem Voodoo FIFO/render thread regressions.

## Core Invariants

- Amiberry's SDL helper threads are joinable. Device workers must observe teardown and return before close joins them.
- Every wait loop must have a shutdown escape condition.
- Teardown must wake every event a worker may be blocked on before joining or killing the worker handle.
- Worker busy flags should be cleared on normal exit and shutdown exit.
- For PCAP workers, request shutdown through both local active flags and `pcap_breakloop()` when a capture handle exists.

## Important Code Paths

- `src/osdep/amiberry_uaenet.cpp`: `uaenet_request_worker_stop()`, PCAP loop, reopen and close paths.
- `src/pcem/vid_voodoo.cpp`: worker creation, `thread_run` state, close-time wakeups, event destruction.
- `src/pcem/vid_voodoo_fifo.cpp`: command FIFO waits and packet reads.
- `src/pcem/vid_voodoo_render.cpp` and `.h`: render queue waits and render-thread idle waits.

## Debugging Checklist

1. Find every place the worker can block on an event, external API, FIFO, or ring buffer.
2. Add or verify a shared teardown flag that is set before joins.
3. Wake all events that could unblock FIFO, render, "not full", and external wait paths.
4. Make blocking helper functions return failure on teardown instead of fabricating zero values.
5. Ensure close destroys all events that init created, including optional thread slots.
6. Reopen the same device after close to verify stale thread state is not reused.

## Common Traps

- Setting an active flag without waking the event the worker is sleeping on.
- Waiting for command FIFO data forever during device close.
- Destroying only events used by the common thread count and leaking optional 4-thread events.
- Treating `thread_kill()` as a substitute for making the worker return cleanly.
- Modifying vendored PCem code without marking the Amiberry-local shutdown delta.
