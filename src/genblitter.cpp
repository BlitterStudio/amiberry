 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Optimized blitter minterm function generator
  *
  * Copyright 1995,1996 Bernd Schmidt
  * Copyright 1996 Alessandro Bissacco
  */

#include "sysconfig.h"
#include <stdio.h>
#include <stdlib.h>

#include "genblitter.h"

/* Here is the minterm table used in blitter function generation */

static unsigned char blttbl[]= {
  0x00, 0x0a, 0x2a, 0x30, 0x3a, 0x3c, 0x4a, 0x6a, 0x8a, 0x8c, 0x9a, 0xa8,
  0xaa, 0xb1, 0xca, 0xcc, 0xd8, 0xe2, 0xea, 0xf0, 0xfa, 0xfc
};

static void generate_include(void)
{
  int minterm;
  printf("STATIC_INLINE uae_u32 blit_func(const uae_u32 srca, const uae_u32 srcb, const uae_u32 srcc, const uae_u8 mt)\n{\nswitch(mt){\n");
  for (minterm = 0; minterm < 256; minterm++) {
	  printf("case 0x%x:\n", minterm);
	  printf("\treturn %s;\n", blitops[minterm].s);
  }
  printf("}\n");
  printf("return 0;\n"); /* No, sir, it doesn't! */
  printf("}\n");
}

static void generate_func(void)
{
  unsigned int i;
  printf("#include \"sysconfig.h\"\n");
  printf("#include \"sysdeps.h\"\n");
  printf("#include \"options.h\"\n");
  printf("#include \"memory.h\"\n");
  printf("#include \"newcpu.h\"\n");
  printf("#include \"custom.h\"\n");
  printf("#include \"savestate.h\"\n");
  printf("#include \"blitter.h\"\n");
  printf("#include \"blitfunc.h\"\n\n");

  for (i = 0; i < sizeof(blttbl); i++) {
    int active = blitops[blttbl[i]].used;
    int a_is_on = active & 1, b_is_on = active & 2, c_is_on = active & 4;
    printf("void blitdofast_%x (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)\n",blttbl[i]);
    printf("{\n");
    printf("int i,j;\n");
    printf("uae_u32 totald = 0;\n");
    if (b_is_on) printf("uae_u32 srcb = b->bltbhold;\n");
    if (c_is_on) printf("uae_u32 srcc = b->bltcdat;\n");
    printf("uae_u32 dstd=0;\n");
    printf("uaecptr dstp = 0;\n");
    if (a_is_on) printf("uae_u32 *blit_masktable_p = blit_masktable + BLITTER_MAX_WORDS - b->hblitsize;\n");
    printf("for (j = b->vblitsize; j--;) {\n");
    printf("\tfor (i = b->hblitsize; i--;) {\n\t\tuae_u32 bltadat, srca;\n\n");
    if (c_is_on) printf("\t\tif (ptc) { srcc = do_get_mem_word ((uae_u16 *)ptc); ptc += 2; }\n");
    if (b_is_on) printf("\t\tif (ptb) {\n\t\t\tuae_u32 bltbdat; b->bltbdat = bltbdat = do_get_mem_word ((uae_u16 *)ptb); ptb += 2;\n");
	  if (b_is_on) printf("\t\t\tsrcb = (((uae_u32)b->bltbold << 16) | bltbdat) >> b->blitbshift;\n");
	  if (b_is_on) printf("\t\t\tb->bltbold = bltbdat;\n\t\t}\n");
    if (a_is_on) printf("\t\tif (pta) { b->bltadat = bltadat = do_get_mem_word ((uae_u16 *)pta); pta += 2; } else { bltadat = b->bltadat; }\n");
    if (a_is_on) printf("\t\tbltadat &= blit_masktable_p[i];\n");
	  if (a_is_on) printf("\t\tsrca = (((uae_u32)b->bltaold << 16) | bltadat) >> b->blitashift;\n");
	  if (a_is_on) printf("\t\tb->bltaold = bltadat;\n");
    printf("\t\tif (dstp)\n\t\t\tchipmem_agnus_wput (dstp, dstd);\n");
    printf("\t\tdstd = (%s);\n", blitops[blttbl[i]].s);
    printf("\t\ttotald |= dstd;\n");
    printf("\t\tif (ptd) { dstp = ptd; ptd += 2; }\n");
    printf("\t}\n");
    if (a_is_on) printf("\tif (pta) pta += b->bltamod;\n");
    if (b_is_on) printf("\tif (ptb) ptb += b->bltbmod;\n");
    if (c_is_on) printf("\tif (ptc) ptc += b->bltcmod;\n");
    printf("\tif (ptd) ptd += b->bltdmod;\n");
    printf("}\n");
    if (b_is_on) printf("b->bltbhold = srcb;\n");
    if (c_is_on) printf("b->bltcdat = srcc;\n");
    printf("\t\tif (dstp)\n\t\t\tchipmem_agnus_wput (dstp, dstd);\n");
    printf("if ((totald<<16) != 0) b->blitzero = 0;\n");
    printf("}\n");

    printf("void blitdofast_desc_%x (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)\n",blttbl[i]);
    printf("{\n");
    printf("uae_u32 totald = 0;\n");
    printf("int i,j;\n");

    if (b_is_on) printf("uae_u32 srcb = b->bltbhold;\n");
    if (c_is_on) printf("uae_u32 srcc = b->bltcdat;\n");
    printf("uae_u32 dstd = 0;\n");
    printf("uaecptr dstp = 0;\n");
    if (a_is_on) printf("uae_u32 *blit_masktable_p = blit_masktable + BLITTER_MAX_WORDS - b->hblitsize;\n");
    printf("for (j = b->vblitsize; j--;) {\n");
    printf("\tfor (i = b->hblitsize; i--;) {\n\t\tuae_u32 bltadat, srca;\n");
    if (c_is_on) printf("\t\tif (ptc) { srcc = do_get_mem_word ((uae_u16 *)ptc); ptc -= 2; }\n");
    if (b_is_on) printf("\t\tif (ptb) {\n\t\t\tuae_u32 bltbdat; b->bltbdat = bltbdat = do_get_mem_word ((uae_u16 *)ptb); ptb -= 2;\n");
	  if (b_is_on) printf("\t\t\tsrcb = ((bltbdat << 16) | b->bltbold) >> b->blitdownbshift;\n");
	  if (b_is_on) printf("\t\t\tb->bltbold = bltbdat;\n\t\t}\n");
    if (a_is_on) printf("\t\tif (pta) { b->bltadat = bltadat = do_get_mem_word ((uae_u16 *)pta); pta -= 2; } else { bltadat = b->bltadat; }\n");
    if (a_is_on) printf("\t\tbltadat &= blit_masktable_p[i];\n");
	  if (a_is_on) printf("\t\tsrca = (((uae_u32)bltadat << 16) | b->bltaold) >> b->blitdownashift;\n");
	  if (a_is_on) printf("\t\tb->bltaold = bltadat;\n");
    printf("\t\tif (dstp)\n\t\t\tchipmem_agnus_wput (dstp, dstd);\n");
    printf("\t\tdstd = (%s);\n", blitops[blttbl[i]].s);
    printf("\t\ttotald |= dstd;\n");
    printf("\t\tif (ptd) { dstp = ptd; ptd -= 2; }\n");
    printf("\t}\n");
    if (a_is_on) printf("\tif (pta) pta -= b->bltamod;\n");
    if (b_is_on) printf("\tif (ptb) ptb -= b->bltbmod;\n");
    if (c_is_on) printf("\tif (ptc) ptc -= b->bltcmod;\n");
    printf("\tif (ptd) ptd -= b->bltdmod;\n");
    printf("}\n");
    if (b_is_on) printf("b->bltbhold = srcb;\n");
    if (c_is_on) printf("b->bltcdat = srcc;\n");
    printf("\t\tif (dstp)\n\t\t\tchipmem_agnus_wput (dstp, dstd);\n");
    printf("if ((totald<<16) != 0) b->blitzero = 0;\n");
    printf("}\n");
  }
}

static void generate_table(void)
{
  unsigned int index = 0;
  unsigned int i;
  printf("#include \"sysconfig.h\"\n");
  printf("#include \"sysdeps.h\"\n");
  printf("#include \"options.h\"\n");
  printf("#include \"memory.h\"\n");
  printf("#include \"newcpu.h\"\n");
  printf("#include \"custom.h\"\n");
  printf("#include \"savestate.h\"\n");
  printf("#include \"blitter.h\"\n");
  printf("#include \"blitfunc.h\"\n\n");
  printf("blitter_func * const blitfunc_dofast[256] = {\n");
  for (i = 0; i < 256; i++) {
  	if (index < sizeof(blttbl) && i == blttbl[index]) {
	    printf("blitdofast_%x",i);
	    index++;
  	}
  	else printf("0");
  	if (i < 255) printf(", ");
  	if ((i & 7) == 7) printf("\n");
  }
  printf("};\n\n");

  index = 0;
  printf("blitter_func * const blitfunc_dofast_desc[256] = {\n");
  for (i = 0; i < 256; i++) {
  	if (index < sizeof(blttbl) && i == blttbl[index]) {
	    printf("blitdofast_desc_%x",i);
	    index++;
	  }
	  else printf("0");
	  if (i < 255) printf(", ");
	  if ((i & 7) == 7) printf("\n");
  }
  printf("};\n");
}

static void generate_header(void)
{
  unsigned int i;
  for (i = 0; i < sizeof(blttbl); i++) {
	  printf("extern blitter_func blitdofast_%x;\n",blttbl[i]);
	  printf("extern blitter_func blitdofast_desc_%x;\n",blttbl[i]);
  }
}

int main(int argc, char **argv)
{
  char mode = 'i';

  if (argc == 2) mode = *argv[1];
  switch (mode) {
    case 'i': generate_include();
      break;
    case 'f': generate_func();
      break;
    case 't': generate_table();
      break;
    case 'h': generate_header();
      break;
    default: abort();
  }
  return 0;
}

