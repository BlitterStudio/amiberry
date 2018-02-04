/*
* E-UAE - The portable Amiga Emulator
*
* Generate pixel output code.
*
* (c) 2006 Richard Drummond
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

/* Output for big-endian target if true, little-endian is false. */
int do_bigendian;

typedef int DEPTH_T;

#define DEPTH_8BPP 0
#define DEPTH_16BPP 1
#define DEPTH_32BPP 2
#define DEPTH_MAX DEPTH_32BPP

static const char *get_depth_str (DEPTH_T bpp)
{
	if (bpp == DEPTH_8BPP)
		return "8";
	else if (bpp == DEPTH_16BPP)
		return "16";
	else
		return "32";
}

static const char *get_depth_type_str (DEPTH_T bpp)
{
	if (bpp == DEPTH_8BPP)
		return "uae_u8";
	else if (bpp == DEPTH_16BPP)
		return "uae_u16";
	else
		return "uae_u32";
}

typedef int HMODE_T;

#define HMODE_NORMAL 0
#define HMODE_DOUBLE 1
#define HMODE_DOUBLE2X 2
#define HMODE_HALVE1 3
#define HMODE_HALVE1F 4
#define HMODE_HALVE2 5
#define HMODE_HALVE2F 6
#define HMODE_MAX HMODE_HALVE2F

static const char *get_hmode_str (HMODE_T hmode)
{
	if (hmode == HMODE_DOUBLE)
		return "_stretch1";
	else if (hmode == HMODE_DOUBLE2X)
		return "_stretch2";
	else if (hmode == HMODE_HALVE1)
		return "_shrink1";
	else if (hmode == HMODE_HALVE1F)
		return "_shrink1f";
	else if (hmode == HMODE_HALVE2)
		return "_shrink2";
	else if (hmode == HMODE_HALVE2F)
		return "_shrink2f";
	else
		return "";
}


typedef enum
{
	CMODE_NORMAL,
	CMODE_DUALPF,
	CMODE_EXTRAHB,
	CMODE_HAM
} CMODE_T;
#define CMODE_MAX CMODE_HAM


static FILE *outfile;
static unsigned int outfile_indent = 0;

static void set_outfile (FILE *f)
{
	outfile = f;
}

static int set_indent (int indent)
{
	int old_indent = outfile_indent;
	outfile_indent = indent;
	return old_indent;
}

static void outindent(void)
{
	unsigned int i;
	for (i = 0; i < outfile_indent; i++)
		fputc(' ', outfile);
}

static void outf(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	vfprintf(outfile, s, ap);
}

static void outln (const char *s)
{
	outindent();
	fprintf (outfile, "%s\n", s);
}

static void outlnf (const char *s, ...)
{
	va_list ap;
	outindent();
	va_start (ap, s);
	vfprintf (outfile, s, ap);
	fputc ('\n', outfile);
}

static void out_linetoscr_decl (DEPTH_T bpp, HMODE_T hmode, int aga, int spr, int genlock)
{
	outlnf ("static int NOINLINE linetoscr_%s%s%s%s%s(int spix, int dpix, int dpix_end)",
		get_depth_str (bpp),
		get_hmode_str (hmode), aga ? "_aga" : "", spr > 0 ? "_spr" : (spr < 0 ? "_spronly" : ""), genlock ? "_genlock" : "");
}

static void out_linetoscr_do_srcpix (DEPTH_T bpp, HMODE_T hmode, int aga, CMODE_T cmode, int spr)
{
	if (spr < 0) {
		outln (     "    sprpix_val = 0;");
	} else {
		if (aga && cmode != CMODE_DUALPF) {
			if (spr)
				outln (     "    sprpix_val = pixdata.apixels[spix];");
			if (cmode != CMODE_HAM)
				outln ( 	"    spix_val = (pixdata.apixels[spix] ^ xor_val) & and_val;");
		} else if (cmode != CMODE_HAM) {
			outln ( 	"    spix_val = pixdata.apixels[spix];");
			if (spr)
				outln (     "    sprpix_val = spix_val;");
		}
	}
}

static void out_linetoscr_do_dstpix (DEPTH_T bpp, HMODE_T hmode, int aga, CMODE_T cmode, int spr)
{
	if (spr < 0)
		return;
	if (aga && cmode == CMODE_HAM) {
		outln (	    "    spix_val = ham_linebuf[spix];");
		outln (	    "    dpix_val = CONVERT_RGB (spix_val);");
	} else if (cmode == CMODE_HAM) {
		outln (		"    spix_val = ham_linebuf[spix];");
		outln ( "    dpix_val = p_xcolors[spix_val];");
		if (spr)
			outln ( "    sprpix_val = pixdata.apixels[spix];");
	} else if (aga && cmode == CMODE_DUALPF) {
		outln (     "    {");
		outln (		"        uae_u8 val = lookup[spix_val];");
		outln (		"        if (lookup_no[spix_val])");
		outln (		"            val += dblpfofs[bpldualpf2of];");
		outln (		"        val ^= xor_val;");
		outln (		"        dpix_val = p_acolors[val];");
		outln (		"    }");
	} else if (cmode == CMODE_DUALPF) {
		outln (		"    dpix_val = p_acolors[lookup[spix_val]];");
	} else if (aga && cmode == CMODE_EXTRAHB) {
		outln (		"    if (pixdata.apixels[spix] & 0x20) {");
		outln (		"        unsigned int c = (colors_for_drawing.color_regs_aga[spix_val & 0x1f] >> 1) & 0x7F7F7F;");
		outln (		"        dpix_val = CONVERT_RGB (c);");
		outln (		"    } else");
		outln (		"        dpix_val = p_acolors[spix_val];");
	} else if (cmode == CMODE_EXTRAHB) {
		outln (		"    if (spix_val <= 31)");
		outln (		"        dpix_val = p_acolors[spix_val];");
		outln (		"    else");
		outln (		"        dpix_val = p_xcolors[(colors_for_drawing.color_regs_ecs[spix_val - 32] >> 1) & 0x777];");
	} else
		outln (		"    dpix_val = p_acolors[spix_val];");
}

static void out_linetoscr_do_incspix (DEPTH_T bpp, HMODE_T hmode, int aga, CMODE_T cmode, int spr)
{
	if (spr < 0) {
		outln("    spix++;");
		return;
	}
	if (hmode == HMODE_HALVE1F) {
		outln (         "    {");
		outln (         "    uae_u32 tmp_val;");
		outln (		"    spix++;");
		outln (		"    tmp_val = dpix_val;");
		out_linetoscr_do_srcpix (bpp, hmode, aga, cmode, spr);
		out_linetoscr_do_dstpix (bpp, hmode, aga, cmode, spr);
		outlnf (	"    dpix_val = merge_2pixel%d (dpix_val, tmp_val);", bpp == 0 ? 8 : bpp == 1 ? 16 : 32);
		outln (		"    spix++;");
		outln (         "    }");
	} else if (hmode == HMODE_HALVE2F) {
		outln (         "    {");
		outln (         "    uae_u32 tmp_val, tmp_val2, tmp_val3;");
		outln (		"    spix++;");
		outln (		"    tmp_val = dpix_val;");
		out_linetoscr_do_srcpix (bpp, hmode, aga, cmode, spr);
		out_linetoscr_do_dstpix (bpp, hmode, aga, cmode, spr);
		outln (		"    spix++;");
		outln (		"    tmp_val2 = dpix_val;");
		out_linetoscr_do_srcpix (bpp, hmode, aga, cmode, spr);
		out_linetoscr_do_dstpix (bpp, hmode, aga, cmode, spr);
		outln (		"    spix++;");
		outln (		"    tmp_val3 = dpix_val;");
		out_linetoscr_do_srcpix (bpp, hmode, aga, cmode, spr);
		out_linetoscr_do_dstpix (bpp, hmode, aga, cmode, spr);
		outlnf (        "    tmp_val = merge_2pixel%d (tmp_val, tmp_val2);", bpp == 0 ? 8 : bpp == 1 ? 16 : 32);
		outlnf (        "    tmp_val2 = merge_2pixel%d (tmp_val3, dpix_val);", bpp == 0 ? 8 : bpp == 1 ? 16 : 32);
		outlnf (	"    dpix_val = merge_2pixel%d (tmp_val, tmp_val2);", bpp == 0 ? 8 : bpp == 1 ? 16 : 32);
		outln (		"    spix++;");
		outln (         "    }");
	} else if (hmode == HMODE_HALVE1) {
		outln (		"    spix += 2;");
	} else if (hmode == HMODE_HALVE2) {
		outln (		"    spix += 4;");
	} else {
		outln (		"    spix++;");
	}
}

static void put_dpixsprgenlock(int offset, int genlock)
{
	if (!genlock)
		return;
	if (offset)
		outlnf("            genlock_buf[dpix + %d] = get_genlock_transparency(sprcol);", offset);
	else
		outlnf("            genlock_buf[dpix] = get_genlock_transparency(sprcol);");
}

static void put_dpixgenlock(int offset, CMODE_T cmode, int aga, int genlock, const char *var2)
{
	if (!genlock)
		return;
	outindent();
	if (offset)
		outf("    genlock_buf[dpix + %d] = get_genlock_transparency(", offset);
	else
		outf("    genlock_buf[dpix] = get_genlock_transparency(");

	if (genlock) {
		if (cmode == CMODE_EXTRAHB) {
			outf("%s", var2 ? var2 : "spix_val & 31");
		}
		else if (cmode == CMODE_DUALPF) {
			outf("%s", var2 ? var2 : "lookup[spix_val]");
		}
		else if (cmode == CMODE_HAM) {
			if (aga) {
				outf("%s", var2 ? var2 : "(spix_val >> 2) & 63");
			}
			else {
				outf("%s", var2 ? var2 : "spix_val & 15");
			}
		}
		else {
			outf("%s", var2 ? var2 : "spix_val");
		}
	}
	outf(");\n");
}

static void put_dpix (const char *var)
{
	outlnf("    buf[dpix++] = %s;", var);
}

static void out_sprite (DEPTH_T bpp, HMODE_T hmode, CMODE_T cmode, int aga, int cnt, int spr, int genlock)
{
	if (aga) {
		if (cnt == 1) {
			outlnf ( "    if (spritepixels[dpix].data) {");
			outlnf ( "        sprcol = render_sprites (dpix + 0, %d, sprpix_val, %d);", cmode == CMODE_DUALPF ? 1 : 0, aga);
			outlnf("        if (sprcol) {");
			outlnf ( "            out_val = p_acolors[sprcol];");
			put_dpixsprgenlock(0, genlock);
			outlnf("        }");
			outlnf("    }");
			put_dpix("out_val");
		} else if (cnt == 2) {
			outlnf ( "    {");
			outlnf ( "    uae_u32 out_val1 = out_val;");
			outlnf ( "    uae_u32 out_val2 = out_val;");
			outlnf("    if (spritepixels[dpix + 0].data) {");
			outlnf ( "        sprcol = render_sprites (dpix + 0, %d, sprpix_val, %d);", cmode == CMODE_DUALPF ? 1 : 0, aga);
			outlnf ( "        if (sprcol) {");
			outlnf ( "            out_val1 = p_acolors[sprcol];");
			put_dpixsprgenlock(0, genlock);
			outlnf("        }");
			outlnf("    }");
			outlnf ( "    if (spritepixels[dpix + 1].data) {");
			outlnf ( "        sprcol = render_sprites (dpix + 1, %d, sprpix_val, %d);", cmode == CMODE_DUALPF ? 1 : 0, aga);
			outlnf ( "        if (sprcol) {");
			outlnf ( "            out_val2 = p_acolors[sprcol];");
			put_dpixsprgenlock(1, genlock);
			outlnf("        }");
			outlnf("    }");
			put_dpix("out_val1");
			put_dpix("out_val2");
			outlnf ( "    }");
		} else if (cnt == 4) {
			outlnf ( "    {");
			outlnf ( "    uae_u32 out_val1 = out_val;");
			outlnf ( "    uae_u32 out_val2 = out_val;");
			outlnf ( "    uae_u32 out_val3 = out_val;");
			outlnf ( "    uae_u32 out_val4 = out_val;");
			outlnf("    if (spritepixels[dpix + 0].data) {");
			outlnf ( "        sprcol = render_sprites (dpix + 0, %d, sprpix_val, %d);", cmode == CMODE_DUALPF ? 1 : 0, aga);
			outlnf ( "        if (sprcol) {");
			outlnf ( "            out_val1 = p_acolors[sprcol];");
			put_dpixsprgenlock(0, genlock);
			outlnf("        }");
			outlnf("    }");
			outlnf ( "    if (spritepixels[dpix + 1].data) {");
			outlnf ( "        sprcol = render_sprites (dpix + 1, %d, sprpix_val, %d);", cmode == CMODE_DUALPF ? 1 : 0, aga);
			outlnf ( "        if (sprcol) {");
			outlnf ( "            out_val2 = p_acolors[sprcol];");
			put_dpixsprgenlock(1, genlock);
			outlnf("        }");
			outlnf("    }");
			outlnf ( "    if (spritepixels[dpix + 2].data) {");
			outlnf ( "        sprcol = render_sprites (dpix + 2, %d, sprpix_val, %d);", cmode == CMODE_DUALPF ? 1 : 0, aga);
			outlnf ( "        if (sprcol) {");
			outlnf ( "            out_val3 = p_acolors[sprcol];");
			put_dpixsprgenlock(2, genlock);
			outlnf("        }");
			outlnf("    }");
			outlnf ( "    if (spritepixels[dpix + 3].data) {");
			outlnf ( "        sprcol = render_sprites (dpix + 3, %d, sprpix_val, %d);", cmode == CMODE_DUALPF ? 1 : 0, aga);
			outlnf ( "        if (sprcol) {");
			outlnf ( "            out_val4 = p_acolors[sprcol];");
			put_dpixsprgenlock(3, genlock);
			outlnf("        }");
			outlnf("    }");
			put_dpix("out_val1");
			put_dpix("out_val2");
			put_dpix("out_val3");
			put_dpix("out_val4");
			outlnf ( "    }");
		}
	} else {
		outlnf ( "    if (spritepixels[dpix].data) {");
		outlnf ( "        sprcol = render_sprites (dpix, %d, sprpix_val, %d);", cmode == CMODE_DUALPF ? 1 : 0, aga);
		put_dpixsprgenlock(0, genlock);
		outlnf("        if (sprcol) {");
		outlnf ( "            uae_u32 spcol = p_acolors[sprcol];");
		outlnf ( "            out_val = spcol;");
		outlnf ( "        }");
		outlnf ( "    }");
		while (cnt-- > 0)
			put_dpix("out_val");
	}
}


static void out_linetoscr_mode (DEPTH_T bpp, HMODE_T hmode, int aga, int spr, CMODE_T cmode, int genlock)
{
	int old_indent = set_indent (8);

	if (aga && cmode == CMODE_DUALPF) {
		outln (        "int *lookup    = bpldualpfpri ? dblpf_ind2_aga : dblpf_ind1_aga;");
		outln (        "int *lookup_no = bpldualpfpri ? dblpf_2nd2     : dblpf_2nd1;");
	} else if (cmode == CMODE_DUALPF)
		outln (        "int *lookup = bpldualpfpri ? dblpf_ind2 : dblpf_ind1;");

	if (bpp == DEPTH_16BPP && hmode != HMODE_DOUBLE && hmode != HMODE_DOUBLE2X && spr == 0) {
		outln (		"int rem;");
		outln (		"if (((uintptr_t)&buf[dpix]) & 2) {");
		outln (		"    uae_u32 spix_val;");
		outln (		"    uae_u32 dpix_val;");

		out_linetoscr_do_srcpix (bpp, hmode, aga, cmode, spr);
		out_linetoscr_do_dstpix (bpp, hmode, aga, cmode, spr);
		out_linetoscr_do_incspix (bpp, hmode, aga, cmode, spr);

		put_dpix("dpix_val");
		outln (		"}");
		outln (		"if (dpix >= dpix_end)");
		outln (		"    return spix;");
		outln (		"rem = (((uintptr_t)&buf[dpix_end]) & 2);");
		outln (		"if (rem)");
		outln (		"    dpix_end--;");
	}

	outln (		"while (dpix < dpix_end) {");
	if (spr)
		outln (		"    uae_u32 sprpix_val;");
	if (spr >= 0) {
		outln (		"    uae_u32 spix_val;");
		outln (		"    uae_u32 dpix_val;");
	}
	outln (		"    uae_u32 out_val;");
	outln (		"");

	out_linetoscr_do_srcpix (bpp, hmode, aga, cmode, spr);
	out_linetoscr_do_dstpix (bpp, hmode, aga, cmode, spr);
	out_linetoscr_do_incspix (bpp, hmode, aga, cmode, spr);

	if (spr >= 0)
		outln (		"    out_val = dpix_val;");
	else
		outln (		"    out_val = p_acolors[0];");

	if (hmode == HMODE_DOUBLE) {
		put_dpixgenlock(0, cmode, aga, genlock, NULL);
		put_dpixgenlock(1, cmode, aga, genlock, NULL);
	} else if (hmode == HMODE_DOUBLE2X) {
		put_dpixgenlock(0, cmode, aga, genlock, NULL);
		put_dpixgenlock(1, cmode, aga, genlock, NULL);
		put_dpixgenlock(2, cmode, aga, genlock, NULL);
		put_dpixgenlock(3, cmode, aga, genlock, NULL);
	} else {
		put_dpixgenlock(0, cmode, aga, genlock, NULL);
	}

	if (hmode != HMODE_DOUBLE && hmode != HMODE_DOUBLE2X && bpp == DEPTH_16BPP && spr == 0) {
		out_linetoscr_do_srcpix (bpp, hmode, aga, cmode, spr);
		out_linetoscr_do_dstpix (bpp, hmode, aga, cmode, spr);
		out_linetoscr_do_incspix (bpp, hmode, aga, cmode, spr);

		if (do_bigendian)
			outln (	"    out_val = (out_val << 16) | (dpix_val & 0xFFFF);");
		else
			outln (	"    out_val = (out_val & 0xFFFF) | (dpix_val << 16);");
	}

	if (hmode == HMODE_DOUBLE) {
		if (bpp == DEPTH_8BPP) {
			outln (	"    *((uae_u16 *)&buf[dpix]) = (uae_u16) out_val;");
			outln (	"    dpix += 2;");
		} else if (bpp == DEPTH_16BPP) {
			if (spr) {
				out_sprite(bpp, hmode, cmode, aga, 2, spr, genlock);
			} else {
				outln (	"    *((uae_u32 *)&buf[dpix]) = out_val;");
				outln (	"    dpix += 2;");
			}
		} else {
			if (spr) {
				out_sprite(bpp, hmode, cmode, aga, 2, spr, genlock);
			} else {
				put_dpix("out_val");
				put_dpix("out_val");
			}
		}
	} else if (hmode == HMODE_DOUBLE2X) {
		if (bpp == DEPTH_8BPP) {
			outln (	"    *((uae_u32 *)&buf[dpix]) = (uae_u32) out_val;");
			outln (	"    dpix += 4;");
		} else if (bpp == DEPTH_16BPP) {
			if (spr) {
				out_sprite(bpp, hmode, cmode, aga, 4, spr, genlock);
			} else {
				outln (	"    *((uae_u32 *)&buf[dpix]) = out_val;");
				outln (	"    dpix += 2;");
				outln (	"    *((uae_u32 *)&buf[dpix]) = out_val;");
				outln (	"    dpix += 2;");
			}
		} else {
			if (spr) {
				out_sprite(bpp, hmode, cmode, aga, 4, spr, genlock);
			} else {
				put_dpix("out_val");
				put_dpix("out_val");
				put_dpix("out_val");
				put_dpix("out_val");
			}
		}
	} else {
		if (bpp == DEPTH_16BPP) {
			if (spr) {
				out_sprite(bpp, hmode, cmode, aga, 1, spr, genlock);
			} else {
				outln (	"    *((uae_u32 *)&buf[dpix]) = out_val;");
				outln (	"    dpix += 2;");
			}
		} else {
			if (spr) {
				out_sprite(bpp, hmode, cmode, aga, 1, spr, genlock);
			} else {
				put_dpix("out_val");
			}
		}
	}

	outln (		"}");


	if (bpp == DEPTH_16BPP && hmode != HMODE_DOUBLE && hmode != HMODE_DOUBLE2X && spr == 0) {
		outln (		"if (rem) {");
		outln (		"    uae_u32 spix_val;");
		outln (		"    uae_u32 dpix_val;");

		out_linetoscr_do_srcpix (bpp, hmode, aga, cmode, spr);
		out_linetoscr_do_dstpix (bpp, hmode, aga, cmode, spr);
		out_linetoscr_do_incspix (bpp, hmode, aga, cmode, spr);

		put_dpix("dpix_val");
		outln (		"}");
	}

	set_indent (old_indent);

	return;
}

static void out_linetoscr (DEPTH_T bpp, HMODE_T hmode, int aga, int spr, int genlock)
{
	if (aga)
		outln  ("#ifdef AGA");

	out_linetoscr_decl (bpp, hmode, aga, spr, genlock);
	outln  (	"{");

	outlnf (	"    %s *buf = (%s *) xlinebuffer;", get_depth_type_str (bpp), get_depth_type_str (bpp));
	if (genlock)
		outlnf("    uae_u8 *genlock_buf = xlinebuffer_genlock;");
	if (spr)
		outln ( "    uae_u8 sprcol;");
	if (aga && spr >= 0) {
		outln("    uae_u8 xor_val = bplxor;");
		outln("    uae_u8 and_val = bpland;");
	}
	outln  (	"");

	if (spr >= 0) {
		outln  (	"    if (bplham) {");
		out_linetoscr_mode(bpp, hmode, aga, spr, CMODE_HAM, genlock);
		outln  (	"    } else if (bpldualpf) {");
		out_linetoscr_mode(bpp, hmode, aga, spr, CMODE_DUALPF, genlock);
		outln  (	"    } else if (bplehb) {");
		out_linetoscr_mode(bpp, hmode, aga, spr, CMODE_EXTRAHB, genlock);
		outln  (	"    } else {");
		out_linetoscr_mode(bpp, hmode, aga, spr, CMODE_NORMAL, genlock);
	} else {
		outln  (	"    if (1) {");
		out_linetoscr_mode(bpp, hmode, aga, spr, CMODE_NORMAL, genlock);
	}

	outln  (	"    }\n");
	outln  (	"    return spix;");
	outln  (	"}");

	if (aga)
		outln (	"#endif");
	outln  (	"");
}

int main (int argc, char *argv[])
{
	DEPTH_T bpp;
	int aga, spr;
	HMODE_T hmode;

	do_bigendian = 0;

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] != '-')
			continue;
		if (argv[i][1] == 'b' && argv[i][2] == '\0')
			do_bigendian = 1;
	}

	set_outfile (stdout);

	outln ("/*");
	outln (" * UAE - The portable Amiga emulator.");
	outln (" *");
	outln (" * This file was generated by genlinetoscr. Don't edit.");
	outln (" */");
	outln ("");

	for (bpp = DEPTH_16BPP; bpp <= DEPTH_MAX; bpp++) {
		for (aga = 0; aga <= 1 ; aga++) {
			if (aga && bpp == DEPTH_8BPP)
				continue;
			for (spr = -1; spr <= 1; spr++) {
				if (!aga && spr < 0)
					continue;
				for (hmode = HMODE_NORMAL; hmode <= HMODE_MAX; hmode++) {
					out_linetoscr(bpp, hmode, aga, spr, 0);
					if (spr >= 0)
						out_linetoscr(bpp, hmode, aga, spr, 1);
				}
			}
		}
	}
	return 0;
}
