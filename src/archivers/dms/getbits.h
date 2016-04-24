
extern ULONG dms_mask_bits[], dms_bitbuf;
extern UCHAR *dms_indata, dms_bitcount;

#define GETBITS(n) ((USHORT)(dms_bitbuf >> (dms_bitcount-(n))))
#define DROPBITS(n) {dms_bitbuf &= dms_mask_bits[dms_bitcount-=(n)]; while (dms_bitcount<16) {dms_bitbuf = (dms_bitbuf << 8) | *dms_indata++;  dms_bitcount += 8;}}


void initbitbuf(UCHAR *);

