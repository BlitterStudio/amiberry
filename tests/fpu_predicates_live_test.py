#!/usr/bin/env python3
"""Run an original synthetic ROM's FPSR/FScc/FBcc matrix in both CPU engines.

Usage: python3 tests/fpu_predicates_live_test.py <path-to-amiberry>
"""

import os
from pathlib import Path
import struct
import sys
import tempfile
import time

from ipc_actions_test import Connection, Emulator


def expected_condition(control, condition):
    n, z, nan = bool(control & 8), bool(control & 4), bool(control & 1)
    if nan:
        # The 68882 gives Z priority for ordered predicates on explicit CC writes.
        return bool(condition & 8) or (z and bool(condition & 1))
    return (
        False, z, not (nan or z or n), z or not (nan or n),
        n and not (nan or z), z or (n and not nan), not (nan or z), not nan,
        nan, nan or z, nan or not (n or z), nan or z or not n,
        nan or (n and not z), nan or z or n, not z, True,
    )[condition]


def predicate_rom():
    code = bytearray()

    def words(*values):
        code.extend(struct.pack(">" + "H" * len(values), *values))

    def store(reg, address):
        words(0x23C0 | reg, address >> 16, address & 65535)

    words(0x46FC, 0x2700)  # Mask interrupts.
    words(0x13FC, 3, 0xBF, 0xE201, 0x13FC, 0xFE, 0xBF, 0xE001)  # RAM overlay.
    words(0x7001, 0x4E7B, 2)  # Enable the 68020 instruction cache/JIT.
    words(0x7400, 0x7600, 0x3E3C, 255)  # Clear snapshot registers; warm blocks.
    loop = len(code)
    for control in range(16):
        for condition in range(16):
            words(0x203C, control << 8, 0, 0x223C, 0x1234, 0x5600)
            words(0x44FC, 0x15, 0xF200, 0x8800)  # CCR; D0 -> FPSR.
            words(0xF241, condition, 0x40C2)  # FScc D1; snapshot SR.
            words(0xF280 | condition, 8)
            words(0x40C3, 0x7800, 0x6006)  # Untaken: snapshot SR, D4=0.
            words(0x40C3, 0x7801, 0x4E71)  # Taken: snapshot SR, D4=1.
            address = 0x1000 + (control * 16 + condition) * 16
            for index, reg in enumerate((1, 2, 3, 4)):
                store(reg, address + index * 4)
    words(0x51CF, (loop - (len(code) + 2)) & 65535)
    words(0x23FC, 0x1234, 0x5678, 0, 0x2000, 0x60FE)
    rom = bytearray(512 * 1024)
    struct.pack_into(">II", rom, 0, 0x7F000, 0xF80008)
    rom[8:8 + len(code)] = code
    checksum = 0
    for (word,) in struct.iter_unpack(">I", rom):
        checksum += word
        checksum = (checksum & 0xFFFFFFFF) + (checksum >> 32)
    struct.pack_into(">I", rom, len(rom) - 4, checksum ^ 0xFFFFFFFF)
    return rom


def main(binary):
    binary = os.path.abspath(binary)
    with tempfile.TemporaryDirectory(prefix="amiberry-fpu-predicates-") as directory:
        work = Path(directory)
        rom = work / "predicates.rom"
        rom.write_bytes(predicate_rom())
        env = dict(os.environ)
        env.update({
            "HOME": directory, "XDG_RUNTIME_DIR": directory,
            "XDG_CONFIG_HOME": str(work / "config"), "XDG_DATA_HOME": str(work / "data"),
            "AMIBERRY_HOME_DIR": str(work / "home"), "LOCALAPPDATA": str(work / "local"),
            "APPDATA": str(work / "roaming"),
            "SDL_VIDEODRIVER": "dummy" if sys.platform == "darwin" else "offscreen",
            "SDL_AUDIODRIVER": "dummy",
        })
        env.pop("AMIBERRY_CONFIG_DIR", None)
        for cache in (0, 8192):
            emu = Emulator(binary, env, model="A1200", options=(
                f"kickstart_rom_file={rom}", "cpu_model=68020", "fpu_model=68882",
                "cpu_compatible=false", "cpu_memory_cycle_exact=false",
                "cpu_cycle_exact=false", "cpu_speed=max", "cpu_thread=false",
                "fpu_softfloat=false", "comp_fpu=true", f"cachesize={cache}",
            ))
            control = None
            try:
                control = Connection(emu.endpoint)
                if cache:
                    assert f"JIT=CPU/FPU={cache}." in emu.text(), emu.text()
                deadline = time.monotonic() + 30
                while control.request("READ_MEM\t0x2000\t4") != "OK\t305419896":
                    if time.monotonic() >= deadline:
                        raise AssertionError(f"Guest did not finish (cache={cache})\n{emu.text()}")
                    time.sleep(0.05)
                replies = control.pipeline(
                    [f"READ_MEM\t{address:#x}\t4" for address in range(0x1000, 0x2000, 4)])
                assert len(replies) == 1024, len(replies)
                values = []
                for reply in replies:
                    assert reply.startswith("OK\t"), reply
                    values.append(int(reply.split("\t")[1]))
                for index in range(256):
                    control_bits, condition = divmod(index, 16)
                    expected = expected_condition(control_bits, condition)
                    actual = values[index * 4:index * 4 + 4]
                    wanted = [0x123456FF if expected else 0x12345600,
                              0x2715, 0x2715, int(expected)]
                    assert actual == wanted, (
                        f"cache={cache} FPSR={control_bits:x} condition={condition:x}: "
                        f"{actual} != {wanted}")
                print(f"cache={cache}: 256 FPSR combinations/predicates and integer CCR passed",
                      flush=True)
            finally:
                if control:
                    control.close()
                emu.kill()


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
