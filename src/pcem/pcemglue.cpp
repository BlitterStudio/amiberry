#include "sysconfig.h"
#include "sysdeps.h"

#include "uae.h"
#include "ibm.h"
#include "pit.h"
#include "pic.h"
#include "cpu.h"
#include "device.h"
#include "model.h"
#include "x86.h"
#include "x86_ops.h"
#include "codegen.h"
#include "timer.h"
#include "sound.h"
#include "rom.h"
#include "thread.h"

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef WITH_MIDI
#include "midi.h"
#endif

#include "pcemglue.h"

#include "threaddep/thread.h"
#include "machdep/maccess.h"
#include "gfxboard.h"
#include "uae/time.h"

#include "video.h"
#include "vid_svga.h"

CPU_STATE cpu_state;

PIT pit, pit2;
PIC pic, pic2;
dma_t dma[8];
int AT;

int ppispeakon;
int gated, speakval, speakon;

PPI ppi;

int codegen_flags_changed;
uint32_t codegen_endpc;
int codegen_in_recompile;
int codegen_flat_ds, codegen_flat_ss;
uint32_t recomp_page;
uint16_t *codeblock_hash;
codeblock_t *codeblock;

void codegen_reset(void)
{
}
void codegen_block_init(unsigned int)
{
}
void codegen_block_remove()
{
}
void codegen_block_start_recompile(struct codeblock_t *)
{
}
void codegen_block_end_recompile(struct codeblock_t *)
{
}
void codegen_block_end()
{
}
void codegen_generate_call(uint8_t opcode, OpFn op, uint32_t fetchdat, uint32_t new_pc, uint32_t old_pc)
{
}
void codegen_check_flush(struct page_t *page, uint64_t mask, uint32_t phys_addr)
{
}
void codegen_flush(void)
{
}
void codegen_init(void)
{
}

void pclog(char const *format, ...)
{
	va_list parms;
	va_start(parms, format);
	char buf[256];
	vsnprintf(buf, sizeof buf, format, parms);
	write_log("%s", buf);
	va_end(parms);
}
#ifdef DEBUGGER
extern void activate_debugger(void);
#endif
void fatal(char const *format, ...)
{
	va_list parms;
	va_start(parms, format);
	char buf[256];
	vsnprintf(buf, sizeof buf, format, parms);
	write_log("PCEMFATAL: %s", buf);
	va_end(parms);
#ifdef DEBUGGER
	activate_debugger();
#endif
}

void video_updatetiming(void)
{
	write_log(_T("video_updatetiming\n"));
}

void device_speed_changed(void)
{
	write_log(_T("device_speed_changed\n"));
}

int model;
int cpuspeed;
int insc;
int hasfpu;
int romset;
int nmi_mask;
uint32_t rammask;
uintptr_t *readlookup2;
uintptr_t *writelookup2;
uint16_t flags, eflags;
int writelookup[256], writelookupp[256];
int readlookup[256], readlookupp[256];
int readlnext;
int writelnext;
uint32_t olddslimit, oldsslimit, olddslimitw, oldsslimitw;
int pci_burst_time, pci_nonburst_time;
uint32_t oxpc;
char logs_path[512];
uint32_t ealimit, ealimitw;
int MCA;
struct svga_t *mb_vga;

uint32_t x87_pc_off, x87_op_off;
uint16_t x87_pc_seg, x87_op_seg;

int xi8088_bios_128kb(void)
{
	return 0;
}

uint8_t portin(uint16_t portnum);
uint8_t inb(uint16_t port)
{
	return portin(port);
}

void portout(uint16_t, uint8_t);
void outb(uint16_t port, uint8_t v)
{
	portout(port, v);
}
uint16_t portin16(uint16_t portnum);
uint16_t inw(uint16_t port)
{
	return portin16(port);
}
void portout16(uint16_t portnum, uint16_t value);
void outw(uint16_t port, uint16_t val)
{
	portout16(port, val);
}
uint32_t portin32(uint16_t portnum);
uint32_t inl(uint16_t port)
{
	return portin32(port);
}
void portout32(uint16_t portnum, uint32_t value);
void outl(uint16_t port, uint32_t val)
{
	portout32(port, val);
}


void model_init(void)
{

}

int AMSTRAD;
int TANDY;
int keybsenddelay;
int mouse_buttons;
int mouse_type;
uint8_t pcem_key[272];

int mouse_get_type(int mouse_type)
{
	return 0;
}

void t3100e_notify_set(unsigned char v)
{

}
void t3100e_display_set(unsigned char v)
{

}
void t3100e_turbo_set(unsigned char v)
{

}
void t3100e_mono_set(unsigned char v)
{

}
uint8_t t3100e_display_get()
{
	return 0;
}
uint8_t t3100e_config_get()
{
	return 0;
}
uint8_t t3100e_mono_get()
{
	return 0;
}
void xi8088_turbo_set(unsigned char v)
{

}
uint8_t xi8088_turbo_get()
{
	return 0;
}

void ps2_cache_clean(void)
{

}

int video_is_mda()
{
	return 0;
}

static FPU fpus_none[] =
{
	{ "None", "none", FPU_NONE },
	{ NULL, NULL, 0 }
};
static FPU fpus_8088[] =
{
	{ "None", "none", FPU_NONE },
	{ "8087", "8087", FPU_8087 },
	{ NULL, NULL, 0 }
};
static FPU fpus_80286[] =
{
	{ "None", "none", FPU_NONE },
	{ "287", "287", FPU_287 },
	{ "287XL", "287xl", FPU_287XL },
	{ NULL, NULL, 0 }
};
static FPU fpus_80386[] =
{
	{ "None", "none", FPU_NONE },
	{ "387", "387", FPU_387 },
	{ NULL, NULL, 0 }
};
static FPU fpus_builtin[] =
{
	{ "Built-in", "builtin", FPU_BUILTIN },
	{ NULL, NULL, 0 }
};

CPU cpus_8088[] =
{
	/*8088 standard*/
	{ "8088/4.77", CPU_8088, fpus_8088, 0, 4772728, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
	{ "8088/7.16", CPU_8088, fpus_8088, 1, 14318184 / 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
	{ "8088/8", CPU_8088, fpus_8088, 1, 8000000, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
	{ "8088/10", CPU_8088, fpus_8088, 2, 10000000, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
	{ "8088/12", CPU_8088, fpus_8088, 3, 12000000, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
	{ "8088/16", CPU_8088, fpus_8088, 4, 16000000, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
	{ "", -1, 0, 0, 0, 0 }
};

CPU cpus_286[] =
{
	/*286*/
	{ "286/6", CPU_286, fpus_80286, 0, 6000000, 1, 0, 0, 0, 0, 0, 2, 2, 2, 2, 1 },
	{ "286/8", CPU_286, fpus_80286, 1, 8000000, 1, 0, 0, 0, 0, 0, 2, 2, 2, 2, 1 },
	{ "286/10", CPU_286, fpus_80286, 2, 10000000, 1, 0, 0, 0, 0, 0, 2, 2, 2, 2, 1 },
	{ "286/12", CPU_286, fpus_80286, 3, 12000000, 1, 0, 0, 0, 0, 0, 3, 3, 3, 3, 2 },
	{ "286/16", CPU_286, fpus_80286, 4, 16000000, 1, 0, 0, 0, 0, 0, 3, 3, 3, 3, 2 },
	{ "286/20", CPU_286, fpus_80286, 5, 20000000, 1, 0, 0, 0, 0, 0, 4, 4, 4, 4, 3 },
	{ "286/25", CPU_286, fpus_80286, 6, 25000000, 1, 0, 0, 0, 0, 0, 4, 4, 4, 4, 3 },
	{ "", -1, 0, 0, 0, 0 }
};

CPU cpus_i386SX[] =
{
	/*i386SX*/
	{ "i386SX/16", CPU_386SX, fpus_80386, 0, 16000000, 1, 0, 0x2308, 0, 0, 0, 3, 3, 3, 3, 2 },
	{ "i386SX/20", CPU_386SX, fpus_80386, 1, 20000000, 1, 0, 0x2308, 0, 0, 0, 4, 4, 3, 3, 3 },
	{ "i386SX/25", CPU_386SX, fpus_80386, 2, 25000000, 1, 0, 0x2308, 0, 0, 0, 4, 4, 3, 3, 3 },
	{ "i386SX/33", CPU_386SX, fpus_80386, 3, 33333333, 1, 0, 0x2308, 0, 0, 0, 6, 6, 3, 3, 4 },
	{ "", -1, 0, 0, 0 }
};

CPU cpus_i386DX[] =
{
	/*i386DX*/
	{ "i386DX/16", CPU_386DX, fpus_80386, 0, 16000000, 1, 0, 0x0308, 0, 0, 0, 3, 3, 3, 3, 2 },
	{ "i386DX/20", CPU_386DX, fpus_80386, 1, 20000000, 1, 0, 0x0308, 0, 0, 0, 4, 4, 3, 3, 3 },
	{ "i386DX/25", CPU_386DX, fpus_80386, 2, 25000000, 1, 0, 0x0308, 0, 0, 0, 4, 4, 3, 3, 3 },
	{ "i386DX/33", CPU_386DX, fpus_80386, 3, 33333333, 1, 0, 0x0308, 0, 0, 0, 6, 6, 3, 3, 4 },
	{ "", -1, 0, 0, 0 }
};

CPU cpus_i486[] =
{
	/*i486*/
	{ "i486SX/16", CPU_i486SX, fpus_none, 0, 16000000, 1, 16000000, 0x42a, 0, 0, CPU_SUPPORTS_DYNAREC, 3, 3, 3, 3, 2 },
	{ "i486SX/20", CPU_i486SX, fpus_none, 1, 20000000, 1, 20000000, 0x42a, 0, 0, CPU_SUPPORTS_DYNAREC, 4, 4, 3, 3, 3 },
	{ "i486SX/25", CPU_i486SX, fpus_none, 2, 25000000, 1, 25000000, 0x42a, 0, 0, CPU_SUPPORTS_DYNAREC, 4, 4, 3, 3, 3 },
	{ "i486SX/33", CPU_i486SX, fpus_none, 3, 33333333, 1, 33333333, 0x42a, 0, 0, CPU_SUPPORTS_DYNAREC, 6, 6, 3, 3, 4 },
	{ "i486SX2/50", CPU_i486SX, fpus_none, 5, 50000000, 2, 25000000, 0x45b, 0, 0, CPU_SUPPORTS_DYNAREC, 8, 8, 6, 6, 2 * 3 },
	{ "i486DX/25", CPU_i486DX, fpus_builtin, 2, 25000000, 1, 25000000, 0x404, 0, 0, CPU_SUPPORTS_DYNAREC, 4, 4, 3, 3, 3 },
	{ "i486DX/33", CPU_i486DX, fpus_builtin, 3, 33333333, 1, 33333333, 0x404, 0, 0, CPU_SUPPORTS_DYNAREC, 6, 6, 3, 3, 4 },
	{ "i486DX/50", CPU_i486DX, fpus_builtin, 5, 50000000, 1, 25000000, 0x404, 0, 0, CPU_SUPPORTS_DYNAREC, 8, 8, 4, 4, 6 },
	{ "i486DX2/40", CPU_i486DX, fpus_builtin, 4, 40000000, 2, 20000000, 0x430, 0, 0, CPU_SUPPORTS_DYNAREC, 8, 8, 6, 6, 2 * 3 },
	{ "i486DX2/50", CPU_i486DX, fpus_builtin, 5, 50000000, 2, 25000000, 0x430, 0, 0, CPU_SUPPORTS_DYNAREC, 8, 8, 6, 6, 2 * 3 },
	{ "i486DX2/66", CPU_i486DX, fpus_builtin, 6, 66666666, 2, 33333333, 0x430, 0, 0, CPU_SUPPORTS_DYNAREC, 12, 12, 6, 6, 2 * 4 },
	{ "", -1, 0, 0, 0 }
};


MODEL models[] =
{
	{ "[8088] Generic XT clone", ROM_GENXT, "genxt", { { "", cpus_8088 }, { "", NULL }, { "", NULL } }, MODEL_GFX_NONE, 32, 704, 16, model_init, NULL },
	{ "[286] AMI 286 clone", ROM_AMI286, "ami286", { { "", cpus_286 }, { "", NULL }, { "", NULL } }, MODEL_GFX_NONE | MODEL_AT | MODEL_HAS_IDE, 512, 16384, 128, model_init, NULL },
	{ "[386SX] AMI 386SX clone", ROM_AMI386SX, "ami386", { { "386SX", cpus_i386SX }, { "386DX", cpus_i386DX }, { "486", cpus_i486 } }, MODEL_GFX_NONE | MODEL_AT | MODEL_HAS_IDE, 512, 16384, 128, model_init, NULL },
};

int rom_present(const char *s)
{
	return 0;
}
static int midi_opened;

void midi_write(uint8_t v)
{
#ifdef WITH_MIDI
	if (!midi_opened) {
		midi_opened = Midi_Open();
	}
	Midi_Parse(midi_output, &v);
#endif
}

void pcem_close(void)
{
#ifdef WITH_MIDI
	if (midi_opened)
		Midi_Close();
#endif
	midi_opened = 0;
}

#ifdef CPU_i386
void codegen_set_rounding_mode(int mode)
{
	/*SSE*/
	cpu_state.new_fp_control = (cpu_state.old_fp_control & ~0x6000) | (mode << 13);
	/*x87 - used for double -> i64 conversions*/
	cpu_state.new_fp_control2 = (cpu_state.old_fp_control2 & ~0x0c00) | (mode << 10);
}
#else
void codegen_set_rounding_mode(int mode)
{
	/*SSE*/
	cpu_state.new_fp_control = (cpu_state.old_fp_control & ~0x6000) | (mode << 13);
}
#endif

// VIDEO STUFF

int video_timing_read_b;
int video_timing_read_w;
int video_timing_read_l;

int video_timing_write_b;
int video_timing_write_w;
int video_timing_write_l;

int cycles_lost;

uint8_t fontdat[2048][8];
uint8_t fontdatm[2048][16];
uint8_t fontdatw[512][32];	/* Wyse700 font */
uint8_t fontdat8x12[256][16];	/* MDSI Genius font */
uint8_t fontdat12x18[256][36];	/* IM1024 font */
uint8_t fontdatksc5601[16384][32]; /* Korean KSC-5601 font */
uint8_t fontdatksc5601_user[192][32]; /* Korean KSC-5601 user defined font */
uint32_t *video_6to8;

int PCI;
int readflash;
int egareads;
int egawrites;
int changeframecount;
PCBITMAP *buffer32;
static int buffer32size;
static uae_u8 *tempbuf;
int vid_resize;
int xsize, ysize;
uint64_t timer_freq;

uint8_t edatlookup[4][4];
uint8_t rotatevga[8][256];
uint32_t *video_15to32, *video_16to32;
monitor_t monitors[MONITORS_NUM];
bitmap_t target_bitmap;

static void *gfxboard_priv;

static mem_mapping_t *pcem_mapping_linear;
void *pcem_mapping_linear_priv;
uae_u32 pcem_mapping_linear_offset;
uint8_t(*pcem_linear_read_b)(uint32_t addr, void *priv);
uint16_t(*pcem_linear_read_w)(uint32_t addr, void *priv);
uint32_t(*pcem_linear_read_l)(uint32_t addr, void *priv);
void (*pcem_linear_write_b)(uint32_t addr, uint8_t  val, void *priv);
void (*pcem_linear_write_w)(uint32_t addr, uint16_t val, void *priv);
void (*pcem_linear_write_l)(uint32_t addr, uint32_t val, void *priv);

#define MAX_PCEMMAPPINGS 10
static mem_mapping_t *pcemmappings[MAX_PCEMMAPPINGS];
#define PCEMMAPBLOCKSIZE 0x10000
#define MAX_PCEMMAPBLOCKS (0x80000000 / PCEMMAPBLOCKSIZE)
mem_mapping_t **pcemmap;

static uint8_t dummy_bread(uint32_t addr, void *p)
{
	return 0;
}
static uint16_t dummy_wread(uint32_t addr, void *p)
{
	return 0;
}
static uint32_t dummy_lread(uint32_t addr, void *p)
{
	return 0;
}
static void dummy_bwrite(uint32_t addr, uint8_t v, void *p)
{

}
static void dummy_wwrite(uint32_t addr, uint16_t v, void *p)
{

}
static void dummy_lwrite(uint32_t addr, uint32_t v, void *p)
{

}

uint64_t timer_read(void)
{
	return read_processor_time();
}

static pc_timer_t *timer_head = NULL;

void timer_enablex(pc_timer_t *timer)
{
	pc_timer_t *timer_node = timer_head;
	if (timer->enabled)
		timer_disablex(timer);
	timer->enabled = 1;
	if (!timer_head)
	{
		timer_head = timer;
		timer->next = timer->prev = NULL;
		return;
	}
	timer_node = timer_head;
}
void timer_disablex(pc_timer_t *timer)
{
	if (!timer->enabled)
		return;
	timer->enabled = 0;
	if (timer->prev)
		timer->prev->next = timer->next;
	else
		timer_head = timer->next;
	if (timer->next)
		timer->next->prev = timer->prev;
	timer->prev = timer->next = NULL;
}
void timer_addx(pc_timer_t *timer, void (*callback)(void* p), void *p, int start_timer)
{
	memset(timer, 0, sizeof(pc_timer_t));

	timer->callback = callback;
	timer->p = p;
	timer->enabled = 0;
	timer->prev = timer->next = NULL;
}
void timer_set_delay_u64x(pc_timer_t *timer, uint64_t delay)
{
	timer_enablex(timer);
}
static void timer_remove_headx(void)
{
	if (timer_head)
	{
		pc_timer_t *timer = timer_head;
		timer_head = timer->next;
		if (timer_head) {
			timer_head->prev = NULL;
		}
		timer->next = timer->prev = NULL;
		timer->enabled = 0;
	}
}
void timer_advance_u64x(pc_timer_t *timer, uint64_t delay)
{
	uint32_t int_delay = delay >> 32;
	uint32_t frac_delay = delay & 0xffffffff;

	if ((frac_delay + timer->ts_frac) < frac_delay)
		timer->ts_integer++;
	timer->ts_frac += frac_delay;
	timer->ts_integer += int_delay;

	timer_enablex(timer);
}


void pcemglue_hsync(void)
{
	while (timer_head) {
		timer_head->callback(timer_head->p);
		timer_remove_headx();
	}
}

void pcemvideorbswap(bool swapped)
{
	for (int c = 0; c < 65536; c++) {
		if (swapped) {
			video_15to32[c] = (((c >> 10) & 31) << 3) | (((c >> 5) & 31) << 11) | (((c >> 0) & 31) << 19);
		} else {
			video_15to32[c] = ((c & 31) << 3) | (((c >> 5) & 31) << 11) | (((c >> 10) & 31) << 19);
		}
		if (swapped) {
			video_16to32[c] = (((c >> 11) & 31) << 3) | (((c >> 5) & 63) << 10) | (((c >> 0) & 31) << 19);
		} else {
			video_16to32[c] = ((c & 31) << 3) | (((c >> 5) & 63) << 10) | (((c >> 11) & 31) << 19);
		}
	}
}

static int calc_6to8(int c)
{
	int    ic;
	int    i8;
	double d8;

	ic = c;
	if (ic == 64)
		ic = 63;
	else
		ic &= 0x3f;
	d8 = (ic / 63.0) * 255.0;
	i8 = (int)d8;

	return (i8 & 0xff);
}

void initpcemvideo(void *p, bool swapped)
{
	int c, d, e;

	int pcemblocks = MAX_PCEMMAPBLOCKS;
	if (!pcemmap) {
		pcemmap = xcalloc(mem_mapping_t *, pcemblocks);
		if (!pcemmap) {
			gui_message(_T("out of memory for PCI mappin table\n"));
			abort();
		}
	}
	for (int i = 0; i < pcemblocks; i++) {
		pcemmap[i] = NULL;
	}
	for (int i = 0; i < MAX_PCEMMAPPINGS; i++) {
		pcemmappings[i] = NULL;
	}

	gfxboard_priv = p;
	has_vlb = 0;
	timer_freq = syncbase;

	for (c = 0; c < 256; c++)
	{
		e = c;
		for (d = 0; d < 8; d++)
		{
			rotatevga[d][c] = e;
			e = (e >> 1) | ((e & 1) ? 0x80 : 0);
		}
	}
	for (c = 0; c < 4; c++)
	{
		for (d = 0; d < 4; d++)
		{
			edatlookup[c][d] = 0;
			if (c & 1) edatlookup[c][d] |= 1;
			if (d & 1) edatlookup[c][d] |= 2;
			if (c & 2) edatlookup[c][d] |= 0x10;
			if (d & 2) edatlookup[c][d] |= 0x20;
			//                        printf("Edat %i,%i now %02X\n",c,d,edatlookup[c][d]);
		}
	}

	if (!video_15to32)
		video_15to32 = (uint32_t*)malloc(4 * 65536);
	if (!video_16to32)
		video_16to32 = (uint32_t*)malloc(4 * 65536);
	pcemvideorbswap(swapped);

	if (!buffer32) {
		buffer32 = (PCBITMAP *)calloc(sizeof(PCBITMAP) + sizeof(uint8_t *) * 4096, 1);
		buffer32->w = 2048;
		buffer32->h = 4096;
		target_bitmap.w = buffer32->w;
		target_bitmap.h = buffer32->h;
		buffer32->dat = xcalloc(uae_u8, buffer32->w * buffer32->h * 4);
		target_bitmap.dat = (uae_u32*)buffer32->dat;
		for (int i = 0; i < buffer32->h; i++) {
			buffer32->line[i] = buffer32->dat + buffer32->w * 4 * i;
			target_bitmap.line[i] = (uae_u32*)buffer32->line[i];
		}
	}

	video_6to8 = (uint32_t*)malloc(4 * 256);
	for (uint16_t c = 0; c < 256; c++) {
		video_6to8[c] = calc_6to8(c);
	}

	pcem_linear_read_b = dummy_bread;
	pcem_linear_read_w = dummy_wread;
	pcem_linear_read_l = dummy_lread;
	pcem_linear_write_b = dummy_bwrite;
	pcem_linear_write_w = dummy_wwrite;
	pcem_linear_write_l = dummy_lwrite;
	pcem_mapping_linear = NULL;
	pcem_mapping_linear_offset = 0;
	timer_head = NULL;

	monitors[0].mon_changeframecount = 2;
	monitors[0].target_buffer = &target_bitmap;
}

extern void *svga_get_object(void);

uae_u8 pcem_read_io(int port, int index)
{
	svga_t *svga = (svga_t*)svga_get_object();
	if (port == 0x3d5) {
		return svga->crtc[index];
	} else {
		write_log("unknown port %04x!\n", port);
		abort();
	}
}

#if 0
void update_pcembuffer32(void *buf, int w, int h, int bytesperline)
{
	if (!buf) {
		w = 2048;
		h = 2048;
		if (!tempbuf) {
			tempbuf = xmalloc(uae_u8, w * 4);
		}
		buf = tempbuf;
		bytesperline = 0;
	}
	if (!buffer32 || h > buffer32->h) {
		free(buffer32);
		buffer32 = (PCBITMAP *)calloc(sizeof(PCBITMAP) + sizeof(uint8_t *) * h, 1);
	}
	if (buffer32->w == w && buffer32->h == h && buffer32->line[0] == buf)
		return;
	buffer32->w = w;
	buffer32->h = h;
	uae_u8 *b = (uae_u8*)buf;
	for (int i = 0; i < h; i++) {
		buffer32->line[i] = b;
		b += bytesperline;
	}
}
#endif

uae_u8 *getpcembuffer32(int x, int y, int yy)
{
	return buffer32->line[y + yy] + x * 4;
}

void video_inform_monitor(int type, const video_timings_t *ptr, int monitor_index)
{
}

int monitor_index_global;

void video_wait_for_buffer(void)
{
	//write_log(_T("video_wait_for_buffer\n"));
}
void updatewindowsize(int x, int mx, int y, int my)
{
	x *= mx;
	gfxboard_resize(x, y, mx, my, gfxboard_priv);
}

#define MAX_ADDED_DEVICES 8

struct addeddevice {
	const device_t *dev;
	void *priv;
};

static addeddevice added_devices[MAX_ADDED_DEVICES];

void *device_add(const device_t *d)
{
	for (int i = 0; i < MAX_ADDED_DEVICES; i++) {
		if (!added_devices[i].dev) {
			added_devices[i].dev = d;
			added_devices[i].priv = d->init(d);
			return added_devices[i].priv;
		}
	}
	return NULL;
}

void pcemfreeaddeddevices(void)
{
	for (int i = 0; i < MAX_ADDED_DEVICES; i++) {
		if (added_devices[i].dev) {
			added_devices[i].dev->close(added_devices[i].priv);
			added_devices[i].dev = NULL;
		}
	}
}

static void (*pci_card_write)(int func, int addr, uint8_t val, void *priv);
static uint8_t(*pci_card_read)(int func, int addr, void *priv);
static void *pci_card_priv;

void put_pci_pcem(uaecptr addr, uae_u8 v)
{
	if (pci_card_write) {
		pci_card_write(0, addr, v, pci_card_priv);
	}
}
uae_u8 get_pci_pcem(uaecptr addr)
{
	uae_u8 v = 0;
	if (pci_card_read) {
		v = pci_card_read(0, addr, pci_card_priv);
	}
	return v;
}

void pci_add_card(uint8_t add_type, uint8_t(*read)(int func, int addr, void *priv),
	void (*write)(int func, int addr, uint8_t val, void *priv), void *priv, uint8_t *slot)
{
	pci_card_read = read;
	pci_card_write = write;
	pci_card_priv = priv;
}

int pci_add(uint8_t(*read)(int func, int addr, void *priv), void (*write)(int func, int addr, uint8_t val, void *priv), void *priv)
{
	pci_card_read = read;
	pci_card_write = write;
	pci_card_priv = priv;
	return 0;
}
extern void gfxboard_intreq(void *, int, bool);
void pci_set_irq_routing(int card, int irq)
{
	//write_log(_T("pci_set_irq_routing %d %d\n"), card, irq);
}
void pci_set_irq(int card, int pci_int, uint8_t *state)
{
	//write_log(_T("pci_set_irq %d %d\n"), card, pci_int);
	gfxboard_intreq(gfxboard_priv, 1, true);
}
void pci_clear_irq(int card, int pci_int, uint8_t *state)
{
	//write_log(_T("pci_clear_irq %d %d\n"), card, pci_int);
	gfxboard_intreq(gfxboard_priv, 0, true);
}
int rom_init(rom_t *rom, const char *fn, uint32_t address, int size, int mask, int file_offset, uint32_t flags)
{
	return 0;
}

void thread_sleep(int n)
{
	sleep_millis(n);
}

thread_t *thread_create(int (*thread_rout)(void *param), void *param)
{
	uae_thread_id tid;
	uae_start_thread(_T("PCem helper"), thread_rout, param, &tid);
	return tid;
}
void thread_kill(thread_t *handle)
{
	uae_end_thread((uae_thread_id*)&handle);
}
event_t *thread_create_event(void)
{
	uae_sem_t sem = { 0 };
	uae_sem_init(&sem, 1, 0);
	return sem;
}
int thread_wait(thread_t *arg)
{
	if (!arg) {
		return 0;
	}
	uae_sem_wait((uae_sem_t*)&arg);
	return 1;
}
void thread_set_event(event_t *event)
{
	uae_sem_post((uae_sem_t*)&event);
}
void thread_reset_event(event_t *_event)
{
	uae_sem_init((uae_sem_t*)&_event, 1, 0);
}
#ifndef _WIN32
#define INFINITE            0xFFFFFFFF  // Infinite timeout
#endif
int thread_wait_event(event_t *event, int timeout)
{
	uae_sem_trywait_delay((uae_sem_t*)&event, timeout < 0 ? INFINITE : timeout);
	return 0;
}
void thread_destroy_event(event_t *_event)
{
	uae_sem_destroy((uae_sem_t*)&_event);
}

typedef struct win_mutex_t
{
#ifdef _WIN32
	HANDLE handle;
#else
	SDL_mutex* handle;
#endif
} win_mutex_t;

mutex_t* thread_create_mutex(void)
{
	win_mutex_t *mutex = xcalloc(win_mutex_t,1);
#ifdef _WIN32
	mutex->handle = CreateSemaphore(NULL, 1, 1, NULL);
#else
	mutex->handle = SDL_CreateMutex();
#endif
	return mutex;
}

int thread_wait_mutex(mutex_t *_mutex)
{
	if (!_mutex) {
		return 0;
	}
	win_mutex_t *mutex = (win_mutex_t*)_mutex;
#ifdef _WIN32
	WaitForSingleObject(mutex->handle, INFINITE);
#else
	SDL_LockMutex(mutex->handle);
#endif
	return 1;
}

int thread_release_mutex(mutex_t *_mutex)
{
	if (!_mutex) {
		return 0;
	}
	win_mutex_t *mutex = (win_mutex_t*)_mutex;
#ifdef _WIN32
	ReleaseSemaphore(mutex->handle, 1, NULL);
#else
	SDL_UnlockMutex(mutex->handle);
#endif
	return 1;
}

void thread_close_mutex(mutex_t *_mutex)
{
	win_mutex_t *mutex = (win_mutex_t*)_mutex;
#ifdef _WIN32
	CloseHandle(mutex->handle);
#else
	SDL_DestroyMutex(mutex->handle);
#endif
	xfree(mutex);
}

int thread_test_mutex(mutex_t *_mutex)
{
	if (!_mutex) {
		return 0;
	}
	win_mutex_t *mutex = (win_mutex_t *)_mutex;
#ifdef _WIN32
	DWORD ret = WaitForSingleObject(mutex->handle, 0);
	if (ret == WAIT_OBJECT_0)
		return 1;
#else
	if (SDL_TryLockMutex(mutex->handle) == 0) {
		SDL_UnlockMutex(mutex->handle);
		return 1;
	}
#endif
	return 0;
}



static mem_mapping_t *getmm(uaecptr *addrp)
{
	uaecptr addr = *addrp;
	addr &= 0x7fffffff;
	int index = addr / PCEMMAPBLOCKSIZE;
	mem_mapping_t *m = pcemmap[index];
	if (!m) {
		write_log(_T("%08x no mapping\n"), addr);
	}
	return m;
}

void put_mem_pcem(uaecptr addr, uae_u32 v, int size)
{
#if 0
	write_log("put_mem_pcem %08x %08x %d\n", addr, v, size);
#endif
	mem_mapping_t *m = getmm(&addr);
	if (m) {
		if (size == 0) {
			m->write_b(addr, v, m->p);
		} else if (size == 1) {
			m->write_w(addr, v, m->p);
		} else if (size == 2) {
			m->write_l(addr, v, m->p);
		}
	}
}
uae_u32 get_mem_pcem(uaecptr addr, int size)
{
	uae_u32 v = 0;
	mem_mapping_t *m = getmm(&addr);
	if (m) {
		if (size == 0) {
			v = m->read_b(addr, m->p);
		} else if (size == 1) {
			v = m->read_w(addr, m->p);
		} else if (size == 2) {
			v = m->read_l(addr, m->p);
		}
	}
#if 0
	write_log("get_mem_pcem %08x %08x %d\n", addr, v, size);
#endif
	return v;
}

static void mapping_recalc(mem_mapping_t *mapping)
{
	if (mapping->read_b == NULL)
		mapping->read_b = dummy_bread;
	if (mapping->read_w == NULL)
		mapping->read_w = dummy_wread;
	if (mapping->read_l == NULL)
		mapping->read_l = dummy_lread;
	if (mapping->write_b == NULL)
		mapping->write_b = dummy_bwrite;
	if (mapping->write_w == NULL)
		mapping->write_w = dummy_wwrite;
	if (mapping->write_l == NULL)
		mapping->write_l = dummy_lwrite;


	if (mapping == pcem_mapping_linear || (!pcem_mapping_linear && mapping->size >= 1024 * 1024)) {
		pcem_mapping_linear = mapping;
		pcem_mapping_linear_priv = mapping->p;
		pcem_mapping_linear_offset = mapping->base;
		if (!mapping->enable) {
			pcem_linear_read_b = dummy_bread;
			pcem_linear_read_w = dummy_wread;
			pcem_linear_read_l = dummy_lread;
			pcem_linear_write_b = dummy_bwrite;
			pcem_linear_write_w = dummy_wwrite;
			pcem_linear_write_l = dummy_lwrite;
		} else {
			pcem_linear_read_b = mapping->read_b;
			pcem_linear_read_w = mapping->read_w;
			pcem_linear_read_l = mapping->read_l;
			pcem_linear_write_b = mapping->write_b;
			pcem_linear_write_w = mapping->write_w;
			pcem_linear_write_l = mapping->write_l;
		}
	}

	for (uae_u32 i = 0; i < mapping->size; i += PCEMMAPBLOCKSIZE) {
		uae_u32 addr = i + mapping->base;
		addr &= 0x7fffffff;
		int offset = addr / PCEMMAPBLOCKSIZE;
		if (offset >= 0 && offset < MAX_PCEMMAPBLOCKS) {
			if (mapping->enable) {
				pcemmap[offset] = mapping;
			} else {
				pcemmap[offset] = NULL;
			}
		}
	}
}

void mem_mapping_addx(mem_mapping_t *mapping,
	uint32_t base,
	uint32_t size,
	uint8_t(*read_b)(uint32_t addr, void *p),
	uint16_t(*read_w)(uint32_t addr, void *p),
	uint32_t(*read_l)(uint32_t addr, void *p),
	void (*write_b)(uint32_t addr, uint8_t  val, void *p),
	void (*write_w)(uint32_t addr, uint16_t val, void *p),
	void (*write_l)(uint32_t addr, uint32_t val, void *p),
	uint8_t *exec,
	uint32_t flags,
	void *p)
{
	mapping->enable = 0;
	for (int i = 0; i < MAX_PCEMMAPPINGS; i++) {
		if (pcemmappings[i] == mapping)
			return;
	}
	for (int i = 0; i < MAX_PCEMMAPPINGS; i++) {
		if (pcemmappings[i] == NULL) {
			pcemmappings[i] = mapping;
			mapping->base = base;
			mapping->size = size;
			mapping->read_b = read_b;
			mapping->read_w = read_w;
			mapping->read_l = read_l;
			mapping->write_b = write_b;
			mapping->write_w = write_w;
			mapping->write_l = write_l;
			mapping->exec = exec;
			mapping->flags = flags;
			mapping->p = p;
			mapping->next = NULL;
			mapping_recalc(mapping);
			return;
		}
	}
}

void mem_mapping_set_handlerx(mem_mapping_t *mapping,
	uint8_t(*read_b)(uint32_t addr, void *p),
	uint16_t(*read_w)(uint32_t addr, void *p),
	uint32_t(*read_l)(uint32_t addr, void *p),
	void (*write_b)(uint32_t addr, uint8_t  val, void *p),
	void (*write_w)(uint32_t addr, uint16_t val, void *p),
	void (*write_l)(uint32_t addr, uint32_t val, void *p))
{
	mapping->read_b = read_b;
	mapping->read_w = read_w;
	mapping->read_l = read_l;
	mapping->write_b = write_b;
	mapping->write_w = write_w;
	mapping->write_l = write_l;
	mapping_recalc(mapping);
}

void mem_mapping_set_px(mem_mapping_t *mapping, void *p)
{
	mapping->p = p;
}

void pci_add_specific(int card, uint8_t(*read)(int func, int addr, void *priv), void (*write)(int func, int addr, uint8_t val, void *priv), void *priv)
{
}

void mca_add(uint8_t(*read)(int addr, void *priv), void (*write)(int addr, uint8_t val, void *priv), void (*reset)(void *priv), void *priv)
{
}

#define MAX_IO_PORT 0x10000
static uint8_t(*port_inb[MAX_IO_PORT])(uint16_t addr, void *priv);
static uint16_t(*port_inw[MAX_IO_PORT])(uint16_t addr, void *priv);
static uint32_t(*port_inl[MAX_IO_PORT])(uint16_t addr, void *priv);
static void(*port_outb[MAX_IO_PORT])(uint16_t addr, uint8_t  val, void *priv);
static void(*port_outw[MAX_IO_PORT])(uint16_t addr, uint16_t val, void *priv);
static void(*port_outl[MAX_IO_PORT])(uint16_t addr, uint32_t val, void *priv);
static void *port_priv[MAX_IO_PORT];

void put_io_pcem(uaecptr addr, uae_u32 v, int size)
{
#if 0
	write_log(_T("put_io_pcem(%08x,%08x,%d)\n"), addr, v, size);
#endif

	addr &= MAX_IO_PORT - 1;
	if (size == 0) {
		if (port_outb[addr])
			port_outb[addr](addr, v, port_priv[addr]);
	} else if (size == 1) {
		if (port_outw[addr]) {
			port_outw[addr](addr, v, port_priv[addr]);
		} else {
			put_io_pcem(addr + 0, v >> 0, 0);
			put_io_pcem(addr + 1, v >> 8, 0);
		}
	} else if (size == 2) {
		if (port_outl[addr]) {
			port_outl[addr](addr, v, port_priv[addr]);
		} else {
			put_io_pcem(addr + 0, v >> 16, 1);
			put_io_pcem(addr + 2, v >> 0, 1);
		}
	}
}
uae_u32 get_io_pcem(uaecptr addr, int size)
{
#if 0
	write_log(_T("get_io_pcem(%08x,%d)\n"), addr, size);
#endif

	uae_u32 v = 0;
	addr &= MAX_IO_PORT - 1;
	if (size == 0) {
		if (port_inb[addr])
			v = port_inb[addr](addr, port_priv[addr]);
	} else if (size == 1) {
		if (port_inw[addr]) {
			v = port_inw[addr](addr, port_priv[addr]);
		} else {
			v = get_io_pcem(addr + 0, 0) << 0;
			v |= get_io_pcem(addr + 1, 0) << 8;
		}
	} else if (size == 2) {
		if (port_inl[addr]) {
			v = port_inl[addr](addr, port_priv[addr]);
		} else {
			v = get_io_pcem(addr + 0, 1) << 16;
			v |= get_io_pcem(addr + 2, 1) << 0;
		}
	}
	return v;
}

void io_sethandlerx(uint16_t base, int size,
	uint8_t(*inb)(uint16_t addr, void *priv),
	uint16_t(*inw)(uint16_t addr, void *priv),
	uint32_t(*inl)(uint16_t addr, void *priv),
	void (*outb)(uint16_t addr, uint8_t  val, void *priv),
	void (*outw)(uint16_t addr, uint16_t val, void *priv),
	void (*outl)(uint16_t addr, uint32_t val, void *priv),
	void *priv)
{
	if (base + size > MAX_IO_PORT)
		return;

	for (int i = 0; i < size; i++) {
		int io = base + i;
		port_inb[io] = inb;
		port_inw[io] = inw;
		port_inl[io] = inl;
		port_outb[io] = outb;
		port_outw[io] = outw;
		port_outl[io] = outl;
		port_priv[io] = priv;
	}
}

void io_removehandlerx(uint16_t base, int size,
	uint8_t(*inb)(uint16_t addr, void *priv),
	uint16_t(*inw)(uint16_t addr, void *priv),
	uint32_t(*inl)(uint16_t addr, void *priv),
	void (*outb)(uint16_t addr, uint8_t  val, void *priv),
	void (*outw)(uint16_t addr, uint16_t val, void *priv),
	void (*outl)(uint16_t addr, uint32_t val, void *priv),
	void *priv)
{
	if (base + size > MAX_IO_PORT)
		return;

	for (int i = 0; i < size; i++) {
		int io = base + i;
		port_inb[io] = NULL;
		port_inw[io] = NULL;
		port_inl[io] = NULL;
		port_outb[io] = NULL;
		port_outw[io] = NULL;
		port_outl[io] = NULL;
	}
}

void mem_mapping_set_addrx(mem_mapping_t *mapping, uint32_t base, uint32_t size)
{
	mapping->enable = 0;
	mapping_recalc(mapping);
	mapping->enable = 1;
	mapping->base = base;
	mapping->size = size;
	mapping_recalc(mapping);
}

void mem_mapping_disablex(mem_mapping_t *mapping)
{
	mapping->enable = 0;
	mapping_recalc(mapping);
}

void mem_mapping_enablex(mem_mapping_t *mapping)
{
	mapping->enable = 1;
	mapping_recalc(mapping);
}

void pcem_linear_mark(int offset)
{
	if (!pcem_mapping_linear)
		return;
	uae_u16 w = pcem_linear_read_w(offset, pcem_mapping_linear_priv);
	pcem_linear_write_w(offset, w, pcem_mapping_linear_priv);
}

int model_get_config_int(const char *s)
{
	return 0;
}
char *model_get_config_string(const char *s)
{
	return NULL;
}
void upc_set_mouse(void (*mouse_write)(uint8_t, void*), void *p)
{
}


/* DMA Bus Master Page Read/Write */
void
dma_bm_read(uint32_t PhysAddress, uint8_t *DataRead, uint32_t TotalSize, int TransferSize)
{
#if 0
	uint32_t n;
	uint32_t n2;
	uint8_t  bytes[4] = { 0, 0, 0, 0 };

	n = TotalSize & ~(TransferSize - 1);
	n2 = TotalSize - n;

	/* Do the divisible block, if there is one. */
	if (n) {
		for (uint32_t i = 0; i < n; i += TransferSize)
			mem_read_phys((void *)&(DataRead[i]), PhysAddress + i, TransferSize);
	}

	/* Do the non-divisible block, if there is one. */
	if (n2) {
		mem_read_phys((void *)bytes, PhysAddress + n, TransferSize);
		memcpy((void *)&(DataRead[n]), bytes, n2);
	}
#endif
}

void
dma_bm_write(uint32_t PhysAddress, const uint8_t *DataWrite, uint32_t TotalSize, int TransferSize)
{
#if 0
	uint32_t n;
	uint32_t n2;
	uint8_t  bytes[4] = { 0, 0, 0, 0 };

	n = TotalSize & ~(TransferSize - 1);
	n2 = TotalSize - n;

	/* Do the divisible block, if there is one. */
	if (n) {
		for (uint32_t i = 0; i < n; i += TransferSize)
			mem_write_phys((void *)&(DataWrite[i]), PhysAddress + i, TransferSize);
	}

	/* Do the non-divisible block, if there is one. */
	if (n2) {
		mem_read_phys((void *)bytes, PhysAddress + n, TransferSize);
		memcpy(bytes, (void *)&(DataWrite[n]), n2);
		mem_write_phys((void *)bytes, PhysAddress + n, TransferSize);
	}

	if (dma_at)
		mem_invalidate_range(PhysAddress, PhysAddress + TotalSize - 1);
#endif
}

void video_force_resize_set_monitor(uint8_t res, int monitor_index)
{
}
