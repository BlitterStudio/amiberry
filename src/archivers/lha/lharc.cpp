/* ------------------------------------------------------------------------ */
/* LHa for UNIX    															*/
/*				lharc.c -- append to archive								*/
/*																			*/
/*		Copyright (C) MCMLXXXIX Yooichi.Tagawa								*/
/*		Modified          		Nobutaka Watazaki							*/
/*							Thanks to H.Yoshizaki. (MS-DOS LHarc)			*/
/*																			*/
/*  Ver. 0.00  Original							1988.05.23  Y.Tagawa		*/
/*  Ver. 0.01  Alpha Version (for 4.2BSD)		1989.05.28  Y.Tagawa		*/
/*  Ver. 0.02  Alpha Version Rel.2				1989.05.29  Y.Tagawa		*/
/*  Ver. 0.03  Release #3  Beta Version			1989.07.02  Y.Tagawa		*/
/*  Ver. 0.03a Debug							1989.07.03  Y.Tagawa		*/
/*  Ver. 0.03b Modified							1989.07.13  Y.Tagawa		*/
/*  Ver. 0.03c Debug (Thanks to void@rena.dit.junet)						*/
/*												1989.08.09  Y.Tagawa		*/
/*  Ver. 0.03d Modified (quiet and verbose)		1989.09.14  Y.Tagawa		*/
/*  V1.00  Fixed								1989.09.22  Y.Tagawa		*/
/*  V1.01  Bug Fixed							1989.12.25  Y.Tagawa		*/
/*																			*/
/*  DOS-Version Original LHx V C2.01 		(C) H.Yohizaki					*/
/*																			*/
/*  V2.00  UNIX Lharc + DOS LHx -> OSK LHx		1990.11.01  Momozou			*/
/*  V2.01  Minor Modified						1990.11.24  Momozou			*/
/*																			*/
/*  Ver. 0.02  LHx for UNIX						1991.11.18  M.Oki			*/
/*  Ver. 0.03  LHa for UNIX						1991.12.17  M.Oki			*/
/*  Ver. 0.04  LHa for UNIX	beta version		1992.01.20  M.Oki			*/
/*  Ver. 1.00  LHa for UNIX	Fixed				1992.03.19  M.Oki			*/
/*																			*/
/*  Ver. 1.10  for Symblic Link					1993.06.25  N.Watazaki		*/
/*  Ver. 1.11  for Symblic Link	Bug Fixed		1993.08.18  N.Watazaki		*/
/*  Ver. 1.12  for File Date Check				1993.10.28  N.Watazaki		*/
/*  Ver. 1.13  Bug Fixed (Idicator calcurate)	1994.02.21  N.Watazaki		*/
/*  Ver. 1.13a Bug Fixed (Sym. Link delete)		1994.03.11  N.Watazaki		*/
/*	Ver. 1.13b Bug Fixed (Sym. Link delete)		1994.07.29  N.Watazaki		*/
/*	Ver. 1.14  Source All chagned				1995.01.14	N.Watazaki		*/
/*	Ver. 1.14b,c  Bug Fixed                     1996.03.07  t.okamoto		*/
/*  Ver. 1.14d Version up                       1997.01.12  t.okamoto       */
/*  Ver. 1.14g Bug Fixed                        2000.05.06  t.okamoto       */
/*  Ver. 1.14i Modified                         2000.10.06  t.okamoto       */
/* ------------------------------------------------------------------------ */
#define LHA_MAIN_SRC

#include "lha.h"

/* ------------------------------------------------------------------------ */
/*								PROGRAM										*/
/* ------------------------------------------------------------------------ */
static int      cmd = CMD_UNKNOWN;

/* 1996.8.13 t.okamoto */
#if 0
char          **cmd_filev;
int             cmd_filec;

char           *archive_name;
char            expanded_archive_name[FILENAME_LENGTH];
char            temporary_name[FILENAME_LENGTH];
char            backup_archive_name[FILENAME_LENGTH];
#endif

/* static functions */
static void     sort_files();
static void		print_version();

char		    *extract_directory = NULL;
char		  **xfilev;
int             xfilec = 257;

/* 1996.8.13 t.okamoto */
#if 0
char           *writting_filename;
char           *reading_filename;

int             archive_file_mode;
int             archive_file_gid;
#endif
/* ------------------------------------------------------------------------ */
static void
init_variable()		/* Added N.Watazaki */
{
/* options */
	quiet			= FALSE;
	text_mode		= FALSE;
	verbose			= FALSE;
	noexec			= FALSE;	/* debugging option */
	force			= FALSE;
	prof			= FALSE;
#ifndef SUPPORT_LH7
	compress_method = LZHUFF5_METHOD_NUM;
#endif
#ifdef SUPPORT_LH7
	compress_method = LZHUFF7_METHOD_NUM;
#endif

	header_level	= HEADER_LEVEL1;
	quiet_mode		= 0;

#ifdef EUC
	euc_mode		= FALSE;
#endif

/* view command flags */
	verbose_listing = FALSE;

/* extract command flags */
	output_to_stdout = FALSE;

/* append command flags */
	new_archive			= FALSE;
	update_if_newer		= FALSE;
	delete_after_append = FALSE;
	generic_format		= FALSE;

	remove_temporary_at_error 				= FALSE;
	recover_archive_when_interrupt			= FALSE;
	remove_extracting_file_when_interrupt	= FALSE;
	get_filename_from_stdin					= FALSE;
	ignore_directory						= FALSE;
	verify_mode								= FALSE;

	noconvertcase							= FALSE;

	extract_directory = NULL;
	xfilec = 257;
}

/* ------------------------------------------------------------------------ */
/*																			*/
/* ------------------------------------------------------------------------ */
static int sort_by_ascii(char **a, char **b)
{
	register char  *p, *q;
	register int    c1, c2;

	p = *a, q = *b;
	if (generic_format) {
		do {
			c1 = *(unsigned char *) p++;
			c2 = *(unsigned char *) q++;
			if (!c1 || !c2)
				break;
			if (islower(c1))
				c1 = toupper(c1);
			if (islower(c2))
				c2 = toupper(c2);
		}
		while (c1 == c2);
		return c1 - c2;
	}
	else {
		while (*p == *q && *p != '\0')
			p++, q++;
		return *(unsigned char *) p - *(unsigned char *) q;
	}
}

/* ------------------------------------------------------------------------ */
char *xxrealloc(char *old, int size)
{
	char           *p = (char *) xrealloc(char, old, size);
	if (!p)
		fatal_error(_T("Not enough memory"));
	return p;
}

/* ------------------------------------------------------------------------ */
/*								STRING POOL									*/
/* ------------------------------------------------------------------------ */
/*
  string pool :
	+-------------+-------------+------+-------------+----------+
	| N A M E 1 \0| N A M E 2 \0| .... | N A M E n \0|			|
	+-------------+-------------+------+-------------+----------+
	  ^ ^		 ^ buffer+0 buffer+used buffer+size

  vector :
	+---------------+---------------+------------- -----------------+
	| pointer to	| pointer to	| pointer to   ...  pointer to	|
	|  stringpool	|  N A M E 1	|  N A M E 2   ...   N A M E n	|
	+---------------+---------------+-------------     -------------+
	^ malloc base      returned
*/

/* ------------------------------------------------------------------------ */
void init_sp(struct string_pool *sp)
{
	sp->size = 1024 - 8;	/* any ( >=0 ) */
	sp->used = 0;
	sp->n = 0;
	sp->buffer = (char *) xmalloc(char, sp->size);
}

/* ------------------------------------------------------------------------ */
void add_sp(struct string_pool *sp, char *name, int len)
{
	while (sp->used + len > sp->size) {
		sp->size *= 2;
		sp->buffer = (char *) xxrealloc(sp->buffer, sp->size * sizeof(char));
	}
	bcopy(name, sp->buffer + sp->used, len);
	sp->used += len;
	sp->n++;
}

/* ------------------------------------------------------------------------ */
void finish_sp(struct string_pool *sp, int *v_count, char ***v_vector)
{
	int             i;
	register char  *p;
	char          **v;

	v = (char **) xmalloc(char*, sp->n + 1);
	*v++ = sp->buffer;
	*v_vector = v;
	*v_count = sp->n;
	p = sp->buffer;
	for (i = sp->n; i; i--) {
		*v++ = p;
		if (i - 1)
			p += strlen(p) + 1;
	}
}

/* ------------------------------------------------------------------------ */
void free_sp(char **vector)
{
	vector--;
	free(*vector);		/* free string pool */
	free(vector);
}


/* ------------------------------------------------------------------------ */
/*							READ DIRECTORY FILES							*/
/* ------------------------------------------------------------------------ */
static boolean include_path_p(char *path, char *name)
{
	char           *n = name;
	while (*path)
		if (*path++ != *n++)
			return (path[-1] == '/' && *n == '\0');
	return (*n == '/' || (n != name && path[-1] == '/' && n[-1] == '/'));
}
