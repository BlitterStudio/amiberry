/* ------------------------------------------------------------------------ */
/* LHa for UNIX    															*/
/*				huf.c -- new static Huffman									*/
/*																			*/
/*		Modified          		Nobutaka Watazaki							*/
/*																			*/
/*	Ver. 1.14 	Source All chagned				1995.01.14	N.Watazaki		*/
/*	Ver. 1.14i 	Support LH7 & Bug Fixed			2000.10. 6	t.okamoto		*/
/* ------------------------------------------------------------------------ */
#include "lha.h"

#ifdef sony_news
#include <sys/param.h>
#endif

#if defined(__STDC__) || defined(NEWSOS)
#include <stdlib.h>
#endif

/* ------------------------------------------------------------------------ */
unsigned short  h_left[2 * NC - 1], h_right[2 * NC - 1];
unsigned char   c_len[NC], pt_len[NPT];
unsigned short  c_freq[2 * NC - 1], c_table[4096], c_code[NC], p_freq[2 * NP - 1],
		pt_table[256], pt_code[NPT], t_freq[2 * NT - 1];

static unsigned char *buf;
static unsigned int bufsiz;
static unsigned short blocksize;
static unsigned short output_pos, output_mask;
static 			int	  pbit;
static 			int	  np;
/* ------------------------------------------------------------------------ */
/*								Encording									*/
/* ------------------------------------------------------------------------ */
static void count_t_freq(void)
{
	short           i, k, n, count;

	for (i = 0; i < NT; i++)
		t_freq[i] = 0;
	n = NC;
	while (n > 0 && c_len[n - 1] == 0)
		n--;
	i = 0;
	while (i < n) {
		k = c_len[i++];
		if (k == 0) {
			count = 1;
			while (i < n && c_len[i] == 0) {
				i++;
				count++;
			}
			if (count <= 2)
				t_freq[0] += count;
			else if (count <= 18)
				t_freq[1]++;
			else if (count == 19) {
				t_freq[0]++;
				t_freq[1]++;
			}
			else
				t_freq[2]++;
		} else
			t_freq[k + 2]++;
	}
}

/* ------------------------------------------------------------------------ */
unsigned char  *alloc_buf(void)
{
	bufsiz = 16 * 1024 *2;	/* 65408U; */ /* t.okamoto */
	while ((buf = (unsigned char *) malloc(bufsiz)) == NULL) {
		bufsiz = (bufsiz / 10) * 9;
		if (bufsiz < 4 * 1024)
			break;
	}
	return buf;
}

/* ------------------------------------------------------------------------ */
/*								decoding									*/
/* ------------------------------------------------------------------------ */
static void read_pt_len(short nn, short nbit, short i_special)
{
	int           i, c, n;

	n = getbits(nbit);
	if (n == 0) {
		c = getbits(nbit);
		for (i = 0; i < nn; i++)
			pt_len[i] = 0;
		for (i = 0; i < 256; i++)
			pt_table[i] = c;
	}
	else {
		i = 0;
		while (i < n) {
			c = lhabitbuf >> (16 - 3);
			if (c == 7) {
				unsigned short  mask = 1 << (16 - 4);
				while (mask & lhabitbuf) {
					mask >>= 1;
					c++;
				}
			}
			fillbuf((c < 7) ? 3 : c - 3);
			pt_len[i++] = c;
			if (i == i_special) {
				c = getbits(2);
				while (--c >= 0)
					pt_len[i++] = 0;
			}
		}
		while (i < nn)
			pt_len[i++] = 0;
		lha_make_table(nn, pt_len, 8, pt_table);
	}
}

/* ------------------------------------------------------------------------ */
static void read_c_len(void)
{
	short           i, c, n;

	n = getbits(CBIT);
	if (n == 0) {
		c = getbits(CBIT);
		for (i = 0; i < NC; i++)
			c_len[i] = 0;
		for (i = 0; i < 4096; i++)
			c_table[i] = c;
	} else {
		i = 0;
		while (i < n) {
			c = pt_table[lhabitbuf >> (16 - 8)];
			if (c >= NT) {
				unsigned short  mask = 1 << (16 - 9);
				do {
					if (lhabitbuf & mask)
						c = h_right[c];
					else
						c = h_left[c];
					mask >>= 1;
				} while (c >= NT);
			}
			fillbuf(pt_len[c]);
			if (c <= 2) {
				if (c == 0)
					c = 1;
				else if (c == 1)
					c = getbits(4) + 3;
				else
					c = getbits(CBIT) + 20;
				while (--c >= 0)
					c_len[i++] = 0;
			}
			else
				c_len[i++] = c - 2;
		}
		while (i < NC)
			c_len[i++] = 0;
		lha_make_table(NC, c_len, 12, c_table);
	}
}

/* ------------------------------------------------------------------------ */
unsigned short decode_c_st1(void)
{
	unsigned short  j, mask;

	if (blocksize == 0) {
		blocksize = getbits(16);
		read_pt_len(NT, TBIT, 3);
		read_c_len();
		read_pt_len(np, pbit, -1);
	}
	blocksize--;
	j = c_table[lhabitbuf >> 4];
	if (j < NC)
		fillbuf(c_len[j]);
	else {
		fillbuf(12);
		mask = 1 << (16 - 1);
		do {
			if (lhabitbuf & mask)
				j = h_right[j];
			else
				j = h_left[j];
			mask >>= 1;
		} while (j >= NC);
		fillbuf(c_len[j] - 12);
	}
	return j;
}

/* ------------------------------------------------------------------------ */
unsigned short decode_p_st1(void)
{
	unsigned short  j, mask;

	j = pt_table[lhabitbuf >> (16 - 8)];
	if (j < np)
		fillbuf(pt_len[j]);
	else {
		fillbuf(8);
		mask = 1 << (16 - 1);
		do {
			if (lhabitbuf & mask)
				j = h_right[j];
			else
				j = h_left[j];
			mask >>= 1;
		} while (j >= np);
		fillbuf(pt_len[j] - 8);
	}
	if (j != 0)
		j = (1 << (j - 1)) + getbits(j - 1);
	return j;
}

/* ------------------------------------------------------------------------ */
void decode_start_st1(void)
{
	if (dicbit <= 13)  {
		np = 14;
		pbit = 4;
	} else {
		if (dicbit == 16) {
			np = 17; /* for -lh7- */
		} else {
			np = 16;
		}
		pbit = 5;
	}

	init_getbits();
	blocksize = 0;
}

/* Local Variables: */
/* mode:c */
/* tab-width:4 */
/* End: */
