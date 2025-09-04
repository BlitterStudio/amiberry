
#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "newcpu.h"

#include "cputest/cputest_defines.h"


typedef uae_u32 REGPARAM3 cpuop_func(uae_u32) REGPARAM;

#include "cputbl_test.h"

#define CPUFUNC(x) x##_ff
#define SET_CFLG_ALWAYS(x) SET_CFLG(x)
#define SET_NFLG_ALWAYS(x) SET_NFLG(x)

#define m68k_dreg(r,num) ((r).regs[(num)])
#define m68k_areg(r,num) (((r).regs + 8)[(num)])

int cctrue(int);

extern const int areg_byteinc[];
extern const int imm8_table[];

extern const struct cputbl op_smalltbl_90_test_ff[];
extern const struct cputbl op_smalltbl_91_test_ff[];
extern const struct cputbl op_smalltbl_92_test_ff[];
extern const struct cputbl op_smalltbl_93_test_ff[];
extern const struct cputbl op_smalltbl_94_test_ff[];
extern const struct cputbl op_smalltbl_95_test_ff[];

extern struct flag_struct regflags;

extern int movem_index1[256];
extern int movem_index2[256];
extern int movem_next[256];

void do_cycles_test(int);
int intlev(void);

uae_u16 get_word_test_prefetch(int);
uae_u16 get_wordi_test(int);

void put_byte_test(uaecptr, uae_u32);
void put_word_test(uaecptr, uae_u32);
void put_long_test(uaecptr, uae_u32);

uae_u32 get_byte_test(uaecptr);
uae_u32 get_word_test(uaecptr);
uae_u32 get_long_test(uaecptr);

uae_u32 get_disp_ea_test(uae_u32, uae_u32);
void m68k_incpci(int);
uaecptr m68k_getpci(void);
void m68k_setpci_j(uaecptr);
void m68k_do_rtsi(void);
void m68k_do_bsri(uaecptr, uae_s32);
void m68k_do_bsr_ce(uaecptr, uae_s32);
void m68k_do_bsr_ce(uaecptr oldpc, uae_s32 offset);
void m68k_do_jsr_ce(uaecptr oldpc, uaecptr dest);

void m68k_setstopped(int);
void check_t0_trace(void);

bool cpureset(void);
