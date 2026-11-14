# Multi-threaded CPU

Amiberry runs the 68k interpreter or JIT on a worker thread of its own while
the main emulation thread runs the chipset, when the CPU speed is "fastest
possible", the CPU is a 68020 or later and the host has at least two logical
CPUs. The choice is made automatically and re-checked every frame, so a
configuration change takes effect on the next frame without a restart.

Threaded: 68020 to 68060 at the fastest speed, with the JIT or the
interpreter, More compatible, MMU and any FPU.

Emulation stays on the main thread (single-threaded, as before) when:

- the CPU is a 68000 or 68010;
- the CPU speed is not "fastest possible" (real or another approximate speed);
- cycle-exact or memory cycle-exact CPU emulation is selected;
- beam racing ("low latency vsync") is active;
- PPC emulation is enabled;
- a freezer cartridge (Action Replay and similar) is active;
- the GDB server is enabled;
- the process may run on a single host CPU (counted from its affinity mask and,
  on Linux, a cgroup v2 CPU quota, as a container, a VM or `taskset` can leave
  one), or the worker thread cannot be started. Two threads sharing one CPU
  would make every hand-off wait for the scheduler; the log reports the count
  ("CPU thread: N usable host CPU(s)");
- the guest floods the hardware with accesses (see [Request floods](#request-floods)).

At an approximate speed a CPU thread would only add host CPU use, and 68000
software tends to depend on the CPU's timing. The log reports `CPU emulation
runs on the CPU thread.` or `... main thread.` when it changes.

The "Multi-threaded CPU" checkbox on the CPU panel (config option
`cpu_threaded`, default `true`) turns the CPU thread off. It is available only
in the configurations above that can use it.

## Pacing

Every hardware access (custom chips, CIAs, chip RAM through the chipset banks,
autoconfig boards) is served on the main thread, at the time it is made. The
two threads are paced so that the chipset never runs far ahead of the CPU, and
the CPU never runs far ahead of the chipset:

- The worker publishes progress (instructions executed, JIT block cycles,
  hardware accesses served) and the chipset spends it as time. When the worker shows no progress (a JIT compile, a host deschedule),
  the chipset may run on at most 64 colour clocks of credit before it waits,
  which bounds how far a CPU timing measurement can be off.
- **STOP**: the chipset runs freely while the CPU is stopped. When an
  interrupt that will end the STOP is raised, the main thread waits for the
  worker to take it, so it is not delivered late even when the host keeps the
  worker off a core for a few milliseconds (the wait gives up only after
  100 ms, against a worker that will not leave STOP). A stopped
  worker polls for 50 µs before it sleeps, because interrupts tend to follow
  one another closely, but an idle AmigaOS stops and is woken over a thousand
  times a second.

The CPU loop paces the frame. While the chipset waits for the worker, it
answers each hardware access at once, rather than after a polling back-off.

The main thread can serve several worker accesses at the same colour clock.
A change that single-threaded emulation applies one cycle later as a
last-write value (AGA FMODE) would then be applied out of order, so threaded,
delayed FMODE writes replace one another.

A nasty blit (DMACON BLTPRI set) keeps the CPU off the bus until the blit
ends. Single-threaded, the CPU loop waits for it. Threaded, the main thread
holds the worker's next hardware access, or the reply to the write that
started the blit, until the blitter is done. Track loaders that start a blit
and use its result without waiting for the blitter depend on this.

Host CPU use: the worker runs the CPU as fast as it can, as single-threaded
"fastest possible" emulation does, and the chipset runs on a second core.
The main thread polls while the worker uses the hardware and sleeps in the
frame limiter otherwise. An idle Workbench, whose CPU waits in STOP, uses less
host CPU than single-threaded "fastest possible" emulation; a guest that
busy-polls the hardware keeps both cores busy.

Pausing parks the worker at an instruction or JIT-dispatch boundary, including
when the guest is in STOP. Bounded JIT recording/recompilation already in progress
finishes before parking: abandoning it after its hot-block counter expires would
corrupt the next compilation attempt. Unbounded interpreted JIT fallback loops
instead yield at instruction boundaries. Saving a state or rewind capture also parks the
worker while copying CPU state and RAM. The native debugger and the IPC
`READ_MEM`, `WRITE_MEM`, `GET_CPU_REGS` and `DISASSEMBLE` commands use the same
coordination.
Separate IPC requests can observe different points in execution unless the
emulator remains paused. IPC commands that change debugger tables or remap
memory banks require `PAUSE` (see [IPC](ipc.md)). Enabling the GDB server
keeps emulation on the main thread.

Hardware-access exceptions are returned to the worker for guest exception
handling. A reset or mode change completes an outstanding hardware transaction
before stopping the worker.

While the worker is accessing hardware, both threads poll for each other so
that an access does not wait for a thread wake-up. After about a millisecond
without hardware accesses, the main thread sleeps in the frame limiter instead
of spinning. It also naps through waits shorter than a millisecond (a line
at a time while the worker reads the beam position from the snapshot), on a
condition variable with a nanosecond timeout; a request ends the nap at once,
as it ends a longer sleep. A worker that
has waited 0.1 ms for a reply also sleeps until woken. The handshake is
sequentially consistent: a request or reply is stored before the other
thread's sleeping flag is read, and a thread that goes to sleep sets its flag
before it reads the mailbox, so a wake-up cannot be lost. The other thread is
signalled only when it has announced that it sleeps.
Pause requests raise a special flag, as debugger breaks already do.
The interpreter's per-instruction check therefore stays a single test.

Compiled JIT code checks for work only when its cycle countdown runs out, and
in threaded mode that countdown also tells the chipset how far the CPU has
got. Raising an interrupt, trace or pause flag therefore "kicks" the
countdown: it is swapped for a large negative value, and the value it
replaced is saved, so the worker leaves compiled code at the end of the
current block without losing any executed cycles. The worker, compiled code
included, updates the countdown without a lock and can overwrite a kick. The
main thread kicks again while such flags remain pending.

Pauses are often short (one IPC memory read, for example), so both threads
poll for up to 50 µs while parking and resuming before they sleep. A worker
sleeping in STOP or in the debugger is already at an instruction boundary: a
pause takes it over where it is, without waking it. If an interrupt wakes it
during the pause, it waits for the resume before it continues.

A pause holds chipset time still, so an access the worker makes between two
pauses would be served at the same colour clock as the one before it. A guest
polling SERDATR and writing SERDAT would then overwrite a byte that the
chipset has not yet moved to the shift register. To prevent this, a resume
inside the chipset loop is deferred until the loop next advances time. All
pauses taken in between (for example, a run of IPC commands handled in one
event) share one park.

## Request floods

Each hardware access the snapshot cannot answer costs a round trip between the
threads. A guest that makes thousands of them per frame, a CIA write loop or a
demo polling custom registers in a tight loop, can then run slower threaded
than unthreaded, and the frames run long. When at least 8 of 10 frames
overrun by more than 4% and the worker made at least 4000 hardware requests
per frame, the CPU moves to the main thread. The first 2 s on the thread
(JIT compilation, display set-up) are not measured.

While held on the main thread, the CPU's memory banks count the accesses that
would have been requests, for 25 frames at a time (counting slows a custom
register read by up to a quarter, so it is not left on). The first count comes
a second into the hold, so that a short flood ends a short hold; while the
flood lasts, the counts come at doubling intervals up to 10 s. The CPU returns
to its thread when a count averages fewer than 2000 per frame, half the rate
that starts a hold, so a workload near the limit does not move back and forth
(each move flushes the JIT cache). If a retry's very first measured window is
a flood again, the count on the main thread missed what the thread sees: each
such re-flood doubles the wait for the next hold's first count, until the
hold runs to its safety net, and the log says so. A flood that comes back
after a quiet window on the thread is a new flood. A safety net returns it
anyway after 16 s, after the first counts; each new flood doubles this, up to
256 s, and a minute on the thread without one resets it. A reset,
a state load, a CPU setting change or warp mode forgets the flood. The log
reports each move.

## Register snapshot

A hardware access served on the main thread costs a cross-core round trip,
which is slower than an unthreaded register read. Guests that
poll the beam position, interrupt or DMA state, or the mouse/joystick fire
buttons would run slower threaded than unthreaded. The worker
therefore reads a few side-effect-free registers from a snapshot that the main
thread publishes:

- VPOSR and VHPOSR, republished every colour clock;
- DMACONR, INTENAR, INTREQR and ADKCONR, republished when they change and at
  least every 128 colour clocks;
- CIA-A PRA (byte reads of `$BFE001` only), recomputed every 16 colour clocks.

A group of registers is published only after the worker has read one of them
through the main thread, and stops about a frame (65536 colour clocks) after
that, so guests that never poll pay nothing. A stopped group falls back to
the main thread, which re-arms it.

The snapshot is a sequence lock: the worker never sees a torn or partly
updated group. A read that catches a group mid-write reads it again, a few
times, before it makes a round trip: the main thread rewrites the beam
position every colour clock, and a beam polling loop would otherwise send a
few percent of its reads through the mailbox (about 4,500 per frame on the
Jim Power title screen), which the counts on the main thread during a flood
hold never see. It stays coherent with the CPU's own accesses because every
access served on the main thread republishes the armed groups before the
reply, and a resume after a pause or a reset republishes them before the worker
continues. When the worker takes an interrupt, it does not use the interrupt
registers until the main thread has published them again, so the interrupt
handler does not see INTREQR as it was before the interrupt. Word and
longword reads at odd addresses, mirrored CIA addresses, custom register
mirrors below `$DF8000` on Fat Gary chipsets, debugger memory watchpoints,
dongles and input recording/playback all use the main thread.

The main thread waits for real time to catch up at intervals
of 32 lines, and the chipset time stands still meanwhile while the CPU thread
runs on. While the snapshot is in use, it waits after every line instead, as
unthreaded emulation runs the CPU for at most a line of host time at one
chipset time. Otherwise a guest that polls a register with an iteration bound
(waiting for the blitter, for example) could run out of iterations before the
chipset moves on.

The main thread also stops the chipset while it renders a frame or when the
host deschedules it. If the chipset time has not moved for 128 µs of host time,
about two lines, the CPU thread reads through the main thread again until it
does.

## Diagnostics

Enable optional diagnostics before starting Amiberry:

```sh
AMIBERRY_CPU_THREAD_DIAGNOSTICS=1 amiberry
```

Accepted enabled values are `1`, `true`, `yes` and `on`; disabled values are
`0`, `false`, `no` and `off`. The default is disabled. Other values produce
a diagnostic and leave collection disabled.

On normal exit from threaded emulation, the log reports mailbox wait count,
total elapsed host time and maximum wait in nanoseconds. Skipped guest-cycle
counts are reported separately: guest cycles are not measurements of elapsed
host time. Diagnostics add timing/counter overhead, so leave them disabled
when measuring normal throughput.

## Regression checks

ROM-free synchronization/accounting and diagnostics tests:

```sh
sh tests/test_amitorture_regressions.sh
python3 -m unittest discover -s tests -p test_cpu_thread_diagnostics.py -v
```

The following live test generates its own synthetic ROMs and isolates emulator
state in temporary directories. It covers pause/resume, running and paused
snapshots, reset/shutdown, STOP, warmed-block trace exceptions and delegated
hardware bus errors, with threaded/non-threaded interpreter and JIT controls:

```sh
python3 tests/cpu_thread_live_test.py build/amiberry
```

The live matrix requires a standalone JIT-enabled build with IPC support.
