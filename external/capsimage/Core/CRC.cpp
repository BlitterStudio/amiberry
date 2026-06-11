#include "stdafx.h"



int crcinit = 0;
uint32_t crctab[256];
uint16_t crctab_ccitt[256];
uint16_t crctab_ansi[256];
const uint8_t crcpoly[] = { 0, 1, 2, 4, 5, 7, 8, 10, 11, 12, 16, 22, 23, 26 };
const uint8_t crcpoly_ccitt[] = { 0, 5, 12 };
const uint8_t crcpoly_ansi[] = { 0, 2, 15 };


// initialize CRC table
void MakeCRCTable()
{
	uint32_t poly = 0;
	int i;

	if (crcinit)
		return;

	// crc32
	for (i = 0; i < sizeof(crcpoly) / sizeof(uint8_t); i++)
		poly |= 1UL << (31 - crcpoly[i]);

	for (i = 0; i < 256; i++) {
		uint32_t crc = i;
		int bit;

		for (bit = 0; bit < 8; bit++)
			crc = crc & 1 ? poly ^ (crc >> 1) : crc >> 1;

		crctab[i] = crc;
	}

	// crc-ccitt
	for (i = 0, poly = 0; i < sizeof(crcpoly_ccitt) / sizeof(uint8_t); i++)
		poly |= 1UL << crcpoly_ccitt[i];

	for (i = 0; i < 256; i++) {
		uint16_t crc = i << 8;
		int bit;

		for (bit = 0; bit < 8; bit++)
			crc = crc & 0x8000 ? (uint16_t)poly ^ (crc << 1) : crc << 1;

		crctab_ccitt[i] = crc;
	}

	// crc-ansi
	for (i = 0, poly = 0; i < sizeof(crcpoly_ansi) / sizeof(uint8_t); i++)
		poly |= 1UL << crcpoly_ansi[i];

	for (i = 0; i < 256; i++) {
		uint16_t crc = i << 8;
		int bit;

		for (bit = 0; bit < 8; bit++)
			crc = crc & 0x8000 ? (uint16_t)poly ^ (crc << 1) : crc << 1;

		crctab_ansi[i] = crc;
	}

	crcinit = 1;
}

// create CRC value from buffer
uint32_t CalcCRC(const uint8_t *buf, size_t len)
{
	uint32_t crc = ~0;

	while (len--)
		crc = crctab[(crc^*buf++) & 0xff] ^ (crc >> 8);

	return crc ^ ~0;
}

// create CRC32 from seed value
uint32_t CalcCRC32(const uint8_t *buf, size_t len, uint32_t seed)
{
	uint32_t crc = ~seed;

	while (len--)
		crc = crctab[(crc^*buf++) & 0xff] ^ (crc >> 8);

	return crc ^ ~0;
}

// create CRC-CCITT value from buffer
uint16_t CalcCRC_CCITT(const uint8_t *buf, size_t len)
{
	uint16_t crc = ~0;

	while (len--)
		crc = crctab_ccitt[*buf++ ^ (crc >> 8)] ^ (crc << 8);

	return crc;
}

// CRC16 from seed value
uint32_t CalcCRC16(const uint8_t *buf, size_t len, uint32_t seed)
{
	uint16_t crc = (uint16_t)seed;

	while (len--)
		crc = crctab_ccitt[*buf++ ^ (crc >> 8)] ^ (crc << 8);

	return crc;
}

// create CRC-ANSI value from buffer
uint16_t CalcCRC_ANSI(const uint8_t *buf, size_t len)
{
	uint16_t crc = 0;

	while (len--)
		crc = crctab_ansi[*buf++ ^ (crc >> 8)] ^ (crc << 8);

	return crc;
}

