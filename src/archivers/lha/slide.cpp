/* ------------------------------------------------------------------------ */
/* LHa for UNIX    															*/
/*				slice.c -- sliding dictionary with percolating update		*/
/*																			*/
/*		Modified          		Nobutaka Watazaki							*/
/*																			*/
/*	Ver. 1.14d 	Exchanging a search algorithm  1997.01.11    T.Okamoto      */
/* ------------------------------------------------------------------------ */

#include "lha.h"


/* ------------------------------------------------------------------------ */

static unsigned long encoded_origsize;

/* ------------------------------------------------------------------------ */

static unsigned int *hash;
static unsigned int *prev;

/* static unsigned char  *text; */
unsigned char *too_flag;

static struct decode_option decode_define[] = {
	/* lh1 */
	{decode_c_dyn, decode_p_st0, decode_start_fix},
	/* lh2 */
	{decode_c_dyn, decode_p_dyn, decode_start_dyn},
	/* lh3 */
	{decode_c_st0, decode_p_st0, decode_start_st0},
	/* lh4 */
	{decode_c_st1, decode_p_st1, decode_start_st1},
	/* lh5 */
	{decode_c_st1, decode_p_st1, decode_start_st1},
	/* lh6 */
	{decode_c_st1, decode_p_st1, decode_start_st1},
	/* lh7 */
	{decode_c_st1, decode_p_st1, decode_start_st1},
	/* lzs */
	{decode_c_lzs, decode_p_lzs, decode_start_lzs},
	/* lz5 */
	{decode_c_lz5, decode_p_lz5, decode_start_lz5}
};

static struct encode_option encode_set;
static struct decode_option decode_set;

#ifdef SUPPORT_LH7
#define DICSIZ (1L << 16)
#define TXTSIZ (DICSIZ * 2L + MAXMATCH)
#else
#define DICSIZ (((unsigned long)1) << 15)
#define TXTSIZ (DICSIZ * 2 + MAXMATCH)
#endif

#define HSHSIZ (((unsigned long)1) <<15)
#define NIL 0
#define LIMIT 0x100	/* chain Ä¹¤Î limit */

static unsigned int txtsiz;

static unsigned long dicsiz;

static unsigned int hval;
static int matchlen;
static unsigned int matchpos;
static unsigned int pos;
static unsigned int remainderlh;

int decode(struct interfacing *lhinterface)
{
	unsigned int i, j, k, c;
	unsigned int dicsiz1, offset;
	unsigned char *dtext;


	infile = lhinterface->infile;
	outfile = lhinterface->outfile;
	dicbit = lhinterface->dicbit;
	origsize = lhinterface->original;
	compsize = lhinterface->packed;
	decode_set = decode_define[lhinterface->method - 1];

	crc = 0;
	prev_char = -1;
	dicsiz = 1L << dicbit;
	dtext = (unsigned char *) malloc(dicsiz);
	if (dtext == NULL)
	    return 0;
	for (i=0; i<dicsiz; i++) dtext[i] = 0x20;
	decode_set.decode_start();
	dicsiz1 = dicsiz - 1;
	offset = (lhinterface->method == LARC_METHOD_NUM) ? 0x100 - 2 : 0x100 - 3;
	lhcount = 0;
	loc = 0;
	while (lhcount < origsize) {
		c = decode_set.decode_c();
		if (c <= UCHAR_MAX) {
			dtext[loc++] = c;
			if (loc == dicsiz) {
				fwrite_crc(dtext, dicsiz, outfile);
				loc = 0;
			}
			lhcount++;
		}
		else {
			j = c - offset;
			i = (loc - decode_set.decode_p() - 1) & dicsiz1;
			lhcount += j;
			for (k = 0; k < j; k++) {
				c = dtext[(i + k) & dicsiz1];

				dtext[loc++] = c;
				if (loc == dicsiz) {
					fwrite_crc(dtext, dicsiz, outfile);
					loc = 0;
				}
			}
		}
	}
	if (loc != 0) {
		fwrite_crc(dtext, loc, outfile);
	}

	free(dtext);
    return 1;
}

/* Local Variables: */
/* mode:c */
/* tab-width:4 */
/* End: */
