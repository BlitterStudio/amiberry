
STATIC_INLINE uae_u32 get_word_prefetch (struct regstruct &regs, int o)
{
  uae_u32 v = regs.irc;
	regs.irc = get_wordi (m68k_getpc () + o);
  return v;
}
STATIC_INLINE uae_u32 get_long_prefetch (struct regstruct &regs, int o)
{
	uae_u32 v = get_word_prefetch (regs, o) << 16;
	v |= get_word_prefetch (regs, o + 2);
  return v;
}

STATIC_INLINE uae_u32 get_disp_ea_000 (struct regstruct &regs, uae_u32 base, uae_u32 dp)
{
  int reg = (dp >> 12) & 15;
  uae_s32 regd = regs.regs[reg];
  if ((dp & 0x800) == 0)
  	regd = (uae_s32)(uae_s16)regd;
  return base + (uae_s8)dp + regd;
}
