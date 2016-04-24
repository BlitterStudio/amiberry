/* ------------------------------------------------------------------------ */
/* LHa for UNIX    Archiver Driver	macro define							*/
/*																			*/
/*		Modified          		Nobutaka Watazaki							*/
/*																			*/
/*	Ver. 1.14	Soruce All chagned				1995.01.14	N.Watazaki		*/
/*	Ver. 1.14g	modified   						2000.05.06	T.OKAMOTO		*/
/* ------------------------------------------------------------------------ */

/* external variables */
//extern int      errno;

/* used by qsort() for alphabetic-sort */
#define STRING_COMPARE(a,b)		strcmp((a),(b))

#define FILENAME_LENGTH	1024

/* ------------------------------------------------------------------------ */
/* YOUR CUSTOMIZIES															*/
/* ------------------------------------------------------------------------ */

#ifndef ARCHIVENAME_EXTENTION
#define ARCHIVENAME_EXTENTION	".lzh"
#endif
#ifndef BACKUPNAME_EXTENTION
#define BACKUPNAME_EXTENTION	".bak"
#endif
#ifndef TMP_FILENAME_TEMPLATE
#define TMP_FILENAME_TEMPLATE	"/tmp/lhXXXXXX"
#endif

#define SJC_FIRST_P(c)			\
  (((unsigned char)(c) >= 0x80) &&	\
   (((unsigned char)(c) < 0xa0) ||	\
    ((unsigned char)(c) >= 0xe0) &&	\
    ((unsigned char)(c) < 0xfd)))
#define SJC_SECOND_P(c)			\
  (((unsigned char)(c) >= 0x40) &&	\
   ((unsigned char)(c) < 0xfd) &&	\
   ((unsigned char)(c) != 0x7f))

#ifdef MULTIBYTE_CHAR
#define MULTIBYTE_FIRST_P	SJC_FIRST_P
#define MULTIBYTE_SECOND_P	SJC_SECOND_P
#endif				/* MULTIBYTE_CHAR */

/* ------------------------------------------------------------------------ */
/*	LHa File Definitions													*/
/* ------------------------------------------------------------------------ */
#ifdef S_IFLNK
#define GETSTAT lstat
#else
#define GETSTAT stat
#endif

#ifdef LHA_MAIN_SRC
#define EXTERN
#else
#define EXTERN				extern
#endif	/* LHA_MAIN_SRC */

#define LZHUFF0_METHOD			"-lh0-"
#define LZHUFF1_METHOD			"-lh1-"
#define LZHUFF2_METHOD			"-lh2-"
#define LZHUFF3_METHOD			"-lh3-"
#define LZHUFF4_METHOD			"-lh4-"
#define LZHUFF5_METHOD			"-lh5-"
#define LZHUFF6_METHOD			"-lh6-"
#define LZHUFF7_METHOD			"-lh7-"
#define LARC_METHOD				"-lzs-"
#define LARC5_METHOD			"-lz5-"
#define LARC4_METHOD			"-lz4-"
#define LZHDIRS_METHOD			"-lhd-"

#define METHOD_TYPE_STRAGE		5

/* Added N.Watazaki ..V */
#define LZHUFF0_METHOD_NUM		0
#define LZHUFF1_METHOD_NUM		1
#define LZHUFF2_METHOD_NUM		2
#define LZHUFF3_METHOD_NUM		3
#define LZHUFF4_METHOD_NUM		4
#define LZHUFF5_METHOD_NUM		5
#define LZHUFF6_METHOD_NUM		6
#define LZHUFF7_METHOD_NUM		7
#define LARC_METHOD_NUM			8
#define LARC5_METHOD_NUM		9
#define LARC4_METHOD_NUM		10
#define LZHDIRS_METHOD_NUM		11
/* Added N.Watazaki ..^ */

#define I_HEADER_SIZE			0
#define I_HEADER_CHECKSUM		1
#define I_METHOD				2
#define I_PACKED_SIZE			7
#define I_ORIGINAL_SIZE			11
#define I_LAST_MODIFIED_STAMP	15
#define I_ATTRIBUTE				19
#define I_HEADER_LEVEL			20
#define I_NAME_LENGTH			21
#define I_NAME					22

#define I_CRC						22	/* + name_length */
#define I_EXTEND_TYPE				24	/* + name_length */
#define I_MINOR_VERSION				25	/* + name_length */
#define I_UNIX_LAST_MODIFIED_STAMP	26	/* + name_length */
#define I_UNIX_MODE					30	/* + name_length */
#define I_UNIX_UID					32	/* + name_length */
#define I_UNIX_GID					34	/* + name_length */
#define I_UNIX_EXTEND_BOTTOM		36	/* + name_length */

#define I_GENERIC_HEADER_BOTTOM		I_EXTEND_TYPE

#define EXTEND_GENERIC			0
#define EXTEND_UNIX				'U'
#define EXTEND_MSDOS			'M'
#define EXTEND_MACOS			'm'
#define EXTEND_OS9				'9'
#define EXTEND_OS2				'2'
#define EXTEND_OS68K			'K'
#define EXTEND_OS386			'3'	/* OS-9000??? */
#define EXTEND_HUMAN			'H'
#define EXTEND_CPM				'C'
#define EXTEND_FLEX				'F'
#define EXTEND_RUNSER			'R'
#define EXTEND_AMIGAOS			'A'

/* this OS type is not official */

#define EXTEND_TOWNSOS			'T'
#define EXTEND_XOSK				'X'

/*---------------------------------------------------------------------------*/

#define GENERIC_ATTRIBUTE				0x20
#define GENERIC_DIRECTORY_ATTRIBUTE		0x10
#define HEADER_LEVEL0					0x00
#define HEADER_LEVEL1					0x01
#define HEADER_LEVEL2					0x02

#define CURRENT_UNIX_MINOR_VERSION		0x00

#define DELIM		('/')
#define DELIM2		(0xff)
#define DELIMSTR	"/"

#define OSK_RW_RW_RW			0000033
#define OSK_FILE_REGULAR		0000000
#define OSK_DIRECTORY_PERM		0000200
#define OSK_SHARED_PERM			0000100
#define OSK_OTHER_EXEC_PERM		0000040
#define OSK_OTHER_WRITE_PERM	0000020
#define OSK_OTHER_READ_PERM		0000010
#define OSK_OWNER_EXEC_PERM		0000004
#define OSK_OWNER_WRITE_PERM	0000002
#define OSK_OWNER_READ_PERM		0000001

#define UNIX_FILE_TYPEMASK		0170000
#define UNIX_FILE_REGULAR		0100000
#define UNIX_FILE_DIRECTORY		0040000
#define UNIX_FILE_SYMLINK		0120000
#define UNIX_SETUID				0004000
#define UNIX_SETGID				0002000
#define UNIX_STYCKYBIT			0001000
#define UNIX_OWNER_READ_PERM	0000400
#define UNIX_OWNER_WRITE_PERM	0000200
#define UNIX_OWNER_EXEC_PERM	0000100
#define UNIX_GROUP_READ_PERM	0000040
#define UNIX_GROUP_WRITE_PERM	0000020
#define UNIX_GROUP_EXEC_PERM	0000010
#define UNIX_OTHER_READ_PERM	0000004
#define UNIX_OTHER_WRITE_PERM	0000002
#define UNIX_OTHER_EXEC_PERM	0000001
#define UNIX_RW_RW_RW			0000666

#define LZHEADER_STRAGE			4096

#define MAX_INDICATOR_COUNT		64

typedef short   				node;

/* ------------------------------------------------------------------------ */
/*	Slide relation															*/
/* ------------------------------------------------------------------------ */
#if defined(__STDC__) || defined(AIX)

#include <limits.h>

#else

#ifndef CHAR_BIT
#define CHAR_BIT  8
#endif

#ifndef UCHAR_MAX
#define UCHAR_MAX				((1<<(sizeof(unsigned char)*8))-1)
#endif

#ifndef USHRT_MAX
#define USHRT_MAX				((1<<(sizeof(unsigned short)*8))-1)
#endif

#ifndef SHRT_MAX
#define SHRT_MAX				((1<<(sizeof(short)*8-1))-1)
#endif

#ifndef SHRT_MIN
#define SHRT_MIN				(SHRT_MAX-USHRT_MAX)
#endif

#ifndef ULONG_MAX
#define ULONG_MAX	((1<<(sizeof(unsigned long)*8))-1)
#endif

#ifndef LONG_MAX
#define LONG_MAX	((1<<(sizeof(long)*8-1))-1)
#endif

#ifndef LONG_MIN
#define LONG_MIN	(LONG_MAX-ULONG_MAX)
#endif

#endif	/* not __STDC__ */

/* ------------------------------------------------------------------------ */
/*	FILE Attribute															*/
/* ------------------------------------------------------------------------ */
#define is_directory(statp)		(((statp)->st_mode & S_IFMT) == S_IFDIR)
#define is_symlink(statp)		(((statp)->st_mode & S_IFMT) == S_IFLNK)
#define is_regularfile(statp)	(((statp)->st_mode & S_IFMT) == S_IFREG)

#ifdef MSDOS
#define WRITE_BINARY	"wb"
#define READ_BINARY		"rb"
#else
#define WRITE_BINARY	"w"
#define READ_BINARY		"r"
#endif

/* ------------------------------------------------------------------------ */
/*	Memory and String function												*/
/* ------------------------------------------------------------------------ */
#include <string.h>

#ifdef NOINDEX
#define index			strchr
#define rindex			strrchr
#endif	/* NOINDEX */

#ifdef NOBSTRING
#define bcmp(a,b,n)		memcmp ((a),(b),(n))
#define bzero(d,n)		memset((d),0,(n))
#define bcopy(s,d,n)	memmove((d),(s),(n))
#endif	/* NOBSTRING */

#ifdef USESTRCASECMP
#define strucmp(p,q)	strcasecmp((p),(q))
#endif

/* ------------------------------------------------------------------------ */
/* Individual macro define													*/
/* ------------------------------------------------------------------------ */

/* from crcio.c */
#define CRCPOLY			0xA001		/* CRC-16 */
#define UPDATE_CRC(c)	crc = crctable[(crc ^ (c)) & 0xFF] ^ (crc >> CHAR_BIT)

/* dhuf.c */
#define N_CHAR      (256 + 60 - THRESHOLD + 1)
#define TREESIZE_C  (N_CHAR * 2)
#define TREESIZE_P  (128 * 2)
#define TREESIZE    (TREESIZE_C + TREESIZE_P)
#define ROOT_C      0
#define ROOT_P      TREESIZE_C

/* header.c */
#define setup_get(PTR)	(get_ptr = (PTR))
#define get_byte()		(*get_ptr++ & 0xff)
#define put_ptr			get_ptr
#define setup_put(PTR)	(put_ptr = (PTR))
#define put_byte(c)		(*put_ptr++ = (char)(c))

/* huf.c */
#define NP			(MAX_DICBIT + 1)
#define NT			(USHRT_BIT + 3)
#if 0
#define PBIT		4		/* smallest integer such that (1 << PBIT) > * NP */
#define TBIT 		5		/* smallest integer such that (1 << TBIT) > * NT */
#endif

#define PBIT		5		/* smallest integer such that (1 << PBIT) > * NP */
#define TBIT 		5		/* smallest integer such that (1 << TBIT) > * NT */

#define NC 			(UCHAR_MAX + MAXMATCH + 2 - THRESHOLD)

/*		#if NT > NP #define NPT NT #else #define NPT NP #endif	*/
#define NPT			0x80

/* larc.c */
#define MAGIC0		18
#define MAGIC5		19

/* lharc.c */
#define CMD_UNKNOWN	0
#define CMD_EXTRACT	1
#define CMD_ADD		2
#define CMD_LIST	3
#define CMD_DELETE	4

#define STREQU(a,b)	(((a)[0] == (b)[0]) ? (strcmp ((a),(b)) == 0) : FALSE)

/* shuf.c */
#define N1			286				/* alphabet size */
#define N2			(2 * N1 - 1)	/* # of nodes in Huffman tree */
#define EXTRABITS	8				/* >= log2(F-THRESHOLD+258-N1) */
#define BUFBITS		16				/* >= log2(MAXBUF) */
#define LENFIELD	4				/* bit size of length field for tree output */

/* util.c */
#define BUFFERSIZE	2048
#define MAXSFXCODE	1024*64

#ifndef NULL
#define NULL		(char *)0
#endif

/* slide.c */
/*
#define PERCOLATE  1
#define NIL        0
#define HASH(p, c) ((p) + ((c) << hash1) + hash2)
*/

/* slide.c */
#ifdef SUPPORT_LH7
#define MAX_DICBIT			16      /* lh7 use 16bits */
#endif

#ifndef SUPPORT_LH7
#define MAX_DICBIT			15      /* lh6 use 15bits */
#endif

#define MAX_DICSIZ			(1 << MAX_DICBIT)
#define MATCHBIT			8	/* bits for MAXMATCH - THRESHOLD */
#define MAXMATCH			256	/* formerly F (not more than UCHAR_MAX + 1) */
#define THRESHOLD			3	/* choose optimal value */

/* from huf.c */

/* alphabet = {0, 1, 2, ..., NC - 1} */
#define CBIT				9	/* $\lfloor \log_2 NC \rfloor + 1$ */
#define USHRT_BIT			16	/* (CHAR_BIT * sizeof(ushort)) */

/* Local Variables: */
/* mode:c */
/* tab-width:4 */
/* End: */
