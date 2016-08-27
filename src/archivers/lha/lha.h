#include "sysconfig.h"
#include "sysdeps.h"
#include "zfile.h"

#define SYSTIME_HAS_NO_TM
#define NODIRECTORY
#define FTIME
#define NOBSTRING
#define NOINDEX
#define MKTIME

/* ------------------------------------------------------------------------ */
/* LHa for UNIX    Archiver Driver											*/
/*																			*/
/*		Modified          		Nobutaka Watazaki							*/
/*																			*/
/*	Ver. 1.14 	Soruce All chagned				1995.01.14	N.Watazaki		*/
/*	Ver. 1.14i 	Modified and bug fixed			2000.10.06	t.okamoto   	*/
/* ------------------------------------------------------------------------ */
/*
	Included...
		lharc.h		interface.h		slidehuf.h
*/
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <signal.h>

#include "lha_macro.h"

struct encode_option {
#if 1 || defined(__STDC__) || defined(AIX)
	void            (*output) ();
	void            (*encode_start) ();
	void            (*encode_end) ();
#else
	int             (*output) ();
	int             (*encode_start) ();
	int             (*encode_end) ();
#endif
};

struct decode_option {
	unsigned short  (*decode_c) ();
	unsigned short  (*decode_p) ();
#if 1 || defined(__STDC__) || defined(AIX)
	void            (*decode_start) ();
#else
	int             (*decode_start) ();
#endif
};

/* ------------------------------------------------------------------------ */
/*	LHa File Type Definition												*/
/* ------------------------------------------------------------------------ */
struct string_pool {
	int             used;
	int             size;
	int             n;
	char           *buffer;
};

typedef struct LzHeader {
	unsigned char   header_size;
	char            method[METHOD_TYPE_STRAGE];
	long            packed_size;
	long            original_size;
	long            last_modified_stamp;
	unsigned char   attribute;
	unsigned char   header_level;
	char            name[256];
	unsigned short  crc;
	boolean         has_crc;
	unsigned char   extend_type;
	unsigned char   minor_version;

	/* extend_type == EXTEND_UNIX  and convert from other type. */
	time_t          unix_last_modified_stamp;
	unsigned short  unix_mode;
	unsigned short  unix_uid;
	unsigned short  unix_gid;
}  LzHeader;

struct interfacing {
	struct zfile		*infile;
	struct zfile		*outfile;
	unsigned long   original;
	unsigned long   packed;
	int             dicbit;
	int             method;
};


/* ------------------------------------------------------------------------ */
/*	Option switch variable													*/
/* ------------------------------------------------------------------------ */
/* command line options (common options) */
EXTERN boolean  quiet;
EXTERN boolean  text_mode;
EXTERN boolean  verbose;
EXTERN boolean  noexec;		/* debugging option */
EXTERN boolean  force;
EXTERN boolean  prof;
EXTERN boolean  delete_after_append;
EXTERN int		compress_method;
EXTERN int		header_level;
/* EXTERN int		quiet_mode; */   /* 1996.8.13 t.okamoto */
#ifdef EUC
EXTERN boolean	euc_mode;
#endif

/* list command flags */
EXTERN boolean  verbose_listing;

/* extract/print command flags */
EXTERN boolean  output_to_stdout;

/* add/update/delete command flags */
EXTERN boolean  new_archive;
EXTERN boolean  update_if_newer;
EXTERN boolean  generic_format;

EXTERN boolean	remove_temporary_at_error;
EXTERN boolean	recover_archive_when_interrupt;
EXTERN boolean	remove_extracting_file_when_interrupt;
EXTERN boolean	get_filename_from_stdin;
EXTERN boolean	ignore_directory;
EXTERN boolean	verify_mode;

/* Indicator flag */
EXTERN int		quiet_mode;

/* ------------------------------------------------------------------------ */
/*	Globale Variable														*/
/* ------------------------------------------------------------------------ */
EXTERN char		**cmd_filev;
EXTERN int      cmd_filec;

EXTERN char		*archive_name;
EXTERN char     expanded_archive_name[FILENAME_LENGTH];
EXTERN char     temporary_name[FILENAME_LENGTH];
EXTERN char     backup_archive_name[FILENAME_LENGTH];

EXTERN char		*reading_filename, *writting_filename;

/* 1996.8.13 t.okamoto */
#if 0
EXTERN boolean  remove_temporary_at_error;
EXTERN boolean  recover_archive_when_interrupt;
EXTERN boolean  remove_extracting_file_when_interrupt;
#endif

EXTERN int      archive_file_mode;
EXTERN int      archive_file_gid;

EXTERN node		*next;
/* EXTERN unsigned short crc; */  /* 1996.8.13 t.okamoto */

EXTERN int      noconvertcase; /* 2000.10.6 */

/* slide.c */
EXTERN int      unpackable;
EXTERN unsigned long origsize, compsize;
EXTERN unsigned short dicbit;
EXTERN unsigned short maxmatch;
EXTERN unsigned long lhcount;
EXTERN unsigned long loc;			/* short -> long .. Changed N.Watazaki */
EXTERN unsigned char *text;
EXTERN int		prev_char;

/* huf.c */
#ifndef LHA_MAIN_SRC  /* t.okamoto 96/2/20 */
EXTERN unsigned short h_left[], h_right[];
EXTERN unsigned char c_len[], pt_len[];
EXTERN unsigned short c_freq[], c_table[], c_code[];
EXTERN unsigned short p_freq[], pt_table[], pt_code[], t_freq[];
#endif

/* append.c */
#ifdef NEED_INCREMENTAL_INDICATOR
EXTERN long		indicator_count;
EXTERN long		indicator_threshold;
#endif

/* crcio.c */
EXTERN struct zfile	*infile, *outfile;
EXTERN unsigned short crc, lhabitbuf;
EXTERN int      dispflg;
EXTERN long		reading_size;

/* from dhuf.c */
EXTERN unsigned int n_max;

/* lhadd.c */
EXTERN FILE		*temporary_fp;

/* ------------------------------------------------------------------------ */
/*	Functions																*/
/* ------------------------------------------------------------------------ */
/* from lharc.c */
extern int		patmatch();

extern void		interrupt();

extern void		message();
extern void		warning();
extern void		error();
extern void		fatal_error();

extern boolean	need_file();
extern int		inquire();
extern FILE		*xfopen();

extern boolean	find_files();
extern void		free_files();

extern void		init_sp();
extern void		add_sp();
extern void		finish_sp();
extern void		free_sp();
extern void		cleaning_files();

extern void		build_temporary_name();
extern void		build_backup_file_name();
extern void		build_standard_archive_name();

extern FILE		*open_old_archive();
extern void		init_header();
extern boolean get_header(struct zfile *fp, LzHeader *hdr);
extern boolean	archive_is_msdos_sfx1();
extern boolean	skip_msdos_sfx1_code();
extern void		write_header();
extern void		write_archive_tail();
extern void		copy_old_one();
extern unsigned char *convdelim(unsigned char *path, unsigned char delim);
extern long		copyfile();

extern void		cmd_list(), cmd_extract(), cmd_add(), cmd_delete();

extern char		*extract_directory;

/* from slide.c */

extern int		encode_alloc();
extern void		encode();
extern int		decode(struct interfacing*);

/* from append.c */
extern void     start_indicator();
extern void     finish_indicator();
extern void     finish_indicator2();

/* slide.c */
extern void     output_st1();
extern unsigned char *alloc_buf();
extern void     encode_start_st1();
extern void     encode_end_st1();
extern unsigned short decode_c_st1();
extern unsigned short decode_p_st1();
extern void     decode_start_st1();

/* from shuf.c */
extern void     decode_start_st0();
extern void     encode_p_st0( /* unsigned short j */ );
extern void     encode_start_fix();
extern void     decode_start_fix();
extern unsigned short decode_c_st0();
extern unsigned short decode_p_st0();

/* from dhuf.c */
extern void     start_c_dyn();
extern void     decode_start_dyn();
extern unsigned short decode_c_dyn();
extern unsigned short decode_p_dyn();
extern void     output_dyn( /* int code, unsigned int pos */ );
extern void     encode_end_dyn();

extern int      decode_lzhuf();

/* from larc.c */

extern unsigned short decode_c_lzs();
extern unsigned short decode_p_lzs();
extern unsigned short decode_c_lz5();
extern unsigned short decode_p_lz5();
extern void			  decode_start_lzs();
extern void			  decode_start_lz5();

extern void lha_make_table(short nchar, unsigned char bitlen[], short tablebits, unsigned short table[]);


/* from maketree.c */
/*
 * void make_code(short n, uchar len[], ushort code[]); short make_tree(short
 * nparm, ushort freqparm[], uchar lenparm[], ushort codeparam[]);
 */
extern void		make_code( /* int n, uchar len[], ushort code[] */ );
extern short	make_tree( /* int nparm, ushort freqparm[], uchar lenparm[],
								ushort codeparam[] */ );

/* from crcio.c */
extern void				make_crctable();
extern unsigned short	calccrc(unsigned char *p, unsigned int n);
extern void				fillbuf(unsigned char n);
extern unsigned short	getbits(unsigned char n);
extern void				putcode(unsigned char n, unsigned short x);
extern void				putbits(unsigned char n, unsigned short x);
extern int				fread_crc(unsigned char *p, int n, struct zfile *f);
extern void				fwrite_crc(unsigned char *p, int n, struct zfile *f);
extern void				init_getbits();
extern void				init_putbits();
extern void     		make_crctable();
extern unsigned 		short calccrc();

/* from lhadd.c */
extern int		encode_lzhuf();
extern int      encode_stored_crc();

#define warning write_log
#define fatal_error write_log
#define error write_log
