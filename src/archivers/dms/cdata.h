
/*
 *     xDMS  v1.3  -  Portable DMS archive unpacker  -  Public Domain
 *     Written by     Andre Rodrigues de la Rocha  <adlroc@usa.net>
 *
 *     Main types of variables used in xDMS, some implementation
 *     dependant features and other global stuff
 */


#ifndef UCHAR
#define UCHAR unsigned char
#endif

#ifndef USHORT
#define USHORT unsigned short
#endif

#ifndef SHORT
#define SHORT short
#endif

#ifndef ULONG
#define ULONG unsigned long
#endif



#ifndef INLINE
	#ifdef __cplusplus
		#define INLINE inline
	#else
		#ifdef __GNUC__
			#define INLINE inline
		#else
			#ifdef __SASC
				#define INLINE __inline
			#else
				#define INLINE static
			#endif
		#endif
	#endif
#endif


#ifndef UNDER_DOS
	#ifdef __MSDOS__
		#define UNDER_DOS
	#else
		#ifdef __MSDOS
			#define UNDER_DOS
		#else
			#ifdef _OS2
				#define UNDER_DOS
			#else
				#ifdef _QC
					#define UNDER_DOS
				#endif
			#endif
		#endif
	#endif
#endif


#ifndef DIR_CHAR
	#ifdef UNDER_DOS
		/* running under MSDOS or DOS-like OS */
		#define DIR_CHAR '\\'
	#else
		#define DIR_CHAR '/'
	#endif
#endif


#define DIR_SEPARATORS ":\\/"


extern UCHAR *dms_text;
extern USHORT dms_lastlen, dms_np;

