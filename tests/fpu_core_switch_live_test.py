#!/usr/bin/env python3
"""Synthetic-ROM FPU regressions: fpu_core_switch_live_test.py <path-to-amiberry>.

- A live Host <-> Exact core switch (fpu_modechange()) keeps the guest's FPCR
  rounding mode and precision in effect in the new core.
- Compiled FPU arithmetic (the exact core's JIT path) reads a memory operand
  once, and only after a pending FPU exception: the operand is CIA-B's ICR,
  which a read clears. A compiled read reaches the CIA only when compiled code
  accesses memory through the banks (jit_n_addr_unsafe: board memory that
  needs it, such as some RTG boards); with direct access the early read had no
  side effect, so on such hosts this checks the order, not the old bug.
"""

import os
import platform
from pathlib import Path
import struct
import sys
import tempfile
import time
import unittest

from cpu_thread_live_test import rom_image
from ipc_actions_test import HEADLESS_SDL_ENV, Connection, Emulator

CODE_BASE = 0x30000  # rom_image() copies the code there and jumps to it

# Only the ARM64 JIT compiles the exact core's FPU instructions (the x86-64
# JIT leaves them to the interpreter): elsewhere the JIT case still checks the
# results, but not that the instruction was compiled.
JIT_COMPILES_EXACT_FPU = platform.machine().lower() in ("aarch64", "arm64")


class Asm:
    """Just enough of an assembler: words, longs, labels and absolute fixups."""

    def __init__(self):
        self.code = bytearray()
        self.labels = {}
        self.fixups = []

    def w(self, *values):
        self.code += struct.pack(">" + "H" * len(values), *values)

    def l(self, value):
        self.code += struct.pack(">I", value)

    def label(self, name):
        self.labels[name] = len(self.code)

    def abs_l(self, name):
        self.fixups.append((len(self.code), name))
        self.l(0)

    def bytes(self):
        for offset, name in self.fixups:
            struct.pack_into(">I", self.code, offset, CODE_BASE + self.labels[name])
        return bytes(self.code)


def fpcr(value, a):
    a.w(0xF23C, 0x9000)  # FMOVE.L #imm,FPCR
    a.l(value)


def switch_code():
    a = Asm()
    a.w(0x46FC, 0x2700)
    fpcr(0x50, a)        # single precision, round to zero
    a.label("loop")
    a.w(0xF23C, 0x4000); a.l(1)         # FMOVE.L #1,FP0
    a.w(0xF23C, 0x4020); a.l(3)         # FDIV.L #3,FP0
    a.w(0xF239, 0x6800); a.l(0x1000)    # FMOVE.X FP0,$1000
    a.w(0xF239, 0xB000); a.l(0x1010)    # FMOVE.L FPCR,$1010
    a.w(0x52B9); a.l(0x1020)            # ADDQ.L #1,$1020
    a.w(0x60FF); a.l((a.labels["loop"] - len(a.code)) & 0xFFFFFFFF)  # BRA.L loop
    return a.bytes()


def cia_code():
    a = Asm()
    a.w(0x46FC, 0x2700)
    a.w(0x13FC, 1); a.l(0x2000)                 # MOVE.B #1,$2000 (the warm-up operand)
    a.w(0x23FC); a.abs_l("handler"); a.l(0xC8)  # vector 50: FPU divide by zero
    a.w(0x203C); a.l(0x8001); a.w(0x4E7B, 2)    # MOVE.L #$8001,D0; MOVEC D0,CACR

    # A: an exception enabled (OPERR), none raised
    fpcr(0x2000, a)
    a.w(0x3C3C, 499)                            # MOVE.W #499,D6
    a.label("warm_a")
    a.w(0x41F9); a.l(0x2000)                    # LEA $2000,A0
    a.w(0x4EB9); a.abs_l("read_a")              # JSR read_a
    a.w(0x51CE, (a.labels["warm_a"] - (len(a.code) + 2)) & 0xFFFF)  # DBRA D6,warm_a
    a.w(0x4EB9); a.abs_l("cia_ta")
    a.w(0x41F9); a.l(0xBFDD00)                  # LEA CIAB ICR,A0
    a.w(0x4EB9); a.abs_l("read_a")
    fpcr(0, a)
    a.w(0xF239, 0x6000); a.l(0x1000)            # FMOVE.L FP0,$1000

    # B: a divide by zero exception pending when the operand is read
    a.w(0x3C3C, 499)
    a.label("warm_b")
    a.w(0x41F9); a.l(0x2000)
    a.w(0x4EB9); a.abs_l("pend")
    a.w(0x4EB9); a.abs_l("read_b")
    a.w(0x51CE, (a.labels["warm_b"] - (len(a.code) + 2)) & 0xFFFF)
    a.w(0x4EB9); a.abs_l("cia_ta")
    a.w(0x4EB9); a.abs_l("pend")
    a.w(0x41F9); a.l(0xBFDD00)
    a.w(0x4EB9); a.abs_l("read_b")
    a.w(0x23FC); a.l(0x600DF00D); a.l(0x100C)   # done
    a.w(0x60FE)

    # the instructions under test, one copy per case (a block of their own)
    for name in ("read_a", "read_b"):
        a.label(name)
        a.w(0xF210, 0x5800)                     # FMOVE.B (A0),FP0
        a.w(0x4E75)

    a.label("pend")                             # DZ enabled, 1/0: pending on the 68882
    fpcr(0x0400, a)
    a.w(0xF23C, 0x4080); a.l(1)                 # FMOVE.L #1,FP1
    a.w(0xF23C, 0x40A0); a.l(0)                 # FDIV.L #0,FP1
    a.w(0x4E75)

    a.label("cia_ta")                           # CIA-B timer A underflow: ICR bit 0
    a.w(0x1039); a.l(0xBFDD00)                  # MOVE.B ICR,D0 (clears it)
    a.w(0x13FC, 0x10); a.l(0xBFD400)            # timer A low
    a.w(0x13FC, 0x00); a.l(0xBFD500)            # timer A high
    a.w(0x13FC, 0x19); a.l(0xBFDE00)            # CRA: start, one-shot, load
    a.label("cia_wait")
    a.w(0x0839, 0); a.l(0xBFDE00)               # BTST #0,CRA (clears at underflow)
    a.w(0x6600 | ((a.labels["cia_wait"] - (len(a.code) + 2)) & 0xFF))
    a.w(0x4E75)

    a.label("handler")                          # FSAVE clears the pending exception
    a.w(0x13F9); a.l(0xBFDD00); a.l(0x1007)     # MOVE.B ICR,$1007: what the operand read would get
    a.w(0xF327)                                 # FSAVE -(A7)
    a.w(0x7E00, 0x1E2F, 1, 0x4FF7, 0x7004)     # MOVEQ #0,D7; MOVE.B 1(A7),D7; LEA 4(A7,D7.W),A7
    fpcr(0, a)
    a.w(0x52B9); a.l(0x1008)                    # ADDQ.L #1,$1008
    a.w(0x4E73)                                 # RTE (the instruction runs again)
    return a.bytes()


class FpuLiveTests(unittest.TestCase):
    binary = None

    def request(self, command):
        result = self.control.request(command)
        self.assertTrue(result == "OK" or result.startswith("OK\t"), result)
        return result

    def read(self, address):
        return int(self.request(f"READ_MEM\t{address:#x}\t4").split("\t")[1])

    def start(self, directory, code, core, cache, stats=None):
        work = Path(directory)
        rom = work / "synthetic.rom"
        rom.write_bytes(rom_image(code))
        env = dict(os.environ, HOME=directory, XDG_RUNTIME_DIR=directory,
                   XDG_CONFIG_HOME=str(work / "config"), XDG_DATA_HOME=str(work / "data"),
                   AMIBERRY_HOME_DIR=str(work / "home"), LOCALAPPDATA=str(work / "local"),
                   APPDATA=str(work / "roaming"), **HEADLESS_SDL_ENV)
        env.pop("AMIBERRY_CONFIG_DIR", None)
        if stats:
            env["AMIBERRY_JIT_FPU_STATS"] = stats
        options = [f"kickstart_rom_file={rom}", "cpu_model=68020", "fpu_model=68882",
                   f"fpu_core={core}", "cpu_compatible=false", "cpu_memory_cycle_exact=false",
                   "cpu_cycle_exact=false", "cpu_speed=max", f"cachesize={cache}"]
        emu = Emulator(self.binary, env, model="A1200", options=options)
        self.control = Connection(emu.endpoint, 8)
        if cache and self.request("SETTINGS_GET_ACTIVE\tcachesize") == "OK\tcachesize=0":
            self.skipTest("this build has no JIT")
        return emu

    def finish(self, emu):
        if self.control:
            self.request("QUIT")
            self.control.close()
            self.control = None
        try:
            emu.proc.wait(timeout=10)
        finally:
            emu.kill()
            emu.proc.stdout.close()

    def switch_case(self, cache):
        with tempfile.TemporaryDirectory(prefix="amiberry-fpucore-") as directory:
            emu = self.start(directory, switch_code(), "host", cache)
            try:
                def settled():
                    count = self.read(0x1020)
                    deadline = time.monotonic() + 5
                    while self.read(0x1020) - count < 1000 and time.monotonic() < deadline:
                        time.sleep(0.05)
                    return ([self.read(0x1000 + i) for i in (0, 4, 8)], self.read(0x1010))

                # 1/3 at single precision, rounded to zero (not the extended,
                # round-to-nearest $AAAAAAAAAAAAAAAB of the core's defaults)
                expected = ([0x3FFD0000, 0xAAAAAA00, 0], 0x50)
                self.assertEqual(settled(), expected, "host core")
                for core in ("exact", "host", "exact"):
                    self.request(f"SETTINGS_SET\tfpu_core\t{core}")
                    self.assertTrue(self.request("SETTINGS_COMMIT").startswith("OK\tchanged=true"))
                    deadline = time.monotonic() + 5
                    while (self.request("SETTINGS_GET_ACTIVE\tfpu_core") != f"OK\tfpu_core={core}"
                           and time.monotonic() < deadline):
                        time.sleep(0.05)
                    self.assertEqual(self.request("SETTINGS_GET_ACTIVE\tfpu_core"), f"OK\tfpu_core={core}")
                    self.assertEqual(settled(), expected, f"after the switch to {core}")
            except Exception:
                print(emu.text()[-8000:], file=sys.stderr)
                raise
            finally:
                self.finish(emu)

    def cia_case(self, cache):
        with tempfile.TemporaryDirectory(prefix="amiberry-fpucia-") as directory:
            stats = str(Path(directory) / "fpustats.txt") if cache and JIT_COMPILES_EXACT_FPU else None
            emu = self.start(directory, cia_code(), "exact", cache, stats)
            try:
                deadline = time.monotonic() + 10
                while self.read(0x100C) != 0x600DF00D and time.monotonic() < deadline:
                    time.sleep(0.05)
                self.assertEqual(self.read(0x100C), 0x600DF00D, "the ROM did not finish")
                # timer A's ICR bit: lost if the operand was read before
                self.assertEqual(self.read(0x1000) & 1, 1, "exception enabled: ICR read twice")
                # the handler of the pending exception reads ICR before the
                # instruction runs again (and reads it, compiled now)
                self.assertEqual(self.read(0x1004) & 1, 1, "exception pending: ICR read before it")
                self.assertEqual(self.read(0x1008), 501, "pending exceptions taken")
            except Exception:
                print(emu.text()[-8000:], file=sys.stderr)
                raise
            finally:
                self.finish(emu)
            if stats:
                # FMOVE.B (A0),FP0 was compiled: EA mode 2, opclass 2, .B, opmode 0
                compiled = 0
                for line in Path(stats).read_text().splitlines()[1:]:
                    f = line.split()
                    if len(f) >= 9 and f[5:9] == ["2", "2", "B", "00"]:
                        compiled = int(f[1]) + int(f[2]) + int(f[3])
                self.assertGreater(compiled, 0, "FMOVE.B (A0),FP0 not compiled")

    def test_core_switch_keeps_fpcr(self):
        for cache in (0, 8192):
            with self.subTest(cache=cache):
                self.switch_case(cache)

    def test_operand_read_after_exception_gate(self):
        for cache in (0, 8192):
            with self.subTest(cache=cache):
                self.cia_case(cache)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    FpuLiveTests.binary = str(Path(sys.argv.pop()).resolve())
    unittest.main()
