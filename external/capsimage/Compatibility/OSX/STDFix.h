#ifndef STDFIX_H
#define STDFIX_H

// add missing STD features
namespace std {

#ifndef __clang__
// This function isn't implemented in GCC 4.x
#if (defined(__GNUC__) && (__GNUC__ < 5))

	// FUNCTION align
inline void *align(size_t _Bound, size_t _Size,
	void *& _Ptr, size_t& _Space)
{	// try to carve out _Size bytes on boundary _Bound
	size_t _Off = (size_t)((uintptr_t)_Ptr & (_Bound - 1));
	if (0 < _Off)
		_Off = _Bound - _Off;	// number of bytes to skip
	if (_Space < _Off || _Space - _Off < _Size)
		return (0);
	else
	{	// enough room, update
		_Ptr = (char *)_Ptr + _Off;
		_Space -= _Off;
		return (_Ptr);
	}
}

#endif // __GNUC__ < 5
#endif //_clang__
}

#endif // STDFIX_H
