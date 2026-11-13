#!/usr/bin/env python3
"""Synthetic-ROM regressions: cpu_thread_live_test.py <path-to-amiberry>."""

import os
from pathlib import Path
import struct
import sys
import tempfile
import time
import unittest
import zlib

from ipc_actions_test import Connection, Emulator


def words(*values):
    return struct.pack(">" + "H" * len(values), *values)


def rom_image(code):
    rom = bytearray(512 * 1024)
    struct.pack_into(">II", rom, 0, 0x7f000, 0xf80008)
    bootstrap = words(
        0x46fc, 0x2700,
        0x13fc, 3, 0xbf, 0xe201, 0x13fc, 0xfe, 0xbf, 0xe001,
        0x207c, 0xf8, 0x0080, 0x227c, 3, 0,
        0x203c, 0, len(code) // 2 - 1,
        0x32d8, 0x51c8, 0xfffc, 0x4ef9, 3, 0,
    )
    rom[8:8 + len(bootstrap)] = bootstrap
    rom[128:128 + len(code)] = code
    checksum = 0
    for (value,) in struct.iter_unpack(">I", rom):
        checksum += value
        checksum = (checksum & 0xffffffff) + (checksum >> 32)
    struct.pack_into(">I", rom, len(rom) - 4, checksum ^ 0xffffffff)
    return rom


def counter_code():
    # Enable the guest instruction cache, then increment D0 and mirror it in RAM.
    return words(0x46fc, 0x2700, 0x203c, 0, 0x8001, 0x4e7b, 2,
                 0x7000, 0x23fc, 0x1234, 0x5678, 0, 0x8000,
                 0x5280, 0x23c0, 0, 0x8004, 0x60f6)


def trace_code():
    code = bytearray(words(
        0x46fc, 0x2700, 0x203c, 0, 0x8001, 0x4e7b, 2,
        0x23fc, 3, 0x0200, 0, 0x24,
        0x7000, 0x3e3c, 1023,
        0x4eb9, 3, 0x0102, 0x51cf, 0xfff8,
        0x7000, 0x223c, 0, 0xa700, 0x4eb9, 3, 0x0100,
        0x46fc, 0x2700, 0x23fc, 0x1234, 0x5678, 0, 0x8000, 0x60fe))
    code.extend(bytes(0x100 - len(code)))
    # Warm the block after MOVE D1,SR before entering it with T1 set.
    code.extend(words(0x46c1, *([0x5280] * 8), 0x4e75))
    code.extend(bytes(0x200 - len(code)))
    code.extend(words(0x23c0, 0, 0x8010, 0x202f, 2,
                      0x23c0, 0, 0x8014, 0x0257, 0x7fff, 0x4e73))
    return code


def stop_code():
    # Master mode exercises the JIT's interpreted-special-mode STOP path too.
    return counter_code()[:26] + words(
        0x4e72, 0x3700, 0x23fc, 0xbad, 0xdead, 0, 0x8000, 0x60fe)


def io_code():
    return counter_code()[:-2] + words(0x1239, 0xbf, 0xe001, 0x60f0)


def bus_code():
    code = bytearray(words(
        0x46fc, 0x2700, 0x203c, 0, 0x8001, 0x4e7b, 2,
        0x23fc, 3, 0x0100, 0, 8,
        0x13fc, 0x80, 0xde, 1, 0x1039, 0xbf, 0x0000,
        0x23fc, 0xbad, 0xdead, 0, 0x8000, 0x60fe))
    code.extend(bytes(0x100 - len(code)))
    code.extend(words(0x23fc, 0x1234, 0x5678, 0, 0x8000, 0x60fe))
    return code


def saved_counter(path):
    data = path.read_bytes()
    chunks = {}
    offset = 0
    while offset + 12 <= len(data):
        name = data[offset:offset + 4]
        size, flags = struct.unpack_from(">II", data, offset + 4)
        if name == b"END ":
            break
        if size < 12:
            raise ValueError(f"Invalid savestate chunk {name!r} at {offset}")
        payload = data[offset + 12:offset + size]
        if name in (b"CPU ", b"CRAM"):
            chunks[name] = zlib.decompress(payload[4:]) if flags & 1 else payload
        offset += size + 4 - (size & 3)
    return (struct.unpack_from(">I", chunks[b"CPU "], 8)[0],
            struct.unpack_from(">I", chunks[b"CRAM"], 0x8004)[0])


class CpuThreadLiveTests(unittest.TestCase):
    binary = None

    def request(self, command):
        result = self.control.request(command)
        self.assertTrue(result == "OK" or result.startswith("OK\t"), result)
        return result

    def read(self, address):
        return int(self.request(f"READ_MEM\t{address:#x}\t4").split("\t")[1])

    def snapshot(self, work, name):
        path = work / f"{name}.uss"
        self.assertEqual(self.request(f"SAVESTATE\t{path}\t{work / 'saved.uae'}"), "OK")
        d0, memory = saved_counter(path)
        self.assertIn((d0 - memory) & 0xffffffff, (0, 1), (d0, memory))

    def run_case(self, kind, threaded, cache):
        with tempfile.TemporaryDirectory(prefix="amiberry-cputhread-") as directory:
            work = Path(directory)
            code = {"counter": counter_code, "trace": trace_code,
                    "bus": bus_code,
                    "stop": stop_code, "io": io_code}[kind]()
            rom = work / "synthetic.rom"
            rom.write_bytes(rom_image(code))
            env = dict(os.environ, HOME=directory, XDG_RUNTIME_DIR=directory,
                       XDG_CONFIG_HOME=str(work / "config"), XDG_DATA_HOME=str(work / "data"),
                       AMIBERRY_HOME_DIR=str(work / "home"), LOCALAPPDATA=str(work / "local"),
                       APPDATA=str(work / "roaming"), SDL_AUDIODRIVER="dummy",
                       SDL_VIDEODRIVER="dummy" if sys.platform == "darwin" else "offscreen",
                       AMIBERRY_CPU_THREAD_DIAGNOSTICS="1")
            env.pop("AMIBERRY_CONFIG_DIR", None)
            options = [f"kickstart_rom_file={rom}", "cpu_model=68020", "fpu_model=0",
                       "cpu_compatible=false", "cpu_memory_cycle_exact=false",
                       "cpu_cycle_exact=false", "cpu_speed=max",
                       f"cpu_threaded={'true' if threaded else 'false'}", f"cachesize={cache}"]
            if kind == "bus":
                options += ["chipset_compatible=A4000", "fatgary=0"]
            emu = Emulator(self.binary, env, model="A1200", options=options)
            self.control = None
            try:
                self.control = Connection(emu.endpoint, 8)
                self.assertEqual(self.request("SETTINGS_GET_ACTIVE\tcpu_threaded"),
                                 f"OK\tcpu_threaded={'true' if threaded else 'false'}")
                active_cache = self.request("SETTINGS_GET_ACTIVE\tcachesize")
                if cache and active_cache == "OK\tcachesize=0":
                    self.skipTest("this build has no JIT")
                self.assertEqual(active_cache, f"OK\tcachesize={cache}")
                self.assertEqual(self.read(0x8000), 0x12345678)
                self.assertTrue(self.request("GET_CPU_REGS").startswith("OK\tD0="))
                self.request("WRITE_MEM\t0x8020\t4\t0x89abcdef")
                self.assertEqual(self.read(0x8020), 0x89abcdef)
                self.assertTrue(self.request("DISASSEMBLE\t30000\t2").startswith("OK\t"))
                if kind == "trace":
                    self.assertEqual(self.read(0x8010), 1)
                    self.assertEqual(self.read(0x8014), 0x30104)
                if kind in ("counter", "io"):
                    for repeat in range(3):
                        self.snapshot(work, f"running-{repeat}")
                        self.assertEqual(self.request("PAUSE"), "OK")
                        self.assertIn("Paused=true", self.request("GET_STATUS"))
                        before = self.read(0x8004)
                        time.sleep(0.1)
                        self.assertEqual(self.read(0x8004), before)
                        self.snapshot(work, f"paused-{repeat}")
                        self.assertEqual(self.request("RESUME"), "OK")
                        time.sleep(0.1)
                        self.assertNotEqual(self.read(0x8004), before)
                    # Reset repeatedly while the worker is running.
                    if threaded:
                        for _ in range(3):
                            self.request("RESET\tHARD")
                            time.sleep(0.3)
                            self.assertEqual(self.read(0x8000), 0x12345678)
                if kind in ("counter", "io", "stop"):
                    self.request("PAUSE")
                    self.request("GET_STATUS")
                self.request("QUIT")
                self.assertEqual(emu.proc.wait(timeout=10), 0)
                # The CPU thread is chosen only with two or more usable host CPUs.
                usable = len(os.sched_getaffinity(0)) if hasattr(os, "sched_getaffinity") else (os.cpu_count() or 1)
                ran_threaded = threaded and usable >= 2
                deadline = time.monotonic() + 1
                while "END DIAGNOSTICS REPORT" not in emu.text() and time.monotonic() < deadline:
                    time.sleep(0.01)
                self.assertIn(f"CPU threading: {'Yes' if ran_threaded else 'No'};", emu.text())
            except unittest.SkipTest:
                raise
            except Exception:
                print(emu.text()[-12000:], file=sys.stderr)
                raise
            finally:
                if self.control:
                    self.control.close()
                emu.kill()
                emu.proc.stdout.close()

    def test_execution_matrix(self):
        for kind in ("counter", "trace", "bus", "stop", "io"):
            for threaded in (False, True):
                for cache in (0, 8192):
                    with self.subTest(kind=kind, threaded=threaded, cache=cache):
                        self.run_case(kind, threaded, cache)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    CpuThreadLiveTests.binary = str(Path(sys.argv.pop()).resolve())
    unittest.main()
