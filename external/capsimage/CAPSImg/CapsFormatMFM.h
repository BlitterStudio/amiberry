#ifndef CAPSFORMATMFM_H
#define CAPSFORMATMFM_H

int FmfmGetSize(PCAPSFORMATTRACK pf);
int FmfmConvert(PCAPSFORMATTRACK pf);
uint32_t FmfmWriteDataByte(PCAPSFORMATTRACK pf, uint32_t state, uint32_t value, int count);
uint32_t FmfmWriteMarkByte(PCAPSFORMATTRACK pf, uint32_t state, uint32_t value, int count);
uint16_t FmfmCrc(uint16_t crc, uint32_t value, int count = 1);
int FmfmSectorLength(int value);
uint32_t FmfmWriteBlockIndex(PCAPSFORMATTRACK pf, uint32_t state, PCAPSFORMATBLOCK pb);
uint32_t FmfmWriteBlockData(PCAPSFORMATTRACK pf, uint32_t state, PCAPSFORMATBLOCK pb);

#endif
