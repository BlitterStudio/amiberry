#!/usr/bin/env python3
"""Check that a savestate keeps the accrued exceptions of compiled FPU code.

The JITs' inline host FPU code leaves its exceptions in the host FPU's sticky
flags (the x87 status word on x86-64, the FPSR on AArch64), and the 68k FPSR
gets them only when it is read. An original synthetic ROM divides by zero in
a compiled block after it has cleared the FPSR, then waits without touching
the FPU. A savestate saved then must have the accrued divide-by-zero bit in
its FPSR, with the CPU on its own thread (which owns the x87) and without.

Usage: python3 tests/fpu_jit_savestate_live_test.py <path-to-amiberry>
"""

import gzip
import os
from pathlib import Path
import platform
import struct
import sys
import tempfile
import time

from ipc_actions_test import HEADLESS_SDL_ENV, Connection, Emulator
from fpu_jit_loads_live_test import RAM_CODE, assemble, make_rom

READY = 0x3000
FPSR_AE_DZ = 0x10
# the FPU chunk: model, flags, FP0-FP7 (10 bytes each), FPCR, FPSR
FPSR_OFFSET = 4 + 4 + 8 * 10 + 4


def test_code():
    code = []
    words = code.extend
    words([0x7E09])  # MOVEQ #9,D7
    top = len(code)
    words([0x4EB9]); calls = [len(code)]; words([0, 0])  # JSR divide (compiles it)
    words([0x51CF, (2 * (top - len(code) - 1)) & 65535])  # DBRA D7,top
    words([0xF23C, 0x8800, 0, 0])  # FMOVE.L #0,FPSR
    words([0x4EB9]); calls.append(len(code)); words([0, 0])  # JSR divide (compiled)
    words([0x23FC, 0x1234, 0x5678, READY >> 16, READY & 65535])  # MOVE.L #$12345678,READY
    words([0x60FE])  # BRA *
    divide = RAM_CODE + len(code) * 2
    words([0xF23C, 0x4000, 0, 1])  # FMOVE.L #1,FP0
    words([0xF23C, 0x4020, 0, 0])  # FDIV.L #0,FP0
    words([0x4E75])  # RTS
    for ref in calls:
        code[ref:ref + 2] = [divide >> 16, divide & 65535]
    payload = assemble(code)
    return payload + bytes(-len(payload) % 4)


def saved_fpsr(binary, env, rom, work, threaded):
    emu = Emulator(binary, env, model="A1200", options=(
        f"kickstart_rom_file={rom}", "cpu_model=68020", "fpu_model=68882",
        "cpu_compatible=false", "cpu_memory_cycle_exact=false",
        "cpu_cycle_exact=false", "cpu_speed=max", f"cpu_threaded={threaded}",
        "fpu_softfloat=false", "fpu_core=host", "comp_fpu=true", "cachesize=8192",
    ))
    control = None
    try:
        control = Connection(emu.endpoint)
        deadline = time.monotonic() + 60
        while control.request(f"READ_MEM\t{READY:#x}\t4") != "OK\t305419896":
            if time.monotonic() >= deadline:
                raise AssertionError(f"Guest did not finish (cpu_threaded={threaded})\n{emu.text()}")
            time.sleep(0.05)
        state = work / f"fpsr-{threaded}.uss"
        reply = control.request(f"SAVESTATE\t{state}\t{work / f'fpsr-{threaded}.uae'}")
        assert reply.startswith("OK"), reply
        # the main thread writes the state between frames
        deadline = time.monotonic() + 30
        size = -1
        while not state.exists() or state.stat().st_size != size or size <= 0:
            if time.monotonic() >= deadline:
                raise AssertionError(f"no savestate written: {reply}\n{emu.text()}")
            size = state.stat().st_size if state.exists() else -1
            time.sleep(0.2)
        data = state.read_bytes()
        if data[:2] == b"\x1f\x8b":
            data = gzip.decompress(data)
        chunk = data.index(b"FPU ") + 12  # name, length, flags
        return struct.unpack(">I", data[chunk + FPSR_OFFSET:chunk + FPSR_OFFSET + 4])[0]
    finally:
        if control:
            control.close()
        emu.kill()


def main(binary):
    if platform.machine().lower() not in ("x86_64", "amd64", "aarch64", "arm64"):
        # Only the x86-64 and AArch64 JITs have inline FPU code.
        print("FPU JIT savestate test skipped: not an x86-64 or AArch64 host", flush=True)
        return
    binary = os.path.abspath(binary)
    with tempfile.TemporaryDirectory(prefix="amiberry-fpu-savestate-") as directory:
        work = Path(directory)
        rom = work / "savestate.rom"
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
        for threaded in ("true", "false"):
            fpsr = saved_fpsr(binary, env, rom, work, threaded)
            assert fpsr & FPSR_AE_DZ, (
                f"cpu_threaded={threaded}: saved FPSR {fpsr:#010x} lacks the accrued "
                f"divide by zero of compiled code")
            print(f"cpu_threaded={threaded}: saved FPSR {fpsr:#010x} has the accrued "
                  f"divide by zero", flush=True)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
