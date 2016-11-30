/* Note:
 *    xcolors[] contains 16-bit color information in both words
 *    colors_for_drawing.acolors (non AGA) contains 16-bit color information in both words
 *    colors_for_drawing.acolors (AGA) contains 16-bit color information in one word
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

    if (bplham) {
        int rem;
        if (((long)&buf[dpix]) & 2) {
            buf[dpix++] = ham_linebuf[spix++];
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((long)&buf[dpix_end]) & 2);
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
    } else if (bpldualpf) {
        int *lookup = bpldualpfpri ? dblpf_ind2 : dblpf_ind1;
        int rem;
        if (((long)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            spix_val = pixdata.apixels[spix++];
            buf[dpix++] = colors_for_drawing.acolors[lookup[spix_val]];
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((long)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix++];
            out_val = colors_for_drawing.acolors[lookup[spix_val]];
            spix_val = pixdata.apixels[spix++];
            dpix_val = colors_for_drawing.acolors[lookup[spix_val]];
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            spix_val = pixdata.apixels[spix++];
            buf[dpix++] = colors_for_drawing.acolors[lookup[spix_val]];
        }
    } else if (bplehb) {
        int rem;
        if (((long)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = pixdata.apixels[spix++];
            if (spix_val <= 31)
                dpix_val = colors_for_drawing.acolors[spix_val];
            else
                dpix_val = xcolors[(colors_for_drawing.color_regs_ecs[spix_val - 32] >> 1) & 0x777];
            buf[dpix++] = dpix_val;
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((long)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix++];
            if (spix_val <= 31)
                out_val = colors_for_drawing.acolors[spix_val];
            else
                out_val = xcolors[(colors_for_drawing.color_regs_ecs[spix_val - 32] >> 1) & 0x777];
            spix_val = pixdata.apixels[spix++];
            if (spix_val <= 31)
                dpix_val = colors_for_drawing.acolors[spix_val];
            else
                dpix_val = xcolors[(colors_for_drawing.color_regs_ecs[spix_val - 32] >> 1) & 0x777];
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = pixdata.apixels[spix++];
            if (spix_val <= 31)
                dpix_val = colors_for_drawing.acolors[spix_val];
            else
                dpix_val = xcolors[(colors_for_drawing.color_regs_ecs[spix_val - 32] >> 1) & 0x777];
            buf[dpix++] = dpix_val;
        }
    } else {
        int rem;
        if (((long)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            spix_val = pixdata.apixels[spix++];
            buf[dpix++] = colors_for_drawing.acolors[spix_val];
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((long)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix++];
            out_val = colors_for_drawing.acolors[spix_val];
            spix_val = pixdata.apixels[spix++];
            dpix_val = colors_for_drawing.acolors[spix_val];
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            spix_val = pixdata.apixels[spix++];
            buf[dpix++] = colors_for_drawing.acolors[spix_val];
        }
    }

    return spix;
}

static int NOINLINE linetoscr_16_stretch1 (int spix, int dpix, int dpix_end)
{
    uae_u16 *buf = (uae_u16 *) xlinebuffer;

    if (bplham) {
        while (dpix < dpix_end) {
            uae_u32 out_val = ham_linebuf[spix++];
            *((uae_u32 *)&buf[dpix]) = double_word(out_val);
            dpix += 2;
        }
    } else if (bpldualpf) {
        int *lookup = bpldualpfpri ? dblpf_ind2 : dblpf_ind1;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
        
            spix_val = pixdata.apixels[spix++];
            *((uae_u32 *)&buf[dpix]) = colors_for_drawing.acolors[lookup[spix_val]];
            dpix += 2;
        }
    } else if (bplehb) {
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix++];
            if (spix_val <= 31)
              out_val = colors_for_drawing.acolors[spix_val];
            else
              out_val = xcolors[(colors_for_drawing.color_regs_ecs[spix_val - 32] >> 1) & 0x0777];
              *((uae_u32 *)&buf[dpix]) = double_word(out_val);
            dpix += 2;
        }
    } else {
        while (dpix < dpix_end) {
            uae_u32 spix_val;
        
            spix_val = pixdata.apixels[spix++];
            *((uae_u32 *)&buf[dpix]) = colors_for_drawing.acolors[spix_val];
            dpix += 2;
        }
    }

    return spix;
}

static int NOINLINE linetoscr_16_shrink1 (int spix, int dpix, int dpix_end)
{
    uae_u16 *buf = (uae_u16 *) xlinebuffer;

    if (bplham) {
        int rem;
        if (((long)&buf[dpix]) & 2) {
            uae_u32 dpix_val;
            dpix_val = ham_linebuf[spix];
            spix += 2;
            buf[dpix++] = dpix_val;
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((long)&buf[dpix_end]) & 2);
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
    } else if (bpldualpf) {
        int *lookup = bpldualpfpri ? dblpf_ind2 : dblpf_ind1;
        int rem;
        if (((long)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = pixdata.apixels[spix];
            dpix_val = colors_for_drawing.acolors[lookup[spix_val]];
            spix += 2;
            buf[dpix++] = dpix_val;
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((long)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            out_val = colors_for_drawing.acolors[lookup[spix_val]];
            spix += 2;
            spix_val = pixdata.apixels[spix];
            dpix_val = colors_for_drawing.acolors[lookup[spix_val]];
            spix += 2;
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = pixdata.apixels[spix];
            dpix_val = colors_for_drawing.acolors[lookup[spix_val]];
            spix += 2;
            buf[dpix++] = dpix_val;
        }
    } else if (bplehb) {
        int rem;
        if (((long)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = pixdata.apixels[spix];
            if (spix_val <= 31)
                dpix_val = colors_for_drawing.acolors[spix_val];
            else
                dpix_val = xcolors[(colors_for_drawing.color_regs_ecs[spix_val - 32] >> 1) & 0x777];
            spix += 2;
            buf[dpix++] = dpix_val;
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((long)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            if (spix_val <= 31)
                out_val = colors_for_drawing.acolors[spix_val];
            else
                out_val = xcolors[(colors_for_drawing.color_regs_ecs[spix_val - 32] >> 1) & 0x777];
            spix += 2;
            spix_val = pixdata.apixels[spix];
            if (spix_val <= 31)
                dpix_val = colors_for_drawing.acolors[spix_val];
            else
                dpix_val = xcolors[(colors_for_drawing.color_regs_ecs[spix_val - 32] >> 1) & 0x777];
            spix += 2;
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = pixdata.apixels[spix];
            if (spix_val <= 31)
                dpix_val = colors_for_drawing.acolors[spix_val];
            else
                dpix_val = xcolors[(colors_for_drawing.color_regs_ecs[spix_val - 32] >> 1) & 0x777];
            spix += 2;
            buf[dpix++] = dpix_val;
        }
    } else {
        int rem;
        if (((long)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            spix_val = pixdata.apixels[spix];
            spix += 2;
            buf[dpix++] = colors_for_drawing.acolors[spix_val];
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((long)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix];
            out_val = colors_for_drawing.acolors[spix_val];
            spix += 2;
            spix_val = pixdata.apixels[spix];
            dpix_val = colors_for_drawing.acolors[spix_val];
            spix += 2;
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            spix_val = pixdata.apixels[spix];
            spix += 2;
            buf[dpix++] = colors_for_drawing.acolors[spix_val];
        }
    }

    return spix;
}

#ifdef AGA

static int NOINLINE linetoscr_16_aga (int spix, int dpix, int dpix_end)
{
    uae_u16 *buf = (uae_u16 *) xlinebuffer;
    uae_u8 xor_val = bplxor;

    if (bplham) {
        int rem;
        if (((long)&buf[dpix]) & 2) {
            buf[dpix++] = ham_linebuf[spix];
            spix++;
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((long)&buf[dpix_end]) & 2);
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
    } else if (bpldualpf) {
        int *lookup    = bpldualpfpri ? dblpf_ind2_aga : dblpf_ind1_aga;
        int *lookup_no = bpldualpfpri ? dblpf_2nd2     : dblpf_2nd1;
        int rem;
        if (((long)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            if (spritepixels[spix]) {
                dpix_val = colors_for_drawing.acolors[spritepixels[spix]];
            } else {
                spix_val = pixdata.apixels[spix];
                uae_u8 val = lookup[spix_val];
                if (lookup_no[spix_val] == 2)
                    val += dblpfofs[bpldualpf2of];
        	      val ^= xor_val;
                dpix_val = colors_for_drawing.acolors[val];
            }
            spix++;
            buf[dpix++] = dpix_val;
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((long)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            if (spritepixels[spix]) {
                out_val = colors_for_drawing.acolors[spritepixels[spix]];
            } else {
                spix_val = pixdata.apixels[spix];
                uae_u8 val = lookup[spix_val];
                if (lookup_no[spix_val] == 2)
                    val += dblpfofs[bpldualpf2of];
        	      val ^= xor_val;
                out_val = colors_for_drawing.acolors[val];
            }
            spix++;
            if (spritepixels[spix]) {
                dpix_val = colors_for_drawing.acolors[spritepixels[spix]];
            } else {
                spix_val = pixdata.apixels[spix];
                uae_u8 val = lookup[spix_val];
                if (lookup_no[spix_val] == 2)
                    val += dblpfofs[bpldualpf2of];
        	      val ^= xor_val;
                dpix_val = colors_for_drawing.acolors[val];
            }
            spix++;
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            if (spritepixels[spix]) {
                dpix_val = colors_for_drawing.acolors[spritepixels[spix]];
            } else {
                spix_val = pixdata.apixels[spix];
                uae_u8 val = lookup[spix_val];
                if (lookup_no[spix_val] == 2)
                    val += dblpfofs[bpldualpf2of];
        	      val ^= xor_val;
                dpix_val = colors_for_drawing.acolors[val];
            }
            spix++;
            buf[dpix++] = dpix_val;
        }
    } else if (bplehb) {
        int rem;
        if (((long)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = pixdata.apixels[spix++] ^ xor_val;
            if (spix_val >= 32 && spix_val < 64) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val - 32] >> 1) & 0x7F7F7F;
                dpix_val = CONVERT_RGB (c);
            } else
                dpix_val = colors_for_drawing.acolors[spix_val];
            buf[dpix++] = dpix_val;
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((long)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix++] ^ xor_val;
            if (spix_val >= 32 && spix_val < 64) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val - 32] >> 1) & 0x7F7F7F;
                out_val = CONVERT_RGB (c);
            } else
                out_val = colors_for_drawing.acolors[spix_val];
            spix_val = pixdata.apixels[spix++] ^ xor_val;
            if (spix_val >= 32 && spix_val < 64) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val - 32] >> 1) & 0x7F7F7F;
                dpix_val = CONVERT_RGB (c);
            } else
                dpix_val = colors_for_drawing.acolors[spix_val];
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = pixdata.apixels[spix++] ^ xor_val;
            if (spix_val >= 32 && spix_val < 64) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val - 32] >> 1) & 0x7F7F7F;
                dpix_val = CONVERT_RGB (c);
            } else
                dpix_val = colors_for_drawing.acolors[spix_val];
            buf[dpix++] = dpix_val;
        }
    } else {
        int rem;
        if (((long)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            spix_val = pixdata.apixels[spix++] ^ xor_val;
            buf[dpix++] = colors_for_drawing.acolors[spix_val];
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((long)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix++] ^ xor_val;
            out_val = colors_for_drawing.acolors[spix_val];
            spix_val = pixdata.apixels[spix++] ^ xor_val;
            dpix_val = colors_for_drawing.acolors[spix_val];
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            spix_val = pixdata.apixels[spix++] ^ xor_val;
            buf[dpix++] = colors_for_drawing.acolors[spix_val];
        }
    }

    return spix;
}
#endif

#ifdef AGA
static int NOINLINE linetoscr_16_stretch1_aga (int spix, int dpix, int dpix_end)
{
    uae_u16 *buf = (uae_u16 *) xlinebuffer;
    uae_u8 xor_val = bplxor;

    if (bplham) {
        while (dpix < dpix_end) {
            uae_u32 out_val;
            out_val = ham_linebuf[spix++];
            *((uae_u32 *)&buf[dpix]) = double_word(out_val);
            dpix += 2;
        }
    } else if (bpldualpf) {
        int *lookup    = bpldualpfpri ? dblpf_ind2_aga : dblpf_ind1_aga;
        int *lookup_no = bpldualpfpri ? dblpf_2nd2     : dblpf_2nd1;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 out_val;
        
            if (spritepixels[spix]) {
                out_val = colors_for_drawing.acolors[spritepixels[spix]];
            } else {
                spix_val = pixdata.apixels[spix];
                uae_u8 val = lookup[spix_val];
                if (lookup_no[spix_val] == 2)
                    val += dblpfofs[bpldualpf2of];
        	      val ^= xor_val;
                out_val = colors_for_drawing.acolors[val];
            }
            spix++;
            *((uae_u32 *)&buf[dpix]) = double_word(out_val);
            dpix += 2;
        }
    } else if (bplehb) {
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix++] ^ xor_val;
            if (spix_val >= 32 && spix_val < 64) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val - 32] >> 1) & 0x7F7F7F;
                out_val = CONVERT_RGB (c);
            } else
                out_val = colors_for_drawing.acolors[spix_val];
            *((uae_u32 *)&buf[dpix]) = double_word(out_val);
            dpix += 2;
        }
    } else {
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix++] ^ xor_val;
            out_val = colors_for_drawing.acolors[spix_val];
            *((uae_u32 *)&buf[dpix]) = double_word(out_val);
            dpix += 2;
        }
    }

    return spix;
}
#endif

#ifdef AGA
static int NOINLINE linetoscr_16_shrink1_aga (int spix, int dpix, int dpix_end)
{
    uae_u16 *buf = (uae_u16 *) xlinebuffer;
    uae_u8 xor_val = bplxor;

    if (bplham) {
        int rem;
        if (((long)&buf[dpix]) & 2) {
            buf[dpix++] = ham_linebuf[spix];
            spix += 2;
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((long)&buf[dpix_end]) & 2);
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
    } else if (bpldualpf) {
        int *lookup    = bpldualpfpri ? dblpf_ind2_aga : dblpf_ind1_aga;
        int *lookup_no = bpldualpfpri ? dblpf_2nd2     : dblpf_2nd1;
        int rem;
        if (((long)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            if (spritepixels[spix]) {
                dpix_val = colors_for_drawing.acolors[spritepixels[spix]];
            } else {
                spix_val = pixdata.apixels[spix];
                uae_u8 val = lookup[spix_val];
                if (lookup_no[spix_val] == 2)
                    val += dblpfofs[bpldualpf2of];
        	      val ^= xor_val;
                dpix_val = colors_for_drawing.acolors[val];
            }
            spix += 2;
            buf[dpix++] = dpix_val;
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((long)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            if (spritepixels[spix]) {
                out_val = colors_for_drawing.acolors[spritepixels[spix]];
            } else {
                spix_val = pixdata.apixels[spix];
                uae_u8 val = lookup[spix_val];
                if (lookup_no[spix_val] == 2)
                    val += dblpfofs[bpldualpf2of];
        	      val ^= xor_val;
                out_val = colors_for_drawing.acolors[val];
            }
            spix += 2;
            if (spritepixels[spix]) {
                dpix_val = colors_for_drawing.acolors[spritepixels[spix]];
            } else {
                spix_val = pixdata.apixels[spix];
                uae_u8 val = lookup[spix_val];
                if (lookup_no[spix_val] == 2)
                    val += dblpfofs[bpldualpf2of];
        	      val ^= xor_val;
                dpix_val = colors_for_drawing.acolors[val];
            }
            spix += 2;
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            if (spritepixels[spix]) {
                dpix_val = colors_for_drawing.acolors[spritepixels[spix]];
            } else {
                spix_val = pixdata.apixels[spix];
                uae_u8 val = lookup[spix_val];
                if (lookup_no[spix_val] == 2)
                    val += dblpfofs[bpldualpf2of];
        	      val ^= xor_val;
                dpix_val = colors_for_drawing.acolors[val];
            }
            spix += 2;
            buf[dpix++] = dpix_val;
        }
    } else if (bplehb) {
        int rem;
        if (((long)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = pixdata.apixels[spix] ^ xor_val;
            if (spix_val >= 32 && spix_val < 64) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val - 32] >> 1) & 0x7F7F7F;
                dpix_val = CONVERT_RGB (c);
            } else
                dpix_val = colors_for_drawing.acolors[spix_val];
            spix += 2;
            buf[dpix++] = dpix_val;
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((long)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix] ^ xor_val;
            if (spix_val >= 32 && spix_val < 64) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val - 32] >> 1) & 0x7F7F7F;
                out_val = CONVERT_RGB (c);
            } else
                out_val = colors_for_drawing.acolors[spix_val];
            spix += 2;
            spix_val = pixdata.apixels[spix] ^ xor_val;
            if (spix_val >= 32 && spix_val < 64) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val - 32] >> 1) & 0x7F7F7F;
                dpix_val = CONVERT_RGB (c);
            } else
                dpix_val = colors_for_drawing.acolors[spix_val];
            spix += 2;
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            spix_val = pixdata.apixels[spix] ^ xor_val;
            if (spix_val >= 32 && spix_val < 64) {
                unsigned int c = (colors_for_drawing.color_regs_aga[spix_val - 32] >> 1) & 0x7F7F7F;
                dpix_val = CONVERT_RGB (c);
            } else
                dpix_val = colors_for_drawing.acolors[spix_val];
            spix += 2;
            buf[dpix++] = dpix_val;
        }
    } else {
        int rem;
        if (((long)&buf[dpix]) & 2) {
            uae_u32 spix_val;
            spix_val = pixdata.apixels[spix] ^ xor_val;
            spix += 2;
            buf[dpix++] = colors_for_drawing.acolors[spix_val];
        }
        if (dpix >= dpix_end)
            return spix;
        rem = (((long)&buf[dpix_end]) & 2);
        if (rem)
            dpix_end--;
        while (dpix < dpix_end) {
            uae_u32 spix_val;
            uae_u32 dpix_val;
            uae_u32 out_val;
        
            spix_val = pixdata.apixels[spix] ^ xor_val;
            out_val = colors_for_drawing.acolors[spix_val];
            spix += 2;
            spix_val = pixdata.apixels[spix] ^ xor_val;
            dpix_val = colors_for_drawing.acolors[spix_val];
            spix += 2;
            *((uae_u32 *)&buf[dpix]) = merge_words(out_val, dpix_val);
            dpix += 2;
        }
        if (rem) {
            uae_u32 spix_val;
            spix_val = pixdata.apixels[spix] ^ xor_val;
            spix += 2;
            buf[dpix++] = colors_for_drawing.acolors[spix_val];
        }
    }

    return spix;
}
#endif
