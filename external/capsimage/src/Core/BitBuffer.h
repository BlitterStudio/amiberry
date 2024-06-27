#ifndef BITBUFFER_H
#define BITBUFFER_H

// largest bitfield size supported in a single, atomic operation
#define MAX_BITBUFFER_LEN 32



// bit level buffer access helper
class CBitBuffer
{
public:
	CBitBuffer();
	virtual ~CBitBuffer();
	void InitByteSize(uint8_t *buf, uint32_t bytesize);
	void InitBitSize(uint8_t *buf, uint32_t bitsize);

	uint32_t ReadBitWrap(uint32_t bitpos, int bitcnt);
	static uint32_t ReadBitWrap(uint8_t *buf, uint32_t bufwrap, uint32_t bitpos, int bitcnt);
	uint32_t ReadBit(uint32_t bitpos, int bitcnt);
	static uint32_t ReadBit(uint8_t *buf, uint32_t bitpos, int bitcnt);
	uint32_t ReadBit(uint32_t bitpos);
	static uint32_t ReadBit(uint8_t *buf, uint32_t bitpos);
	uint32_t ReadBit8(uint32_t bitpos);
	static uint32_t ReadBit8(uint8_t *buf, uint32_t bitpos);
	static uint32_t ReadBit16(uint8_t *buf);
	static uint32_t ReadBitLE16(uint8_t *buf);
	uint32_t ReadBit16(uint32_t bitpos);
	static uint32_t ReadBit16(uint8_t *buf, uint32_t bitpos);
	static uint32_t ReadBit32(uint8_t *buf);
	static uint32_t ReadBitLE32(uint8_t *buf);
	uint32_t ReadBit32(uint32_t bitpos);
	static uint32_t ReadBit32(uint8_t *buf, uint32_t bitpos);
	uint32_t ReadBit10(uint32_t bitpos);
	static uint32_t ReadBit10(uint8_t *buf, uint32_t bitpos);

	void WriteBitWrap(uint32_t bitpos, uint32_t value, int bitcnt);
	static void WriteBitWrap(uint8_t *buf, uint32_t bufwrap, uint32_t bitpos, uint32_t value, int bitcnt);
	void WriteBit(uint32_t bitpos, uint32_t value, int bitcnt);
	static void WriteBit(uint8_t *buf, uint32_t bitpos, uint32_t value, int bitcnt);
	void WriteBit(uint32_t bitpos, uint32_t value);
	static void WriteBit(uint8_t *buf, uint32_t bitpos, uint32_t value);
	static void WriteBit8(uint8_t *buf, uint32_t value);
	static void WriteBit16(uint8_t *buf, uint32_t value);
	static void WriteBitLE16(uint8_t *buf, uint32_t value);
	static void WriteBit24(uint8_t *buf, uint32_t value);
	static void WriteBit32(uint8_t *buf, uint32_t value);
	static void WriteBitLE32(uint8_t *buf, uint32_t value);

	void ClearBitWrap(uint32_t bitpos, int bitcnt);
	static void ClearBitWrap(uint8_t *buf, uint32_t bufwrap, uint32_t bitpos, int bitcnt);
	void ClearBit(uint32_t bitpos, int bitcnt);
	static void ClearBit(uint8_t *buf, uint32_t bitpos, int bitcnt);

	int CompareBit(uint32_t buf1pos, uint32_t buf2pos, int bitcnt);
	static int CompareBit(uint8_t *buf1, uint32_t buf1pos, uint8_t *buf2, uint32_t buf2pos, int bitcnt);
	int CompareAndCountBit(uint32_t buf1pos, uint32_t buf2pos, int bitcnt);
	static int CompareAndCountBit(uint8_t *buf1, uint32_t buf1pos, uint8_t *buf2, uint32_t buf2pos, int bitcnt);

	void CopyBitWrap(uint32_t srcpos, uint32_t dstpos, int bitcnt);
	static void CopyBitWrap(uint8_t *srcbuf, uint32_t srcwrap, uint32_t srcpos, uint8_t *dstbuf, uint32_t dstwrap, uint32_t dstpos, int bitcnt);
	void CopyBit(uint32_t srcpos, uint32_t dstpos, int bitcnt);
	static void CopyBit(uint8_t *srcbuf, uint32_t srcpos, uint8_t *dstbuf, uint32_t dstpos, int bitcnt);

	static uint32_t CalculateByteSize(uint32_t bitsize);

protected:
	void Clear();

protected:
	uint8_t *bufmem; // buffer memory
	uint32_t bufsize; // buffer size in bytes
	uint32_t bufbits; // buffer size in bits
};

typedef CBitBuffer *PCBITBUFFER;



// read up to 32 data bits from the class associated buffer, wrap around
inline uint32_t CBitBuffer::ReadBitWrap(uint32_t bitpos, int bitcnt)
{
	return ReadBitWrap(bufmem, bufbits, bitpos, bitcnt);
}

// read up to 32 data bits from the class associated buffer
inline uint32_t CBitBuffer::ReadBit(uint32_t bitpos, int bitcnt)
{
	return ReadBit(bufmem, bitpos, bitcnt);
}

// read 1 data bit from the class associated buffer
inline uint32_t CBitBuffer::ReadBit(uint32_t bitpos)
{
	return ReadBit(bufmem, bitpos);
}

// read 1 data bit from the specified buffer
inline uint32_t CBitBuffer::ReadBit(uint8_t *buf, uint32_t bitpos)
{
	return buf[bitpos >> 3] >> ((bitpos & 7) ^ 7) & 1;
}

// read 8 data bits from the class associated buffer
inline uint32_t CBitBuffer::ReadBit8(uint32_t bitpos)
{
	return ReadBit8(bufmem, bitpos);
}

// read 8 data bits from the specified buffer
inline uint32_t CBitBuffer::ReadBit8(uint8_t *buf, uint32_t bitpos)
{
	int shf = bitpos & 7;
	uint32_t pos = bitpos >> 3;

	if (!shf)
		return buf[pos];

	return ((buf[pos] << shf) | (buf[pos + 1] >> (8 - shf))) & 0xff;
}

// read 16 data bits from byte aligned address
inline uint32_t CBitBuffer::ReadBit16(uint8_t *buf)
{
	return buf[0] << 8 | buf[1];
}

// read 16 data bits from byte aligned address, little-endian
inline uint32_t CBitBuffer::ReadBitLE16(uint8_t *buf)
{
	return buf[1] << 8 | buf[0];
}

// read 16 data bits from the class associated buffer
inline uint32_t CBitBuffer::ReadBit16(uint32_t bitpos)
{
	return ReadBit16(bufmem, bitpos);
}

// read 16 data bits from the specified buffer
inline uint32_t CBitBuffer::ReadBit16(uint8_t *buf, uint32_t bitpos)
{
	int shf = bitpos & 7;
	uint32_t pos = bitpos >> 3;
	uint32_t res = buf[pos] << 8 | buf[pos + 1];

	if (!shf)
		return res;

	return ((res << shf) | (buf[pos + 2] >> (8 - shf))) & 0xffff;
}

// read 32 data bits from byte aligned address
inline uint32_t CBitBuffer::ReadBit32(uint8_t *buf)
{
	return buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];
}

// read 32 data bits from byte aligned address, little-endian
inline uint32_t CBitBuffer::ReadBitLE32(uint8_t *buf)
{
	return buf[3] << 24 | buf[2] << 16 | buf[1] << 8 | buf[0];
}

// read 32 data bits from the class associated buffer
inline uint32_t CBitBuffer::ReadBit32(uint32_t bitpos)
{
	return ReadBit32(bufmem, bitpos);
}

// read 32 data bits from the specified buffer
inline uint32_t CBitBuffer::ReadBit32(uint8_t *buf, uint32_t bitpos)
{
	int shf = bitpos & 7;
	uint32_t pos = bitpos >> 3;
	uint32_t res = buf[pos] << 24 | buf[pos + 1] << 16 | buf[pos + 2] << 8 | buf[pos + 3];

	if (!shf)
		return res;

	return ((res << shf) | (buf[pos + 4] >> (8 - shf)));
}

// read 10 data bits from the class associated buffer
inline uint32_t CBitBuffer::ReadBit10(uint32_t bitpos)
{
	return ReadBit10(bufmem, bitpos);
}

// read 10 data bits from the specified buffer
inline uint32_t CBitBuffer::ReadBit10(uint8_t *buf, uint32_t bitpos)
{
	int shf = bitpos & 7;
	uint32_t pos = bitpos >> 3;

	if (shf == 7)
		return ((buf[pos] << 9) | (buf[pos + 1] << 1) | (buf[pos + 2] >> 7)) & 0x3ff;
	else
		return ((buf[pos] << (shf + 2)) | (buf[pos + 1] >> (6 - shf))) & 0x3ff;
}



// write up to 32 data bits to the class associated buffer, wrap around
inline void CBitBuffer::WriteBitWrap(uint32_t bitpos, uint32_t value, int bitcnt)
{
	WriteBitWrap(bufmem, bufbits, bitpos, value, bitcnt);
}

// write up to 32 data bits to the class associated buffer
inline void CBitBuffer::WriteBit(uint32_t bitpos, uint32_t value, int bitcnt)
{
	WriteBit(bufmem, bitpos, value, bitcnt);
}

// write 1 data bit to the class associated buffer
inline void CBitBuffer::WriteBit(uint32_t bitpos, uint32_t value)
{
	WriteBit(bufmem, bitpos, value);
}

// write 1 data bit to the specified buffer
inline void CBitBuffer::WriteBit(uint8_t *buf, uint32_t bitpos, uint32_t value)
{
	// write buffer position
	uint8_t *wpos = buf + (bitpos >> 3);

	// bitmask for selected bit in byte
	int bmask = 1 << ((bitpos & 7) ^ 7);

	if (value & 1)
		*wpos |= bmask;
	else
		*wpos &= ~bmask;
}

// write 8 bit value to byte aligned address
inline void CBitBuffer::WriteBit8(uint8_t *buf, uint32_t value)
{
	buf[0] = uint8_t(value);
}

// write 16 bit value to byte aligned address
inline void CBitBuffer::WriteBit16(uint8_t *buf, uint32_t value)
{
	buf[0] = uint8_t(value >> 8);
	buf[1] = uint8_t(value);
}

// write 16 bit value to byte aligned address, little-endian
inline void CBitBuffer::WriteBitLE16(uint8_t *buf, uint32_t value)
{
	buf[0] = uint8_t(value);
	buf[1] = uint8_t(value >> 8);
}

// write 24 bit value to byte aligned address
inline void CBitBuffer::WriteBit24(uint8_t *buf, uint32_t value)
{
	buf[0] = uint8_t(value >> 16);
	buf[1] = uint8_t(value >> 8);
	buf[2] = uint8_t(value);
}

// write 32 bit value to byte aligned address
inline void CBitBuffer::WriteBit32(uint8_t *buf, uint32_t value)
{
	buf[0] = uint8_t(value >> 24);
	buf[1] = uint8_t(value >> 16);
	buf[2] = uint8_t(value >> 8);
	buf[3] = uint8_t(value);
}

// write 32 bit value to byte aligned address, little-endian
inline void CBitBuffer::WriteBitLE32(uint8_t *buf, uint32_t value)
{
	buf[0] = uint8_t(value);
	buf[1] = uint8_t(value >> 8);
	buf[2] = uint8_t(value >> 16);
	buf[3] = uint8_t(value >> 24);
}



// clear bit field in the class associated buffer, wrap around
inline void CBitBuffer::ClearBitWrap(uint32_t bitpos, int bitcnt)
{
	ClearBitWrap(bufmem, bufbits, bitpos, bitcnt);
}

// clear bit field in the class associated buffer
inline void CBitBuffer::ClearBit(uint32_t bitpos, int bitcnt)
{
	ClearBit(bufmem, bitpos, bitcnt);
}



// compare two bitfields in the class associated buffer, return 0 if they are identical
inline int CBitBuffer::CompareBit(uint32_t buf1pos, uint32_t buf2pos, int bitcnt)
{
	return CompareBit(bufmem, buf1pos, bufmem, buf2pos, bitcnt);
}

// compare two bitfields in the class associated buffer, return the number of the first N identical bits
inline int CBitBuffer::CompareAndCountBit(uint32_t buf1pos, uint32_t buf2pos, int bitcnt)
{
	return CompareAndCountBit(bufmem, buf1pos, bufmem, buf2pos, bitcnt);
}



// copy a bitfield in the class associated buffer, wrap around
inline void CBitBuffer::CopyBitWrap(uint32_t srcpos, uint32_t dstpos, int bitcnt)
{
	CopyBitWrap(bufmem, bufbits, srcpos, bufmem, bufbits, dstpos, bitcnt);
}

// copy a bitfield in the class associated buffer
inline void CBitBuffer::CopyBit(uint32_t srcpos, uint32_t dstpos, int bitcnt)
{
	CopyBit(bufmem, srcpos, bufmem, dstpos, bitcnt);
}



// calculate rounded byte size from bitfield size
inline uint32_t CBitBuffer::CalculateByteSize(uint32_t bitsize)
{
	return ((bitsize + 7) >> 3);
}

#endif
