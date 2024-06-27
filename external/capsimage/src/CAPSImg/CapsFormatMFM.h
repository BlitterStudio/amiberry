#ifndef CAPSFORMATMFM_H
#define CAPSFORMATMFM_H

int FmfmGetSize(PCAPSFORMATTRACK pf);
int FmfmConvert(PCAPSFORMATTRACK pf);
UDWORD FmfmWriteDataByte(PCAPSFORMATTRACK pf, UDWORD state, UDWORD value, int count);
UDWORD FmfmWriteMarkByte(PCAPSFORMATTRACK pf, UDWORD state, UDWORD value, int count);
UWORD FmfmCrc(UWORD crc, UDWORD value, int count=1);
int FmfmSectorLength(int value);
UDWORD FmfmWriteBlockIndex(PCAPSFORMATTRACK pf, UDWORD state, PCAPSFORMATBLOCK pb);
UDWORD FmfmWriteBlockData(PCAPSFORMATTRACK pf, UDWORD state, PCAPSFORMATBLOCK pb);

#endif
