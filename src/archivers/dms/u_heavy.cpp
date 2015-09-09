
/*
 *     xDMS  v1.3  -  Portable DMS archive unpacker  -  Public Domain
 *     Written by     Andre Rodrigues de la Rocha  <adlroc@usa.net>
 *
 *     Lempel-Ziv-Huffman decompression functions used in Heavy 1 & 2
 *     compression modes. Based on LZH decompression functions from
 *     UNIX LHA made by Masaru Oki
 *
 */


#include "cdata.h"
#include "u_heavy.h"
#include "getbits.h"
#include "maketbl.h"


#define NC 510
#define NPT 20
#define N1 510
#define OFFSET 253

USHORT left[2 * NC - 1], right[2 * NC - 1 + 9];
static UCHAR c_len[NC], pt_len[NPT];
static USHORT c_table[4096], pt_table[256];
USHORT lastlen, np;
USHORT heavy_text_loc;


static USHORT read_tree_c(void);
static USHORT read_tree_p(void);
INLINE USHORT decode_c(void);
INLINE USHORT decode_p(void);



USHORT Unpack_HEAVY(UCHAR *in, UCHAR *out, UCHAR flags, USHORT origsize){
	USHORT j, i, c, bitmask;
	UCHAR *outend;

	/*  Heavy 1 uses a 4Kb dictionary,  Heavy 2 uses 8Kb  */

	if (flags & 8) {
		np = 15;
		bitmask = 0x1fff;
	} else {
		np = 14;
		bitmask = 0x0fff;
	}

	initbitbuf(in);

	if (flags & 2) {
		if (read_tree_c()) return 1;
		if (read_tree_p()) return 2;
	}

	outend = out+origsize;

	while (out<outend) {
		c = decode_c();
		if (c < 256) {
			*out++ = text[heavy_text_loc++ & bitmask] = (UCHAR)c;
		} else {
			j = (USHORT) (c - OFFSET);
			i = (USHORT) (heavy_text_loc - decode_p() - 1);
			while(j--) *out++ = text[heavy_text_loc++ & bitmask] = text[i++ & bitmask];
		}
	}

	return 0;
}



INLINE USHORT decode_c(void){
	USHORT i, j, m;

	j = c_table[GETBITS(12)];
	if (j < N1) {
		DROPBITS(c_len[j]);
	} else {
		DROPBITS(12);
		i = GETBITS(16);
		m = 0x8000;
		do {
			if (i & m) j = right[j];
			else              j = left [j];
			m >>= 1;
		} while (j >= N1);
		DROPBITS(c_len[j] - 12);
	}
	return j;
}



INLINE USHORT decode_p(void){
	USHORT i, j, m;

	j = pt_table[GETBITS(8)];
	if (j < np) {
		DROPBITS(pt_len[j]);
	} else {
		DROPBITS(8);
		i = GETBITS(16);
		m = 0x8000;
		do {
			if (i & m) j = right[j];
			else             j = left [j];
			m >>= 1;
		} while (j >= np);
		DROPBITS(pt_len[j] - 8);
	}

	if (j != np-1) {
		if (j > 0) {
			j = (USHORT)(GETBITS(i=(USHORT)(j-1)) | (1U << (j-1)));
			DROPBITS(i);
		}
		lastlen=j;
	}

	return lastlen;

}



static USHORT read_tree_c(void){
	USHORT i,n;

	n = GETBITS(9);
	DROPBITS(9);
	if (n>0){
		for (i=0; i<n; i++) {
			c_len[i] = (UCHAR)GETBITS(5);
			DROPBITS(5);
		}
		for (i=n; i<510; i++) c_len[i] = 0;
		if (make_table(510,c_len,12,c_table)) return 1;
	} else {
		n = GETBITS(9);
		DROPBITS(9);
		for (i=0; i<510; i++) c_len[i] = 0;
		for (i=0; i<4096; i++) c_table[i] = n;
	}
	return 0;
}



static USHORT read_tree_p(void){
	USHORT i,n;

	n = GETBITS(5);
	DROPBITS(5);
	if (n>0){
		for (i=0; i<n; i++) {
			pt_len[i] = (UCHAR)GETBITS(4);
			DROPBITS(4);
		}
		for (i=n; i<np; i++) pt_len[i] = 0;
		if (make_table(np,pt_len,8,pt_table)) return 1;
	} else {
		n = GETBITS(5);
		DROPBITS(5);
		for (i=0; i<np; i++) pt_len[i] = 0;
		for (i=0; i<256; i++) pt_table[i] = n;
	}
	return 0;
}


