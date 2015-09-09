/* ------------------------------------------------------------------------ */
/* LHa for UNIX    															*/
/*				maketbl.c -- makes decoding table							*/
/*																			*/
/*		Modified          		Nobutaka Watazaki							*/
/*																			*/
/*	Ver. 1.14 	Source All chagned				1995.01.14	N.Watazaki		*/
/* ------------------------------------------------------------------------ */
#include "lha.h"

void
lha_make_table(short nchar, unsigned char bitlen[], short tablebits, unsigned short table[])
{
	unsigned short  count[17];	/* count of bitlen */
	unsigned short  weight[17];	/* 0x10000ul >> bitlen */
	unsigned short  start[17];	/* first code of bitlen */
	unsigned short  total;
	unsigned int    i, l;
	int             j, k, m, n, avail;
	unsigned short *p;

	avail = nchar;

	/* initialize */
	for (i = 1; i <= 16; i++) {
		count[i] = 0;
		weight[i] = 1 << (16 - i);
	}

	/* count */
	for (i = 0; i < nchar; i++)
		count[bitlen[i]]++;

	/* calculate first code */
	total = 0;
	for (i = 1; i <= 16; i++) {
		start[i] = total;
		total += weight[i] * count[i];
	}
	if ((total & 0xffff) != 0)
		error("make_table()", "Bad table (5)\n");

	/* shift data for make table. */
	m = 16 - tablebits;
	for (i = 1; i <= tablebits; i++) {
		start[i] >>= m;
		weight[i] >>= m;
	}

	/* initialize */
	j = start[tablebits + 1] >> m;
	k = 1 << tablebits;
	if (j != 0)
		for (i = j; i < k; i++)
			table[i] = 0;

	/* create table and tree */
	for (j = 0; j < nchar; j++) {
		k = bitlen[j];
		if (k == 0)
			continue;
		l = start[k] + weight[k];
		if (k <= tablebits) {
			/* code in table */
			for (i = start[k]; i < l; i++)
				table[i] = j;
		}
		else {
			/* code not in table */
			p = &table[(i = start[k]) >> m];
			i <<= tablebits;
			n = k - tablebits;
			/* make tree (n length) */
			while (--n >= 0) {
				if (*p == 0) {
					lha_right[avail] = lha_left[avail] = 0;
					*p = avail++;
				}
				if (i & 0x8000)
					p = &lha_right[*p];
				else
					p = &lha_left[*p];
				i <<= 1;
			}
			*p = j;
		}
		start[k] = l;
	}
}

/* Local Variables: */
/* mode:c */
/* tab-width:4 */
/* End: */
