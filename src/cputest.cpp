
#include "cputest.h"
#include "cputbl_test.h"
#include "readcpu.h"
#include "disasm.h"
#include "ini.h"
#include "fpp.h"
#include "softfloat/softfloat-specialize.h"

#include "zlib.h"

#include "options.h"

#define MAX_REGISTERS 16

#define EAFLAG_SP 1

#define FPUOPP_ILLEGAL 0x80
static floatx80 fpuregisters[8];
static uae_u32 fpu_fpiar, fpu_fpcr, fpu_fpsr;

struct regtype
{
	const TCHAR *name;
	uae_u8 type;
};
struct regdata
{
	uae_u32 data[3];
	uae_u8 type;
};

const int areg_byteinc[] = { 1, 1, 1, 1, 1, 1, 1, 2 };
const int imm8_table[] = { 8, 1, 2, 3, 4, 5, 6, 7 };

int movem_index1[256];
int movem_index2[256];
int movem_next[256];
int hardware_bus_error, hardware_bus_error_fake;

struct mmufixup mmufixup[2];
cpuop_func *cpufunctbl[65536];
cpuop_func_noret *cpufunctbl_noret[65536];
struct cputbl_data
{
	uae_s16 length;
	uae_s8 disp020[2];
	uae_u8 branch;
};
static struct cputbl_data cpudatatbl[65536];

struct regstruct regs;
struct flag_struct regflags;
int cpu_cycles;
static int cycle_count_disable;

static int verbose = 1;
static int feature_exception3_data = 0;
static int feature_exception3_instruction = 0;
static int feature_sr_mask = 0;
static int feature_undefined_ccr = 0;
static int feature_initial_interrupt = 0;
static int feature_initial_interrupt_mask = 0;
static int feature_min_interrupt_mask = 0;
static int feature_loop_mode_cnt = 0;
static int feature_loop_mode_register = -1;
static int feature_loop_mode_68010 = 0;
static int feature_loop_mode_jit = 0;
static int feature_full_extension_format = 0;
static int feature_test_rounds = 2;
static int feature_test_rounds_opcode = 0;
static int feature_flag_mode = 0;
static int feature_usp = 0;
static int feature_exception_vectors = 0;
static int feature_interrupts = 0;
static int feature_waitstates = 0;
static int feature_instruction_size = 0;
static int fpu_min_exponent, fpu_max_exponent, fpu_max_precision, fpu_unnormals;
static int feature_ipl_delay;
static int max_file_size;
static int rnd_seed, rnd_seed_prev;
static TCHAR *feature_instruction_size_text = NULL;
static uae_u32 feature_addressing_modes[2];
static uae_u32 feature_condition_codes;
static int feature_gzip = 0;
static int ad8r[2], pc8r[2];
static int multi_mode;
#define MAX_TARGET_EA 20
static uae_u32 feature_target_ea[MAX_TARGET_EA][3];
static int target_ea_src_cnt, target_ea_dst_cnt, target_ea_opcode_cnt;
static int target_ea_src_max, target_ea_dst_max, target_ea_opcode_max;
static uae_u32 target_ea[3];
static int maincpu[6];
static uae_u8 exceptionenabletable[256];
#define MAX_REGDATAS 32
static int regdatacnt;
static struct regdata regdatas[MAX_REGDATAS];
static uae_u32 ignore_register_mask;

#define HIGH_MEMORY_START (addressing_mask == 0xffffffff ? 0xffff8000 : 0x00ff8000)

// large enough for RTD
#define STACK_SIZE (0x8000 + 8)
#define RESERVED_SUPERSTACK 1024
// space between superstack and USP
#define RESERVED_USERSTACK_EXTRA 128
// space for extra exception, not part of test region
#define EXTRA_RESERVED_SPACE 1024

static uae_u32 test_low_memory_start;
static uae_u32 test_low_memory_end;
static uae_u32 test_high_memory_start;
static uae_u32 test_high_memory_end;
static uae_u32 low_memory_size = 32768;
static uae_u32 high_memory_size = 32768;
static uae_u32 safe_memory_start;
static uae_u32 safe_memory_end;
static int safe_memory_mode;
static uae_u32 user_stack_memory, super_stack_memory;
static uae_u32 user_stack_memory_use;

static uae_u8 *low_memory, *high_memory, *test_memory;
static uae_u8 *low_memory_temp, *high_memory_temp, *test_memory_temp;
static uae_u8 dummy_memory[4];
static uaecptr test_memory_start, test_memory_end, opcode_memory_start;
static uae_u32 test_memory_size;
static int hmem_rom, lmem_rom;
static uae_u8 *opcode_memory;
static uae_u8 *storage_buffer;
static char inst_name[16+1];
static int storage_buffer_watermark_size;
static int storage_buffer_watermark;
static int max_storage_buffer;

static bool out_of_test_space;
static uaecptr out_of_test_space_addr;
static int forced_immediate_mode;
static int test_exception, test_exception_orig;
static int test_exception_extra;
static int exception_stack_frame_size;
static uae_u8 exception_extra_frame[100];
static int exception_extra_frame_size, exception_extra_frame_type;
static uaecptr test_exception_addr;
static int test_exception_3_w;
static int test_exception_3_fc;
static int test_exception_3_size;
static int test_exception_3_di;
static uae_u16 test_exception_3_sr;
static int test_exception_opcode;
static uae_u32 trace_store_pc;
static uae_u16 trace_store_sr;
static int generate_address_mode;
static int test_memory_access_mask;
static uae_u32 opcode_memory_address;
static uaecptr branch_target;
static uaecptr branch_target_pc;
static uae_u16 test_opcode;
static int test_absw;

static uae_u8 imm8_cnt;
static uae_u16 imm16_cnt;
static uae_u32 imm32_cnt;
static uae_u32 immabsw_cnt;
static uae_u32 immabsl_cnt;
static uae_u32 specials_cnt;
static uae_u32 immfpu_cnt;
static uae_u32 addressing_mask;
static int opcodecnt;
static int cpu_stopped;
static int cpu_halted;
static int cpu_lvl = 0;
static int test_count;
static int testing_active;
static uae_u16 testing_active_opcode;
static time_t starttime;
static int filecount;
static uae_u16 sr_undefined_mask;
static int low_memory_accessed;
static int high_memory_accessed;
static int test_memory_accessed;
static uae_u16 extra_or, extra_and;
static struct regstruct cur_regs;
static uae_u16 read_buffer_prev;
static int interrupt_count;
static uaecptr interrupt_pc;
static int interrupt_cycle_cnt, interrupt_delay_cnt;
static int interrupt_level;
static int waitstate_cycle_cnt;
static int waitstate_delay_cnt;
static uaecptr test_instruction_end_pc;
static uaecptr lm_safe_address1, lm_safe_address2;
static uae_u8 ccr_cnt;
static int condition_cnt;
static int subtest_count;
static int test_count_missed;

struct uae_prefs currprefs;

struct accesshistory
{
	uaecptr addr;
	uae_u32 val;
	uae_u32 oldval;
	int size;
	bool donotsave;
};
static int ahcnt_current, ahcnt_written;
static int noaccesshistory = 0;

#define MAX_ACCESSHIST 32000
static struct accesshistory ahist[MAX_ACCESSHIST];

static void pw(uae_u8 *p, uae_u16 v)
{
	p[0] = v >> 8;
	p[1] = v >> 0;
}
static void pl(uae_u8 *p, uae_u32 v)
{
	p[0] = v >> 24;
	p[1] = v >> 16;
	p[2] = v >> 8;
	p[3] = v >> 0;
}
static uae_u32 gl(uae_u8 *p)
{
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3] << 0);
}
static uae_u32 gw(uae_u8 *p)
{
	return (p[0] << 8) | (p[1] << 0);
}

void cputester_fault(void)
{
	test_exception = -1;
}

static int is_superstack_use_required(void)
{
	switch (testing_active_opcode)
	{
	case 0x4e73: // RTE
		return 1;
	}
	return 0;
}

static bool valid_address(uaecptr addr, int size, int rwp)
{
	int w = (rwp & 0x7fff) == 2;
	addr &= addressing_mask;
	size--;

	if (low_memory_size != 0xffffffff && addr + size < low_memory_size) {
		if (addr < test_low_memory_start || test_low_memory_start == 0xffffffff)
			goto oob;
		// only accept low memory when testing short absolute addressing
		if (!test_absw) {
			goto oob;
		}
		// exception vectors needed during tests
		if (currprefs.cpu_model == 68000) {
			if ((addr + size >= 0x08 && addr < 0x30 || (addr + size >= 0x80 && addr < 0xc0)))
				goto oob;
			if (feature_interrupts && (addr + size >= 0x60 && addr < 0x80))
				goto oob;
		}
		if (addr + size >= test_low_memory_end)
			goto oob;
		if (w && lmem_rom)
			goto oob;
		if (testing_active) {
			low_memory_accessed = w ? -1 : 1;
		}
		return 1;
	}
	if (high_memory_size != 0xffffffff && addr >= HIGH_MEMORY_START && addr <= HIGH_MEMORY_START + 0x7fff) {
		if (addr < test_high_memory_start || test_high_memory_start == 0xffffffff)
			goto oob;
		if (addr + size >= test_high_memory_end)
			goto oob;
		// only accept high memory when testing short absolute addressing
		if (!test_absw) {
			goto oob;
		}
		if (w && hmem_rom)
			goto oob;
		if (testing_active) {
			high_memory_accessed = w ? -1 : 1;
		}
		return 1;
	}
	if (addr >= super_stack_memory - RESERVED_SUPERSTACK && addr + size < super_stack_memory) {
		// allow only instructions that have to access super stack, for example RTE
		// read-only
		if (w) {
			goto oob;
		}
		if (testing_active) {
			if (is_superstack_use_required()) {
				test_memory_accessed = 1;
				return 1;
			}
		}
		goto oob;
	}
	if (addr >= test_memory_end && addr + size < test_memory_end + EXTRA_RESERVED_SPACE) {
		if (testing_active < 0)
			return 1;
	}
	if (addr >= test_memory_start && addr + size < test_memory_end) {
		// make sure we don't modify our test instruction
		if ((testing_active && w) || (rwp > 0 && (rwp & 0x8000))) {
			if (addr >= opcode_memory_start && addr + size < opcode_memory_start + OPCODE_AREA)
				goto oob;
		}
		// don't read data from our test instruction or nop/illegal words. Prefetches allowed.
		if (testing_active && (rwp & 1)) {
			if (addr >= opcode_memory_start && addr + size < opcode_memory_start + OPCODE_AREA)
				goto oob;
		}
		if (testing_active) {
			test_memory_accessed = w ? -1 : 1;
		}
		return 1;
	}
oob:
	return 0;
}

static bool check_valid_addr(uaecptr addr, int size, int rwp)
{
	if (!valid_address(addr, 1, rwp | 0x8000))
		return false;
	if (!valid_address(addr + size, 1, rwp | 0x8000))
		return false;
	return true;
}

static bool is_nowrite_address(uaecptr addr, int size)
{
	return addr + size > safe_memory_start && addr < safe_memory_end;
}

static void validate_addr(uaecptr addr, int size)
{
	if (valid_address(addr, size, 0))
		return;
	wprintf(_T(" Trying to store invalid memory address %08x!?\n"), addr);
	abort();
}


static uae_u8 *get_addr(uaecptr addr, int size, int rwp)
{
	// allow debug output to read memory even if oob condition
	if (rwp >= 0 && out_of_test_space)
		goto oob;
	if (!valid_address(addr, 1, rwp))
		goto oob;
	if (size > 1) {
		if (!valid_address(addr + size - 1, 1, rwp))
			goto oob;
	}
	addr &= addressing_mask;
	size--;

	// if loop mode: loop mode buffer can be only accessed by loop mode store instruction
	if (feature_loop_mode_jit && testing_active && addr >= test_memory_start && addr + size < test_memory_start + LM_BUFFER && (lm_safe_address1 != regs.pc && lm_safe_address2 != regs.pc)) {
		goto oob;
	}

	if (low_memory_size != 0xffffffff && addr + size < low_memory_size) {
		return low_memory + addr;
	} else if (high_memory_size != 0xffffffff && addr >= HIGH_MEMORY_START && addr <= HIGH_MEMORY_START + 0x7fff) {
		return high_memory + (addr - HIGH_MEMORY_START);
	} else if (addr >= test_memory_start && addr + size < test_memory_end + EXTRA_RESERVED_SPACE) {
		return test_memory + (addr - test_memory_start);
	}
oob:
	if (rwp >= 0) {
		if (!out_of_test_space) {
			out_of_test_space = true;
			out_of_test_space_addr = addr;
		}
	}
	dummy_memory[0] = 0;
	dummy_memory[1] = 0;
	dummy_memory[2] = 0;
	dummy_memory[3] = 0;
	return dummy_memory;
}

static void count_cycles(int cycles)
{
	if (cycle_count_disable) {
		return;
	}
	while (cycles > 0) {
		cycles--;
		cpu_cycles++;
		if (interrupt_cycle_cnt != 0) {
			bool waspos = interrupt_cycle_cnt > 0;
			if (interrupt_cycle_cnt < 0) {
				interrupt_cycle_cnt++;
			} else {
				interrupt_cycle_cnt--;
			}
			if (interrupt_cycle_cnt == 0) {
				if (regs.ipl_pin < IPL_TEST_IPL_LEVEL) {
					int ipl = IPL_TEST_IPL_LEVEL;
					if (feature_ipl_delay && waspos &&
						(((regs.ipl_pin & 1) && !(ipl & 1)) ||
						((regs.ipl_pin & 2) && !(ipl & 2)) ||
						((regs.ipl_pin & 4) && !(ipl & 4)))) {
						interrupt_cycle_cnt = -1;
						continue;
					}
					regs.ipl_pin = IPL_TEST_IPL_LEVEL;
					regs.ipl_pin_change_evt = cpu_cycles;
				}
				if (cpu_cycles == regs.ipl_evt_pre + cpuipldelay2) {
					if (regs.ipl_evt_pre_mode) {
						ipl_fetch_next();
					} else {
						ipl_fetch_now();
					}
				}
				interrupt_pc = regs.pc;
				interrupt_level = regs.ipl_pin;
				interrupt_cycle_cnt = 0;
			}
		}
	}
}

void do_cycles_test(int cycles)
{
	if (!testing_active)
		return;
	count_cycles(cycles);
}

static void add_memory_cycles(uaecptr addr, int c)
{
	if (cycle_count_disable) {
		return;
	}
	if (!testing_active) {
		return;
	}
	if (trace_store_pc != 0xffffffff) {
		return;
	}
	if (waitstate_cycle_cnt && (addr & addressing_mask) < 0x200000 && c > 0) {
		c *= 2;
		while (c > 0) {
			int now = cpu_cycles;
			int ipl = regs.ipl_pin;
			count_cycles(2);
			c--;
			// wait for free bus cycle
			for (;;) {
				int cb = (cpu_cycles - waitstate_cycle_cnt) / 2;
				// remove init cycles
				cb -= 3;
				if (cb < 0) {
					break;
				}
				if (feature_waitstates == 1) {
					cb %= 3;
					// AB-AB-AB-..
					if (cb == 2) {
						break;
					}
				} else {
					// -BC--BCD-BCD..
					// 012301230123
					if (cb == 0 || cb == 3) {
						break;
					}
					cb %= 4;
					if (cb == 0) {
						break;
					}
				}
				ipl = regs.ipl_pin;
				count_cycles(2);
			}
			count_cycles(2);
			c--;
			if (now == regs.ipl_evt) {
				regs.ipl[0] = ipl;
			}
		}
	} else {
		c *= 4;
		count_cycles(c);
	}
}

static void check_bus_error(uaecptr addr, int write, int fc)
{
	if (!testing_active)
		return;
	if (!write && (fc & 2)) {
		test_memory_access_mask |= 4;
	} else if (!write && !(fc & 2)) {
		test_memory_access_mask |= 1;
	} else if (write) {
		test_memory_access_mask |= 2;
	}
	if (safe_memory_start == 0xffffffff && safe_memory_end == 0xffffffff)
		return;
	if (addr >= safe_memory_start && addr < safe_memory_end) {
		hardware_bus_error_fake = -1;
		if ((safe_memory_mode & 4) && !write && (fc & 2)) {
			hardware_bus_error |= 4;
			hardware_bus_error_fake |= 1;
		} else if ((safe_memory_mode & 1) && !write && !(fc & 2)) {
			hardware_bus_error |= 1;
			hardware_bus_error_fake |= 1;
		} else if ((safe_memory_mode & 2) && write) {
			hardware_bus_error |= 2;
			hardware_bus_error_fake |= 2;
		}
		if (!write && (fc & 2) && feature_usp == 3) {
			out_of_test_space = true;
			out_of_test_space_addr = addr;
		}
	}
}


static uae_u8 get_ibyte_test(uaecptr addr)
{
	check_bus_error(addr, 0, regs.s ? 5 : 1);
	uae_u8 *p = get_addr(addr, 1, 4);
	add_memory_cycles(addr, 1);
	return *p;
}

static uae_u16 get_iword_test(uaecptr addr)
{
	check_bus_error(addr, 0, regs.s ? 6 : 2);
	if (addr & 1) {
		return (get_ibyte_test(addr + 0) << 8) | (get_ibyte_test(addr + 1) << 0);
	} else {
		uae_u8 *p = get_addr(addr, 2, 4);
		add_memory_cycles(addr, 1);
		return (p[0] << 8) | (p[1]);
	}
}

uae_u32 get_ilong_test(uaecptr addr)
{
	uae_u32 v;
	check_bus_error(addr, 0, regs.s ? 6 : 2);
	if (addr & 1) {
		uae_u8 v0 = get_ibyte_test(addr + 0);
		uae_u16 v1 = get_iword_test(addr + 1);
		uae_u8 v3 = get_ibyte_test(addr + 3);
		v = (v0 << 24) | (v1 << 8) | (v3 << 0);
	} else if (addr & 2) {
		uae_u16 v0 = get_iword_test(addr + 0);
		uae_u16 v1 = get_iword_test(addr + 2);
		v = (v0 << 16) | (v1 << 0);
	} else {
		uae_u8 *p = get_addr(addr, 4, 4);
		v = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3]);
		add_memory_cycles(addr, 2);
	}
	return v;
}

uae_u16 get_word_test_prefetch(int o)
{
	// no real prefetch
	if (cpu_lvl < 2) {
		o -= 2;
	}
	cycle_count_disable = 1;
	regs.irc = get_iword_test(m68k_getpci() + o + 2);
	cycle_count_disable = 0;
	read_buffer_prev = regs.read_buffer;
	regs.read_buffer = regs.irc;
	uae_u16 v = get_iword_test(m68k_getpci() + o);
	return v;
}

static void previoussame(uaecptr addr, int size, uae_u32 *old)
{
	if (!ahcnt_current || ahcnt_current == ahcnt_written)
		return;
	// Move from SR does two writes to same address.
	// Loop mode can write different values to same address.
	// Mark old values as do not save.
	// Also loop mode test can do multi writes and it needs original value.
	bool gotold = false;
	for (int i = ahcnt_written; i < ahcnt_current; i++) {
		struct accesshistory  *ah = &ahist[i];
		if (((!feature_loop_mode_jit && !feature_loop_mode_68010) || !testing_active) && ah->size == size && ah->addr == addr) {
			ah->donotsave = true;
			if (!gotold) {
				*old = ah->oldval;
				gotold = true;
			}
		}
		if (cpu_lvl < 2) {
			if (size == sz_long) {
				if (ah->size == sz_word && ah->addr == addr) {
					ah->donotsave = true;
				}
				if (ah->size == sz_word && ah->addr == addr + 2) {
					ah->donotsave = true;
				}
			}
		}
	}
}

void put_byte_test(uaecptr addr, uae_u32 v)
{
	if (!testing_active && is_nowrite_address(addr, 1))
		return;
	if (feature_interrupts >= 2) {
		if (addr == IPL_TRIGGER_ADDR) {
			add_memory_cycles(addr, 1);
#if IPL_TRIGGER_ADDR_SIZE == 1
			interrupt_cycle_cnt = INTERRUPT_CYCLES;
#endif
			return;
		}
	}
	check_bus_error(addr, 1, regs.s ? 5 : 1);
	uae_u8 *p = get_addr(addr, 1, 2);
	if (!out_of_test_space && !noaccesshistory && !hardware_bus_error_fake) {
		uae_u32 old = p[0];
		previoussame(addr, sz_byte, &old);
		if (ahcnt_current >= MAX_ACCESSHIST) {
			wprintf(_T(" ahist overflow!"));
			abort();
		}
		struct accesshistory *ah = &ahist[ahcnt_current++];
		ah->addr = addr;
		ah->val = v & 0xff;
		ah->oldval = old & 0xff;
		ah->size = sz_byte;
		ah->donotsave = false;
	}
	regs.write_buffer &= 0xff00;
	regs.write_buffer |= v & 0xff;
	*p = v;
	add_memory_cycles(addr, 1);
}
void put_word_test(uaecptr addr, uae_u32 v)
{
	if (!testing_active && is_nowrite_address(addr, 1))
		return;
	if (feature_interrupts >= 2) {
		if (addr == IPL_BLTSIZE) {
			add_memory_cycles(addr, 1);
			waitstate_cycle_cnt = cpu_cycles;
			return;
		}
		if (addr == IPL_TRIGGER_ADDR) {
			add_memory_cycles(addr, 1);
#if IPL_TRIGGER_ADDR_SIZE == 2
			interrupt_cycle_cnt = INTERRUPT_CYCLES;
#endif
			return;
		}
	}
	check_bus_error(addr, 1, regs.s ? 5 : 1);
	if (addr & 1) {
		put_byte_test(addr + 0, v >> 8);
		put_byte_test(addr + 1, v >> 0);
	} else {
		uae_u8 *p = get_addr(addr, 2, 2);
		if (!out_of_test_space && !noaccesshistory && !hardware_bus_error_fake) {
			uae_u32 old = (p[0] << 8) | p[1];
			previoussame(addr, sz_word, &old);
			if (ahcnt_current >= MAX_ACCESSHIST) {
				wprintf(_T(" ahist overflow!"));
				abort();
			}
			struct accesshistory *ah = &ahist[ahcnt_current++];
			ah->addr = addr;
			ah->val = v & 0xffff;
			ah->oldval = old & 0xffff;
			ah->size = sz_word;
			ah->donotsave = false;
		}
		p[0] = v >> 8;
		p[1] = v & 0xff;
	}
	regs.write_buffer = v;
	add_memory_cycles(addr, 1);
}
void put_long_test(uaecptr addr, uae_u32 v)
{
	if (!testing_active && is_nowrite_address(addr, 1))
		return;
	check_bus_error(addr, 1, regs.s ? 5 : 1);
	if (addr & 1) {
		put_byte_test(addr + 0, v >> 24);
		put_word_test(addr + 1, v >> 8);
		put_byte_test(addr + 3, v >> 0);
	} else if (addr & 2) {
		put_word_test(addr + 0, v >> 16);
		put_word_test(addr + 2, v >> 0);
	} else {
		uae_u8 *p = get_addr(addr, 4, 2);
		if (!out_of_test_space && !noaccesshistory && !hardware_bus_error_fake) {
			uae_u32 old = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
			previoussame(addr, sz_long, &old);
			if (ahcnt_current >= MAX_ACCESSHIST) {
				wprintf(_T(" ahist overflow!"));
				abort();
			}
			struct accesshistory *ah = &ahist[ahcnt_current++];
			ah->addr = addr;
			ah->val = v;
			ah->oldval = old;
			ah->size = sz_long;
			ah->donotsave = false;
		}
		p[0] = v >> 24;
		p[1] = v >> 16;
		p[2] = v >> 8;
		p[3] = v >> 0;
		add_memory_cycles(addr, 2);
	}
	regs.write_buffer = v;
}

static void undo_memory(struct accesshistory *ahp, int end)
{
	out_of_test_space = 0;
	noaccesshistory = 1;
	for (int i = ahcnt_current - 1; i >= end; i--) {
		struct accesshistory *ah = &ahp[i];
		switch (ah->size)
		{
		case sz_byte:
			put_byte_test(ah->addr, ah->oldval);
			break;
		case sz_word:
			put_word_test(ah->addr, ah->oldval);
			break;
		case sz_long:
			put_long_test(ah->addr, ah->oldval);
			break;
		}
	}
	noaccesshistory = 0;
	if (out_of_test_space) {
		wprintf(_T(" undo_memory out of test space fault!?\n"));
		abort();
	}
	ahcnt_current = end;
}

uae_u32 get_byte_test(uaecptr addr)
{
	check_bus_error(addr, 0, regs.s ? 5 : 1);
	uae_u8 *p = get_addr(addr, 1, 1);
	read_buffer_prev = regs.read_buffer;
	regs.read_buffer &= 0xff00;
	regs.read_buffer |= *p;
	add_memory_cycles(addr, 1);
	return *p;
}
uae_u32 get_word_test(uaecptr addr)
{
	uae_u16 v;
	check_bus_error(addr, 0, regs.s ? 5 : 1);
	if (addr & 1) {
		v = (get_byte_test(addr + 0) << 8) | (get_byte_test(addr + 1) << 0);
	} else {
		uae_u8 *p = get_addr(addr, 2, 1);
		v = (p[0] << 8) | (p[1]);
	}
	read_buffer_prev = regs.read_buffer;
	regs.read_buffer = v;
	add_memory_cycles(addr, 1);
	return v;
}
uae_u32 get_long_test(uaecptr addr)
{
	uae_u32 v;
	check_bus_error(addr, 0, regs.s ? 5 : 1);
	if (addr & 1) {
		uae_u8 v0 = get_byte_test(addr + 0);
		uae_u16 v1 = get_word_test(addr + 1);
		uae_u8 v3 = get_byte_test(addr + 3);
		v = (v0 << 24) | (v1 << 8) | (v3 << 0);
	} else if (addr & 2) {
		uae_u16 v0 = get_word_test(addr + 0);
		uae_u16 v1 = get_word_test(addr + 2);
		v = (v0 << 16) | (v1 << 0);
	} else {
		uae_u8 *p = get_addr(addr, 4, 1);
		v = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3]);
		add_memory_cycles(addr, 2);
	}
	read_buffer_prev = regs.read_buffer;
	regs.read_buffer = v;
	return v;
}

uae_u32 get_byte_debug(uaecptr addr)
{
	uae_u8 *p = get_addr(addr, 1, -1);
	return *p;
}
uae_u32 get_word_debug(uaecptr addr)
{
	uae_u8 *p = get_addr(addr, 2, -1);
	return (p[0] << 8) | (p[1]);
}
uae_u32 get_long_debug(uaecptr addr)
{
	uae_u8 *p = get_addr(addr, 4, -1);
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3]);
}
uae_u32 get_iword_debug(uaecptr addr)
{
	return get_word_debug(addr);
}
uae_u32 get_ilong_debug(uaecptr addr)
{
	return get_long_debug(addr);
}

uae_u32 get_byte_cache_debug(uaecptr addr, bool *cached)
{
	*cached = false;
	return get_byte_test(addr);
}
uae_u32 get_word_cache_debug(uaecptr addr, bool *cached)
{
	*cached = false;
	return get_word_test(addr);
}
uae_u32 get_long_cache_debug(uaecptr addr, bool *cached)
{
	*cached = false;
	return get_long_test(addr);
}

uae_u32 sfc_nommu_get_byte(uaecptr addr)
{
	return get_byte_test(addr);
}
uae_u32 sfc_nommu_get_word(uaecptr addr)
{
	return get_word_test(addr);
}
uae_u32 sfc_nommu_get_long(uaecptr addr)
{
	return get_long_test(addr);
}

void dfc_nommu_put_byte(uaecptr addr, uae_u32 v)
{
	put_byte_test(addr, v);
}
void dfc_nommu_put_word(uaecptr addr, uae_u32 v)
{
	put_word_test(addr, v);
}
void dfc_nommu_put_long(uaecptr addr, uae_u32 v)
{
	put_long_test(addr, v);
}

uae_u16 get_wordi_test(int o)
{
	uae_u32 v = get_word_test_prefetch(o);
	regs.pc += 2;
	return v;
}

uae_u32 memory_get_byte(uaecptr addr)
{
	return get_byte_test(addr);
}
uae_u32 memory_get_word(uaecptr addr)
{
	return get_word_test(addr);
}
uae_u32 memory_get_wordi(uaecptr addr)
{
	return get_iword_test(addr);
}
uae_u32 memory_get_long(uaecptr addr)
{
	return get_long_test(addr);
}
uae_u32 memory_get_longi(uaecptr addr)
{
	return get_ilong_test(addr);
}

void memory_put_long(uaecptr addr, uae_u32 v)
{
	put_long_test(addr, v);
}
void memory_put_word(uaecptr addr, uae_u32 v)
{
	put_word_test(addr, v);
}
void memory_put_byte(uaecptr addr, uae_u32 v)
{
	put_byte_test(addr, v);
}

uae_u8 *memory_get_real_address(uaecptr addr)
{
	return NULL;
}

uae_u32 next_iword_test(void)
{
	uae_u32 v = get_word_test_prefetch(0);
	regs.pc += 2;
	return v;
}

uae_u32 next_ilong_test(void)
{
	uae_u32 v = get_word_test_prefetch(0) << 16;
	v |= get_word_test_prefetch(2);
	regs.pc += 4;
	return v;
}

bool mmu_op30(uaecptr pc, uae_u32 opcode, uae_u16 extra, uaecptr extraa)
{
	m68k_setpc(pc);
	op_illg_noret(opcode);
	return true;
}

bool is_cycle_ce(uaecptr addr)
{
	return 0;
}

void ipl_fetch_next_pre(void)
{
	ipl_fetch_next();
	regs.ipl_evt_pre = cpu_cycles;
	regs.ipl_evt_pre_mode = 1;
}

void ipl_fetch_now_pre(void)
{
	ipl_fetch_now();
	regs.ipl_evt_pre = cpu_cycles;
	regs.ipl_evt_pre_mode = 0;
}

// ipl check was early enough, interrupt possible after current instruction
void ipl_fetch_now(void)
{
	int c = cpu_cycles;
	regs.ipl_evt = c;
	regs.ipl[0] = regs.ipl_pin;
	regs.ipl[1] = 0;
}
// ipl check was too late, interrupt possible after following instruction
void ipl_fetch_next(void)
{
	int c = cpu_cycles;
	if (c - regs.ipl_pin_change_evt >= cpuipldelay4) {
		regs.ipl[0] = regs.ipl_pin;
		regs.ipl[1] = 0;
	} else {
		regs.ipl[1] = regs.ipl_pin;
	}
}

int intlev(void)
{
	return interrupt_level;
}

void do_cycles_stop(int c)
{
	do_cycles_test(c);
}

uae_u32(*x_get_long)(uaecptr);
uae_u32(*x_get_word)(uaecptr);
uae_u32(*x_get_byte)(uaecptr);
void (*x_put_long)(uaecptr, uae_u32);
void (*x_put_word)(uaecptr, uae_u32);
void (*x_put_byte)(uaecptr, uae_u32);

uae_u32(*x_cp_get_long)(uaecptr);
uae_u32(*x_cp_get_word)(uaecptr);
uae_u32(*x_cp_get_byte)(uaecptr);
void (*x_cp_put_long)(uaecptr, uae_u32);
void (*x_cp_put_word)(uaecptr, uae_u32);
void (*x_cp_put_byte)(uaecptr, uae_u32);
uae_u32(*x_cp_next_iword)(void);
uae_u32(*x_cp_next_ilong)(void);

uae_u32(*x_next_iword)(void);
uae_u32(*x_next_ilong)(void);
void (*x_do_cycles)(int);

uae_u32(REGPARAM3 *x_cp_get_disp_ea_020)(uae_u32 base, int idx) REGPARAM;

void m68k_do_rts_ce(void)
{
	uaecptr pc;
	pc = x_get_word(m68k_areg(regs, 7)) << 16;
	pc |= x_get_word(m68k_areg(regs, 7) + 2);
	m68k_areg(regs, 7) += 4;
	m68k_setpci(pc);
}

void m68k_do_bsr_ce(uaecptr oldpc, uae_s32 offset)
{
	m68k_areg(regs, 7) -= 4;
	x_put_word(m68k_areg(regs, 7), oldpc >> 16);
	x_put_word(m68k_areg(regs, 7) + 2, oldpc);
	m68k_incpci(offset);
}

void m68k_do_jsr_ce(uaecptr oldpc, uaecptr dest)
{
	m68k_areg(regs, 7) -= 4;
	x_put_word(m68k_areg(regs, 7), oldpc >> 16);
	x_put_word(m68k_areg(regs, 7) + 2, oldpc);
	m68k_setpci(dest);
}

static int flag_SPCFLAG_TRACE, flag_SPCFLAG_DOTRACE;

uae_u32 get_disp_ea_test(uae_u32 base, uae_u32 dp)
{
	int reg = (dp >> 12) & 15;
	uae_s32 regd = regs.regs[reg];
	if ((dp & 0x800) == 0)
		regd = (uae_s32)(uae_s16)regd;
	return base + (uae_s32)((uae_s8)(dp)) + regd;
}

static void activate_trace(void)
{
	flag_SPCFLAG_TRACE = 0;
	flag_SPCFLAG_DOTRACE = 1;
}

static void do_trace(void)
{
	if (cpu_stopped) {
		return;
	}
	regs.trace_pc = regs.pc;
	if (regs.t0 && !regs.t1 && currprefs.cpu_model >= 68020) {
		// this is obsolete
		return;
	}
	if (regs.t1) {
		activate_trace();
	}
}

void REGPARAM2 MakeSR(void)
{
	regs.sr = ((regs.t1 << 15) | (regs.t0 << 14)
		| (regs.s << 13) | (regs.m << 12) | (regs.intmask << 8)
		| (GET_XFLG() << 4) | (GET_NFLG() << 3)
		| (GET_ZFLG() << 2) | (GET_VFLG() << 1)
		| GET_CFLG());
}
void MakeFromSR_x(int t0trace)
{
	int oldm = regs.m;
	int olds = regs.s;
	int oldt0 = regs.t0;
	int oldt1 = regs.t1;

	SET_XFLG((regs.sr >> 4) & 1);
	SET_NFLG((regs.sr >> 3) & 1);
	SET_ZFLG((regs.sr >> 2) & 1);
	SET_VFLG((regs.sr >> 1) & 1);
	SET_CFLG(regs.sr & 1);

	if (regs.t1 == ((regs.sr >> 15) & 1) &&
		regs.t0 == ((regs.sr >> 14) & 1) &&
		regs.s  == ((regs.sr >> 13) & 1) &&
		regs.m  == ((regs.sr >> 12) & 1) &&
		regs.intmask == ((regs.sr >> 8) & 7))
		return;

	regs.t1 = (regs.sr >> 15) & 1;
	regs.t0 = (regs.sr >> 14) & 1;
	regs.s  = (regs.sr >> 13) & 1;
	regs.m  = (regs.sr >> 12) & 1;
	
	if (regs.intmask != ((regs.sr >> 8) & 7)) {
		int newimask = (regs.sr >> 8) & 7;
		// STOP intmask change enabling already active interrupt: delay it by 1 STOP round
		if (t0trace < 0 && regs.ipl[0] <= regs.intmask && regs.ipl[0] > newimask && regs.ipl[0] < 7) {
			regs.ipl[0] = 0;
		}
		regs.intmask = newimask;
	}

	if (currprefs.cpu_model >= 68020) {
		/* 68060 does not have MSP but does have M-bit.. */
		if (currprefs.cpu_model >= 68060)
			regs.msp = regs.isp;
		if (olds != regs.s) {
			if (olds) {
				if (oldm)
					regs.msp = m68k_areg (regs, 7);
				else
					regs.isp = m68k_areg (regs, 7);
				m68k_areg (regs, 7) = regs.usp;
			} else {
				regs.usp = m68k_areg (regs, 7);
				m68k_areg (regs, 7) = regs.m ? regs.msp : regs.isp;
			}
		} else if (olds && oldm != regs.m) {
			if (oldm) {
				regs.msp = m68k_areg (regs, 7);
				m68k_areg (regs, 7) = regs.isp;
			} else {
				regs.isp = m68k_areg (regs, 7);
				m68k_areg (regs, 7) = regs.msp;
			}
		}
		if (currprefs.cpu_model >= 68060)
			regs.t0 = 0;
	} else {
		regs.t0 = regs.m = 0;
		if (olds != regs.s) {
			if (olds) {
				regs.isp = m68k_areg (regs, 7);
				m68k_areg (regs, 7) = regs.usp;
			} else {
				regs.usp = m68k_areg (regs, 7);
				m68k_areg (regs, 7) = regs.isp;
			}
		}
	}

	if (regs.t1 || regs.t0) {
		flag_SPCFLAG_TRACE = 1;
	} else {
		/* Keep SPCFLAG_DOTRACE, we still want a trace exception for
		SR-modifying instructions (including STOP).  */
		flag_SPCFLAG_TRACE = 0;
	}
	// STOP SR-modification does not generate T0
	// If this SR modification set Tx bit, no trace until next instruction.
	if (!cpu_stopped && ((oldt0 && t0trace && currprefs.cpu_model >= 68020) || oldt1)) {
		// Always trace if Tx bits were already set, even if this SR modification cleared them.
		activate_trace();
	}
}

void REGPARAM2 MakeFromSR_T0(void)
{
	MakeFromSR_x(1);
}
void REGPARAM2 MakeFromSR(void)
{
	MakeFromSR_x(0);
}

void REGPARAM2 MakeFromSR_STOP(void)
{
	MakeFromSR_x(-1);
}

void REGPARAM2 MakeFromSR_intmask(uae_u16 oldsr, uae_u16 newsr)
{
}

void intlev_load(void)
{
}

void checkint(void)
{
}

void cpu_halt(int halt)
{
	cpu_halted = halt;
}

void m68k_setstopped(int stoptype)
{
	cpu_stopped = stoptype;
}

void check_t0_trace(void)
{
	if (regs.t0 && !regs.t1 && currprefs.cpu_model >= 68020) {
		flag_SPCFLAG_TRACE = 0;
		flag_SPCFLAG_DOTRACE = 1;
	}
}

bool cpureset(void)
{
	cpu_halted = -1;
	return false;
}

static void doexcstack2(void)
{
	// generate exception but don't store it with test results

	int noac = noaccesshistory;
	int ta = testing_active;
	int cycs = cpu_cycles;
	noaccesshistory = 1;
	testing_active = -1;

	int opcode = (opcode_memory[0] << 8) | (opcode_memory[1]);
	if (test_exception_opcode >= 0) {
		opcode = test_exception_opcode;
	}
	if (flag_SPCFLAG_DOTRACE && test_exception == 9) {
		flag_SPCFLAG_DOTRACE = 0;
	}

	int sv = regs.s;
	uaecptr tmp = m68k_areg(regs, 7);
	m68k_areg(regs, 7) = test_memory_end + EXTRA_RESERVED_SPACE;
	if (cpu_lvl == 0) {
		if (test_exception == 2 || test_exception == 3) {
			uae_u16 mode = (sv ? 4 : 0) | test_exception_3_fc;
			mode |= test_exception_3_w ? 0 : 16;
			Exception_build_68000_address_error_stack_frame(mode, opcode, test_exception_addr, regs.pc);
			flag_SPCFLAG_DOTRACE = 0;
		} else {
			m68k_areg(regs, 7) -= 4;
			x_put_long(m68k_areg(regs, 7), regs.pc);
			m68k_areg(regs, 7) -= 2;
			x_put_word(m68k_areg(regs, 7), regs.sr);
		}
	} else if (cpu_lvl == 1) {
		if (test_exception == 2 || test_exception == 3) {
			uae_u16 ssw = (sv ? 4 : 0) | test_exception_3_fc;
			ssw |= test_exception_3_di > 0 ? 0x0000 : (test_exception_3_di < 0 ? (0x2000 | 0x1000) : 0x2000);
			ssw |= (!test_exception_3_w && test_exception_3_di) ? 0x1000 : 0x000; // DF
			ssw |= (test_exception_opcode & 0x10000) ? 0x0400 : 0x0000; // HB
			ssw |= test_exception_3_size == 0 ? 0x0200 : 0x0000; // BY
			ssw |= test_exception_3_w ? 0x0000 : 0x0100; // RW
			ssw |= (test_exception_opcode & 0x80000) ? 0x0800 : 0x0000; // RM
			regs.mmu_fault_addr = test_exception_addr;
			Exception_build_stack_frame(regs.instruction_pc, regs.pc, ssw, test_exception, 0x08);
			flag_SPCFLAG_DOTRACE = 0;
		} else {
			Exception_build_stack_frame_common(regs.instruction_pc, regs.pc, 0, test_exception, test_exception);
		}
	} else if (cpu_lvl == 2 || cpu_lvl == 3) {
		if (test_exception == 3) {
			uae_u16 ssw = (sv ? 4 : 0) | test_exception_3_fc;
			ssw |= 0x20;
			regs.mmu_fault_addr = test_exception_addr;
			Exception_build_stack_frame(regs.instruction_pc, regs.pc, ssw, test_exception, 0x0b);
		} else {
			Exception_build_stack_frame_common(regs.instruction_pc, regs.pc, 0, test_exception, test_exception);
		}
	} else {
		if (test_exception == 3) {
			if (currprefs.cpu_model >= 68040)
				test_exception_addr &= ~1;
			Exception_build_stack_frame(test_exception_addr, regs.pc, 0, test_exception, 0x02);
		} else {
			Exception_build_stack_frame_common(regs.instruction_pc, regs.pc, 0, test_exception, test_exception);
		}
	}
	exception_stack_frame_size = test_memory_end + EXTRA_RESERVED_SPACE - m68k_areg(regs, 7);

	m68k_areg(regs, 7) = tmp;
	testing_active = ta;
	noaccesshistory = noac;
	cpu_cycles = cycs;
}

static void doexcstack(void)
{
	bool g1 = generates_group1_exception(regs.ir);
	uaecptr original_pc = regs.pc;

	doexcstack2();
	if (test_exception < 4)
		return;

	// did we got bus error or address error
	// when fetching exception vector?
	// (bus error not yet tested)
	if (!feature_exception_vectors)
		return;

	int interrupt = test_exception >= 24 && test_exception < 24 + 8;
	int original_exception = test_exception;
	// odd exception vector
	test_exception = 3;
	test_exception_addr = feature_exception_vectors;

	// store original exception stack (which may not be complete)
	uae_u8 *sf = test_memory + test_memory_size + EXTRA_RESERVED_SPACE - exception_stack_frame_size;
	exception_extra_frame_size = exception_stack_frame_size;
	exception_extra_frame_type = original_exception;
	memcpy(exception_extra_frame, sf, exception_extra_frame_size);

	MakeSR();
	regs.sr |= 0x2000;
	regs.sr &= ~(0x8000 | 0x4000);
	MakeFromSR();

	regs.pc = original_exception * 4;

	int flags = 0;
	if (cpu_lvl == 1) {
		// IF = 1
		flags |= 0x40000;
		// low word of address
		regs.irc = (uae_u16)test_exception_addr;
		// low word of address
		regs.read_buffer = regs.irc;
		// vector offset (not vbr + offset)
		regs.write_buffer = original_exception * 4;
		regs.pc = original_pc;
	}

	if (interrupt) {
		regs.intmask = original_exception - 24;
		regs.ir = original_exception;
		if (cpu_lvl == 0) {
			flags |= 0x10000 | 0x20000;
		} else {
			flags |= 0x20000;
		}
		if (cpu_lvl < 5) {
			regs.m = 0;
		}
	}

	// set I/N if original exception was group 1 exception.
	flags |= 0x20000;
	if (g1 && cpu_lvl == 0) {
		flags |= 0x10000;
	}

	exception3_read(regs.ir | flags, test_exception_addr, 1, 2);
}

void REGPARAM2 op_illg_1_noret(uae_u32 opcode)
{
	if ((opcode & 0xf000) == 0xf000) {
		if (currprefs.cpu_model == 68030) {
			// mmu instruction extra check
			// because mmu checks following word to detect if addressing mode is supported,
			// we can't test any MMU opcodes without parsing following word. TODO.
			if ((opcode & 0xfe00) == 0xf000) {
				test_exception = -1;
				return;
			}
		}
		test_exception = 11;
		if (privileged_copro_instruction(opcode)) {
			test_exception = 8;
		}
	} else if ((opcode & 0xf000) == 0xa000) {
		test_exception = 10;
	} else {
		test_exception = 4;
	}
	doexcstack();
}
void REGPARAM2 op_unimpl(uae_u32 opcode)
{
	test_exception = 61;
	doexcstack();
}
void REGPARAM2 op_unimpl_1_noret(uae_u32 opcode)
{
	if ((opcode & 0xf000) == 0xf000 || currprefs.cpu_model < 68060) {
		op_illg_noret(opcode);
	} else {
		op_unimpl(opcode);
	}
}
uae_u32 REGPARAM2 op_unimpl_1(uae_u32 opcode)
{
	op_unimpl_1_noret(opcode);
	return 0;
}

void REGPARAM2 op_illg_noret(uae_u32 opcode)
{
	op_illg_1_noret(opcode);
}
uae_u32 REGPARAM2 op_illg_1(uae_u32 opcode)
{
	op_illg_1_noret(opcode);
	return 0;
}
uae_u32 REGPARAM2 op_illg(uae_u32 opcode)
{
	op_illg_1_noret(opcode);
	return 0;
}

static void exception3_pc_inc(void)
{
	if (currprefs.cpu_model == 68000) {
		m68k_incpci(2);
	}
}


static void exception2_fetch_common(uae_u32 opcode, int offset)
{
	test_exception = 2;
	test_exception_3_w = 0;
	test_exception_addr = m68k_getpci() + offset;
	test_exception_opcode = opcode;
	test_exception_3_fc = 2;
	test_exception_3_size = sz_word;
	test_exception_3_di = 0;

	if (currprefs.cpu_model == 68000) {
		if (generates_group1_exception(regs.ir) && !(opcode & 0x20000)) {
			test_exception_3_fc |= 8;  // set N/I
		}
		if (opcode & 0x10000)
			test_exception_3_fc |= 8;
	}
}

void exception2_fetch(uae_u32 opcode, int offset, int pcoffset)
{
	exception2_fetch_common(opcode, offset);
	regs.pc = test_exception_addr;
	regs.pc += pcoffset;
	doexcstack();
}

void exception2_fetch_opcode(uae_u32 opcode, int offset, int pcoffset)
{
	exception2_fetch_common(opcode, offset);
	if (currprefs.cpu_model == 68010) {
		test_exception_3_di = -1;
	}
	regs.pc = test_exception_addr;
	regs.pc += pcoffset;
	doexcstack();
}

void exception2_read(uae_u32 opcode, uaecptr addr, int size, int fc)
{
	test_exception = 2;
	test_exception_3_w = 0;
	test_exception_addr = addr;
	test_exception_opcode = opcode;
	test_exception_3_fc = fc;
	test_exception_3_size = size & 15;
	test_exception_3_di = 1;

	if (currprefs.cpu_model == 68000) {
		if (generates_group1_exception(regs.ir) && !(opcode & 0x20000)) {
			test_exception_3_fc |= 8;  // set N/I
		}
		if (opcode & 0x10000)
			test_exception_3_fc |= 8;
		if (!(opcode & 0x20000))
			test_exception_opcode = regs.ir;
	}

	doexcstack();
}

void exception2_write(uae_u32 opcode, uaecptr addr, int size, uae_u32 val, int fc)
{
	test_exception = 2;
	test_exception_3_w = 1;
	test_exception_addr = addr;
	test_exception_opcode = opcode;
	test_exception_3_fc = fc;
	test_exception_3_size = size & 15;
	if (size & 0x100) {
		regs.write_buffer = val;
	} else {
		if (size == sz_byte) {
			regs.write_buffer &= 0xff00;
			regs.write_buffer |= val & 0xff;
		} else {
			regs.write_buffer = val;
		}
	}
	test_exception_3_di = 1;

	if (currprefs.cpu_model == 68000) {
		if (generates_group1_exception(regs.ir) && !(opcode & 0x20000)) {
			test_exception_3_fc |= 8;  // set N/I
		}
		if (opcode & 0x10000)
			test_exception_3_fc |= 8;
		if (!(opcode & 0x20000))
			test_exception_opcode = regs.ir;
	}

	doexcstack();
}

void exception3_read(uae_u32 opcode, uae_u32 addr, int size, int fc)
{
	test_exception = 3;
	test_exception_3_w = 0;
	test_exception_addr = addr;
	test_exception_opcode = opcode;
	test_exception_3_fc = fc;
	test_exception_3_size = size & 15;
	test_exception_3_di = 1;

	if (currprefs.cpu_model == 68000) {
		if (generates_group1_exception(regs.ir) && !(opcode & 0x20000)) {
			test_exception_3_fc |= 8; // set N/I
		}
		if (opcode & 0x10000)
			test_exception_3_fc |= 8;
		test_exception_opcode = regs.ir;
	}
	if (currprefs.cpu_model == 68010) {
		if (opcode & 0x40000) {
			test_exception_3_di = 0;
		}
	}

	doexcstack();
}
void exception3_write(uae_u32 opcode, uae_u32 addr, int size, uae_u32 val, int fc)
{
	test_exception = 3;
	test_exception_3_w = 1;
	test_exception_addr = addr;
	test_exception_opcode = opcode;
	test_exception_3_fc = fc;
	test_exception_3_size = size & 15;
	regs.write_buffer = val;
	test_exception_3_di = 1;

	if (currprefs.cpu_model == 68000) {
		if (generates_group1_exception(regs.ir) && !(opcode & 0x20000)) {
			test_exception_3_fc |= 8; // set N/I
		}
		if (opcode & 0x10000)
			test_exception_3_fc |= 8;
		test_exception_opcode = regs.ir;
	}

	doexcstack();
}

// load to irc only
void exception3_read_prefetch_only(uae_u32 opcode, uae_u32 addr)
{
	if (cpu_lvl == 1) {
		uae_u16 prev = regs.read_buffer;
		get_word_test(addr & ~1);
		regs.irc = regs.read_buffer;
		regs.read_buffer = prev;
	} else {
		add_memory_cycles(addr, 1);
	}
	test_exception = 3;
	test_exception_3_w = 0;
	test_exception_addr = addr;
	test_exception_opcode = opcode;
	test_exception_3_fc = 2;
	test_exception_3_size = sz_word;
	test_exception_3_di = 0;

	doexcstack();
}

void exception3_read_prefetch(uae_u32 opcode, uae_u32 addr)
{
	if (cpu_lvl == 1) {
		get_word_test(addr & ~1);
	} else {
		add_memory_cycles(addr, 1);
	}
	exception3_pc_inc();

	test_exception = 3;
	test_exception_3_w = 0;
	test_exception_addr = addr;
	test_exception_opcode = opcode;
	test_exception_3_fc = 2;
	test_exception_3_size = sz_word;
	test_exception_3_di = 0;

	doexcstack();
}

void exception3_read_prefetch_68040bug(uae_u32 opcode, uae_u32 addr, uae_u16 secondarysr)
{
	if (cpu_lvl == 1) {
		get_word_test(addr & ~1);
	} else {
		add_memory_cycles(addr, 1);
	}
	exception3_pc_inc();

	test_exception = 3;
	test_exception_3_w = 0;
	test_exception_addr = addr;
	test_exception_opcode = opcode | 0x10000;
	test_exception_3_fc = 2;
	test_exception_3_size = sz_word;
	test_exception_3_di = 0;
	test_exception_3_sr = secondarysr;

	doexcstack();
}


void exception3_read_access2(uae_u32 opcode, uae_u32 addr, int size, int fc)
{
	get_word_test(addr & ~1);
	get_word_test(addr & ~1);
	exception3_read(opcode, addr, size, fc);
}

void exception3_read_access(uae_u32 opcode, uae_u32 addr, int size, int fc)
{
	if (cpu_lvl == 1) {
		get_word_test(addr & ~1);
	} else {
		add_memory_cycles(addr, 1);
	}
	exception3_read(opcode, addr, size, fc);
}

void exception3_write_access(uae_u32 opcode, uae_u32 addr, int size, uae_u32 val, int fc)
{
	add_memory_cycles(addr, 1);
	exception3_write(opcode, addr, size, val, fc);
}

uae_u16 exception3_word_read(uaecptr addr)
{
	add_memory_cycles(addr, 1);
	return 0;
}

void REGPARAM2 Exception(int n)
{
	if (cpu_stopped) {
		m68k_incpci(4);
		cpu_stopped = 0;
	}

	test_exception = n;
	test_exception_addr = m68k_getpci();
	test_exception_opcode = -1;
	doexcstack();
}
void REGPARAM2 Exception_cpu_oldpc(int n, uaecptr oldpc)
{
	test_exception = n;
	test_exception_addr = m68k_getpci();
	test_exception_opcode = -1;

	bool t0 = currprefs.cpu_model >= 68020 && regs.t0 && !regs.t1;
	if (n != 14) {
		// check T0 trace
		if (t0) {
			activate_trace();
		}
	}
	doexcstack();
}
void REGPARAM2 Exception_cpu(int n)
{
	Exception_cpu_oldpc(n, 0xffffffff);
}
void exception3i(uae_u32 opcode, uaecptr addr)
{
	test_exception = 3;
	test_exception_3_fc = 2;
	test_exception_3_w = 0;
	test_exception_addr = addr;
	test_exception_opcode = opcode;
	test_exception_3_di = 0;
	test_exception_3_size = sz_word;
	doexcstack();
}
void exception3b(uae_u32 opcode, uaecptr addr, bool w, bool i, uaecptr pc)
{
	test_exception = 3;
	test_exception_3_fc = i ? 2 : 1;
	test_exception_3_w = w;
	test_exception_addr = addr;
	test_exception_opcode = opcode;
	test_exception_3_di = 0;
	test_exception_3_size = sz_word;
	doexcstack();
}

#define CCR_C 1
#define CCR_V 2
#define CCR_Z 4
#define CCR_N 8

void getcc(int cc, uae_u8 *ccrorp, uae_u8 *ccrandp)
{
	uae_u8 cor = 0, cand = 0;
	switch (cc)
	{
	case 2: cand |= CCR_C | CCR_Z; break;
	case 3: cor |= CCR_C | CCR_Z; break;
	case 4: cand |= CCR_C; break;
	case 5: cor |= CCR_C; break;
	case 6: cand |= CCR_Z; break;
	case 7: cor |= CCR_Z; break;
	case 8: cand |= CCR_V; break;
	case 9: cor |= CCR_V; break;
	case 10: cand |= CCR_N; break;
	case 11: cor |= CCR_N; break;
	case 12: cor |= CCR_N | CCR_V; break;
	case 13: cor |= CCR_N; cand |= CCR_V; break;
	case 14: cor |= CCR_N | CCR_V; cand |= CCR_Z; break;
	case 15: cor |= CCR_N | CCR_Z; cand |= CCR_V; break;
	}
	cand ^= 0xff;
	*ccrorp = cor;
	*ccrandp = cand;
}

int cctrue(int cc)
{
	uae_u32 cznv = regflags.cznv;

	switch (cc) {
	case 0:  return 1;											/*					T  */
	case 1:  return 0;											/*					F  */
	case 2:  return (cznv & (FLAGVAL_C | FLAGVAL_Z)) == 0;		/* !CFLG && !ZFLG	HI */
	case 3:  return (cznv & (FLAGVAL_C | FLAGVAL_Z)) != 0;		/*  CFLG || ZFLG	LS */
	case 4:  return (cznv & FLAGVAL_C) == 0;					/* !CFLG			CC */
	case 5:  return (cznv & FLAGVAL_C) != 0;					/*  CFLG			CS */
	case 6:  return (cznv & FLAGVAL_Z) == 0;					/* !ZFLG			NE */
	case 7:  return (cznv & FLAGVAL_Z) != 0;					/*  ZFLG			EQ */
	case 8:  return (cznv & FLAGVAL_V) == 0;					/* !VFLG			VC */
	case 9:  return (cznv & FLAGVAL_V) != 0;					/*  VFLG			VS */
	case 10: return (cznv & FLAGVAL_N) == 0;					/* !NFLG			PL */
	case 11: return (cznv & FLAGVAL_N) != 0;					/*  NFLG			MI */

	case 12: /*  NFLG == VFLG		GE */
		return ((cznv >> FLAGBIT_N) & 1) == ((cznv >> FLAGBIT_V) & 1);
	case 13: /*  NFLG != VFLG		LT */
		return ((cznv >> FLAGBIT_N) & 1) != ((cznv >> FLAGBIT_V) & 1);
	case 14: /* !GET_ZFLG && (GET_NFLG == GET_VFLG);  GT */
		return !(cznv & FLAGVAL_Z) && (((cznv >> FLAGBIT_N) & 1) == ((cznv >> FLAGBIT_V) & 1));
	case 15: /* GET_ZFLG || (GET_NFLG != GET_VFLG);   LE */
		return (cznv & FLAGVAL_Z) || (((cznv >> FLAGBIT_N) & 1) != ((cznv >> FLAGBIT_V) & 1));
	}
	return 0;
}

static uae_u32 xorshiftstate, xorshiftstate_prev;
static uae_u32 xorshift32(void)
{
	uae_u32 x = xorshiftstate;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	xorshiftstate = x;
	return xorshiftstate;
}

static int rand16_cnt, rand16_cnt_prev;
static uae_u16 rand16(void)
{
	int cnt = rand16_cnt & 15;
	uae_u16 v = 0;
	rand16_cnt++;
	if (cnt < 15) {
		v = (uae_u16)xorshift32();
		if (cnt <= 6)
			v &= ~0x8000;
		else
			v |= 0x8000;
	}
	if (forced_immediate_mode == 1)
		v &= ~1;
	if (forced_immediate_mode == 2)
		v |= 1;
	return v;
}
static int rand32_cnt, rand32_cnt_prev;
static uae_u32 rand32(void)
{
	int cnt = rand32_cnt & 31;
	uae_u32 v = 0;
	rand32_cnt++;
	if (cnt < 31) {
		v = xorshift32();
		if (cnt <= 14)
			v &= ~0x80000000;
		else
			v |= 0x80000000;
	}
	if (forced_immediate_mode == 1)
		v &= ~1;
	if (forced_immediate_mode == 2)
		v |= 1;
	return v;
}

// first 3 values: positive
// next 4 values: negative
// last: zero
static int rand8_cnt, rand8_cnt_prev;
static uae_u8 rand8(void)
{
	int cnt = rand8_cnt & 7;
	uae_u8 v = 0;
	rand8_cnt++;
	if (cnt < 7) {
		v = (uae_u8)xorshift32();
		if (cnt <= 2)
			v &= ~0x80;
		else
			v |= 0x80;
	}
	if (forced_immediate_mode == 1)
		v &= ~1;
	if (forced_immediate_mode == 2)
		v |= 1;
	return v;
}

static uae_u8 frand8(void)
{
	return (uae_u8)xorshift32();
}

static bool fpu_precision_valid(floatx80 f)
{
	int exp = f.high & 0x7fff;
	if (exp != 0x0000) {
		exp -= 16384;
		if (exp < fpu_min_exponent || exp > fpu_max_exponent) {
			return false;
		}
	}
	float_status status = { 0 };
	status.floatx80_rounding_precision = 80;
	status.float_rounding_mode = float_round_nearest_even;
	status.float_exception_flags = 0;
	if (fpu_max_precision == 2) {
		float64 fo = floatx80_to_float64(f, &status);
		int exp = (float64_val(fo) >> 52) & 0x7FF;
		if (exp >= 0x700) {
			return false;
		}
		if (float64_is_nan(fo)) {
			return false;
		}
	} else if (fpu_max_precision == 1) {
		float32 fo = floatx80_to_float32(f, &status);
		int exp = (float32_val(fo) >> 23) & 0xFF;
		if (exp >= 0xe0) {
			return false;
		}
		if (float32_is_nan(fo)) {
			return false;
		}
	}
	if (fpu_max_precision) {
		if (status.float_exception_flags & (float_flag_underflow | float_flag_overflow | float_flag_denormal | float_flag_invalid)) {
			return false;
		}
		if (floatx80_is_any_nan(f)) {
			return false;
		}
	}
	if (!fpu_unnormals) {
		if (!(f.low & 0x8000000000000000) && (f.high & 0x7fff) && (f.high & 0x7fff) != 0x7fff) {
			return false;
		}
		if (status.float_exception_flags & float_flag_denormal) {
			return false;
		}
	}
	return true;
}

static uae_u32 registers[] =
{
	0x00000010,	// 0
	0x00000000,	// 1
	0xffffffff,	// 2
	0xffffff00,	// 3
	0xffff0000,	// 4
	0x80008080,	// 5
	0x00010101,	// 6
	0xaaaaaaaa,	// 7

	0x00000000,	// 8
	0x00000078,	// 9
	0x00007ff0,	// 10
	0x00007fff,	// 11
	0xfffffffe,	// 12
	0xffffff00,	// 13
	0x00000000, // 14 replaced with opcode memory
	0x00000000  // 15 replaced with stack
};
static int regcnts[16];
static int fpuregcnts[8];
static float_status fpustatus;

static floatx80 fpu_random(void)
{
	floatx80 v = int32_to_floatx80(rand32());
	for (int i = 0; i < 10; i++) {
		uae_u64 n = rand32() | (((uae_u64)rand16()) << 32);
		// don't create denormals yet
		if (!((v.low + n) & 0x8000000000000000)) {
			v.low |= 0x8000000000000000;
			continue;
		}
		v.low += n;
	}
	return v;
}

static bool fpuregchange(int reg, fpdata *regs)
{
	int regcnt = fpuregcnts[reg];
	floatx80 v = regs[reg].fpx;
	floatx80 add;

	// don't unnecessarily modify static forced register
	for (int i = 0; i < regdatacnt; i++) {
		struct regdata *rd = &regdatas[i];
		int mode = rd->type & CT_DATA_MASK;
		int size = rd->type & CT_SIZE_MASK;
		if (size == CT_SIZE_FPU && mode == CT_DREG + reg) {
			return false;
		}
	}

	for(;;) {

		switch(reg)
		{
		case 0: // positive
			add = floatx80_div(int32_to_floatx80(1), int32_to_floatx80(5 + regcnt), &fpustatus);
			v = floatx80_add(v, add, &fpustatus);
			break;
		case 1: // negative
			add = floatx80_div(int32_to_floatx80(1), int32_to_floatx80(6 + regcnt), &fpustatus);
			v = floatx80_sub(v, add, &fpustatus);
			break;
		case 2: // positive/negative zero
			v.high ^= 0x8000;
			break;
		case 3:
			// very large value, larger than fits in double
			if (fpu_max_precision) {
				v = fpu_random();
			} else {
				v = packFloatx80(1, 0x7000 + (rand16() & 0xfff), 0x8000000000000000 | (((uae_u64)rand32()) << 32));
				if (regcnt & 1) {
					v.high ^= 0x8000;
				}
			}
			break;
		case 4:
			// value that fits in double but does not fit in single
			{
				int exp;
				if (fpu_max_precision < 128) {
					exp = fpu_max_precision + rand8();
				} else {
					exp = 128 + rand8();
				}
				exp += 16384;
				v = packFloatx80(1, exp, 0x8000000000000000 | (((uae_u64)rand32()) << 32));
				if (regcnt & 1) {
					v.high ^= 0x8000;
				}
			}
			break;
		case 5:
		case 6:
			// random
			v = fpu_random();
			break;
		case 7: // +NaN, -Nan, +Inf, -Inf
			if (fpu_max_precision) {
				v = fpu_random();
			} else {
				if ((regcnt & 3) == 0) {
					v = floatx80_default_nan(NULL);
				} else if ((regcnt & 3) == 1) {
					v = floatx80_default_nan(NULL);
					v.high ^= 0x8000;
				} else if ((regcnt & 3) == 2) {
					v = packFloatx80(0, 0x7FFF, floatx80_default_infinity_low);
				} else if ((regcnt & 3) == 3) {
					v = packFloatx80(1, 0x7FFF, floatx80_default_infinity_low);
				}
			}
			break;
		}

		if (fpu_precision_valid(v)) {
			fpuregcnts[reg]++;
			regs[reg].fpx = v;
			return true;
		}
	}
}

static bool regchange(int reg, uae_u32 *regs)
{
	int regcnt = regcnts[reg];
	uae_u32 v = regs[reg];

	if (generate_address_mode && reg >= 8)
		return false;
	if (feature_loop_mode_register == reg)
		return false;
	if ((1 << reg) & ignore_register_mask)
		return false;

	// don't unnecessarily modify static forced register
	for (int i = 0; i < regdatacnt; i++) {
		struct regdata *rd = &regdatas[i];
		int mode = rd->type & CT_DATA_MASK;
		int size = rd->type & CT_SIZE_MASK;
		if (size != CT_SIZE_FPU && mode == CT_DREG + reg) {
			return false;
		}
	}

	switch (reg)
	{
	case 0:
		v += 0x12;
		break;
	case 1:
		return false;
	case 2:
		if (regcnt) {
			v ^= 1 << ((regcnt - 1) & 31);
		}
		v ^= 0x80008080;
		v ^= 1 << (regcnt & 31);
		break;
	case 3:
		if (regcnt) {
			v ^= 1 << ((regcnt - 1) & 31);
		}
		v = (v >> 2) | (v << 30);
		v ^= 1 << (regcnt & 31);
		break;
	case 4:
		if (regcnt) {
			v ^= 1 << ((regcnt - 1) & 31);
		}
		v &= 0x7fffff7f;
		v = (v >> 2) | (v << 30);
		v |= 0x80000080;
		v ^= 1 << (regcnt & 31);
		break;
	case 5:
		v ^= 0x80008080;
		v += 0x00010101;
		break;
	case 6:
		v = ((((v & 0xff) << 1) | ((v & 0xff) >> 7)) & 0xff) |
			((((v & 0xff00) << 1) | ((v & 0xff00) >> 7)) & 0xff00) |
			((((v & 0xffff0000) << 1) | ((v & 0xffff0000) >> 15)) & 0xffff0000);
		break;
	case 7:
		v = rand32();
		break;
	case 8:
		return false;
	case 9:
		v += 1;
		v &= 0xff;
		if (v < 0x70)
			v = 0x90;
		if (v >= 0x90) {
			v -= 0x90 - 0x70;
		}
		break;
	case 10:
		v += 0x13;
		v &= 0xffff;
		if (v < 0x7fe0)
			v = 0x8020;
		if (v >= 0x8020) {
			v -= 0x8020 - 0x7fe0;
		}
		break;
	case 11:
		if (feature_loop_mode_jit) {
			return false;
		}
		v ^= 0x8000;
		break;
	case 12:
		v ^= 0x80000000;
		v = (v & 0xffffff00) | ((v + 0x14) & 0xff);
		break;
	case 13:
		v = (v >> 2) | (v << 30);
		break;
	default:
		return false;
	}
	regcnts[reg]++;
	regs[reg] = v;
	return true;
}

static void fill_memory_buffer(uae_u8 *p, int size)
{
	uae_u8 *pend = p + size - 64;
	int i = 0;
	while (p < pend) {
		int x = (i & 3);
		if (x == 1) {
			floatx80 v = int32_to_floatx80(xorshift32());
			*p++ = v.high >> 8;
			*p++ = v.high >> 0;
			*p++ = 0;
			*p++ = 0;
			*p++ = (uae_u8)(v.low >> 56);
			*p++ = (uae_u8)(v.low >> 48);
			*p++ = (uae_u8)(v.low >> 40);
			*p++ = (uae_u8)(v.low >> 32);
			if ((i & 15) < 8) {
				*p++ = (uae_u8)(v.low >> 24);
				*p++ = (uae_u8)(v.low >> 16);
				*p++ = (uae_u8)(v.low >>  8);
				*p++ = (uae_u8)(v.low >>  0);
			} else {
				*p++ = frand8();
				*p++ = frand8();
				*p++ = frand8();
				*p++ = frand8();
			}
		} else if (x == 2) {
			floatx80 v = int32_to_floatx80(xorshift32());
			float64 v2 = floatx80_to_float64(v, &fpustatus);
			*p++ = (uae_u8)(v2 >> 56);
			*p++ = (uae_u8)(v2 >> 48);
			*p++ = (uae_u8)(v2 >> 40);
			*p++ = (uae_u8)(v2 >> 32);
			*p++ = (uae_u8)(v2 >> 24);
			*p++ = (uae_u8)(v2 >> 16);
			*p++ = (uae_u8)(v2 >>  8);
			*p++ = (uae_u8)(v2 >>  0);
		} else if (x == 3) {
			floatx80 v = int32_to_floatx80(xorshift32());
			float32 v2 = floatx80_to_float32(v, &fpustatus);
			*p++ = (uae_u8)(v2 >> 24);
			*p++ = (uae_u8)(v2 >> 16);
			*p++ = (uae_u8)(v2 >> 8);
			*p++ = (uae_u8)(v2 >> 0);
		}
		i++;
	}
}

static void fill_memory(void)
{
	if (low_memory_temp)
		fill_memory_buffer(low_memory_temp, low_memory_size);
	if (high_memory_temp)
		fill_memory_buffer(high_memory_temp, high_memory_size);
	fill_memory_buffer(test_memory_temp, test_memory_size);
	if (feature_loop_mode_jit) {
		memset(test_memory_temp, 0xff, LM_BUFFER);
	}
}

static void compressfile(TCHAR *path, int flags)
{
	if (feature_gzip & flags) {
		FILE *f = _tfopen(path, _T("rb"));
		fseek(f, 0, SEEK_END);
		int size = ftell(f);
		fseek(f, 0, SEEK_SET);
		uae_u8 *mem = (uae_u8 *)malloc(size);
		fread(mem, 1, size, f);
		fclose(f);
		_tunlink(path);
		if (_tcschr(path, '.')) {
			path[_tcslen(path) - 1] = 'z';
		} else {
			_tcscat(path, _T(".gz"));
		}
		_tunlink(path);
		f = _tfopen(path, _T("wb"));
		int fd = fileno(f);
		gzFile gz = gzdopen(dup(fd), "wb9");
		gzwrite(gz, mem, size);
		gzclose(gz);
		fclose(f);
		free(mem);
	} else {
		if (_tcschr(path, '.')) {
			path[_tcslen(path) - 1] = 'z';
		} else {
			_tcscat(path, _T(".gz"));
		}
		_tunlink(path);
	}
}

static void compressfiles(const TCHAR *dir)
{
	for (int i = 0; i < filecount; i++) {
		TCHAR path[1000];
		_stprintf(path, _T("%s/%04d.dat"), dir, i);
		compressfile(path, 1);
	}
}

static void mergefiles(const TCHAR *dir)
{
	int tsize = 0;
	int mergecount = 0;
	FILE *of = NULL;
	int oi = -1;
	unsigned char zero[4] = { 0, 0, 0, 0 };
	unsigned char head[4] = { 0xaf, 'M', 'R', 'G'};
	TCHAR opath[1000];
	for (int i = 0; i < filecount; i++) {
		TCHAR path[1000];
		_stprintf(path, _T("%s/%04d.dat"), dir, i);
		if (feature_gzip & 1) {
			if (_tcschr(path, '.')) {
				path[_tcslen(path) - 1] = 'z';
			} else {
				_tcscat(path, _T(".gz"));
			}
		}
		FILE *f = _tfopen(path, _T("rb"));
		fseek(f, 0, SEEK_END);
		int size = ftell(f);
		fseek(f, 0, SEEK_SET);
		if (tsize > 0 && max_file_size > 0 && tsize + size >= max_file_size * 1024) {
			fwrite(head, 1, 4, of);
			fwrite(zero, 1, 4, of);
			fclose(of);
			of = NULL;
			tsize = 0;
		}
		uae_u8 *mem = (uae_u8 *)malloc(size);
		fread(mem, 1, size, f);
		fclose(f);
		if (!of) {
			oi++;
			_stprintf(opath, _T("%s/%04d.dat"), dir, oi);
			if (feature_gzip & 1) {
				if (_tcschr(opath, '.')) {
					opath[_tcslen(opath) - 1] = 'z';
				} else {
					_tcscat(opath, _T(".gz"));
				}
			}
			of = _tfopen(opath, _T("wb"));
			if (!of) {
				wprintf(_T("Couldn't open '%s'\n"), opath);
				abort();
			}
			mergecount++;
		}
		if (oi != i) {
			_tunlink(path);
		}
		unsigned char b;
		fwrite(head, 1, 4, of);
		b = size >> 24;
		fwrite(&b, 1, 1, of);
		b = size >> 16;
		fwrite(&b, 1, 1, of);
		b = size >> 8;
		fwrite(&b, 1, 1, of);
		b = size >> 0;
		fwrite(&b, 1, 1, of);
		fwrite(mem, 1, size, of);
		tsize += size;
		free(mem);
	}
	if (of) {
		fwrite(zero, 1, 4, of);
		fclose(of);
	}
	wprintf(_T("Number of files after merge: %d -> %d\n"), filecount, mergecount);
}

static void save_memory(const TCHAR *path, const TCHAR *name, uae_u8 *p, int size)
{
	TCHAR fname[1000];
	_stprintf(fname, _T("%s%s"), path, name);
	FILE *f = _tfopen(fname, _T("wb"));
	if (!f) {
		wprintf(_T("Couldn't open '%s'\n"), fname);
		abort();
	}
	fwrite(p, 1, size, f);
	fclose(f);
	compressfile(fname, 2);
}

static void deletefile(const TCHAR *path, const TCHAR *name)
{
	TCHAR path2[1000];
	_tcscpy(path2, path);
	_tcscat(path2, name);
	_tcscat(path2, _T(".dat"));
	_tunlink(path2);
	_tcscpy(path2, path);
	_tcscat(path2, name);
	_tcscat(path2, _T(".daz"));
	_tunlink(path2);
}

static uae_u8 *modify_reg(uae_u8 *dst, struct regstruct *regs, uae_u8 type, uae_u32 *valp)
{
	int mode = type & CT_DATA_MASK;
	int size = type & CT_SIZE_MASK;
	uae_u32 val = valp[0];
	if (size != CT_SIZE_FPU && mode >= CT_DREG && mode < CT_DREG + 8) {
		if (regs->regs[mode - CT_DREG] == val)
			return dst;
		regs->regs[mode - CT_DREG] = val;
	} else if (size != CT_SIZE_FPU && mode >= CT_AREG && mode < CT_AREG + 8) {
		if (regs->regs[mode - CT_AREG + 8] == val)
			return dst;
		regs->regs[mode - CT_AREG + 8] = val;
	} else if (size == CT_SIZE_FPU && mode >= CT_DREG && mode < CT_DREG + 7) {
		fpdata *fpd = &regs->fp[mode - CT_DREG];
		uae_u64 v = ((uae_u64)valp[1] << 32) | valp[2];
		if (fpd->fpx.high == val && fpd->fpx.low == v)
			return dst;
		fpd->fpx.high = val;
		fpd->fpx.low = v;
	} else if (mode == CT_SR) {
		if (size == CT_SIZE_BYTE) {
			if ((regs->sr & 0xff) == (val & 0xff))
				return dst;
			regs->sr &= 0xff00;
			regs->sr |= val & 0xff;
		} else {
			if (regs->sr == val)
				return dst;
			regs->sr = val;
		}
	} else if (mode == CT_FPSR) {
		if (regs->fpsr == val)
			return dst;
		regs->fpsr = val;
	} else if (mode == CT_FPCR) {
		if (regs->fpcr == val)
			return dst;
		regs->fpcr = val;
	} else if (mode == CT_FPIAR) {
		if (regs->fpiar == val)
			return dst;
		regs->fpiar = val;
	}
	if (dst) {
		*dst++ = CT_OVERRIDE_REG;
		*dst++ = type;
		if (size == CT_SIZE_BYTE) {
			*dst = (uae_u8)val;
			dst += 1;
		} else if (size == CT_SIZE_WORD) {
			pw(dst, (uae_u16)val);
			dst += 2;
		} else if (size == CT_SIZE_LONG) {
			pl(dst, val);
			dst += 4;
		} else if (size == CT_SIZE_FPU) {
			pl(dst, val);
			dst += 4;
			pl(dst, valp[1]);
			dst += 4;
			pl(dst, valp[2]);
			dst += 4;
		}
	}
	return dst;
}

static uae_u8 *store_rel(uae_u8 *dst, uae_u8 mode, uae_u32 s, uae_u32 d, int ordered)
{
	int diff = (uae_s32)d - (uae_s32)s;
	if (diff == 0) {
		if (!ordered)
			return dst;
		*dst++ = CT_EMPTY;
		return dst;
	}
	if (diff >= -128 && diff < 128) {
		*dst++ = mode | CT_RELATIVE_START_BYTE;
		*dst++ = diff & 0xff;
	} else if (diff >= -32768 && diff < 32768) {
		*dst++ = mode | CT_RELATIVE_START_WORD;
		*dst++ = (diff >> 8) & 0xff;
		*dst++ = diff & 0xff;
	} else if (d < 0x8000) {
		*dst++ = mode | CT_ABSOLUTE_WORD;
		*dst++ = (d >> 8) & 0xff;
		*dst++ = d & 0xff;
	} else if ((d & ~addressing_mask) == ~addressing_mask && (d & addressing_mask) >= (HIGH_MEMORY_START & addressing_mask)) {
		*dst++ = mode | CT_ABSOLUTE_WORD;
		*dst++ = (d >> 8) & 0xff;
		*dst++ = d & 0xff;
	} else {
		*dst++ = mode | CT_ABSOLUTE_LONG;
		*dst++ = (d >> 24) & 0xff;
		*dst++ = (d >> 16) & 0xff;
		*dst++ = (d >> 8) & 0xff;
		*dst++ = d & 0xff;
	}
	return dst;
}

static uae_u8 *store_fpureg(uae_u8 *dst, uae_u8 mode, floatx80 *s, floatx80 d, int forced)
{
	if (forced || s == NULL) {
		*dst++ = mode | CT_SIZE_FPU;
		*dst++ = 0xff;
		*dst++ = (d.high >> 8) & 0xff;
		*dst++ = (d.high >> 0) & 0xff;
		*dst++ = (d.low >> 56) & 0xff;
		*dst++ = (d.low >> 48) & 0xff;
		*dst++ = (d.low >> 40) & 0xff;
		*dst++ = (d.low >> 32) & 0xff;
		*dst++ = (d.low >> 24) & 0xff;
		*dst++ = (d.low >> 16) & 0xff;
		*dst++ = (d.low >> 8) & 0xff;
		*dst++ = (d.low >> 0) & 0xff;
		return dst;
	}
	if (s->high == d.high && s->low == d.low) {
		return dst;
	}
	uae_u8 fs[10], fd[10];
	fs[0] = s->high >> 8;
	fs[1] = (uae_u8)s->high;
	fs[2] = (uae_u8)(s->low >> 56);
	fs[3] = (uae_u8)(s->low >> 48);
	fs[4] = (uae_u8)(s->low >> 40);
	fs[5] = (uae_u8)(s->low >> 32);
	fs[6] = (uae_u8)(s->low >> 24);
	fs[7] = (uae_u8)(s->low >> 16);
	fs[8] = (uae_u8)(s->low >> 8);
	fs[9] = (uae_u8)(s->low >> 0);
	fd[0] = d.high >> 8;
	fd[1] = (uae_u8)d.high;
	fd[2] = (uae_u8)(d.low >> 56);
	fd[3] = (uae_u8)(d.low >> 48);
	fd[4] = (uae_u8)(d.low >> 40);
	fd[5] = (uae_u8)(d.low >> 32);
	fd[6] = (uae_u8)(d.low >> 24);
	fd[7] = (uae_u8)(d.low >> 16);
	fd[8] = (uae_u8)(d.low >> 8);
	fd[9] = (uae_u8)(d.low >> 0);
	*dst++ = mode | CT_SIZE_FPU;
	if (fs[4] != fd[4] || fs[5] != fd[5]) {
		*dst++ = 0xff;
		*dst++ = (d.high >> 8) & 0xff;
		*dst++ = (d.high >> 0) & 0xff;
		*dst++ = (d.low >> 56) & 0xff;
		*dst++ = (d.low >> 48) & 0xff;
		*dst++ = (d.low >> 40) & 0xff;
		*dst++ = (d.low >> 32) & 0xff;
		*dst++ = (d.low >> 24) & 0xff;
		*dst++ = (d.low >> 16) & 0xff;
		*dst++ = (d.low >> 8) & 0xff;
		*dst++ = (d.low >> 0) & 0xff;
		return dst;
	}
	uae_u8 *flagp = dst;
	*flagp = 0;
	dst++;
	if ((fs[0] != fd[0] || fs[1] != fd[1] || fs[2] != fd[2] || fs[3] != fd[3]) && fs[4] == fd[4]) {
		uae_u8 cnt = 4;
		for (int i = 3; i >= 0; i--) {
			if (fs[i] != fd[i])
				break;
			cnt--;
		}
		if (cnt > 0) {
			*flagp |= cnt << 4;
			for (int i = 0; i < cnt; i++) {
				*dst++ = fd[i];
			}
		}
	}
	if ((fs[9] != fd[9] || fs[8] != fd[8] || fs[7] != fd[7] || fs[6] != fd[6]) && fs[5] == fd[5]) {
		uae_u8 cnt = 4;
		for (int i = 6; i <= 9; i++) {
			if (fs[i] != fd[i])
				break;
			cnt--;
		}
		if (cnt > 0) {
			*flagp |= cnt;
			for (int i = 0; i < cnt; i++) {
				*dst++ = fd[9 - i];
			}
		}
	}
	return dst;
}

static uae_u8 *store_reg(uae_u8 *dst, uae_u8 mode, uae_u32 s, uae_u32 d, int size)
{
	if (s == d && size == -1)
		return dst;
	if (((s & 0xffffff00) == (d & 0xffffff00) && size < 0) || size == sz_byte) {
		*dst++ = mode | CT_SIZE_BYTE;
		*dst++ = d & 0xff;
	} else if (((s & 0xffff0000) == (d & 0xffff0000) && size < 0) || size == sz_word) {
		*dst++ = mode | CT_SIZE_WORD;
		*dst++ = (d >> 8) & 0xff;
		*dst++ = d & 0xff;
	} else {
		*dst++ = mode | CT_SIZE_LONG;
		*dst++ = (d >> 24) & 0xff;
		*dst++ = (d >> 16) & 0xff;
		*dst++ = (d >> 8) & 0xff;
		*dst++ = d & 0xff;
	}
	return dst;
}

static uae_u8 *store_mem_bytes(uae_u8 *dst, uaecptr start, int len, uae_u8 *old)
{
	if (!len)
		return dst;
	if (len > 256) {
		wprintf(_T(" too long byte count!\n"));
		abort();
	}
	if (is_nowrite_address(start, 1) || is_nowrite_address(start + len - 1, 1)) {
		wprintf(_T(" store_mem_bytes accessing safe memory area! (%08x - %08x)\n"), start, start + len - 1);
		abort();
	}

	uaecptr oldstart = start;
	uae_u8 offset = 0;
	// start
	if (old) {
		for (int i = 0; i < len; i++) {
			uae_u8 v = get_byte_test(start);
			if (v != *old)
				break;
			start++;
			old++;
		}
	}
	// end
	offset = start - oldstart;
	len -= offset;
	if (old) {
		for (int i = len - 1; i >= 0; i--) {
			uae_u8 v = get_byte_test(start + i);
			if (v != old[i])
				break;
			len--;
		}
	}
	if (!len)
		return dst;
	*dst++ = CT_MEMWRITES | CT_PC_BYTES;
	if ((len > 32 || offset > 7) || (len == 31 && offset == 7)) {
		*dst++ = 0xff;
		*dst++ = offset;
		*dst++ = len;
	} else {
		*dst++ = (offset << 5) | (uae_u8)(len == 32 ? 0 : len);
	}
	for (int i = 0; i < len; i++) {
		*dst++ = get_byte_test(start);
	}
	return dst;
}

static uae_u8 *store_mem_writes(uae_u8 *dst, int storealways)
{
	if (ahcnt_current == ahcnt_written)
		return dst;
	for (int i = ahcnt_current - 1; i >= ahcnt_written; i--) {
		struct accesshistory *ah = &ahist[i];
		if (ah->oldval == ah->val && !storealways)
 			continue;
		if (ah->donotsave)
			continue;
		validate_addr(ah->addr, 1 << ah->size);
		uaecptr addr = ah->addr;
		addr &= addressing_mask;
		if (is_nowrite_address(addr, 1)) {
			wprintf(_T("attempting to save safe memory address %08x!\n"), addr);
			abort();
		}	
		if (addr < 0x8000) {
			*dst++ = CT_MEMWRITE | CT_ABSOLUTE_WORD;
			*dst++ = (addr >> 8) & 0xff;
			*dst++ = addr & 0xff;
		} else if (addr >= HIGH_MEMORY_START) {
			*dst++ = CT_MEMWRITE | CT_ABSOLUTE_WORD;
			*dst++ = (addr >> 8) & 0xff;
			*dst++ = addr & 0xff;
		} else if (addr >= opcode_memory_start && addr < opcode_memory_start + 32768) {
			*dst++ = CT_MEMWRITE | CT_RELATIVE_START_WORD;
			uae_u16 diff = addr - opcode_memory_start;
			*dst++ = (diff >> 8) & 0xff;
			*dst++ = diff & 0xff;
		} else if (addr < opcode_memory_start && addr >= opcode_memory_start - 32768) {
			*dst++ = CT_MEMWRITE | CT_RELATIVE_START_WORD;
			uae_u16 diff = addr - opcode_memory_start;
			*dst++ = (diff >> 8) & 0xff;
			*dst++ = diff & 0xff;
		} else {
			*dst++ = CT_MEMWRITE | CT_ABSOLUTE_LONG;
			*dst++ = (addr >> 24) & 0xff;
			*dst++ = (addr >> 16) & 0xff;
			*dst++ = (addr >> 8) & 0xff;
			*dst++ = addr & 0xff;
		}
		dst = store_reg(dst, CT_MEMWRITE, 0, ah->oldval, ah->size);
		dst = store_reg(dst, CT_MEMWRITE, 0, ah->val, ah->size);
	}
	ahcnt_written = ahcnt_current;
	return dst;
}

static bool load_file(const TCHAR *path, const TCHAR *file, uae_u8 *p, int size, int offset)
{
	TCHAR fname[1000];
	if (path) {
		_stprintf(fname, _T("%s%s"), path, file);
	} else {
		_tcscpy(fname, file);
	}
	FILE *f = _tfopen(fname, _T("rb"));
	if (!f)
		return false;
	if (offset < 0) {
		fseek(f, -size, SEEK_END);
	} else {
		fseek(f, offset, SEEK_SET);
	}
	int lsize = fread(p, 1, size, f);
	if (lsize != size) {
		wprintf(_T("Couldn't read file '%s'\n"), fname);
		exit(0);
	}
	fclose(f);
	return true;
}

static void markfile(const TCHAR *dir)
{
	TCHAR path[1000];
	if (filecount <= 1)
		return;
	_stprintf(path, _T("%s/%04d.dat"), dir, filecount - 1);
	FILE *f = _tfopen(path, _T("r+b"));
	if (f) {
		fseek(f, -1, SEEK_END);
		uae_u8 b = CT_END_FINISH;
		fwrite(&b, 1, 1, f);
		fclose(f);
	}
}

static void save_data(uae_u8 *dst, const TCHAR *dir, int size)
{
	TCHAR path[1000];

	if (dst == storage_buffer)
		return;

	if (dst - storage_buffer > max_storage_buffer + storage_buffer_watermark_size) {
		wprintf(_T("data buffer overrun!\n"));
		abort();
	}

	_wmkdir(dir);
	_stprintf(path, _T("%s/%04d.dat"), dir, filecount);
	FILE *f = _tfopen(path, _T("wb"));
	if (!f) {
		wprintf(_T("couldn't open '%s'\n"), path);
		abort();
	}
	wprintf(_T("%s\n"), path);
	if (filecount == 0) {
		uae_u8 data[4];
		pl(data, DATA_VERSION);
		fwrite(data, 1, 4, f);
		pl(data, (uae_u32)starttime);
		fwrite(data, 1, 4, f);
		pl(data, ((hmem_rom | (test_high_memory_start == 0xffffffff ? 0x8000 : 0x0000)) << 16) |
			(lmem_rom | (test_low_memory_start == 0xffffffff ? 0x8000 : 0x0000)));
		fwrite(data, 1, 4, f);
		pl(data, test_memory_start);
		fwrite(data, 1, 4, f);
		pl(data, test_memory_size);
		fwrite(data, 1, 4, f);
		pl(data, opcode_memory_start);
		fwrite(data, 1, 4, f);
		pl(data, (cpu_lvl << 16) | sr_undefined_mask | (addressing_mask == 0xffffffff ? 0x80000000 : 0) |
			(feature_min_interrupt_mask << 20) | (safe_memory_mode << 23) | (feature_interrupts << 26) |
			((feature_loop_mode_jit ? 1 : 0) << 28) | ((feature_loop_mode_68010 ? 1 : 0) << 29));
		fwrite(data, 1, 4, f);
		pl(data, (feature_initial_interrupt_mask & 7) | ((feature_initial_interrupt & 7) << 3) | (fpu_max_precision << 6));
		fwrite(data, 1, 4, f);
		pl(data, 0);
		fwrite(data, 1, 4, f);
		pl(data, 0);
		fwrite(data, 1, 4, f);
		pl(data, currprefs.fpu_model);
		fwrite(data, 1, 4, f);
		pl(data, test_low_memory_start);
		fwrite(data, 1, 4, f);
		pl(data, test_low_memory_end);
		fwrite(data, 1, 4, f);
		pl(data, test_high_memory_start);
		fwrite(data, 1, 4, f);
		pl(data, test_high_memory_end);
		fwrite(data, 1, 4, f);
		pl(data, safe_memory_start);
		fwrite(data, 1, 4, f);
		pl(data, safe_memory_end);
		fwrite(data, 1, 4, f);
		pl(data, user_stack_memory_use);
		fwrite(data, 1, 4, f);
		pl(data, super_stack_memory);
		fwrite(data, 1, 4, f);
		pl(data, feature_exception_vectors);
		fwrite(data, 1, 4, f);
		data[0] = data[1] = data[2] = 0;
		pl(data, feature_loop_mode_cnt | (feature_loop_mode_jit << 8));
		fwrite(&data[0], 1, 4, f);
		fwrite(&data[1], 1, 4, f);
		fwrite(&data[2], 1, 4, f);
		fwrite(inst_name, 1, sizeof(inst_name) - 1, f);
		data[0] = CT_END_FINISH;
		data[1] = 0;
		fwrite(data, 1, 2, f);
		fclose(f);
		filecount++;
		save_data(dst, dir, size);
	} else {
		uae_u8 data[4];
		pl(data, DATA_VERSION);
		fwrite(data, 1, 4, f);
		pl(data, (uae_u32)starttime);
		fwrite(data, 1, 4, f);
		pl(data, (size & 3));
		fwrite(data, 1, 4, f);
		pl(data, 0);
		fwrite(data, 1, 4, f);
		*dst++ = CT_END_FINISH;
		*dst++ = filecount;
		fwrite(storage_buffer, 1, dst - storage_buffer, f);
		fclose(f);
		filecount++;
	}
}

static int full_format_cnt;
static int fpu_imm_cnt;

static const int bytesizes[] = { 4, 4, 12, 12, 2, 8, 1, 12 };
static int fpuopsize;

static int putfpuimm(uaecptr addr, int opcodesize, int *isconstant)
{
	uaecptr start = addr;

	switch (opcodesize)
	{
	case 0: // L
		put_long_test(addr, rand32());
		addr += 4;
		break;
	case 4: // W
		put_word_test(addr, rand16());
		addr += 2;
		break;
	case 6: // B
		put_byte_test(addr, rand8());
		addr += 1;
		break;
	case 1: // S
	{
		floatx80 v;
		if (fpu_imm_cnt & 1) {
			v = int32_to_floatx80(rand32());
		} else {
			v = fpuregisters[(fpu_imm_cnt >> 1) & 7];
		}
		fpu_imm_cnt++;
		float32 v2 = floatx80_to_float32(v, &fpustatus);
		put_long_test(addr, v2);
		addr += 4;
		break;
	}
	case 2: // X
	{
		floatx80 v;
		if (fpu_imm_cnt & 1) {
			v = int32_to_floatx80(rand32());
		} else {
			v = fpuregisters[(fpu_imm_cnt >> 1) & 7];
		}
		fpu_imm_cnt++;
		put_long_test(addr + 0, v.high);
		put_long_test(addr + 4, v.low >> 32);
		put_long_test(addr + 8, (uae_u32)v.low);
		addr += 12;
		break;
	}
	case 3: // P
	{
		fpdata fpd = { 0 };
		if (fpu_imm_cnt & 1) {
			fpd.fpx = int32_to_floatx80(rand32());
		} else {
			fpd.fpx = fpuregisters[(fpu_imm_cnt >> 1) & 7];
		}
		fpu_imm_cnt++;
		uae_u32 out[3];
		fpp_from_pack(&fpd, out, (rand8() & 63) - 31);
		put_long_test(addr + 0, out[0]);
		put_long_test(addr + 4, out[1]);
		put_long_test(addr + 8, out[2]);
		addr += 12;
		break;
	}
	case 5: // D
	{
		floatx80 v;
		if (fpu_imm_cnt & 1) {
			v = int32_to_floatx80(rand32());
		} else {
			v = fpuregisters[(fpu_imm_cnt >> 1) & 7];
		}
		fpu_imm_cnt++;
		float64 v2 = floatx80_to_float64(v, &fpustatus);
		put_long_test(addr + 0, v2 >> 32);
		put_long_test(addr + 4, (uae_u32)v2);
		addr += 8;
		break;
	}
	}
	if (out_of_test_space) {
		wprintf(_T(" putfpuimm out of bounds access!?"));
		abort();
	}
	return addr - start;
}

static int ea_state_found[3];

static void reset_ea_state(void)
{
	ea_state_found[0] = 0;
	ea_state_found[1] = 0;
	ea_state_found[2] = 0;
}

// if other EA is target EA, don't point this EA
// to same address space.
static bool other_targetea_same(int srcdst, uae_u32 v)
{
	if (target_ea[srcdst ^ 1] == 0xffffffff)
		return false;
	if (!is_nowrite_address(target_ea[srcdst ^ 1], 1))
		return false;
	if (!is_nowrite_address(v, 1))
		return false;
	return true;
}

// attempt to find at least one zero, positive and negative source value
static int analyze_address(struct instr *dp, int srcdst, uae_u32 addr)
{
	uae_u32 v;
	uae_u32 mask;

	if (srcdst)
		return 1;
	if (dp->size == sz_byte) {
		v = get_byte_test(addr);
		mask = 0x80;
	} else if (dp->size == sz_word) {
		v = get_word_test(addr);
		mask = 0x8000;
	} else {
		v = get_long_test(addr);
		mask = 0x80000000;
	}
	if (out_of_test_space) {
		out_of_test_space = false;
		return 0;
	}
	if (ea_state_found[0] >= 2 && ea_state_found[1] >= 2 && ea_state_found[2] >= 2) {
		ea_state_found[0] = ea_state_found[1] = ea_state_found[2] = 0;
	}
	// zero
	if (v == 0) {
		if (ea_state_found[0] >= 2 && (ea_state_found[1] < 2 || ea_state_found[2] < 2))
			return 0;
		ea_state_found[0]++;
	}
	// negative value
	if (v & mask) {
		if (ea_state_found[1] >= 2 && (ea_state_found[0] < 2 || ea_state_found[2] < 2))
			return 0;
		ea_state_found[1]++;
	}
	// positive value
	if (v < mask && v > 0) {
		if (ea_state_found[2] >= 2 && (ea_state_found[0] < 2 || ea_state_found[1] < 2))
			return 0;
		ea_state_found[2]++;
	}
	return 1;
}

static int generate_fpu_memory_read(uae_u16 opcode, uaecptr pc, struct instr *dp, int *isconstant, uae_u32 *eap, int *regused);

// generate mostly random EA.
static int create_ea_random(uae_u16 *opcodep, uaecptr pc, int mode, int reg, struct instr *dp, int *isconstant, int srcdst, int fpuopcode, int opcodesize, uae_u32 *eap, int *regused, int *fpuregused, int *flagsp)
{
	uaecptr old_pc = pc;
	uae_u16 opcode = *opcodep;
	bool loopmodelimit = false;

	// feature_full_extension_format = 2 and 68020 addressing modes enabled: do not generate any normal addressing modes
	if (feature_full_extension_format == 2 && (ad8r[srcdst] || pc8r[srcdst]) && (mode != Ad8r && mode != PC8r))
		return -1;

	if (feature_loop_mode_jit) {
		// loop mode JMP or JSR: skip if A3 or A7 is used in EA calculation
		if (dp->mnemo == i_JMP || dp->mnemo == i_JSR) {
			if (mode == Areg || mode == Aind || mode == Aipi || mode == Apdi || mode == Ad16 || mode == Ad8r) {
				if (reg == 3 || reg == 7) {
					return -1;
				}
				loopmodelimit = true;
			}
		}
	}

	switch (mode)
	{
	case Dreg:
		for (;;) {
			if (reg == feature_loop_mode_register) {
				if (((dp->sduse & 0x20) && !srcdst) || ((dp->sduse & 0x02) && srcdst)) {
					reg = (reg + 1) & 7;
					int pos = srcdst ? dp->dpos : dp->spos;
					opcode &= ~(7 << pos);
					opcode |= reg << pos;
				} else {
					break;
				}
			} else if (((1 << reg) & ignore_register_mask)) {
				return -1;
			} else {
				break;
			}
		}
		*regused = reg;
		break;
	case Areg:
		*regused = reg + 8;
		break;
	case Aind:
	case Aipi:
		*regused = reg + 8;
		*eap = cur_regs.regs[reg + 8];
		break;
	case Apdi:
		*regused = reg + 8;
		if (fpuopsize < 0) {
			*eap = cur_regs.regs[reg + 8] - (1 << dp->size);
		} else {
			*eap = cur_regs.regs[reg + 8] - bytesizes[fpuopsize];
		}
		break;
	case Ad16:
	{
		uae_u16 v;
		uae_u32 addr;
		int maxcnt = 1000;
		for (;;) {
			v = rand16();
			addr = cur_regs.regs[reg + 8] + (uae_s16)v;
			*regused = reg + 8;
			if (fpuopsize >= 0) {
				if (check_valid_addr(addr, bytesizes[fpuopsize], 2))
					break;
			} else {
				if (analyze_address(dp, srcdst, addr))
					break;
			}
			maxcnt--;
			if (maxcnt < 0)
				break;
		}
		put_word_test(pc, v);
		*isconstant = 16;
		pc += 2;
		*eap = addr;
		break;
	}
	case PC16:
	{
		uae_u32 pct = pc + 2 - 2;
		uae_u16 v;
		uae_u32 addr;
		int maxcnt = 1000;
		for (;;) {
			v = rand16();
			addr = pct + (uae_s16)v;
			if (fpuopsize >= 0) {
				if (check_valid_addr(addr, bytesizes[fpuopsize], 2))
					break;
			} else {
				if (analyze_address(dp, srcdst, addr))
					break;
			}
			maxcnt--;
			if (maxcnt < 0)
				break;
		}
		put_word_test(pc, v);
		*isconstant = 16;
		pc += 2;
		*eap = 1;
		break;
	}
	case Ad8r:
	case PC8r:
	{
		uae_u32 addr;
		uae_u16 v = rand16() & 0x0100;
		if (!feature_full_extension_format)
			v &= ~0x100;
		if (mode == Ad8r) {
			if ((ad8r[srcdst] & 3) == 1)
				v &= ~0x100;
			if ((ad8r[srcdst] & 3) == 2)
				v |= 0x100;
		} else if (mode == PC8r) {
			if ((pc8r[srcdst] & 3) == 1)
				v &= ~0x100;
			if ((pc8r[srcdst] & 3) == 2)
				v |= 0x100;
		}
		if (currprefs.cpu_model < 68020 || (v & 0x100) == 0) {
			// brief format extension
			uae_u32 add = 0;
			int maxcnt = 1000;
			for (;;) {
				int ereg;
				if (mode == PC8r) {
					addr = pc + 2 - 2;
				} else {
					addr = cur_regs.regs[reg + 8];
					if (reg == 7 && flagsp) {
						*flagsp |= EAFLAG_SP;
					}
				}
				for (;;) {
					v = rand16();
					if (currprefs.cpu_model >= 68020)
						v &= ~0x100;
					ereg = v >> 12;
					if (loopmodelimit && (ereg == 0 || ereg == 8 + 3 || ereg == 8 + 7)) {
						continue;
					}
					if ((1 << ereg) & ignore_register_mask) {
						continue;
					}
					break;
				}
				*regused = ereg;
				add = cur_regs.regs[ereg];
				if (currprefs.cpu_model >= 68020) {
					add <<= (v >> 9) & 3; // SCALE
				}
				if (v & 0x0800) {
					// L
					addr += add;
				} else {
					// W
					addr += (uae_s16)add;
				}
				addr += (uae_s8)(v & 0xff); // DISPLACEMENT
				if (fpuopsize >= 0) {
					if (check_valid_addr(addr, bytesizes[fpuopsize], 2))
						break;
				} else {
					if (other_targetea_same(srcdst, addr))
						continue;
					if (analyze_address(dp, srcdst, addr))
						break;
				}
				maxcnt--;
				if (maxcnt < 0)
					break;
			}
			*isconstant = 16;
			put_word_test(pc, v);
			pc += 2;
			*eap = addr;
		} else {
			// full format extension
			// attempt to generate every possible combination,
			// one by one.
			v |= rand16() & ~0x100;
			for (;;) {
				v &= 0xff00;
				v |= full_format_cnt & 0xff;
				// skip reserved combinations for now
				int is = (v >> 6) & 1;
				int iis = v & 7;
				int sz = (v >> 4) & 3;
				int bit3 = (v >> 3) & 1;
				if ((is && iis >= 4) || iis == 4 || sz == 0 || bit3 == 1) {
					full_format_cnt++;
					continue;
				}
				break;
			}
			int ereg = v >> 12;
			if (loopmodelimit && (ereg == 0 || ereg == 8 + 3 || ereg == 8 + 7)) {
				return -1;
			}
			if ((1 << ereg) & ignore_register_mask) {
				return -1;
			}
			*regused = ereg;
			put_word_test(pc, v);
			uaecptr pce = pc;
			pc += 2;
			// calculate length of extension
			if (mode == Ad8r && reg == 7 && flagsp) {
				*flagsp |= EAFLAG_SP;
			}
			*eap = ShowEA_disp(&pce, mode == Ad8r ? regs.regs[reg + 8] : pce, NULL, NULL, false);
			while (pc < pce) {
				v = rand16();
				put_word_test(pc, v);
				pc += 2;
			}
			*isconstant = 128;
		}
		break;
	}
	case absw:
	{
		uae_u32 v;
		if (!high_memory && !low_memory)
			return -1;
		test_absw = 1;
		for (;;) {
			v = rand16();
			if (immabsw_cnt & 1)
				v |= 0x8000;
			else
				v &= ~0x8000;
			immabsw_cnt++;
			if (v >= 0x8000)
				v |= 0xffff0000;
			if (fpuopsize >= 0) {
				if (check_valid_addr((uae_s32)(uae_s16)v, bytesizes[fpuopsize], 2))
					break;
			} else {
				if (other_targetea_same(srcdst, (uae_s32)(uae_s16)v))
					continue;
				if (analyze_address(dp, srcdst, v))
					break;
			}
		}
		put_word_test(pc, v);
		*isconstant = 16;
		pc += 2;
		*eap = v >= 0x8000 ? (0xffff0000 | v) : v;
		break;
	}
	case absl:
	{
		uae_u32 v;
		for (;;) {
			v = rand32();
			if ((immabsl_cnt & 7) == 0) {
				v &= 0x00007fff;
			} else if ((immabsl_cnt & 7) == 1) {
				v &= 0x00007fff;
				v = 0xffff8000 | v;
			} else if ((immabsl_cnt & 7) >= 5) {
				int offset = 0;
				for (;;) {
					offset = (uae_s16)rand16();
					if (offset < -OPCODE_AREA || offset > OPCODE_AREA)
						break;
				}
				v = opcode_memory_start + offset;
			}
			immabsl_cnt++;
			if (fpuopsize >= 0) {
				if (check_valid_addr(v, bytesizes[fpuopsize], 2))
					break;
			} else {
				if (other_targetea_same(srcdst, v))
					continue;
				if (analyze_address(dp, srcdst, v))
					break;
			}
		}
		put_long_test(pc, v);
		*isconstant = 32;
		pc += 4;
		*eap = v;
		break;
	}
	case imm:
		if (fpuopsize >= 0) {
			pc += putfpuimm(pc, fpuopsize, isconstant);
		} else {
			if (dp->size == sz_long) {
				put_long_test(pc, rand32());
				*isconstant = 32;
				pc += 4;
			} else {
				put_word_test(pc, rand16());
				*isconstant = 16;
				pc += 2;
			}
		}
		break;
	case imm0:
	{
		// byte immediate but randomly fill also upper byte
		uae_u16 v = rand16();
		if ((imm8_cnt & 15) == 0) {
			v = 0;
		} else if ((imm8_cnt & 3) == 0) {
			v &= 0xff;
		}
		imm8_cnt++;
		put_word_test(pc, v);
		*isconstant = 16;
		pc += 2;
		break;
	}
	case imm1:
	{
		if (dp->mnemo == i_FBcc || dp->mnemo == i_FTRAPcc || dp->mnemo == i_FScc || dp->mnemo == i_FDBcc) {
			break;
		}
		if (fpuopcode >= 0) {
			int extra = 0;
			uae_u16 v = 0;
			if (opcodesize == 7 && fpuopcode == 0) {
				// FMOVECR (which is technically non-existing FMOVE.P
				// with Packed-Decimal Real with Dynamic k-Factor.
				v = 0x4000;
				v |= opcodesize << 10;
				v |= imm16_cnt & 0x3ff;
				imm16_cnt++;
				if ((opcode & 0x3f) != 0)
					return -1;
				*isconstant = 0;
				if (imm16_cnt < 0x400)
					*isconstant = -1;
			} else if (opcodesize == 8) {
				// FMOVEM/FMOVE to/from (control) registers
				v |= 0x8000;
				v |= (imm16_cnt & 1) ? 0x2000 : 0x0000; // DR
				// keep zero bits zero because at least 68040 can hang if wrong bits are set
				// (at least set bit 10 causes hang if Control registers FMOVEM)
				if (imm16_cnt & 2) {
					// Control registers
					int list = (imm16_cnt >> 2) & 7;
					v |= list << 10; // REGISTER LIST
				} else {
					// FPU registers
					v |= 0x4000;
					uae_u16 mode = (imm16_cnt >> 3) & 3; // MODE
					v |= mode << 11;
					v |= rand8();
				}
				imm16_cnt++;
				*isconstant = 128;
			} else {
				v |= fpuopcode;
				imm16_cnt++;
				if ((imm16_cnt & 3) == 0) {
					// FPUOP EA,FPx
					v |= 0x4000;
					v |= opcodesize << 10;
					fpuopsize = (v >> 10) & 7;
					// generate source addressing mode
					extra = generate_fpu_memory_read(opcode, pc + 2, dp, isconstant, eap, regused);
					if (extra < 0)
						return -2;
					v |= ((imm16_cnt >> 2) & 7) << 7;
					*fpuregused = (v >> 7) & 7;
					*isconstant = 8 * 2;
				} else if ((imm16_cnt & 3) == 1) {
					// FPUOP FPx,FPx
					v |= ((imm16_cnt >> 2) & 7) << 10;
					v |= ((imm16_cnt >> 5) & 7) << 7;
					if (opcode & 0x3f) {
						// mode/reg field non-zero: we already processed same FPU instruction
						return -2;
					}
					if (opcodesize != 2) {
						// not X: skip
						// No need to generate multiple identical tests
						return -2;
					}
					*fpuregused = (v >> 10) & 7;
					*isconstant = 8 * 8 * 2;
				} else if ((imm16_cnt & 3) == 2) {
					if (fpuopcode != 0)
						return -2;
					// FMOVE FPx,EA
					v |= 0x2000 | 0x4000;
					v |= opcodesize << 10;
					int srcspec = (v >> 10) & 7;
					v |= ((imm16_cnt >> 2) & 7) << 7;
					*fpuregused = (v >> 7) & 7;
					*isconstant = 8 * 2;
				} else {
					return -2;
				}
			}
			put_word_test(pc, v);
			pc += 2;
			pc += extra;
		} else {
			if (opcodecnt == 1) {
				// STOP #xxxx: test all combinations
				if (dp->mnemo == i_LPSTOP) {
					uae_u16 lp = 0x01c0;
					if (imm16_cnt & (0x40 | 0x80)) {
						for (;;) {
							lp = rand16();
							if (lp != 0x01c0)
								break;
						}
					}
					put_word_test(pc, lp);
					put_word_test(pc + 2, imm16_cnt++);
					pc += 2;
					if (imm16_cnt == 0)
						*isconstant = 0;
					else
						*isconstant = -1;
				} else if (dp->mnemo == i_MOVE2C || dp->mnemo == i_MOVEC2) {
					uae_u16 c = (imm16_cnt >> 1) & 0xfff;
					opcode = 0x4e7a;
					put_word_test(pc, 0x1000 | c); // movec rc,d1
					pc += 2;
					uae_u32 val = (imm16_cnt & 1) ? 0xffffffff : 0x00000000;
					switch (c)
					{
					case 0x003: // TC
					case 0x004: // ITT0
					case 0x005: // ITT1
					case 0x006: // DTT0
					case 0x007: // DTT1
						val &= ~0x8000;
						break;
					case 0x808: // PCR
						val &= ~(0x80 | 0x40 | 0x20 | 0x10 | 0x08 | 0x04);
						break;
					}
					put_word_test(pc, 0x203c); // move.l #x,d0
					pc += 2;
					put_long_test(pc, val);
					pc += 4;
					put_long_test(pc, 0x4e7b0000 | c); // movec d0,rc
					pc += 4;
					put_long_test(pc, 0x4e7a2000 | c); // movec rc,d2
					pc += 4;
					put_long_test(pc, 0x4e7b1000 | c); // movec d1,rc
					pc += 4;
					put_word_test(pc, 0x7200); // moveq #0,d0
					pc += 2;
					imm16_cnt++;
					multi_mode = 1;
					pc -= 2;
					if (imm16_cnt == 0x1000)
						*isconstant = 0;
					else
						*isconstant = -1;
				} else if (dp->mnemo == i_BSR || dp->mnemo == i_DBcc || dp->mnemo == i_Bcc ||
					dp->mnemo == i_ORSR || dp->mnemo == i_ANDSR || dp->mnemo == i_EORSR ||
					dp->mnemo == i_RTD) {
					// don't try to test all 65536 possibilities..
					uae_u16 i16 = imm16_cnt;
					put_word_test(pc, imm16_cnt);
					imm16_cnt += rand16() & 127;
					if (i16 > imm16_cnt)
						*isconstant = 0;
					else
						*isconstant = -1;
				} else {
					if (dp->mnemo == i_STOP) {
						put_word_test(pc, imm16_cnt | 0x2000);
					} else {
						put_word_test(pc, imm16_cnt);
					}
					if (dp->mnemo == i_STOP && feature_interrupts > 0) {
						// STOP hack to keep STOP test size smaller.
						if (feature_interrupts > 0) {
							imm16_cnt += 0x0080;
						} else {
							imm16_cnt += 0x0001;
						}
					} else {
						imm16_cnt += 0x0001;
					}
					if (imm16_cnt == 0)
						*isconstant = 0; 
					else
						*isconstant = -1;
				}
			} else {
				uae_u16 v = rand16();
				if ((imm16_cnt & 7) == 0) {
					v &= 0x00ff;
				} else if ((imm16_cnt & 15) == 0) {
					v = 0;
				}
				// clear A7 from register mask
				if (dp->mnemo == i_MVMEL) {
					v &= ~0x8000;
				} else if (dp->mnemo == i_MVMLE) {
					v &= ~0x0001;
				}
				imm16_cnt++;
				put_word_test(pc, v);
				*isconstant = 16;
			}
			pc += 2;
		}
		break;
	}
	case imm2:
	{
		if (dp->mnemo == i_FBcc || dp->mnemo == i_FTRAPcc) {
			break;
		}
		// long immediate
		uae_u32 v = rand32();
		if ((imm32_cnt & 63) == 0) {
			v = 0;
		} else if ((imm32_cnt & 7) == 0) {
			v &= 0x0000ffff;
		}
		imm32_cnt++;
		put_long_test(pc, v);
		if (imm32_cnt < 256)
			*isconstant = -1;
		else
			*isconstant = 32;
		pc += 4;
		break;
	}
	}
	*opcodep = opcode;
	return pc - old_pc;
}

static int ea_exact_cnt;

// generate exact EA (for bus error test)
static int create_ea_exact(uae_u16 *opcodep, uaecptr pc, int mode, int reg, struct instr *dp, int *isconstant, int srcdst, int fpuopcode, int opcodesize, uae_u32 *eap, int *regused, int *fpuregused, int *flagsp)
{
	uae_u32 target = target_ea[srcdst];
	ea_exact_cnt++;

	switch (mode)
	{
	// always allow modes that don't have EA
	case Dreg:
		*regused = reg;
		return 0;
	case Areg:
		*regused = reg + 8;
		return 0;
	case Aind:
	case Aipi:
	{
		if (cur_regs.regs[reg + 8] == target) {
			*eap = target;
			*regused = reg + 8;
			return  0;
		}
		return -2;
	}
	case Apdi:
	{
		if (cur_regs.regs[reg + 8] == target + (1 << dp->size)) {
			*eap = target;
			*regused = reg + 8;
			return  0;
		}
		return -2;
	}
	case Ad16:
	{
		uae_u32 v = cur_regs.regs[reg + 8];
		if (target <= v + 0x7ffe && (target >= v - 0x8000 || v < 0x8000)) {
			put_word_test(pc, target - v);
			*eap = target;
			*regused = reg + 8;
			return 2;
		}
		return -2;
	}
	case PC16:
	{
		uae_u32 pct = pc + 2 - 2;
		if (target <= pct + 0x7ffe && target >= pct - 0x8000) {
			put_word_test(pc, target - pct);
			*eap = target;
			return 2;
		}
		return -2;
	}
	case Ad8r:
	{
		for (int r = 0; r < 16; r++) {
			uae_u32 aval = cur_regs.regs[reg + 8];
			int rn = ((ea_exact_cnt >> 1) + r) & 15;
			for (int i = 0; i < 2; i++) {
				if ((ea_exact_cnt & 1) == 0 || i == 1) {
					uae_s32 val32 = cur_regs.regs[rn];
					uae_u32 addr = aval + val32;
					if (target <= addr + 0x7f && target >= addr - 0x80) {
						put_word_test(pc, (rn << 12) | 0x0800 | ((target - addr) & 0xff));
						*eap = target;
						*regused = rn;
						return 2;
					}
				} else {
					uae_s16 val16 = (uae_s16)cur_regs.regs[rn];
					uae_u32 addr = aval + val16;
					if (target <= addr + 0x7f && target >= addr - 0x80) {
						put_word_test(pc, (rn << 12) | 0x0000 | ((target - addr) & 0xff));
						*eap = target;
						*regused = rn;
						return 2;
					}
				}
			}
		}
		return -2;
	}
	case PC8r:
	{
		for (int r = 0; r < 16; r++) {
			uae_u32 aval = pc + 2 - 2;
			int rn = ((ea_exact_cnt >> 1) + r) & 15;
			for (int i = 0; i < 2; i++) {
				if ((ea_exact_cnt & 1) == 0 || i == 1) {
					uae_s32 val32 = cur_regs.regs[rn];
					uae_u32 addr = aval + val32;
					if (target <= addr + 0x7f && target >= addr - 0x80) {
						put_word_test(pc, (rn << 12) | 0x0800 | ((target - addr) & 0xff));
						*eap = target;
						*regused = rn;
						return 2;
					}
				} else {
					uae_s16 val16 = (uae_s16)cur_regs.regs[rn];
					uae_u32 addr = aval + val16;
					if (target <= addr + 0x7f && target >= addr - 0x80) {
						put_word_test(pc, (rn << 12) | 0x0000 | ((target - addr) & 0xff));
						*eap = target;
						*regused = rn;
						return 2;
					}
				}
			}
		}
		return -2;
	}
	case absw:
	{
		if ((target >= test_low_memory_start && target < test_low_memory_end) || (target >= test_high_memory_start && target < test_high_memory_end)) {
			put_word_test(pc, target);
			*eap = target;
			test_absw = 1;
			return 2;
		}
		return -2;
	}
	case absl:
	{
		if (target >= test_low_memory_start && target < test_low_memory_end) {
			put_long_test(pc, target);
			*eap = target;
			return 4;
		}
		if (target >= test_high_memory_start && target < test_high_memory_end) {
			put_long_test(pc, target);
			*eap = target;
			return 4;
		}
		if (target >= test_memory_start && target < test_memory_end) {
			put_long_test(pc, target);
			*eap = target;
			return 4;
		}
		return -2;
	}
	case imm:
		if (srcdst)
			return -2;
		if (dp->size == sz_long) {
			put_long_test(pc, rand32());
			return 4;
		} else {
			put_word_test(pc, rand16());
			return 2;
		}
		break;
	case imm0:
	{
		uae_u16 v;
		if (srcdst)
			return -2;
		v = rand16();
		if ((imm8_cnt & 3) == 0)
			v &= 0xff;
		imm8_cnt++;
		put_word_test(pc, v);
		return 2;
	}
	case imm1:
	{
		bool vgot = false;
		uae_u16 v;
		if (dp->mnemo == i_MOVES) {
			v = imm16_cnt << 11;
			v |= rand16() & 0x07ff;
			imm16_cnt++;
			*isconstant = 32;
			put_word_test(pc, v);
			return 2;
		}
		if (srcdst) {
			if (dp->mnemo != i_DBcc)
				return -2;
		}
		if (dp->mnemo == i_DBcc || dp->mnemo == i_BSR || dp->mnemo == i_Bcc) {
			uae_u32 pct = pc + 2 - 2;
			if (target <= pct + 0x7ffe && target >= pct - 0x8000) {
				v = target - pct;
				*eap = target;
				vgot = true;
			} else {
				return -2;
			}
		}
		if (!vgot)
			v = rand16();
		put_word_test(pc, v);
		return 2;
	}
	case imm2:
	{
		bool vgot = false;
		uae_u32 v;
		if (srcdst)
			return -2;
		if (dp->mnemo == i_BSR || dp->mnemo == i_Bcc) {
			if (currprefs.cpu_model < 68020)
				return -2;
			uae_u32 pct = pc + 2 - 2;
			v = target - pct;
			*eap = target;
			vgot = true;
		}
		if (!vgot) {
			v = rand32();
		}
		put_long_test(pc, v);
		return 4;
	}
	case immi:
	{
		uae_u8 v = (*opcodep) & 0xff;
		if (srcdst)
			return -2;
		if (dp->mnemo == i_BSR || dp->mnemo == i_Bcc) {
			uae_u32 pct = pc - 2 + 2;
			if (pct + v == target) {
				*eap = target;
				return 0;
			}
		}
		return -2;
	}
	}
	return -2;
}

static int create_ea(uae_u16 *opcodep, uaecptr pc, int mode, int reg, struct instr *dp, int *isconstantp, int srcdst, int fpuopcode, int opcodesize, uae_u32 *eap, int *regusedp, int *fpuregusedp, int *flagsp)
{
	int am = mode >= imm ? imm : mode;
	int v;

	if (!((1 << am) & feature_addressing_modes[srcdst]))
		return -1;

	if (target_ea[srcdst] == 0xffffffff) {
		v = create_ea_random(opcodep, pc, mode, reg, dp, isconstantp, srcdst, fpuopcode, opcodesize, eap, regusedp, fpuregusedp, flagsp);
	} else {
		v = create_ea_exact(opcodep, pc, mode, reg, dp, isconstantp, srcdst, fpuopcode, opcodesize, eap, regusedp, fpuregusedp, flagsp);
	}
	if (*regusedp == 8 + 7) {
		*flagsp |= EAFLAG_SP;
	}
	return v;
}

// generate valid FPU memory access that returns valid FPU data.
static int generate_fpu_memory_read(uae_u16 opcode, uaecptr pc, struct instr *dp, int *isconstant, uae_u32 *eap, int *regused)
{
	int fpuopsize_t = fpuopsize;
	int reg = opcode & 7;
	int mode = (opcode >> 3) & 7;
	if (mode == 7) {
		mode = absw + reg;
		reg = 0;
		if (mode == imm && fpuopsize == 6) {
			fpuopsize = 4;
		}
	}
	uaecptr addr = 0xff0000ff; // FIXME: use separate flag
	int o = create_ea_random(&opcode, pc, mode, reg, dp, isconstant, 0, -1, 8, &addr, regused, NULL, NULL);
	fpuopsize = fpuopsize_t;
	if (o < 0)
		return o;
	if (addr != 0xff0000ff) {
		if (!check_valid_addr(addr, bytesizes[fpuopsize], 2))
			return -2;
		putfpuimm(addr, fpuopsize, NULL);
		*eap = addr;
	}
	return o;
}

static int imm_special;

static int handle_specials_preea(uae_u16 opcode, uaecptr pc, struct instr *dp, int *isconstant)
{
	int off = 0;
	int f = 0;
	if (dp->mnemo == i_FTRAPcc) {
		uae_u16 v = imm_special & 63;
		int mode = opcode & 7;
		put_word_test(pc, v);
		imm_special++;
		off += 2;
		pc += 2;
		if (mode == 2) {
			f = 1;
		} else if (mode == 3) {
			f = 2;
		} else {
			f = -1;
		}
		*isconstant = 64;
	}
	if (dp->mnemo == i_FScc || dp->mnemo == i_FDBcc) {
		uae_u16 v = immfpu_cnt & 63;
		immfpu_cnt++;
		*isconstant = 64;
		put_word_test(pc, v);
		pc += 2;
		off += 2;
		if (dp->mnemo == i_FDBcc) {
			f = 1;
		}
	} else if (dp->mnemo == i_FBcc) {
		opcode |= immfpu_cnt & 63;
		immfpu_cnt++;
		*isconstant = 64;
		f = (opcode & 0x40) ? 2 : 1;
	}
	if (f) {
		uae_s16 v;
		for (;;) {
			v = rand16() & ~1;
			if (v > 128 || v < -128)
				break;
		}
		if (f == 2) {
			put_long_test(pc, v);
			off += 4;
		} else if (f == 1) {
			put_word_test(pc, v);
			off += 2;
		}
	}
	if (dp->mnemo == i_MOVE16) {
		if (opcode & 0x20) {
			uae_u16 v = 0;
			v |= imm_special << 12;
			put_word_test(pc, v);
			imm_special++;
			off = 2;
		}
	}
	return off;
}

static int handle_specials_branch(uae_u16 opcode, uaecptr pc, struct instr *dp, int *isconstant)
{
	// 68020 BCC.L is BCC.B to odd address if 68000/010
	if ((opcode & 0xf0ff) == 0x60ff) {
		if (currprefs.cpu_model >= 68020) {
			return 0;
		}
		return -2;
	}
	return 0;
}

static int handle_specials_misc(uae_u16 opcode, uaecptr pc, struct instr *dp, int *isconstant)
{
	// PACK and UNPK has third parameter
	if (dp->mnemo == i_PACK || dp->mnemo == i_UNPK) {
		uae_u16 v = rand16();
		put_word_test(pc, v);
		*isconstant = 16;
		return 2;
	} else if (dp->mnemo == i_CALLM) {
		// CALLM has extra parameter
		uae_u16 v = rand16();
		put_word_test(pc, v);
		*isconstant = 16;
		return 2;
	}
	return 0;
}

static uaecptr handle_specials_extra(uae_u16 opcode, uaecptr pc, struct instr *dp, int *regused)
{
	// CAS undocumented (marked as zero in document) fields do something weird, for example
	// setting bit 9 will make "Du" address register but results are not correct.
	// so lets make sure unused zero bits are zeroed.
	specials_cnt++;
	switch (dp->mnemo)
	{
	case i_CAS:
	{
		uae_u16 extra = get_word_test(opcode_memory_start + 2);
		uae_u16 extra2 = extra;
		extra &= (7 << 6) | (7 << 0);
		if (extra != extra2) {
			put_word_test(opcode_memory_start + 2, extra);
		}
		if (specials_cnt & 1) {
			*regused = extra & 7;
		} else {
			*regused = (extra >> 6) & 7;
		}
		break;
	}
	case i_CAS2:
	{
		uae_u16 extra = get_word_test(opcode_memory_start + 2);
		uae_u16 extra2 = extra;
		extra &= (7 << 6) | (7 << 0) | (15 << 12);
		if (extra != extra2) {
			put_word_test(opcode_memory_start + 2, extra);
		}
		extra = get_word_test(opcode_memory_start + 4);
		extra2 = extra;
		extra &= (7 << 6) | (7 << 0) | (15 << 12);
		if (extra != extra2) {
			put_word_test(opcode_memory_start + 4, extra);
		}
		switch (specials_cnt & 7)
		{
		case 0:
		case 6:
			*regused = extra >> 12;
			break;
		case 1:
			*regused = (extra >> 6) & 7;
			break;
		case 2:
			*regused = (extra >> 0) & 7;
			break;
		case 7:
		case 3:
			*regused = extra2 >> 12;
			break;
		case 4:
			*regused = (extra2 >> 6) & 7;
			break;
		case 5:
			*regused = (extra2 >> 0) & 7;
			break;
		}
		break;
	}
	case i_CHK2: // also CMP2
	{
		uae_u16 extra = get_word_test(opcode_memory_start + 2);
		uae_u16 extra2 = extra;
		extra &= (31 << 11);
		if (extra != extra2) {
			put_word_test(opcode_memory_start + 2, extra);
		}
		*regused = extra >> 12;
		break;
	}
	case i_BFSET:
	case i_BFTST:
	case i_BFCLR:
	case i_BFCHG:
	{
		uae_u16 extra = get_word_test(opcode_memory_start + 2);
		if ((extra & 0x0800) && (extra & 0x0020)) {
			if (specials_cnt & 1) {
				*regused = (extra >> 6) & 7;
			} else {
				*regused = (extra >> 0) & 7;
			}
		} else if (extra & 0x0800) {
			*regused = (extra >> 6) & 7;
		} else if (extra & 0x0020) {
			*regused = (extra >> 0) & 7;
		}
		break;
	}
	case i_BFINS:
	case i_BFFFO:
	case i_BFEXTS:
	case i_BFEXTU:
	{
		uae_u16 extra = get_word_test(opcode_memory_start + 2);
		if (cpu_lvl >= 4) {
			// 68040+ and extra word bit 15 not zero (hidden A/D field):
			// REGISTER field becomes address register in some internal
			// operations, results are also wrong. So clear it here..
			if (extra & 0x8000) {
				extra &= ~0x8000;
				put_word_test(opcode_memory_start + 2, extra);
			}
		}
		if((extra & 0x0800) || (extra & 0x0020)) {
			if ((extra & 0x0800) && (specials_cnt & 3) == 0) {
				*regused = (extra >> 6) & 7;
			} else if ((extra & 0x0020) && (specials_cnt & 3) == 1) {
				*regused = (extra >> 0) & 7;
			} else {
				*regused = (extra >> 12) & 7;
			}
		} else {
			*regused = (extra >> 12) & 7;
		}
		break;
	}
	case i_DIVL:
	case i_MULL:
	{
		uae_u16 extra = get_word_test(opcode_memory_start + 2);
		if (cpu_lvl >= 4) {
			// same as BF instructions but also other bits need clearing
			// or results are unexplained..
			if (extra & 0x83f8) {
				extra &= ~0x83f8;
				put_word_test(opcode_memory_start + 2, extra);
			}
		}
		if (specials_cnt & 1) {
			*regused = (extra >> 12) & 7;
		} else {
			*regused = extra & 7;
		}
		break;
	}
	}
	return pc;
}

static uae_u32 generate_stack_return(int cnt)
{
	uae_u32 v;
	if (feature_loop_mode_jit) {
		// if loop mode: return after test instruction
		v = test_instruction_end_pc;
	} else if (target_ea[0] != 0xffffffff && feature_usp < 3) {
		// if target sp mode: always generate valid address
		v = target_ea[0];
	} else {
		v = rand32();
		switch (cnt & 3)
		{
		case 0:
		case 3:
			v = opcode_memory_start + 32768;
			break;
		case 1:
			v &= 0xffff;
			if (test_low_memory_start == 0xffffffff)
				v |= 0x8000;
			if (test_high_memory_start == 0xffffffff)
				v &= 0x7fff;
			break;
		case 2:
			v = opcode_memory_start + (uae_s16)v;
			break;
		}
		if (!feature_exception3_instruction)
			v &= ~1;
	}
	return v;
}

static int handle_rte(uae_u16 opcode, uaecptr pc, struct instr *dp, int *isconstant, uaecptr addr, int *stackoffset)
{
	// skip bus error/address error frames because internal fields can't be simply randomized
	int offset = 2;
	int frame, v;

	imm_special++;
	for (;;) {
		frame = (imm_special >> 2) & 15;
		// 68010 bus/address error
		if (cpu_lvl == 1 && (frame == 8)) {
			imm_special += 4;
			continue;
		}
		if ((cpu_lvl == 2 || cpu_lvl == 3) && (frame == 9 || frame == 10 || frame == 11)) {
			imm_special += 4;
			continue;
		}
		// 68040 FP post-instruction, FP unimplemented, Address error
		if (cpu_lvl == 4 && (frame == 3 || frame == 4 || frame == 7)) {
			imm_special += 4;
			continue;
		}
		// 68060 access fault/FP disabled, FP post-instruction
		if (cpu_lvl == 5 && (frame == 3 || frame == 4)) {
			imm_special += 4;
			continue;
		}
		// throwaway frame
		if (frame == 1) {
			imm_special += 4;
			continue;
		}
		if (feature_loop_mode_jit && frame != 0 && frame != 2) {
			imm_special += 4;
			continue;
		}
		break;	
	}
	v = imm_special >> 6;
	uae_u16 sr = v & 31;
	sr |= (v >> 5) << 12;
	if (feature_loop_mode_jit) {
		sr &= ~(0x8000 | 0x4000 | 0x1000);
		sr |= 0x2000;
	}
	put_word_test(addr, sr);
	addr += 2 + 4;
	// frame + vector offset
	put_word_test(addr, (frame << 12) | (rand16() & 0x0fff));
	addr += 2;
	*stackoffset += 2;
#if 0
	if (frame == 1) {
		int imm_special_tmp = imm_special;
		imm_special &= ~(15 << 2);
		if (rand8() & 1)
			imm_special |= 2 << 2;
		handle_rte(opcode, addr, dp, isconstant, addr);
		imm_special = imm_special_tmp;
		v = generate_stack_return(imm_special);
		put_long_test(addr + 2, v);
		offset += 8 + 2;
#endif
	if (frame == 2) {
		put_long_test(addr, rand32());
		*stackoffset += 4;
	}
	return offset;
}

static int handle_specials_stack(uae_u16 opcode, uaecptr pc, struct instr *dp, int *isconstant, int *stackoffset)
{
	int offset = 0;
	uaecptr addr = regs.regs[15] + (*stackoffset);
	if (dp->mnemo == i_RTE || dp->mnemo == i_RTD || dp->mnemo == i_RTS || dp->mnemo == i_RTR) {
		uae_u32 v;
		// RTE, RTD, RTS and RTR
 		if (dp->mnemo == i_RTR) {
			// RTR
			v = imm_special++;
			uae_u16 ccr = v & 31;
			ccr |= rand16() & ~31;
			put_word_test(addr, ccr);
			addr += 2;
			offset += 2;
			*isconstant = imm_special >= (1 << (0 + 5)) * 4 ? 0 : -1;
		} else if (dp->mnemo == i_RTE) {
			// RTE
			if (currprefs.cpu_model == 68000) {
				imm_special++;
				v = imm_special >> 2;
				uae_u16 sr = 0;
				if (v & 32)
					sr |= 0x2000;
				if (v & 64)
					sr |= 0x8000;
				// fill also unused bits
				sr |= sr >> 1;
				sr |= v & 31;
				sr |= (v & 7) << 5;
				sr |= feature_min_interrupt_mask << 8;
				put_word_test(addr, sr);
				addr += 2;
				offset += 2;
				*isconstant = imm_special >= (1 << (2 + 5)) * 4 ? 0 : -1;
			} else {
				offset += handle_rte(opcode, pc, dp, isconstant, addr, stackoffset);
				addr += 2;
				*isconstant = imm_special >= (1 << (4 + 5)) * 4 ? 0 : -1;
			}
		} else if (dp->mnemo == i_RTS) {
			// RTS
			imm_special++;
			*isconstant = imm_special >= 256 ? 0 : -1;
		}
		v = generate_stack_return(imm_special);
		put_long_test(addr, v);
		*stackoffset += offset + 4;
		if (out_of_test_space) {
			wprintf(_T(" handle_specials out of bounds access!?"));
			abort();
		}
	}
	return offset;
}

// instruction that reads or writes stack
static int stackinst(struct instr *dp)
{
	switch (dp->mnemo)
	{
	case i_RTS:
	case i_RTR:
	case i_RTD:
	case i_RTE:
	case i_UNLK:
		return 1;
	case i_BSR:
	case i_JSR:
	case i_LINK:
	case i_PEA:
		return 2;
	}
	return 0;
}

// instructions that check CCs
// does not use dp->ccuse because TrapV isn't actual CC instruction
static int isccinst(struct instr *dp)
{
	switch (dp->mnemo)
	{
	case i_Bcc:
	case i_DBcc:
	case i_Scc:
	case i_FBcc:
	case i_FDBcc:
	case i_FScc:
		return 1;
	case i_TRAPcc:
	case i_TRAPV:
	case i_FTRAPcc:
		return 2;

	}
	return 0;
}

// any instruction that can branch execution
static int isbranchinst(struct instr *dp)
{
	switch (dp->mnemo)
	{
	case i_Bcc:
	case i_BSR:
	case i_JMP:
	case i_JSR:
		return 1;
	case i_RTS:
	case i_RTR:
	case i_RTD:
	case i_RTE:
		return 2;
	case i_DBcc:
	case i_FBcc:
	case i_FDBcc:
		return -1;
	}
	return 0;
}

static int isunsupported(struct instr *dp)
{
	switch (dp->mnemo)
	{
	case i_MOVE2C:
	case i_FSAVE:
	case i_FRESTORE:
	case i_PFLUSH:
	case i_PFLUSHA:
	case i_PFLUSHAN:
	case i_PFLUSHN:
	case i_PTESTR:
	case i_PTESTW:
	case i_CPUSHA:
	case i_CPUSHL:
	case i_CPUSHP:
	case i_CINVA:
	case i_CINVL:
	case i_CINVP:
	case i_PLPAR:
	case i_PLPAW:
	case i_CALLM:
	case i_RTM:
		return 1;
	}
	if (feature_loop_mode_jit) {
		switch (dp->mnemo)
		{
		case i_MOVEC2:
		case i_RTE:
		case i_ILLG:
			return 1;
		}
	}
	return 0;
}

static const int interrupt_levels[] =
{
	0, 1, 1, 1, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 6, 6
};

static void check_interrupts(void)
{
	if (feature_interrupts == 1) {
		int ic = interrupt_count & 15;
		int lvl = interrupt_levels[ic];
		if (lvl > 0 && lvl > feature_min_interrupt_mask) {
			regs.ipl_pin = lvl;
			return;
		}
	}
}

static int get_ipl(void)
{
	return regs.ipl[0];
}

static void execute_ins(uaecptr endpc, uaecptr targetpc, struct instr *dp, bool fpumode)
{
	uae_u16 opc = regs.ir;
	int off = opcode_memory_address - opcode_memory_start + 2;
	uae_u16 opw1 = (opcode_memory[off + 0] << 8) | (opcode_memory[off + 1] << 0);
	uae_u16 opw2 = (opcode_memory[off + 2] << 8) | (opcode_memory[off + 3] << 0);
	if (opc == 0x4e72
		&& opw1 == 0xa000
		//&& opw2 == 0x4afc
		)
		printf("");
	if (regs.sr & 0x2000)
		printf("");
	if (regs.sr & 0x8000)
		printf("");
	if (regs.sr & 0x4000)
		printf("");

	// execute instruction
	flag_SPCFLAG_TRACE = 0;
	flag_SPCFLAG_DOTRACE = 0;
	trace_store_pc = 0xffffffff;
	mmufixup[0].reg = -1;
	mmufixup[1].reg = -1;

	MakeFromSR();

	low_memory_accessed = 0;
	high_memory_accessed = 0;
	test_memory_accessed = 0;
	test_memory_access_mask = 0;
	testing_active = 1;
	testing_active_opcode = opc;
	hardware_bus_error = 0;
	hardware_bus_error_fake = 0;
	read_buffer_prev = regs.irc;
	regs.read_buffer = regs.irc;
	regs.write_buffer = 0xf00d;
	exception_extra_frame_size = 0;
	exception_extra_frame_type = 0;
	cpu_cycles = 0;
	regs.loop_mode = 0;
	regs.ipl_pin = feature_initial_interrupt;
	interrupt_level = regs.ipl_pin;
	regs.ipl[0] = regs.ipl[1] = 0;
	regs.ipl_evt_pre = 0;
	regs.ipl_evt = 0;
	interrupt_level = 0;
	interrupt_cycle_cnt = 0;
	test_exception_orig = 0;
	waitstate_cycle_cnt = 0;
	cpuipldelay2 = 2;
	cpuipldelay4 = 4;

	int cnt = 2;
	uaecptr first_pc = regs.pc;
	uae_u32 loop_mode_reg = 0;
	int stop_count = 0;
	bool starts = regs.s;
	int startimask = regs.intmask;
	bool imaskintprevented = false;

	if (regs.ipl_pin > regs.intmask) {
		wprintf(_T("Interrupt immediately active! IPL=%d IMASK=%d\n"), regs.ipl_pin, regs.intmask);
		abort();
	}
	if (feature_interrupts == 1) {
		check_interrupts();
		if (regs.ipl_pin > regs.intmask) {
			Exception(24 + regs.ipl_pin);
			goto end;
		}
	}

	if (feature_loop_mode_68010) {
		// 68010 loop mode
		cnt = (feature_loop_mode_cnt + 1) * 2;
	} else if (feature_loop_mode_jit) {
		// JIT loop mode
		cnt = (feature_loop_mode_cnt + 1) * 20 * 2;
	}
	if (multi_mode) {
		cnt = 100;
	}

	for (;;) {

		// if supervisor stack is odd: exit
		if (regs.s && (regs.isp & 1)) {
			test_exception = -1;
			break;
		}

		if (cnt <= 0) {
			if (feature_loop_mode_jit && isccinst(dp) == 2) {
				// if instruction was always false
				test_exception = -1;
				break;
			}
			wprintf(_T(" Loop mode didn't end!?\n"));
			abort();
		}

		if (flag_SPCFLAG_TRACE) {
			do_trace();
		}

		check_interrupts();

		regs.instruction_pc = regs.pc;
		regs.fp_ea_set = false;
		uaecptr a7 = regs.regs[15];
		int s = regs.s;

		bool test_ins = false;
		if (regs.pc == first_pc) {
			test_ins = true;
			if (feature_loop_mode_jit || feature_loop_mode_68010) {
				loop_mode_reg = regs.regs[feature_loop_mode_register];
			}
		}

		if (cpu_stopped) {
			stop_count++;
			if (stop_count > 256) {
				break;
			}
		}

		test_opcode = opc;
		(*cpufunctbl_noret[opc])(opc);

		if (fpumode) {
			// skip result if it has too large or small exponent
			for (int i = 0; i < 8; i++) {
				if (regs.fp[i].fpx.high != cur_regs.fp[i].fpx.high || regs.fp[i].fpx.low != cur_regs.fp[i].fpx.low) {
					if (!fpu_precision_valid(regs.fp[i].fpx)) {
						test_exception = -1;
					}
				}
			}
		}

		if (test_ins) {
			// skip if test instruction modified loop mode register
			if (feature_loop_mode_jit || feature_loop_mode_68010) {
				if ((regs.regs[feature_loop_mode_register] & 0xffff) != (loop_mode_reg & 0xffff)) {
					test_exception = -1;
					break;
				}
			}
		}


		// One or more out of bounds memory accesses: skip
		if (out_of_test_space) {
			break;
		}

		// Supervisor mode and A7 was modified and not RTE+stack mode: skip this test round.
		if (s && regs.s && regs.regs[15] != a7 && (dp->mnemo != i_RTE || feature_usp < 3)) {
			// but not if RTE
			if (!is_superstack_use_required()) {
				test_exception = -1;
				break;
			}
		}

		// skip test if SR interrupt mask got too low
		if (regs.intmask < feature_min_interrupt_mask) {
			test_exception = -1;
			break;
		}

		// Amiga Chip ram does not support TAS or MOVE16
		if ((dp->mnemo == i_TAS || dp->mnemo == i_MOVE16) && low_memory_accessed) {
			test_exception = -1;
			break;
		}

		if (test_exception) {
			break;
		}

		if (((regs.pc == endpc && feature_interrupts < 2) || (regs.pc == targetpc && feature_interrupts < 2)) && !cpu_stopped) {
			// Trace is only added as an exception if there was no other exceptions
			// Trace stacked with other exception is handled later
			if (flag_SPCFLAG_DOTRACE && !test_exception && trace_store_pc == 0xffffffffff) {
				Exception(9);
				break;
			}
		}

		if (feature_interrupts >= 1) {
			if (flag_SPCFLAG_DOTRACE) {
				if (trace_store_pc != 0xffffffff) {
					wprintf(_T(" Full trace in interrupt mode!?\n"));
					abort();
				}
				MakeSR();
				if (cpu_stopped) {
					cpu_stopped = 0;
					m68k_incpci(4);
					regs.ir = get_iword_test(regs.pc + 0);
					regs.irc = get_iword_test(regs.pc + 2);
				}
				trace_store_pc = regs.pc;
				trace_store_sr = regs.sr;
				flag_SPCFLAG_DOTRACE = 0;
				// pending interrupt always triggers during trace processing
				regs.ipl[0] = regs.ipl_pin;
			}
		}

		int ipl = get_ipl();
		if (ipl > 0) {
			if (ipl > regs.intmask) {
				Exception(24 + ipl);
				break;
			}
			if (ipl > startimask && ipl <= regs.intmask && !imaskintprevented) {
				imaskintprevented = true;
			}
		}

		regs.ipl[0] = regs.ipl[1];
		regs.ipl[1] = 0;

		if (test_exception) {
			break;
		}

		if (!valid_address(regs.pc, 2, 0))
			break;

		if (!feature_loop_mode_jit && !feature_loop_mode_68010) {
			// trace after NOP
			if (flag_SPCFLAG_DOTRACE) {
				if (feature_interrupts == 3) {
					Exception(9);
					break;
				}
				MakeSR();
				// store only first
				if (trace_store_pc == 0xffffffff) {
					trace_store_pc = regs.pc;
					trace_store_sr = regs.sr;
					flag_SPCFLAG_DOTRACE = 0;
				}
			}
			if (currprefs.cpu_model >= 68020) {
				regs.ir = get_iword_test(regs.pc + 0);
				regs.irc = get_iword_test(regs.pc + 2);
			}
			opc = regs.ir;
			continue;
		}

		cnt--;

		if (!feature_loop_mode_jit && !feature_loop_mode_68010 && !multi_mode && opc != NOP_OPCODE) {
			wprintf(_T(" Test instruction didn't finish in single step in non-loop mode!?\n"));
			abort();
		}

		if (!regs.loop_mode)
			regs.ird = opc;

		if (currprefs.cpu_model >= 68020) {
			regs.ir = get_iword_test(regs.pc + 0);
			regs.irc = get_iword_test(regs.pc + 2);
		}
		opc = regs.ir;
	}

end:
	test_exception_orig = test_exception;

	// if still stopped: skip test
	if (cpu_stopped) {
		test_exception = -1;
	}

	if (feature_interrupts >= 2) {
		// Skip test if end exception and no prevented interrupt
		if ((regs.pc == endpc || regs.pc == targetpc) && !imaskintprevented) {
			test_exception = -1;
		}
		// skip if check or privilege violation
		if (test_exception == 6 || test_exception == 8) {
			test_exception = -1;
		}
		// Skip test if no exception
		if (!test_exception) {
			test_exception = -1;
		}
		// Interrupt start must be before test instruction,
		// after test instruction
		// or after end NOP instruction
		if (test_exception >= 24 && test_exception <= 24 + 8) {
			if (test_exception_addr != opcode_memory_address &&
				test_exception_addr != opcode_memory_address - 2 &&
				test_exception_addr != test_instruction_end_pc - 4 &&
				test_exception_addr != test_instruction_end_pc - 2 &&
				test_exception_addr != test_instruction_end_pc &&
				test_exception_addr != branch_target &&
				test_exception_addr != branch_target + 2) {
				test_exception = -1;
			}
		}
		if (starts && (test_exception >= 24 && test_exception <= 24 + 8)) {
			printf("");
		}

	}

	testing_active = 0;

	if (regs.s) {
		regs.regs[15] = regs.usp;
	}
}

static int noregistercheck(struct instr *dp)
{
	return 0;
}

static uae_u8 last_exception[256];
static int last_exception_len;
static int last_exception_extra;

// save expected exception stack frame
static uae_u8 *save_exception(uae_u8 *p, struct instr *dp)
{
	uae_u8 *op = p;
	p++;
	*p++ = test_exception_extra | (exception_extra_frame_type ? 0x40 : 0x00);
	// bus/address error aborted group 2 exception
	if (exception_extra_frame_type) {
		*p++ = exception_extra_frame_type;
	}
	// Separate, non-stacked Trace
	if (test_exception_extra & 0x80) {
		*p++ = trace_store_sr >> 8;
		*p++ = (uae_u8)trace_store_sr;
		p = store_rel(p, 0, opcode_memory_start, trace_store_pc, 1);
	}
	uae_u8 *sf = NULL;
	if (test_exception > 0) {
		sf = test_memory + test_memory_size + EXTRA_RESERVED_SPACE - exception_stack_frame_size;
		// parse exception and store fields that are unique
		// SR and PC was already saved with non-exception data
		if (cpu_lvl == 0) {
			if (test_exception == 2 || test_exception == 3) {
				// status
				uae_u8 st = sf[1];
				// save also current opcode if different than stacked opcode
				// used by basic exception check
				if (((sf[6] << 8) | sf[7]) != regs.opcode) {
					st |= 0x20;
				}
				*p++ = st;
				// opcode (which is not necessarily current opcode!)
				*p++ = sf[6];
				*p++ = sf[7];
				if (st & 0x20) {
					*p++ = regs.opcode >> 8;
					*p++ = (uae_u8)regs.opcode;
				}
				// access address
				p = store_rel(p, 0, opcode_memory_start, gl(sf + 2), 1);
				// program counter
				p = store_rel(p, 0, opcode_memory_start, gl(sf + 10), 1);
			} else {
				// program counter
				p = store_rel(p, 0, opcode_memory_start, gl(sf + 2), 1);
			}
		} else if (cpu_lvl > 0) {
			uae_u8 ccrmask = 0;
			uae_u16 frame = (sf[6] << 8) | sf[7];
			// program counter
			p = store_rel(p, 0, opcode_memory_start, gl(sf + 2), 1);
			// frame + vector offset
			*p++ = sf[6];
			*p++ = sf[7];
			switch (frame >> 12)
			{
			case 0:
				break;
			case 2:
				// instruction address or effective address
				p = store_rel(p, 0, opcode_memory_start, gl(sf + 8), 1);
				if (test_exception == 11 || test_exception == 55) { // FPU unimplemented?
					*p++ = regs.fp_ea_set ? 0x01 : 0x00;
				}
				break;
			case 3:
				// effective address
				p = store_rel(p, 0, opcode_memory_start, gl(sf + 8), 1);
				*p++ = regs.fp_ea_set ? 0x01 : 0x00;
				break;
			case 4: // floating point unimplemented (68040/060)
				// fault/effective address
				p = store_rel(p, 0, opcode_memory_start, gl(sf + 8), 1);
				// FSLW or PC of faulted instruction
				p = store_rel(p, 0, opcode_memory_start, gl(sf + 12), 1);
				*p++ = regs.fp_ea_set ? 0x01 : 0x00;
				break;
			case 8: // 68010 address/bus error
				// program counter
				p = store_rel(p, 0, opcode_memory_start, gl(sf + 2), 1);
				// SSW
				*p++ = sf[8];
				*p++ = sf[9];
				// fault address
				p = store_rel(p, 0, opcode_memory_start, gl(sf + 10), 1);
				// data output
				*p++ = sf[16];
				*p++ = sf[17];
				// data input
				*p++ = sf[20];
				*p++ = sf[21];
				// instruction
				*p++ = sf[24];
				*p++ = sf[25];
				// optional data input (some hardware does real memory fetch when CPU does the dummy fetch, some don't)
				*p++ = read_buffer_prev >> 8;
				*p++ = (uae_u8)read_buffer_prev;
				break;
			case 0x0a: // 68020/030 address error.
			case 0x0b: // Don't save anything extra, too many undefined fields and bits..
				exception_stack_frame_size = 0x08;
				break;
			default:
				wprintf(_T(" Unknown frame %04x!\n"), frame);
				abort();
			}
		}
	}
	if (last_exception_len > 0 && last_exception_len == exception_stack_frame_size && test_exception_extra == last_exception_extra && (!sf || !memcmp(sf, last_exception, exception_stack_frame_size))) {
		// stack frame was identical to previous
		p = op;
		*p++ = 0xff;
	} else {
		int datalen = (p - op) - 1;
		last_exception_len = exception_stack_frame_size;
		last_exception_extra = test_exception_extra;
		*op = (uae_u8)datalen;
		if (sf) {
			memcpy(last_exception, sf, exception_stack_frame_size);
		}
	}
	return p;
}

static uae_u16 get_ccr_ignore(struct instr *dp, uae_u16 extra, bool prerun)
{
	uae_u16 ccrignoremask = 0;
	if (!feature_undefined_ccr) {
		if (test_exception == 5 || exception_extra_frame_type == 5) {
			if (dp->mnemo == i_DIVS || dp->mnemo == i_DIVU || dp->mnemo == i_DIVL) {
				// DIV + Divide by Zero: flags are undefined
				ccrignoremask |= 0x08 | 0x04 | 0x02 | 0x01;
			}
		} else {
			if ((dp->mnemo == i_DIVS || dp->mnemo == i_DIVU || dp->mnemo == i_DIVL) && ((regs.sr & 0x02) || prerun)) {
				// DIV + V: other flags are undefined
				ccrignoremask |= 0x08 | 0x04 | 0x01;
			}
		}
		if (dp->mnemo == i_CHK2) {
			// CHK: N and V are undefined
			ccrignoremask |= 0x08 | 0x02;
		}
		if (dp->mnemo == i_CHK) {
			// CHK: N and V are undefined
			ccrignoremask |= 0x04 | 0x02 | 0x01;
		}
	}
	return ccrignoremask;
}

static int isfpp(int mnemo)
{
	switch (mnemo)
	{
	case i_FPP:
	case i_FBcc:
	case i_FDBcc:
	case i_FTRAPcc:
	case i_FScc:
	case i_FRESTORE:
	case i_FSAVE:
		return 1;
	}
	return 0;
}

static void outbytes(const TCHAR *s, uaecptr addr)
{
#if 0
	wprintf(_T(" %s=%08x="), s, addr);
	for (int i = 0; i < 8; i++) {
		uae_u8 c = get_byte_test(addr + i);
		if (i > 0)
			wprintf(_T("."));
		wprintf(_T("%02x"), c);
	}
#endif
}


static void generate_target_registers(uae_u32 target_address, uae_u32 *out)
{
	uae_u32 *a = out + 8;
	a[0] = target_address;
	a[1] = target_address + 1;
	a[2] = target_address + 2;
	a[3] = target_address + 4;
	a[4] = target_address - 6;
	a[5] = target_address + 6;
	for (int i = 0; i < 7; i++) {
		out[i] = i - 4;
	}
	out[7] = target_address - opcode_memory_start;
	generate_address_mode = 1;
}

static const TCHAR *sizes[] = { _T("B"), _T("W"), _T("L") };

static void test_mnemo(const TCHAR *path, const TCHAR *mnemo, const TCHAR *ovrfilename, int opcodesize, int fpuopcode)
{
	TCHAR dir[1000];
	uae_u8 *dst = NULL;
	int mn;
	int size;
	bool fpumode = 0;

	uae_u32 test_cnt = 0;

	if (mnemo == NULL || mnemo[0] == 0)
		return;

	size = 3;
	if (fpuopcode < 0) {
		if (opcodesize == 0)
			size = 2;
		else if (opcodesize == 4)
			size = 1;
		else if (opcodesize == 6)
			size = 0;
		if (opcodesize == -1)
			opcodesize = 8;
	}

	if (feature_instruction_size && !(feature_instruction_size & (1 << opcodesize))) {
		return;
	}

	filecount = 0;

	struct mnemolookup *lookup = NULL;
	for (int i = 0; lookuptab[i].name; i++) {
		lookup = &lookuptab[i];
		if (!_tcsicmp(lookup->name, mnemo) || (lookup->friendlyname && !_tcsicmp(lookup->friendlyname, mnemo)))
			break;
	}
	if (!lookup) {
		wprintf(_T("'%s' not found.\n"), mnemo);
		return;
	}

	mn = lookup->mnemo;
	xorshiftstate = lookup - lookuptab;
	TCHAR mns[200];
	_tcscpy(mns, lookup->name);
	if (fpuopcode >= 0) {
		xorshiftstate = 128 + fpuopcode;
		if (fpuopcodes[fpuopcode] == NULL) {
			_stprintf(mns, _T("FPP%02X"), fpuopcode);
		} else {
			_tcscpy(mns, fpuopcodes[fpuopcode]);
		}
		if (opcodesize == 7) {
			_tcscpy(mns, _T("FMOVECR"));
			xorshiftstate = 128 + 64;
		} else if (opcodesize == 8) {
			_tcscpy(mns, _T("FMOVEM"));
			xorshiftstate = 128 + 64 + 1;
		}
	}
	xorshiftstate += 256 * opcodesize;
	if (ovrfilename) {
		_tcscpy(mns, ovrfilename);
		for (int i = 0; i < _tcslen(ovrfilename); i++) {
			xorshiftstate <<= 1;
			xorshiftstate ^= ovrfilename[i];
		}
	}
	xorshiftstate ^= rnd_seed;

	int pathlen = _tcslen(path);
	_stprintf(dir, _T("%s%s"), path, mns);
	if (fpuopcode < 0) {
		if (size < 3) {
			_tcscat(dir, _T("."));
			_tcscat(dir, sizes[size]);
		}
	} else if (fpuopcode != FPUOPP_ILLEGAL) {
		_tcscat(dir, _T("."));
		_tcscat(dir, fpsizes[opcodesize < 7 ? opcodesize : 2]);
	}
	memset(inst_name, 0, sizeof(inst_name));
	ua_copy(inst_name, sizeof(inst_name), dir + pathlen);

	opcodecnt = 0;
	for (int opcode = 0; opcode < 65536; opcode++) {
		struct instr *dp = table68k + opcode;
		// if FPU mode: skip everything else than FPU instructions
		if (currprefs.fpu_model && (opcode & 0xf000) != 0xf000)
			continue;
		// match requested mnemonic
		if (dp->mnemo != lookup->mnemo)
			continue;
		// match requested size
		if ((size >= 0 && size <= 2) && (size != dp->size || dp->unsized))
			continue;
		if (size == 3 && !dp->unsized)
			continue;
		// skip all unsupported instructions if not specifically testing i_ILLG
		if (lookup->mnemo != i_ILLG && fpuopcode != FPUOPP_ILLEGAL) {
			if (dp->clev > cpu_lvl)
				continue;
			if (isunsupported(dp))
				return;
			if (isfpp(lookup->mnemo) && !currprefs.fpu_model)
				return;
		}
		opcodecnt++;
		fpumode = currprefs.fpu_model && isfpp(lookup->mnemo);
	}

	if (!opcodecnt)
		return;

	wprintf(_T("%s\n"), dir);

	int quick = 0;
	int rounds = feature_test_rounds;
	int data_saved = 0;
	int first_save = 1;

	int count = 0;

	subtest_count = 0;

	if (feature_loop_mode_jit) {
		registers[8 + 3] = test_memory_start; // A3 = start of test memory
	}

	ignore_register_mask = 0;
	ignore_register_mask |= 1 << 15; // SP
	if (feature_loop_mode_register >= 0) {
		ignore_register_mask |= 1 << feature_loop_mode_register;
	}
	if (feature_interrupts >= 2) {
		// D0 and D7 is modified by delay code
		ignore_register_mask |= 1 << 0;
		ignore_register_mask |= 1 << 7;
	}

	registers[8 + 6] = opcode_memory_start - 0x100;
	registers[15] = user_stack_memory_use;

	uae_u32 target_address = 0xffffffff;
	uae_u32 target_opcode_address = 0xffffffff;
	uae_u32 target_usp_address = 0xffffffff;
	target_ea[0] = 0xffffffff;
	target_ea[1] = 0xffffffff;
	target_ea[2] = 0xffffffff;
	if (feature_target_ea[0][2] && feature_target_ea[0][2] != 0xffffffff) {
		if (feature_usp == 3) {
			target_usp_address = feature_target_ea[0][2];
			target_ea[2] = target_usp_address;
			target_usp_address += opcode_memory_start;
		} else {
			target_opcode_address = feature_target_ea[0][2];
			target_ea[2] = target_opcode_address;
		}
	}
	if (feature_target_ea[0][0] != 0xffffffff) {
		target_address = feature_target_ea[0][0];
		target_ea[0] = target_address;
	} else if (feature_target_ea[0][1] != 0xffffffff) {
		target_address = feature_target_ea[0][1];
		target_ea[1] = target_address;
	}
	target_ea_src_cnt = 0;
	target_ea_dst_cnt = 0;
	target_ea_opcode_cnt = 0;
	generate_address_mode = 0;

	if (fpu_max_precision == 2) {
		fpustatus.floatx80_rounding_precision = 64;
	} else if (fpu_max_precision == 1) {
		fpustatus.floatx80_rounding_precision = 32;
	} else {
		fpustatus.floatx80_rounding_precision = 80;
	}
	fpustatus.float_rounding_mode = float_round_nearest_even;

	// 1.0
	fpuregisters[0] = int32_to_floatx80(1);
	// -1.0
	fpuregisters[1] = int32_to_floatx80(-1);
	// 0.0
	fpuregisters[2] = int32_to_floatx80(0);
	if (fpu_max_precision) {
		fpuregisters[7] = int32_to_floatx80(2);
	} else {
		// NaN
		fpuregisters[7] = floatx80_default_nan(NULL);
	}

	for (int i = 3; i < 7; i++) {
		uae_u32 v = rand32();
		if (v < 15 || v > -15)
			continue;
		fpuregisters[i] = int32_to_floatx80(v);
	}

	for (int i = 0; i < MAX_REGISTERS; i++) {
		cur_regs.regs[i] = registers[i];
	}
	for (int i = 0; i < 8; i++) {
		cur_regs.fp[i].fpx = fpuregisters[i];
	}
	cur_regs.fpiar = 0xffffffff;

	if (target_address != 0xffffffff) {
		generate_target_registers(target_address, cur_regs.regs);
	}

	dst = storage_buffer;

	if (low_memory && low_memory_size != 0xffffffff) {
		memcpy(low_memory, low_memory_temp, low_memory_size);
	}
	if (high_memory && high_memory_size != 0xffffffff) {
		memcpy(high_memory, high_memory_temp, high_memory_size);
	}
	memcpy(test_memory, test_memory_temp, test_memory_size);

	memset(storage_buffer, 0xdd, 1000);

	full_format_cnt = 0;
	last_exception_len = -1;
	interrupt_count = 0;
	interrupt_delay_cnt = 0;
	interrupt_pc = 0;
	waitstate_delay_cnt = 0;
	test_count_missed = 0;

	int sr_override = 0;

	uae_u32 target_ea_bak[3], target_address_bak, target_opcode_address_bak;

	rnd_seed_prev = rnd_seed;
	rand8_cnt_prev = rand8_cnt;
	rand16_cnt_prev = rand16_cnt_prev;
	rand32_cnt_prev = rand32_cnt_prev;
	xorshiftstate_prev = xorshiftstate;

	for (;;) {
		int got_something = 0;

		if (feature_interrupts >= 2) {
			rnd_seed = rnd_seed;
			rand8_cnt = rand8_cnt;
			rand16_cnt = rand16_cnt_prev;
			rand32_cnt = rand32_cnt_prev;
			xorshiftstate = xorshiftstate_prev;
		}

		target_ea_bak[0] = target_ea[0];
		target_ea_bak[1] = target_ea[1];
		target_ea_bak[2] = target_ea[2];
		target_address_bak = target_address;
		target_opcode_address_bak = target_opcode_address;

		opcode_memory_address = opcode_memory_start;
		uae_u8 *opcode_memory_ptr = opcode_memory;
		if (target_opcode_address != 0xffffffff) {
			opcode_memory_address += target_opcode_address;
			opcode_memory_ptr = get_addr(opcode_memory_address, 2, 0);
			if (opcode_memory_address >= safe_memory_start && opcode_memory_address < safe_memory_end)
				break;
		}

		if (quick)
			break;

		// overrides
		for (int i = 0; i < regdatacnt; i++) {
			struct regdata *rd = &regdatas[i];
			uae_u8 v = rd->type & CT_DATA_MASK;
			modify_reg(NULL, &cur_regs, rd->type, rd->data);
		}

		if (feature_loop_mode_jit || feature_loop_mode_68010) {
			cur_regs.regs[feature_loop_mode_register] &= 0xffff0000;
			cur_regs.regs[feature_loop_mode_register] |= feature_loop_mode_cnt - 1;
			cur_regs.regs[feature_loop_mode_register] |= 0x80000000;
		}

		for (int i = 0; i < MAX_REGISTERS; i++) {
			dst = store_reg(dst, CT_DREG + i, 0, cur_regs.regs[i], -1);
		}
		if (fpumode) {
			for (int i = 0; i < 8; i++) {
				dst = store_fpureg(dst, CT_FPREG + i, NULL, cur_regs.fp[i].fpx, 1);
			}
			dst = store_reg(dst, CT_FPIAR, 0, cur_regs.fpiar, -1);
		}
		cur_regs.sr = feature_initial_interrupt_mask << 8;

		uae_u32 srcaddr_old = 0xffffffff;
		uae_u32 dstaddr_old = 0xffffffff;
		uae_u32 srcaddr = 0xffffffff;
		uae_u32 dstaddr = 0xffffffff;
		uae_u32 branchtarget_old = 0xffffffff;
		uae_u32 instructionendpc_old = opcode_memory_start;
		uae_u32 startpc_old = opcode_memory_start;
		int branch_target_swap_mode_old = 0;
		int doopcodeswap = 1;

		if (feature_interrupts >= 2) {
			doopcodeswap = 0;
		}

		if (verbose) {
			if (target_ea[0] != 0xffffffff)
				wprintf(_T(" Target EA SRC=%08x\n"), target_ea[0]);
			if (target_ea[1] != 0xffffffff)
				wprintf(_T(" Target EA DST=%08x\n"), target_ea[1]);
			if (target_ea[2] != 0xffffffff)
				wprintf(_T(" Target EA OPCODE=%08x\n"), target_ea[2]);
		}

		for (int opcode = 0; opcode < 65536; opcode++) {

			struct instr *dp = table68k + opcode;
			// match requested mnemonic
			if (dp->mnemo != lookup->mnemo)
				continue;

			if (lookup->mnemo != i_ILLG && fpuopcode != FPUOPP_ILLEGAL) {
				// match requested size
				if ((size >= 0 && size <= 2) && (size != dp->size || dp->unsized))
					continue;
				if (size == 3 && !dp->unsized)
					continue;
				// skip all unsupported instructions if not specifically testing i_ILLG
				if (dp->clev > cpu_lvl)
					continue;
				if (dp->ccuse && !(feature_condition_codes & (1 << dp->cc)))
					continue;
			}

			if (feature_loop_mode_68010) {
				if (!opcode_loop_mode(opcode))
					continue;
				if (dp->mnemo == i_DBcc)
					continue;
			}

			got_something = 1;

			target_ea[0] = target_ea_bak[0];
			target_ea[1] = target_ea_bak[1];
			target_ea[2] = target_ea_bak[2];
			target_address = target_address_bak;
			target_opcode_address = target_opcode_address_bak;

			int extra_loops = 3;
			while (extra_loops-- > 0) {

				// force both odd and even immediate values
				// for better address error testing
				forced_immediate_mode = 0;
				if ((currprefs.cpu_model == 68000 || currprefs.cpu_model == 68010) && (feature_exception3_data == 1 || feature_exception3_instruction == 1)) {
					if (dp->size > 0) {
						if (extra_loops == 1)
							forced_immediate_mode = 1;
						if (extra_loops == 0)
							forced_immediate_mode = 2;
					} else {
						extra_loops = 0;
					}
				} else if (currprefs.cpu_model == 68020 && ((ad8r[0] & 3) == 2 || (pc8r[0] & 3) == 2 || (ad8r[1] & 3) == 2 || (pc8r[1] & 3) == 2)) {
					; // if only 68020+ addressing modes: do extra loops
				} else {
					extra_loops = 0;
				}

				imm8_cnt = 0;
				imm16_cnt = 0;
				imm32_cnt = 0;
				immabsl_cnt = 0;
				immabsw_cnt = 0;
				imm_special = 0;
				immfpu_cnt = 0;

				target_ea[0] = target_ea_bak[0];
				target_ea[1] = target_ea_bak[1];
				target_ea[2] = target_ea_bak[1];
				target_address = target_address_bak;
				target_opcode_address = target_opcode_address_bak;

				reset_ea_state();
				// retry few times if out of bounds access
				int oob_retries = 10;
				// if instruction has immediate(s), repeat instruction test multiple times
				// each round generates new random immediate(s)
				int constant_loops = 256;
				if (dp->mnemo == i_ILLG || fpuopcode == FPUOPP_ILLEGAL) {
					if (fpumode) {
						constant_loops = 65536;
					} else {
						constant_loops = 1;
					}
				}
				if (feature_interrupts) {
					constant_loops = 1;
				}

				while (constant_loops-- > 0) {
					uae_u8 oldcodebytes[OPCODE_AREA];
					uae_u8 oldbcbytes[BRANCHTARGET_AREA];
					uae_u8 *oldbcbytes_ptr = NULL;
					int oldbcbytes_inuse = 0;
					memcpy(oldcodebytes, opcode_memory, sizeof(oldcodebytes));

					uae_u16 opc = opcode;
					int isconstant_src = 0;
					int isconstant_dst = 0;
					int did_out_of_bounds = 0;
					int prev_test_count = test_count;
					int prev_subtest_count = subtest_count;

					uae_u32 branch_target_swap_address = 0;
					int branch_target_swap_mode = 0;
					uae_u32 branch_target_data_original = (ILLG_OPCODE << 16) | NOP_OPCODE;
					uae_u32 branch_target_data = branch_target_data_original;

					int srcregused = -1;
					int dstregused = -1;
					int srcfpuregused = -1;
					int dstfpuregused = -1;
					int srceaflags = 0;
					int dsteaflags = 0;

					int loopmode_jump_offset = 0;

					out_of_test_space = 0;
					noaccesshistory = 0;
					hardware_bus_error_fake = 0;
					hardware_bus_error = 0;
					ahcnt_current = 0;
					ahcnt_written = 0;
					multi_mode = 0;
					fpuopsize = -1;
					test_exception = 0;
					test_exception_extra = 0;
					lm_safe_address1 = 0xffffffff;
					lm_safe_address2 = 0xffffffff;
					test_absw = 0;

					target_ea[0] = target_ea_bak[0];
					target_ea[1] = target_ea_bak[1];
					target_ea[2] = target_ea_bak[2];
					target_address = target_address_bak;
					target_opcode_address = target_opcode_address_bak;

					if (target_usp_address != 0xffffffff) {
						cur_regs.regs[15] = target_usp_address;
					}

					memcpy(&regs, &cur_regs, sizeof(cur_regs));

					regs.usp = regs.regs[15];
					regs.isp = super_stack_memory - 0x80;

					if (opc == 0xf27c)
						printf("");
					//if (subtest_count >= 700)
					//	printf("");


					uaecptr startpc = opcode_memory_start;
					uaecptr pc = startpc + 2;

					// target opcode mode
					if (target_opcode_address != 0xffffffff) {
						pc -= 2;
						int cnt = 0;
						while (pc - 2 != opcode_memory_address) {
							put_word_test(pc, NOP_OPCODE);
							pc += 2;
							cnt++;
							if (cnt >= 16) {
								wprintf(_T("opcode target is too far from opcode address\n"));
								abort();
							}
						}
						startpc = opcode_memory_address;
					}

					// interrupt IPL timing test mode
					if (feature_interrupts >= 2) {
						pc -= 2;
						if (!feature_waitstates) {
							// save CCR
							if (cpu_lvl == 0) {
								// move sr,d7
								put_word_test(pc + 0, 0x40c7); // 4 cycles
							} else {
								// move ccr,d7
								put_word_test(pc + 0, 0x42c7); // 4 cycles
							}
							pc += 2;
						}
#if IPL_TRIGGER_ADDR_SIZE == 1
						// move.b #x,xxx.L (4 * 4 + 4)
						put_long_test(pc, (0x13fc << 16) | IPL_TRIGGER_DATA);
#else
						// move.w #x,xxx.L (4 * 4 + 4)
						put_long_test(pc, (0x33fc << 16) | IPL_TRIGGER_DATA);
#endif
						pc += 4;
						put_long_test(pc, IPL_TRIGGER_ADDR);
						pc += 4;
						// moveq #x,d0 (4 cycles)
						int shift = interrupt_delay_cnt;
						int ashift;
						if (shift > 63) {
							shift -= 63;
							ashift = 63;
						} else {
							ashift = 0;
						}
						if (shift > 63 || ashift > 63) {
							wprintf(_T("IPL delay count too large!\n"));
							abort();
						}

						put_word_test(pc, 0x7000 | (ashift & 63));
						pc += 2;
						// ror.l d0,d0 (4 + 4 + d0 * 2 cycles)
						put_word_test(pc, 0xe0b8);
						pc += 2;
						// moveq #x,d0 (4 cycles)
						put_word_test(pc, 0x7000 | (shift & 63));
						pc += 2;
						// ror.l d0,d0 (4 + 4 + d0 * 2 cycles)
						put_word_test(pc, 0xe0b8);
						pc += 2;
						if (feature_waitstates) {
							// move.w #x,0xdff058
							put_long_test(pc, (0x33fc << 16) | (2 * 64 + 63));
							pc += 4;
							put_long_test(pc, IPL_BLTSIZE);
							pc += 4;
							// lsl.b #1+,d0
							put_word_test(pc, 0xe308 + waitstate_delay_cnt * 0x200); // 4+n*2 cycles
							pc += 2;
						}
						if (!feature_waitstates) {
							// restore CCR
							// move d7, ccr
							put_word_test(pc, 0x44c7); // 12 cycles
							pc += 2;
						}
						put_word_test(pc, REAL_NOP_OPCODE); // 4 cycles
						pc += 2;
						if (feature_interrupts >= 3) {
							// or #$8000,sr
							put_long_test(pc, 0x007c8000);
							pc += 4;
						} else {
							put_word_test(pc, IPL_TEST_NOP1); // 4 cycles
							pc += 2;
						}
						opcode_memory_address = pc;
						pc += 2;
					}

					// UNLK loop mode special case
					if (dp->mnemo == i_UNLK && feature_loop_mode_jit) {
						pc -= 2;
						// link an,#x
						put_long_test(pc, 0x4e500004 | (dp->sreg << 16));
						pc += 4;
						opcode_memory_address = pc;
						loopmode_jump_offset = 4;
						pc += 2;
					}

					// Start address to start address + 3 must be accessible or
					// jump prefetch would cause early bus error which we don't want
					if (is_nowrite_address(startpc, 4)) {
						goto nextopcode;
					}

					if (dp->mnemo != i_ILLG && fpuopcode != FPUOPP_ILLEGAL) {

						pc += handle_specials_preea(opc, pc, dp, &isconstant_src);

						uae_u32 srcea = 0xffffffff;
						uae_u32 dstea = 0xffffffff;

						// create source addressing mode
						if (dp->suse) {
							int isconstant_tmp = isconstant_src;
							int o = create_ea(&opc, pc, dp->smode, dp->sreg, dp, &isconstant_src, 0, fpuopcode, opcodesize, &srcea, &srcregused, &srcfpuregused, &srceaflags);
							if (isconstant_src < isconstant_tmp && isconstant_src > 0) {
								isconstant_src = isconstant_tmp;
							}
							if (o < 0) {
								memcpy(opcode_memory, oldcodebytes, sizeof(oldcodebytes));
								if (o == -1)
									goto nextopcode;
								continue;
							}
							pc += o;
						}

						uae_u8 *ao = opcode_memory_ptr + 2;
						uae_u16 apw1 = (ao[0] << 8) | (ao[1] << 0);
						uae_u16 apw2 = (ao[2] << 8) | (ao[3] << 0);
						if (opc == 0x4662
							// && apw1 == 0x0000
							//&& apw2 == 0x7479
							)
							printf("");

						if (target_address != 0xffffffff && (dp->mnemo == i_MVMEL || dp->mnemo == i_MVMLE)) {
							// if MOVEM and more than 1 register: randomize address so that any MOVEM
							// access can hit target address
							uae_u16 mask = (opcode_memory_ptr[2] << 8) | opcode_memory_ptr[3];
							int count = 0;
							for (int i = 0; i < 16; i++) {
								if (mask & (1 << i))
									count++;
							}
							if (count > 0) {
								int diff = (rand8() % count);
 								if (dp->dmode == Apdi) {
									diff = -diff;
								}
								uae_u32 ta = target_address;
								target_address -= diff * (1 << dp->size);
								if (target_ea[0] == ta)
									target_ea[0] = target_address;
								if (target_ea[1] == ta)
									target_ea[1] = target_address;
							}
						}

						// if source EA modified opcode
						dp = table68k + opc;

						// create destination addressing mode
						if (dp->duse && fpuopsize < 0) {
							if (safe_memory_mode & 4) {
								target_ea[1] = target_ea[0];
							}
							int o = create_ea(&opc, pc, dp->dmode, dp->dreg, dp, &isconstant_dst, 1, fpuopcode, opcodesize, &dstea, &dstregused, &dstfpuregused, &dsteaflags);
							if (o < 0) {
								memcpy(opcode_memory, oldcodebytes, sizeof(oldcodebytes));
								if (o == -1)
									goto nextopcode;
								continue;
							}
							pc += o;
						}

						if (dp->mnemo == i_MOVES) {
							// MOVES is stupid
							if (!(get_word_test(pc - 2) & 0x0800)) {
								uae_u32 vv = dstea;
								dstea = srcea;
								srcea = vv;
							}
						}

						// requested target address but no EA? skip
						if (target_address != 0xffffffff && isbranchinst(dp) != 2 && (feature_usp < 3 || !stackinst(dp))) {
							if (srcea != target_address && dstea != target_address) {
								memcpy(opcode_memory, oldcodebytes, sizeof(oldcodebytes));
								continue;
							}
						}


						pc = handle_specials_extra(opc, pc, dp, &srcregused);

						// if destination EA modified opcode
						dp = table68k + opc;

						uae_u8 *bo = opcode_memory_ptr + 2;
						uae_u16 bopw1 = (bo[0] << 8) | (bo[1] << 0);
						uae_u16 bopw2 = (bo[2] << 8) | (bo[3] << 0);
						if (opc == 0x0ab3
							&& bopw1 == 0x5aa9
							&& bopw2 == 0xe5cc
							)
							printf("");

						// bcc.x
						pc += handle_specials_branch(opc, pc, dp, &isconstant_src);
						// misc
						pc += handle_specials_misc(opc, pc, dp, &isconstant_src);

						if (fpumode) {
							// append FNOP so that we detect pending FPU exceptions immediately
							put_long_test(pc, 0xf2800000);
							pc += 4;
						}

					} else {

						if (fpumode) {
							put_word_test(pc, 65535 - constant_loops);
							pc += 2;
						}

					}

					if (feature_interrupts >= 2) {
						put_long_test(pc, (IPL_TEST_NOP2 << 16) | REAL_NOP_OPCODE); // 8 cycles
						pc += 4;
					}

					// if bus error stack checking and RTE: copy USP to ISP before RTE
					if (dp->mnemo == i_RTE && feature_usp == 3) {
						put_word_test(opcode_memory_address, 0x4e6f); // MOVE USP,A7
						put_word_test(opcode_memory_address + 2, opc);
						pc += 2;
					} else {
						put_word_test(opcode_memory_address, opc);
					}

					if (extra_or || extra_and) {
						uae_u16 ew = get_word_test(opcode_memory_address + 2);
						uae_u16 ew2 = (ew | extra_or) & ~extra_and;
						if (ew2 != ew) {
							put_word_test(opcode_memory_address + 2, ew2);
						}
					}

					uae_u16 extraword = get_word_test(opcode_memory_address + 2);

					test_instruction_end_pc = pc;

					// loop mode
 					if (feature_loop_mode_jit || feature_loop_mode_68010) {
						int cctype = isccinst(dp);
						// dbf dn, opcode_memory_start
						if (feature_loop_mode_jit == 1 || feature_loop_mode_jit == 2) {
							// move ccr,(a3)+
							lm_safe_address1 = pc;
							put_word(pc, LM_OPCODE);
							pc += 2;
						} else if (feature_loop_mode_jit == 3) {
							static const int consts[] = { 5, 7, 9, 11, 0 };
							int c = condition_cnt % 5;
							int cond = consts[c];
							if (cond == 0) {
								// X
								put_long(pc, 0x91c8c188); // SUBA.L A0,A0 ; EXG.L D0,A0
								pc += 4;
								put_word(pc, 0x4040); // NEGX.W D0
								pc += 2;
							} else {
								// CZVN
								put_word(pc, (cond << 8) | 0x50c0); // Scc D0
								pc += 2;
							}
							put_word(pc, 0x3040); // MOVE.W D0,A0
							pc += 2;
							lm_safe_address1 = pc;
							put_word(pc, 0x36c8); // MOVE.W A0,(A3)+
							pc += 2;
							put_long(pc, 0x307c0000 | c); // MOVEA.W #x,A0
							pc += 4;
							lm_safe_address2 = pc;
							put_word(pc, 0x36c8); // MOVE.W A0,(A3)+
							pc += 2;
							condition_cnt++;
						}
						// dbf dn,label
						put_long_test(pc, ((0x51c8 | feature_loop_mode_register) << 16) | ((opcode_memory_address - pc - loopmode_jump_offset - 2) & 0xffff));
						pc += 4;
						if (feature_loop_mode_jit) {
							if (cctype == 2) {
								// generate matching CCR, for TRAPcc, TRAPv, FTRAPcc
								uae_u8 ccor, ccand;
								getcc(dp->cc, &ccor, &ccand);
								// move ccr,d0
								put_word(pc, 0x42c1);
								pc += 2;
								// or.w #x,d0
								put_long(pc, 0x00410000 | ccor);
								pc += 4;
								// and.w #x,d0
								put_long(pc, 0x02410000 | ccand);
								pc += 4;
								// move d0,ccr
								put_word(pc, 0x44c1);
								pc += 2;
								// bra.s start
								put_word(pc, 0x6000 + (0xfe - pc - opcode_memory_address));
								pc += 2;
							} else if (feature_loop_mode_jit == 2 || feature_loop_mode_jit == 4) {
								// generate extra round with randomized CCR
								// tst.l dx
								put_word(pc, 0x4a80 | feature_loop_mode_register);
								pc += 2;
								// bpl.s end
								put_word(pc, 0x6a08);
								pc += 2;
								// moveq #x,dx
								put_word(pc, 0x7000 | (feature_loop_mode_register << 9) | (feature_loop_mode_cnt - 1));
								pc += 2;
								// move #x,ccr
								put_long(pc, 0x44fc0000 | ((ccr_cnt++) & 0x1f));
								pc += 4;
								// bra.s start
								put_word_test(pc, 0x6000 | ((opcode_memory_address - pc - loopmode_jump_offset - 2) & 0xff));
								pc += 2;
							}
						}
					}

					if (pc - opcode_memory_address > OPCODE_AREA) {
						wprintf(_T("Test code too large!\n"));
						abort();
					}

					// if instruction was long enough to hit safe area, decrease pc until we are back to normal area
					while (is_nowrite_address(pc - 1, 1)) {
						pc -= 2;
					}

					// end word, needed to detect if instruction finished normally when
					// running on real hardware.
					uae_u32 originalendopcode = (NOP_OPCODE << 16) | ILLG_OPCODE;
					uae_u32 endopcode = originalendopcode;
					uae_u32 actualendpc = pc;
					uae_u32 instruction_end_pc = pc;

					if (fpuopcode != FPUOPP_ILLEGAL) {
						if (isconstant_src < 0 || isconstant_dst < 0) {
							constant_loops++;
							quick = 1;
						} else {
							// the smaller the immediate, the less test loops
							if (constant_loops > isconstant_src && constant_loops > isconstant_dst)
								constant_loops = isconstant_dst > isconstant_src ? isconstant_dst : isconstant_src;

							// don't do extra tests if no immediates
							if (!isconstant_dst && !isconstant_src)
								extra_loops = 0;
						}
					}

					if (out_of_test_space) {
						wprintf(_T( "Setting up test instruction generated out of bounds error!?\n"));
						abort();
					}

					noaccesshistory++;
					if (!is_nowrite_address(pc, 4)) {
						put_long_test(pc, endopcode); // illegal instruction + nop
						actualendpc += 4;
					} else if (!is_nowrite_address(pc, 2)) {
						put_word_test(pc, endopcode >> 16);
						actualendpc += 2;
					}
					noaccesshistory--;
					pc += 4;

					if (out_of_test_space) {
						wprintf(_T("out_of_test_space set!?\n"));
						abort();
					}

					TCHAR out[256];
					memset(out, 0, sizeof(out));
					// disassemble and output generated instruction
					memcpy(&regs, &cur_regs, sizeof(cur_regs));

					uaecptr nextpc;
					srcaddr = 0xffffffff;
					dstaddr = 0xffffffff;

					uae_u32 dflags = m68k_disasm_2(out, sizeof(out) / sizeof(TCHAR), opcode_memory_address, NULL, 0, &nextpc, 1, &srcaddr, &dstaddr, 0xffffffff, 0);
					if (verbose) {
						my_trim(out);
						wprintf(_T("%08u %s"), subtest_count, out);
						if (srcaddr != 0xffffffff) {
							outbytes(_T("S"), srcaddr);
						}
						if (dstaddr != 0xffffffff && srcaddr != dstaddr) {
							outbytes(_T("D"), dstaddr);
						}
						if (feature_loop_mode_jit || feature_loop_mode_68010 || verbose >= 3) {
							wprintf(_T("\n"));
							nextpc = opcode_memory_start;
							for (int i = 0; i < 20; i++) {
								out_of_test_space = false;
								if (get_word_test(nextpc) == ILLG_OPCODE)
									break;
								m68k_disasm_2(out, sizeof(out) / sizeof(TCHAR), nextpc, NULL, 0, &nextpc, 1, NULL, NULL, 0xffffffff, 0);
								my_trim(out);
								wprintf(_T("%08u %s\n"), subtest_count, out);
							}
						}
					}

					// disassembler may set this
 					out_of_test_space = false;

					if ((dflags & 1) && target_ea[0] != 0xffffffff && srcaddr != 0xffffffff && srcaddr != target_ea[0]) {
						if (verbose) {
							wprintf(_T(" Source address mismatch %08x <> %08x\n"), target_ea[0], srcaddr);
						}
						memcpy(opcode_memory, oldcodebytes, sizeof(oldcodebytes));
						continue;
					}
					if ((dflags & 2) && target_ea[1] != 0xffffffff && dstaddr != target_ea[1]) {
						if (verbose) {
							wprintf(_T(" Destination address mismatch %08x <> %08x\n"), target_ea[1], dstaddr);
						}
						memcpy(opcode_memory, oldcodebytes, sizeof(oldcodebytes));
						continue;
					}

					if ((dflags & 1) && target_ea[0] == 0xffffffff && (srcaddr & addressing_mask) >= safe_memory_start - 4 && (srcaddr & addressing_mask) < safe_memory_end + 4) {
						// random generated EA must never be inside safe memory
						memcpy(opcode_memory, oldcodebytes, sizeof(oldcodebytes));
						if (verbose) {
							wprintf(_T("\n"));
						}
						continue;
					}
					if ((dflags & 2) && target_ea[1] == 0xffffffff && (dstaddr & addressing_mask) >= safe_memory_start - 4 && (dstaddr & addressing_mask) < safe_memory_end + 4) {
						// random generated EA must never be inside safe memory
						memcpy(opcode_memory, oldcodebytes, sizeof(oldcodebytes));
						if (verbose) {
							wprintf(_T("\n"));
						}
						continue;
					}

#if 0
					// can't test because dp may be empty if instruction is invalid
					if (nextpc != pc - 2) {
						wprintf(_T(" Disassembler/generator instruction length mismatch!\n"));
						abort();
					}
#endif

					branch_target = 0xffffffff;
					branch_target_pc = 0xffffffff;
					int bc = isbranchinst(dp);
					if (bc) {
						if (bc < 0) {
							srcaddr = dstaddr;
						}
						if (bc == 2) {
							// RTS and friends
							int stackpcoffset = 0;
							int stackoffset = 0;
							// if loop mode: all loop mode rounds need stacked data
							for (int i = 0; i < (feature_loop_mode_cnt ? feature_loop_mode_cnt : 1); i++) {
								stackpcoffset = handle_specials_stack(opc, pc, dp, &isconstant_src, &stackoffset);
							}
							if (isconstant_src < 0 || isconstant_dst < 0) {
								constant_loops++;
								quick = 0;
							}
							srcaddr = get_long_test(regs.regs[15] + stackpcoffset);
						}
						// branch target is not accessible? skip.
						if ((((srcaddr & addressing_mask) >= cur_regs.regs[15] - 16 && (srcaddr & addressing_mask) <= cur_regs.regs[15] + 16) && dp->mnemo != i_RTE) || ((srcaddr & 1) && !feature_exception3_instruction && feature_usp < 2)) {
							// lets not jump directly to stack..
							if (verbose) {
								if (srcaddr & 1)
									wprintf(_T(" Branch target is odd (%08x)\n"), srcaddr);
								else
									wprintf(_T(" Branch target is stack (%08x)\n"), srcaddr);
							}
							memcpy(opcode_memory, oldcodebytes, sizeof(oldcodebytes));
							continue;
						}				
						testing_active = 1;
						if (feature_loop_mode_jit) {
							if ((srcaddr & addressing_mask) != test_instruction_end_pc && (!valid_address(srcaddr, 8, 1) ||
								((srcaddr + 2) & addressing_mask) == opcode_memory_start || ((srcaddr + 4) & addressing_mask) == opcode_memory_start ||
								((srcaddr + 6) & addressing_mask) == opcode_memory_start || ((srcaddr + 8) & addressing_mask) == opcode_memory_start)) {
								if (verbose) {
									wprintf(_T(" Branch target inaccessible (%08x)\n"), srcaddr);
								}
								testing_active = 0;
								memcpy(opcode_memory, oldcodebytes, sizeof(oldcodebytes));
								continue;
							}
						} else {
							if (!valid_address(srcaddr, 2, 1) || ((srcaddr + 2) & addressing_mask) == opcode_memory_start ||
								((srcaddr + 4) & addressing_mask) == opcode_memory_start || ((srcaddr + 6) & addressing_mask) == opcode_memory_start) {
								if (verbose) {
									wprintf(_T(" Branch target inaccessible (%08x)\n"), srcaddr);
								}
								testing_active = 0;
								memcpy(opcode_memory, oldcodebytes, sizeof(oldcodebytes));
								continue;
							}
						}
						testing_active = 0;
					}

					// no exit continue or goto after this

					uae_u8 *prev_dst = dst;

					oldbcbytes_inuse = 0;
					if (bc) {
						if (feature_loop_mode_jit) {
							if (srcaddr != test_instruction_end_pc) {
								if (feature_loop_mode_jit >= 3) {
									// addq.w #4,a3, jmp xxx
									put_long_test(srcaddr, 0x584b4ef9);
								} else {
									// addq.w #2,a3, jmp xxx
									put_long_test(srcaddr, 0x544b4ef9);
								}
								put_long_test(srcaddr + 4, test_instruction_end_pc);
							}
						} else {
							branch_target = srcaddr;
							// branch target = generate exception
							if (!is_nowrite_address(branch_target, 4)) {
								branch_target_swap_address = branch_target;
								branch_target_swap_mode = 1;
								if (!(branch_target & 1)) {
									oldbcbytes_inuse = 4;
									oldbcbytes_ptr = get_addr(branch_target, oldbcbytes_inuse, 0);
									memcpy(oldbcbytes, oldbcbytes_ptr, oldbcbytes_inuse);
									put_long_test(branch_target, branch_target_data);
								}
							} else if (!is_nowrite_address(branch_target, 2)) {
								branch_target_swap_address = branch_target;
								branch_target_swap_mode = 2;
								if (!(branch_target & 1)) {
									oldbcbytes_inuse = 2;
									oldbcbytes_ptr = get_addr(branch_target, oldbcbytes_inuse, 0);
									memcpy(oldbcbytes, oldbcbytes_ptr, oldbcbytes_inuse);
									put_word_test(branch_target, branch_target_data >> 16);
								}
							}
						}
					}

					if (subtest_count > 0)
						printf("");

					// save opcode memory
					dst = store_mem_bytes(dst, opcode_memory_start, instruction_end_pc - opcode_memory_start, oldcodebytes);

					// store branch target and stack modifications (these needs to be rolled back after the test)
					dst = store_mem_writes(dst, 1);

					uae_u32 instructionendpc_old_prev = instructionendpc_old;
					uae_u32 startpc_old_prev = startpc_old;
					uae_u32 branchtarget_old_prev = branchtarget_old;
					uae_u32 srcaddr_old_prev = srcaddr_old;
					uae_u32 dstaddr_old_prev = dstaddr_old;
					int interrupt_count_old_prev = interrupt_count;

					if (startpc != startpc_old) {
						dst = store_reg(dst, CT_PC, startpc_old, startpc, -1);
						startpc_old = startpc;
					}
					if (instruction_end_pc != instructionendpc_old) {
						dst = store_reg(dst, CT_ENDPC, instructionendpc_old, instruction_end_pc, -1);
						instructionendpc_old = instruction_end_pc;
					}
					if (srcaddr != srcaddr_old && (dflags & 1)) {
						dst = store_reg(dst, CT_SRCADDR, srcaddr_old, srcaddr, -1);
						srcaddr_old = srcaddr;
					}
					if (dstaddr != dstaddr_old && (dflags & 2)) {
						dst = store_reg(dst, CT_DSTADDR, dstaddr_old, dstaddr, -1);
						dstaddr_old = dstaddr;
					}
					if (branch_target != branchtarget_old || branch_target_swap_mode != branch_target_swap_mode_old) {
						dst = store_reg(dst, CT_BRANCHTARGET, branchtarget_old, branch_target, -2);
						branchtarget_old = branch_target;
						*dst++ = branch_target_swap_mode;
					}

					if (feature_usp >= 3) {
						dst = store_reg(dst, CT_AREG + 7, 0, target_usp_address, sz_long);
					}

					if (feature_interrupts >= 2) {
						*dst++ = CT_EDATA;
						*dst++ = CT_EDATA_IRQ_CYCLES;
						*dst++ = interrupt_delay_cnt;
					}

					// pre-test data end
					*dst++ = CT_END_INIT;

					int exception_array[256] = { 0 };
					int ok = 0;
					int cnt_stopped = 0;

					uae_u32 last_cpu_cycles = 0;
					regstruct last_regs;

					int ccr_done = 0;
					int prev_s_cnt = 0;
					int s_cnt = 0;
					int t_cnt = 0;

					int extraccr = 0;

					// extra loops for supervisor and trace
					uae_u16 sr_allowed_mask = feature_sr_mask & 0xf000;
					uae_u16 sr_mask_request = feature_sr_mask & 0xf000;

					for (;;) {
						uae_u16 sr_mask = 0;

						int maxflag;
						int maxflagcnt;
						int maxflagshift;
						int flagmode = 0;
						if (fpumode) {
							if (fpuopcode == FPUOPP_ILLEGAL || feature_flag_mode > 1) {
								// Illegal FPU instruction: all on/all off only (*2)
								maxflag = 2;
								maxflagshift = 1;
							} else if (dp->mnemo == i_FDBcc || dp->mnemo == i_FScc || dp->mnemo == i_FTRAPcc || dp->mnemo == i_FBcc) {
								// FXXcc-instruction: FPU condition codes (*16)
								maxflag = 16;
								maxflagshift = 4;
								flagmode = 1;
							} else {
								// Other FPU instructions: FPU rounding and precision (*16)
								maxflag = 16;
								maxflagshift = 4;
							}
						} else {
							maxflag = 32; // all flag combinations (*32)
							maxflagshift = 5;
							if (feature_flag_mode > 1 || (feature_flag_mode == 1 && (dp->mnemo == i_ILLG || !isccinst(dp)))) {
								// if not cc instruction or illegal or forced: all on/all off (*2)
								maxflag = 2;
								maxflagshift = 1;
							}
						}

						if (extraccr & 1)
							sr_mask |= 0x2000; // S
						if (extraccr & 2)
							sr_mask |= 0x4000; // T0
						if (extraccr & 4)
							sr_mask |= 0x8000; // T1
						if (extraccr & 8)
							sr_mask |= 0x1000; // M

						if((sr_mask & ~sr_allowed_mask) || (sr_mask & ~sr_mask_request))
							goto nextextra;

						if (extraccr) {
							*dst++ = (uae_u8)extraccr;
						}
						*dst++ = (uae_u8)(maxflag | (fpumode ? 0x80 : 0x00) | (flagmode ? 0x40 : 0x00));

						maxflagcnt = maxflag;

						// Test every CPU CCR or FPU SR/rounding/precision combination
						for (int ccrcnt = 0; ccrcnt < maxflagcnt; ccrcnt++) {

							bool skipped = false;
							int ccr = ccrcnt & ((1 << maxflagshift) - 1);

							out_of_test_space = 0;
							test_exception = 0;
							test_exception_extra = 0;
							cpu_stopped = 0;
							cpu_halted = 0;
							ahcnt_current = ahcnt_written;
							int ahcnt_start = ahcnt_current;

							if (feature_interrupts == 1) {
								*dst++ = (uae_u8)interrupt_count;
								interrupt_count++;
								interrupt_count &= 15;
							}

							memset(&regs, 0, sizeof(regs));

							// swap end opcode illegal/nop
							noaccesshistory++;
							if (doopcodeswap) {
								endopcode = (endopcode >> 16) | (endopcode << 16);
							}
							int extraopcodeendsize = ((endopcode >> 16) == NOP_OPCODE) ? 2 : 0;
							int endopcodesize = 0;
							if (!is_nowrite_address(pc - 4, 4)) {
								put_long_test(pc - 4, endopcode);
								endopcodesize = (endopcode >> 16) == NOP_OPCODE ? 2 : 4;
							} else if (!is_nowrite_address(pc - 4, 2)) {
								put_word_test(pc - 4, endopcode >> 16);
								endopcodesize = 2;
							}
							noaccesshistory--;
							
							// swap branch target illegal/nop
							noaccesshistory++;
							if (branch_target_swap_mode) {
								branch_target_pc = branch_target;
								if (branch_target_swap_mode == 1) {
									if (!(branch_target_swap_address & 1)) {
										branch_target_data = (branch_target_data >> 16) | (branch_target_data << 16);
										put_long_test(branch_target_swap_address, branch_target_data);
										if ((branch_target_data >> 16) == NOP_OPCODE)
											branch_target_pc = branch_target + 2;
										else
											branch_target_pc = branch_target;
									}
								} else if (branch_target_swap_mode == 2) {
									if (!(branch_target_swap_address & 1)) {
										branch_target_data = (branch_target_data >> 16) | (branch_target_data << 16);
										put_word_test(branch_target_swap_address, branch_target_data >> 16);
									}
								}
							} else {
								branch_target_pc = branch_target;
							}
							noaccesshistory--;

							// initialize CPU state

							regs.pc = startpc;
							regs.ir = get_word_test(regs.pc + 0);
							regs.irc = get_word_test(regs.pc + 2);

							if (regs.ir == ILLG_OPCODE && dp->mnemo != i_ILLG) {
								wprintf(_T(" Illegal as starting opcode!?\n"));
								abort();
							}

							// set up registers
							for (int i = 0; i < MAX_REGISTERS; i++) {
								regs.regs[i] = cur_regs.regs[i];
							}
							if (fpumode) {
								for (int i = 0; i < 8; i++) {
									regs.fp[i].fpx = cur_regs.fp[i].fpx;
								}
								regs.fpiar = cur_regs.fpiar;
								uae_u32 fpsr = 0, fpcr = 0;
								if (maxflag == 16) {
									if (flagmode) {
										fpsr = (ccr & 15) << 24;
									} else {
										fpcr = (ccr & 15) << 4;
									}
								} else {
									// 2
									fpsr = ((ccr & 1) ? 15 : 0) << 24;
									fpcr = ((ccr & 1) ? 15 : 0) << 4;
								}
								// override special register values
								for (int i = 0; i < regdatacnt; i++) {
									struct regdata *rd = &regdatas[i];
									uae_u8 v = rd->type & CT_DATA_MASK;
									if (v == CT_FPSR || v == CT_FPCR || v == CT_FPIAR) {
										if (v == CT_FPSR) {
											regs.fpsr = fpsr;
										} else if (v == CT_FPCR) {
											regs.fpcr = fpcr;
										}
										dst = modify_reg(dst, &regs, rd->type, rd->data);
										if (v == CT_FPSR) {
											fpsr = regs.fpsr;
										} else if (v == CT_FPCR) {
											fpcr = regs.fpcr;
										}
									}
								}
								// condition codes
								fpp_set_fpsr(fpsr);
								// precision and rounding
								fpp_set_fpcr(fpcr);
							}

							// all CCR combinations or only all ones/all zeros?
							if (maxflag >= 32) {
								regs.sr = (ccr & 0xff) | sr_mask;
							} else {
								regs.sr = ((ccr & 1) ? 31 : 0) | sr_mask;
							}
							if (feature_initial_interrupt_mask > ((regs.sr >> 8) & 7)) {
								regs.sr &= ~(7 << 8);
								regs.sr |= feature_initial_interrupt_mask << 8;
							}

							// override special register values
							for (int i = 0; i < regdatacnt; i++) {
								struct regdata *rd = &regdatas[i];
								uae_u8 v = rd->type & CT_DATA_MASK;
								if (v == CT_SR) {
									dst = modify_reg(dst, &regs, rd->type, rd->data);
								}
							}

							regs.usp = regs.regs[15];
							regs.isp = super_stack_memory - 0x80;
							if (regs.usp >= test_memory_start && regs.usp < test_memory_start + test_memory_size) {
								// copy user stack to super stack, for RTE etc support
								memcpy(test_memory + (regs.isp - test_memory_start), test_memory + (regs.usp - test_memory_start), 0x20);
							} else {
								skipped = 1;
							}
							regs.msp = super_stack_memory;
		
							// data size optimization, only store data
							// if it is different than in previous round
							if (!ccr && !extraccr) {
								memcpy(&last_regs, &regs, sizeof(regstruct));
							}

							if (regs.sr & 0x2000)
								prev_s_cnt++;

							if (subtest_count == 9)
 								printf("");

							// execute test instruction(s)
							execute_ins(pc - endopcodesize, branch_target_pc, dp, fpumode);

							if (regs.s)
								s_cnt++;

							uae_u8 *pcaddr;
							// validate PC (PC in exception space if exception is fine, don't set out of space)
							if (test_exception && regs.pc < 0x100 && low_memory) {
								pcaddr = low_memory + regs.pc;
							} else {
								pcaddr = get_addr(regs.pc, 2, 0);
							}

							// examine results

							if (fpumode) {
								// if FPU illegal test: skip unless PC equals start of instruction
								if (fpuopcode == FPUOPP_ILLEGAL && (test_exception != 11 && test_exception != 4 && test_exception != 8)) {
									skipped = 1;
								}
							}

							if (dp->mnemo == i_JMP || dp->mnemo == i_JSR) {
								// if A7 relative JMP/JSR and S=1: skip it because branch target was calculated using USP.
								if (((srceaflags | dsteaflags) & EAFLAG_SP) && regs.s) {
									skipped = 1;
								}
							}

							// exception check disabled
							if (test_exception < 0 || !exceptionenabletable[test_exception]) {
								skipped = 1;
							}

							// if only testing read bus errors, skip tests that generated only writes and vice-versa
							// skip also all tests don't generate any bus errors
							if ((hardware_bus_error == 0 && safe_memory_mode) ||
								((hardware_bus_error & 4) && !(safe_memory_mode & 4)) ||
								((hardware_bus_error & 1) && !(safe_memory_mode & 1)) ||
								((hardware_bus_error & 2) && !(safe_memory_mode & 2))) {
								skipped = 1;
							}

							// if testing bus errors: skip test if instruction under test didn't generate it
							// (it could have been following nop/illegal used for aligning instruction before
							// bus error boundary)
							if (hardware_bus_error && safe_memory_mode && regs.instruction_pc != startpc) {
								skipped = 1;
							}

							// skip if feature_target_opcode_offset mode and non-prefetch bus error
							if (target_opcode_address != 0xffffffff && (hardware_bus_error & 3)) {
								skipped = 1;
							}

							// exception 3 required but didn't get one?
							if (test_exception != 3) {
								if (feature_exception3_data == 2) {
									skipped = 1;
								}
								if (feature_exception3_instruction == 2) {
									skipped = 1;
								}
								if (feature_exception_vectors) {
									skipped = 1;
								}
							} else {
								// wanted read address error but got write
								if (target_ea[0] != 0xffffffff && (target_ea[0] & 1) && target_ea[1] == 0xffffffff && test_exception_3_w) {
									skipped = 1;
								}
								// wanted write address error but got read
								if (target_ea[1] != 0xffffffff && (target_ea[1] & 1) && target_ea[0] == 0xffffffff && !test_exception_3_w) {
									skipped = 1;
								}
								if (safe_memory_mode) {
									skipped = 1;
								}				
							}

							// skip exceptions if loop mode and not CC instruction
							if (test_exception >= 2 && test_exception != 4 && feature_loop_mode_jit && !isccinst(dp)) {
								skipped = 1;
							}

							if (feature_usp == 2) {
								// need exception 3
								if (test_exception != 3) {
									skipped = 1;
								}
							}

							if (cpu_stopped) {
								cnt_stopped++;
								// CPU stopped, skip test
								skipped = 1;
							} else if (cpu_halted) {
								// CPU halted or reset, skip test
								skipped = 1;
							} else if (out_of_test_space) {
								exception_array[0]++;
								// instruction accessed memory out of test address space bounds
								skipped = 1;
								did_out_of_bounds = true;
							} else if (test_exception < 0) {
								// something happened that requires test skip
								skipped = 1;
							} else if (test_exception) {
								// generated exception
								exception_array[test_exception]++;
								if (test_exception == 8) {
									if (!(sr_mask & 0x2000) && !(feature_sr_mask & 0x2000)) {
										// Privilege violation exception and S mask not set? Switch to super mode in next round.
										sr_mask_request |= 0x2000;
										sr_allowed_mask |= 0x2000;
									}
								}
								// got exception 3 but didn't want them?
								if (test_exception == 3) {
									if ((feature_usp != 1 && feature_usp != 2) && !feature_exception3_data && !(test_exception_3_fc & 2) && !feature_exception_vectors) {
										skipped = 1;
									}
									if ((feature_usp != 1 && feature_usp != 2) && !feature_exception3_instruction && (test_exception_3_fc & 2) && !feature_exception_vectors) {
										skipped = 1;
									}
								}
								if (flag_SPCFLAG_DOTRACE || test_exception == 9) {
									t_cnt++;
								}
							} else if (!skipped) {
								// instruction executed successfully
								ok++;
								// validate branch instructions
								if (isbranchinst(dp)) {
									if ((regs.pc != branch_target_pc && regs.pc != pc - endopcodesize)) {
										wprintf(_T(" Branch instruction target fault\n"));
										abort();
									}
									if (safe_memory_start != 0xffffffff && (branch_target_pc & addressing_mask) < safe_memory_start || (branch_target_pc & addressing_mask) >= safe_memory_end) {
										if ((pcaddr[0] != 0x4a && pcaddr[1] != 0xfc) && (pcaddr[0] != 0x4e && pcaddr[1] != 0x71)) {
											wprintf(_T(" Branch instruction target fault\n"));
											abort();
										}
									}
									if (!feature_loop_mode_jit) {
										uae_u32 t1 = gl(pcaddr - 2);
										uae_u32 t2 = gl(pcaddr);
										if (t1 != ((NOP_OPCODE << 16) | ILLG_OPCODE) && t2 != ((ILLG_OPCODE << 16) | NOP_OPCODE)) {
											wprintf(_T(" Branch instruction PC didn't point to branch target instructions!\n"));
											abort();
										}
									}
								}
							}

							// restore original branch target (This is not part of saved data)
							noaccesshistory++;
							put_long_test(pc - 4, originalendopcode);
							if (branch_target_swap_mode && !(branch_target_swap_address & 1)) {
								if (branch_target_swap_mode == 1) {
									put_long_test(branch_target_swap_address, branch_target_data_original);
								} else if (branch_target_swap_mode == 2) {
									put_word_test(branch_target_swap_address, branch_target_data_original >> 16);
								}
							}
							noaccesshistory--;

							if (flag_SPCFLAG_DOTRACE && test_exception_extra) {
								wprintf(_T(" Trace and stored trace at the same time!\n"));
								abort();
							}

							// did we have trace also active?
							if (flag_SPCFLAG_DOTRACE) {
								test_exception_extra = 0;
								if (regs.t1 || regs.t0) {
									if ((cpu_lvl < 4 && (test_exception == 5 || test_exception == 6 || test_exception == 7 || (test_exception >= 32 && test_exception <= 47)))
										||
										(cpu_lvl == 1 && test_exception == 14)) {
											test_exception_extra = 9;
									}
								}
							}
							if (trace_store_pc != 0xffffffff) {
								test_exception_extra = 9 | 0x80;
							}
							MakeSR();

							if (!skipped) {
								bool storeregs = true;
								// tell m68k code to skip register checks?
								if (noregistercheck(dp)) {
									*dst++ = CT_SKIP_REGS;
									storeregs = false;
								}
								// save modified registers
								for (int i = 0; i < MAX_REGISTERS; i++) {
									uae_u32 s = last_regs.regs[i];
									uae_u32 d = regs.regs[i];
									if (s != d || first_save) {
										if (storeregs) {
											dst = store_reg(dst, CT_DREG + i, s, d, first_save ? sz_long : -1);
										}
										last_regs.regs[i] = d;
									}
								}
								// SR/CCR
								uae_u32 ccrignoremask = get_ccr_ignore(dp, extraword, false) << 16;
								if ((regs.sr | ccrignoremask) != last_regs.sr || first_save) {
									dst = store_reg(dst, CT_SR, last_regs.sr, regs.sr | ccrignoremask, first_save ? sz_word : -1);
									last_regs.sr = regs.sr | ccrignoremask;
								}
								// PC
								if (regs.pc - extraopcodeendsize != last_regs.pc || first_save) {
									dst = store_rel(dst, CT_PC, last_regs.pc, regs.pc - extraopcodeendsize, 0);
									last_regs.pc = regs.pc - extraopcodeendsize;
								}
								// FPU stuff
								if (currprefs.fpu_model) {
									for (int i = 0; i < 8; i++) {
										floatx80 s = last_regs.fp[i].fpx;
										floatx80 d = regs.fp[i].fpx;
										if (s.high != d.high || s.low != d.low || first_save) {
											dst = store_fpureg(dst, CT_FPREG + i, &s, d, 0);
											last_regs.fp[i].fpx = d;
										}
									}
									if (regs.fpiar != last_regs.fpiar || first_save) {
										dst = store_reg(dst, CT_FPIAR, last_regs.fpiar, regs.fpiar, first_save  ? sz_long : -1);
										last_regs.fpiar = regs.fpiar;
									}
									if (regs.fpsr != last_regs.fpsr || first_save) {
										dst = store_reg(dst, CT_FPSR, last_regs.fpsr, regs.fpsr, first_save ? sz_long : -1);
										last_regs.fpsr = regs.fpsr;
									}
									if (regs.fpcr != last_regs.fpcr || first_save) {
										dst = store_reg(dst, CT_FPCR, last_regs.fpcr, regs.fpcr, first_save ? sz_long : -1);
										last_regs.fpcr = regs.fpcr;
									}
								}
								if (cpu_lvl <= 1 && (last_cpu_cycles != cpu_cycles || first_save)) {
									dst = store_reg(dst, CT_CYCLES, last_cpu_cycles, cpu_cycles, first_save ? sz_word : -1);
									last_cpu_cycles = cpu_cycles;
								}

								first_save = 0;

								// store test instruction generated changes
								dst = store_mem_writes(dst, 0);
								uae_u8 bcflag = regs.pc == branch_target_pc ? CT_BRANCHED : 0;
								// save exception, possible combinations:
								// - any exception except trace
								// - any exception except trace + trace stacked on top of previous exception
								// - any exception except trace + following instruction generated trace
								// - trace only
								if (test_exception) {
									*dst++ = CT_END | test_exception | bcflag;
									dst = save_exception(dst, dp);
								} else if (test_exception_extra) {
									*dst++ = CT_END | 1 | bcflag;
									dst = save_exception(dst, dp);
								} else {
									*dst++ = CT_END | bcflag;
								}
								test_count++;
								subtest_count++;
								ccr_done++;
							} else {
								*dst++ = CT_END_SKIP;
								last_exception_len = -1;
							}
							// undo any test instruction generated memory modifications
							undo_memory(ahist, ahcnt_start);
						}

					nextextra:
						extraccr++;
						if (extraccr >= 16)
							break;
					}
					*dst++ = CT_END;

					if (!ccr_done) {
						// undo whole previous ccr/extra loop if all tests were skipped
						dst = prev_dst;
						memcpy(opcode_memory, oldcodebytes, sizeof(oldcodebytes));
						if (oldbcbytes_inuse > 0)
							memcpy(oldbcbytes_ptr, oldbcbytes, oldbcbytes_inuse);
						oldbcbytes_inuse = 0;
						test_count = prev_test_count;
						subtest_count = prev_subtest_count;
						last_exception_len = -1;
						srcaddr_old = srcaddr_old_prev;
						dstaddr_old = dstaddr_old_prev;
						branchtarget_old = branchtarget_old_prev;
						instructionendpc_old = instructionendpc_old_prev;
						startpc_old = startpc_old_prev;
						interrupt_count = interrupt_count_old_prev;
						test_count_missed++;
					} else {
						test_count_missed = 0;
						full_format_cnt++;
						data_saved = 1;
						// if test used data or fpu register as a source or destination: modify it
						if (srcregused >= 0) {
							uae_u32 prev = cur_regs.regs[srcregused];
							if (regchange(srcregused, cur_regs.regs)) {
								dst = store_reg(dst, CT_DREG + srcregused, prev, cur_regs.regs[srcregused], -1);
							}
						}
						if (dstregused >= 0 && srcregused != dstregused) {
							uae_u32 prev = cur_regs.regs[dstregused];
							if (regchange(dstregused, cur_regs.regs)) {
								dst = store_reg(dst, CT_DREG + dstregused, prev, cur_regs.regs[dstregused], -1);
							}
						}
						if (srcfpuregused >= 0) {
							floatx80 prev = cur_regs.fp[srcfpuregused].fpx;
							if (fpuregchange(srcfpuregused, cur_regs.fp)) {
								dst = store_fpureg(dst, CT_FPREG + srcfpuregused, &prev, cur_regs.fp[srcfpuregused].fpx, 0);
							}
						}
						if (dstfpuregused >= 0 && srcfpuregused != dstfpuregused) {
							floatx80 prev = cur_regs.fp[dstfpuregused].fpx;
							if (fpuregchange(dstfpuregused, cur_regs.fp)) {
								dst = store_fpureg(dst, CT_FPREG + dstfpuregused, &prev, cur_regs.fp[dstfpuregused].fpx, 0);
							}
						}
					}
					if (verbose) {
						wprintf(_T(" OK=%d OB=%d S=%d/%d T=%d S=%d I=%d/%d EX=%d %08x %08x %08x"), ok, exception_array[0],
							prev_s_cnt, s_cnt, t_cnt, cnt_stopped, interrupt_delay_cnt, interrupt_cycle_cnt,
							test_exception_orig,
							test_exception_addr, regs.pc, interrupt_pc);
						if (!ccr_done)
							wprintf(_T(" X"));
						for (int i = 2; i < 128; i++) {
							if (exception_array[i])
								wprintf(_T(" E%d=%d"), i, exception_array[i]);
						}
					}
					// save to file and create new file if watermark reached
					if (dst - storage_buffer >= storage_buffer_watermark) {
						if (subtest_count > 0) {
							save_data(dst, dir, size);

							branchtarget_old = 0xffffffff;
							srcaddr_old = 0xffffffff;
							dstaddr_old = 0xffffffff;
							instructionendpc_old = opcode_memory_start;
							startpc_old = opcode_memory_start;
						}
						dst = storage_buffer;
						for (int i = 0; i < MAX_REGISTERS; i++) {
							dst = store_reg(dst, CT_DREG + i, 0, cur_regs.regs[i], -1);
							regs.regs[i] = cur_regs.regs[i];
						}
						if (currprefs.fpu_model) {
							for (int i = 0; i < 8; i++) {
								dst = store_fpureg(dst, CT_FPREG + i, NULL, cur_regs.fp[i].fpx, 0);
								regs.fp[i].fpx = cur_regs.fp[i].fpx;
							}
						}
					}
					if (verbose) {
						wprintf(_T("\n"));
					}
					// if we got out of bounds access, add extra retries
					if (did_out_of_bounds) {
						if (oob_retries) {
							oob_retries--;
							constant_loops++;
						} else {
							full_format_cnt++;
						}
					}
				}
			}
		nextopcode:;
			if (fpuopcode == FPUOPP_ILLEGAL) {
				break;
			}
		}

		if (data_saved) {
			save_data(dst, dir, size);
			data_saved = 0;
		}
		dst = storage_buffer;

		if (feature_interrupts < 2 && opcodecnt == 1 && target_address == 0xffffffff &&
			target_opcode_address == 0xffffffff && target_usp_address == 0xffffffff && subtest_count >= feature_test_rounds_opcode)
			break;

		if (lookup->mnemo == i_ILLG || fpuopcode == FPUOPP_ILLEGAL)
			break;

		int nextround = 0;
		if (target_address != 0xffffffff) {
			target_ea_src_cnt++;
			if (target_ea_src_cnt >= target_ea_src_max) {
				target_ea_src_cnt = 0;
				if (target_ea_src_max > 0)
					nextround = 1;
			}
			target_ea_dst_cnt++;
			if (target_ea_dst_cnt >= target_ea_dst_max) {
				target_ea_dst_cnt = 0;
				if (target_ea_dst_max > 0)
					nextround = 1;
			}
			target_ea[0] = 0xffffffff;
			target_ea[1] = 0xffffffff;
			if (feature_target_ea[target_ea_src_cnt][0] != 0xffffffff) {
				target_address = feature_target_ea[target_ea_src_cnt][0];
				target_ea[0] = target_address;
			} else if (feature_target_ea[target_ea_dst_cnt][1] != 0xffffffff) {
				target_address = feature_target_ea[target_ea_dst_cnt][1];
				target_ea[1] = target_address;
			}
			generate_target_registers(target_address, cur_regs.regs);
		} else {
			nextround = 1;
		}

		// interrupt delay test
		if (feature_interrupts >= 2) {
			if (feature_waitstates) {
				waitstate_delay_cnt++;
				if (waitstate_delay_cnt >= 3) {
					waitstate_delay_cnt = 0;
					interrupt_delay_cnt++;
				}
			} else {
				interrupt_delay_cnt++;
			}
			if (interrupt_delay_cnt > 2 * 63) {
				break;
			} else {
				nextround = -1;
				rounds = 1;
				quick = 0;
			}
		}

		if (target_opcode_address != 0xffffffff || target_usp_address != 0xffffffff) {
			nextround = 0;
			target_ea_opcode_cnt++;
			if (target_ea_opcode_cnt >= target_ea_opcode_max) {
				target_ea_opcode_cnt = 0;
				if (target_ea_opcode_max > 0)
					nextround = 1;
			} else {
				quick = 0;
			}
			if (feature_usp == 3) {
				target_usp_address = feature_target_ea[target_ea_opcode_cnt][2];
				target_usp_address += opcode_memory_start;
				target_ea[2] =  target_usp_address;
			} else {
				target_opcode_address = feature_target_ea[target_ea_opcode_cnt][2];
				target_ea[2] = opcode_memory_address + target_opcode_address;
			}
		}

		if (nextround) {
			rounds--;
			if (rounds < 0) {
				if (subtest_count >= feature_test_rounds_opcode || !got_something)
					break;
				rounds = 1;
			}
		}

		if (test_count_missed > 10000) {
			break;
		}

		if (nextround >= 0) {
			for (;;) {
				int r = rand8() & 7;
				if (regchange(r, cur_regs.regs)) {
					break;
				}
			}
			for (;;) {
				int r = rand8() & 7;
				if (regchange(r + 8, cur_regs.regs)) {
					break;
				}
			}
			cur_regs.regs[0] &= 0xffff;
			cur_regs.regs[8 + 0] &= 0xffff;
			cur_regs.regs[8 + 6]--;
			cur_regs.regs[8 + 7] -= 2;
			rnd_seed_prev = rnd_seed;
			rand8_cnt_prev = rand8_cnt;
			rand16_cnt_prev = rand16_cnt;
			rand32_cnt_prev = rand32_cnt;
			xorshiftstate_prev = xorshiftstate;
		}
	}

	markfile(dir);

	compressfiles(dir);

	mergefiles(dir);

	wprintf(_T("- %d tests\n"), subtest_count);
}

static void test_mnemo_text(const TCHAR *path, const TCHAR *mode)
{
	TCHAR modetxt[100];
	int mnemo = -1;
	int fpuopcode = -1;
	int sizes = -1;

	extra_and = 0;
	extra_or = 0;

	_tcscpy(modetxt, mode);
	my_trim(modetxt);
	TCHAR *s = _tcschr(modetxt, '.');
	if (s || feature_instruction_size) {
		TCHAR c = 0;
		if (s) {
			*s = 0;
			c = _totupper(s[1]);
		}
		if (c == 'B')
			sizes = 6;
		if (c == 'W')
			sizes = 4;
		if (c == 'L')
			sizes = 0;
		if (c == 'U')
			sizes = 8;
		if (c == 'S')
			sizes = 1;
		if (c == 'X')
			sizes = 2;
		if (c == 'P')
			sizes = 3;
		if (c == 'D')
			sizes = 5;
	}

	const TCHAR *ovrname = NULL;
	if (!_tcsicmp(modetxt, _T("DIVUL"))) {
		_tcscpy(modetxt, _T("DIVL"));
		extra_and = 0x0800;
		extra_and |= 0x0400;
		ovrname = _T("DIVUL");
	} else if (!_tcsicmp(modetxt, _T("DIVSL"))) {
		_tcscpy(modetxt, _T("DIVL"));
		extra_or = 0x0800;
		extra_and = 0x0400;
		ovrname = _T("DIVSL");
	} else if (!_tcsicmp(modetxt, _T("DIVS")) && sizes == 0) {
		_tcscpy(modetxt, _T("DIVL"));
		extra_or = 0x0800;
		extra_or |= 0x0400;
		ovrname = _T("DIVS");
	} else if (!_tcsicmp(modetxt, _T("DIVU")) && sizes == 0) {
		_tcscpy(modetxt, _T("DIVL"));
		extra_and = 0x0800;
		extra_or |= 0x0400;
		ovrname = _T("DIVU");
	} else if (!_tcsicmp(modetxt, _T("CHK2"))) {
		_tcscpy(modetxt, _T("CHK2"));
		extra_or = 0x0800;
		ovrname = _T("CHK2");
	} else if (!_tcsicmp(modetxt, _T("CMP2"))) {
		_tcscpy(modetxt, _T("CHK2"));
		extra_and = 0x0800;
		ovrname = _T("CMP2");
	} else if (!_tcsicmp(modetxt, _T("MULS")) && sizes == 0) {
		_tcscpy(modetxt, _T("MULL"));
		extra_or = 0x0800;
		ovrname = _T("MULS");
	} else if (!_tcsicmp(modetxt, _T("MULU")) && sizes == 0) {
		_tcscpy(modetxt, _T("MULL"));
		extra_and = 0x0800;
		ovrname = _T("MULU");
	} else if (!_tcsicmp(modetxt, _T("MOVEC"))) {
		_tcscpy(modetxt, _T("MOVEC2"));
		ovrname = _T("MOVEC");
	}

	for (int j = 0; lookuptab[j].name; j++) {
		if (!_tcsicmp(modetxt, lookuptab[j].name)) {
			mnemo = j;
			break;
		}
	}
	if (mnemo < 0) {
		if (_totlower(modetxt[0]) == 'f') {
			if (!_tcsicmp(modetxt, _T("fmovecr"))) {
				mnemo = i_FPP;
				sizes = 7;
				fpuopcode = 0;
			} else if (!_tcsicmp(modetxt, _T("fmovem"))) {
				mnemo = i_FPP;
				sizes = 8;
				fpuopcode = 0;
			} else if (!_tcsicmp(modetxt, _T("fillegal"))) {
				mnemo = i_FPP;
				fpuopcode = FPUOPP_ILLEGAL;
			} else {
				for (int i = 0; i < 128; i++) {
					TCHAR name[100];
					if (fpuopcodes[i]) {
						_tcscpy(name, fpuopcodes[i]);
					} else {
						_stprintf(name, _T("FPP%02X"), i);
					}
					if (!_tcsicmp(modetxt, name)) {
						mnemo = i_FPP;
						fpuopcode = i;
						break;
					}
				}
			}
		}

		if (mnemo < 0) {
			wprintf(_T("Couldn't find '%s'\n"), modetxt);
			return;
		}
	}

	if (mnemo >= 0 && sizes < 0) {
		if (fpuopcode >= 0) {
			for (int i = 0; i < 7; i++) {
				test_mnemo(path, lookuptab[mnemo].name, ovrname, i, fpuopcode);
				if (fpuopcode == FPUOPP_ILLEGAL)
					break;
			}
		} else {
			if (lookuptab[mnemo].mnemo == i_ILLG) {
				test_mnemo(path, lookuptab[mnemo].name, ovrname, -1, -1);
			} else {
				test_mnemo(path, lookuptab[mnemo].name, ovrname, 0, -1);
				test_mnemo(path, lookuptab[mnemo].name, ovrname, 4, -1);
				test_mnemo(path, lookuptab[mnemo].name, ovrname, 6, -1);
				test_mnemo(path, lookuptab[mnemo].name, ovrname, -1, -1);
			}
		}
	} else {
		test_mnemo(path, lookuptab[mnemo].name, ovrname, sizes, fpuopcode);
		if (fpuopcode > 0 && sizes == 3) {
			// Dynamic k-Factor packed (0 = FMOVE = FMOVECR)
			test_mnemo(path, lookuptab[mnemo].name, ovrname, 7, fpuopcode);
		}
	}
}

static void my_trim(TCHAR *s)
{
	int len;
	while (_tcslen(s) > 0 && _tcscspn(s, _T("\t \r\n")) == 0)
		memmove(s, s + 1, (_tcslen(s + 1) + 1) * sizeof(TCHAR));
	len = _tcslen(s);
	while (len > 0 && _tcscspn(s + len - 1, _T("\t \r\n")) == 0)
		s[--len] = '\0';
}

static int check_safe_memory(uae_u32 start, uae_u32 end)
{
	if (start == 0xffffffff)
		return 0;
	if (safe_memory_start < start)
		return 1;
	if (safe_memory_end > end)
		return 1;
	return 0;
}

static const TCHAR *addrmodes[] =
{
	_T("Dreg"), _T("Areg"), _T("Aind"), _T("Aipi"), _T("Apdi"), _T("Ad16"), _T("Ad8r"),
	_T("absw"), _T("absl"), _T("PC16"), _T("PC8r"), _T("imm"), _T("Ad8rf"), _T("PC8rf"),
	NULL
};
static const TCHAR *ccnames[] =
{
	_T("T"), _T("F"), _T("HI"),_T("LS"),_T("CC"),_T("CS"),_T("NE"),_T("EQ"),
	_T("VC"),_T("VS"),_T("PL"),_T("MI"),_T("GE"),_T("LT"),_T("GT"),_T("LE")
};


#define INISECTION _T("cputest")

static bool ini_getvalx(struct ini_data *ini, const TCHAR *sections, const TCHAR *key, int *val)
{
	bool ret = false;
	while (*sections) {
		const TCHAR *sect = sections;
		if (_tcsicmp(sections, INISECTION)) {
			TCHAR *tout = NULL;
			if (ini_getstring(ini, sections, key, &tout)) {
				if (!_tcsicmp(tout, _T("*"))) {
					sect = INISECTION;
				}
				xfree(tout);
			}
		}
		if (ini_getval(ini, sect, key, val))
			ret = true;
		sections += _tcslen(sections) + 1;
	}
	if (ret)
		wprintf(_T("%s=%08x (%d)\n"), key, *val, *val);
	return ret;
}
static bool ini_getstringx(struct ini_data *ini, const TCHAR *sections, const TCHAR *key, TCHAR **out)
{
	struct ini_context ictx;
	ini_initcontext(ini, &ictx);
	bool ret = false;
	*out = NULL;
	while (*sections) {
		TCHAR *tout = NULL;
		if (ini_getstring_multi(ini, sections, key, &tout, &ictx)) {
			if (!_tcsicmp(tout, _T("*"))) {
				xfree(tout);
				if (!ini_getstring(ini, INISECTION, key, &tout)) {
					tout = my_strdup(_T(""));
				}
			}
			ret = true;
			if (*out) {
				free(*out);
			}
			*out = tout;
			TCHAR *tout2 = NULL;
			ini_setnextasstart(ini, &ictx);
			while (ini_getstring_multi(ini, sections, key, &tout2, &ictx)) {
				TCHAR *out2 = xcalloc(TCHAR, _tcslen(*out) + _tcslen(tout2) + 1 + 1);
				_tcscpy(out2, *out);
				_tcscat(out2, _T(","));
				_tcscat(out2, tout2);
				xfree(*out);
				*out = out2;
				ini_setnextasstart(ini, &ictx);
			}
		}
		sections += _tcslen(sections) + 1;
	}
	if (ret)
		wprintf(_T("%s=%s\n"), key, *out);
	return ret;
}

static const struct regtype regtypes[] =
{
	{ _T("D0"), CT_SIZE_LONG | (CT_DREG + 0) },
	{ _T("D1"), CT_SIZE_LONG | (CT_DREG + 1) },
	{ _T("D2"), CT_SIZE_LONG | (CT_DREG + 2) },
	{ _T("D3"), CT_SIZE_LONG | (CT_DREG + 3) },
	{ _T("D4"), CT_SIZE_LONG | (CT_DREG + 4) },
	{ _T("D5"), CT_SIZE_LONG | (CT_DREG + 5) },
	{ _T("D6"), CT_SIZE_LONG | (CT_DREG + 6) },
	{ _T("D7"), CT_SIZE_LONG | (CT_DREG + 7) },

	{ _T("A0"), CT_SIZE_LONG | (CT_AREG + 0) },
	{ _T("A1"), CT_SIZE_LONG | (CT_AREG + 1) },
	{ _T("A2"), CT_SIZE_LONG | (CT_AREG + 2) },
	{ _T("A3"), CT_SIZE_LONG | (CT_AREG + 3) },
	{ _T("A4"), CT_SIZE_LONG | (CT_AREG + 4) },
	{ _T("A5"), CT_SIZE_LONG | (CT_AREG + 5) },
	{ _T("A6"), CT_SIZE_LONG | (CT_AREG + 6) },
	{ _T("A7"), CT_SIZE_LONG | (CT_AREG + 7) },

	{ _T("FP0"), CT_SIZE_FPU | (CT_FPREG + 0) },
	{ _T("FP1"), CT_SIZE_FPU | (CT_FPREG + 1) },
	{ _T("FP2"), CT_SIZE_FPU | (CT_FPREG + 2) },
	{ _T("FP3"), CT_SIZE_FPU | (CT_FPREG + 3) },
	{ _T("FP4"), CT_SIZE_FPU | (CT_FPREG + 4) },
	{ _T("FP5"), CT_SIZE_FPU | (CT_FPREG + 5) },
	{ _T("FP6"), CT_SIZE_FPU | (CT_FPREG + 6) },
	{ _T("FP7"), CT_SIZE_FPU | (CT_FPREG + 7) },

	{ _T("CCR"), CT_SR | CT_SIZE_BYTE, },
	{ _T("SR"), CT_SR | CT_SIZE_WORD, },
	{ _T("FPCR"), CT_FPCR | CT_SIZE_LONG, },
	{ _T("FPSR"), CT_FPSR | CT_SIZE_LONG, },
	{ _T("FPIAR"), CT_FPIAR | CT_SIZE_LONG, },

	{ NULL }
};

static int cputoindex(int cpu)
{
	if (cpu == 68000) {
		return 0;
	} else if (cpu == 68010) {
		return 1;
	} else if (cpu == 68020) {
		return 2;
	} else if (cpu == 68030) {
		return 3;
	} else if (cpu == 68040) {
		return 4;
	} else if (cpu == 68060) {
		return 5;
	}
	return -1;
}

static int test(struct ini_data *ini, const TCHAR *sections, const TCHAR *testname)
{
	const struct cputbl *tbl = NULL;
	TCHAR path[1000], *vs;
	int v;

	wprintf(_T("Generating test '%s'\n"), testname);

	memset(exceptionenabletable, 1, sizeof(exceptionenabletable));

	v = 0;
	ini_getvalx(ini, sections, _T("enabled"), &v);
	if (!v) {
		wprintf(_T("Test disabled\n"));
		return 1;
	}

	bool cpufound = false;
	if (ini_getstringx(ini, sections, _T("cpu"), &vs)) {
		TCHAR *p = vs;
		while (p && *p) {
			TCHAR *pp1 = _tcschr(p, ',');
			TCHAR *pp2 = _tcschr(p, '-');
			if (pp1) {
				*pp1++ = 0;
				int idx = cputoindex(_tstol(p));
				if (idx < 0) {
					wprintf(_T("Invalid CPU string '%s'\n"), vs);
					return 0;
				}
				if (maincpu[idx]) {
					cpufound = true;
					break;
				}
				p = pp1;
			} else if (pp2) {
				*pp2++ = 0;
				int idx1 = cputoindex(_tstol(p));
				int idx2 = cputoindex(_tstol(pp2));
				if (idx1 < 0 || idx2 < 0) {
					wprintf(_T("Invalid CPU string '%s'\n"), vs);
					return 0;
				}
				for (int i = idx1; i <= idx2; i++) {
					if (maincpu[i]) {
						cpufound = true;
						break;
					}
				}
				if (cpufound) {
					break;
				}
				p = pp2;
			} else {
				int idx = cputoindex(_tstol(p));
				if (idx < 0) {
					wprintf(_T("Invalid CPU string '%s'\n"), vs);
					return 0;
				}
				if (maincpu[idx]) {
					cpufound = true;
				}
				break;
			}
		}
		xfree(vs);
	}

	if (!cpufound) {
		wprintf(_T("Test skipped. CPU does not match.\n"));
		return 1;
	}

	currprefs.address_space_24 = 1;
	addressing_mask = 0x00ffffff;
	v = 68030;
	ini_getvalx(ini, sections, _T("cpu_address_space"), &v);
	if (v < 68000) {
		if (v == 32) {
			currprefs.address_space_24 = 0;
			addressing_mask = 0xffffffff;
		}
	} else if (currprefs.cpu_model >= v) {
		currprefs.address_space_24 = 0;
		addressing_mask = 0xffffffff;
	}

	currprefs.int_no_unimplemented = true;
	ini_getvalx(ini, sections, _T("cpu_unimplemented"), &v);
	if (v) {
		currprefs.int_no_unimplemented = false;
	}

	currprefs.fpu_model = 0;
	currprefs.fpu_mode = 1;
	ini_getvalx(ini, sections, _T("fpu"), &currprefs.fpu_model);
	if (currprefs.fpu_model) {
		if (currprefs.fpu_model < 0) {
			currprefs.fpu_model = currprefs.cpu_model;
			if (currprefs.fpu_model == 68020 || currprefs.fpu_model == 68030) {
				currprefs.fpu_model = 68882;
			}
		} else if (currprefs.cpu_model >= 68040) {
			currprefs.fpu_model = currprefs.cpu_model;
		} else if (currprefs.cpu_model >= 68020 && currprefs.cpu_model <= 68030 && currprefs.fpu_model < 68881) {
			currprefs.fpu_model = 68882;
		}
		if (currprefs.fpu_model && currprefs.cpu_model < 68020) {
			wprintf(_T("FPU requires 68020+ CPU.\n"));
			return 0;
		}
		if (currprefs.fpu_model != 68881 && currprefs.fpu_model != 68882 && currprefs.fpu_model != 68040 && currprefs.fpu_model != 68060) {
			wprintf(_T("Unsupported FPU model.\n"));
			return 0;
		}
	}

	currprefs.fpu_no_unimplemented = true;
	if (ini_getvalx(ini, sections, _T("fpu_unimplemented"), &v)) {
		if (v) {
			currprefs.fpu_no_unimplemented = false;
		}
	}

	fpu_min_exponent = -65535;
	fpu_max_exponent = 65535;
	fpu_max_precision = 0;
	if (ini_getvalx(ini, sections, _T("fpu_min_exponent"), &v)) {
		if (v >= 0) {
			fpu_min_exponent = v;
		}
	}
	if (ini_getvalx(ini, sections, _T("fpu_max_exponent"), &v)) {
		if (v >= 0) {
			fpu_max_exponent = v;
		}
	}
	if (ini_getvalx(ini, sections, _T("fpu_max_precision"), &v)) {
		if (v == 1 || v == 2) {
			fpu_max_precision = v;
		}
	}
	if (ini_getvalx(ini, sections, _T("fpu_unnormals"), &v)) {
		if (v) {
			fpu_unnormals = 1;
		}
	}

	rnd_seed = 0;
	ini_getvalx(ini, sections, _T("seed"), &rnd_seed);

	verbose = 1;
	ini_getvalx(ini, sections, _T("verbose"), &verbose);

	feature_gzip = 0;
	ini_getvalx(ini, sections, _T("feature_gzip"), &feature_gzip);

	feature_addressing_modes[0] = 0xffffffff;
	feature_addressing_modes[1] = 0xffffffff;
	ad8r[0] = ad8r[1] = 1;
	pc8r[0] = pc8r[1] = 1;
	feature_condition_codes = 0xffffffff;

	feature_exception3_data = 0;
	ini_getvalx(ini, sections, _T("feature_exception3_data"), &feature_exception3_data);
	feature_exception3_instruction = 0;
	ini_getvalx(ini, sections, _T("feature_exception3_instruction"), &feature_exception3_instruction);

	safe_memory_start = 0xffffffff;
	if (ini_getvalx(ini, sections, _T("feature_safe_memory_start"), &v))
		safe_memory_start = v;
	safe_memory_end = 0xffffffff;
	if (ini_getvalx(ini, sections, _T("feature_safe_memory_size"), &v))
		safe_memory_end = safe_memory_start + v;
	safe_memory_mode = 0;
	if (ini_getstringx(ini, sections, _T("feature_safe_memory_mode"), &vs)) {
		if (_totupper(vs[0]) == 'R')
			safe_memory_mode |= 1;
		if (_totupper(vs[0]) == 'W')
			safe_memory_mode |= 2;
		if (_totupper(vs[0]) == 'P')
			safe_memory_mode |= 1 | 2 | 4;
		xfree(vs);
	}
	if (safe_memory_start == 0xffffffff || safe_memory_end == 0xffffffff) {
		safe_memory_end = 0xffffffff;
		safe_memory_start = 0xffffffff;
		safe_memory_mode = 0;
	}

	if (ini_getstringx(ini, sections, _T("feature_forced_register"), &vs)) {
		bool first = true;
		TCHAR *p = vs;
		while (p && *p) {
			TCHAR *pp = _tcschr(p, ',');
			if (pp) {
				*pp++ = 0;
			}
			TCHAR *pp2 = _tcschr(p, ':');
			if (pp2) {
				*pp2++ = 0;
				for (int i = 0; regtypes[i].name; i++) {
					if (!_tcsicmp(regtypes[i].name, p)) {
						if (regdatacnt >= MAX_REGDATAS) {
							wprintf(_T("Out of regdata!\n"));
							abort();
						}
						struct regdata *rd = &regdatas[regdatacnt];
						rd->type = regtypes[i].type;
						if (pp2[0] == '0' && (pp2[1] == 'x' || pp2[1] == 'X')) {
							TCHAR *endptr;
							rd->data[0] = _tcstol(pp2, &endptr, 16);
						} else {
							rd->data[0] = _tstol(pp2);
						}
						regdatacnt++;
						break;
					}
				}
			}
			p = pp;
		}
		xfree(vs);
	}

	

	if (ini_getstringx(ini, sections, _T("exceptions"), &vs)) {
		bool first = true;
		TCHAR *p = vs;
		while (p && *p) {
			TCHAR *pp = _tcschr(p, ',');
			if (pp) {
				*pp++ = 0;
			}
			int exc = _tstol(p);
			bool neg = exc < 0;
			if (exc < 0) {
				exc = -exc;
			}
			if (exc <= 2 || exc >= 256) {
				wprintf(_T("Invalid exception number %d.\n"), exc);
				return 0;
			}
			if (first) {
				memset(exceptionenabletable, neg ? 1 : 0, sizeof(exceptionenabletable));
				first = false;
			}
			exceptionenabletable[exc] = neg ? 0 : 1;
			p = pp;
		}
		xfree(vs);
	}

	for (int i = 0; i < MAX_TARGET_EA; i++) {
		feature_target_ea[i][0] = 0xffffffff;
		feature_target_ea[i][1] = 0xffffffff;
		feature_target_ea[i][2] = 0xffffffff;
	}
	for (int i = 0; i < 3; i++) {
		if (ini_getstringx(ini, sections, i == 2 ? _T("feature_target_opcode_offset") : (i ? _T("feature_target_dst_ea") : _T("feature_target_src_ea")), &vs)) {
			int cnt = 0;
			TCHAR *p = vs;
			int exp3cnt = 0;
			while (p && *p) {
				if (cnt >= MAX_TARGET_EA)
					break;
				TCHAR *pp = _tcschr(p, ',');
				if (pp) {
					*pp++ = 0;
				}
				TCHAR *endptr;
				int radix = i == 2 ? 10 : 16;
				if (_tcslen(p) > 2 && p[0] == '0' && _totupper(p[1]) == 'X') {
					p += 2;
					radix = 16;
				}
				feature_target_ea[cnt][i] = _tcstol(p, &endptr, radix);
				if (feature_target_ea[cnt][i] & 1) {
					exp3cnt++;
				}
				p = pp;
				cnt++;
			}
			if (i == 2) {
				target_ea_opcode_max = cnt;
				if (cnt > 0) {
					safe_memory_mode = 1 | 4;
				}
			} else if (i) {
				target_ea_dst_max = cnt;
			} else {
				target_ea_src_max = cnt;
			}
			xfree(vs);
			if (cnt > 0 && cpu_lvl <= 1) {
				if (exp3cnt == cnt) {
					if (feature_exception3_data < 2)
						feature_exception3_data = 2;
					if (feature_exception3_instruction < 2)
						feature_exception3_instruction = 2;
				} else {
					feature_exception3_data = 3;
					feature_exception3_instruction = 3;
				}
			}
		}
	}

	feature_sr_mask = 0;
	ini_getvalx(ini, sections, _T("feature_sr_mask"), &feature_sr_mask);
	feature_undefined_ccr = 0;
	ini_getvalx(ini, sections, _T("feature_undefined_ccr"), &feature_undefined_ccr);

	feature_initial_interrupt = 0;
	ini_getvalx(ini, sections, _T("feature_initial_interrupt"), &feature_initial_interrupt);

	feature_initial_interrupt_mask = 0;
	ini_getvalx(ini, sections, _T("feature_initial_interrupt_mask"), &feature_initial_interrupt_mask);

	feature_min_interrupt_mask = 0;
	ini_getvalx(ini, sections, _T("feature_min_interrupt_mask"), &feature_min_interrupt_mask);

	feature_loop_mode_jit = 0;
	feature_loop_mode_68010 = 0;
	ini_getvalx(ini, sections, _T("feature_loop_mode"), &feature_loop_mode_jit);
	if (feature_loop_mode_jit) {
		feature_loop_mode_cnt = 8;
		feature_loop_mode_68010 = 0;
		feature_loop_mode_register = 7;
	}
	feature_loop_mode_68010 = 0;
	ini_getvalx(ini, sections, _T("feature_loop_mode_68010"), &feature_loop_mode_68010);
	if (feature_loop_mode_68010) {
		feature_loop_mode_cnt = 8;
		feature_loop_mode_jit = 0;
		feature_loop_mode_register = 7;
	}
	if (feature_loop_mode_jit || feature_loop_mode_68010) {
		ini_getvalx(ini, sections, _T("feature_loop_mode_register"), &feature_loop_mode_register);
		ini_getvalx(ini, sections, _T("feature_loop_mode_cnt"), &feature_loop_mode_cnt);
	}

	feature_flag_mode = 0;
	ini_getvalx(ini, sections, _T("feature_flags_mode"), &feature_flag_mode);
	feature_usp = 0;
	ini_getvalx(ini, sections, _T("feature_usp"), &feature_usp);
	feature_exception_vectors = 0;
	ini_getvalx(ini, sections, _T("feature_exception_vectors"), &feature_exception_vectors);
	feature_interrupts = 0;
	ini_getvalx(ini, sections, _T("feature_interrupts"), &feature_interrupts);
	feature_waitstates = 0;
	ini_getvalx(ini, sections, _T("feature_waitstates"), &feature_waitstates);
	feature_ipl_delay = 0;
	ini_getvalx(ini, sections, _T("feature_ipl_delay"), &feature_ipl_delay);

	feature_full_extension_format = 0;
	if (currprefs.cpu_model >= 68020) {
		ini_getvalx(ini, sections, _T("feature_full_extension_format"), &feature_full_extension_format);
		if (feature_full_extension_format) {
			ad8r[0] |= 2;
			ad8r[1] |= 2;
			pc8r[0] |= 2;
			pc8r[1] |= 2;
		}
	}

	for (int j = 0; j < 2; j++) {
		TCHAR *am = NULL;
		if (ini_getstringx(ini, sections, j ? _T("feature_addressing_modes_dst") : _T("feature_addressing_modes_src"), &am)) {
			if (_tcslen(am) > 0) {
				feature_addressing_modes[j] = 0;
				ad8r[j] = 0;
				pc8r[j] = 0;
				TCHAR *p = am;
				while (p && *p) {
					TCHAR *pp = _tcschr(p, ',');
					if (pp) {
						*pp++ = 0;
					}
					TCHAR amtext[256];
					_tcscpy(amtext, p);
					my_trim(amtext);
					for (int i = 0; addrmodes[i]; i++) {
						if (!_tcsicmp(addrmodes[i], amtext)) {
							feature_addressing_modes[j] |= 1 << i;
							break;
						}
					}
					p = pp;
				}
				if (feature_addressing_modes[j] & (1 << Ad8r))
					ad8r[j] |= 1;
				if (feature_addressing_modes[j] & (1 << imm0)) // Ad8rf
					ad8r[j] |= 2;
				if (feature_addressing_modes[j] & (1 << PC8r))
					pc8r[j] |= 1;
				if (feature_addressing_modes[j] & (1 << imm1)) // PC8rf
					pc8r[j] |= 2;
				if (ad8r[j])
					feature_addressing_modes[j] |= 1 << Ad8r;
				if (pc8r[j])
					feature_addressing_modes[j] |= 1 << PC8r;
			}
			xfree(am);
		}
	}

	TCHAR *cc = NULL;
	if (ini_getstringx(ini, sections, _T("feature_condition_codes"), &cc)) {
		if (cc[0]) {
			feature_condition_codes = 0;
			TCHAR *p = cc;
			while (p && *p) {
				TCHAR *pp = _tcschr(p, ',');
				if (pp) {
					*pp++ = 0;
				}
				TCHAR cctext[256];
				_tcscpy(cctext, p);
				my_trim(cctext);
				for (int i = 0; ccnames[i]; i++) {
					if (!_tcsicmp(ccnames[i], cctext)) {
						feature_condition_codes |= 1 << i;
						break;
					}
				}
				p = pp;
			}
		}
		xfree(cc);
	}


	TCHAR *mode = NULL;
	ini_getstringx(ini, sections, _T("mode"), &mode);

	TCHAR *ipath = NULL;
	ini_getstringx(ini, sections, _T("path"), &ipath);
	if (!ipath) {
		_tcscpy(path, _T("data/"));
	} else {
		_tcscpy(path, ipath);
	}
	free(ipath);

	_stprintf(path + _tcslen(path), _T("%u_%s/"), (currprefs.cpu_model - 68000) / 10, testname);
	_wmkdir(path);

	xorshiftstate = 1;

	feature_test_rounds = 2;
	ini_getvalx(ini, sections, _T("test_rounds"), &feature_test_rounds);

	feature_test_rounds_opcode = 0;
	ini_getvalx(ini, sections, _T("min_opcode_test_rounds"), &feature_test_rounds_opcode);

	max_file_size = 200;
	ini_getvalx(ini, sections, _T("max_file_size"), &max_file_size);

	ini_getstringx(ini, sections, _T("feature_instruction_size"), &feature_instruction_size_text);
	for (int i = 0; i < _tcslen(feature_instruction_size_text); i++) {
		TCHAR c = _totupper(feature_instruction_size_text[i]);
		int sizes = -1;
		if (c == 'B')
			sizes = 6;
		else if (c == 'W')
			sizes = 4;
		else if (c == 'L')
			sizes = 0;
		else if (c == 'U')
			sizes = 8;
		else if (c == 'S')
			sizes = 1;
		else if (c == 'X')
			sizes = 2;
		else if (c == 'P')
			sizes = 3;
		else if (c == 'D')
			sizes = 5;
		if (sizes >= 0) {
			feature_instruction_size |= 1 << sizes;
		}
	}
	xfree(feature_instruction_size_text);

	ini_getvalx(ini, sections, _T("test_rounds"), &feature_test_rounds);

	v = 0;
	ini_getvalx(ini, sections, _T("test_memory_start"), &v);
	if (!v) {
		wprintf(_T("test_memory_start is required\n"));
		return 0;
	}
	test_memory_start = v;

	v = 0;
	ini_getvalx(ini, sections, _T("test_memory_size"), &v);
	if (!v) {
		wprintf(_T("test_memory_start is required\n"));
		return 0;
	}
	test_memory_size = v;
	test_memory_end = test_memory_start + test_memory_size;

	test_low_memory_start = 0xffffffff;
	test_low_memory_end = 0xffffffff;
	v = 0;
	if (ini_getvalx(ini, sections, _T("test_low_memory_start"), &v))
		test_low_memory_start = v;
	v = 0;
	if (ini_getvalx(ini, sections, _T("test_low_memory_end"), &v))
		test_low_memory_end = v;

	test_high_memory_start = 0xffffffff;
	test_high_memory_end = 0xffffffff;
	v = 0;
	if (ini_getvalx(ini, sections, _T("test_high_memory_start"), &v))
		test_high_memory_start = v;
	v = 0;
	if (ini_getvalx(ini, sections, _T("test_high_memory_end"), &v))
		test_high_memory_end = v;

	if ((addressing_mask == 0xffffffff && test_high_memory_end <= 0x01000000) || test_high_memory_start == 0xffffffff || test_high_memory_end == 0xffffffff) {
		test_high_memory_start = 0xffffffff;
		test_high_memory_end = 0xffffffff;
		high_memory_size = 0xffffffff;
	}

	if (safe_memory_start != 0xffffffff) {
		int err = check_safe_memory(test_memory_start, test_memory_end);
		err += check_safe_memory(test_low_memory_start, test_low_memory_end);
		err += check_safe_memory(test_high_memory_start, test_high_memory_end);
		if (err == 3) {
			wprintf(_T("Safe_memory outside of all test memory regions!\n"));
			return 0;
		}
	}

	test_memory = (uae_u8 *)calloc(1, test_memory_size + EXTRA_RESERVED_SPACE);
	test_memory_temp = (uae_u8 *)calloc(1, test_memory_size);
	if (!test_memory || !test_memory_temp) {
		wprintf(_T("Couldn't allocate test memory\n"));
		return 0;
	}

	v = 0;
	if (ini_getvalx(ini, sections, _T("opcode_memory_start"), &v)) {
		if (v == -1) {
			opcode_memory = test_memory + test_memory_size / 2;
			opcode_memory_start = test_memory_start + test_memory_size / 2;
		} else {
			opcode_memory_start = v;
			opcode_memory = test_memory + (opcode_memory_start - test_memory_start);
			if (opcode_memory_start < test_memory_start || opcode_memory_start + OPCODE_AREA >= test_memory_end) {
				wprintf(_T("Opcode memory out of bounds, using default location.\n"));
				opcode_memory = test_memory + test_memory_size / 2;
				opcode_memory_start = test_memory_start + test_memory_size / 2;
			}
		}
	} else {
		opcode_memory = test_memory + test_memory_size / 2;
		opcode_memory_start = test_memory_start + test_memory_size / 2;
	}
	if (opcode_memory_start < opcode_memory_start || opcode_memory_start > test_memory_start + test_memory_size - 256) {
		wprintf(_T("Opcode memory out of bounds\n"));
		return 0;
	}
	if (ini_getvalx(ini, sections, _T("feature_stack_memory"), &v)) {
		super_stack_memory = v;
		user_stack_memory = super_stack_memory - (RESERVED_SUPERSTACK + RESERVED_USERSTACK_EXTRA);
	} else {
		super_stack_memory = test_memory_start + (2 * RESERVED_SUPERSTACK + RESERVED_USERSTACK_EXTRA);
		user_stack_memory = test_memory_start + RESERVED_SUPERSTACK;
	}
	user_stack_memory_use = user_stack_memory;
	if (feature_usp == 1 || feature_usp == 2) {
		user_stack_memory_use |= 1;
	}

	storage_buffer_watermark_size = 200000;
	max_storage_buffer = 1000000;
	ini_getvalx(ini, sections, _T("buffer_size"), &max_storage_buffer);
	ini_getvalx(ini, sections, _T("watermark"), &storage_buffer_watermark_size);

	if (test_low_memory_start == 0xffffffff || test_low_memory_end == 0xffffffff) {
		low_memory_size = 0xffffffff;
		test_low_memory_start = 0xffffffff;
		test_low_memory_end = 0xffffffff;
	} else {
		low_memory_size = test_low_memory_end;
		if (low_memory_size < 0x8000)
			low_memory_size = 0x8000;
	}
	if (low_memory_size != 0xffffffff) {
		low_memory = (uae_u8 *)calloc(1, low_memory_size);
		low_memory_temp = (uae_u8 *)calloc(1, low_memory_size);
	}
	if (high_memory_size != 0xffffffff) {
		high_memory = (uae_u8 *)calloc(1, high_memory_size);
		high_memory_temp = (uae_u8 *)calloc(1, high_memory_size);
	}

	fill_memory();

	if (test_low_memory_start != 0xffffffff) {
		TCHAR *lmem_rom_name = NULL;
		ini_getstringx(ini, sections, _T("low_rom"), &lmem_rom_name);
		if (lmem_rom_name) {
			if (load_file(NULL, lmem_rom_name, low_memory_temp, low_memory_size, 0)) {
				wprintf(_T("Low test memory ROM loaded\n"));
				lmem_rom = 1;
			}
		}
		free(lmem_rom_name);
		save_memory(path, _T("lmem.dat"), low_memory_temp, low_memory_size);
	} else {
		deletefile(path, _T("lmem"));
	}

	if (test_high_memory_start != 0xffffffff) {
		TCHAR *hmem_rom_name = NULL;
		ini_getstringx(ini, sections, _T("high_rom"), &hmem_rom_name);
		if (hmem_rom_name) {
			if (load_file(NULL, hmem_rom_name, high_memory_temp, high_memory_size, -1)) {
				wprintf(_T("High test memory ROM loaded\n"));
				hmem_rom = 1;
			}
		}
		free(hmem_rom_name);
		save_memory(path, _T("hmem.dat"), high_memory_temp, high_memory_size);
	} else {
		deletefile(path, _T("hmem"));
	}

	save_memory(path, _T("tmem.dat"), test_memory_temp, test_memory_size);

	storage_buffer = (uae_u8 *)calloc(max_storage_buffer + storage_buffer_watermark_size, 1);
	// FMOVEM stores can use lots of memory
	storage_buffer_watermark = max_storage_buffer - storage_buffer_watermark_size;

	for (int i = 0; i < 256; i++) {
		int j;
		for (j = 0; j < 8; j++) {
			if (i & (1 << j)) break;
		}
		movem_index1[i] = j;
		movem_index2[i] = 7 - j;
		movem_next[i] = i & (~(1 << j));
	}

	init_table68k();

	if (currprefs.cpu_model == 68000) {
		tbl = op_smalltbl_90_test_ff;
		cpu_lvl = 0;
	} else if (currprefs.cpu_model == 68010) {
		tbl = op_smalltbl_91_test_ff;
		cpu_lvl = 1;
	} else if (currprefs.cpu_model == 68020) {
		tbl = op_smalltbl_92_test_ff;
		cpu_lvl = 2;
	} else if (currprefs.cpu_model == 68030) {
		tbl = op_smalltbl_93_test_ff;
		cpu_lvl = 3;
	} else if (currprefs.cpu_model == 68040 ) {
		tbl = op_smalltbl_94_test_ff;
		cpu_lvl = 4;
	} else if (currprefs.cpu_model == 68060) {
		tbl = op_smalltbl_95_test_ff;
		cpu_lvl = 5;
	} else {
		wprintf(_T("Unsupported CPU model.\n"));
		abort();
	}

	for (int opcode = 0; opcode < 65536; opcode++) {
		cpufunctbl[opcode] = op_illg_1;
		cpufunctbl_noret[opcode] = op_illg_1_noret;
	}
	for (int i = 0; tbl[i].handler_ff != NULL || tbl[i].handler_ff_noret != NULL; i++) {
		int opcode = tbl[i].opcode;
		cpufunctbl[opcode] = tbl[i].handler_ff;
		cpufunctbl_noret[opcode] = tbl[i].handler_ff_noret;
		cpudatatbl[opcode].length = tbl[i].length;
		cpudatatbl[opcode].disp020[0] = tbl[i].disp020[0];
		cpudatatbl[opcode].disp020[1] = tbl[i].disp020[1];
		cpudatatbl[opcode].branch = tbl[i].branch;
	}

	for (int opcode = 0; opcode < 65536; opcode++) {
		instr *table = &table68k[opcode];

		if (table->mnemo == i_ILLG)
			continue;

		/* unimplemented opcode? */
		if (table->unimpclev > 0 && cpu_lvl >= table->unimpclev) {
			if (currprefs.cpu_model == 68060) {
				// remove unimplemented integer instructions
				// unimpclev == 5: not implemented in 68060,
				// generates unimplemented instruction exception.
				if (currprefs.int_no_unimplemented && table->unimpclev == 5) {
					cpufunctbl[opcode] = op_unimpl_1;
					cpufunctbl_noret[opcode] = op_unimpl_1_noret;
					continue;
				}
				// remove unimplemented instruction that were removed in previous models,
				// generates normal illegal instruction exception.
				// unimplclev < 5: instruction was removed in 68040 or previous model.
				// clev=4: implemented in 68040 or later. unimpclev=5: not in 68060
				if (table->unimpclev < 5 || (table->clev == 4 && table->unimpclev == 5)) {
					cpufunctbl[opcode] = op_illg_1;
					cpufunctbl_noret[opcode] = op_illg_1_noret;
					continue;
				}
			} else {
				cpufunctbl_noret[opcode] = op_illg_1_noret;
				continue;
			}
		}

		if (table->clev > cpu_lvl) {
			continue;
		}

		if (!currprefs.fpu_model) {
			int fppskip = isfpp(table->mnemo);
			if (fppskip)
				continue;
		}

		if (table->handler != -1) {
			int idx = table->handler;
			if (cpufunctbl[idx] == op_illg_1 || cpufunctbl_noret[idx] == op_illg_1_noret)
				abort();
			cpufunctbl[opcode] = cpufunctbl[idx];
			cpufunctbl_noret[opcode] = cpufunctbl_noret[idx];
			memcpy(&cpudatatbl[opcode], &cpudatatbl[idx], sizeof(struct cputbl_data));
		}

		if (opcode_loop_mode(opcode)) {
			loop_mode_table[opcode] = cpufunctbl[opcode];
		}

	}

	x_get_long = get_long_test;
	x_get_word = get_word_test;
	x_get_byte = get_byte_test;
	x_put_long = put_long_test;
	x_put_word = put_word_test;
	x_put_byte = put_byte_test;

	x_next_iword = next_iword_test;
	x_cp_next_iword = next_iword_test;
	x_next_ilong = next_ilong_test;
	x_cp_next_ilong = next_ilong_test;

	x_cp_get_disp_ea_020 = x_get_disp_ea_020;

	x_cp_get_long = get_long_test;
	x_cp_get_word = get_word_test;
	x_cp_get_byte = get_byte_test;
	x_cp_put_long = put_long_test;
	x_cp_put_word = put_word_test;
	x_cp_put_byte = put_byte_test;

	fpu_reset();

	starttime = time(0);

	if (!mode) {
		wprintf(_T("Mode must be 'all', 'fall', 'branch', 'branchj', 'branchs' or '<mnemonic>'\n"));
		return 0;
	}

	TCHAR *modep = mode;
	while(modep) {

		int all = 0;

		if (!_tcsicmp(mode, _T("all"))) {

			if (verbose == 1)
				verbose = 0;
			for (int j = 1; lookuptab[j].name[0]; j++) {
				test_mnemo_text(path, lookuptab[j].name);
			}
			// Illegal instructions last. All currently selected CPU model's unsupported opcodes
			// (Generates illegal instruction, a-line or f-line exception)
			test_mnemo_text(path, _T("ILLEGAL"));
			break;
		}

		if (!_tcsicmp(mode, _T("branch")) || !_tcsicmp(mode, _T("branchj")) || !_tcsicmp(mode, _T("branchs"))) {

			static const TCHAR *branchs[] = {
				_T("RTS"), _T("RTD"), _T("RTR"), _T("RTE"), _T("JSR"), _T("BSR"), NULL
			};
			static const TCHAR *branchj[] = {
				_T("DBcc"), _T("Bcc"), _T("JMP"), _T("FDBcc"), _T("FBcc"), NULL
			};
			if (!_tcsicmp(mode, _T("branch")) || !_tcsicmp(mode, _T("branchj"))) {
				for (int i = 0; branchj[i]; i++) {
					test_mnemo_text(path, branchj[i]);
				}
			}
			if (!_tcsicmp(mode, _T("branch")) || !_tcsicmp(mode, _T("branchs"))) {
				for (int i = 0; branchs[i]; i++) {
					test_mnemo_text(path, branchs[i]);
				}
			}
			break;
		}

		if (!_tcsicmp(mode, _T("fall"))) {

			if (verbose == 1)
				verbose = 0;
			test_mnemo_text(path, _T("FMOVECR"));
			TCHAR prev[100];
			prev[0] = 0;
			TCHAR namebuf[100];
			for (int j = 0; j < 128; j++) {
				const TCHAR *name = fpuopcodes[j];
				if (name == NULL) {
					// test also one unsupported opcode
					if (j == 0x3f) {
						_stprintf(namebuf, _T("FPP%02X"), j);
						name = namebuf;
					}
				}
				if (name && _tcscmp(prev, name)) {
					test_mnemo_text(path, name);
					_tcscpy(prev, name);
				}
			}
			for (int j = 1; lookuptab[j].name; j++) {
				if (lookuptab[j].name[0] == 'F' && _tcscmp(lookuptab[j].name, _T("FPP"))) {
					test_mnemo_text(path, lookuptab[j].name);
				}
			}
			test_mnemo_text(path, _T("FMOVEM"));
			break;
		}


		TCHAR *sp = _tcschr(modep, ',');
		if (sp)
			*sp++ = 0;

		test_mnemo_text(path, modep);

		modep = sp;
	}

	xfree(low_memory);
	xfree(low_memory_temp);
	xfree(high_memory);
	xfree(high_memory_temp);
	xfree(test_memory);
	xfree(test_memory_temp);
	xfree(storage_buffer);

	wprintf(_T("%d total tests generated\n"), test_count);

	return 1;
}

static TCHAR sections[1000];

int __cdecl main(int argc, char *argv[])
{
	struct ini_data *ini = ini_load(_T("cputestgen.ini"), false);
	if (!ini) {
		wprintf(_T("Couldn't open cputestgen.ini\n"));
		return 0;
	}

	disasm_init();

	TCHAR *sptr = sections;
	_tcscpy(sections, INISECTION);
	sptr += _tcslen(sptr) + 1;

	int cpu;
	if (!ini_getval(ini, INISECTION, _T("cpu"), &cpu)) {
		wprintf(_T("CPU model is not set\n"));
		return 0;
	}

	int cpuidx = cputoindex(cpu);
	if (cpuidx < 0) {
		wprintf(_T("Unsupported CPU model\n"));
		return 0;
	}
	maincpu[cpuidx] = 1;
	currprefs.cpu_model = cpu;

	int idx = 0;
	for (;;) {
		TCHAR *section = NULL;
		if (!ini_getsection(ini, idx, &section))
			break;
		if (!_tcsnicmp(section, _T("test="), 5)) {
			_tcscpy(sptr, section);
			TCHAR testname[1000];
			_tcscpy(testname, section + 5);
			const TCHAR *p = _tcschr(testname, '|');
			if (p) {
				testname[p - testname] = 0;
			}
			if (!test(ini, sections, testname))
				break;
		}
		idx++;
	}

}
