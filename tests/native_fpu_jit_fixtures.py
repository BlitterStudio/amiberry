"""Production decoded-EA FPU helpers with ROM-free memory/fault fixtures."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def production_jit_frontend():
    source = (ROOT / "src/fpp.cpp").read_text()

    def function(signature):
        start = source.index(signature)
        return source[start:source.index("\n}", start) + 2]

    stubs = r"""
#undef SUPPORT_MMU
#define SUPPORT_MMU 0
#define write_log(...) ((void)0)
#define _T(x) x
static bool support_denormals = true, fpu_mmu_fixup;
static uae_u32 fpcr_mask = 0xfff0;
struct mmufixup mmufixup[2];
static uae_u8 guest_memory[4096];
static void jit_setpc(uaecptr pc) {
    assert(pc < sizeof(guest_memory));
    regs.pc = pc;
    regs.pc_p = regs.pc_oldp = guest_memory + pc;
}
#define m68k_setpc_j jit_setpc
static uae_u32 read_byte(uaecptr ad) {
    assert(ad < sizeof(guest_memory));
    return guest_memory[ad];
}
static void jit_put_byte(uaecptr ad, uae_u32 v) {
    assert(ad < sizeof(guest_memory));
    guest_memory[ad] = v;
}
static uae_u32 read_word(uaecptr ad) { return (read_byte(ad) << 8) | read_byte(ad + 1); }
static uae_u32 read_long(uaecptr ad) { return (read_word(ad) << 16) | read_word(ad + 2); }
static void jit_put_word(uaecptr ad, uae_u32 v) { jit_put_byte(ad, v >> 8); jit_put_byte(ad + 1, v); }
static void jit_put_long(uaecptr ad, uae_u32 v) { jit_put_word(ad, v >> 16); jit_put_word(ad + 2, v); }
static uae_u32 next_word() { assert(false); return 0; }
static uae_u32 next_long() { assert(false); return 0; }
uae_u32 (*x_cp_get_byte)(uaecptr) = read_byte;
uae_u32 (*x_cp_get_word)(uaecptr) = read_word;
uae_u32 (*x_cp_get_long)(uaecptr) = read_long;
void (*x_cp_put_byte)(uaecptr, uae_u32) = jit_put_byte;
void (*x_cp_put_word)(uaecptr, uae_u32) = jit_put_word;
void (*x_cp_put_long)(uaecptr, uae_u32) = jit_put_long;
uae_u32 (*x_cp_next_iword)() = next_word;
uae_u32 (*x_cp_next_ilong)() = next_long;
uae_u32 (REGPARAM3 *x_cp_get_disp_ea_020)(uae_u32, int) = nullptr;
template<class... A> static bool fault_if_no_fpu(A...) { return false; }
template<class... A> static bool fault_if_no_6888x(A...) { return false; }
template<class... A> static bool fault_if_unimplemented_680x0(A...) { return false; }
template<class... A> static bool fault_if_nonexisting_opmode(A...) { return false; }
template<class... A> static bool fault_if_no_packed_support(A...) { return false; }
static bool fault_if_60() { return false; }
static bool if_no_fpu() { return false; }
template<class... A> static void fp_unimp_datatype(A...) { assert(false); }
template<class... A> static void fpu_noinst(A...) { assert(false); }
static void trace_t0_68040() {}
static bool fp_arithmetic(fpdata *, fpdata *, int);
static bool fpsr_check_arithmetic_exception(
    uae_u32, fpdata *, uae_u32, uae_u16, uae_u32, bool, uaecptr pc) {
    const uae_u32 exceptions = regs.fpsr & regs.fpcr & 0xff00;
    if (exceptions) {
        regs.fp_exp_pend = fpsr_get_vector(exceptions);
        regs.fpiar = pc;
    }
    return exceptions != 0;
}
"""
    functions = (
        "STATIC_INLINE bool fp_is_dyadic(",
        "static void fpsr_clear_status(", "static void fpsr_get_quotient(",
        "static void maybe_set_fpiar(", "uae_u32 fpp_get_fpcr(",
        "void fpp_set_fpcr (", "void fpp_set_fpiar(", "uae_u32 fpp_get_fpiar(",
        "static bool normalize_or_fault_if_no_denormal_support(",
        "static bool normalize_or_fault_if_no_denormal_support_dst(",
        "static bool fault_if_68040_integer_nonmaskable(",
        "static int get_fp_value(", "static int put_fp_value2(",
        "static int get_fp_ad (", "static int put_fp_value(",
        "static uaecptr fmovem2mem (", "static uaecptr fmovem2fpp (",
        "static bool fp_arithmetic(fpdata *src, fpdata *dst, int extra)\n{",
        "static void fpuop_arithmetic2 (", "void fpuop_arithmetic (",
        "void REGPARAM3 fpuop_arithmetic_jit(",
    )
    return stubs + "\n".join(function(signature) for signature in functions) + r"""
#undef m68k_setpc_j
#undef write_log
"""
