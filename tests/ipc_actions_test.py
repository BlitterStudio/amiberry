#!/usr/bin/env python3
"""Behavioral test for IPC runtime actions and the settings edit session.
Starts a headless isolated emulator (AROS ROM fallback, dummy/offscreen SDL
drivers) and drives it over the control socket: actions posted while the
emulator runs take effect, are applied in order, the classic commands keep
their replies, and SETTINGS_SET/VALIDATE/COMMIT round-trips a validation
adjustment (24-bit addressing on a 68040) without logging the correction.

Usage: ipc_actions_test.py <path-to-amiberry>
"""

import os
import re
import socket
import subprocess
import sys
import tempfile
import threading
import time
import shutil

IS_WINDOWS = sys.platform == "win32"
failures = []


def check(name, condition, detail=""):
    print(("ok   " if condition else "FAIL ") + name + (f" ({detail})" if detail else ""), flush=True)
    if not condition:
        failures.append(name)


class Emulator:
    def __init__(self, binary, env):
        self.output = []
        self.proc = subprocess.Popen(
            [binary, "--model", "A500", "-G", "--log"], env=env,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL)
        threading.Thread(target=self._drain, daemon=True).start()
        deadline = time.monotonic() + 60
        self.endpoint = None
        while self.endpoint is None and time.monotonic() < deadline and self.proc.poll() is None:
            for line in list(self.output):
                match = re.search(r"IPC: Listening on (\S+)", line)
                if match:
                    self.endpoint = match.group(1)
            time.sleep(0.05)
        if self.endpoint is None:
            self.kill()
            sys.exit("IPC endpoint never reported; output:\n" + "".join(self.output[-40:]))
        # Let the emulation reach its steady frame loop.
        time.sleep(3)

    def _drain(self):
        for raw in self.proc.stdout:
            self.output.append(raw.decode(errors="replace"))

    def text(self):
        return "".join(self.output)

    def kill(self):
        if self.proc.poll() is None:
            self.proc.kill()
            self.proc.wait()


class Connection:
    def __init__(self, endpoint, timeout=15.0):
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
                time.sleep(0.02)

    def request(self, line):
        data = (line + "\n").encode()
        if IS_WINDOWS:
            self._pipe.write(data)
            buf = b""
            while not buf.endswith(b"\n"):
                chunk = self._pipe.read(65536)
                if not chunk:
                    break
                buf += chunk
        else:
            self._sock.sendall(data)
            buf = b""
            while not buf.endswith(b"\n"):
                chunk = self._sock.recv(65536)
                if not chunk:
                    break
                buf += chunk
        return buf.decode(errors="replace").rstrip("\n")

    def close(self):
        (self._pipe if IS_WINDOWS else self._sock).close()


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    binary = sys.argv[1]
    work = tempfile.mkdtemp(prefix="amiberry-actions-")
    env = dict(os.environ)
    env.update({"HOME": work, "XDG_RUNTIME_DIR": work, "XDG_CONFIG_HOME": os.path.join(work, "config"),
                "XDG_DATA_HOME": os.path.join(work, "data"), "AMIBERRY_HOME_DIR": os.path.join(work, "home"),
                "LOCALAPPDATA": os.path.join(work, "local"), "APPDATA": os.path.join(work, "roaming"),
                # SDL's offscreen driver cannot create windows on macOS; the
                # dummy driver can.
                "SDL_VIDEODRIVER": "dummy" if sys.platform == "darwin" else "offscreen",
                "SDL_AUDIODRIVER": "dummy"})
    env.pop("AMIBERRY_CONFIG_DIR", None)

    emu = Emulator(binary, env)
    try:
        control = Connection(emu.endpoint, 30)
        check("emulation is running", control.request("GET_FPS").startswith("OK\t"))

        # A posted action takes effect on the running emulator (executed on
        # the emulation thread between frames).
        check("PAUSE applies",
              control.request("PAUSE") == "OK"
              and "Paused=true" in control.request("GET_STATUS"))
        check("RESUME applies",
              control.request("RESUME") == "OK"
              and "Paused=false" in control.request("GET_STATUS"))

        # Actions are applied in order: the CPU becomes a 68040 first, then
        # the JIT is enabled. The A500 default keeps More compatible set, and
        # the core fixup clears it for a 68040 with the JIT - validation
        # reports that adjustment only because both sets applied in order.
        check("SETTINGS_SET cpu_model 68040",
              control.request("SETTINGS_SET\tcpu_model\t68040") == "OK")
        check("SETTINGS_SET cachesize 8192 enables the JIT",
              control.request("SETTINGS_SET\tcachesize\t8192") == "OK")
        validate = control.request("SETTINGS_VALIDATE")
        check("SETTINGS_VALIDATE reports the More compatible adjustment",
              validate.startswith("OK\t")
              and "cpu_compatible=true->false" in validate, validate)
        check("validation does not modify the session",
              control.request("SETTINGS_GET\tcpu_compatible")
              == "OK\tcpu_compatible=true")
        check("preview fixup is silent (fixup_prefs_preview)",
              "24-bit address space is not supported" not in emu.text())
        commit = control.request("SETTINGS_COMMIT")
        check("SETTINGS_COMMIT stores the change",
              commit.startswith("OK\tchanged=true\tapply="), commit)
        check("commit resolves the adjustment",
              control.request("SETTINGS_GET\tcpu_compatible")
              == "OK\tcpu_compatible=false")
        check("commit reached the pending configuration",
              control.request("GET_CONFIG\tcpu_model") == "OK\t68040")
        check("commit kept the JIT cache size",
              control.request("SETTINGS_GET\tcachesize") == "OK\tcachesize=8192")

        # RESET_SESSION discards edits that were never committed.
        check("SETTINGS_RESET_SESSION",
              control.request("SETTINGS_RESET_SESSION") == "OK")
        check("discarded edit does not survive",
              control.request("SETTINGS_SET\tcpu_model\t68030") == "OK"
              and control.request("SETTINGS_GET\tcpu_model") == "OK\tcpu_model=68030"
              and control.request("SETTINGS_RESET_SESSION") == "OK"
              and control.request("SETTINGS_GET\tcpu_model") == "OK\tcpu_model=68040")
        check("unknown option reported",
              control.request("SETTINGS_GET\tdefinitely_not_an_option")
              == "ERROR\tUnknown option: definitely_not_an_option")

        # The classic action commands keep behaving as before.
        adf = os.path.join(work, "boot.adf")
        with open(adf, "wb") as f:
            f.write(b"ADFFLOPPY-PLACEHOLDER" * 10)
        check("INSERTFLOPPY replies OK",
              control.request(f"INSERTFLOPPY\t{adf}\t0") == "OK")
        status = control.request("GET_STATUS")
        check("inserted floppy shows in GET_STATUS",
              f"Floppy0={adf}" in status, status)
        check("QUERYDISKSWAP still answers", control.request("QUERYDISKSWAP\t0").startswith("OK\t"))
        state = os.path.join(work, "state.uss")
        cfg = os.path.join(work, "state.uae")
        check("SAVESTATE replies OK", control.request(f"SAVESTATE\t{state}\t{cfg}") == "OK")
        check("state file written", os.path.exists(state))
        check("LOADSTATE replies OK", control.request(f"LOADSTATE\t{state}") == "OK")
        shot = os.path.join(work, "shot.png")
        check("SCREENSHOT replies OK with the path",
              control.request(f"SCREENSHOT\t{shot}") == f"OK\t{shot}")
        check("screenshot file written", os.path.exists(shot))
        check("RESET replies OK", control.request("RESET") == "OK")
        check("RESET with bad type still refused",
              control.request("RESET\tBOGUS") == "ERROR\tInvalid reset type")
        check("EJECT_FLOPPY range check unchanged",
              control.request("EJECT_FLOPPY\t9") == "ERROR\tDrive number must be 0-3")

        check("QUIT answered", control.request("QUIT") == "OK")
        control.close()
        try:
            emu.proc.wait(30)
        except subprocess.TimeoutExpired:
            pass
        check("process exits after QUIT", emu.proc.poll() is not None)
    finally:
        emu.kill()
        shutil.rmtree(work, ignore_errors=True)

    if failures:
        sys.exit(f"{len(failures)} action check(s) failed")
    print("IPC actions behavioral test passed")


if __name__ == "__main__":
    main()
