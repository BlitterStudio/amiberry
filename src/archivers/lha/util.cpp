/* ------------------------------------------------------------------------ */
/* LHa for UNIX    															*/
/*				util.c -- LHarc Util										*/
/*																			*/
/*		Modified          		Nobutaka Watazaki							*/
/*																			*/
/*	Ver. 1.14 	Source All chagned				1995.01.14	N.Watazaki		*/
/*  Ver. 1.14e  Support for sfx archives		1999.05.28	T.Okamoto       */
/* ------------------------------------------------------------------------ */
#include "lha.h"
/*
 * util.c - part of LHa for UNIX Feb 26 1992 modified by Masaru Oki Mar  4
 * 1992 modified by Masaru Oki #ifndef USESTRCASECMP added. Mar 31 1992
 * modified by Masaru Oki #ifdef NOMEMSET added.
 */

/* ------------------------------------------------------------------------ */
extern unsigned short crc;

/* ------------------------------------------------------------------------ */
/*	convert path delimit
	erreturns *filename														*/
/* ------------------------------------------------------------------------ */
unsigned char *convdelim(unsigned char *path, unsigned char delim)
{
	unsigned char   c;
	unsigned char  *p;
#ifdef MULTIBYTE_CHAR
	int             kflg;

	kflg = 0;
#endif
	for (p = path; (c = *p) != 0; p++) {
#ifdef MULTIBYTE_CHAR
		if (kflg) {
			kflg = 0;
		}
		else if (MULTIBYTE_FIRST_P(c)) {
			kflg = 1;
		}
		else
#endif
		if (c == '\\' || c == DELIM || c == DELIM2) {
			*p = delim;
			path = p + 1;
		}
	}
	return path;
}


/*
 * strdup(3)
 */

/* ------------------------------------------------------------------------ */
#ifdef NOSTRDUP
char           *
strdup(buf)
	char           *buf;
{
	char           *p;

	if ((p = (char *) malloc(strlen(buf) + 1)) == NULL)
		return NULL;
	strcpy(p, buf);
	return p;
}
#endif

/*
 * memmove( char *dst , char *src , size_t cnt )
 */

/* ------------------------------------------------------------------------ */
#if 0 && defined(NOBSTRING) && !defined(__STDC__)
void           *
memmove(dst, src, cnt)
	register char  *dst, *src;
	register int    cnt;
{
	if (dst == src)
		return dst;
	if (src > dst) {
		while (--cnt >= 0)
			*dst++ = *src++;
	}
	else {
		dst += cnt;
		src += cnt;
		while (--cnt >= 0)
			*--dst = *--src;
	}
	return dst;
}
#endif

/*
 * rename - change the name of file 91.11.02 by Tomohiro Ishikawa
 * (ishikawa@gaia.cow.melco.CO.JP) 92.01.20 little modified (added #ifdef) by
 * Masaru Oki 92.01.28 added mkdir() and rmdir() by Tomohiro Ishikawa
 */

#if defined(NOFTRUNCATE) && !defined(_MINIX)

/* ------------------------------------------------------------------------ */
int
rename(from, to)
	char           *from, *to;
{
	struct stat     s1, s2;
	extern int      errno;

	if (stat(from, &s1) < 0)
		return (-1);
	/* is 'FROM' file a directory? */
	if ((s1.st_mode & S_IFMT) == S_IFDIR) {
		errno = ENOTDIR;
		return (-1);
	}
	if (stat(to, &s2) >= 0) {	/* 'TO' exists! */
		/* is 'TO' file a directory? */
		if ((s2.st_mode & S_IFMT) == S_IFDIR) {
			errno = EISDIR;
			return (-1);
		}
		if (unlink(to) < 0)
			return (-1);
	}
	if (link(from, to) < 0)
		return (-1);
	if (unlink(from) < 0)
		return (-1);
	return (0);
}
#endif				/* NOFTRUNCATE */
/* ------------------------------------------------------------------------ */

#ifdef	NOMKDIR
#ifndef	MKDIRPATH
#define	MKDIRPATH	"/bin/mkdir"
#endif
#ifndef	RMDIRPATH
#define	RMDIRPATH	"/bin/rmdir"
#endif
int
rmdir(path)
	char           *path;
{
	int             stat, rtn = 0;
	char           *cmdname;
	if ((cmdname = (char *) malloc(strlen(RMDIRPATH) + 1 + strlen(path) + 1))
	    == 0)
		return (-1);
	strcpy(cmdname, RMDIRPATH);
	*(cmdname + strlen(RMDIRPATH)) = ' ';
	strcpy(cmdname + strlen(RMDIRPATH) + 1, path);
	if ((stat = system(cmdname)) < 0)
		rtn = -1;	/* fork or exec error */
	else if (stat) {	/* RMDIR command error */
		errno = EIO;
		rtn = -1;
	}
	free(cmdname);
	return (rtn);
}

/* ------------------------------------------------------------------------ */
int
mkdir(path, mode)
	char           *path;
	int             mode;
{
	int             child, stat;
	char           *cmdname, *cmdpath = MKDIRPATH;
	if ((cmdname = (char *) strrchr(cmdpath, '/')) == (char *) 0)
		cmdname = cmdpath;
	if ((child = fork()) < 0)
		return (-1);	/* fork error */
	else if (child) {	/* parent process */
		while (child != wait(&stat))	/* ignore signals */
			continue;
	}
	else {			/* child process */
		int             maskvalue;
		maskvalue = umask(0);	/* get current umask() value */
		umask(maskvalue | (0777 & ~mode));	/* set it! */
		execl(cmdpath, cmdname, path, (char *) 0);
		/* never come here except execl is error */
		return (-1);
	}
	if (stat != 0) {
		errno = EIO;	/* cannot get error num. */
		return (-1);
	}
	return (0);
}
#endif

/*
 * strucmp modified: Oct 29 1991 by Masaru Oki
 */

#ifndef USESTRCASECMP
static int my_toupper(int n)
{
	if (n >= 'a' && n <= 'z')
		return n & (~('a' - 'A'));
	return n;
}

/* ------------------------------------------------------------------------ */
int strucmp(char *s, char *t)
{
	while (my_toupper(*s++) == my_toupper(*t++))
		if (!*s || !*t)
			break;
	if (!*s && !*t)
		return 0;
	return 1;
}
#endif

/* ------------------------------------------------------------------------ */
#ifdef NOMEMSET
/* Public Domain memset(3) */
char           *
memset(s, c, n)
	char           *s;
	int             c, n;
{
	char           *p = s;
	while (n--)
		*p++ = (char) c;
	return s;
}
#endif

/* Local Variables: */
/* mode:c */
/* tab-width:4 */
/* compile-command:"gcc -c util.c" */
/* End: */
