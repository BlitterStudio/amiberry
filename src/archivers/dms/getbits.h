
extern ULONG mask_bits[], bitbuf;
extern UCHAR *indata, bitcount;

#define GETBITS(n) ((USHORT)(bitbuf >> (bitcount-(n))))
#define DROPBITS(n) {bitbuf &= mask_bits[bitcount-=(n)]; while (bitcount<16) {bitbuf = (bitbuf << 8) | *indata++;  bitcount += 8;}}


void initbitbuf(UCHAR *);

