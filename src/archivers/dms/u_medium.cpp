
/*
 *     xDMS  v1.3  -  Portable DMS archive unpacker  -  Public Domain
 *     Written by     Andre Rodrigues de la Rocha  <adlroc@usa.net>
 *
 *     Main decompression functions used in MEDIUM mode
 *
 */


#include <string.h>

#include "cdata.h"
#include "u_medium.h"
#include "getbits.h"
#include "tables.h"


#define MBITMASK 0x3fff


USHORT medium_text_loc;



USHORT Unpack_MEDIUM(UCHAR *in, UCHAR *out, USHORT origsize){
	USHORT i, j, c;
	UCHAR u, *outend;


	initbitbuf(in);

	outend = out+origsize;
	while (out < outend) {
		if (GETBITS(1)!=0) {
			DROPBITS(1);
			*out++ = text[medium_text_loc++ & MBITMASK] = (UCHAR)GETBITS(8);
			DROPBITS(8);
		} else {
			DROPBITS(1);
			c = GETBITS(8);  DROPBITS(8);
			j = (USHORT) (d_code[c]+3);
			u = d_len[c];
			c = (USHORT) (((c << u) | GETBITS(u)) & 0xff);  DROPBITS(u);
			u = d_len[c];
			c = (USHORT) ((d_code[c] << 8) | (((c << u) | GETBITS(u)) & 0xff));  DROPBITS(u);
			i = (USHORT) (medium_text_loc - c - 1);

			while(j--) *out++ = text[medium_text_loc++ & MBITMASK] = text[i++ & MBITMASK];

		}
	}
	medium_text_loc = (USHORT)((medium_text_loc+66) & MBITMASK);

	return 0;
}


