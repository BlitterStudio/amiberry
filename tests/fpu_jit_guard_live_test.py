#!/usr/bin/env python3
"""Check the JIT FPU's FPCR entry guard against the interpreter.

The JIT's inline host FPU code assumes the FPCR exception enables (and, on
AArch64, the single rounding precision) are clear; a compiled block checks
the FPCR on entry and leaves for the interpreter's exact helpers when they
are not.  An original synthetic ROM runs, with FPCR values that alternate
between 0 and rounding precisions, an exception enable that never fires and
an inexact enable that fires every operation (with the 80-bit host core):
- a subroutine called through a register, so that one compiled block is
  entered with each FPCR value, past the recompilation limit;
- an FPU-only DBRA loop that branches to itself;
- the same arithmetic inline after each FPCR write.
Every result, the FPSR after it and the count of FPU exceptions taken must
be the same with the JIT as in the interpreter.

Usage: python3 tests/fpu_jit_guard_live_test.py <path-to-amiberry>
"""

import os
from pathlib import Path
import platform
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
# single and double precision, an overflow enable that does not fire, single
# precision with round to zero, and an inexact enable that fires
CALLED = (0, 0x40, 0, 0x80, 0, 0x1000, 0, 0x50, 0, 0x0200, 0)
INLINE = (0x40, 0x80, 0x1000, 0x50, 0x0200, 0)
CALC_BYTES = 12 + 12 + 4  # 1/3 and sqrt(2) as extended, then the FPSR
LOOP_BYTES = 12 + 4
CALLED_SLOT = CALC_BYTES + LOOP_BYTES
FPSR_EXC = 0x0000FF00  # the inline code leaves the exception byte to the next helper


def calc():
    return (
        0xF23C, 0x4000, 0, 1,  # FMOVE.L #1,FP0
        0xF23C, 0x4020, 0, 3,  # FDIV.L #3,FP0
        0xF23C, 0x4080, 0, 2,  # FMOVE.L #2,FP1
        0xF200, 0x0484,  # FSQRT.X FP1
        0xF218, 0x6800,  # FMOVE.X FP0,(A0)+
        0xF218, 0x6880,  # FMOVE.X FP1,(A0)+
        0xF218, 0xA800,  # FMOVE.L FPSR,(A0)+
    )


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
    for ref, value in [(ref, handler) for ref in handler_refs] + [
            (calc_ref, calc_address), (loop_ref, loop_address)]:
        code[ref:ref + 2] = [value >> 16, value & 65535]
    payload = assemble(code)
    return payload + bytes(-len(payload) % 4)


def results_size():
    return ITERATIONS * (len(CALLED) * CALLED_SLOT + len(INLINE) * CALC_BYTES)


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
            what = ("1/3", "sqrt(2)", "FPSR")[min(offset // 12, 2)]
    else:
        entry, offset = divmod(offset - called, CALC_BYTES)
        where = f"inline FPCR=${INLINE[entry]:04x}"
        what = ("1/3", "sqrt(2)", "FPSR")[min(offset // 12, 2)]
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
            f"READ_MEM\t{OUT + offset:#x}\t4" for offset in range(0, results_size(), 4)]
        values = []
        for start in range(0, len(requests), 512):
            for reply in control.pipeline(requests[start:start + 512]):
                assert reply.startswith("OK\t"), reply
                values.append(int(reply.split("\t")[1]))
        return values[0], values[1:], emu.text()
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
        rom.write_bytes(make_rom(test_code()))
        env = dict(os.environ)
        env.update({
            "HOME": directory, "XDG_RUNTIME_DIR": directory,
            "XDG_CONFIG_HOME": str(work / "config"), "XDG_DATA_HOME": str(work / "data"),
            "AMIBERRY_HOME_DIR": str(work / "home"), "LOCALAPPDATA": str(work / "local"),
            "APPDATA": str(work / "roaming"),
            **HEADLESS_SDL_ENV,
        })
        env.pop("AMIBERRY_CONFIG_DIR", None)
        count, interpreted, log = run(binary, env, rom, 0)
        jit_count, compiled, _ = run(binary, env, rom, 8192)
        # 1/3 rounded to single precision, then to double precision
        single = struct.pack(">HHII", 0x3FFD, 0, 0xAAAAAB00, 0)
        double = struct.pack(">HHII", 0x3FFD, 0, 0xAAAAAAAA, 0xAAAAA800)
        words = struct.pack(f">{len(interpreted)}I", *interpreted)
        assert words[CALLED_SLOT:CALLED_SLOT + 12] == single, words[CALLED_SLOT:CALLED_SLOT + 12].hex()
        assert words[3 * CALLED_SLOT:3 * CALLED_SLOT + 12] == double, words[3 * CALLED_SLOT:][:12].hex()
        # only the 80-bit host core takes FPU exceptions; the 64-bit one has none
        if "FPU=68882 (host 80b" in log:
            assert count > 0, "the inexact enable took no exception"
        fpsr = fpsr_offsets()
        failures = []
        if count != jit_count:
            failures.append(f"FPU exceptions taken: interpreter {count} JIT {jit_count}")
        for index, (want, got) in enumerate(zip(interpreted, compiled)):
            mask = ~FPSR_EXC if index * 4 in fpsr else ~0
            if (want ^ got) & mask:
                failures.append(f"{describe(index * 4)}: interpreter {want:#010x} JIT {got:#010x}")
        assert not failures, "\n".join(failures[:40] + [f"({len(failures)} differences)"])
        print(f"{len(interpreted)} result words and {count} FPU exceptions equal in the "
              f"interpreter and JIT", flush=True)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
