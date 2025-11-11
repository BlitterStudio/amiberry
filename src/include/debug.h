 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Debugger
  *
  * (c) 1995 Bernd Schmidt
  *
  */

#ifndef UAE_DEBUG_H
#define UAE_DEBUG_H

#include "uae/types.h"

#ifndef D
#define D
#endif
#ifndef bug
#define bug write_log
#endif

int getregidx(TCHAR **c);
uae_u32 returnregx(int regid);

#ifdef DEBUGGER

#define	MAX_HIST 500
#define MAX_LINEWIDTH 10000

extern int debugging;
extern int memwatch_enabled;
extern int exception_debugging;
extern int debug_copper;
extern int debug_dma, debug_heatmap;
extern uae_u32 debug_sprite_mask;
extern int debug_bpl_mask, debug_bpl_mask_one;
extern int debugger_active;
extern int debug_illegal;
extern uae_u64 debug_illegal_mask;
extern int debugger_used;

extern void debug (void);
extern void debugger_change (int mode);
extern void activate_debugger(void);
extern void activate_debugger_new(void);
extern void activate_debugger_new_pc(uaecptr pc, int len);
extern void deactivate_debugger (void);
extern const TCHAR *debuginfo (int);
extern void record_copper (uaecptr addr, uaecptr nextaddr, uae_u16 word1, uae_u16 word2, int hpos, int vpos);
extern void record_copper_blitwait (uaecptr addr, int hpos, int vpos);
extern void record_copper_reset (void);
extern int mmu_init (int, uaecptr,uaecptr);
extern void mmu_do_hit (void);
extern void dump_aga_custom (void);
extern void memory_map_dump (void);
extern void debug_help (void);
extern uaecptr dumpmem2 (uaecptr addr, TCHAR *out, int osize);
extern void update_debug_info (void);
extern int instruction_breakpoint (TCHAR **c);
extern int debug_bankchange (int);
extern void log_dma_record (void);
extern void debug_parser (const TCHAR *cmd, TCHAR *out, uae_u32 outsize);
extern void mmu_disasm (uaecptr pc, int lines);
extern int debug_read_memory_16 (uaecptr addr);
extern int debug_peek_memory_16 (uaecptr addr);
extern int debug_read_memory_8 (uaecptr addr);
extern int debug_write_memory_16 (uaecptr addr, uae_u16 v);
extern int debug_write_memory_8 (uaecptr addr, uae_u8 v);
extern bool debug_enforcer(void);
extern int debug_safe_addr(uaecptr addr, int size);
extern void debug_invalid_reg(int reg, int size, uae_u16 val);
extern void debug_check_reg(uae_u32 addr, int write, uae_u16 v);
extern int memwatch_access_validator;
#define DEBUG_SPRINTF_ADDRESS 0xbfff00
extern bool debug_sprintf(uaecptr, uae_u32, int);
extern bool debug_get_prefetch(int idx, uae_u16 *opword);
extern void debug_hsync(void);
extern void debug_exception(int);

extern void debug_init_trainer(const TCHAR*);
extern void debug_trainer_match(void);
extern bool debug_opcode_watch;
extern bool debug_trainer_event(int evt, int state);

extern void debug_smc_clear(uaecptr addr, int size);

#define BREAKPOINT_TOTAL 20

#define BREAKPOINT_REG_Dx 0
#define BREAKPOINT_REG_Ax 8
#define BREAKPOINT_REG_PC 16
#define BREAKPOINT_REG_USP 17
#define BREAKPOINT_REG_MSP 18
#define BREAKPOINT_REG_ISP 19
#define BREAKPOINT_REG_VBR 20
#define BREAKPOINT_REG_SR 21
#define BREAKPOINT_REG_CCR 22
#define BREAKPOINT_REG_CACR 23
#define BREAKPOINT_REG_CAAR 24
#define BREAKPOINT_REG_SFC 25
#define BREAKPOINT_REG_DFC 26
#define BREAKPOINT_REG_TC 27
#define BREAKPOINT_REG_ITT0 28
#define BREAKPOINT_REG_ITT1 29
#define BREAKPOINT_REG_DTT0 30
#define BREAKPOINT_REG_DTT1 31
#define BREAKPOINT_REG_BUSC 32
#define BREAKPOINT_REG_PCR 33
#define BREAKPOINT_REG_FPIAR 34
#define BREAKPOINT_REG_FPCR 35
#define BREAKPOINT_REG_FPSR 36
#define BREAKPOINT_REG_END 37

#define BREAKPOINT_CMP_EQUAL 0
#define BREAKPOINT_CMP_NEQUAL 1
#define BREAKPOINT_CMP_SMALLER_EQUAL 2
#define BREAKPOINT_CMP_LARGER_EQUAL 3
#define BREAKPOINT_CMP_SMALLER 2
#define BREAKPOINT_CMP_LARGER 3
#define BREAKPOINT_CMP_RANGE 4
#define BREAKPOINT_CMP_NRANGE 5

struct breakpoint_node {
	uae_u32 value1;
	uae_u32 value2;
	uae_u32 mask;
	int type;
	int oper;
	bool opersigned;
	int enabled;
	int cnt;
	int chain;
};
extern struct breakpoint_node bpnodes[BREAKPOINT_TOTAL];

#define MW_MASK_CPU_I			0x00000001
#define MW_MASK_CPU_D_R			0x00000002
#define MW_MASK_CPU_D_W			0x00000004
#define MW_MASK_BLITTER_A		0x00000008
#define MW_MASK_BLITTER_B		0x00000010
#define MW_MASK_BLITTER_C		0x00000020
#define MW_MASK_BLITTER_D_N		0x00000040
#define MW_MASK_BLITTER_D_L		0x00000080
#define MW_MASK_BLITTER_D_F		0x00000100
#define MW_MASK_COPPER			0x00000200
#define MW_MASK_DISK			0x00000400
#define MW_MASK_AUDIO_0			0x00000800
#define MW_MASK_AUDIO_1			0x00001000
#define MW_MASK_AUDIO_2			0x00002000
#define MW_MASK_AUDIO_3			0x00004000
#define MW_MASK_BPL_0			0x00008000
#define MW_MASK_BPL_1			0x00010000
#define MW_MASK_BPL_2			0x00020000
#define MW_MASK_BPL_3			0x00040000
#define MW_MASK_BPL_4			0x00080000
#define MW_MASK_BPL_5			0x00100000
#define MW_MASK_BPL_6			0x00200000
#define MW_MASK_BPL_7			0x00400000
#define MW_MASK_SPR_0			0x00800000
#define MW_MASK_SPR_1			0x01000000
#define MW_MASK_SPR_2			0x02000000
#define MW_MASK_SPR_3			0x04000000
#define MW_MASK_SPR_4			0x08000000
#define MW_MASK_SPR_5			0x10000000
#define MW_MASK_SPR_6			0x20000000
#define MW_MASK_SPR_7			0x40000000
#define MW_MASK_NONE			0x80000000
#define MW_MASK_ALL				(MW_MASK_NONE - 1)

#define MEMWATCH_TOTAL 20
struct memwatch_node {
	uaecptr addr;
	int size;
	int rwi;
	uae_u32 val, val_mask, access_mask;
	int val_size, val_enabled;
	int mustchange;
	uae_u32 modval;
	int modval_written;
	int frozen;
	uae_u32 reg;
	uaecptr pc;
	bool nobreak;
	bool reportonly;
	int bus_error;
};
extern struct memwatch_node mwnodes[MEMWATCH_TOTAL];

extern void memwatch_dump2 (TCHAR *buf, int bufsize, int num);

void debug_getpeekdma_chipram(uaecptr addr, uae_u32 mask, int reg);
void debug_getpeekdma_value(uae_u32);
void debug_getpeekdma_value_long(uae_u32, int);
uae_u32 debug_putpeekdma_chipram(uaecptr addr, uae_u32 v, uae_u32 mask, int reg);
uae_u32 debug_putpeekdma_chipset(uaecptr addr, uae_u32 v, uae_u32 mask, int reg);
void debug_lgetpeek(uaecptr addr, uae_u32 v);
void debug_wgetpeek(uaecptr addr, uae_u32 v);
void debug_bgetpeek(uaecptr addr, uae_u32 v);
void debug_bputpeek(uaecptr addr, uae_u32 v);
void debug_wputpeek(uaecptr addr, uae_u32 v);
void debug_lputpeek(uaecptr addr, uae_u32 v);

uae_u32 get_byte_debug (uaecptr addr);
uae_u32 get_word_debug (uaecptr addr);
uae_u32 get_long_debug (uaecptr addr);
uae_u32 get_ilong_debug (uaecptr addr);
uae_u32 get_iword_debug (uaecptr addr);

uae_u32 get_byte_cache_debug(uaecptr addr, bool *cached);
uae_u32 get_word_cache_debug(uaecptr addr, bool *cached);
uae_u32 get_long_cache_debug(uaecptr addr, bool *cached);

enum debugtest_item { DEBUGTEST_BLITTER, DEBUGTEST_KEYBOARD, DEBUGTEST_FLOPPY, DEBUGTEST_MAX };
void debugtest (enum debugtest_item, const TCHAR *, ...);

struct peekdma
{
	int type;
	uaecptr addr;
	uae_u32 v;
	uae_u32 mask;
	int reg;
	int ptrreg;
};
extern struct peekdma peekdma_data;

struct dma_rec
{
	int hpos, vpos[2];
	int frame;
	uae_u32 tick;
	int dhpos[2];
    uae_u16 reg;
    uae_u64 dat;
	uae_u16 size;
    uae_u32 addr;
    uae_u32 evt;
	uae_u32 agnus_evt, agnus_evt_changed;
	uae_u32 denise_evt[2], denise_evt_changed[2];
	uae_u32 evtdata;
	bool evtdataset;
    uae_s16 type;
	uae_u16 extra;
	uae_s8 intlev, ipl, ipl2;
	uae_u16 cf_reg, cf_dat, cf_addr;
	int ciareg;
	int ciamask;
	bool ciarw;
	int ciaphase;
	uae_u16 ciavalue;
	uaecptr miscaddr;
	uae_u32 miscval;
	int miscsize;
	bool end;
	bool cs, hs, vs;
};

extern struct dma_rec *last_dma_rec;

#define DENISE_EVENT_VB 1
#define DENISE_EVENT_HB 2
#define DENISE_EVENT_BURST 4
#define DENISE_EVENT_HDIW 8
#define DENISE_EVENT_BPL1DAT_HDIW 16
#define DENISE_EVENT_COPIED 0x40000000
#define DENISE_EVENT_UNKNOWN 0x80000000

#define AGNUS_EVENT_HW_HS 1
#define AGNUS_EVENT_HW_VS 2
#define AGNUS_EVENT_HW_CS 4
#define AGNUS_EVENT_HW_VB 8
#define AGNUS_EVENT_PRG_HS 16
#define AGNUS_EVENT_PRG_VS 32
#define AGNUS_EVENT_PRG_CS 64
#define AGNUS_EVENT_PRG_VB 128
#define AGNUS_EVENT_VDIW 256
#define AGNUS_EVENT_BPRUN 512
#define AGNUS_EVENT_VE 1024
#define AGNUS_EVENT_P_VE 2048
#define AGNUS_EVENT_HB 4096
#define AGNUS_EVENT_BPRUN2 8192

#define DMA_EVENT_BLITIRQ 1
#define DMA_EVENT_BLITFINALD 2
#define DMA_EVENT_BLITSTARTFINISH 4
#define DMA_EVENT_BPLFETCHUPDATE 8
#define DMA_EVENT_COPPERWAKE 16
#define DMA_EVENT_CPUIRQ 32
#define DMA_EVENT_INTREQ 64
#define DMA_EVENT_COPPERWANTED 128
#define DMA_EVENT_COPPERWAKE2 256
#define DMA_EVENT_CPUBLITTERSTEAL 512
#define DMA_EVENT_CPUBLITTERSTOLEN 1024
#define DMA_EVENT_COPPERSKIP 2048
#define DMA_EVENT_DDFSTRT 4096
#define DMA_EVENT_DDFSTOP 8192
#define DMA_EVENT_DDFSTOP2 16384
#define DMA_EVENT_SPECIAL 32768
#define DMA_EVENT_IPL			0x00010000
#define DMA_EVENT_IPLSAMPLE		0x00020000
#define DMA_EVENT_COPPERUSE		0x00040000
#define DMA_EVENT_MODADD		0x00080000
#define DMA_EVENT_LOF			0x00100000
#define DMA_EVENT_LOL			0x00200000

#define DMA_EVENT_CIAA_IRQ		0x08000000
#define DMA_EVENT_CIAB_IRQ		0x10000000
#define DMA_EVENT_CPUSTOP		0x20000000
#define DMA_EVENT_CPUSTOPIPL	0x40000000
#define DMA_EVENT_CPUINS		0x80000000

#define DMARECORD_REFRESH 1
#define DMARECORD_CPU 2
#define DMARECORD_COPPER 3
#define DMARECORD_AUDIO 4
#define DMARECORD_BLITTER 5
#define DMARECORD_BITPLANE 6
#define DMARECORD_SPRITE 7
#define DMARECORD_DISK 8
#define DMARECORD_UHRESBPL 9
#define DMARECORD_UHRESSPR 10
#define DMARECORD_CONFLICT 11
#define DMARECORD_MAX 12

extern void record_dma_read(uae_u16 reg, uae_u32 addr, int type, int extra);
extern void record_dma_write(uae_u16 reg, uae_u32 v, uae_u32 addr, int type, int extra);
extern void record_dma_read_value(uae_u32 v);
extern void record_dma_read_value_pos(uae_u32 v);
extern void record_dma_read_value_wide(uae_u64 v, bool quad);
extern void record_dma_replace( int type, int extra);
extern void record_dma_reset(int);
extern void record_dma_event_agnus(uae_u32 evt, bool onoff);
extern void record_dma_event_denise(struct dma_rec *rd, int h, uae_u32 evt, bool onoff);
extern void record_dma_event(uae_u32 evt);
extern void record_dma_event_data(uae_u32 evt, uae_u32 data);
extern void record_dma_clear(void);
extern bool record_dma_check(void);
extern void record_cia_access(int r, int mask, uae_u16 value, bool rw, int phase);
extern void record_rom_access(uaecptr, uae_u32 value, int size, bool rw);
extern void record_dma_ipl(void);
extern void record_dma_ipl_sample(void);
extern void debug_mark_refreshed(uaecptr);
extern void debug_draw(uae_u8 *buf, uae_u8 *genlock, int line, int width, int height, uae_u32 *xredcolors, uae_u32 *xgreencolors, uae_u32 *xbluescolors);
extern struct dma_rec *record_dma_next_cycle(int hpos, int vpos, int vvpos);

#define TRACE_SKIP_INS 1
#define TRACE_MATCH_PC 2
#define TRACE_MATCH_INS 3
#define TRACE_RANGE_PC 4
#define TRACE_SKIP_LINE 5
#define TRACE_RAM_PC 6
#define TRACE_CHECKONLY 10

#else

STATIC_INLINE void activate_debugger (void) { };

#endif /* DEBUGGER */

#endif /* UAE_DEBUG_H */
