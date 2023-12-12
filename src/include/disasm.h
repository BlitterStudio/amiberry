
uaecptr ShowEA(void *f, uaecptr pc, uae_u16 opcode, int reg, amodes mode, wordsizes size, TCHAR *buf, uae_u32 *eaddr, int *actualea, int safemode);
uaecptr ShowEA_disp(uaecptr *pcp, uaecptr base, TCHAR *buffer, const TCHAR *name, bool pcrel);
uae_u32 m68k_disasm_2(TCHAR *buf, int bufsize, uaecptr pc, uae_u16 *bufpc, int bufpccount, uaecptr *nextpc, int cnt, uae_u32 *seaddr, uae_u32 *deaddr, uaecptr lastpc, int safemode);
void sm68k_disasm(TCHAR *instrname, TCHAR *instrcode, uaecptr addr, uaecptr *nextpc, uaecptr lastpc);
uae_u32 REGPARAM2 op_illg_1(uae_u32 opcode);
uae_u32 REGPARAM2 op_unimpl_1(uae_u32 opcode);
void disasm_init(void);

extern struct cpum2c m2cregs[];
extern const TCHAR *fpuopcodes[];
extern const TCHAR *fpsizes[];

extern int disasm_flags;
extern int disasm_min_words;
extern int disasm_max_words;
extern TCHAR disasm_hexprefix[3];

#define DISASM_FLAG_LC_MNEMO 1
#define DISASM_FLAG_LC_REG 2
#define DISASM_FLAG_LC_HEX 8
#define DISASM_FLAG_LC_SIZE 16
#define DISASM_FLAG_CC 32
#define DISASM_FLAG_EA 64
#define DISASM_FLAG_VAL 128
#define DISASM_FLAG_WORDS 256
#define DISASM_FLAG_ABSSHORTLONG 512
#define DISASM_FLAG_VAL_FORCE 1024
