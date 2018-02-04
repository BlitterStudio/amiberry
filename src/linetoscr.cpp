/* Note:
 *    p_xcolors[] contains 16-bit color information in both words
 *    p_acolors contains 16-bit color information in both words
 */

STATIC_INLINE uae_u32 merge_words(uae_u32 val, uae_u32 val2)
{
  __asm__ (
			"pkhbt   %[o], %[o], %[d], lsl #16 \n\t"
      : [o] "+r" (val) : [d] "r" (val2) );
  return val;
}

STATIC_INLINE uae_u32 double_word(uae_u32 val)
{
  __asm__ (
			"pkhbt   %[o], %[o], %[o], lsl #16 \n\t"
      : [o] "+r" (val) );
  return val;
}
 
static int NOINLINE linetoscr_16 (int spix, int dpix, int dpix_end)
{
  uae_u16 *buf = (uae_u16 *) xlinebuffer;

  switch(bplmode)
  {
    case CMODE_NORMAL:
    {
        int rem;
        if (((uintptr_t)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            spix_val = pixdata.apixels[spix++];
            buf[dpix++] = p_acolors[spix_val];
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((uintptr_t)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix++];
            out_val = p_acolors[spix_val];
            spix_val = pixdata.apixels[spix++];
            dpix_val = p_acolors[spix_val];
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            spix_val = pixdata.apixels[spix++];
            buf[dpix++] = p_acolors[spix_val];
        }
    }
    break;
    case CMODE_HAM:
    {
        int rem;
        if (((uintptr_t)&buf[dpix]) & 2) {
            buf[dpix++] = ham_linebuf[spix++];
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((uintptr_t)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 out_val;
        
            out_val = *((uae_u32 *)&ham_linebuf[spix]);
            spix += 2;
            *((uae_u32 *)&buf[dpix]) = out_val;
            dpix += 2;
        }
        if (rem) {
            buf[dpix++] = ham_linebuf[spix++];
        }
    }
    break;
    case CMODE_DUALPF:
    {
        int *lookup = bpldualpfpri ? dblpf_ind2 : dblpf_ind1;
        int rem;
        if (((uintptr_t)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            spix_val = pixdata.apixels[spix++];
            buf[dpix++] = p_acolors[lookup[spix_val]];
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((uintptr_t)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix++];
            out_val = p_acolors[lookup[spix_val]];
            spix_val = pixdata.apixels[spix++];
            dpix_val = p_acolors[lookup[spix_val]];
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            spix_val = pixdata.apixels[spix++];
            buf[dpix++] = p_acolors[lookup[spix_val]];
        }
    }
    break;
    case CMODE_EXTRAHB:
    {
        int rem;
        if (((uintptr_t)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = pixdata.apixels[spix++];
            if (spix_val <= 31)
                dpix_val = p_acolors[spix_val];
            else
                dpix_val = p_xcolors[(colors_for_drawing.color_regs_ecs[spix_val - 32] >> 1) & 0x777];
            buf[dpix++] = dpix_val;
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((uintptr_t)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix++];
            if (spix_val <= 31)
                out_val = p_acolors[spix_val];
            else
                out_val = p_xcolors[(colors_for_drawing.color_regs_ecs[spix_val - 32] >> 1) & 0x777];
            spix_val = pixdata.apixels[spix++];
            if (spix_val <= 31)
                dpix_val = p_acolors[spix_val];
            else
                dpix_val = p_xcolors[(colors_for_drawing.color_regs_ecs[spix_val - 32] >> 1) & 0x777];
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = pixdata.apixels[spix++];
            if (spix_val <= 31)
                dpix_val = p_acolors[spix_val];
            else
                dpix_val = p_xcolors[(colors_for_drawing.color_regs_ecs[spix_val - 32] >> 1) & 0x777];
            buf[dpix++] = dpix_val;
        }
    }
    break;
    case CMODE_EXTRAHB_ECS_KILLEHB:
    {
        int rem;
        if (((uintptr_t)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            spix_val = pixdata.apixels[spix++];
            buf[dpix++] = p_acolors[spix_val & 31];
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((uintptr_t)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix++];
            out_val = p_acolors[spix_val & 31];
            spix_val = pixdata.apixels[spix++];
            dpix_val = p_acolors[spix_val & 31];
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            spix_val = pixdata.apixels[spix++];
            buf[dpix++] = p_acolors[spix_val & 31];
        }
    }
    break;
  }
  return spix;
}

static int NOINLINE linetoscr_16_stretch1 (int spix, int dpix, int dpix_end)
{
  uae_u16 *buf = (uae_u16 *) xlinebuffer;

  switch(bplmode)
  {
    case CMODE_NORMAL:
    {
        while (dpix < dpix_end) {
            uae_u32 spix_val;
        
            spix_val = pixdata.apixels[spix++];
            *((uae_u32 *)&buf[dpix]) = p_acolors[spix_val];
            dpix += 2;
        }
    }
    break;
    case CMODE_HAM:
    {
        while (dpix < dpix_end) {
            uae_u32 out_val = ham_linebuf[spix++];
            *((uae_u32 *)&buf[dpix]) = double_word(out_val);
            dpix += 2;
        }
    }
    break;
    case CMODE_DUALPF:
    {
        int *lookup = bpldualpfpri ? dblpf_ind2 : dblpf_ind1;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
        
            spix_val = pixdata.apixels[spix++];
            *((uae_u32 *)&buf[dpix]) = p_acolors[lookup[spix_val]];
            dpix += 2;
        }
    }
    break;
    case CMODE_EXTRAHB:
    {
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix++];
            if (spix_val <= 31)
              out_val = p_acolors[spix_val];
            else
              out_val = p_xcolors[(colors_for_drawing.color_regs_ecs[spix_val - 32] >> 1) & 0x0777];
              *((uae_u32 *)&buf[dpix]) = out_val;
            dpix += 2;
        }
    }
    break;
    case CMODE_EXTRAHB_ECS_KILLEHB:
    {
        while (dpix < dpix_end) {
            uae_u32 spix_val;
        
            spix_val = pixdata.apixels[spix++];
            *((uae_u32 *)&buf[dpix]) = p_acolors[spix_val & 31];
            dpix += 2;
        }
    }
    break;
  }

  return spix;
}

static int NOINLINE linetoscr_16_shrink1 (int spix, int dpix, int dpix_end)
{
  uae_u16 *buf = (uae_u16 *) xlinebuffer;

  switch(bplmode)
  {
    case CMODE_NORMAL:
    {
        int rem;
        if (((uintptr_t)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            spix_val = pixdata.apixels[spix];
            spix += 2;
            buf[dpix++] = p_acolors[spix_val];
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((uintptr_t)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            out_val = p_acolors[spix_val];
            spix += 2;
            spix_val = pixdata.apixels[spix];
            dpix_val = p_acolors[spix_val];
            spix += 2;
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            spix_val = pixdata.apixels[spix];
            spix += 2;
            buf[dpix++] = p_acolors[spix_val];
        }
    }
    break;
    case CMODE_HAM:
    {
        int rem;
        if (((uintptr_t)&buf[dpix]) & 2) {
            uae_u32 dpix_val;
            dpix_val = ham_linebuf[spix];
            spix += 2;
            buf[dpix++] = dpix_val;
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((uintptr_t)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            out_val = ham_linebuf[spix];
            spix += 2;
            dpix_val = ham_linebuf[spix];
            spix += 2;
            out_val = merge_words(out_val, dpix_val);

            *((uae_u32 *)&buf[dpix]) = out_val;
            dpix += 2;
        }
        if (rem) {
            uae_u32 dpix_val;
            dpix_val = ham_linebuf[spix];
            spix += 2;
            buf[dpix++] = dpix_val;
        }
    }
    break;
    case CMODE_DUALPF:
    {
        int *lookup = bpldualpfpri ? dblpf_ind2 : dblpf_ind1;
        int rem;
        if (((uintptr_t)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = pixdata.apixels[spix];
            dpix_val = p_acolors[lookup[spix_val]];
            spix += 2;
            buf[dpix++] = dpix_val;
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((uintptr_t)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            out_val = p_acolors[lookup[spix_val]];
            spix += 2;
            spix_val = pixdata.apixels[spix];
            dpix_val = p_acolors[lookup[spix_val]];
            spix += 2;
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = pixdata.apixels[spix];
            dpix_val = p_acolors[lookup[spix_val]];
            spix += 2;
            buf[dpix++] = dpix_val;
        }
    }
    break;
    case CMODE_EXTRAHB:
    {
        int rem;
        if (((uintptr_t)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = pixdata.apixels[spix];
            if (spix_val <= 31)
                dpix_val = p_acolors[spix_val];
            else
                dpix_val = p_xcolors[(colors_for_drawing.color_regs_ecs[spix_val - 32] >> 1) & 0x777];
            spix += 2;
            buf[dpix++] = dpix_val;
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((uintptr_t)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            if (spix_val <= 31)
                out_val = p_acolors[spix_val];
            else
                out_val = p_xcolors[(colors_for_drawing.color_regs_ecs[spix_val - 32] >> 1) & 0x777];
            spix += 2;
            spix_val = pixdata.apixels[spix];
            if (spix_val <= 31)
                dpix_val = p_acolors[spix_val];
            else
                dpix_val = p_xcolors[(colors_for_drawing.color_regs_ecs[spix_val - 32] >> 1) & 0x777];
            spix += 2;
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = pixdata.apixels[spix];
            if (spix_val <= 31)
                dpix_val = p_acolors[spix_val];
            else
                dpix_val = p_xcolors[(colors_for_drawing.color_regs_ecs[spix_val - 32] >> 1) & 0x777];
            spix += 2;
            buf[dpix++] = dpix_val;
        }
    }
    break;
    case CMODE_EXTRAHB_ECS_KILLEHB:
    {
        int rem;
        if (((uintptr_t)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            spix_val = pixdata.apixels[spix];
            spix += 2;
            buf[dpix++] = p_acolors[spix_val & 31];
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((uintptr_t)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            out_val = p_acolors[spix_val & 31];
            spix += 2;
            spix_val = pixdata.apixels[spix];
            dpix_val = p_acolors[spix_val & 31];
            spix += 2;
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            spix_val = pixdata.apixels[spix];
            spix += 2;
            buf[dpix++] = p_acolors[spix_val & 31];
        }
    }
    break;
  }

  return spix;
}

static int NOINLINE linetoscr_16_spr(int spix, int dpix, int dpix_end)
{
  uae_u16 *buf = (uae_u16 *) xlinebuffer;
  uae_u8 sprcol;

  switch(bplmode)
  {
    case CMODE_NORMAL:
    {
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            sprpix_val = spix_val;
            out_val = p_acolors[spix_val];
            spix++;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix, 0, sprpix_val, 0);
                if (sprcol) {
                    uae_u32 spcol = p_acolors[sprcol];
                    out_val = spcol;
                }
            }
            buf[dpix++] = out_val;
        }
    }
    break;
    case CMODE_HAM:
    {
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 out_val;
        
            out_val = ham_linebuf[spix];
            sprpix_val = pixdata.apixels[spix];
            spix++;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix, 0, sprpix_val, 0);
                if (sprcol) {
                    uae_u32 spcol = p_acolors[sprcol];
                    out_val = spcol;
                }
            }
            buf[dpix++] = out_val;
        }
    }
    break;
    case CMODE_DUALPF:
    {
        int *lookup = bpldualpfpri ? dblpf_ind2 : dblpf_ind1;
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            sprpix_val = spix_val;
            out_val = p_acolors[lookup[spix_val]];
            spix++;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix, 1, sprpix_val, 0);
                if (sprcol) {
                    uae_u32 spcol = p_acolors[sprcol];
                    out_val = spcol;
                }
            }
            buf[dpix++] = out_val;
        }
    }
    break;
    case CMODE_EXTRAHB:
    {
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            sprpix_val = spix_val;
            if (spix_val <= 31)
                out_val = p_acolors[spix_val];
            else
                out_val = p_xcolors[(colors_for_drawing.color_regs_ecs[spix_val - 32] >> 1) & 0x777];
            spix++;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix, 0, sprpix_val, 0);
                if (sprcol) {
                    uae_u32 spcol = p_acolors[sprcol];
                    out_val = spcol;
                }
            }
            buf[dpix++] = out_val;
        }
    }
    break;
    case CMODE_EXTRAHB_ECS_KILLEHB:
    {
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            sprpix_val = spix_val;
            out_val = p_acolors[spix_val & 31];
            spix++;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix, 0, sprpix_val, 0);
                if (sprcol) {
                    uae_u32 spcol = p_acolors[sprcol];
                    out_val = spcol;
                }
            }
            buf[dpix++] = out_val;
        }
    }
    break;
  }

  return spix;
}

static int NOINLINE linetoscr_16_stretch1_spr(int spix, int dpix, int dpix_end)
{
  uae_u16 *buf = (uae_u16 *) xlinebuffer;
  uae_u8 sprcol;

  switch(bplmode)
  {
    case CMODE_NORMAL:
    {
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            sprpix_val = spix_val;
            out_val = p_acolors[spix_val];
            spix++;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix, 0, sprpix_val, 0);
                if (sprcol) {
                    uae_u32 spcol = p_acolors[sprcol];
                    out_val = spcol;
                }
            }
            *((uae_u32 *)&buf[dpix]) = double_word(out_val);
            dpix += 2;
        }
    }
    break;
    case CMODE_HAM:
    {
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            out_val = ham_linebuf[spix];
            sprpix_val = pixdata.apixels[spix];
            spix++;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix, 0, sprpix_val, 0);
                if (sprcol) {
                    uae_u32 spcol = p_acolors[sprcol];
                    out_val = spcol;
                }
            }
            *((uae_u32 *)&buf[dpix]) = double_word(out_val);
            dpix += 2;
        }
    }
    break;
    case CMODE_DUALPF:
    {
        int *lookup = bpldualpfpri ? dblpf_ind2 : dblpf_ind1;
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            sprpix_val = spix_val;
            out_val = p_acolors[lookup[spix_val]];
            spix++;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix, 1, sprpix_val, 0);
                if (sprcol) {
                    uae_u32 spcol = p_acolors[sprcol];
                    out_val = spcol;
                }
            }
            *((uae_u32 *)&buf[dpix]) = double_word(out_val);
            dpix += 2;
        }
    }
    break;
    case CMODE_EXTRAHB:
    {
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            sprpix_val = spix_val;
            if (spix_val <= 31)
                out_val = p_acolors[spix_val];
            else
                out_val = p_xcolors[(colors_for_drawing.color_regs_ecs[spix_val - 32] >> 1) & 0x777];
            spix++;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix, 0, sprpix_val, 0);
                if (sprcol) {
                    uae_u32 spcol = p_acolors[sprcol];
                    out_val = spcol;
                }
            }
            *((uae_u32 *)&buf[dpix]) = double_word(out_val);
            dpix += 2;
        }
    }
    break;
    case CMODE_EXTRAHB_ECS_KILLEHB:
    {
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            sprpix_val = spix_val;
            out_val = p_acolors[spix_val & 31];
            spix++;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix, 0, sprpix_val, 0);
                if (sprcol) {
                    uae_u32 spcol = p_acolors[sprcol];
                    out_val = spcol;
                }
            }
            *((uae_u32 *)&buf[dpix]) = double_word(out_val);
            dpix += 2;
        }
    }
    break;
  }

  return spix;
}

static int NOINLINE linetoscr_16_shrink1_spr(int spix, int dpix, int dpix_end)
{
  uae_u16 *buf = (uae_u16 *) xlinebuffer;
  uae_u8 sprcol;

  switch(bplmode)
  {
    case CMODE_NORMAL:
    {
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            sprpix_val = spix_val;
            out_val = p_acolors[spix_val];
            spix += 2;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix, 0, sprpix_val, 0);
                if (sprcol) {
                    uae_u32 spcol = p_acolors[sprcol];
                    out_val = spcol;
                }
            }
            buf[dpix++] = out_val;
        }
    }
    break;
    case CMODE_HAM:
    {
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            out_val = ham_linebuf[spix];
            sprpix_val = pixdata.apixels[spix];
            spix += 2;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix, 0, sprpix_val, 0);
                if (sprcol) {
                    uae_u32 spcol = p_acolors[sprcol];
                    out_val = spcol;
                }
            }
            buf[dpix++] = out_val;
        }
    }
    break;
    case CMODE_DUALPF:
    {
        int *lookup = bpldualpfpri ? dblpf_ind2 : dblpf_ind1;
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            sprpix_val = spix_val;
            out_val = p_acolors[lookup[spix_val]];
            spix += 2;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix, 1, sprpix_val, 0);
                if (sprcol) {
                    uae_u32 spcol = p_acolors[sprcol];
                    out_val = spcol;
                }
            }
            buf[dpix++] = out_val;
        }
    }
    break;
    case CMODE_EXTRAHB:
    {
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            sprpix_val = spix_val;
            if (spix_val <= 31)
                out_val = p_acolors[spix_val];
            else
                out_val = p_xcolors[(colors_for_drawing.color_regs_ecs[spix_val - 32] >> 1) & 0x777];
            spix += 2;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix, 0, sprpix_val, 0);
                if (sprcol) {
                    uae_u32 spcol = p_acolors[sprcol];
                    out_val = spcol;
                }
            }
            buf[dpix++] = out_val;
        }
    }
    break;
    case CMODE_EXTRAHB_ECS_KILLEHB:
    {
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            sprpix_val = spix_val;
            out_val = p_acolors[spix_val & 31];
            spix += 2;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix, 0, sprpix_val, 0);
                if (sprcol) {
                    uae_u32 spcol = p_acolors[sprcol];
                    out_val = spcol;
                }
            }
            buf[dpix++] = out_val;
        }
    }
    break;
  }

  return spix;
}

#ifdef AGA
static int NOINLINE linetoscr_16_aga_spronly(int spix, int dpix, int dpix_end)
{
    uae_u16 *buf = (uae_u16 *) xlinebuffer;
    uae_u8 sprcol;

    while (dpix < dpix_end) {
        uae_u32 out_val;
    
        spix++;
        out_val = p_acolors[0];
        if (spritepixels[dpix].data) {
            sprcol = render_sprites (dpix, 0, 0, 1);
            if (sprcol) {
                out_val = p_acolors[sprcol];
            }
        }
        buf[dpix++] = out_val;
    }

    return spix;
}
#endif

#ifdef AGA
static int NOINLINE linetoscr_16_stretch1_aga_spronly(int spix, int dpix, int dpix_end)
{
    uae_u16 *buf = (uae_u16 *) xlinebuffer;
    uae_u8 sprcol;

    while (dpix < dpix_end) {
        uae_u32 out_val;
    
        spix++;
        out_val = p_acolors[0];
        {
        uae_u32 out_val1 = out_val;
        uae_u32 out_val2 = out_val;
        if (spritepixels[dpix + 0].data) {
            sprcol = render_sprites (dpix + 0, 0, 0, 1);
            if (sprcol) {
                out_val1 = p_acolors[sprcol];
            }
        }
        if (spritepixels[dpix + 1].data) {
            sprcol = render_sprites (dpix + 1, 0, 0, 1);
            if (sprcol) {
                out_val2 = p_acolors[sprcol];
            }
        }
        buf[dpix++] = out_val1;
        buf[dpix++] = out_val2;
        }
    }

    return spix;
}
#endif

#ifdef AGA
static int NOINLINE linetoscr_16_shrink1_aga_spronly(int spix, int dpix, int dpix_end)
{
    uae_u16 *buf = (uae_u16 *) xlinebuffer;
    uae_u8 sprcol;

    while (dpix < dpix_end) {
        uae_u32 out_val;
    
        spix++;
        out_val = p_acolors[0];
        if (spritepixels[dpix].data) {
            sprcol = render_sprites (dpix, 0, 0, 1);
            if (sprcol) {
                out_val = p_acolors[sprcol];
            }
        }
        buf[dpix++] = out_val;
    }

    return spix;
}
#endif

#ifdef AGA
static int NOINLINE linetoscr_16_shrink2_aga_spronly(int spix, int dpix, int dpix_end)
{
    uae_u16 *buf = (uae_u16 *) xlinebuffer;
    uae_u8 sprcol;

    while (dpix < dpix_end) {
        uae_u32 out_val;
    
        spix++;
        out_val = p_acolors[0];
        if (spritepixels[dpix].data) {
            sprcol = render_sprites (dpix, 0, 0, 1);
            if (sprcol) {
                out_val = p_acolors[sprcol];
            }
        }
        buf[dpix++] = out_val;
    }

    return spix;
}
#endif

#ifdef AGA
static int NOINLINE linetoscr_16_aga (int spix, int dpix, int dpix_end)
{
  uae_u16 *buf = (uae_u16 *) xlinebuffer;
  uae_u8 xor_val = bplxor;
  uae_u8 and_val = bpland;
  
  switch(bplmode)
  {
    case CMODE_NORMAL:
    {
        int rem;
        if (((uintptr_t)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            spix_val = (pixdata.apixels[spix++] ^ xor_val) & and_val;
            buf[dpix++] = p_acolors[spix_val];
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((uintptr_t)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = (pixdata.apixels[spix++] ^ xor_val) & and_val;
            out_val = p_acolors[spix_val];
            spix_val = (pixdata.apixels[spix++] ^ xor_val) & and_val;
            dpix_val = p_acolors[spix_val];
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            spix_val = (pixdata.apixels[spix++] ^ xor_val) & and_val;
            buf[dpix++] = p_acolors[spix_val];
        }
    }
    break;
    case CMODE_HAM:
    {
        int rem;
        if (((uintptr_t)&buf[dpix]) & 2) {
            buf[dpix++] = ham_linebuf[spix];
            spix++;
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((uintptr_t)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 out_val;
        
            out_val = *((uae_u32 *)&ham_linebuf[spix]);
            spix += 2;
            *((uae_u32 *)&buf[dpix]) = out_val;
            dpix += 2;
        }
        if (rem) {
            buf[dpix++] = ham_linebuf[spix];
            spix++;
        }
    }
    break;
    case CMODE_DUALPF:
    {
        int *lookup    = bpldualpfpri ? dblpf_ind2_aga : dblpf_ind1_aga;
        int *lookup_no = bpldualpfpri ? dblpf_2nd2     : dblpf_2nd1;
        int rem;
        if (((uintptr_t)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = pixdata.apixels[spix];
            uae_u8 val = lookup[spix_val];
            if (lookup_no[spix_val])
                val += dblpfofs[bpldualpf2of];
    	      val ^= xor_val;
            dpix_val = p_acolors[val];
            spix++;
            buf[dpix++] = dpix_val;
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((uintptr_t)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            uae_u8 val = lookup[spix_val];
            if (lookup_no[spix_val])
                val += dblpfofs[bpldualpf2of];
    	      val ^= xor_val;
            out_val = p_acolors[val];
            spix++;
            spix_val = pixdata.apixels[spix];
            val = lookup[spix_val];
            if (lookup_no[spix_val])
                val += dblpfofs[bpldualpf2of];
    	      val ^= xor_val;
            dpix_val = p_acolors[val];
            spix++;
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = pixdata.apixels[spix];
            uae_u8 val = lookup[spix_val];
            if (lookup_no[spix_val])
                val += dblpfofs[bpldualpf2of];
    	      val ^= xor_val;
            dpix_val = p_acolors[val];
            spix++;
            buf[dpix++] = dpix_val;
        }
    }
    break;
    case CMODE_EXTRAHB:
    {
        int rem;
        if (((uintptr_t)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            if (pixdata.apixels[spix] & 0x20) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val & 0x1f] >> 1) & 0x7F7F7F;
                dpix_val = CONVERT_RGB_16 (c);
            } else
                dpix_val = p_acolors[spix_val];
            spix++;
            buf[dpix++] = dpix_val;
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((uintptr_t)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            if (pixdata.apixels[spix] & 0x20) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val & 0x1f] >> 1) & 0x7F7F7F;
                out_val = CONVERT_RGB_16 (c);
            } else
                out_val = p_acolors[spix_val];
            spix++;
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            if (pixdata.apixels[spix] & 0x20) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val & 0x1f] >> 1) & 0x7F7F7F;
                dpix_val = CONVERT_RGB_16 (c);
            } else
                dpix_val = p_acolors[spix_val];
            spix++;
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            if (pixdata.apixels[spix] & 0x20) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val & 0x1f] >> 1) & 0x7F7F7F;
                dpix_val = CONVERT_RGB_16 (c);
            } else
                dpix_val = p_acolors[spix_val];
            spix++;
            buf[dpix++] = dpix_val;
        }
    }
    break;
  }

  return spix;
}
#endif

#ifdef AGA
static int NOINLINE linetoscr_16_stretch1_aga (int spix, int dpix, int dpix_end)
{
  uae_u16 *buf = (uae_u16 *) xlinebuffer;
  uae_u8 xor_val = bplxor;
  uae_u8 and_val = bpland;

  switch(bplmode)
  {
    case CMODE_NORMAL:
    {
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 out_val;
        
            spix_val = (pixdata.apixels[spix++] ^ xor_val) & and_val;
            out_val = p_acolors[spix_val];
            *((uae_u32 *)&buf[dpix]) = out_val;
            dpix += 2;
        }
    }
    break;
    case CMODE_HAM:
    {
        while (dpix < dpix_end) {
            uae_u32 out_val;
            out_val = ham_linebuf[spix++];
            *((uae_u32 *)&buf[dpix]) = double_word(out_val);
            dpix += 2;
        }
    }
    break;
    case CMODE_DUALPF:
    {
        int *lookup    = bpldualpfpri ? dblpf_ind2_aga : dblpf_ind1_aga;
        int *lookup_no = bpldualpfpri ? dblpf_2nd2     : dblpf_2nd1;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            uae_u8 val = lookup[spix_val];
            if (lookup_no[spix_val])
                val += dblpfofs[bpldualpf2of];
    	      val ^= xor_val;
            out_val = p_acolors[val];
            spix++;
            *((uae_u32 *)&buf[dpix]) = out_val;
            dpix += 2;
        }
    }
    break;
    case CMODE_EXTRAHB:
    {
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 out_val;
        
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            if (pixdata.apixels[spix] & 0x20) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val & 0x1f] >> 1) & 0x7F7F7F;
                out_val = CONVERT_RGB (c);
            } else
                out_val = p_acolors[spix_val];
            spix++;
            *((uae_u32 *)&buf[dpix]) = out_val;
            dpix += 2;
        }
    }
    break;
  }

  return spix;
}
#endif

#ifdef AGA
static int NOINLINE linetoscr_16_shrink1_aga (int spix, int dpix, int dpix_end)
{
  uae_u16 *buf = (uae_u16 *) xlinebuffer;
  uae_u8 xor_val = bplxor;
  uae_u8 and_val = bpland;

  switch(bplmode)
  {
    case CMODE_NORMAL:
    {
        int rem;
        if (((uintptr_t)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            spix += 2;
            buf[dpix++] = p_acolors[spix_val];
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((uintptr_t)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            out_val = p_acolors[spix_val];
            spix += 2;
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            dpix_val = p_acolors[spix_val];
            spix += 2;
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            spix += 2;
            buf[dpix++] = p_acolors[spix_val];
        }
    }
    break;
    case CMODE_HAM:
    {
        int rem;
        if (((uintptr_t)&buf[dpix]) & 2) {
            buf[dpix++] = ham_linebuf[spix];
            spix += 2;
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((uintptr_t)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            out_val = ham_linebuf[spix];
            spix += 2;
            dpix_val = ham_linebuf[spix];
            spix += 2;
            out_val = merge_words(out_val, dpix_val);
            *((uae_u32 *)&buf[dpix]) = out_val;
            dpix += 2;
        }
        if (rem) {
            buf[dpix++] = ham_linebuf[spix];
            spix += 2;
        }
    }
    break;
    case CMODE_DUALPF:
    {
        int *lookup    = bpldualpfpri ? dblpf_ind2_aga : dblpf_ind1_aga;
        int *lookup_no = bpldualpfpri ? dblpf_2nd2     : dblpf_2nd1;
        int rem;
        if (((uintptr_t)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = pixdata.apixels[spix];
            uae_u8 val = lookup[spix_val];
            if (lookup_no[spix_val])
                val += dblpfofs[bpldualpf2of];
    	      val ^= xor_val;
            dpix_val = p_acolors[val];
            spix += 2;
            buf[dpix++] = dpix_val;
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((uintptr_t)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            uae_u8 val = lookup[spix_val];
            if (lookup_no[spix_val])
                val += dblpfofs[bpldualpf2of];
    	      val ^= xor_val;
            out_val = p_acolors[val];
            spix += 2;
            spix_val = pixdata.apixels[spix];
            val = lookup[spix_val];
            if (lookup_no[spix_val])
                val += dblpfofs[bpldualpf2of];
    	      val ^= xor_val;
            dpix_val = p_acolors[val];
            spix += 2;
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = pixdata.apixels[spix];
            uae_u8 val = lookup[spix_val];
            if (lookup_no[spix_val])
                val += dblpfofs[bpldualpf2of];
    	      val ^= xor_val;
            dpix_val = p_acolors[val];
            spix += 2;
            buf[dpix++] = dpix_val;
        }
    }
    break;
    case CMODE_EXTRAHB:
    {
        int rem;
        if (((uintptr_t)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            if (pixdata.apixels[spix] & 0x20) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val & 0x1f] >> 1) & 0x7F7F7F;
                dpix_val = CONVERT_RGB_16 (c);
            } else
                dpix_val = p_acolors[spix_val];
            spix += 2;
            buf[dpix++] = dpix_val;
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((uintptr_t)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            if (pixdata.apixels[spix] & 0x20) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val & 0x1f] >> 1) & 0x7F7F7F;
                out_val = CONVERT_RGB_16 (c);
            } else
                out_val = p_acolors[spix_val];
            spix += 2;
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            if (pixdata.apixels[spix] & 0x20) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val & 0x1f] >> 1) & 0x7F7F7F;
                dpix_val = CONVERT_RGB_16 (c);
            } else
                dpix_val = p_acolors[spix_val];
            spix += 2;
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            if (pixdata.apixels[spix] & 0x20) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val & 0x1f] >> 1) & 0x7F7F7F;
                dpix_val = CONVERT_RGB_16 (c);
            } else
                dpix_val = p_acolors[spix_val];
            spix += 2;
            buf[dpix++] = dpix_val;
        }
    }
    break;
  }

  return spix;
}
#endif

#ifdef AGA
static int NOINLINE linetoscr_16_shrink2_aga (int spix, int dpix, int dpix_end)
{
  uae_u16 *buf = (uae_u16 *) xlinebuffer;
  uae_u8 xor_val = bplxor;
  uae_u8 and_val = bpland;

  switch(bplmode)
  {
    case CMODE_NORMAL:
    {
        int rem;
        if (((uintptr_t)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            spix += 4;
            buf[dpix++] = p_acolors[spix_val];
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((uintptr_t)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            out_val = p_acolors[spix_val];
            spix += 4;
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            dpix_val = p_acolors[spix_val];
            spix += 4;
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            spix += 4;
            buf[dpix++] = p_acolors[spix_val];
        }
    }
    break;
    case CMODE_HAM:
    {
        int rem;
        if (((uintptr_t)&buf[dpix]) & 2) {
            buf[dpix++] = ham_linebuf[spix];
            spix += 4;
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((uintptr_t)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            out_val = ham_linebuf[spix];
            spix += 4;
            dpix_val = ham_linebuf[spix];
            spix += 4;
            out_val = merge_words(out_val, dpix_val);
            *((uae_u32 *)&buf[dpix]) = out_val;
            dpix += 2;
        }
        if (rem) {
            buf[dpix++] = ham_linebuf[spix];
            spix += 4;
        }
    }
    break;
    case CMODE_DUALPF:
    {
        int *lookup    = bpldualpfpri ? dblpf_ind2_aga : dblpf_ind1_aga;
        int *lookup_no = bpldualpfpri ? dblpf_2nd2     : dblpf_2nd1;
        int rem;
        if (((uintptr_t)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = pixdata.apixels[spix];
            uae_u8 val = lookup[spix_val];
            if (lookup_no[spix_val])
                val += dblpfofs[bpldualpf2of];
    	      val ^= xor_val;
            dpix_val = p_acolors[val];
            spix += 4;
            buf[dpix++] = dpix_val;
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((uintptr_t)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            uae_u8 val = lookup[spix_val];
            if (lookup_no[spix_val])
                val += dblpfofs[bpldualpf2of];
    	      val ^= xor_val;
            out_val = p_acolors[val];
            spix += 4;
            spix_val = pixdata.apixels[spix];
            val = lookup[spix_val];
            if (lookup_no[spix_val])
                val += dblpfofs[bpldualpf2of];
    	      val ^= xor_val;
            dpix_val = p_acolors[val];
            spix += 4;
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = pixdata.apixels[spix];
            uae_u8 val = lookup[spix_val];
            if (lookup_no[spix_val])
                val += dblpfofs[bpldualpf2of];
    	      val ^= xor_val;
            dpix_val = p_acolors[val];
            spix += 4;
            buf[dpix++] = dpix_val;
        }
    }
    break;
    case CMODE_EXTRAHB:
    {
        int rem;
        if (((uintptr_t)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            if (pixdata.apixels[spix] & 0x20) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val & 0x1f] >> 1) & 0x7F7F7F;
                dpix_val = CONVERT_RGB_16 (c);
            } else
                dpix_val = p_acolors[spix_val];
            spix += 4;
            buf[dpix++] = dpix_val;
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((uintptr_t)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            if (pixdata.apixels[spix] & 0x20) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val & 0x1f] >> 1) & 0x7F7F7F;
                out_val = CONVERT_RGB_16 (c);
            } else
                out_val = p_acolors[spix_val];
            spix += 4;
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            if (pixdata.apixels[spix] & 0x20) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val & 0x1f] >> 1) & 0x7F7F7F;
                dpix_val = CONVERT_RGB_16 (c);
            } else
                dpix_val = p_acolors[spix_val];
            spix += 4;
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            if (pixdata.apixels[spix] & 0x20) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val & 0x1f] >> 1) & 0x7F7F7F;
                dpix_val = CONVERT_RGB_16 (c);
            } else
                dpix_val = p_acolors[spix_val];
            spix += 4;
            buf[dpix++] = dpix_val;
        }
    }
    break;
  }

  return spix;
}
#endif

#ifdef AGA
static int NOINLINE linetoscr_16_aga_spr(int spix, int dpix, int dpix_end)
{
  uae_u16 *buf = (uae_u16 *) xlinebuffer;
  uae_u8 sprcol;
  uae_u8 xor_val = bplxor;
  uae_u8 and_val = bpland;

  switch(bplmode)
  {
    case CMODE_NORMAL:
    {
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            sprpix_val = pixdata.apixels[spix];
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            out_val = p_acolors[spix_val];
            spix++;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix + 0, 0, sprpix_val, 1);
                if (sprcol) {
                    out_val = p_acolors[sprcol];
                }
            }
            buf[dpix++] = out_val;
        }
    }
    break;
    case CMODE_HAM:
    {
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 out_val;
        
            sprpix_val = pixdata.apixels[spix];
            out_val = ham_linebuf[spix++];
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix + 0, 0, sprpix_val, 1);
                if (sprcol) {
                    out_val = p_acolors[sprcol];
                }
            }
            buf[dpix++] = out_val;
        }
    }
    break;
    case CMODE_DUALPF:
    {
        int *lookup    = bpldualpfpri ? dblpf_ind2_aga : dblpf_ind1_aga;
        int *lookup_no = bpldualpfpri ? dblpf_2nd2     : dblpf_2nd1;
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            sprpix_val = spix_val;
            uae_u8 val = lookup[spix_val];
            if (lookup_no[spix_val])
                val += dblpfofs[bpldualpf2of];
            val ^= xor_val;
            out_val = p_acolors[val];
            spix++;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix + 0, 1, sprpix_val, 1);
                if (sprcol) {
                    out_val = p_acolors[sprcol];
                }
            }
            buf[dpix++] = out_val;
        }
    }
    break;
    case CMODE_EXTRAHB:
    {
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            sprpix_val = pixdata.apixels[spix];
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            if (pixdata.apixels[spix] & 0x20) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val & 0x1f] >> 1) & 0x7F7F7F;
                out_val = CONVERT_RGB (c);
            } else
                out_val = p_acolors[spix_val];
            spix++;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix + 0, 0, sprpix_val, 1);
                if (sprcol) {
                    out_val = p_acolors[sprcol];
                }
            }
            buf[dpix++] = out_val;
        }
    }
    break;
  }

  return spix;
}
#endif

#ifdef AGA
static int NOINLINE linetoscr_16_stretch1_aga_spr(int spix, int dpix, int dpix_end)
{
  uae_u16 *buf = (uae_u16 *) xlinebuffer;
  uae_u8 sprcol;
  uae_u8 xor_val = bplxor;
  uae_u8 and_val = bpland;

  switch(bplmode)
  {
    case CMODE_NORMAL:
    {
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            sprpix_val = pixdata.apixels[spix];
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            out_val = p_acolors[spix_val];
            spix++;
            {
            uae_u32 out_val1 = out_val;
            uae_u32 out_val2 = out_val;
            if (spritepixels[dpix + 0].data) {
                sprcol = render_sprites (dpix + 0, 0, sprpix_val, 1);
                if (sprcol) {
                    out_val1 = p_acolors[sprcol];
                }
            }
            if (spritepixels[dpix + 1].data) {
                sprcol = render_sprites (dpix + 1, 0, sprpix_val, 1);
                if (sprcol) {
                    out_val2 = p_acolors[sprcol];
                }
            }
            buf[dpix++] = out_val1;
            buf[dpix++] = out_val2;
            }
        }
    }
    break;
    case CMODE_HAM:
    {
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            sprpix_val = pixdata.apixels[spix];
            out_val = ham_linebuf[spix];
            spix++;
            {
            uae_u32 out_val1 = out_val;
            uae_u32 out_val2 = out_val;
            if (spritepixels[dpix + 0].data) {
                sprcol = render_sprites (dpix + 0, 0, sprpix_val, 1);
                if (sprcol) {
                    out_val1 = p_acolors[sprcol];
                }
            }
            if (spritepixels[dpix + 1].data) {
                sprcol = render_sprites (dpix + 1, 0, sprpix_val, 1);
                if (sprcol) {
                    out_val2 = p_acolors[sprcol];
                }
            }
            buf[dpix++] = out_val1;
            buf[dpix++] = out_val2;
            }
        }
    }
    break;
    case CMODE_DUALPF:
    {
        int *lookup    = bpldualpfpri ? dblpf_ind2_aga : dblpf_ind1_aga;
        int *lookup_no = bpldualpfpri ? dblpf_2nd2     : dblpf_2nd1;
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            sprpix_val = spix_val;
            uae_u8 val = lookup[spix_val];
            if (lookup_no[spix_val])
                val += dblpfofs[bpldualpf2of];
            val ^= xor_val;
            out_val = p_acolors[val];
            spix++;
            {
            uae_u32 out_val1 = out_val;
            uae_u32 out_val2 = out_val;
            if (spritepixels[dpix + 0].data) {
                sprcol = render_sprites (dpix + 0, 1, sprpix_val, 1);
                if (sprcol) {
                    out_val1 = p_acolors[sprcol];
                }
            }
            if (spritepixels[dpix + 1].data) {
                sprcol = render_sprites (dpix + 1, 1, sprpix_val, 1);
                if (sprcol) {
                    out_val2 = p_acolors[sprcol];
                }
            }
            buf[dpix++] = out_val1;
            buf[dpix++] = out_val2;
            }
        }
    }
    break;
    case CMODE_EXTRAHB:
    {
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            sprpix_val = pixdata.apixels[spix];
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            if (pixdata.apixels[spix] & 0x20) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val & 0x1f] >> 1) & 0x7F7F7F;
                out_val = CONVERT_RGB (c);
            } else
                out_val = p_acolors[spix_val];
            spix++;
            {
            uae_u32 out_val1 = out_val;
            uae_u32 out_val2 = out_val;
            if (spritepixels[dpix + 0].data) {
                sprcol = render_sprites (dpix + 0, 0, sprpix_val, 1);
                if (sprcol) {
                    out_val1 = p_acolors[sprcol];
                }
            }
            if (spritepixels[dpix + 1].data) {
                sprcol = render_sprites (dpix + 1, 0, sprpix_val, 1);
                if (sprcol) {
                    out_val2 = p_acolors[sprcol];
                }
            }
            buf[dpix++] = out_val1;
            buf[dpix++] = out_val2;
            }
        }
    }
    break;
  }

  return spix;
}
#endif

#ifdef AGA
static int NOINLINE linetoscr_16_shrink1_aga_spr(int spix, int dpix, int dpix_end)
{
  uae_u16 *buf = (uae_u16 *) xlinebuffer;
  uae_u8 sprcol;
  uae_u8 xor_val = bplxor;
  uae_u8 and_val = bpland;

  switch(bplmode)
  {
    case CMODE_NORMAL:
    {
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            sprpix_val = pixdata.apixels[spix];
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            out_val = p_acolors[spix_val];
            spix += 2;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix + 0, 0, sprpix_val, 1);
                if (sprcol) {
                    out_val = p_acolors[sprcol];
                }
            }
            buf[dpix++] = out_val;
        }
    }
    break;
    case CMODE_HAM:
    {
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 out_val;
        
            sprpix_val = pixdata.apixels[spix];
            out_val = ham_linebuf[spix];
            spix += 2;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix + 0, 0, sprpix_val, 1);
                if (sprcol) {
                    out_val = p_acolors[sprcol];
                }
            }
            buf[dpix++] = out_val;
        }
    }
    break;
    case CMODE_DUALPF:
    {
        int *lookup    = bpldualpfpri ? dblpf_ind2_aga : dblpf_ind1_aga;
        int *lookup_no = bpldualpfpri ? dblpf_2nd2     : dblpf_2nd1;
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            sprpix_val = spix_val;
            uae_u8 val = lookup[spix_val];
            if (lookup_no[spix_val])
                val += dblpfofs[bpldualpf2of];
            val ^= xor_val;
            out_val = p_acolors[val];
            spix += 2;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix + 0, 1, sprpix_val, 1);
                if (sprcol) {
                    out_val = p_acolors[sprcol];
                }
            }
            buf[dpix++] = out_val;
        }
    }
    break;
    case CMODE_EXTRAHB:
    {
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            sprpix_val = pixdata.apixels[spix];
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            if (pixdata.apixels[spix] & 0x20) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val & 0x1f] >> 1) & 0x7F7F7F;
                out_val = CONVERT_RGB (c);
            } else
                out_val = p_acolors[spix_val];
            spix += 2;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix + 0, 0, sprpix_val, 1);
                if (sprcol) {
                    out_val = p_acolors[sprcol];
                }
            }
            buf[dpix++] = out_val;
        }
    }
    break;
  }

  return spix;
}
#endif

#ifdef AGA
static int NOINLINE linetoscr_16_shrink2_aga_spr(int spix, int dpix, int dpix_end)
{
  uae_u16 *buf = (uae_u16 *) xlinebuffer;
  uae_u8 sprcol;
  uae_u8 xor_val = bplxor;
  uae_u8 and_val = bpland;

  switch(bplmode)
  {
    case CMODE_NORMAL:
    {
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            sprpix_val = pixdata.apixels[spix];
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            out_val = p_acolors[spix_val];
            spix += 4;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix + 0, 0, sprpix_val, 1);
                if (sprcol) {
                    out_val = p_acolors[sprcol];
                }
            }
            buf[dpix++] = out_val;
        }
    }
    break;
    case CMODE_HAM:
    {
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 out_val;
        
            sprpix_val = pixdata.apixels[spix];
            out_val = ham_linebuf[spix];
            spix += 4;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix + 0, 0, sprpix_val, 1);
                if (sprcol) {
                    out_val = p_acolors[sprcol];
                }
            }
            buf[dpix++] = out_val;
        }
    }
    break;
    case CMODE_DUALPF:
    {
        int *lookup    = bpldualpfpri ? dblpf_ind2_aga : dblpf_ind1_aga;
        int *lookup_no = bpldualpfpri ? dblpf_2nd2     : dblpf_2nd1;
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            sprpix_val = spix_val;
            uae_u8 val = lookup[spix_val];
            if (lookup_no[spix_val])
                val += dblpfofs[bpldualpf2of];
            val ^= xor_val;
            out_val = p_acolors[val];
            spix += 4;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix + 0, 1, sprpix_val, 1);
                if (sprcol) {
                    out_val = p_acolors[sprcol];
                }
            }
            buf[dpix++] = out_val;
        }
    }
    break;
    case CMODE_EXTRAHB:
    {
        while (dpix < dpix_end) {
            uae_u32 sprpix_val;
            uae_u32 spix_val;
            uae_u32 out_val;
        
            sprpix_val = pixdata.apixels[spix];
            spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;
            if (pixdata.apixels[spix] & 0x20) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val & 0x1f] >> 1) & 0x7F7F7F;
                out_val = CONVERT_RGB (c);
            } else
                out_val = p_acolors[spix_val];
            spix += 4;
            if (spritepixels[dpix].data) {
                sprcol = render_sprites (dpix + 0, 0, sprpix_val, 1);
                if (sprcol) {
                    out_val = p_acolors[sprcol];
                }
            }
            buf[dpix++] = out_val;
        }
    }
    break;
  }

  return spix;
}
#endif
