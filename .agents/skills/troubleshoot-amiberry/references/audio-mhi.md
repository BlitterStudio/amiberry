# MHI Audio Troubleshooting

Use this reference for `src/osdep/mhi_host.cpp`, MHI MP3 playback, mpg123 decode behavior, audio underflow, token completion, and playback drain regressions.

## Core Invariants

- mpg123 can return more decoded PCM frames than the MHI PCM ring has space for.
- Preserve overflow decoded PCM in a pending buffer and drain it before feeding more MP3 data.
- Do not complete decode tokens when encoded bytes are merely copied out of the input queue. Complete them after decoded output tied to those tokens has been accepted or drained.
- `out_of_data` should mean playback is fully drained: no encoded bytes, no ring PCM, no pending PCM, no pending decode tokens, no decoder drain state, and no active decode operation.
- Signal the Amiga task when meaningful completion or out-of-data state changes happen, not from every intermediate queue movement.

## Important Code Paths

- `MhiHostSession`: queue state, pending PCM, pending decode tokens, drain flags, and playback flags.
- `push_pcm_frames()` / `flush_pending_pcm_locked()`: ring writes and pending PCM drain.
- `copy_encoded_chunk()`: moves encoded data to the decoder worker and records tokens.
- `decode_chunk()`: feeds mpg123 and transfers decoded frames into PCM queues.
- `pop_pcm_frame()`: audio callback path where final drain and `out_of_data` are observed.

## Debugging Checklist

1. Reproduce with a stream that can produce decoder output bursts larger than ring free space.
2. Check whether pending PCM is flushed before any new encoded input is decoded.
3. Confirm token completion is delayed until after decoded PCM is accepted or fully drained.
4. Confirm `decoder_busy` or equivalent state prevents false out-of-data while decode is in progress.
5. Test pause, resume, stop, and free while pending PCM or decode tokens exist.
6. Verify `mhi_host_query()` advertises only features actually supported by the host implementation.

## Common Traps

- Dropping overflow decoded PCM when the ring is full.
- Marking encoded tokens complete before their decoded audio can play.
- Setting `out_of_data` while pending PCM still exists.
- Sleeping the decode worker even though pending PCM can now be flushed.
- Holding decoder and session locks in an order that can deadlock teardown.
