# PPC QEMU Runtime Troubleshooting

Use this reference for QEMU-UAE PPC runtime behavior, plugin JIT flush slowdowns, PPC spinlock behavior, and execute-loop regressions.

## Core Invariants

- Keep hot PPC execute paths minimal. `uae_ppc_execute_check()` and `uae_ppc_execute_quick()` can run frequently enough that optional plugin calls cause visible slowdowns.
- Do not call QEMU-UAE plugin JIT flush hooks from hot execute paths unless there is a proven coherency bug and a bounded trigger.
- Separate packaging/plugin load failures from runtime performance regressions:
  - packaging belongs in `amiberry-packaging-install`
  - execute-loop and spinlock behavior belongs here
- Treat optional plugin symbols as compatibility surface, not as work that must run every scheduler tick.

## Important Code Paths

- `src/ppc/ppc.cpp`: plugin symbol loading, `using_qemu()`, PPC thread, execute check and quick execute paths.
- `src/include/uae/ppc.h`: public PPC plugin function declarations.
- `docs/ppc-qemu-plugin.md`: packaging and artifact expectations.

## Debugging Checklist

1. Confirm whether the active PPC implementation is QEMU-UAE or another PPC backend.
2. Profile or log around `uae_ppc_execute_check()` and `uae_ppc_execute_quick()` before adding plugin calls there.
3. If a flush is needed, find the precise invalidation event instead of flushing every execute check.
4. Verify spinlock release/reacquire behavior after removing or moving plugin calls.
5. Test with an OS4/PPC workload that previously showed slowdown.

## Common Traps

- Importing optional plugin functions and then calling them unconditionally in hot paths.
- Solving a stale-code hypothesis with a global flush before proving stale QEMU translated blocks exist.
- Mixing CI artifact dependency assumptions with runtime scheduler changes.
- Removing a plugin symbol from the loader but leaving stale declarations in headers.
