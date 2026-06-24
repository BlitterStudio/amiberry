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
	void Clear();
	void InitByteSize(uint8_t *buf, uint32_t bytesize);
	void InitByteSize(const uint8_t *buf, uint32_t bytesize);
	void InitBitSize(uint8_t *buf, uint32_t bitsize);
	void InitBitSize(const uint8_t *buf, uint32_t bitsize);
	uint32_t GetBitSize() const;
	bool IsEmpty() const;
	bool IsReadOnly() const;

	uint32_t ReadBitWrap(uint32_t bitpos, int bitcnt) const;
	static uint32_t ReadBitWrap(const uint8_t *buf, uint32_t bufwrap, uint32_t bitpos, int bitcnt);
	uint32_t ReadBit(uint32_t bitpos, int bitcnt) const;
	static uint32_t ReadBit(const uint8_t *buf, uint32_t bitpos, int bitcnt);
	uint8_t ReadBit(uint32_t bitpos) const;
	static uint8_t ReadBit(const uint8_t *buf, uint32_t bitpos);
	uint8_t ReadBit8(uint32_t bitpos) const;
	static uint8_t ReadBit8(const uint8_t *buf, uint32_t bitpos);
	static uint16_t ReadBit16(const uint8_t *buf);
	static uint16_t ReadBitLE16(const uint8_t *buf);
	uint16_t ReadBit16(uint32_t bitpos) const;
	static uint16_t ReadBit16(const uint8_t *buf, uint32_t bitpos);
	static uint32_t ReadBit32(const uint8_t *buf);
	static uint32_t ReadBitLE32(const uint8_t *buf);
	uint32_t ReadBit32(uint32_t bitpos) const;
	static uint32_t ReadBit32(const uint8_t *buf, uint32_t bitpos);
	uint16_t ReadBit10(uint32_t bitpos) const;
	static uint16_t ReadBit10(const uint8_t *buf, uint32_t bitpos);

	void WriteBitWrap(uint32_t bitpos, uint32_t value, int bitcnt) const;
	static void WriteBitWrap(uint8_t *buf, uint32_t bufwrap, uint32_t bitpos, uint32_t value, int bitcnt);
	void WriteBit(uint32_t bitpos, uint32_t value, int bitcnt) const;
	static void WriteBit(uint8_t *buf, uint32_t bitpos, uint32_t value, int bitcnt);
	void WriteBit(uint32_t bitpos, uint32_t value) const;
	static void WriteBit(uint8_t *buf, uint32_t bitpos, uint32_t value);
	static void WriteBit8(uint8_t *buf, uint32_t value);
	static void WriteBit16(uint8_t *buf, uint32_t value);
	static void WriteBitLE16(uint8_t *buf, uint32_t value);
	static void WriteBit24(uint8_t *buf, uint32_t value);
	static void WriteBit32(uint8_t *buf, uint32_t value);
	static void WriteBitLE32(uint8_t *buf, uint32_t value);

	void ClearBitWrap(uint32_t bitpos, int bitcnt) const;
	static void ClearBitWrap(uint8_t *buf, uint32_t bufwrap, uint32_t bitpos, int bitcnt);
	void ClearBit(uint32_t bitpos, int bitcnt) const;
	static void ClearBit(uint8_t *buf, uint32_t bitpos, int bitcnt);

	int CompareBit(uint32_t buf1pos, uint32_t buf2pos, int bitcnt) const;
	static int CompareBit(const uint8_t *buf1, uint32_t buf1pos, const uint8_t *buf2, uint32_t buf2pos, int bitcnt);
	int CompareAndCountBit(uint32_t buf1pos, uint32_t buf2pos, int bitcnt) const;
	static int CompareAndCountBit(const uint8_t *buf1, uint32_t buf1pos, const uint8_t *buf2, uint32_t buf2pos, int bitcnt);

	void CopyBitWrap(uint32_t srcpos, uint32_t dstpos, int bitcnt) const;
	static void CopyBitWrap(const uint8_t *srcbuf, uint32_t srcwrap, uint32_t srcpos, uint8_t *dstbuf, uint32_t dstwrap, uint32_t dstpos, int bitcnt);
	void CopyBit(uint32_t srcpos, uint32_t dstpos, int bitcnt) const;
	static void CopyBit(const uint8_t *srcbuf, uint32_t srcpos, uint8_t *dstbuf, uint32_t dstpos, int bitcnt);

	static uint32_t CalculateByteSize(uint32_t bitsize);

	template <typename T>
	static T LittleEndian(const T &t);
	template <typename T>
	static T BigEndian(const T &t);

	template <typename T, typename V>
	static void LittleEndian(T &t, V v);
	template <typename T, typename V>
	static void BigEndian(T &t, V v);

protected:
	uint8_t *buf_write; // write access to buffer memory; all write functions use this
	const uint8_t *buf_read; // read-only access to buffer memory; all read functions use this
	uint32_t buf_size; // buffer size in bytes
	uint32_t buf_bits; // buffer size in bits
};

typedef CBitBuffer *PCBITBUFFER;



// get the size of the buffer in bits
inline uint32_t CBitBuffer::GetBitSize() const
{
	return buf_read ? buf_bits : 0;
}

// return true if the bit buffer is empty or cleared
inline bool CBitBuffer::IsEmpty() const
{
	return GetBitSize() ? false : true;
}

// return true if the buffer is read-only
inline bool CBitBuffer::IsReadOnly() const
{
	return buf_write ? false : true;
}

// read up to 32 data bits from the class associated buffer, wrap around
inline uint32_t CBitBuffer::ReadBitWrap(uint32_t bitpos, int bitcnt) const
{
	return ReadBitWrap(buf_read, buf_bits, bitpos, bitcnt);
}

// read up to 32 data bits from the class associated buffer
inline uint32_t CBitBuffer::ReadBit(uint32_t bitpos, int bitcnt) const
{
	return ReadBit(buf_read, bitpos, bitcnt);
}

// read 1 data bit from the class associated buffer
inline uint8_t CBitBuffer::ReadBit(uint32_t bitpos) const
{
	return ReadBit(buf_read, bitpos);
}

// read 1 data bit from the specified buffer
inline uint8_t CBitBuffer::ReadBit(const uint8_t *buf, uint32_t bitpos)
{
	return buf[bitpos >> 3] >> ((bitpos & 7) ^ 7) & 1;
}

// read 8 data bits from the class associated buffer
inline uint8_t CBitBuffer::ReadBit8(uint32_t bitpos) const
{
	return ReadBit8(buf_read, bitpos);
}

// read 8 data bits from the specified buffer
inline uint8_t CBitBuffer::ReadBit8(const uint8_t *buf, uint32_t bitpos)
{
	int shf = bitpos & 7;
	uint32_t pos = bitpos >> 3;

	if (!shf)
		return buf[pos];

	return ((buf[pos] << shf) | (buf[pos + 1] >> (8 - shf)));
}

// read 16 data bits from byte aligned address
inline uint16_t CBitBuffer::ReadBit16(const uint8_t *buf)
{
	return buf[0] << 8 | buf[1];
}

// read 16 data bits from byte aligned address, little-endian
inline uint16_t CBitBuffer::ReadBitLE16(const uint8_t *buf)
{
	return buf[1] << 8 | buf[0];
}

// read 16 data bits from the class associated buffer
inline uint16_t CBitBuffer::ReadBit16(uint32_t bitpos) const
{
	return ReadBit16(buf_read, bitpos);
}

// read 16 data bits from the specified buffer
inline uint16_t CBitBuffer::ReadBit16(const uint8_t *buf, uint32_t bitpos)
{
	int shf = bitpos & 7;
	uint32_t pos = bitpos >> 3;
	uint16_t res = buf[pos] << 8 | buf[pos + 1];

	if (!shf)
		return res;

	return (res << shf) | (buf[pos + 2] >> (8 - shf));
}

// read 32 data bits from byte aligned address
inline uint32_t CBitBuffer::ReadBit32(const uint8_t *buf)
{
	return buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];
}

// read 32 data bits from byte aligned address, little-endian
inline uint32_t CBitBuffer::ReadBitLE32(const uint8_t *buf)
{
	return buf[3] << 24 | buf[2] << 16 | buf[1] << 8 | buf[0];
}

// read 32 data bits from the class associated buffer
inline uint32_t CBitBuffer::ReadBit32(uint32_t bitpos) const
{
	return ReadBit32(buf_read, bitpos);
}

// read 32 data bits from the specified buffer
inline uint32_t CBitBuffer::ReadBit32(const uint8_t *buf, uint32_t bitpos)
{
	int shf = bitpos & 7;
	uint32_t pos = bitpos >> 3;
	uint32_t res = buf[pos] << 24 | buf[pos + 1] << 16 | buf[pos + 2] << 8 | buf[pos + 3];

	if (!shf)
		return res;

	return ((res << shf) | (buf[pos + 4] >> (8 - shf)));
}

// read 10 data bits from the class associated buffer
inline uint16_t CBitBuffer::ReadBit10(uint32_t bitpos) const
{
	return ReadBit10(buf_read, bitpos);
}

// read 10 data bits from the specified buffer
inline uint16_t CBitBuffer::ReadBit10(const uint8_t *buf, uint32_t bitpos)
{
	int shf = bitpos & 7;
	uint32_t pos = bitpos >> 3;

	if (shf == 7)
		return ((buf[pos] << 9) | (buf[pos + 1] << 1) | (buf[pos + 2] >> 7)) & 0x3ff;
	else
		return ((buf[pos] << (shf + 2)) | (buf[pos + 1] >> (6 - shf))) & 0x3ff;
}



// write up to 32 data bits to the class associated buffer, wrap around
inline void CBitBuffer::WriteBitWrap(uint32_t bitpos, uint32_t value, int bitcnt) const
{
	WriteBitWrap(buf_write, buf_bits, bitpos, value, bitcnt);
}

// write up to 32 data bits to the class associated buffer
inline void CBitBuffer::WriteBit(uint32_t bitpos, uint32_t value, int bitcnt) const
{
	WriteBit(buf_write, bitpos, value, bitcnt);
}

// write 1 data bit to the class associated buffer
inline void CBitBuffer::WriteBit(uint32_t bitpos, uint32_t value) const
{
	WriteBit(buf_write, bitpos, value);
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
inline void CBitBuffer::ClearBitWrap(uint32_t bitpos, int bitcnt) const
{
	ClearBitWrap(buf_write, buf_bits, bitpos, bitcnt);
}

// clear bit field in the class associated buffer
inline void CBitBuffer::ClearBit(uint32_t bitpos, int bitcnt) const
{
	ClearBit(buf_write, bitpos, bitcnt);
}



// compare two bitfields in the class associated buffer, return 0 if they are identical
inline int CBitBuffer::CompareBit(uint32_t buf1pos, uint32_t buf2pos, int bitcnt) const
{
	return CompareBit(buf_read, buf1pos, buf_read, buf2pos, bitcnt);
}

// compare two bitfields in the class associated buffer, return the number of the first N identical bits
inline int CBitBuffer::CompareAndCountBit(uint32_t buf1pos, uint32_t buf2pos, int bitcnt) const
{
	return CompareAndCountBit(buf_read, buf1pos, buf_read, buf2pos, bitcnt);
}



// copy a bitfield in the class associated buffer, wrap around
inline void CBitBuffer::CopyBitWrap(uint32_t srcpos, uint32_t dstpos, int bitcnt) const
{
	CopyBitWrap(buf_read, buf_bits, srcpos, buf_write, buf_bits, dstpos, bitcnt);
}

// copy a bitfield in the class associated buffer
inline void CBitBuffer::CopyBit(uint32_t srcpos, uint32_t dstpos, int bitcnt) const
{
	CopyBit(buf_read, srcpos, buf_write, dstpos, bitcnt);
}



// calculate rounded byte size from bitfield size
inline uint32_t CBitBuffer::CalculateByteSize(uint32_t bitsize)
{
	return ((bitsize + 7) >> 3);
}



// read an integral type as little endian
template <typename T>
inline T CBitBuffer::LittleEndian(const T &t)
{
	// convert to unsigned type to ensure bitshifts are not arithmetic
	std::make_unsigned_t<T> x = 0;

	// the number of bits in the unsigned type used
	int l = std::numeric_limits<decltype(x)>::digits;

	// byte access to the memory holding the source value
	const uint8_t *s = (const uint8_t *)&t;

	// shift each byte into the result
	while ((l -= 8) >= 0)
		x |= (decltype(x))s[l >> 3] << l;

	return x;
}

// read an integral type as big endian
template <typename T>
inline T CBitBuffer::BigEndian(const T &t)
{
	// convert to unsigned type to ensure bitshifts are not arithmetic
	std::make_unsigned_t<T> x = 0;

	// the number of bits in the unsigned type used
	int l = std::numeric_limits<decltype(x)>::digits;

	// avoid undefined behaviour in shift when x is 8 bit
	if (l == 8)
		return t;

	// byte access to the memory holding the source value
	const uint8_t *s = (const uint8_t *)&t;

	// shift each byte into the result
	while ((l -= 8) >= 0) {
		x <<= 8;
		x |= *s++;
	}

	return x;
}

// write an integral type as little endian
template <typename T, typename V>
inline void CBitBuffer::LittleEndian(T &t, V v)
{
	// convert v to unsigned first to ensure bitshifts are not arithmetic
	std::make_unsigned_t<V> y;
	y = static_cast<decltype(y)>(v);

	// now convert v to the same size as t to avoid undefined behaviour when shift count is the same as bit count
	std::make_unsigned_t<T> x;
	x = static_cast<decltype(x)>(y);

	// the number of bits in the unsigned target type used
	int l = std::numeric_limits<std::make_unsigned_t<T>>::digits;

	// byte access to the memory holding the target
	uint8_t *s = (uint8_t *)&t;

	// shift each byte into the result
	while ((l -= 8) >= 0) {
		*s++ = (uint8_t)x;

		// don't care about undefined behaviour when x is 8 bits, since we won't re-use x in; 1 iteration in the loop
		x >>= 8;
	}
}

// write an integral type as little endian
template <typename T, typename V>
inline void CBitBuffer::BigEndian(T &t, V v)
{
	// convert v to unsigned first to ensure bitshifts are not arithmetic
	std::make_unsigned_t<V> y;
	y = static_cast<decltype(y)>(v);

	// now convert v to the same size as t to avoid undefined behaviour when shift count is the same as bit count
	std::make_unsigned_t<T> x;
	x = static_cast<decltype(x)>(y);

	// the number of bits in the unsigned target type used
	int l = std::numeric_limits<decltype(x)>::digits;

	// byte access to the memory holding the target
	uint8_t *s = (uint8_t *)&t;

	// shift each byte into the result
	while ((l -= 8) >= 0)
		*s++ = (uint8_t)(x >> l);
}

#endif
