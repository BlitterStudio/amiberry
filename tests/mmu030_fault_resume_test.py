#!/usr/bin/env python3
"""Behavioral test for 68030 MMU fault resume.

Boots tests/data/mmu030/fault_resume.bin as the Kickstart of a 68030+MMU
machine, once with cpu_compatible off and once on, and reads its results over
IPC. The ROM faults user-mode (A7)+ and -(A7) reads on an unmapped page, lets
the handler map the page and RTE resume the instruction, then takes an
odd-PC address error. Checks that the resumed instructions leave USP and the
supervisor stack where the hardware would, and that the address error frame
does not carry the earlier fault's register fixup.

Usage: mmu030_fault_resume_test.py <path-to-amiberry-binary>
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

IS_WINDOWS = sys.platform == "win32"
ROM_CODE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data", "mmu030", "fault_resume.bin")
ROM_SIZE = 512 * 1024
RESULTS = 0x2000
FIELDS = ["magic", "c1usp", "c1ssp", "c2usp", "c2ssp", "c3fmt", "c3w36", "c3w34", "berr", "unexp", "ssp"]
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


def run(binary, rom, work, compatible):
    home = os.path.join(work, "compat" if compatible else "fast")
    env = dict(os.environ)
    env.update({"HOME": home, "XDG_RUNTIME_DIR": home, "XDG_CONFIG_HOME": os.path.join(home, "config"),
                "XDG_DATA_HOME": os.path.join(home, "data"), "AMIBERRY_HOME_DIR": os.path.join(home, "home"),
                "LOCALAPPDATA": os.path.join(home, "local"), "APPDATA": os.path.join(home, "roaming"),
                "SDL_VIDEODRIVER": "dummy" if sys.platform == "darwin" else "offscreen",
                "SDL_AUDIODRIVER": "dummy"})
    env.pop("AMIBERRY_CONFIG_DIR", None)
    os.makedirs(home)
    args = [binary, "--model", "A4000", "-G"]
    for setting in ("cpu_model=68030", "mmu_model=68030", "fpu_model=0", "cpu_24bit_addressing=false",
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


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    binary = sys.argv[1]
    work = tempfile.mkdtemp(prefix="amiberry-mmu030-")
    try:
        rom = os.path.join(work, "fault_resume.rom")
        make_rom(rom)
        for compatible in (False, True):
            mode = f"cpu_compatible={'on' if compatible else 'off'}"
            r = run(binary, rom, work, compatible)
            detail = " ".join(f"{k}={v:#x}" if v is not None else f"{k}=None" for k, v in r.items())
            check(f"{mode}: test ROM finished", r["magic"] == 0x444F4E45 and r["unexp"] == 0xFFFF, detail)
            check(f"{mode}: both page faults were taken", r["berr"] == 2, detail)
            check(f"{mode}: (A7)+ resumed with USP advanced once", r["c1usp"] == 0x40002, detail)
            check(f"{mode}: -(A7) resumed with USP decremented once", r["c2usp"] == 0x40FFE, detail)
            check(f"{mode}: supervisor stack unchanged by the resumes",
                  r["ssp"] == 0x80000 and r["c1ssp"] == r["ssp"] and r["c2ssp"] == r["ssp"], detail)
            check(f"{mode}: odd-PC address error builds a format B frame", r["c3fmt"] == 0xB00C, detail)
            check(f"{mode}: address error frame carries no register fixup",
                  r["c3w36"] is not None and r["c3w34"] is not None
                  and r["c3w36"] >> 8 == 0 and r["c3w34"] >> 8 == 0, detail)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    if failures:
        sys.exit(f"{len(failures)} check(s) failed")
    print("68030 MMU fault resume behavioral test passed")


main()
