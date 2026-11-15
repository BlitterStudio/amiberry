"""The inline extended-precision FADD/FSUB/FMUL (src/softfloat/softfloat_fastx80.h)
give softfloat's results and flags: tests/softfloat_fastx80_test.cpp compiled
with softfloat.cpp and the build's flags, then run.

Run: python3 tests/softfloat_fastx80_test.py
AMIBERRY_BUILD_DIR selects a CMake build with exported compile commands.
"""

import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class SoftfloatFastx80(unittest.TestCase):
    def test_fast_paths_match_softfloat(self):
        database = ROOT / os.environ.get("AMIBERRY_BUILD_DIR", "build") / "compile_commands.json"
        if not database.exists():
            self.skipTest("Configure CMake with CMAKE_EXPORT_COMPILE_COMMANDS=ON")
        entry = next(row for row in json.loads(database.read_text())
                     if row["file"].endswith("/softfloat/softfloat.cpp"))
        command = shlex.split(entry["command"])
        output = command.index("-o")
        del command[output:output + 2]
        command.remove("-c")
        command.remove(entry["file"])
        with tempfile.TemporaryDirectory(prefix=".softfloat-fastx80-", dir=ROOT / "tests") as directory:
            binary = Path(directory) / "softfloat-fastx80-test"
            environment = dict(os.environ, TMPDIR=directory, CCACHE_TEMPDIR=directory)
            subprocess.run(command + [str(ROOT / "tests/softfloat_fastx80_test.cpp"),
                           str(ROOT / "src/softfloat/softfloat.cpp"),
                           "-UNDEBUG", "-o", str(binary), "-lm"],
                           cwd=entry["directory"], env=environment, check=True, timeout=300)
            result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=300)
            if result.returncode == 77:
                self.skipTest("The fast paths need a 128-bit integer type")
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
