#ifndef CRC_H
#define CRC_H

extern UDWORD crctab[256];
extern UWORD crctab_ccitt[256];

void MakeCRCTable();
UDWORD CalcCRC(PUBYTE buf, int len);
UDWORD CalcCRC32(PUBYTE buf, int len, UDWORD seed=0);
UWORD CalcCRC_CCITT(PUBYTE buf, int len);
UDWORD CalcCRC16(PUBYTE buf, int len, UDWORD seed);
UWORD CalcCRC_ANSI(PUBYTE buf, int len);

#endif
