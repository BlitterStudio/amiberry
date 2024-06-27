#ifndef COMMONTYPES_H
#define COMMONTYPES_H

#ifndef _WIN32
typedef int8_t BYTE;
typedef int16_t WORD;
typedef int32_t DWORD;
#endif

#ifdef AMIGA
#include <exec/types.h>
typedef UBYTE CapsUByte;
typedef LONG  CapsLong;
typedef ULONG CapsULong;
#else
#include <stdint.h>
typedef uint8_t  CapsUByte;
typedef int32_t  CapsLong;
typedef uint32_t CapsULong;
#endif // AMIGA

typedef void *PVOID;
typedef char *PCHAR;

typedef uint8_t UBYTE;
typedef uint16_t UWORD;
typedef uint32_t UDWORD;
typedef uint64_t UQUAD;
typedef int8_t SBYTE;
typedef int16_t SWORD;
typedef int32_t SDWORD;
typedef int64_t SQUAD;

typedef UBYTE *PUBYTE;
typedef UWORD *PUWORD;
typedef UDWORD *PUDWORD;
typedef UQUAD *PUQUAD;
typedef SBYTE *PSBYTE;
typedef SWORD *PSWORD;
typedef SDWORD *PSDWORD;
typedef SQUAD *PSQUAD;

enum {
	DB_0 = 0,
	DB_1,
	DB_2,
	DB_3,
	DB_4,
	DB_5,
	DB_6,
	DB_7,
	DB_8,
	DB_9,
	DB_10,
	DB_11,
	DB_12,
	DB_13,
	DB_14,
	DB_15,
	DB_16,
	DB_17,
	DB_18,
	DB_19,
	DB_20,
	DB_21,
	DB_22,
	DB_23,
	DB_24,
	DB_25,
	DB_26,
	DB_27,
	DB_28,
	DB_29,
	DB_30,
	DB_31
};

#define DF_0 (1UL<<DB_0)
#define DF_1 (1UL<<DB_1)
#define DF_2 (1UL<<DB_2)
#define DF_3 (1UL<<DB_3)
#define DF_4 (1UL<<DB_4)
#define DF_5 (1UL<<DB_5)
#define DF_6 (1UL<<DB_6)
#define DF_7 (1UL<<DB_7)
#define DF_8 (1UL<<DB_8)
#define DF_9 (1UL<<DB_9)
#define DF_10 (1UL<<DB_10)
#define DF_11 (1UL<<DB_11)
#define DF_12 (1UL<<DB_12)
#define DF_13 (1UL<<DB_13)
#define DF_14 (1UL<<DB_14)
#define DF_15 (1UL<<DB_15)
#define DF_16 (1UL<<DB_16)
#define DF_17 (1UL<<DB_17)
#define DF_18 (1UL<<DB_18)
#define DF_19 (1UL<<DB_19)
#define DF_20 (1UL<<DB_20)
#define DF_21 (1UL<<DB_21)
#define DF_22 (1UL<<DB_22)
#define DF_23 (1UL<<DB_23)
#define DF_24 (1UL<<DB_24)
#define DF_25 (1UL<<DB_25)
#define DF_26 (1UL<<DB_26)
#define DF_27 (1UL<<DB_27)
#define DF_28 (1UL<<DB_28)
#define DF_29 (1UL<<DB_29)
#define DF_30 (1UL<<DB_30)
#define DF_31 (1UL<<DB_31)

#define DllImport __declspec(dllimport)
#define DllExport __declspec(dllexport)

#define Naked __declspec(naked)

#ifdef _DEBUG
#define NODEFAULT   assert(0)
#else
#define NODEFAULT   __assume(0)
#endif

#endif
