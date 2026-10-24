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
    def __init__(self, binary, env, model="A500", options=()):
        self.output = []
        extra = [arg for option in options for arg in ("-s", option)]
        self.proc = subprocess.Popen(
            [binary, "--model", model, *extra, "-G", "--log"], env=env,
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

    def pipeline(self, lines):
        """Sends several requests in one write, so the server handles them in
        one pass, and returns their replies."""
        data = "".join(line + "\n" for line in lines).encode()
        buf = b""
        if IS_WINDOWS:
            self._pipe.write(data)
            while buf.count(b"\n") < len(lines):
                chunk = self._pipe.read(65536)
                if not chunk:
                    break
                buf += chunk
        else:
            self._sock.sendall(data)
            while buf.count(b"\n") < len(lines):
                chunk = self._sock.recv(65536)
                if not chunk:
                    break
                buf += chunk
        return buf.decode(errors="replace").splitlines()

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

        # Discovery and the active configuration.
        options = control.request("SETTINGS_LIST").split("\t")
        check("SETTINGS_LIST lists the options", options[0] == "OK" and "cpu_model" in options[1:], options[:4])
        fields = control.request("SETTINGS_DESCRIBE\tcpu_model").split("\t")
        check("SETTINGS_DESCRIBE describes an option",
              fields[0] == "OK" and "type=choice" in fields and "apply=live" in fields
              and "group=cpu" in fields and "choice=68000" in fields, fields)
        fields = control.request("SETTINGS_DESCRIBE\tgfx_monitorblankdelay").split("\t")
        check("SETTINGS_DESCRIBE gives a range in whole numbers", "max=60000" in fields, fields)
        check("SETTINGS_DESCRIBE refuses an unknown option",
              control.request("SETTINGS_DESCRIBE\tnope").startswith("ERROR"))
        # The committed CPU applies live, so the running value catches up.
        pending = control.request("SETTINGS_GET\tcpu_model")
        deadline = time.monotonic() + 3
        while control.request("SETTINGS_GET_ACTIVE\tcpu_model") != pending and time.monotonic() < deadline:
            time.sleep(0.05)
        check("SETTINGS_GET_ACTIVE reads the running configuration",
              control.request("SETTINGS_GET_ACTIVE\tcpu_model") == pending, pending)
        # One commit that changes options the display check handles in
        # different passes (the mouse capture block returns early) reaches
        # all of them.
        check("magic mouse and on-screen joystick set together",
              control.request("SETTINGS_SET\tmagic_mouse\ttrue") == "OK"
              and control.request("SETTINGS_SET\tamiberry.onscreen_joystick\ttrue") == "OK"
              and control.request("SETTINGS_COMMIT").startswith("OK\tchanged=true"))
        deadline = time.monotonic() + 3
        while time.monotonic() < deadline and (
                control.request("SETTINGS_GET_ACTIVE\tamiberry.onscreen_joystick")
                != "OK\tamiberry.onscreen_joystick=true"):
            time.sleep(0.05)
        check("both reach the running emulation",
              control.request("SETTINGS_GET_ACTIVE\tmagic_mouse") == "OK\tmagic_mouse=true"
              and control.request("SETTINGS_GET_ACTIVE\tamiberry.onscreen_joystick")
              == "OK\tamiberry.onscreen_joystick=true")
        control.request("SETTINGS_SET\tmagic_mouse\tfalse")
        control.request("SETTINGS_SET\tamiberry.onscreen_joystick\tfalse")
        control.request("SETTINGS_COMMIT")
        # A keyboard shortcut applies live: the running emulation listens
        # for the new key.
        check("screenshot shortcut set",
              control.request("SETTINGS_SET\tamiberry.screenshot\tLCtrl+F10") == "OK"
              and control.request("SETTINGS_COMMIT").startswith("OK\tchanged=true\tapply=live"))
        deadline = time.monotonic() + 3
        while time.monotonic() < deadline and (
                control.request("SETTINGS_GET_ACTIVE\tamiberry.screenshot")
                != "OK\tamiberry.screenshot=LCtrl+F10"):
            time.sleep(0.05)
        check("the shortcut reaches the running emulation",
              control.request("SETTINGS_GET_ACTIVE\tamiberry.screenshot") == "OK\tamiberry.screenshot=LCtrl+F10")
        control.request("SETTINGS_SET\tamiberry.screenshot\t")
        control.request("SETTINGS_COMMIT")
        reply = control.request("SETTINGS_APPLY\tsometime")
        check("SETTINGS_APPLY refuses an unknown class", reply.startswith("ERROR\tUsage"), reply)
        # A path longer than the short value buffer still reads back.
        long_rom = "/" + "r" * 300 + ".rom"
        check("SETTINGS_GET reads a long path",
              control.request(f"SETTINGS_SET\tkickstart_rom_file\t{long_rom}") == "OK"
              and control.request("SETTINGS_GET\tkickstart_rom_file") == f"OK\tkickstart_rom_file={long_rom}")
        control.request("SETTINGS_RESET_SESSION")

        # The mount list: session edits through the real uaehfN parser and
        # writer, committed as a whole list that needs a reset.
        base = os.path.realpath(work)
        drive = os.path.join(base, "drive dir")
        os.makedirs(drive)
        hdf = os.path.join(base, "disk,a.hdf")
        with open(hdf, "wb") as f:
            f.write(bytes(512 * 64))
        check("SETTINGS_MOUNTS starts empty", control.request("SETTINGS_MOUNTS") == "OK\tnone")
        check("SETTINGS_MOUNT_ADD a directory",
              control.request(f"SETTINGS_MOUNT_ADD\tdir,rw,DH0:Work:{drive},0") == "OK")
        check("SETTINGS_MOUNT_ADD a hardfile with a comma in its name",
              control.request(f'SETTINGS_MOUNT_ADD\thdf,rw,DH1:"{hdf}",32,1,2,512,0,,uae0') == "OK")
        reply = control.request("SETTINGS_MOUNTS")
        # The second UAE unit moves to the next free unit number, as the
        # hard drive panel numbers it.
        check("SETTINGS_MOUNTS lists both in the file's spelling",
              reply == f'OK\tdir,rw,DH0:Work:{drive},0\thdf,rw,DH1:"{hdf}",32,1,2,512,0,,uae1', reply)
        reply = control.request(f"SETTINGS_MOUNT_ADD\tdir,rw,DH0:Other:{drive},0")
        check("SETTINGS_MOUNT_ADD refuses a device name in use", reply.startswith("ERROR\tInvalid mount entry"), reply)
        reply = control.request("SETTINGS_MOUNT_ADD\tnot a mount")
        check("SETTINGS_MOUNT_ADD refuses a malformed entry", reply.startswith("ERROR\tInvalid mount entry"), reply)
        reply = control.request("SETTINGS_MOUNT_ADD\tdir,rw,DH4")
        check("SETTINGS_MOUNT_ADD refuses an entry missing its fields", reply.startswith("ERROR\tInvalid mount entry"), reply)
        reply = control.request("SETTINGS_MOUNT_ADD\tcd8,ro,:,0,0,0,2048,0,,scsi2_a4091")
        check("SETTINGS_MOUNT_ADD refuses a CD unit past the last", reply.startswith("ERROR\tInvalid mount entry"), reply)
        reply = control.request(f"SETTINGS_MOUNT_ADD\tdir,rw,DH4:Bad\rVolume:{drive},0")
        check("SETTINGS_MOUNT_ADD refuses a line break in a field", reply.startswith("ERROR\tInvalid mount entry"), reply)
        # An unknown controller name reads as the UAE controller, as the
        # loader reads it; a long one is no longer copied past its buffer.
        reply = control.request(f'SETTINGS_MOUNT_ADD\thdf,rw,DH4:"{hdf}",32,1,2,512,0,,' + "u" * 2000)
        check("SETTINGS_MOUNT_ADD reads a long unknown controller as UAE", reply == "OK", reply)
        reply = control.request("SETTINGS_MOUNTS")
        check("the long controller entry is stored as a UAE unit",
              reply.split("\t")[-1] == f'hdf,rw,DH4:"{hdf}",32,1,2,512,0,,uae2', reply)
        check("SETTINGS_MOUNT_REMOVE it again", control.request("SETTINGS_MOUNT_REMOVE\t2") == "OK")
        reply = control.request("SETTINGS_MOUNT_ADD\tdir,rw,DH5:Long:/" + "x" * 3000 + ",0")
        check("SETTINGS_MOUNT_ADD refuses an entry too long for its fields", reply.startswith("ERROR"), reply)
        if sys.platform == "darwin":
            # Each field fits, but the value is longer than the MAX_DPATH
            # (PATH_MAX, 1024) buffer the loader reads a value into, so the
            # drive would vanish after a save and a load.
            value = 'hdf,rw,DH5:"/' + "y" * 700 + '",32,1,2,512,0,"/' + "z" * 400 + '",uae0'
            reply = control.request("SETTINGS_MOUNT_ADD\t" + value)
            check("SETTINGS_MOUNT_ADD refuses an entry the loader would empty", reply.startswith("ERROR"), reply)
        check("SETTINGS_MOUNT_REPLACE",
              control.request(f"SETTINGS_MOUNT_REPLACE\t0\tdir,ro,DH0:Work2:{drive},5") == "OK")
        reply = control.request("SETTINGS_MOUNTS")
        check("SETTINGS_MOUNT_REPLACE keeps the entry's place",
              reply.startswith(f"OK\tdir,ro,DH0:Work2:{drive},5\thdf,"), reply)
        check("SETTINGS_MOUNT_REMOVE", control.request("SETTINGS_MOUNT_REMOVE\t1") == "OK")
        reply = control.request("SETTINGS_MOUNT_REMOVE\t9")
        check("SETTINGS_MOUNT_REMOVE refuses a missing entry", reply == "ERROR\tNo mount entry 9", reply)
        reply = control.request("SETTINGS_MOUNT_REMOVE\t-1")
        check("SETTINGS_MOUNT_REMOVE refuses a malformed index", reply.startswith("ERROR\tUsage"), reply)
        reply = control.request("SETTINGS_COMMIT")
        check("SETTINGS_COMMIT stores the list and asks for a reset",
              reply == "OK\tchanged=true\tapply=reset", reply)
        reply = control.request("SETTINGS_MOUNTS")
        check("the committed list is what a new session sees",
              reply == f"OK\tdir,ro,DH0:Work2:{drive},5", reply)
        # SETTINGS_APPLY reset makes the committed list the active one
        # (LIST_HARDDRIVES reads it), which the core attaches at the hard
        # reset it also performs: a reset-only change committed with it
        # shows the reset ran.
        check("the committed drive is not active before the reset",
              control.request("LIST_HARDDRIVES") == "OK\t<no harddrives mounted>")
        chip = control.request("SETTINGS_GET\tchipmem_size")
        check("a reset-only change goes with it",
              control.request("SETTINGS_SET\tchipmem_size\t4") == "OK"
              and control.request("SETTINGS_COMMIT").startswith("OK\tchanged=true\tapply=reset"))
        check("SETTINGS_APPLY reset", control.request("SETTINGS_APPLY\treset") == "OK")
        reply = control.request("LIST_HARDDRIVES")
        check("the committed drive is the active one", reply == f"OK\tunit0=dir:{drive} (readonly)", reply)
        deadline = time.monotonic() + 5
        while control.request("SETTINGS_GET_ACTIVE\tchipmem_size") != "OK\tchipmem_size=4" \
                and time.monotonic() < deadline:
            time.sleep(0.05)
        check("the hard reset ran", control.request("SETTINGS_GET_ACTIVE\tchipmem_size") == "OK\tchipmem_size=4")
        control.request(f"SETTINGS_SET\tchipmem_size\t{chip.split('=', 1)[1]}")
        check("SETTINGS_MOUNT_REMOVE the last entry", control.request("SETTINGS_MOUNT_REMOVE\t0") == "OK")
        check("SETTINGS_COMMIT the empty list",
              control.request("SETTINGS_COMMIT") == "OK\tchanged=true\tapply=reset")
        check("SETTINGS_APPLY reset again", control.request("SETTINGS_APPLY\treset") == "OK")
        check("the next reset makes the empty list active",
              control.request("LIST_HARDDRIVES") == "OK\t<no harddrives mounted>")

        # Expansion boards: a unit edited in the session over the real board
        # table, committed as a restart, and kept by the configuration
        # writer (a board enabled without a ROM is saved as ":NOROM").
        reply = control.request("SETTINGS_BOARDS")
        types = reply.split("\t")
        check("SETTINGS_BOARDS lists the expansion boards, not the accelerator",
              types[0] == "OK" and "a2091" in types and "ide_mb" in types and "cpuboard" not in types, reply[:200])
        reply = control.request("SETTINGS_BOARD_DESCRIBE\ta2091")
        check("SETTINGS_BOARD_DESCRIBE lists the unit count and fields",
              "\tunits=4\t" in reply and "\tfield=enabled\tfield=subtype\tfield=rom_file" in reply, reply)
        reply = control.request("SETTINGS_BOARD_DESCRIBE\ta2091\tsubtype")
        check("SETTINGS_BOARD_DESCRIBE describes a field",
              reply.startswith("OK\ttype=choice\tapply=restart") and "\tchoice=dmac02" in reply, reply)
        check("an A2091 unit starts disabled",
              control.request("SETTINGS_BOARD_GET\ta2091\t1\tenabled") == "OK\tenabled=false\tavailable=true")
        reply = control.request("SETTINGS_BOARD_SET\ta2091\t1\tsubtype\tdmac02")
        check("a disabled unit has no variant to set", reply.startswith("ERROR\tOption unavailable"), reply)
        check("SETTINGS_BOARD_SET enables the unit",
              control.request("SETTINGS_BOARD_SET\ta2091\t1\tenabled\ttrue") == "OK")
        check("SETTINGS_BOARD_SET picks its variant",
              control.request("SETTINGS_BOARD_SET\ta2091\t1\tsubtype\tdmac02") == "OK")
        check("SETTINGS_BOARD_SET takes an empty value (a board without its ROM)",
              control.request("SETTINGS_BOARD_SET\ta2091\t1\trom_file\t") == "OK"
              and control.request("SETTINGS_BOARD_GET\ta2091\t1\trom_file") == "OK\trom_file=\tavailable=true")
        reply = control.request("SETTINGS_BOARD_GET\ta2091\t9\tenabled")
        check("a unit past the board's count is refused", reply.startswith("ERROR\tUnknown board field"), reply)
        reply = control.request("SETTINGS_COMMIT")
        check("a changed board commits as a restart", reply == "OK\tchanged=true\tapply=restart", reply)
        saved_state = os.path.join(work, "boards.uss")
        saved_cfg = os.path.join(work, "boards.uae")
        check("a description is set for the saved state's companion configuration",
              control.request("SETTINGS_SET\tconfig_description\tBoards") == "OK"
              and control.request("SETTINGS_COMMIT") == "OK\tchanged=true\tapply=live")
        description = "OK\tconfig_description=Boards"
        check("SAVESTATE writes the configuration",
              control.request(f"SAVESTATE\t{saved_state}\t{saved_cfg}") == "OK" and os.path.exists(saved_cfg))
        check("SAVESTATE keeps the pending configuration's description",
              control.request("SETTINGS_GET\tconfig_description") == description, description)
        with open(saved_cfg, errors="replace") as f:
            lines = f.read().splitlines()
        check("the board is written with its ROM marker", "a2091-2_rom_file=:NOROM" in lines,
              [line for line in lines if line.startswith("a2091")])
        check("the board is written with its variant",
              any(line.startswith("a2091-2_rom_options=") and "subtype=dmac02" in line for line in lines),
              [line for line in lines if line.startswith("a2091")])
        check("SETTINGS_BOARD_SET disables the unit again",
              control.request("SETTINGS_BOARD_SET\ta2091\t1\tenabled\tfalse") == "OK"
              and control.request("SETTINGS_COMMIT") == "OK\tchanged=true\tapply=restart")

        # Custom controls: the controls and events are listed; without a
        # joystick connected (headless), a port has none to edit.
        reply = control.request("SETTINGS_CUSTOM_CONTROLS")
        check("SETTINGS_CUSTOM_CONTROLS lists buttons, then axes",
              reply.startswith("OK\ta=button\tb=button") and "\tleftx=axis" in reply, reply[:200])
        reply = control.request("SETTINGS_INPUT_EVENTS")
        check("SETTINGS_INPUT_EVENTS lists events by their configuration names",
              reply.startswith("OK\t") and "\tSpace\t" in reply and "\tJoy2 Fire/Mouse2 Left Button\t" in reply,
              reply[:200])
        reply = control.request("SETTINGS_CUSTOM_CONTROL_GET\t1\tnone\ta")
        check("a port without a joystick has no custom controls", reply == "ERROR\tPort 1 has no joystick", reply)
        reply = control.request("SETTINGS_CUSTOM_CONTROL_GET\t1\tshift\ta")
        check("the mode is none or hotkey", reply.startswith("ERROR\tUsage: SETTINGS_CUSTOM_CONTROL_GET"), reply)
        reply = control.request("SETTINGS_CUSTOM_CONTROL_SET\t1\thotkey\ta\t")
        check("SETTINGS_CUSTOM_CONTROL_SET takes an empty event (the control's default)",
              reply == "ERROR\tControl a is not available on port 1", reply)
        reply = control.request("SETTINGS_CUSTOM_CONTROL_SET\t1\tnone\ta\tFire")
        check("an unknown event is refused", reply == "ERROR\tUnknown input event: Fire", reply)

        # Status and settings must not serve data from before an earlier
        # command in the same pass: LOAD_CONFIG changes the pending
        # configuration directly, and the next GET_STATUS and SETTINGS_GET
        # (the session holds no uncommitted edits) must see it at once.
        loaded = os.path.join(work, "loaded.uae")
        with open(loaded, "w") as f:
            f.write("config_description=LoadedViaIPC\ncpu_model=68030\n")
        check("LOAD_CONFIG applies", control.request(f"LOAD_CONFIG\t{loaded}") == "OK")
        status = control.request("GET_STATUS")
        check("GET_STATUS is fresh after LOAD_CONFIG",
              "Config=LoadedViaIPC" in status, status)
        check("SETTINGS_GET is fresh after LOAD_CONFIG",
              control.request("SETTINGS_GET\tcpu_model") == "OK\tcpu_model=68030")
        check("the loaded description is the config_description option",
              control.request("SETTINGS_GET\tconfig_description") == "OK\tconfig_description=LoadedViaIPC")
        # A file that cannot be loaded leaves the pending configuration as it
        # was (the loader starts from the defaults).
        missing = os.path.join(work, "missing.uae")
        reply = control.request(f"LOAD_CONFIG\t{missing}")
        check("LOAD_CONFIG of a missing file fails", reply == f"ERROR\tConfiguration file not found: {missing}", reply)
        reply = control.request(f"LOAD_CONFIG\t{work}")
        check("LOAD_CONFIG of a folder fails", reply.startswith("ERROR\tNot a configuration file"), reply)
        check("a failed load keeps the pending configuration",
              control.request("SETTINGS_GET\tcpu_model") == "OK\tcpu_model=68030"
              and control.request("SETTINGS_GET\tconfig_description") == "OK\tconfig_description=LoadedViaIPC")
        # A bare name is found in the configuration directory, as the loader
        # retries there.
        configs = os.path.join(work, "home", "Configurations")
        os.makedirs(configs, exist_ok=True)
        with open(os.path.join(configs, "Games.uae"), "w") as f:
            f.write("config_description=FromConfigDir\ncpu_model=68020\n")
        check("LOAD_CONFIG finds a bare name in the configuration directory",
              control.request("LOAD_CONFIG\tGames.uae") == "OK"
              and control.request("SETTINGS_GET\tconfig_description") == "OK\tconfig_description=FromConfigDir")
        check("LOAD_CONFIG restores the earlier file",
              control.request(f"LOAD_CONFIG\t{loaded}") == "OK")
        # SAVE_CONFIG writes the pending configuration, with what a session
        # committed.
        check("the description and category are set and committed",
              control.request("SETTINGS_SET\tconfig_description\tSaved via IPC") == "OK"
              and control.request("SETTINGS_SET\tconfig_category\t Demos ") == "OK"
              and control.request("SETTINGS_COMMIT").startswith("OK\tchanged=true"))
        saved_config = os.path.join(work, "saved.uae")
        check("SAVE_CONFIG writes the file", control.request(f"SAVE_CONFIG\t{saved_config}") == "OK"
              and os.path.exists(saved_config))
        with open(saved_config, errors="replace") as f:
            lines = f.read().splitlines()
        check("the saved file has the description, the trimmed category and the machine",
              "config_description=Saved via IPC" in lines and "config_category=Demos" in lines
              and "cpu_model=68030" in lines, [line for line in lines if line.startswith(("config_", "cpu_model"))])
        reply = control.request(f"SAVE_CONFIG\t{os.path.join(work, 'no-such-folder', 'x.uae')}")
        check("SAVE_CONFIG to a folder that does not exist fails",
              reply.startswith("ERROR\tFailed to save configuration to"), reply)
        # A staged edit survives a load: the session stays open while it
        # holds uncommitted SETs.
        check("staged edit survives a load",
              control.request("SETTINGS_SET\tcpu_model\t68020") == "OK"
              and control.request(f"LOAD_CONFIG\t{loaded}") == "OK"
              and control.request("SETTINGS_GET\tcpu_model") == "OK\tcpu_model=68020")


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
        # A missing state file fails before a restore (and its reset) is
        # scheduled; master replied OK and then reset the guest.
        missing = os.path.join(work, "missing.uss")
        check("LOADSTATE of a missing file fails",
              control.request(f"LOADSTATE\t{missing}").startswith("ERROR\tState file not found"))
        # So does a file restore_state() would reject (no AmigaStateFile
        # header): its restore would reset the guest and load nothing.
        bogus = os.path.join(work, "bogus.uss")
        with open(bogus, "wb") as f:
            f.write(b"NOT A STATE FILE" * 4)
        check("LOADSTATE of a non-state file fails",
              control.request(f"LOADSTATE\t{bogus}").startswith("ERROR\tNot a state file"))
        # A save that cannot write its file fails the action: no success
        # reply, and no companion config either.
        bad_state = os.path.join(work, "no-such-dir", "state.uss")
        bad_cfg = os.path.join(work, "bad-state.uae")
        check("SAVESTATE reports a write failure",
              control.request(f"SAVESTATE\t{bad_state}\t{bad_cfg}").startswith("ERROR\t"),
              control.request(f"SAVESTATE\t{bad_state}\t{bad_cfg}"))
        check("failed SAVESTATE writes no config", not os.path.exists(bad_cfg))
        check("QUICKLOAD of an empty slot fails",
              control.request("QUICKLOAD\t9").startswith("ERROR\t"),
              control.request("QUICKLOAD\t9"))
        shot = os.path.join(work, "shot.png")
        check("SCREENSHOT replies OK with the path",
              control.request(f"SCREENSHOT\t{shot}") == f"OK\t{shot}")
        check("screenshot file written", os.path.exists(shot))
        check("RESET replies OK", control.request("RESET") == "OK")
        check("RESET with bad type still refused",
              control.request("RESET\tBOGUS") == "ERROR\tInvalid reset type")
        check("EJECT_FLOPPY range check unchanged",
              control.request("EJECT_FLOPPY\t9") == "ERROR\tDrive number must be 0-3")
        # An empty path clears the slot, exactly as writing an empty path to
        # the pending configuration always did.
        check("INSERTFLOPPY with empty path clears the slot",
              control.request("INSERTFLOPPY\t\t0") == "OK"
              and f"Floppy0={adf}" not in control.request("GET_STATUS"))
        check("INSERTCD with empty path is accepted",
              control.request("INSERTCD\t\t") == "OK")

        # Swapper inserts handled in one pass compose in order: the second
        # sees the first one's pending insert and moves the disk instead of
        # leaving it in both drives.
        swap_cfg = os.path.join(work, "swapper.uae")
        with open(swap_cfg, "w") as f:
            f.write(f"diskimage0={adf}\n")
        check("LOAD_CONFIG stores a swapper entry",
              control.request(f"LOAD_CONFIG\t{swap_cfg}") == "OK")
        replies = control.pipeline(["DISKSWAP\t0\t0", "DISKSWAP\t0\t1"])
        status = control.request("GET_STATUS")
        check("pipelined DISKSWAP replies OK", replies == ["OK", "OK"], str(replies))
        check("second DISKSWAP moves the disk",
              f"Floppy1={adf}" in status and f"Floppy0={adf}" not in status, status)

        # An empty value is a real settings value; a trailing tab keeps it.
        reply = control.request("SETTINGS_SET\tppc_model\t")
        check("SETTINGS_SET keeps a trailing empty value",
              not reply.startswith("ERROR\tUsage"), reply)
        check("SETTINGS_RESET_SESSION after the empty value",
              control.request("SETTINGS_RESET_SESSION") == "OK")

        # Folders and media lists come from the media API.
        states = os.path.join(work, "states")
        os.makedirs(states)
        for name in ("b.uss", "a.USS", "notes.txt", "tab\there.uss"):
            open(os.path.join(states, name), "w").close()
        try:  # not valid UTF-8, so not listed (APFS refuses such names)
            open(os.path.join(os.fsencode(states), b"bad\xff.uss"), "w").close()
        except OSError:
            pass
        check("SET_FOLDER SAVESTATES", control.request(f"SET_FOLDER\tSAVESTATES\t{states}") == "OK")
        reply = control.request("GET_FOLDER\tsavestates")
        check("GET_FOLDER returns the folder set",
              reply.startswith("OK\t") and reply[3:].rstrip("/") == states, reply)
        reply = control.request("LIST_MEDIA\tSAVESTATES")
        expected = "OK\t" + os.path.join(states, "a.USS") + "\t" + os.path.join(states, "b.uss")
        check("LIST_MEDIA lists the state files sorted, by extension only, without protocol delimiters",
              reply == expected, reply)
        check("LIST_MEDIA refuses an unknown list",
              control.request("LIST_MEDIA\tBOGUS").startswith("ERROR"))
        reply = control.request("SET_FOLDER\tROMS\t" + "a" * 5000)
        check("SET_FOLDER refuses a path beyond the host's limit",
              reply == "ERROR\tInvalid path: " + "a" * 5000, reply[:40])
        reply = control.request("SET_FOLDER\tROMS\t/tmp/roms ")
        check("SET_FOLDER refuses a path the settings file would trim", reply.startswith("ERROR"), reply)
        reply = control.request(f"SET_FOLDER\tROMS\t{os.path.join(states, 'b.uss')}")
        check("SET_FOLDER refuses a file", reply.startswith("ERROR"), reply)
        if os.name != "nt":
            reply = control.request(f"SET_FOLDER\tROMS\t{states}\\")
            check("SET_FOLDER refuses a trailing backslash outside Windows", reply.startswith("ERROR"), reply)
        # LIST_CONFIGS keeps answering with bare .uae names.
        configs = os.path.join(work, "configs")
        os.makedirs(configs)
        for name in ("game.uae", "other.txt"):
            open(os.path.join(configs, name), "w").close()
        check("SET_FOLDER CONFIGURATIONS",
              control.request(f"SET_FOLDER\tCONFIGURATIONS\t{configs}") == "OK")
        reply = control.request("LIST_CONFIGS")
        check("LIST_CONFIGS lists bare configuration names", reply == "OK\tgame.uae", reply)

        # SEND_KEY presses and releases Amiga keys through the key action:
        # a press needs a running emulation without the settings UI over it,
        # a release always goes through so no key stays down.
        check("SEND_KEY press (Esc)", control.request("SEND_KEY\t69\t1") == "OK")
        check("SEND_KEY release (Esc)", control.request("SEND_KEY\t69\t0") == "OK")
        check("OPEN_SETTINGS for the key checks", control.request("OPEN_SETTINGS") == "OK")
        deadline = time.monotonic() + 5
        while "Paused=true" not in control.request("GET_STATUS") and time.monotonic() < deadline:
            time.sleep(0.05)
        reply = control.request("SEND_KEY\t69\t1")
        check("SEND_KEY press under the settings UI is refused",
              reply == "ERROR\tThe settings UI is open", reply)
        check("SEND_KEY release under the settings UI", control.request("SEND_KEY\t69\t0") == "OK")
        check("RESET closes the settings UI (key checks)", control.request("RESET") == "OK")
        deadline = time.monotonic() + 5
        while "Paused=false" not in control.request("GET_STATUS") and time.monotonic() < deadline:
            time.sleep(0.05)

        # OPEN_SETTINGS opens the settings UI at the next frame. It holds the
        # emulation paused, IPC keeps answering, and a reset (an emulation
        # request, as the GUI's Reboot) closes it. Twice: closing it must
        # leave it openable again.
        for round_ in (1, 2):
            check(f"OPEN_SETTINGS (round {round_})", control.request("OPEN_SETTINGS") == "OK")
            deadline = time.monotonic() + 5
            while "Paused=true" not in control.request("GET_STATUS") and time.monotonic() < deadline:
                time.sleep(0.05)
            check(f"the settings UI holds the emulation while IPC keeps answering (round {round_})",
                  "Paused=true" in control.request("GET_STATUS"))
            check(f"OPEN_SETTINGS while it is open changes nothing (round {round_})",
                  control.request("OPEN_SETTINGS") == "OK")
            check(f"RESET from IPC closes the settings UI (round {round_})", control.request("RESET") == "OK")
            deadline = time.monotonic() + 5
            while "Paused=false" not in control.request("GET_STATUS") and time.monotonic() < deadline:
                time.sleep(0.05)
            check(f"emulation runs again (round {round_})", "Paused=false" in control.request("GET_STATUS"))

        # CHOOSE_MEDIA shows a requester over the settings UI, which holds
        # the emulation like OPEN_SETTINGS; a reset closes it. The native
        # Apple front end's requester is a modal panel that IPC cannot
        # answer (AMIBERRY_MODAL_REQUESTERS=1 skips that part).
        check("CHOOSE_MEDIA refuses an unknown request",
              control.request("CHOOSE_MEDIA\tDF7").startswith("ERROR\tUsage"))
        if not os.environ.get("AMIBERRY_MODAL_REQUESTERS"):
            check("CHOOSE_MEDIA DF0", control.request("CHOOSE_MEDIA\tDF0") == "OK")
            deadline = time.monotonic() + 5
            while "Paused=true" not in control.request("GET_STATUS") and time.monotonic() < deadline:
                time.sleep(0.05)
            check("the requester holds the emulation", "Paused=true" in control.request("GET_STATUS"))
            reply = control.request("CHOOSE_MEDIA\tCD")
            check("CHOOSE_MEDIA while a requester is open is refused",
                  reply == "ERROR\tThe settings UI is open", reply)
            check("RESET closes the requester", control.request("RESET") == "OK")
            deadline = time.monotonic() + 5
            while "Paused=false" not in control.request("GET_STATUS") and time.monotonic() < deadline:
                time.sleep(0.05)
            check("emulation runs after the requester", "Paused=false" in control.request("GET_STATUS"))

        # One restore can be pending: a second load in the same pass fails
        # instead of replacing the first after both replied OK.
        replies = control.pipeline([f"LOADSTATE\t{state}", f"LOADSTATE\t{state}"])
        check("pipelined LOADSTATE: first OK, second refused",
              replies == ["OK", "ERROR\tA state save or restore is already pending"], str(replies))

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
