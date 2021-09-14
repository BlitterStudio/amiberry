#ifndef UAE_CPU_PREFETCH_H
#define UAE_CPU_PREFETCH_H

#include "uae/types.h"

#ifdef CPUEMU_20

extern uae_u32 get_word_020_prefetch (int);
extern void continue_020_prefetch(void);

STATIC_INLINE uae_u32 next_iword_020_prefetch (void)
{
	uae_u32 r = get_word_020_prefetch (0);
	m68k_incpci (2);
	return r;
}
STATIC_INLINE uae_u32 next_ilong_020_prefetch (void)
{
	uae_u32 r = next_iword_020_prefetch () << 16;
	r |= next_iword_020_prefetch ();
	return r;
}

STATIC_INLINE uae_u32 get_long_020_prefetch (int o)
{
	uae_u32 r = get_word_020_prefetch (o) << 16;
	r |= get_word_020_prefetch (o + 2);
	return r;
}

#endif

#ifdef CPUEMU_21

STATIC_INLINE void limit_cycles_ce020(int clocks)
{
#if 0
	int cycs = clocks * cpucycleunit;
	int diff = regs.ce020endcycle - regs.ce020startcycle;
	if (diff <= cycs)
		return;
	regs.ce020startcycle = regs.ce020endcycle - cycs;
#endif
}

STATIC_INLINE void limit_all_cycles_ce020(void)
{
#if 0
	regs.ce020startcycle = regs.ce020endcycle;
#endif
}

// only for CPU internal cycles
STATIC_INLINE void do_cycles_ce020_internal(int clocks)
{
	if (currprefs.m68k_speed < 0) {
		regs.ce020extracycles += clocks;
		return;
	}
	int cycs = clocks * cpucycleunit;
	int diff = regs.ce020endcycle - regs.ce020startcycle;
	if (diff > 0) {
		if (diff >= cycs) {
			regs.ce020startcycle += cycs;
			return;
		}
		regs.ce020startcycle = regs.ce020endcycle;
		cycs -= diff;
	}
#if 0
	if (regs.ce020memcycles > 0) {
		if (regs.ce020memcycles >= cycs) {
			regs.ce020memcycles -= cycs;
			return;
		}
		cycs = cycs - regs.ce020memcycles;
	}
	regs.ce020memcycles = 0;
#endif
	x_do_cycles (cycs);
}

STATIC_INLINE void do_cycles_020_internal(int clocks)
{
	if (currprefs.m68k_speed < 0)
		return;
	x_do_cycles(clocks * cpucycleunit);
}

STATIC_INLINE void do_cycles_ce020_mem (int clocks, uae_u32 val)
{
	x_do_cycles_post (clocks * cpucycleunit, val);
}

#if 0
STATIC_INLINE void do_head_cycles_ce020 (int h)
{
	if (regs.ce020_tail) {
		int cycs = regs.ce020_tail_cycles - get_cycles ();
		if (cycs < 0)
			cycs = 0;
		cycs -= h * cpucycleunit;
		if (cycs)
			x_do_cycles (cycs < 0 ? -cycs : cycs);
	} else if (h > 0) {
		do_cycles_ce020 (h);
	}
}
#endif

STATIC_INLINE uae_u32 get_long_ce020 (uaecptr addr)
{
	return mem_access_delay_long_read_ce020 (addr);
}
STATIC_INLINE uae_u32 get_word_ce020 (uaecptr addr)
{
	return mem_access_delay_word_read_ce020 (addr);
}
STATIC_INLINE uae_u32 get_byte_ce020 (uaecptr addr)
{
	return mem_access_delay_byte_read_ce020 (addr);
}

STATIC_INLINE void put_long_ce020 (uaecptr addr, uae_u32 v)
{
	mem_access_delay_long_write_ce020 (addr, v);
}
STATIC_INLINE void put_word_ce020 (uaecptr addr, uae_u32 v)
{
	mem_access_delay_word_write_ce020 (addr, v);
}
STATIC_INLINE void put_byte_ce020 (uaecptr addr, uae_u32 v)
{
	mem_access_delay_byte_write_ce020 (addr, v);
}

extern void continue_ce020_prefetch(void);
extern uae_u32 get_word_ce020_prefetch(int);
extern uae_u32 get_word_ce020_prefetch_opcode(int);

STATIC_INLINE uae_u32 get_long_ce020_prefetch (int o)
{
	uae_u32 v;
	uae_u16 tmp;
	v = get_word_ce020_prefetch (o) << 16;
	tmp = regs.db;
	v |= get_word_ce020_prefetch (o + 2);
	regs.db = tmp;
	return v;
}

STATIC_INLINE uae_u32 next_iword_020ce (void)
{
	uae_u32 r = get_word_ce020_prefetch (0);
	m68k_incpci (2);
	return r;
}
STATIC_INLINE uae_u32 next_ilong_020ce (void)
{
	uae_u32 r = get_long_ce020_prefetch (0);
	m68k_incpci (4);
	return r;
}

STATIC_INLINE void m68k_do_bsr_ce020 (uaecptr oldpc, uae_s32 offset)
{
	m68k_areg (regs, 7) -= 4;
	x_put_long (m68k_areg (regs, 7), oldpc);
	m68k_incpci (offset);
}
STATIC_INLINE void m68k_do_rts_ce020 (void)
{
	m68k_setpci (x_get_long (m68k_areg (regs, 7)));
	m68k_areg (regs, 7) += 4;
}


#endif

#ifdef CPUEMU_22

extern void continue_030_prefetch(void);
extern uae_u32 get_word_030_prefetch(int);

STATIC_INLINE void put_long_030(uaecptr addr, uae_u32 v)
{
	write_data_030_lput(addr, v);
}
STATIC_INLINE void put_word_030(uaecptr addr, uae_u32 v)
{
	write_data_030_wput(addr, v);
}
STATIC_INLINE void put_byte_030(uaecptr addr, uae_u32 v)
{
	write_data_030_bput(addr, v);
}
STATIC_INLINE uae_u32 get_long_030(uaecptr addr)
{
	return read_data_030_lget(addr);
}
STATIC_INLINE uae_u32 get_word_030(uaecptr addr)
{
	return read_data_030_wget(addr);
}
STATIC_INLINE uae_u32 get_byte_030(uaecptr addr)
{
	return read_data_030_bget(addr);
}

STATIC_INLINE uae_u32 get_long_030_prefetch(int o)
{
	uae_u32 v;
	v = get_word_030_prefetch(o) << 16;
	v |= get_word_030_prefetch(o + 2);
	return v;
}

STATIC_INLINE uae_u32 next_iword_030_prefetch(void)
{
	uae_u32 r = get_word_030_prefetch(0);
	m68k_incpci(2);
	return r;
}
STATIC_INLINE uae_u32 next_ilong_030_prefetch(void)
{
	uae_u32 r = get_long_030_prefetch(0);
	m68k_incpci(4);
	return r;
}

STATIC_INLINE void m68k_do_bsr_030(uaecptr oldpc, uae_s32 offset)
{
	m68k_areg(regs, 7) -= 4;
	put_long_030(m68k_areg(regs, 7), oldpc);
	m68k_incpci(offset);
}
STATIC_INLINE void m68k_do_rts_030(void)
{
	m68k_setpc(get_long_030(m68k_areg(regs, 7)));
	m68k_areg(regs, 7) += 4;
}

#endif

#ifdef CPUEMU_23

extern void continue_ce030_prefetch(void);
extern uae_u32 get_word_ce030_prefetch(int);
extern uae_u32 get_word_ce030_prefetch_opcode(int);

STATIC_INLINE uae_u32 get_long_ce030 (uaecptr addr)
{
	return mem_access_delay_long_read_ce020 (addr);
}
STATIC_INLINE uae_u32 get_word_ce030 (uaecptr addr)
{
	return mem_access_delay_word_read_ce020 (addr);
}
STATIC_INLINE uae_u32 get_byte_ce030 (uaecptr addr)
{
	return mem_access_delay_byte_read_ce020 (addr);
}

STATIC_INLINE void put_long_ce030 (uaecptr addr, uae_u32 v)
{
	mem_access_delay_long_write_ce020 (addr, v);
}
STATIC_INLINE void put_word_ce030 (uaecptr addr, uae_u32 v)
{
	mem_access_delay_word_write_ce020 (addr, v);
}
STATIC_INLINE void put_byte_ce030 (uaecptr addr, uae_u32 v)
{
	mem_access_delay_byte_write_ce020 (addr, v);
}


STATIC_INLINE void put_long_dc030 (uaecptr addr, uae_u32 v)
{
	write_dcache030_lput(addr, v, (regs.s ? 4 : 0) | 1);
}
STATIC_INLINE void put_word_dc030 (uaecptr addr, uae_u32 v)
{
	write_dcache030_wput(addr, v, (regs.s ? 4 : 0) | 1);
}
STATIC_INLINE void put_byte_dc030 (uaecptr addr, uae_u32 v)
{
	write_dcache030_bput(addr, v, (regs.s ? 4 : 0) | 1);
}
STATIC_INLINE uae_u32 get_long_dc030 (uaecptr addr)
{
	return read_dcache030_lget(addr, (regs.s ? 4 : 0) | 1);
}
STATIC_INLINE uae_u32 get_word_dc030 (uaecptr addr)
{
	return read_dcache030_wget(addr, (regs.s ? 4 : 0) | 1);
}
STATIC_INLINE uae_u32 get_byte_dc030 (uaecptr addr)
{
	return read_dcache030_bget(addr, (regs.s ? 4 : 0) | 1);
}

STATIC_INLINE uae_u32 get_long_ce030_prefetch (int o)
{
	uae_u32 v;
	v = get_word_ce030_prefetch (o) << 16;
	v |= get_word_ce030_prefetch (o + 2);
	return v;
}

STATIC_INLINE uae_u32 next_iword_030ce (void)
{
	uae_u32 r = get_word_ce030_prefetch (0);
	m68k_incpci (2);
	return r;
}
STATIC_INLINE uae_u32 next_ilong_030ce (void)
{
	uae_u32 r = get_long_ce030_prefetch (0);
	m68k_incpci (4);
	return r;
}

STATIC_INLINE void m68k_do_bsr_ce030 (uaecptr oldpc, uae_s32 offset)
{
	m68k_areg (regs, 7) -= 4;
	x_put_long (m68k_areg (regs, 7), oldpc);
	m68k_incpci (offset);
}
STATIC_INLINE void m68k_do_rts_ce030 (void)
{
	m68k_setpc (x_get_long (m68k_areg (regs, 7)));
	m68k_areg (regs, 7) += 4;
}

#endif

#ifdef CPUEMU_11

STATIC_INLINE uae_u32 get_word_000_prefetch(int o)
{
	uae_u32 v = regs.irc;
	regs.irc = regs.read_buffer = regs.db = get_wordi (m68k_getpci () + o);
	return v;
}
STATIC_INLINE uae_u32 get_byte_000(uaecptr addr)
{
	uae_u32 v = get_byte (addr);
	regs.db = (v << 8) | v;
	regs.read_buffer = v;
	return v;
}
STATIC_INLINE uae_u32 get_word_000(uaecptr addr)
{
	uae_u32 v = get_word (addr);
	regs.db = v;
	regs.read_buffer = v;
	return v;
}
STATIC_INLINE void put_byte_000(uaecptr addr, uae_u32 v)
{
	regs.db = (v << 8) | v;
	regs.write_buffer = v;
	put_byte (addr, v);
}
STATIC_INLINE void put_word_000(uaecptr addr, uae_u32 v)
{
	regs.db = v;
	regs.write_buffer = v;
	put_word (addr, v);
}
#endif

#ifdef CPUEMU_13

STATIC_INLINE void do_cycles_ce000_internal(int clocks)
{
	if (currprefs.m68k_speed < 0)
		return;
	x_do_cycles (clocks * CYCLE_UNIT / 2);
}
STATIC_INLINE void do_cycles_ce000 (int clocks)
{
	x_do_cycles (clocks * CYCLE_UNIT / 2);
}

STATIC_INLINE void ipl_fetch (void)
{
	regs.ipl = regs.ipl_pin;
}

uae_u32 mem_access_delay_word_read (uaecptr addr);
uae_u32 mem_access_delay_wordi_read (uaecptr addr);
uae_u32 mem_access_delay_byte_read (uaecptr addr);
void mem_access_delay_byte_write (uaecptr addr, uae_u32 v);
void mem_access_delay_word_write (uaecptr addr, uae_u32 v);

STATIC_INLINE uae_u32 get_long_ce000 (uaecptr addr)
{
	uae_u32 v = mem_access_delay_word_read (addr) << 16;
	v |= mem_access_delay_word_read (addr + 2);
	return v;
}
STATIC_INLINE uae_u32 get_word_ce000 (uaecptr addr)
{
	return mem_access_delay_word_read (addr);
}
STATIC_INLINE uae_u32 get_wordi_ce000 (int offset)
{
	return mem_access_delay_wordi_read (m68k_getpci () + offset);
}
STATIC_INLINE uae_u32 get_byte_ce000 (uaecptr addr)
{
	return mem_access_delay_byte_read (addr);
}
STATIC_INLINE uae_u32 get_word_ce000_prefetch (int o)
{
	uae_u32 v = regs.irc;
	regs.irc = regs.read_buffer = regs.db = x_get_iword (o);
	return v;
}

STATIC_INLINE void put_long_ce000 (uaecptr addr, uae_u32 v)
{
	mem_access_delay_word_write (addr, v >> 16);
	mem_access_delay_word_write (addr + 2, v);
}
STATIC_INLINE void put_word_ce000 (uaecptr addr, uae_u32 v)
{
	mem_access_delay_word_write (addr, v);
}
STATIC_INLINE void put_byte_ce000 (uaecptr addr, uae_u32 v)
{
	mem_access_delay_byte_write (addr, v);
}

#endif

STATIC_INLINE uae_u32 get_disp_ea_000 (uae_u32 base, uae_u32 dp)
{
	int reg = (dp >> 12) & 15;
	uae_s32 regd = regs.regs[reg];
	if ((dp & 0x800) == 0)
		regd = (uae_s32)(uae_s16)regd;
	return base + (uae_s8)dp + regd;
}

#endif /* UAE_CPU_PREFETCH_H */
