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
    printf("STATIC_INLINE uae_u32 blit_func(uae_u32 srca, uae_u32 srcb, uae_u32 srcc, uae_u8 mt)\n{\nswitch(mt){\n");
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
    printf("#include \"custom.h\"\n");
    printf("#include \"memory.h\"\n");
    printf("#include \"blitter.h\"\n");
    printf("#include \"blitfunc.h\"\n\n");

    for (i = 0; i < sizeof(blttbl); i++) {
	int active = blitops[blttbl[i]].used;
	int a_is_on = active & 1, b_is_on = active & 2, c_is_on = active & 4;
	printf("void blitdofast_%x (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)\n",blttbl[i]);
	printf("{\n");
	printf("int i,j;\n");
	printf("uae_u32 totald = 0;\n");
#if 0
	printf("if (currprefs.blits_32bit_enabled && b->hblitsize > 1");
	if (a_is_on) printf(" && !b->blitashift && b->bltafwm==0xffff && b->bltalwm==0xffff");
	if (b_is_on) printf(" && !b->blitbshift");
	printf(") {\n");
	if (a_is_on) printf("uae_u32 srca=((uae_u32)b->bltadat << 16) | b->bltadat;\n");
	if (b_is_on) printf("uae_u32 srcb=((uae_u32)b->bltbdat << 16) | b->bltbdat;\n");
	if (c_is_on) printf("uae_u32 srcc=((uae_u32)b->bltcdat << 16) | b->bltcdat;\n");
	printf("uae_u32 dest;\n");
	printf("int count=b->hblitsize/2, oddword=b->hblitsize&1;\n");
	printf("for (j=0;j<b->vblitsize;j++) {\n");
	printf("\tfor(i=0;i<count;i++) {\n");
	if (a_is_on) printf("\t\tif (pta) {srca=*((uae_u32 *)pta); pta += 4;}\n");
	if (b_is_on) printf("\t\tif (ptb) {srcb=*((uae_u32 *)ptb); ptb += 4;}\n");
	if (c_is_on) printf("\t\tif (ptc) {srcc=*((uae_u32 *)ptc); ptc += 4;}\n");
	printf("\t\tdest = %s;\n", blitops[blttbl[i]].s);
	printf("\t\ttotald |= dest;\n");
	printf("\t\tif (ptd) {*(uae_u32 *)ptd=dest; ptd += 4;}\n");
	printf("\t}\n");
	printf("\tif (oddword) {\n");
	if (a_is_on) printf("\t\tif (pta) { srca=(uae_u32)*(uae_u16 *)pta; pta += 2; }\n");
	if (b_is_on) printf("\t\tif (ptb) { srcb=(uae_u32)*(uae_u16 *)ptb; ptb += 2; }\n");
	if (c_is_on) printf("\t\tif (ptc) { srcc=(uae_u32)*(uae_u16 *)ptc; ptc += 2; }\n");
	printf("\t\tdest = %s;\n", blitops[blttbl[i]].s);
	printf("\t\ttotald |= dest;\n");
	printf("\t\tif (ptd) { *(uae_u16 *)ptd= dest; ptd += 2; }\n");
	printf("\t}\n");
	if (a_is_on) printf("\tif (pta) pta += b->bltamod;\n");
	if (b_is_on) printf("\tif (ptb) ptb += b->bltbmod;\n");
	if (c_is_on) printf("\tif (ptc) ptc += b->bltcmod;\n");
	printf("\tif (ptd) ptd += b->bltdmod;\n");
	printf("}\n");
	if (a_is_on) printf("if (pta) b->bltadat = (*(pta-b->bltamod-2) << 8) | *(pta - b->bltamod - 1);\n"); /* Maybe not necessary, but I don't want problems */
	if (b_is_on) printf("if (ptb) b->bltbdat = (*(ptb-b->bltbmod-2) << 8) | *(ptb - b->bltbmod - 1);\n");
	if (c_is_on) printf("if (ptc) b->bltcdat = (*(ptc-b->bltcmod-2) << 8) | *(ptc - b->bltcmod - 1);\n");
	printf("if (ptd) b->bltddat = (*(ptd-b->bltdmod-2) << 8) | *(ptd - b->bltdmod - 1);\n");

	printf("} else {\n");
#endif
	if (a_is_on) printf("uae_u32 preva = 0;\n");
	if (b_is_on) printf("uae_u32 prevb = 0, srcb = b->bltbhold;\n");
	if (c_is_on) printf("uae_u32 srcc = b->bltcdat;\n");
	printf("uae_u32 dstd=0;\n");
	printf("uaecptr dstp = 0;\n");
	printf("for (j = 0; j < b->vblitsize; j++) {\n");
	printf("\tfor (i = 0; i < b->hblitsize; i++) {\n\t\tuae_u32 bltadat, srca;\n\n");
	if (c_is_on) printf("\t\tif (ptc) { srcc = chipmem_agnus_wget (ptc); ptc += 2; }\n");
	if (b_is_on) printf("\t\tif (ptb) {\n\t\t\tuae_u32 bltbdat = blt_info.bltbdat = chipmem_agnus_wget (ptb); ptb += 2;\n");
	if (b_is_on) printf("\t\t\tsrcb = (((uae_u32)prevb << 16) | bltbdat) >> b->blitbshift;\n");
	if (b_is_on) printf("\t\t\tprevb = bltbdat;\n\t\t}\n");
	if (a_is_on) printf("\t\tif (pta) { bltadat = blt_info.bltadat = chipmem_agnus_wget (pta); pta += 2; } else { bltadat = blt_info.bltadat; }\n");
	if (a_is_on) printf("\t\tbltadat &= blit_masktable[i];\n");
	if (a_is_on) printf("\t\tsrca = (((uae_u32)preva << 16) | bltadat) >> b->blitashift;\n");
	if (a_is_on) printf("\t\tpreva = bltadat;\n");
	printf("\t\tif (dstp) chipmem_agnus_wput (dstp, dstd);\n");
	printf("\t\tdstd = (%s) & 0xFFFF;\n", blitops[blttbl[i]].s);
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
	printf("\t\tif (dstp) chipmem_agnus_wput (dstp, dstd);\n");
#if 0
	printf("}\n");
#endif
	printf("if (totald != 0) b->blitzero = 0;\n");
	printf("}\n");

	printf("void blitdofast_desc_%x (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)\n",blttbl[i]);
	printf("{\n");
	printf("uae_u32 totald = 0;\n");
	printf("int i,j;\n");
#if 0
	printf("if (currprefs.blits_32bit_enabled && b->hblitsize > 1");
	if (a_is_on) printf(" && !b->blitashift && b->bltafwm==0xffff && b->bltalwm==0xffff");
	if (b_is_on) printf(" && !b->blitbshift");
	printf(") {\n");
	if (a_is_on) printf("uae_u32 srca = ((uae_u32)b->bltadat << 16) | b->bltadat;\n");
	if (b_is_on) printf("uae_u32 srcb = ((uae_u32)b->bltbdat << 16) | b->bltbdat;\n");
	if (c_is_on) printf("uae_u32 srcc = ((uae_u32)b->bltcdat << 16) | b->bltcdat;\n");
	printf("uae_u32 dest;\n");
	printf("int count=b->hblitsize/2, oddword=b->hblitsize&1;\n");
	printf("for (j=0;j<b->vblitsize;j++) {\n");
	printf("\tfor(i=0;i<count;i++) {\n");
	if (a_is_on) printf("\t\tif (pta) { srca=*((uae_u32 *)(pta-2)); pta -= 4;}\n");
	if (b_is_on) printf("\t\tif (ptb) { srcb=*((uae_u32 *)(ptb-2)); ptb -= 4;}\n");
	if (c_is_on) printf("\t\tif (ptc) { srcc=*((uae_u32 *)(ptc-2)); ptc -= 4;}\n");
	printf("\t\tdest = %s;\n", blitops[blttbl[i]].s);
	printf("\t\ttotald |= dest;\n");
	printf("\t\tif (ptd) {*(uae_u32 *)(ptd-2)=dest; ptd -= 4;}\n");
	printf("\t}\n");
	printf("\tif (oddword) {\n");
	if (a_is_on) printf("\t\tif (pta) { srca=(uae_u32)*(uae_u16 *)pta; pta -= 2; }\n");
	if (b_is_on) printf("\t\tif (ptb) { srcb=(uae_u32)*(uae_u16 *)ptb; ptb -= 2; }\n");
	if (c_is_on) printf("\t\tif (ptc) { srcc=(uae_u32)*(uae_u16 *)ptc; ptc -= 2; }\n");
	printf("\t\tdest = %s;\n", blitops[blttbl[i]].s);
	printf("\t\ttotald |= dest;\n");
	printf("\t\tif (ptd) { *(uae_u16 *)ptd= dest; ptd -= 2; }\n");
	printf("\t}\n");
	if (a_is_on) printf("\tif (pta) pta -= b->bltamod;\n");
	if (b_is_on) printf("\tif (ptb) ptb -= b->bltbmod;\n");
	if (c_is_on) printf("\tif (ptc) ptc -= b->bltcmod;\n");
	printf("\tif (ptd) ptd-=b->bltdmod;\n");
	printf("}\n");
	if (a_is_on) printf("if (pta) b->bltadat = (*(pta + b->bltamod + 2) << 8) | *(pta + b->bltamod + 1);\n"); /* Maybe not necessary, but I don't want problems */
	if (b_is_on) printf("if (ptb) b->bltbdat = (*(ptb + b->bltbmod + 2) << 8) | *(ptb + b->bltbmod + 1);\n");
	if (c_is_on) printf("if (ptc) b->bltcdat = (*(ptc + b->bltcmod + 2) << 8) | *(ptc + b->bltcmod + 1);\n");
	printf("if (ptd) b->bltddat = (*(ptd + b->bltdmod + 2) << 8) | *(ptd + b->bltdmod + 1);\n");

	printf("} else {\n");
#endif
	if (a_is_on) printf("uae_u32 preva = 0;\n");
	if (b_is_on) printf("uae_u32 prevb = 0, srcb = b->bltbhold;\n");
	if (c_is_on) printf("uae_u32 srcc = b->bltcdat;\n");
	printf("uae_u32 dstd=0;\n");
	printf("uaecptr dstp = 0;\n");
	printf("for (j = 0; j < b->vblitsize; j++) {\n");
	printf("\tfor (i = 0; i < b->hblitsize; i++) {\n\t\tuae_u32 bltadat, srca;\n");
	if (c_is_on) printf("\t\tif (ptc) { srcc = chipmem_agnus_wget (ptc); ptc -= 2; }\n");
	if (b_is_on) printf("\t\tif (ptb) {\n\t\t\tuae_u32 bltbdat = blt_info.bltbdat = chipmem_agnus_wget (ptb); ptb -= 2;\n");
	if (b_is_on) printf("\t\t\tsrcb = ((bltbdat << 16) | prevb) >> b->blitdownbshift;\n");
	if (b_is_on) printf("\t\t\tprevb = bltbdat;\n\t\t}\n");
	if (a_is_on) printf("\t\tif (pta) { bltadat = blt_info.bltadat = chipmem_agnus_wget (pta); pta -= 2; } else { bltadat = blt_info.bltadat; }\n");
	if (a_is_on) printf("\t\tbltadat &= blit_masktable[i];\n");
	if (a_is_on) printf("\t\tsrca = (((uae_u32)bltadat << 16) | preva) >> b->blitdownashift;\n");
	if (a_is_on) printf("\t\tpreva = bltadat;\n");
	printf("\t\tif (dstp) chipmem_agnus_wput (dstp, dstd);\n");
	printf("\t\tdstd = (%s) & 0xFFFF;\n", blitops[blttbl[i]].s);
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
	printf("\t\tif (dstp) chipmem_agnus_wput (dstp, dstd);\n");
#if 0
	printf("}\n");
#endif
	printf("if (totald != 0) b->blitzero = 0;\n");
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
    printf("#include \"custom.h\"\n");
    printf("#include \"memory.h\"\n");
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

