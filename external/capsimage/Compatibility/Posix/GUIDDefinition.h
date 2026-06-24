#ifndef GUIDDEFINITION_H
#define GUIDDEFINITION_H

#define DEFINE_GUID(name, dw, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	unsigned char name[] = { \
		(dw >> 24) & 0xFF, \
		(dw >> 16) & 0xFF, \
		(dw >>  8) & 0xFF, \
		(dw      ) & 0xFF, \
		(w1 >>  8) & 0xFF, \
		(w1      ) & 0xFF, \
		(w2 >>  8) & 0xFF, \
		(w2      ) & 0xFF, \
		b1, b2, b3, b4,    \
		b5, b6, b7, b8     \
		};

#endif
