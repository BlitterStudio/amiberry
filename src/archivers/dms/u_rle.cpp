
/*
 *     xDMS  v1.3  -  Portable DMS archive unpacker  -  Public Domain
 *     Written by     Andre Rodrigues de la Rocha  <adlroc@usa.net>
 *
 *     Run Length Encoding decompression function used in most
 *     modes after decompression by other algorithm
 *
 */

#include <string.h>
#include "cdata.h"
#include "u_rle.h"



USHORT Unpack_RLE(UCHAR *in, UCHAR *out, USHORT origsize){
	USHORT n;
	UCHAR a,b, *outend;

	outend = out+origsize;
	while (out<outend){
		if ((a = *in++) != 0x90)
			*out++ = a;
		else if (!(b = *in++))
			*out++ = a;
		else {
			a = *in++;
			if (b == 0xff) {
				n = *in++;
				n = (USHORT)((n<<8) + *in++);
			} else
				n = b;
			if (out+n > outend) return 1;
			memset(out,a,(size_t) n);
			out += n;
		}
	}
	return 0;
}


