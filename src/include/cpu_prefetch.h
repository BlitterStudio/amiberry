#ifndef UAE_CPU_PREFETCH_H
#define UAE_CPU_PREFETCH_H

#include "uae/types.h"

STATIC_INLINE uae_u32 get_word_000_prefetch (int o)
{
	uae_u32 v = regs.irc;
	regs.irc = regs.db = get_wordi (m68k_getpci () + o);
	return v;
}
STATIC_INLINE uae_u32 get_byte_000(uaecptr addr)
{
	uae_u32 v = get_byte (addr);
	regs.db = (v << 8) | v;
	return v;
}
STATIC_INLINE uae_u32 get_word_000(uaecptr addr)
{
	uae_u32 v = get_word (addr);
	regs.db = v;
	return v;
}
STATIC_INLINE void put_byte_000(uaecptr addr, uae_u32 v)
{
	regs.db = (v << 8) | v;
	put_byte (addr, v);
}
STATIC_INLINE void put_word_000(uaecptr addr, uae_u32 v)
{
	regs.db = v;
	put_word (addr, v);
}

STATIC_INLINE uae_u32 get_disp_ea_000 (uae_u32 base, uae_u32 dp)
{
  int reg = (dp >> 12) & 15;
  uae_s32 regd = regs.regs[reg];
  if ((dp & 0x800) == 0)
  	regd = (uae_s32)(uae_s16)regd;
  return base + (uae_s8)dp + regd;
}

#endif /* UAE_CPU_PREFETCH_H */
