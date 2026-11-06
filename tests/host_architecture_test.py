"""Verify the target-ABI policy without target system libraries."""

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class HostArchitectureTests(unittest.TestCase):
    def test_cmake_policy_uses_pointer_size_not_processor_name(self):
        for arch in ("i386", "armv7", "powerpc", "riscv32", "x86_64", "aarch64",
                     "powerpc64", "riscv64", "loongarch64"):
            for width in (4, 8, 16):
                with self.subTest(arch=arch, width=width):
                    result = subprocess.run(
                        ["cmake", f"-DCMAKE_SIZEOF_VOID_P={width}",
                         f"-DCMAKE_SYSTEM_PROCESSOR={arch}", "-P",
                         str(ROOT / "cmake/HostArchitecture.cmake")],
                        capture_output=True, text=True,
                    )
                    self.assertEqual(result.returncode == 0, width == 8,
                                     result.stdout + result.stderr)
                    if width != 8:
                        self.assertIn("64-bit target ABI", result.stderr)

    @unittest.skipUnless(shutil.which("clang"), "Clang cross-target frontend unavailable")
    def test_compiler_guard_and_surviving_host_detection(self):
        sysdeps = (ROOT / "src/include/sysdeps.h").read_text()
        start = sysdeps.index("#if defined(CPU_AARCH64)")
        end = sysdeps.index("\n#ifdef _WIN32", start)
        detection = sysdeps[start:end]
        targets = {
            "x86_64-linux-gnu": "CPU_x86_64",
            "aarch64-linux-gnu": "CPU_AARCH64",
            "aarch64-windows-msvc": "CPU_AARCH64",
            "powerpc64-linux-gnu": "CPU_powerpc",
            "riscv64-linux-gnu": "CPU_riscv64",
            "loongarch64-linux-gnu": "CPU_loongarch64",
            "i386-linux-gnu": None,
            "armv7-linux-gnueabihf": None,
            "powerpc-linux-gnu": None,
            "riscv32-linux-gnu": None,
            "x86_64-linux-gnux32": None,
        }
        for target, symbol in targets.items():
            for language, extension, standard in (("c", "c", "c11"),
                                                   ("c++", "cpp", "c++17")):
                with self.subTest(target=target, language=language):
                    with tempfile.TemporaryDirectory() as directory:
                        source = Path(directory) / f"guard.{extension}"
                        text = '#include "host_arch.h"\n'
                        if symbol:
                            text += detection + f"\n#ifndef {symbol}\n#error wrong host\n#endif\n"
                        source.write_text(text)
                        result = subprocess.run(
                            ["clang", f"--target={target}", "-ffreestanding",
                             f"-std={standard}", "-fsyntax-only", "-x", language,
                             "-I", str(ROOT / "src/include"), str(source)],
                            capture_output=True, text=True,
                        )
                        self.assertEqual(result.returncode == 0, symbol is not None,
                                         result.stderr)
                        if symbol is None:
                            self.assertIn("64-bit target ABI", result.stderr)

    @unittest.skipUnless(shutil.which("clang") and shutil.which("clang++"),
                         "Clang cross-target frontend unavailable")
    def test_libretro_rejects_32_bit_compiler_abi(self):
        result = subprocess.run(
            ["make", "-C", str(ROOT / "libretro"), "-n",
             "CXX=clang++ --target=i386-linux-gnu",
             "CC=clang --target=i386-linux-gnu", "ARCH=x86_64"],
            capture_output=True, text=True,
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("64-bit target ABI", result.stderr)

    def test_libretro_normalizes_aliases_without_misclassifying_other_hosts(self):
        for arch in ("aarch64", "arm64", "ARM64", "x86_64", "riscv64",
                     "loongarch64", "ppc64"):
            with self.subTest(arch=arch):
                result = subprocess.run(
                    ["make", "-s", "-C", str(ROOT / "libretro"),
                     "-f", "Makefile", "-f", "-", "show-host-policy",
                     f"ARCH={arch}", "USE_86BOX=1", "CXX=g++"],
                    input="show-host-policy:\n\t@echo '$(ARCH)|$(FLAGS)|$(86BOX_DYNAREC_BACKEND)'\n",
                    capture_output=True, text=True,
                )
                self.assertEqual(result.returncode, 0, result.stderr)
                canonical, flags, backend = result.stdout.strip().split("|")
                if arch in ("aarch64", "arm64", "ARM64"):
                    self.assertEqual(canonical, "aarch64")
                    self.assertIn("-DCPU_AARCH64", flags)
                    self.assertEqual(backend, "AARCH64")
                elif arch == "x86_64":
                    self.assertIn("-DCPU_X86_64", flags)
                    self.assertEqual(backend, "X86_64")
                else:
                    self.assertNotIn("-DCPU_X86_64", flags)
                    self.assertNotIn("-DCPU_AARCH64", flags)
                    self.assertEqual(backend, "")

    @unittest.skipUnless(shutil.which("clang") and shutil.which("clang++"),
                         "Clang cross-target frontend unavailable")
    def test_cmake_rejects_real_32_bit_abis_before_dependencies(self):
        for target in ("i386-linux-gnu", "armv7-linux-gnueabihf",
                       "powerpc-linux-gnu", "riscv32-linux-gnu"):
            with self.subTest(target=target), tempfile.TemporaryDirectory() as directory:
                result = subprocess.run(
                    ["cmake", "-S", str(ROOT), "-B", directory,
                     "-DCMAKE_C_COMPILER=clang", "-DCMAKE_CXX_COMPILER=clang++",
                     f"-DCMAKE_C_COMPILER_TARGET={target}",
                     f"-DCMAKE_CXX_COMPILER_TARGET={target}",
                     "-DCMAKE_SYSTEM_NAME=Linux",
                     "-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY"],
                    capture_output=True, text=True,
                )
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("64-bit target ABI", result.stderr,
                              result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
