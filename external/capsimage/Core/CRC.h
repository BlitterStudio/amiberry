#ifndef CRC_H
#define CRC_H

extern uint32_t crctab[256];
extern uint16_t crctab_ccitt[256];

void MakeCRCTable();
uint32_t CalcCRC(const uint8_t *buf, size_t len);
uint32_t CalcCRC32(const uint8_t *buf, size_t len, uint32_t seed = 0);
uint16_t CalcCRC_CCITT(const uint8_t *buf, size_t len);
uint32_t CalcCRC16(const uint8_t *buf, size_t len, uint32_t seed);
uint16_t CalcCRC_ANSI(const uint8_t *buf, size_t len);

#endif
