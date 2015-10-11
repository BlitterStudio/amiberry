
static __inline__ int LNAME (int spix, int dpix, int stoppos)
{
   TYPE *buf = ((TYPE *)xlinebuffer);
#if AGA_LTS
   uae_u8 xor_val;
#endif

#if AGA_LTS
   xor_val = (uae_u8)(dp_for_drawing->bplcon4 >> 8);
#endif
   if (dp_for_drawing->ham_seen) {
      /* HAM 6 / HAM 8 */
      while (dpix < stoppos) {
#if AGA_LTS
            TYPE d = CONVERT_RGB (ham_linebuf[spix]);
#else
            TYPE d = xcolors[ham_linebuf[spix]];
#endif
            spix += SRC_INC;
            buf[dpix++] = d;
#if HDOUBLE
            buf[dpix++] = d;
#endif
      }
   } else if (bpldualpf) {
#if AGA_LTS
      /* AGA Dual playfield */
      int *lookup = bpldualpfpri ? dblpf_ind2_aga : dblpf_ind1_aga;
      int *lookup_no = bpldualpfpri ? dblpf_2nd2 : dblpf_2nd1;
      while (dpix < stoppos) {
            int pixcol = pixdata.apixels[spix];
            TYPE d;
            if (spritepixels[spix]) {
               d = colors_for_drawing.acolors[spritepixels[spix]];
               spritepixels[spix]=0;
            } else {
               int val = lookup[pixcol];
               if (lookup_no[pixcol] == 2)  val += dblpfofs[bpldualpf2of];
               val ^= xor_val;
               d = colors_for_drawing.acolors[val];
            }
            spix += SRC_INC;
            buf[dpix++] = d;
#if HDOUBLE
            buf[dpix++] = d;
#endif
      }
#else
      /* OCS/ECS Dual playfield  */
      int *lookup = bpldualpfpri ? dblpf_ind2 : dblpf_ind1;
      while (dpix < stoppos) {
            int pixcol = pixdata.apixels[spix];
            TYPE d = colors_for_drawing.acolors[lookup[pixcol]];
            spix += SRC_INC;
            buf[dpix++] = d;
#if HDOUBLE
            buf[dpix++] = d;
#endif
      }
#endif
   } else if (bplehb) {
      while (dpix < stoppos) {
#if AGA_LTS
            /* AGA EHB playfield */
            int p = pixdata.apixels[spix]^xor_val;
            TYPE d;
            spix += SRC_INC;
            if (p>= 32 && p < 64) {
               int c = (colors_for_drawing.color_regs_aga[(p-32)] >> 1) & 0x7F7F7F;
               d = CONVERT_RGB(c);
            }
            else
              d = colors_for_drawing.acolors[p];
#else
            /* OCS/ECS EHB playfield */
            int p = pixdata.apixels[spix];
            TYPE d;
            spix += SRC_INC;
            if (p >= 32)
              d = xcolors[(colors_for_drawing.color_regs_ecs[p-32] >> 1) & 0x777];
            else
              d = colors_for_drawing.acolors[p];
#endif
            buf[dpix++] = d;
#if HDOUBLE
            buf[dpix++] = d;
#endif
      }
   } else {
      while (dpix < stoppos) {
#if AGA_LTS
            TYPE d = colors_for_drawing.acolors[pixdata.apixels[spix]^xor_val];
#else
            TYPE d = colors_for_drawing.acolors[pixdata.apixels[spix]];
#endif
            spix += SRC_INC;
            buf[dpix++] = d;
#if HDOUBLE
            buf[dpix++] = d;
#endif
      }
   }
   return spix;
}

#undef LNAME
#undef HDOUBLE
#undef SRC_INC
#undef AGA_LTS
