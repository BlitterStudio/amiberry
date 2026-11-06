"""Adapt optional external exact vectors to the native backend interface."""

import importlib.util
from pathlib import Path


def oracle_driver(root):
    spec = importlib.util.spec_from_file_location("amitorture_fpu", Path(root) / "tools/gen_fpu.py")
    generator = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(generator)
    operations = [row[0] for row in generator.OPS]
    vectors = [vector for _, group in generator.GROUPS for vector in group]
    cases = []
    for vector in vectors:
        words = lambda name: "{" + ",".join(hex(word) + "U" for word in vector[name]) + "}"
        cases.append("{" + ",".join([
            str(vector["op"]), str(vector["tol"]), str(vector["mask"]), hex(vector["fpcr"]),
            words("src"), words("dst"), words("res"), words("res2"), words("rmask"),
            hex(vector["fpsr"]), hex(vector["fpsr_mask"]),
            '"' + vector["why"].replace("\\", "\\\\").replace('"', '\\"') + '"'
        ]) + "}")
    switches = []
    compiled_switches = []
    opmodes = {
        "fmove": 0x00, "fint": 0x01, "fsinh": 0x02, "fintrz": 0x03,
        "fsqrt": 0x04, "ftanh": 0x09, "fatan": 0x0a, "fsin": 0x0e,
        "ftan": 0x0f, "fetox": 0x10, "ftwotox": 0x11, "ftentox": 0x12,
        "flogn": 0x14, "flog10": 0x15, "flog2": 0x16, "fabs": 0x18,
        "fcosh": 0x19, "fneg": 0x1a, "fcos": 0x1d, "fgetexp": 0x1e,
        "fgetman": 0x1f, "fdiv": 0x20, "fmod": 0x21, "fadd": 0x22,
        "fmul": 0x23, "fsgldiv": 0x24, "frem": 0x25, "fscale": 0x26,
        "fsglmul": 0x27, "fsub": 0x28, "fsincos": 0x32, "fcmp": 0x38,
        "ftst": 0x3a, "fsmove": 0x40, "fssqrt": 0x41, "fdmove": 0x44,
        "fdsqrt": 0x45, "fsabs": 0x58, "fsneg": 0x5a, "fdabs": 0x5c,
        "fdneg": 0x5e, "fsdiv": 0x60, "fsadd": 0x62, "fsmul": 0x63,
        "fddiv": 0x64, "fdadd": 0x66, "fdmul": 0x67, "fssub": 0x68,
        "fdsub": 0x6c,
    }
    sizes = dict(b=6, w=4, l=0, s=1, d=5, x=2, p=3)
    for index, name in enumerate(operations):
        if name in opmodes:
            compiled = f"fpuop_arithmetic_jit(0xf2000400U | {opmodes[name]}, 0);"
        elif name.startswith("fmovecr_"):
            compiled = f"fpuop_arithmetic_jit(0xf2005c00U | 0x{name.rsplit('_', 1)[-1]}, 0);"
        else:
            size = name.split("_")[1]
            if name.endswith("_in") or name == "fmove_x_inout":
                compiled = f"fpuop_arithmetic_jit(0xf2124000U | ({sizes[size]} << 10), 0x200);"
                if name == "fmove_x_inout":
                    compiled += " fpuop_arithmetic_jit(0xf2116800U, 0x300); memory = true;"
            else:
                factor = int(name.rsplit("k", 1)[-1]) if "_k" in name else 0
                compiled = (f"fpuop_arithmetic_jit(0xf2116000U | ({sizes[size]} << 10) | {factor}, 0x300);"
                            " memory = true;")
        compiled_switches.append(f"case {index}: {compiled} break;")
        body = None
        if name.startswith("fmovecr_"):
            body = f"fpu_get_constant(&a, 0x{name.rsplit('_', 1)[-1]});"
        elif name in ("fadd", "fsub", "fmul", "fdiv"):
            body = f"fpp_{name[1:]}(&a, &b, PREC_NORMAL);"
        elif name in ("fsqrt", "fmove", "fabs", "fneg"):
            body = f"fpp_{name[1:]}(&a, &b, PREC_NORMAL);"
        elif name in ("fsadd", "fdadd", "fssub", "fdsub", "fsmul", "fdmul", "fsdiv", "fddiv",
                      "fsmove", "fdmove", "fssqrt", "fdsqrt", "fsabs", "fdabs", "fsneg", "fdneg"):
            body = f"fpp_{name[2:]}(&a, &b, {'PREC_FLOAT' if name[1] == 's' else 'PREC_DOUBLE'});"
        elif name in ("fmod", "frem"):
            body = f"fpp_{name[1:]}(&a, &b, &quotient, &sign);"
        elif name in ("fcmp", "ftst"):
            body = f"cc = a; fpp_{name[1:]}(&cc, &b); cc_separate = true;"
        elif name == "fsincos":
            body = "fpp_sincos(&a, &b, &c);"
        elif not name.startswith("fmove_"):
            body = f"fpp_{name[1:]}(&a, &b);"
        elif name.endswith("_in") or name == "fmove_x_inout":
            size = name.split("_")[1]
            conversions = {
                "b": "fpp_from_int(&b, (uae_s8)(v.src[0] >> 24));",
                "w": "fpp_from_int(&b, (uae_s16)(v.src[0] >> 16));",
                "l": "fpp_from_int(&b, (uae_s32)v.src[0]);",
                "s": "fpp_to_single(&b, v.src[0]);",
                "d": "fpp_to_double(&b, v.src[0], v.src[1]);",
                "x": "fpp_to_exten(&b, v.src[0], v.src[1], v.src[2]);",
                "p": "fpp_to_pack(&b, source, 0);",
            }
            body = conversions[size] + " normalize(&b); fpp_move(&a, &b, PREC_NORMAL);"
            if name == "fmove_x_inout":
                body += (" fpsr_make_status(); regs.fpsr &= ~0xff00U; fpp_clear_status();"
                         " fpp_from_exten(&a, &result[0], &result[1], &result[2]); memory = memory_cc = true;")
        else:
            size = name.split("_")[1]
            conversions = {
                "b": "result[0] = (uae_u32)fpp_to_int(&a, 0) << 24;",
                "w": "result[0] = (uae_u32)fpp_to_int(&a, 1) << 16;",
                "l": "result[0] = fpp_to_int(&a, 2);",
                "s": "result[0] = fpp_from_single(&a);",
                "d": "fpp_from_double(&a, &result[0], &result[1]);",
                "x": "fpp_from_exten(&a, &result[0], &result[1], &result[2]);",
                "p": f"fpp_from_pack(&a, result, {name.rsplit('k', 1)[-1] if '_k' in name else 0});"
            }
            body = conversions[size] + " memory = true;"
        switches.append(f"case {index}: {body} break;")
    return r"""
static void normalize(fpdata *value) {
    if (fpp_is_denormal(value) || fpp_is_unnormal(value))
        fpp_normalize(value);
}
static void run_generated_cases() {
    struct Vector {
        int op, tolerance, models;
        uae_u32 fpcr, src[3], dst[3], res[3], res2[3], mask[3], fpsr, fpsr_mask;
        const char *why;
    };
    const Vector vectors[] = {
""" + ",\n".join(cases) + r"""
    };
    unsigned failed = 0, tested = 0;
    for (bool compiled : {false, true}) {
    currprefs.cachesize = compiled ? 1024 : 0;
    currprefs.compfpu = compiled;
    for (int model : {68882, 68040, 68060}) {
    currprefs.fpu_model = currprefs.cpu_model = model;
    fp_init_native();
    const int model_mask = model == 68882 ? 2 : model == 68040 ? 4 : 8;
    for (const auto &v : vectors) {
        if (!(v.models & model_mask))
            continue;
        ++tested;
        fpdata a, b, c, cc;
        fpp_to_exten_fmovem(&a, v.dst[0], v.dst[1], v.dst[2]);
        fpp_to_exten_fmovem(&b, v.src[0], v.src[1], v.src[2]);
        normalize(&a);
        normalize(&b);
        fpp_from_int(&c, 0);
        mode(v.fpcr);
        uae_u32 source[3] = {v.src[0], v.src[1], v.src[2]}, result[3] = {};
        uae_u64 quotient = 0;
        uae_u8 sign = 0;
        bool memory = false, memory_cc = false, cc_separate = false;
        if (compiled) {
            regs.fp[0] = a;
            regs.fp[1] = b;
            regs.fp[2] = c;
            for (int i = 0; i < 3; ++i) {
                jit_put_long(0x200 + i * 4, v.src[i]);
                jit_put_long(0x300 + i * 4, 0);
            }
            m68k_areg(regs, 2) = 0x200;
            m68k_areg(regs, 1) = 0x300;
            regs.instruction_pc = 0x100;
            regs.pc = 0x104;
            regs.fp_unimp_pend = regs.fp_unimp_ins = 0;
            switch(v.op) {
""" + "\n".join(compiled_switches) + r"""
            default: assert(false);
            }
            assert(!regs.fp_exception);
            a = regs.fp[0];
            if (memory) {
                for (int i = 0; i < 3; ++i)
                    result[i] = read_long(0x300 + i * 4);
            }
        } else {
        switch(v.op) {
""" + "\n".join(switches) + r"""
        default: assert(false);
        }
        if (quotient || sign)
            fpsr_set_quotient(quotient, sign);
        if (!cc_separate)
            cc = a;
        if (!memory || memory_cc) {
            fpsr_set_result_always(&cc);
            fpsr_set_result(&cc);
        }
        fpsr_make_status();
        }
        const uae_u32 status = regs.fpsr;
        bool bad = ((status ^ v.fpsr) & v.fpsr_mask) != 0;
        if (!memory)
            fpp_from_exten_fmovem(&a, &result[0], &result[1], &result[2]);
        bool same = true;
        for (int i = 0; i < 3; ++i)
            same &= ((result[i] ^ v.res[i]) & v.mask[i]) == 0;
        if (!same && v.tolerance && result[0] == v.res[0]) {
            uae_u64 got = ((uae_u64)result[1] << 32) | result[2];
            uae_u64 expected = ((uae_u64)v.res[1] << 32) | v.res[2];
            same = (got > expected ? got - expected : expected - got) <= (unsigned)v.tolerance;
        }
        if (!same || bad) {
            if (failed++ < 35)
                fprintf(stderr, "%d/%s: %s\n result %08x.%08x.%08x expected %08x.%08x.%08x; status %08x expected %08x mask %08x\n",
                    model, compiled ? "compiled-helper" : "native", v.why, result[0], result[1], result[2], v.res[0], v.res[1], v.res[2],
                    status, v.fpsr, v.fpsr_mask);
        }
    }
    }
    }
    fprintf(stderr, "Amitorture native + compiled helper: %u/%u vectors failed\n", failed, tested);
    assert(!failed);
}
"""
