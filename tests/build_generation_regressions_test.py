"""Check generated headers using the production CMake configuration."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class BuildGenerationRegressions(unittest.TestCase):
    def test_mt32emu_header_generation_is_content_aware(self):
        with tempfile.TemporaryDirectory() as directory:
            build = Path(directory)

            def configure():
                result = subprocess.run(
                    [
                        "cmake", "-S", str(ROOT / "external/mt32emu"),
                        "-B", str(build), "-Dlibmt32emu_SHARED=OFF",
                    ],
                    capture_output=True, text=True,
                )
                self.assertEqual(
                    result.returncode, 0, result.stdout + result.stderr,
                )

            configure()
            header = build / "include/mt32emu.h"
            expected = b"#include <mt32emu/mt32emu.h>"
            self.assertEqual(header.read_bytes(), expected)

            timestamp = 1_600_000_000_000_000_000
            os.utime(header, ns=(timestamp, timestamp))
            unchanged_timestamp = header.stat().st_mtime_ns
            configure()
            self.assertEqual(header.read_bytes(), expected)
            self.assertEqual(header.stat().st_mtime_ns, unchanged_timestamp)

            header.write_text("#error stale header\n")
            configure()
            self.assertEqual(header.read_bytes(), expected)

            header.unlink()
            configure()
            self.assertEqual(header.read_bytes(), expected)


if __name__ == "__main__":
    unittest.main()
