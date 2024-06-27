#include "stdafx.h"



CBitBuffer::CBitBuffer()
{
	Clear();
}

CBitBuffer::~CBitBuffer()
{
}

// clear variables
void CBitBuffer::Clear()
{
	bufmem = NULL;
	bufsize = 0;
	bufbits = 0;
}

// initialize byte sized class associated buffer
void CBitBuffer::InitByteSize(uint8_t *buf, uint32_t bytesize)
{
	bufmem = buf;
	bufsize = bytesize;
	bufbits = bytesize << 3;
}

// initialize bit sized class associated buffer
void CBitBuffer::InitBitSize(uint8_t *buf, uint32_t bitsize)
{
	bufmem = buf;
	bufsize = CalculateByteSize(bitsize);
	bufbits = bitsize;
}



// read up to 32 data bits from the specified buffer, wrap around
uint32_t CBitBuffer::ReadBitWrap(uint8_t *buf, uint32_t bufwrap, uint32_t bitpos, int bitcnt)
{
	uint32_t res;

	// slow read mode if buffer would wrap around during reading, otherwise use fast read
	if (bitpos + bitcnt > bufwrap) {
		// slow mode; read bit by bit
		res = 0;

		// read all bits
		while (bitcnt-- > 0) {
			// shift in the next bit
			res <<= 1;
			if (ReadBit(buf, bitpos))
				res |= 1;

			// advance bit position
			bitpos++;

			// handle buffer wrap around
			if (bitpos >= bufwrap)
				bitpos -= bufwrap;
		}
	} else {
		// fast mode; read at once
		res = ReadBit(buf, bitpos, bitcnt);
	}

	return res;
}

// read up to 32 data bits from the specified buffer
uint32_t CBitBuffer::ReadBit(uint8_t *buf, uint32_t bitpos, int bitcnt)
{
	// result is 0 if bit count is 0 or less
	if (bitcnt <= 0)
		return 0;

	uint32_t res = 0;

	// byte offset in buffer
	uint32_t bytepos = bitpos >> 3;

	// bit mask for current byte
	uint8_t bitmask = 1 << ((bitpos & 7) ^ 7);

	// current data byte from buffer
	uint8_t value = buf[bytepos++];

	// read all bits
	while (bitcnt-- > 0) {
		// if all bits have been processed
		if (!bitmask) {
			// reset bit mask to b7 (reading b7...b0)
			bitmask = 0x80;

			// read next data byte from buffer
			value = buf[bytepos++];
		}

		// shift in the masked bit
		res <<= 1;
		if (value & bitmask)
			res |= 1;

		// advance bit mask to next position
		bitmask >>= 1;
	}

	return res;
}



// write up to 32 data bits to the specified buffer, wrap around
void CBitBuffer::WriteBitWrap(uint8_t *buf, uint32_t bufwrap, uint32_t bitpos, uint32_t value, int bitcnt)
{
	// slow write mode if buffer would wrap around during writing, otherwise use fast write
	if (bitpos + bitcnt > bufwrap) {
		// slow mode; set/clear each bit affected and handle buffer wrap

		// bitfield has to be at least 1 bit wide, otherwise finished
		if (bitcnt <= 0)
			return;

		// read bitmask; use bit[bitcnt-1...0] as bitfield from value
		uint32_t rbit = 1 << (bitcnt - 1);

		// write bitmask
		uint32_t wbit = 1 << ((bitpos & 7) ^ 7);

		// write buffer position
		uint8_t *wpos = buf + (bitpos >> 3);

		// read data from buffer
		uint32_t data = *wpos;

		// process all bits
		while (rbit) {
			// set/clear current bit to be written according to the value of the bit selected by the read bitmask
			if (value & rbit)
				data |= wbit;
			else
				data &= ~wbit;

			// get next bit position to read and write
			rbit >>= 1;
			bitpos++;

			if (bitpos == bufwrap) {
				// buffer wraps, commit data
				bitpos = 0;
				*wpos = (uint8_t)data;

				// restart buffer from bit#7
				wpos = buf;
				wbit = 0x80;
				data = *wpos;
			} else {
				if (!(wbit >>= 1)) {
					// data complete, commit and get the next one
					*wpos = (uint8_t)data;
					wpos++;
					wbit = 0x80;
					data = *wpos;
				}
			}
		}

		// commit data
		*wpos = (uint8_t)data;
	} else {
		// fast mode; write at once
		WriteBit(buf, bitpos, value, bitcnt);
	}
}

// write up to 32 data bits to the specified buffer
void CBitBuffer::WriteBit(uint8_t *buf, uint32_t bitpos, uint32_t value, int bitcnt)
{
	// write buffer position
	uint8_t *wpos = buf + (bitpos >> 3);

	while (bitcnt > 0) {
		// start bit offset in byte
		int bofs = bitpos & 7;

		// number of bits to be written in this byte
		int bcnt = 8 - bofs;
		if (bcnt > bitcnt)
			bcnt = bitcnt;

		// next bit position
		bitpos += bcnt;

		// remaining bits
		bitcnt -= bcnt;

		// move the data bits to be written right justified to bit#0
		uint32_t data = value >> bitcnt;

		// process partial byte if needed
		if (bcnt != 8) {
			// left justify data to start from bofs, clear unused bits
			data <<= (8 - (bofs + bcnt));

			// mask for bcnt number of bits left justified
			uint32_t mask = (0xff00 >> bcnt) & 0xff;

			// move mask to the write position, keep any other bit cleared
			mask >>= bofs;

			// clear bitfield value where original content should be kept
			data &= mask;

			// merge original byte value with data
			data |= (*wpos & ~mask);
		}

		// commit data
		*wpos++ = (uint8_t)data;
	}
}



// clear bit field in the specified buffer, wrap around
void CBitBuffer::ClearBitWrap(uint8_t *buf, uint32_t bufwrap, uint32_t bitpos, int bitcnt)
{
	// slow write mode if buffer would wrap around during writing, otherwise use fast write
	if (bitpos + bitcnt > bufwrap) {
		while (bitcnt > 0) {
			// write largest possible bitfield
			int writebc = (bitcnt >= MAX_BITBUFFER_LEN) ? MAX_BITBUFFER_LEN : bitcnt;

			// write 0 as bitfield
			WriteBitWrap(buf, bufwrap, bitpos, 0, writebc);

			// next write position
			bitcnt -= writebc;
			bitpos += writebc;

			// handle buffer wrap around
			if (bitpos >= bufwrap)
				bitpos -= bufwrap;
		}
	}	else {
		// fast mode; write at once
		ClearBit(buf, bitpos, bitcnt);
	}
}

// clear bit field in the specified buffer
void CBitBuffer::ClearBit(uint8_t *buf, uint32_t bitpos, int bitcnt)
{
	while (bitcnt > 0) {
		// write largest possible bitfield
		int writebc = (bitcnt >= MAX_BITBUFFER_LEN) ? MAX_BITBUFFER_LEN : bitcnt;

		// write 0 as bitfield
		WriteBit(buf, bitpos, 0, writebc);

		// next write position
		bitcnt -= writebc;
		bitpos += writebc;
	}
}



// compare two bitfields in the specified buffers, return 0 if they are identical
int CBitBuffer::CompareBit(uint8_t *buf1, uint32_t buf1pos, uint8_t *buf2, uint32_t buf2pos, int bitcnt)
{
	// compare every bit in the two fields, 32 bit at a time, and finally the remaining bits
	while (bitcnt > 0) {
		uint32_t s1, s2;

		if (bitcnt >= 32) {
			// read 32 bits from each field if the remaining field size is at least 32
			s1 = ReadBit32(buf1, buf1pos);
			s2 = ReadBit32(buf2, buf2pos);
			bitcnt -= 32;
			buf1pos += 32;
			buf2pos += 32;
		} else {
			// read the remaining 1...31 bits
			s1 = ReadBit(buf1, buf1pos, bitcnt);
			s2 = ReadBit(buf2, buf2pos, bitcnt);
			bitcnt = 0;
		}

		// error if fields are different
		if (s1 != s2)
			return -1;
	}

	// all bits compared, the fields are identical
	return 0;
}

// compare two bitfields in the specified buffers, return the number of the first N identical bits
int CBitBuffer::CompareAndCountBit(uint8_t *buf1, uint32_t buf1pos, uint8_t *buf2, uint32_t buf2pos, int bitcnt)
{
	// number of bits processed, size of current block
	int bitproc = 0, blocksize;
	uint32_t diff;

	// compare every bit in the two fields, in 32 bit blocks, and finally the remaining 1...31 bits
	while (bitcnt > 0) {
		uint32_t s1, s2;

		if (bitcnt >= 32) {
			// process 32 bits if the remaining field size is at least 32
			blocksize = 32;

			// read 32 bits from each field 
			s1 = ReadBit32(buf1, buf1pos);
			s2 = ReadBit32(buf2, buf2pos);
		} else {
			// process the remaining 1...31 bits
			blocksize = bitcnt;

			// read the remaining 1...31 bits from each field
			s1 = ReadBit(buf1, buf1pos, blocksize);
			s2 = ReadBit(buf2, buf2pos, blocksize);
		}

		// bits that are different in the two fiels are set to 1
		diff = s1 ^ s2;

		// stop if fields are different (any of the bits set)
		if (diff)
			break;

		// advance to the next block
		buf1pos += blocksize;
		buf2pos += blocksize;
		bitproc += blocksize;
		bitcnt -= blocksize;
	}

	// difference found, count the remaining identical bits
	if (bitcnt > 0) {
		// find the first bit set from b<blocksize-1>...0
		for (uint32_t mask = 1U << (blocksize - 1); mask; mask >>= 1) {
			if (diff & mask)
				break;

			// bit not set, another identical bit
			bitproc++;
		}
	}

	// return the number of bits processed and found to be identical
	return bitproc;
}



// copy a bitfield in the specified buffers, wrap around
void CBitBuffer::CopyBitWrap(uint8_t *srcbuf, uint32_t srcwrap, uint32_t srcpos, uint8_t *dstbuf, uint32_t dstwrap, uint32_t dstpos, int bitcnt)
{
	// copy areas in largest possible size up to the track wrap points
	while (bitcnt > 0) {
		// clamp size to largest size to copy until the destination wrap point
		int csize = (dstpos + bitcnt > dstwrap) ? dstwrap - dstpos : bitcnt;

		// clamp size to largest size to copy until the source wrap point
		if (srcpos + csize > srcwrap)
			csize = srcwrap - srcpos;

		// copy the field; it is guaranteed that source or destionation does not wrap
		CopyBit(srcbuf, srcpos, dstbuf, dstpos, csize);

		// advance bit positions forward
		srcpos += csize;
		dstpos += csize;
		bitcnt -= csize;

		// handle buffer wrap around
		if (srcpos >= srcwrap)
			srcpos -= srcwrap;

		if (dstpos >= dstwrap)
			dstpos -= dstwrap;
	}
}

void CBitBuffer::CopyBit(uint8_t *srcbuf, uint32_t srcpos, uint8_t *dstbuf, uint32_t dstpos, int bitcnt)
{
	// copy the field in 32 bit blocks, and finally the remaining 1...31 bits
	while (bitcnt > 0) {
		uint32_t value;
		int blocksize;

		if (bitcnt >= 32) {
			// process 32 bits if the remaining field size is at least 32
			blocksize = 32;

			// read 32 bits from source
			value = ReadBit32(srcbuf, srcpos);
		} else {
			// process the remaining 1...31 bits
			blocksize = bitcnt;

			// read the remaining 1...31 bits from each field
			value = ReadBit(srcbuf, srcpos, blocksize);
		}

		// write the field
		WriteBit(dstbuf, dstpos, value, blocksize);

		// advance to the next block
		srcpos += blocksize;
		dstpos += blocksize;
		bitcnt -= blocksize;
	}
}

