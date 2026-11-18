#!/usr/bin/env python3
"""Compare FMOVECR and FPU memory loads between the interpreter and the JIT.

An original synthetic ROM runs FMOVECR (defined and undefined offsets) and
extended, double and FMOVEM loads of edge-case encodings in every FPCR
rounding mode, storing each result as a double (and the FPSR after FMOVECR).
The results with the JIT's inline host FPU code (x87 on x86-64, AArch64
FPU) must equal the interpreter's.

Usage: python3 tests/fpu_jit_loads_live_test.py <path-to-amiberry>
"""

import os
import platform
from pathlib import Path
import struct
import sys
import tempfile
import time

from ipc_actions_test import HEADLESS_SDL_ENV, Connection, Emulator

ROM_BASE = 0xF80000
RAM_CODE = 0x10000  # the test runs from chip RAM, where the JIT compiles it
DATA = 0x1800  # offset of the operand table from RAM_CODE
OUT = 0x1000
DONE = 0x3000
STRIDE = 0x200  # output bytes per rounding mode

CONSTANTS = (0x00, 0x01, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x30, 0x32, 0x34, 0x36, 0x3B, 0x3F)
EXTENDED = (
    (0x3FFF, 0x80000000, 0x00000000),  # 1.0
    (0x3FFF, 0x80000000, 0x00000401),  # rounds by mode to a double
    (0x0000, 0x00000000, 0x00000123),  # denormal
    (0x0000, 0x80000000, 0x00000000),  # zero exponent, integer bit set
    (0x7FFF, 0x00000000, 0x00000000),  # infinity without the integer bit
    (0xC000, 0xC90FDAA2, 0x2168C235),  # -pi
    (0x3BCE, 0xA0000000, 0x00000001),  # denormal double just above a tie: rounded once
)
DOUBLE = (0x3FF8000000000000, 0x0000000000000001, 0x8000000000000000, 0x3FF0000000000801)
FPSR_EXC = 0x0000FF00  # the inline code leaves the exception byte to the next helper


def results_layout():
    sizes = len(CONSTANTS) * 12 + (len(EXTENDED) * 2 + len(DOUBLE)) * 8
    assert sizes <= STRIDE
    return sizes


def assemble(words_list):
    return struct.pack(">" + "H" * len(words_list), *words_list)


def test_code():
    code = bytearray()

    def words(*values):
        code.extend(assemble(values))

    def long(value):
        words(value >> 16, value & 65535)

    ext_table = RAM_CODE + DATA
    dbl_table = ext_table + len(EXTENDED) * 12

    words(0x3E3C, 255)  # Repeat so that the blocks are compiled.
    loop = len(code)
    for mode in range(4):
        out = OUT + mode * STRIDE
        words(0xF23C, 0x9000); long(mode << 4)  # FMOVE.L #mode,FPCR
        for cr in CONSTANTS:
            reg = 2 + cr % 5
            words(0xF23C, 0x8800); long(0)  # FMOVE.L #0,FPSR
            words(0xF200, 0x5C00 | reg << 7 | cr)  # FMOVECR #cr,FPreg
            words(0xF239, 0x7400 | reg << 7); long(out)  # FMOVE.D FPreg,out
            words(0xF239, 0xA800); long(out + 8)  # FMOVE.L FPSR,out+8
            out += 12
        for index in range(len(EXTENDED)):
            words(0xF239, 0x4800 | 1 << 7); long(ext_table + index * 12)  # FMOVE.X
            words(0xF239, 0x7400 | 1 << 7); long(out)
            out += 8
        for index in range(len(DOUBLE)):
            words(0xF239, 0x5400 | 1 << 7); long(dbl_table + index * 8)  # FMOVE.D
            words(0xF239, 0x7400 | 1 << 7); long(out)
            out += 8
        words(0x41F9); long(ext_table)  # LEA ext_table,A0
        words(0xF218, 0xD000 | (0xFF << (8 - len(EXTENDED))) & 0xFF)  # FMOVEM.X (A0)+
        for reg in range(len(EXTENDED)):
            words(0xF239, 0x7400 | reg << 7); long(out)
            out += 8
    words(0xF23C, 0x9000); long(0)
    words(0x51CF, (loop - (len(code) + 2)) & 65535)  # DBRA D7
    words(0x23FC, 0x1234, 0x5678); long(DONE)
    words(0x60FE)
    assert len(code) <= DATA
    code.extend(bytes(DATA - len(code)))
    for exp, hi, lo in EXTENDED:
        code.extend(struct.pack(">HHII", exp, 0, hi, lo))
    for value in DOUBLE:
        code.extend(struct.pack(">Q", value))
    code.extend(bytes(-len(code) % 4))
    return code


def make_rom(payload):
    """A 512 KB ROM that enables the caches and runs payload from RAM_CODE."""
    head = (
        0x46FC, 0x2700,  # Mask interrupts.
        0x13FC, 3, 0xBF, 0xE201, 0x13FC, 0xFE, 0xBF, 0xE001,  # RAM overlay.
        0x203C, 0x8000, 0x8001, 0x4E7B, 2,  # Enable the instruction cache/JIT.
        0x41FA,  # LEA payload(PC),A0
    )
    tail = (
        0x43F9, RAM_CODE >> 16, RAM_CODE & 65535,  # LEA RAM_CODE,A1
        0x223C, 0, len(payload) // 4 - 1,  # MOVE.L #longs-1,D1
        0x22D8, 0x51C9, 0xFFFC,  # copy: MOVE.L (A0)+,(A1)+; DBRA D1,copy
        0x4EF9, RAM_CODE >> 16, RAM_CODE & 65535,  # JMP RAM_CODE
    )
    size = (len(head) + 1 + len(tail)) * 2
    size += -size % 4
    # the displacement is relative to the LEA's extension word
    loader = assemble(head + (size - len(head) * 2,) + tail)
    loader += bytes(size - len(loader))
    rom = bytearray(512 * 1024)
    struct.pack_into(">II", rom, 0, 0x7F000, ROM_BASE + 8)
    rom[8:8 + len(loader)] = loader
    rom[8 + len(loader):8 + len(loader) + len(payload)] = payload
    checksum = 0
    for (word,) in struct.iter_unpack(">I", rom):
        checksum += word
        checksum = (checksum & 0xFFFFFFFF) + (checksum >> 32)
    struct.pack_into(">I", rom, len(rom) - 4, checksum ^ 0xFFFFFFFF)
    return rom


def loads_rom():
    return make_rom(test_code())


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
        deadline = time.monotonic() + 30
        while control.request(f"READ_MEM\t{DONE:#x}\t4") != "OK\t305419896":
            if time.monotonic() >= deadline:
                raise AssertionError(f"Guest did not finish (cache={cache})\n{emu.text()}")
            time.sleep(0.05)
        size = results_layout()
        replies = control.pipeline([
            f"READ_MEM\t{OUT + mode * STRIDE + offset:#x}\t4"
            for mode in range(4) for offset in range(0, size, 4)])
        values = []
        for reply in replies:
            assert reply.startswith("OK\t"), reply
            values.append(int(reply.split("\t")[1]))
        return values
    finally:
        if control:
            control.close()
        emu.kill()


def describe(index):
    per_mode = results_layout() // 4
    mode, word = divmod(index, per_mode)
    offset = word * 4
    if offset < len(CONSTANTS) * 12:
        entry, part = divmod(offset, 12)
        what = f"FMOVECR #${CONSTANTS[entry]:02x} " + ("FPSR" if part == 8 else "value")
    else:
        offset -= len(CONSTANTS) * 12
        entry = offset // 8
        names = ([f"FMOVE.X {e}" for e in range(len(EXTENDED))] +
                 [f"FMOVE.D {d}" for d in range(len(DOUBLE))] +
                 [f"FMOVEM.X {e}" for e in range(len(EXTENDED))])
        what = names[entry]
    return f"FPCR=${mode << 4:02x} {what}"


def main(binary):
    if platform.machine().lower() not in ("x86_64", "amd64", "aarch64", "arm64"):
        # Only the x86-64 and AArch64 JITs have the inline FPU code this compares.
        print("FPU JIT loads test skipped: not an x86-64 or AArch64 host", flush=True)
        return
    binary = os.path.abspath(binary)
    with tempfile.TemporaryDirectory(prefix="amiberry-fpu-loads-") as directory:
        work = Path(directory)
        rom = work / "loads.rom"
        rom.write_bytes(loads_rom())
        env = dict(os.environ)
        env.update({
            "HOME": directory, "XDG_RUNTIME_DIR": directory,
            "XDG_CONFIG_HOME": str(work / "config"), "XDG_DATA_HOME": str(work / "data"),
            "AMIBERRY_HOME_DIR": str(work / "home"), "LOCALAPPDATA": str(work / "local"),
            "APPDATA": str(work / "roaming"),
            **HEADLESS_SDL_ENV,
        })
        env.pop("AMIBERRY_CONFIG_DIR", None)
        interpreted = run(binary, env, rom, 0)
        compiled = run(binary, env, rom, 8192)
        # FMOVECR #0 (pi) at round to nearest
        assert interpreted[0:2] == [0x400921FB, 0x54442D18], [hex(v) for v in interpreted[0:2]]
        fpsr_words = {entry * 3 + 2 for entry in range(len(CONSTANTS))}
        per_mode = results_layout() // 4
        expected = list(interpreted)
        if platform.machine().lower() in ("aarch64", "arm64"):
            # FMOVECR #$3f (10^4096) at RZ and RM: the host core holds it as a
            # double, already +inf, while the AArch64 JIT's helper converts the
            # extended constant in the rounding mode, giving the largest finite
            # double as a 68882 stores it (issue #2711, item 4)
            entry = CONSTANTS.index(0x3F) * 3
            for mode in (1, 2):
                base = mode * per_mode + entry
                expected[base:base + 3] = [0x7FEFFFFF, 0xFFFFFFFF, 0x00000048]
        failures = []
        for index, (want, got) in enumerate(zip(expected, compiled)):
            mask = ~FPSR_EXC if index % per_mode in fpsr_words else ~0
            if (want ^ got) & mask:
                failures.append(f"{describe(index)}: expected {want:#010x} JIT {got:#010x}")
        assert not failures, "\n".join(failures)
        print(f"{len(interpreted)} FMOVECR/load result words equal in the interpreter and JIT",
              flush=True)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
