
/* single   : S  8*E 23*F */
/* double   : S 11*E 52*F */
/* extended : S 15*E 64*F */
/* E = 0 & F = 0 -> 0 */
/* E = MAX & F = 0 -> Infin */
/* E = MAX & F # 0 -> NotANumber */
/* E = biased by 127 (single) ,1023 (double) ,16383 (extended) */

#define FPSR_BSUN       0x00008000
#define FPSR_SNAN       0x00004000
#define FPSR_OPERR      0x00002000
#define FPSR_OVFL       0x00001000
#define FPSR_UNFL       0x00000800
#define FPSR_DZ         0x00000400
#define FPSR_INEX2      0x00000200
#define FPSR_INEX1      0x00000100

extern void fp_init_native(void);
extern void fp_init_softfloat(void);
extern void fpsr_set_exception(uae_u32 exception);
extern void fpu_modechange(void);

#if defined(CPU_i386) || defined(CPU_x86_64)
extern void init_fpucw_x87(void);
#endif

typedef void (*FPP_ABQS)(fpdata*, fpdata*, uae_u64*, uae_u8*);
typedef void (*FPP_AB)(fpdata*, fpdata*);
typedef void (*FPP_ABP)(fpdata*, fpdata*, int);
typedef void (*FPP_A)(fpdata*);

typedef bool (*FPP_IS)(fpdata*);
typedef void (*FPP_SET_MODE)(uae_u32);
typedef void (*FPP_GET_STATUS)(uae_u32*);
typedef void (*FPP_CLEAR_STATUS)(void);

typedef void (*FPP_FROM_NATIVE)(fptype, fpdata*);
typedef void (*FPP_TO_NATIVE)(fptype*, fpdata*);

typedef void (*FPP_FROM_INT)(fpdata*,uae_s32);
typedef uae_s64 (*FPP_TO_INT)(fpdata*, int);

typedef void (*FPP_TO_SINGLE)(fpdata*, uae_u32);
typedef uae_u32 (*FPP_FROM_SINGLE)(fpdata*);

typedef void (*FPP_TO_DOUBLE)(fpdata*, uae_u32, uae_u32);
typedef void (*FPP_FROM_DOUBLE)(fpdata*, uae_u32*, uae_u32*);

typedef void (*FPP_TO_EXTEN)(fpdata*, uae_u32, uae_u32, uae_u32);
typedef void (*FPP_FROM_EXTEN)(fpdata*, uae_u32*, uae_u32*, uae_u32*);

typedef void (*FPP_PACK)(fpdata*, uae_u32*, int);

typedef const TCHAR* (*FPP_PRINT)(fpdata*,int);
typedef uae_u32 (*FPP_GET32)(void);

typedef void (*FPP_DENORMALIZE)(fpdata*,int);

extern FPP_PRINT fpp_print;

extern FPP_IS fpp_is_snan;
extern FPP_IS fpp_unset_snan;
extern FPP_IS fpp_is_nan;
extern FPP_IS fpp_is_infinity;
extern FPP_IS fpp_is_zero;
extern FPP_IS fpp_is_neg;
extern FPP_IS fpp_is_denormal;
extern FPP_IS fpp_is_unnormal;

extern FPP_GET_STATUS fpp_get_status;
extern FPP_CLEAR_STATUS fpp_clear_status;
extern FPP_SET_MODE fpp_set_mode;

extern FPP_FROM_NATIVE fpp_from_native;
extern FPP_TO_NATIVE fpp_to_native;

extern FPP_TO_INT fpp_to_int;
extern FPP_FROM_INT fpp_from_int;

extern FPP_PACK fpp_to_pack;
extern FPP_PACK fpp_from_pack;

extern FPP_TO_SINGLE fpp_to_single;
extern FPP_FROM_SINGLE fpp_from_single;
extern FPP_TO_DOUBLE fpp_to_double;
extern FPP_FROM_DOUBLE fpp_from_double;
extern FPP_TO_EXTEN fpp_to_exten;
extern FPP_FROM_EXTEN fpp_from_exten;
extern FPP_TO_EXTEN fpp_to_exten_fmovem;
extern FPP_FROM_EXTEN fpp_from_exten_fmovem;

extern FPP_A fpp_round_single;
extern FPP_A fpp_round_double;
extern FPP_A fpp_round32;
extern FPP_A fpp_round64;

extern FPP_A fpp_normalize;
extern FPP_DENORMALIZE fpp_denormalize;
extern FPP_A fpp_get_internal_overflow;
extern FPP_A fpp_get_internal_underflow;
extern FPP_A fpp_get_internal_round_all;
extern FPP_A fpp_get_internal_round;
extern FPP_A fpp_get_internal_round_exten;
extern FPP_A fpp_get_internal;
extern FPP_GET32 fpp_get_internal_grs;

extern FPP_AB fpp_int;
extern FPP_AB fpp_sinh;
extern FPP_AB fpp_intrz;
extern FPP_ABP fpp_sqrt;
extern FPP_AB fpp_lognp1;
extern FPP_AB fpp_etoxm1;
extern FPP_AB fpp_tanh;
extern FPP_AB fpp_atan;
extern FPP_AB fpp_atanh;
extern FPP_AB fpp_sin;
extern FPP_AB fpp_asin;
extern FPP_AB fpp_tan;
extern FPP_AB fpp_etox;
extern FPP_AB fpp_twotox;
extern FPP_AB fpp_tentox;
extern FPP_AB fpp_logn;
extern FPP_AB fpp_log10;
extern FPP_AB fpp_log2;
extern FPP_ABP fpp_abs;
extern FPP_AB fpp_cosh;
extern FPP_ABP fpp_neg;
extern FPP_AB fpp_acos;
extern FPP_AB fpp_cos;
extern FPP_AB fpp_getexp;
extern FPP_AB fpp_getman;
extern FPP_ABP fpp_div;
extern FPP_ABQS fpp_mod;
extern FPP_ABP fpp_add;
extern FPP_ABP fpp_mul;
extern FPP_ABQS fpp_rem;
extern FPP_AB fpp_scale;
extern FPP_ABP fpp_sub;
extern FPP_AB fpp_sgldiv;
extern FPP_AB fpp_sglmul;
extern FPP_AB fpp_cmp;
extern FPP_AB fpp_tst;
extern FPP_ABP fpp_move;
