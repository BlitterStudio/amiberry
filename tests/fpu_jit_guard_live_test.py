#!/usr/bin/env python3
"""Check the JIT FPU's FPCR entry guard against the interpreter.

The JIT's inline host FPU code assumes the FPCR exception enables (and, on
AArch64, the single rounding precision) are clear; a compiled block checks
the FPCR on entry and leaves for the interpreter's exact helpers when they
are not.  An original synthetic ROM runs, with FPCR values that alternate
between 0 and rounding precisions and exception enables (an overflow enable
and an inexact enable; the 64-bit host core takes no FPU exceptions, so the
enables only make the JIT's entry check fall back to the interpreter):
- a subroutine called through a register, so that one compiled block is
  entered with each FPCR value, past the recompilation limit;
- an FPU-only DBRA loop that branches to itself;
- the same arithmetic inline after each FPCR write.
The arithmetic includes 1 + (2^-24 + 2^-70), which the 64-bit host core
rounds to double precision before single precision (to 1.0).
Before that, a copy of the arithmetic per FPCR value is compiled with FPCR
0 and then called with that FPCR, with 0 and with it again, a few times each;
and a block that writes the FPCR jumps directly (BRA) to a block compiled for
the FPCR it had before; and a subroutine called with single precision
restores a null FPU frame (FPCR 0) before the arithmetic. The JIT must log a
recompilation for the FPCR's mode each time such a block is entered in
another mode: it shows that the blocks ran compiled for the fast mode, and
that the entry check caught the change.
Every result and the FPSR after it must be the same with the JIT as in the
interpreter, and so must the count of FPU exceptions taken (0 with the host
core; kept as a guard).

Usage: python3 tests/fpu_jit_guard_live_test.py <path-to-amiberry>
"""

import os
from pathlib import Path
import platform
import re
import struct
import sys
import tempfile
import time

from ipc_actions_test import HEADLESS_SDL_ENV, Connection, Emulator
from fpu_jit_loads_live_test import RAM_CODE, assemble, make_rom

OUT = 0x20000
DONE = 0x3000
COUNT = 0x3004  # FPU exceptions taken
ITERATIONS = 24
# FPCR values a register-called subroutine and a DBRA self-loop run with:
# single and double precision, an overflow enable, single precision with
# round to zero, and an inexact enable
CALLED = (0, 0x40, 0, 0x80, 0, 0x1000, 0, 0x50, 0, 0x0200, 0)
INLINE = (0x40, 0x80, 0x1000, 0x50, 0x0200, 0)
# 1/3 and sqrt(2) as extended, the FPSR, then 1 + (2^-24 + 2^-70) as extended
CALC_BYTES = 12 + 12 + 4 + 12
LOOP_BYTES = 12 + 4
# copies of the arithmetic, compiled with FPCR 0 and then called with each FPCR
# in turn, CALLS times each: 3 mode changes, below the recompilation limit
WARMED = (0x40, 0x80, 0x1000, 0x50, 0x0200)
# FPCR values a block entered with FPCR 0 writes before it branches to a block
# compiled with FPCR 0 (each a change of mode: without one, the writing block
# runs on into the target)
DIRECT = (0x40, 0x0200, 0x50)
WARM_CALLS = 8
CALLS = 4
NULL_CALLS = 8
OUT2 = 0x28000
SCRATCH = 0x2F000
NULL_FRAME = 0x2F800
RECOMPILE = re.compile(
    r"JIT: FPU block ([0-9a-f]{8}) entered with FPCR ([0-9a-f]{4}), "
    r"recompiled for (\w+) mode \(trip (\d+)\)")
CALLED_SLOT = CALC_BYTES + LOOP_BYTES
FPSR_EXC = 0x0000FF00  # the inline code leaves the exception byte to the next helper


def warmed_sequence(fpcr):
    return (fpcr, 0, fpcr)


def jit_mode(fpcr):
    """fpu_jit_fpcr_mode() of a Linux or Windows host"""
    if fpcr & 0xFF00 or fpcr & 0xC0 == 0xC0:
        return "exact"
    return "single" if fpcr & 0xC0 == 0x40 else "fast"


def expected_recompiles(sequence):
    """(FPCR, mode, trip) of each recompilation of a block compiled with FPCR 0
    and then entered with each FPCR of <sequence>"""
    expected, mode = [], "fast"
    for fpcr in sequence:
        if jit_mode(fpcr) != mode:
            mode = jit_mode(fpcr)
            expected.append((fpcr, mode, len(expected) + 1))
    return expected


def calc():
    return (
        0xF23C, 0x4000, 0, 1,  # FMOVE.L #1,FP0
        0xF23C, 0x4020, 0, 3,  # FDIV.L #3,FP0
        0xF23C, 0x4080, 0, 2,  # FMOVE.L #2,FP1
        0xF200, 0x0484,  # FSQRT.X FP1
        0xF218, 0x6800,  # FMOVE.X FP0,(A0)+
        0xF218, 0x6880,  # FMOVE.X FP1,(A0)+
        0xF218, 0xA800,  # FMOVE.L FPSR,(A0)+
        # rounded to double precision first (the 64-bit host core), a tie at
        # single precision that rounds to even: 1.0, not 1 + 2^-23
        0xF23C, 0x4280, 0, 1,  # FMOVE.L #1,FP5
        0xF23C, 0x4AA2, 0x3FE7, 0, 0x8000, 0, 2, 0,  # FADD.X #2^-24+2^-70,FP5
        0xF218, 0x6A80,  # FMOVE.X FP5,(A0)+
    )


def calc_part(offset):
    if offset >= 28:
        return "1+2^-24+2^-70"
    return ("1/3", "sqrt(2)", "FPSR")[min(offset // 12, 2)]


def self_loop():
    return (
        0xF23C, 0x4100, 0, 1,  # FMOVE.L #1,FP2
        0xF23C, 0x4180, 0, 3,  # FMOVE.L #3,FP3
        0xF23C, 0x4200, 0, 1,  # FMOVE.L #1,FP4
        0x323C, 49,  # MOVE.W #49,D1
        0xF200, 0x0D20,  # loop: FDIV.X FP3,FP2
        0xF200, 0x1122,  # FADD.X FP4,FP2
        0x51C9, 0xFFF6,  # DBRA D1,loop
        0xF218, 0x6900,  # FMOVE.X FP2,(A0)+
        0xF218, 0xA800,  # FMOVE.L FPSR,(A0)+
        0x4E75,  # RTS
    )


def test_code():
    code = []

    def words(*values):
        code.extend(values)

    def long(value):
        words(value >> 16, value & 65535)

    def address(index):
        return RAM_CODE + index * 2

    # the addresses of the handler and subroutines are patched in once known
    handler_refs = []
    for vector in range(48, 55):  # MOVE.L #handler,vector.W
        words(0x21FC)
        handler_refs.append(len(code)); long(0); words(vector * 4)
    words(0x42B9); long(COUNT)  # CLR.L COUNT

    # compile the copies, the BRA target, the BRA itself and the BRA source
    # with FPCR 0: without an FPCR change, the source's block runs on into the
    # target, and blocks start at the BRA and its target only when entered there
    warmed_refs = []
    words(0xF23C, 0x9000); long(0)  # FMOVE.L #0,FPCR
    for _ in WARMED + ("target", "bra", "source"):  # not the null FRESTORE
        words(0x47F9); warmed_refs.append(len(code)); long(0)  # LEA copy,A3
        words(0x7000)  # MOVEQ #0,D0 (the FPCR the BRA source writes)
        words(0x3C3C, WARM_CALLS - 1)  # MOVE.W #calls-1,D6
        warm = len(code)
        words(0x41F9); long(SCRATCH)  # LEA SCRATCH,A0
        words(0x4E93)  # JSR (A3)
        words(0x51CE, (2 * (warm - len(code) - 1)) & 65535)  # DBRA D6,warm
    words(0x45F9); long(OUT2)  # LEA OUT2,A2
    for index, fpcr in enumerate(WARMED):
        for value in warmed_sequence(fpcr):
            for _ in range(CALLS):
                words(0xF23C, 0x9000); long(value)  # FMOVE.L #value,FPCR
                words(0x47F9); warmed_refs.append(len(code)); long(index)  # LEA copy,A3
                words(0x204A)  # MOVEA.L A2,A0
                words(0x4E93)  # JSR (A3)
                words(0xD4FC, CALC_BYTES)  # ADDA.W #bytes,A2
    for value in DIRECT:
        for _ in range(CALLS):
            words(0xF23C, 0x9000); long(0)  # FMOVE.L #0,FPCR
            words(0x203C); long(value)  # MOVE.L #value,D0
            words(0x47F9); warmed_refs.append(len(code)); long(len(WARMED) + 2)  # LEA source,A3
            words(0x204A)
            words(0x4E93)
            words(0xD4FC, CALC_BYTES)
    words(0x43F9); long(NULL_FRAME)  # LEA NULL_FRAME,A1
    words(0x4291)  # CLR.L (A1)
    for _ in range(NULL_CALLS):
        words(0xF23C, 0x9000); long(0x40)  # FMOVE.L #$40,FPCR
        words(0x47F9); warmed_refs.append(len(code)); long(len(WARMED) + 3)  # LEA null,A3
        words(0x204A)
        words(0x4E93)
        words(0xD4FC, CALC_BYTES)
    words(0x47F9); calc_ref = len(code); long(0)  # LEA calc,A3
    words(0x49F9); loop_ref = len(code); long(0)  # LEA self_loop,A4
    words(0x45F9); long(OUT)  # LEA OUT,A2
    words(0x3E3C, ITERATIONS - 1)  # MOVE.W #iterations-1,D7
    top = len(code)
    for fpcr in CALLED:
        words(0xF23C, 0x9000); long(fpcr)  # FMOVE.L #fpcr,FPCR
        words(0x204A)  # MOVEA.L A2,A0
        words(0x4E93)  # JSR (A3)
        words(0x4E94)  # JSR (A4)
        words(0xD4FC, CALLED_SLOT)  # ADDA.W #slot,A2
    for fpcr in INLINE:
        words(0xF23C, 0x9000); long(fpcr)
        words(0x204A)
        words(*calc())
        words(0xD4FC, CALC_BYTES)
    words(0xF23C, 0x9000); long(0)
    words(0x51CF, (2 * (top - len(code) - 1)) & 65535)  # DBRA D7,top
    words(0x23FC, 0x1234, 0x5678); long(DONE)
    words(0x60FE)
    handler = address(len(code))
    words(0xF327)  # FSAVE -(A7)
    words(0x52B9); long(COUNT)  # ADDQ.L #1,COUNT
    words(0xF35F)  # FRESTORE (A7)+
    words(0x4E73)  # RTE
    calc_address = address(len(code))
    words(*calc(), 0x4E75)
    loop_address = address(len(code))
    words(*self_loop())
    copies = []
    for _ in WARMED + ("target",):
        copies.append(address(len(code)))
        words(*calc(), 0x4E75)
    # the BRA source: writes the FPCR and branches to the target copy
    source = address(len(code))
    words(0xF200, 0x9000)  # FMOVE.L D0,FPCR
    copies.append(address(len(code)))
    words(0x6000, (copies[len(WARMED)] - address(len(code)) - 2) & 65535)  # BRA.W target
    copies.append(source)
    # restores a null frame (FPCR 0), then the arithmetic
    copies.append(address(len(code)))
    words(0xF351)  # FRESTORE (A1)
    words(*calc(), 0x4E75)
    for ref, index in zip(warmed_refs, [*range(len(WARMED) + 3)] + [
            code[ref + 1] for ref in warmed_refs[len(WARMED) + 3:]]):
        code[ref:ref + 2] = [copies[index] >> 16, copies[index] & 65535]
    test_code.copies = copies
    for ref, value in [(ref, handler) for ref in handler_refs] + [
            (calc_ref, calc_address), (loop_ref, loop_address)]:
        code[ref:ref + 2] = [value >> 16, value & 65535]
    payload = assemble(code)
    return payload + bytes(-len(payload) % 4)


def results_size():
    return ITERATIONS * (len(CALLED) * CALLED_SLOT + len(INLINE) * CALC_BYTES)


def warmed_size():
    return ((len(WARMED) * len(warmed_sequence(0)) + len(DIRECT)) * CALLS +
            NULL_CALLS) * CALC_BYTES


def describe_warmed(offset):
    entry, offset = divmod(offset, CALC_BYTES)
    nulls = (len(WARMED) * len(warmed_sequence(0)) + len(DIRECT)) * CALLS
    if entry >= nulls:
        what = calc_part(offset)
        return f"null FRESTORE after FPCR=$0040 call {entry - nulls} {what}"
    call = entry % CALLS
    entry //= CALLS
    what = calc_part(offset)
    per_copy = len(warmed_sequence(0))
    if entry < len(WARMED) * per_copy:
        copy, step = divmod(entry, per_copy)
        fpcr = warmed_sequence(WARMED[copy])[step]
        return f"copy ${WARMED[copy]:04x} entered with FPCR=${fpcr:04x} call {call} {what}"
    fpcr = DIRECT[entry - len(WARMED) * per_copy]
    return f"BRA after FPCR=${fpcr:04x} call {call} {what}"


def describe(offset):
    per_iteration = results_size() // ITERATIONS
    iteration, offset = divmod(offset, per_iteration)
    called = len(CALLED) * CALLED_SLOT
    if offset < called:
        entry, offset = divmod(offset, CALLED_SLOT)
        where = f"called FPCR=${CALLED[entry]:04x}"
        if offset >= CALC_BYTES:
            where += " self-loop"
            offset -= CALC_BYTES
            what = "FPSR" if offset >= 12 else "value"
        else:
            what = calc_part(offset)
    else:
        entry, offset = divmod(offset - called, CALC_BYTES)
        where = f"inline FPCR=${INLINE[entry]:04x}"
        what = calc_part(offset)
    return f"iteration {iteration} {where} {what}"


def fpsr_offsets():
    offsets = set()
    per_iteration = results_size() // ITERATIONS
    for iteration in range(ITERATIONS):
        base = iteration * per_iteration
        for entry in range(len(CALLED)):
            slot = base + entry * CALLED_SLOT
            offsets.update((slot + 24, slot + CALC_BYTES + 12))
        base += len(CALLED) * CALLED_SLOT
        offsets.update(base + entry * CALC_BYTES + 24 for entry in range(len(INLINE)))
    return offsets


def run(binary, env, rom, cache):
    emu = Emulator(binary, env, model="A1200", options=(
        f"kickstart_rom_file={rom}", "cpu_model=68020", "fpu_model=68882",
        "cpu_compatible=false", "cpu_memory_cycle_exact=false",
        "cpu_cycle_exact=false", "cpu_speed=max", "cpu_threaded=false",
        "fpu_softfloat=false", "fpu_core=host", "comp_fpu=true", f"cachesize={cache}",
    ))
    control = None
    try:
        control = Connection(emu.endpoint)
        assert "FPU=68882 (host" in emu.text(), emu.text()
        if cache:
            assert f"JIT=CPU/FPU={cache}." in emu.text(), emu.text()
        deadline = time.monotonic() + 60
        while control.request(f"READ_MEM\t{DONE:#x}\t4") != "OK\t305419896":
            if time.monotonic() >= deadline:
                raise AssertionError(f"Guest did not finish (cache={cache})\n{emu.text()}")
            time.sleep(0.05)
        requests = [f"READ_MEM\t{COUNT:#x}\t4"] + [
            f"READ_MEM\t{OUT + offset:#x}\t4" for offset in range(0, results_size(), 4)] + [
            f"READ_MEM\t{OUT2 + offset:#x}\t4" for offset in range(0, warmed_size(), 4)]
        values = []
        for start in range(0, len(requests), 512):
            for reply in control.pipeline(requests[start:start + 512]):
                assert reply.startswith("OK\t"), reply
                values.append(int(reply.split("\t")[1]))
        main_words = results_size() // 4
        return values[0], values[1:1 + main_words], values[1 + main_words:], emu.text()
    finally:
        if control:
            control.close()
        emu.kill()


def main(binary):
    if platform.machine().lower() not in ("x86_64", "amd64", "aarch64", "arm64"):
        # Only the x86-64 and AArch64 JITs have the inline FPU code and its guard.
        print("FPU JIT guard test skipped: not an x86-64 or AArch64 host", flush=True)
        return
    binary = os.path.abspath(binary)
    with tempfile.TemporaryDirectory(prefix="amiberry-fpu-guard-") as directory:
        work = Path(directory)
        rom = work / "guard.rom"
        env = dict(os.environ)
        env.update({
            "HOME": directory, "XDG_RUNTIME_DIR": directory,
            "XDG_CONFIG_HOME": str(work / "config"), "XDG_DATA_HOME": str(work / "data"),
            "AMIBERRY_HOME_DIR": str(work / "home"), "LOCALAPPDATA": str(work / "local"),
            "APPDATA": str(work / "roaming"),
            **HEADLESS_SDL_ENV,
        })
        env.pop("AMIBERRY_CONFIG_DIR", None)
        code = test_code()
        copies = test_code.copies
        rom.write_bytes(make_rom(code))
        count, interpreted, warmed, _ = run(binary, env, rom, 0)
        jit_count, compiled, jit_warmed, jit_log = run(binary, env, rom, 8192)
        # 1/3 rounded to single precision, then to double precision
        single = struct.pack(">HHII", 0x3FFD, 0, 0xAAAAAB00, 0)
        double = struct.pack(">HHII", 0x3FFD, 0, 0xAAAAAAAA, 0xAAAAA800)
        words = struct.pack(f">{len(interpreted)}I", *interpreted)
        assert words[CALLED_SLOT:CALLED_SLOT + 12] == single, words[CALLED_SLOT:CALLED_SLOT + 12].hex()
        assert words[3 * CALLED_SLOT:3 * CALLED_SLOT + 12] == double, words[3 * CALLED_SLOT:][:12].hex()
        # 1 + (2^-24 + 2^-70) at single precision: the 64-bit core rounds to
        # double first, and the JIT must do the same
        tie = words[CALLED_SLOT + 28:CALLED_SLOT + 40]
        assert tie == struct.pack(">HHII", 0x3FFF, 0, 0x80000000, 0), tie.hex()
        # after the null FRESTORE, double precision (FPCR 0)
        nulls = warmed_size() - NULL_CALLS * CALC_BYTES
        warmed_words = struct.pack(f">{len(warmed)}I", *warmed)
        assert warmed_words[nulls:nulls + 12] == double, warmed_words[nulls:nulls + 12].hex()
        fpsr = fpsr_offsets()
        failures = []
        if count != jit_count:
            failures.append(f"FPU exceptions taken: interpreter {count} JIT {jit_count}")
        for index, (want, got) in enumerate(zip(interpreted, compiled)):
            mask = ~FPSR_EXC if index * 4 in fpsr else ~0
            if (want ^ got) & mask:
                failures.append(f"{describe(index * 4)}: interpreter {want:#010x} JIT {got:#010x}")
        for index, (want, got) in enumerate(zip(warmed, jit_warmed)):
            mask = ~FPSR_EXC if index * 4 % CALC_BYTES == 24 else ~0
            if (want ^ got) & mask:
                failures.append(f"{describe_warmed(index * 4)}: interpreter {want:#010x} JIT {got:#010x}")
        # the recompilations of the copies and the BRA target, in order
        recompiles = {}
        for match in RECOMPILE.finditer(jit_log):
            recompiles.setdefault(int(match.group(1), 16), []).append(
                (int(match.group(2), 16), match.group(3), int(match.group(4))))
        assert "further FPU mode recompiles are not logged" not in jit_log
        checked = 0
        for index, fpcr in enumerate(WARMED):
            want = expected_recompiles(warmed_sequence(fpcr))
            got = recompiles.get(copies[index], [])
            checked += len(want)
            if got != want:
                failures.append(f"copy ${fpcr:04x} at {copies[index]:08x}: recompiled {got}, "
                                f"expected {want}")
        # the first block with FPU code after the FPCR write: the BRA's, or the
        # target's if the BRA's block has none
        want = expected_recompiles(DIRECT)
        target, bra = copies[len(WARMED)], copies[len(WARMED) + 1]
        got = {address: recompiles.get(address, []) for address in (bra, target)}
        checked += len(want)
        if sorted(got.values()) != sorted([want, []]):
            failures.append(f"after the BRA source: recompiled {got}, expected {want} at "
                            f"{bra:08x} or {target:08x}")
        assert not failures, "\n".join(failures[:40] + [f"({len(failures)} differences)"])
        print(f"{len(interpreted) + len(warmed)} result words, {count} FPU exceptions and "
              f"{checked} FPU mode recompiles as expected", flush=True)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
