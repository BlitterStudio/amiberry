#ifndef UILSB_H
#define UILSB_H

#include <stdint.h>
#include <limits.h>
#if __cplusplus >= 201103L || defined(_MSC_VER)
#include <type_traits>
#endif

template<class T> class uilsb
{

#if __cplusplus >= 201103L || defined(_MSC_VER)
	static_assert(std::is_integral<T>::value,
		"Only integer types supported.");
	static_assert(!std::is_signed<T>::value,
		"Only unsigned types supported.");
#endif

public:
	operator T () {
		return getValue();
	}
	const T operator=(const T v) {
		setValue(v);
		return v;
	}

private:
	static const int nBytes = sizeof(T)*CHAR_BIT / 8;
	uint8_t data[nBytes];

	inline T getValue();
	inline void setValue(const T v);
};

typedef uilsb<uint16_t> uilsb16_t;
typedef uilsb<uint32_t> uilsb32_t;

template<> inline uint16_t uilsb16_t::getValue()
{
	return CBitBuffer::ReadBitLE16(data);
}

template<> inline void uilsb16_t::setValue(const uint16_t v)
{
	CBitBuffer::WriteBitLE16(data, v);
}

template<> inline uint32_t uilsb32_t::getValue()
{
	return CBitBuffer::ReadBitLE32(data);
}

template<> inline void uilsb32_t::setValue(const uint32_t v)
{
	CBitBuffer::WriteBitLE32(data, v);
}

#endif
