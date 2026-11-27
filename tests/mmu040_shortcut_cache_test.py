#!/usr/bin/env python3
"""Behavioral test for the 68040/68060 MMU translation cache.

Boots tests/data/mmu040/shortcut_cache.bin as the Kickstart of a 68040+MMU
and a 68060+MMU machine, each with cpu_compatible off and on, and reads its
results over IPC. The ROM reads logical pages whose translation changes
between reads: a data TTR stops (and starts) covering the page, the MMU is
enabled after an MMU-off read, the page size switches between 4K and 8K,
user and supervisor see different mappings, an exception goes through the
vector page, and a page is covered by a data TTR but not an instruction TTR.
It also writes such pages, including through a write-protected data TTR.
Checks that every access reaches the physical page the hardware would.

Usage: mmu040_shortcut_cache_test.py <path-to-amiberry-binary>
"""
import os
import re
import shutil
import socket
import struct
import subprocess
import sys
import tempfile
import threading
import time

from ipc_actions_test import HEADLESS_SDL_ENV

IS_WINDOWS = sys.platform == "win32"
ROM_CODE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data", "mmu040", "shortcut_cache.bin")
ROM_SIZE = 512 * 1024
RESULTS = 0x2000
FIELDS = ["magic", "unexp", "1a", "1b", "1c", "1d", "2a", "2b", "2c", "3a", "3b", "3c", "3d",
          "4a", "4b", "4c", "4d", "4e", "4f", "4g", "5a", "5b", "5c", "5x", "6a", "6c", "6d", "6e",
          "7a", "7b", "7c", "7d", "7e", "7g"]
failures = []


def check(name, condition, detail=""):
    print(("ok   " + name) if condition else (f"FAIL {name} ({detail})" if detail else f"FAIL {name}"), flush=True)
    if not condition:
        failures.append(name)


def kickstart_checksum(rom):
    total = 0
    for (value,) in struct.iter_unpack(">I", rom):
        total += value
        if total > 0xFFFFFFFF:
            total = (total & 0xFFFFFFFF) + 1
    return total


def make_rom(path):
    with open(ROM_CODE, "rb") as f:
        rom = bytearray(f.read())
    rom += bytes(ROM_SIZE - len(rom))
    struct.pack_into(">I", rom, ROM_SIZE - 20, ROM_SIZE)
    struct.pack_into(">I", rom, ROM_SIZE - 24, ~kickstart_checksum(rom) & 0xFFFFFFFF)
    with open(path, "wb") as f:
        f.write(rom)


class Connection:
    def __init__(self, endpoint, timeout=30.0):
        self._buffer = b""
        deadline = time.monotonic() + timeout
        while True:
            try:
                if IS_WINDOWS:
                    self._pipe = open(endpoint, "r+b", buffering=0)
                else:
                    self._sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                    try:
                        self._sock.connect(endpoint)
                    except OSError:
                        self._sock.close()
                        raise
                    self._sock.settimeout(timeout)
                return
            except OSError:
                if time.monotonic() > deadline:
                    raise
                time.sleep(0.05)

    def request(self, line):
        data = (line + "\n").encode()
        if IS_WINDOWS:
            self._pipe.write(data)
        else:
            self._sock.sendall(data)
        buffer = self._buffer
        while b"\n" not in buffer:
            chunk = self._pipe.read(65536) if IS_WINDOWS else self._sock.recv(65536)
            if not chunk:
                break
            buffer += chunk
        reply, _, self._buffer = buffer.partition(b"\n")
        return reply.decode(errors="replace").split("\t")

    def read_long(self, address):
        reply = self.request(f"READ_MEM\t{address:#x}\t4")
        return int(reply[1]) if reply[0] == "OK" and len(reply) > 1 else None

    def close(self):
        (self._pipe if IS_WINDOWS else self._sock).close()


def run(binary, rom, work, cpu, compatible):
    home = os.path.join(work, f"{cpu}-{'compat' if compatible else 'fast'}")
    env = dict(os.environ)
    env.update({"HOME": home, "XDG_RUNTIME_DIR": home, "XDG_CONFIG_HOME": os.path.join(home, "config"),
                "XDG_DATA_HOME": os.path.join(home, "data"), "AMIBERRY_HOME_DIR": os.path.join(home, "home"),
                "LOCALAPPDATA": os.path.join(home, "local"), "APPDATA": os.path.join(home, "roaming"),
                **HEADLESS_SDL_ENV})
    env.pop("AMIBERRY_CONFIG_DIR", None)
    os.makedirs(home)
    args = [binary, "--model", "A4000", "-G"]
    for setting in (f"cpu_model={cpu}", f"mmu_model={cpu}", "fpu_model=0", "cpu_24bit_addressing=false",
                    "cachesize=0", "cpu_speed=max", "cycle_exact=false", "fastmem_size=0", "z3mem_size=0",
                    "cpu_compatible=" + ("true" if compatible else "false"),
                    "kickstart_rom_file=" + rom, "kickstart_ext_rom_file="):
        args += ["-s", setting]
    output = []
    proc = subprocess.Popen(args, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            stdin=subprocess.DEVNULL)
    threading.Thread(target=lambda: [output.append(line.decode(errors="replace")) for line in proc.stdout],
                     daemon=True).start()
    results = {}
    try:
        endpoint = None
        deadline = time.monotonic() + 60
        while endpoint is None and time.monotonic() < deadline and proc.poll() is None:
            for line in list(output):
                match = re.search(r"IPC: Listening on (\S+)", line)
                if match:
                    endpoint = match.group(1)
            time.sleep(0.05)
        if endpoint is None:
            sys.exit("IPC endpoint never reported; output:\n" + "".join(output[-40:]))
        control = Connection(endpoint)
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline and control.read_long(RESULTS) not in (0x444F4E45, 0x4641494C):
            time.sleep(0.1)
        for index, name in enumerate(FIELDS):
            results[name] = control.read_long(RESULTS + 4 * index)
        control.request("QUIT")
        control.close()
        proc.wait(15)
    finally:
        if proc.poll() is None:
            proc.kill()
            proc.wait()
    return results


def tag(value):
    if value is None:
        return "None"
    text = value.to_bytes(4, "big")
    return repr(text.decode()) if all(32 <= c < 127 for c in text) else f"{value:#x}"


def expect(r, prefix, field, wanted, what):
    want = struct.unpack(">I", wanted.encode())[0] if isinstance(wanted, str) else wanted
    check(f"{prefix}: {what}", r[field] == want, f"read {tag(r[field])}, expected {tag(want)}")


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    binary = sys.argv[1]
    work = tempfile.mkdtemp(prefix="amiberry-mmu040-")
    try:
        rom = os.path.join(work, "shortcut_cache.rom")
        make_rom(rom)
        for cpu in (68040, 68060):
            for compatible in (False, True):
                p = f"{cpu} cpu_compatible={'on' if compatible else 'off'}"
                r = run(binary, rom, work, cpu, compatible)
                detail = " ".join(f"{k}={tag(v)}" for k, v in r.items())
                check(f"{p}: test ROM finished", r["magic"] == 0x444F4E45 and r["unexp"] == 0xFFFF, detail)
                expect(r, p, "1a", "IDEN", "DTT0 covering the page reads it untranslated")
                expect(r, p, "1b", "REMP", "DTT0 disabled: page tables remap the page")
                expect(r, p, "1c", "IDEN", "DTT0 re-enabled: untranslated again")
                expect(r, p, "1d", "REMP", "DTT0 moved elsewhere: page tables remap the page")
                expect(r, p, "2a", "IDEN", "MMU off reads the page untranslated")
                expect(r, p, "2b", "REMP", "MMU enabled: page tables remap the page")
                expect(r, p, "2c", "IDEN", "MMU disabled again: untranslated")
                expect(r, p, "3a", "I43 ", "4K pages: second 4K page is identity mapped")
                expect(r, p, "3b", "R52 ", "8K pages: first half of the 8K page is remapped")
                expect(r, p, "3c", "R53 ", "8K pages: second half of the 8K page is remapped")
                expect(r, p, "3d", "I43 ", "back to 4K pages: identity mapped again")
                expect(r, p, "4a", "IDEN", "SRP tree: supervisor read untranslated")
                expect(r, p, "4b", "REMP", "URP tree: user read of the same page remapped")
                expect(r, p, "4c", "IDEN", "SRP tree: supervisor read untranslated again")
                expect(r, p, "4d", "IDEN", "supervisor-only DTT0: supervisor read untranslated")
                expect(r, p, "4e", "REMP", "supervisor-only DTT0: user read remapped")
                expect(r, p, "4f", "REMP", "user-only DTT0: supervisor read remapped")
                expect(r, p, "4g", "IDEN", "user-only DTT0: user read untranslated")
                expect(r, p, "5a", "VEC1", "vector page read before the exception")
                expect(r, p, "5b", "VEC2", "vector page read after the exception")
                check(f"{p}: vector read after the exception", r["5c"] is not None and r["5c"] == r["5x"],
                      f"read {tag(r['5c'])}, expected {tag(r['5x'])}")
                expect(r, p, "6a", "IDEN", "DTT0 only: data read untranslated")
                expect(r, p, "6c", "REMP", "DTT0 only: code fetch remapped")
                expect(r, p, "6d", "IDEN", "DTT0 and ITT0: code fetch untranslated")
                expect(r, p, "6e", "REMP", "ITT0 disabled again: code fetch remapped")
                expect(r, p, "7a", "W7A ", "DTT0 covering the page: write untranslated")
                expect(r, p, "7b", "W7B ", "DTT0 disabled: page tables remap the write")
                expect(r, p, "7c", "W7C ", "MMU off: write untranslated")
                expect(r, p, "7d", "W7D ", "MMU enabled: page tables remap the write")
                expect(r, p, "7e", "W7E ", "write-protected DTT0: write not performed")
                expect(r, p, "7g", "FALT", "write-protected DTT0: write takes an access fault")
    finally:
        shutil.rmtree(work, ignore_errors=True)
    if failures:
        sys.exit(f"{len(failures)} check(s) failed")
    print("68040/68060 MMU translation cache behavioral test passed")


main()
