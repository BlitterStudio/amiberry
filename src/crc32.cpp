
#include "sysconfig.h"
#include "sysdeps.h"

static unsigned long crc_table32[256];
static unsigned short crc_table16[256];
static void make_crc_table()
{
    unsigned long c;
    unsigned short w;
    int n, k;
    for (n = 0; n < 256; n++) {
	c = (unsigned long)n;
	w = n << 8;
	for (k = 0; k < 8; k++) {
	    c = (c >> 1) ^ (c & 1 ? 0xedb88320 : 0);
	    w = (w << 1) ^ ((w & 0x8000) ? 0x1021 : 0);
	}
	crc_table32[n] = c;
	crc_table16[n] = w;
    }
}
uae_u32 get_crc32 (uae_u8 *buf, int len)
{
    uae_u32 crc;
    if (!crc_table32[1])
	make_crc_table();
    crc = 0xffffffff;
    while (len-- > 0)
	crc = crc_table32[(crc ^ (*buf++)) & 0xff] ^ (crc >> 8);    
    return crc ^ 0xffffffff;
}
uae_u16 get_crc16( uae_u8 *buf, int len)
{
    uae_u16 crc;
    if (!crc_table32[1])
	make_crc_table();
    crc = 0xffff;
    while (len-- > 0)
	crc = (crc << 8) ^ crc_table16[((crc >> 8) ^ (*buf++)) & 0xff];
    return crc;
}
