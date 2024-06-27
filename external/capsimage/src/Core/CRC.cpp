#include "stdafx.h"



int crcinit=0;
UDWORD crctab[256];
UWORD crctab_ccitt[256];
UWORD crctab_ansi[256];
UBYTE crcpoly[]={0, 1, 2, 4, 5, 7, 8, 10, 11, 12, 16, 22, 23, 26};
UBYTE crcpoly_ccitt[]={0, 5, 12};
UBYTE crcpoly_ansi[]={0, 2, 15};


// initialize CRC table
void MakeCRCTable()
{
	UDWORD poly=0;
	int i;

	if (crcinit)
		return;

	// crc32
	for (i=0; i < sizeof(crcpoly)/sizeof(UBYTE); i++)
		poly|=1UL<<(31-crcpoly[i]);

	for (i=0; i < 256; i++) {
		UDWORD crc=i;
		int bit;

		for (bit=0; bit < 8; bit++)
			crc=crc & 1 ? poly^(crc>>1) : crc>>1;

		crctab[i]=crc;
	}

	// crc-ccitt
	for (i=0, poly=0; i < sizeof(crcpoly_ccitt)/sizeof(UBYTE); i++)
		poly|=1UL<<crcpoly_ccitt[i];

	for (i=0; i < 256; i++) {
		UWORD crc=i<<8;
		int bit;

		for (bit=0; bit < 8; bit++)
			crc=crc & 0x8000 ? (UWORD)poly^(crc<<1) : crc<<1;

		crctab_ccitt[i]=crc;
	}

	// crc-ansi
	for (i=0, poly=0; i < sizeof(crcpoly_ansi)/sizeof(UBYTE); i++)
		poly|=1UL<<crcpoly_ansi[i];

	for (i=0; i < 256; i++) {
		UWORD crc=i<<8;
		int bit;

		for (bit=0; bit < 8; bit++)
			crc=crc & 0x8000 ? (UWORD)poly^(crc<<1) : crc<<1;

		crctab_ansi[i]=crc;
	}

	crcinit=1;
}

// create CRC value from buffer
UDWORD CalcCRC(PUBYTE buf, int len)
{
	UDWORD crc=~0;

	while (len--)
		crc=crctab[(crc^*buf++) & 0xff] ^ (crc >> 8);

	return crc^~0;
}

// create CRC32 from seed value
UDWORD CalcCRC32(PUBYTE buf, int len, UDWORD seed)
{
	UDWORD crc=~seed;

	while (len--)
		crc=crctab[(crc^*buf++) & 0xff] ^ (crc >> 8);

	return crc^~0;
}

// create CRC-CCITT value from buffer
UWORD CalcCRC_CCITT(PUBYTE buf, int len)
{
	UWORD crc=~0;

	while (len--)
		crc=crctab_ccitt[*buf++^(crc>>8)] ^ (crc << 8);

	return crc;
}

// CRC16 from seed value
UDWORD CalcCRC16(PUBYTE buf, int len, UDWORD seed)
{
	UWORD crc=(UWORD)seed;

	while (len--)
		crc=crctab_ccitt[*buf++^(crc>>8)] ^ (crc << 8);

	return crc;
}

// create CRC-ANSI value from buffer
UWORD CalcCRC_ANSI(PUBYTE buf, int len)
{
	UWORD crc=0;

	while (len--)
		crc=crctab_ansi[*buf++^(crc>>8)] ^ (crc << 8);

	return crc;
}

