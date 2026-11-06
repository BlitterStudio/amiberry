"""ROM-free execution of the configured native FPU production translation unit.

Run: python3 tests/native_fpu_regressions_test.py
AMIBERRY_BUILD_DIR selects a CMake build with exported compile commands.
"""

import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
import unittest
from native_fpu_jit_fixtures import production_jit_frontend


ROOT = Path(__file__).resolve().parents[1]


def production_frontend():
    text = (ROOT / "src/fpp.cpp").read_text()

    def function(signature):
        start = text.index(signature)
        end = text.index("\n}", start) + 2
        return text[start:end]

    tables = text[text.index("struct fpp_cr_entry {"):text.index("uae_u32 xhex_nan[]")]
    flags = text[text.index("#define FPSR_CC_N "):text.index("static struct {")]
    functions = (
        "static void fpsr_set_result_always(", "static void fpsr_set_result(",
        "static void updateaccrued(", "static uae_u32 fpsr_make_status(",
        "static void fpsr_set_quotient(", "static void fpset (",
        "uae_u32 fpp_get_fpsr (", "void fpp_set_fpsr (", "bool fpu_get_constant(",
    )
    conditions = text[text.index("static const bool condition_table_040_060"):
                      text.index("int fpp_cond(")]
    return (tables + flags + "\nstatic bool support_exceptions = true;\n"
            "bool fpu_fpsr_cc_written;\nstatic uae_u32 fpsr_mask = 0x0ffffff8;\n"
            "uae_u32 xhex_nan[] = {0x7fff0000, 0xffffffff, 0xffffffff};\n"
            "static bool jit_fpu() { return currprefs.cachesize && currprefs.compfpu; }\n"
            "static int trapped_vector;\n"
            "static bool fp_exception_pending(bool) { trapped_vector = regs.fp_exp_pend; if (trapped_vector) regs.fp_exception = true; return trapped_vector != 0; }\n" +
            "\n".join(function(signature) for signature in functions) + "\n" +
            function("static uae_u32 fpsr_get_vector(") +
            "\n#define write_log(...) ((void)0)\n" +
            function("static int fpsr_set_bsun(") + "\n#undef write_log\n" +
            conditions + function("int fpp_cond("))


def production_x87_predicate():
    source = (ROOT / "src/jit/x86/codegen_x86.cpp").read_text()
    start = source.index("static inline void raw_fflags_into_flags(int r)")
    emitter = source[start:source.index("\n}", start) + 2]
    return r"""
#ifdef NATIVE_EXTENDED
#include <sys/mman.h>
#include <vector>
static std::vector<unsigned char> predicate_code;
static constexpr bool have_cmov = true;
static void emit_byte(unsigned char byte) { predicate_code.push_back(byte); }
static void usereg(int) {}
static int stackpos(int) { return 0; }
static void raw_sahf(int) { assert(false); }
""" + emitter + r"""
static void check_emitted_predicate(fpdata *operand, unsigned long expected)
{
    predicate_code.clear();
    raw_fflags_into_flags(0);
    emit_byte(0xc3);
    void *code = mmap(nullptr, 4096, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(code != MAP_FAILED);
    memcpy(code, predicate_code.data(), predicate_code.size());
    assert(mprotect(code, 4096, PROT_READ | PROT_EXEC) == 0);
    unsigned long flags;
    asm volatile("fldt %[operand]\n\tcall *%[code]\n\tpushf\n\tpop %[flags]\n\tfstp %%st(0)"
        : [flags] "=r"(flags)
        : [operand] "m"(operand->fp), [code] "r"(code)
        : "cc", "memory", "st", "st(1)");
    assert((flags & 0x45) == expected);
    assert(munmap(code, 4096) == 0);
}
#endif
"""


def build_native_test(directory, extra_driver=""):
    """Compile the production backend/decoded helper using configured build flags."""
    database = ROOT / os.environ.get("AMIBERRY_BUILD_DIR", "build") / "compile_commands.json"
    if not database.exists():
        raise FileNotFoundError("Configure CMake with CMAKE_EXPORT_COMPILE_COMMANDS=ON")
    entry = next(row for row in json.loads(database.read_text())
                 if row["file"].endswith("/fpp_native.cpp"))
    command = shlex.split(entry["command"])
    output = command.index("-o")
    del command[output:output + 2]
    command.remove("-c")
    command.remove(entry["file"])
    declarations = re.findall(r"^extern (FPP_\w+ \w+);",
                              (ROOT / "src/include/fpp.h").read_text(), re.MULTILINE)
    directory = Path(directory)
    driver = directory / "driver.cpp"
    binary = directory / "native-fpu-test"
    oracle = "static void run_generated_cases() {}"
    if os.environ.get("AMITORTURE_ROOT"):
        from native_fpu_oracle import oracle_driver
        oracle = oracle_driver(os.environ["AMITORTURE_ROOT"])
    driver.write_text(
        f'#include "{ROOT / "src/fpp_native.cpp"}"\n' +
        "\n".join(declaration + ";" for declaration in declarations) + "\n" +
        production_frontend() + "\n#include <cassert>\n" + production_jit_frontend() +
        "\n" + production_x87_predicate() + "\n" +
        "static void run_generated_cases();\n" +
        ("#define main native_regression_main\n" if extra_driver else "") +
        (ROOT / "tests/native_fpu_test.cpp").read_text() + "\n" +
        ("#undef main\n" if extra_driver else "") + oracle + "\n" + extra_driver)
    libraries = shlex.split(subprocess.check_output(
        ["pkg-config", "--libs", "sdl3" if "-DUSE_SDL3" in command else "sdl2"],
        text=True))
    environment = dict(os.environ, TMPDIR=str(directory), CCACHE_TEMPDIR=str(directory))
    subprocess.run(command + [str(driver),
                   str(ROOT / "src/softfloat/softfloat.cpp"),
                   str(ROOT / "src/softfloat/softfloat_decimal.cpp"),
                   str(ROOT / "src/softfloat/softfloat_fpsp.cpp"),
                   "-UNDEBUG", "-o", str(binary), "-lm"] + libraries,
                   cwd=entry["directory"], env=environment, check=True, timeout=120)
    return binary


class NativeFpuRegressions(unittest.TestCase):
    def test_native_registers_arithmetic_conversions_and_status(self):
        with tempfile.TemporaryDirectory(prefix=".native-fpu-", dir=ROOT / "tests") as directory:
            binary = build_native_test(directory)
            result = subprocess.run([str(binary)], timeout=30)
            if result.returncode == 77:
                self.skipTest("Native extended-register regressions require an x87 host")
            self.assertEqual(result.returncode, 0)


if __name__ == "__main__":
    unittest.main()
