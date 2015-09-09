/* ------------------------------------------------------------------------ */
/* LHa for UNIX    															*/
/*				larc.c -- extra *.lzs										*/
/*																			*/
/*		Modified          		Nobutaka Watazaki							*/
/*																			*/
/*	Ver. 1.14 	Source All chagned				1995.01.14	N.Watazaki		*/
/* ------------------------------------------------------------------------ */
#include "lha.h"

/* ------------------------------------------------------------------------ */
static int      flag, flagcnt, matchpos;
/* ------------------------------------------------------------------------ */
unsigned short
decode_c_lzs( /*void*/ )
{
	if (getbits(1)) {
		return getbits(8);
	}
	else {
		matchpos = getbits(11);
		return getbits(4) + 0x100;
	}
}

/* ------------------------------------------------------------------------ */
unsigned short
decode_p_lzs( /*void*/ )
{
	return (loc - matchpos - MAGIC0) & 0x7ff;
}

/* ------------------------------------------------------------------------ */
void
decode_start_lzs( /*void*/ )
{
	init_getbits();
}

/* ------------------------------------------------------------------------ */
unsigned short
decode_c_lz5( /*void*/ )
{
	int             c;

	if (flagcnt == 0) {
		flagcnt = 8;
		flag = zfile_getc(infile);
	}
	flagcnt--;
	c = zfile_getc(infile);
	if ((flag & 1) == 0) {
		matchpos = c;
		c = zfile_getc(infile);
		matchpos += (c & 0xf0) << 4;
		c &= 0x0f;
		c += 0x100;
	}
	flag >>= 1;
	return c;
}

/* ------------------------------------------------------------------------ */
unsigned short
decode_p_lz5( /*void*/ )
{
	return (loc - matchpos - MAGIC5) & 0xfff;
}

/* ------------------------------------------------------------------------ */
void
decode_start_lz5( /*void*/ )
{
	int             i;

	flagcnt = 0;
	for (i = 0; i < 256; i++)
		memset(&lha_text[i * 13 + 18], i, 13);
	for (i = 0; i < 256; i++)
		lha_text[256 * 13 + 18 + i] = i;
	for (i = 0; i < 256; i++)
		lha_text[256 * 13 + 256 + 18 + i] = 255 - i;
	memset(&lha_text[256 * 13 + 512 + 18], 0, 128);
	memset(&lha_text[256 * 13 + 512 + 128 + 18], ' ', 128 - 18);
}
