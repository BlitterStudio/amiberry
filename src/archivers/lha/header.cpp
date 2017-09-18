/* ------------------------------------------------------------------------ */
/* LHa for UNIX    															*/
/*				header.c -- header manipulate functions						*/
/*																			*/
/*		Modified          		Nobutaka Watazaki							*/
/*																			*/
/*	Original												Y.Tagawa		*/
/*	modified									1991.12.16	M.Oki			*/
/*	Ver. 1.10  Symbolic Link added				1993.10.01	N.Watazaki		*/
/*	Ver. 1.13b Symbolic Link Bug Fix			1994.08.22	N.Watazaki		*/
/*	Ver. 1.14  Source All chagned				1995.01.14	N.Watazaki		*/
/*  Ver. 1.14i bug fixed						2000.10.06  t.okamoto       */
/* ------------------------------------------------------------------------ */
#include "lha.h"

/* ------------------------------------------------------------------------ */
static char    *get_ptr;
/* ------------------------------------------------------------------------ */
int calc_sum(char *p, int len)
{
	register int    sum;

	for (sum = 0; len; len--)
		sum += *p++;

	return sum & 0xff;
}

/* ------------------------------------------------------------------------ */
static unsigned short get_word (void)
{
	int             b0, b1;

	b0 = get_byte ();
	b1 = get_byte ();
	return (b1 << 8) + b0;
}

/* ------------------------------------------------------------------------ */
static void put_word (unsigned int v)
{
	put_byte (v);
	put_byte (v >> 8);
}

/* ------------------------------------------------------------------------ */
static long get_longword(void)
{
	long            b0, b1, b2, b3;

	b0 = get_byte ();
	b1 = get_byte ();
	b2 = get_byte ();
	b3 = get_byte ();
	return (b3 << 24) + (b2 << 16) + (b1 << 8) + b0;
}

/* ------------------------------------------------------------------------ */
static void put_longword(long v)
{
	put_byte (v);
	put_byte (v >> 8);
	put_byte (v >> 16);
	put_byte (v >> 24);
}

/* ------------------------------------------------------------------------ */
static void msdos_to_unix_filename(char *name, int len)
{
	register int    i;

#ifdef MULTIBYTE_CHAR
	for (i = 0; i < len; i++) {
		if (MULTIBYTE_FIRST_P(name[i]) &&
		    MULTIBYTE_SECOND_P(name[i + 1]))
			i++;
		else if (name[i] == '\\')
			name[i] = '/';
		else if (!noconvertcase && isupper(name[i]))
			name[i] = tolower(name[i]);
	}
#else
	for (i = 0; i < len; i++) {
		if (name[i] == '\\')
			name[i] = '/';
		else if (!noconvertcase && isupper(name[i]))
			name[i] = tolower(name[i]);
	}
#endif
}

/* ------------------------------------------------------------------------ */
static void generic_to_unix_filename(char *name, int len)
{
	register int    i;
	boolean         lower_case_used = FALSE;

#ifdef MULTIBYTE_CHAR
	for (i = 0; i < len; i++) {
		if (MULTIBYTE_FIRST_P(name[i]) &&
		    MULTIBYTE_SECOND_P(name[i + 1]))
			i++;
		else if (islower(name[i])) {
			lower_case_used = TRUE;
			break;
		}
	}
	for (i = 0; i < len; i++) {
		if (MULTIBYTE_FIRST_P(name[i]) &&
		    MULTIBYTE_SECOND_P(name[i + 1]))
			i++;
		else if (name[i] == '\\')
			name[i] = '/';
		else if (!noconvertcase && !lower_case_used && isupper(name[i]))
			name[i] = tolower(name[i]);
	}
#else
	for (i = 0; i < len; i++)
		if (islower(name[i])) {
			lower_case_used = TRUE;
			break;
		}
	for (i = 0; i < len; i++) {
		if (name[i] == '\\')
			name[i] = '/';
		else if (!noconvertcase && !lower_case_used && isupper(name[i]))
			name[i] = tolower(name[i]);
	}
#endif
}

/* ------------------------------------------------------------------------ */
static void
macos_to_unix_filename(char *name, int len)
{
	register int    i;

	for (i = 0; i < len; i++) {
		if (name[i] == ':')
			name[i] = '/';
		else if (name[i] == '/')
			name[i] = ':';
	}
}

/* ------------------------------------------------------------------------ */
static void
unix_to_generic_filename(char *name, int len)
{
	register int    i;

	for (i = 0; i < len; i++) {
		if (name[i] == '/')
			name[i] = '\\';
		else if (islower(name[i]))
			name[i] = toupper(name[i]);
	}
}

/* ------------------------------------------------------------------------ */
/*																			*/
/* Generic stamp format:						 							*/
/*																			*/
/* 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16							*/
/* |<-------- year ------->|<- month ->|<-- day -->|						*/
/*																			*/
/* 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0							*/
/* |<--- hour --->|<---- minute --->|<- second*2 ->|						*/
/*																			*/
/* ------------------------------------------------------------------------ */

/*
 * NOTE : If you don't have `gettimeofday(2)', or your gettimeofday(2)
 * returns bogus timezone information, try FTIME, MKTIME, TIMELOCAL or TZSET.
 */

/* choose one */
#if defined(MKTIME)
#ifdef TIMELOCAL
#undef TIMELOCAL
#endif
#endif				/* defined(MKTIME) */

#if defined(MKTIME) || defined(TIMELOCAL)
#ifdef TZSET
#undef TZSET
#endif
#endif				/* defined(MKTIME) || defined(TIMELOCAL) */

#if defined(MKTIME) || defined(TIMELOCAL) || defined(TZSET)
#ifdef FTIME
#undef FTIME
#endif
#endif

#if defined(MKTIME) || defined(TIMELOCAL) || defined(TZSET) || defined(FTIME)
#ifdef GETTIMEOFDAY
#undef GETTIMEOFDAY
#endif
#else
#ifndef GETTIMEOFDAY
#define GETTIMEOFDAY		/* use gettimeofday() */
#endif
#endif

#ifdef FTIME
#include <sys/timeb.h>
#endif

/*
 * You may define as : #define TIMEZONE_HOOK		\ extern long
 * timezone ;	\ extern void tzset();
 */
#ifdef TIMEZONE_HOOK
TIMEZONE_HOOK
/* Which do you like better, `TIMEZONE_HOOK' or `TIMEZONE_HOOK;' ? */
#endif

#if defined(TZSET) && defined(_MINIX)
extern long     timezone;		/* not defined in time.h */
#endif

/* ------------------------------------------------------------------------ */
#if defined(FTIME) || defined(GETTIMEOFDAY) || defined(TZSET)
static long
gettz()
#ifdef TZSET
{
	tzset();
	return timezone;
}
#endif

/* ------------------------------------------------------------------------ */
#if !defined(TZSET) && defined(FTIME)
{
	struct timeb    buf;

	ftime(&buf);
	return buf.timezone * 60L;
}
#endif

/* ------------------------------------------------------------------------ */
#if !defined(TZSET) && !defined(FTIME)	/* maybe defined(GETTIMEOFDAY) */
{
#ifdef HAVE_TM_ZONE
	time_t tt;

	time(&tt);
	return -localtime(&tt)->tm_gmtoff;
#else /* HAVE_TM_ZONE */
	struct timeval  tp;
	struct timezone tzp;
	gettimeofday(&tp, &tzp);/* specific to 4.3BSD */
	/*
	 * return (tzp.tz_minuteswest * 60L + (tzp.tz_dsttime != 0 ? 60L *
	 * 60L : 0));
	 */
	return (tzp.tz_minuteswest * 60L);
#endif /* HAVE_TM_ZONE */
}
#endif
#endif				/* defined(FTIME) || defined(GETTIMEOFDAY) ||
				 * defined(TZSET) */

/* ------------------------------------------------------------------------ */
#ifdef NOT_USED
static struct tm *
msdos_to_unix_stamp_tm(a)
	long            a;
{
	static struct tm t;

	t.tm_sec = (a & 0x1f) * 2;
	t.tm_min = (a >> 5) & 0x3f;
	t.tm_hour = (a >> 11) & 0x1f;
	t.tm_mday = (a >> 16) & 0x1f;
	t.tm_mon = ((a >> 16 + 5) & 0x0f) - 1;
	t.tm_year = ((a >> 16 + 9) & 0x7f) + 80;
	return &t;
}
#endif

/* ------------------------------------------------------------------------ */
static time_t generic_to_unix_stamp(long t)
#if defined(MKTIME) || defined(TIMELOCAL)
{
	struct tm       dostm;

	/*
	 * special case:  if MSDOS format date and time were zero, then we
	 * set time to be zero here too.
	 */
	if (t == 0)
		return (time_t) 0;

	dostm.tm_sec = (t & 0x1f) * 2;
	dostm.tm_min = t >> 5 & 0x3f;
	dostm.tm_hour = t >> 11 & 0x1f;
	dostm.tm_mday = t >> 16 & 0x1f;
	dostm.tm_mon = ((t >> 16) + 5 & 0x0f) - 1;	/* 0..11 */
	dostm.tm_year = ((t >> 16) + 9 & 0x7f) + 80;
#if 0
	dostm.tm_isdst = 0;	/* correct? */
#endif
	dostm.tm_isdst = -1;    /* correct? */
#ifdef MKTIME
	return (time_t) mktime(&dostm);
#else				/* maybe defined(TIMELOCAL) */
	return (time_t) timelocal(&dostm);
#endif
}

#else				/* defined(MKTIME) || defined(TIMELOCAL) */
{
	int             year, month, day, hour, min, sec;
	long            longtime;
	static unsigned int dsboy[12] = {0, 31, 59, 90, 120, 151,
	181, 212, 243, 273, 304, 334};
	unsigned int    days;

	/*
	 * special case:  if MSDOS format date and time were zero, then we
	 * set time to be zero here too.
	 */
	if (t == 0)
		return (time_t) 0;

	year = ((int) ((t >> 16) + 9) & 0x7f) + 1980;
	month = (int) ((t >> 16) + 5) & 0x0f;	/* 1..12 means Jan..Dec */
	day = (int) (t >> 16) & 0x1f;	/* 1..31 means 1st,...31st */

	hour = ((int) t >> 11) & 0x1f;
	min = ((int) t >> 5) & 0x3f;
	sec = ((int) t & 0x1f) * 2;

	/* Calculate days since 1970.01.01 */
	days = (365 * (year - 1970) +	/* days due to whole years */
		(year - 1970 + 1) / 4 +	/* days due to leap years */
		dsboy[month - 1] +	/* days since beginning of this year */
		day - 1);	/* days since beginning of month */

	if ((year % 4 == 0) &&
		(year % 100 != 0 || year % 400 == 0) &&		/* 1999.5.24 t.oka */
	    (month >= 3))	/* if this is a leap year and month */
		days++;		/* is March or later, add a day */

	/* Knowing the days, we can find seconds */
	longtime = (((days * 24) + hour) * 60 + min) * 60 + sec;
	longtime += gettz();	/* adjust for timezone */

	/* LONGTIME is now the time in seconds, since 1970/01/01 00:00:00.  */
	return (time_t) longtime;
}
#endif				/* defined(MKTIME) || defined(TIMELOCAL) */

/* ------------------------------------------------------------------------ */
static long unix_to_generic_stamp(time_t t)
{
	struct tm      *tm = localtime(&t);

	return ((((long) (tm->tm_year - 80)) << 25) +
		(((long) (tm->tm_mon + 1)) << 21) +
		(((long) tm->tm_mday) << 16) +
		(long) ((tm->tm_hour << 11) +
			(tm->tm_min << 5) +
			(tm->tm_sec / 2)));
}

/* ------------------------------------------------------------------------ */
/* build header functions													*/
/* ------------------------------------------------------------------------ */
boolean get_header(struct zfile *fp, LzHeader *hdr)
{
	int             header_size;
	int             name_length;
	char            data[LZHEADER_STRAGE];
	char            dirname[FILENAME_LENGTH];
	int             dir_length = 0;
	int             checksum;
	int             i;
	char           *ptr;
	int				extend_size;
	int				dmy;

	bzero(hdr, sizeof(LzHeader));

	if (((header_size = zfile_getc(fp)) == EOF) || (header_size == 0)) {
		return FALSE;	/* finish */
	}

	if (zfile_fread(data + I_HEADER_CHECKSUM,
		  sizeof(char), header_size - 1, fp) < header_size - 1) {
		fatal_error(_T("Invalid header (LHarc file ?)"));
		return FALSE;	/* finish */
	}
	setup_get(data + I_HEADER_LEVEL);
	hdr->header_level = get_byte ();
	if (hdr->header_level != 2 &&
	    zfile_fread(data + header_size, sizeof(char), 2, fp) < 2) {
		fatal_error(_T("Invalid header (LHarc file ?)"));
		return FALSE;	/* finish */
	}

	if (hdr->header_level >= 3) {
		fatal_error(_T("Unknown level header"));
		return FALSE;
	}

	setup_get(data + I_HEADER_CHECKSUM);
	checksum = get_byte ();

	if (hdr->header_level == 2) {
		hdr->header_size = header_size + checksum*256;
	} else {
		hdr->header_size = header_size;
	}
	bcopy(data + I_METHOD, hdr->method, METHOD_TYPE_STRAGE);
	setup_get(data + I_PACKED_SIZE);
	hdr->packed_size = get_longword();
	hdr->original_size = get_longword();
	hdr->last_modified_stamp = get_longword();
	hdr->attribute = get_byte ();

	if ((hdr->header_level = get_byte ()) != 2) {
		if (calc_sum(data + I_METHOD, header_size) != checksum)
			warning(_T("Checksum error (LHarc file?)"), _T(""));
		name_length = get_byte ();
		for (i = 0; i < name_length; i++)
			hdr->name[i] = (char) get_byte ();
		hdr->name[name_length] = '\0';
	}
	else {
		hdr->unix_last_modified_stamp = hdr->last_modified_stamp;
		name_length = 0;
	}

	/* defaults for other type */
	hdr->unix_mode = UNIX_FILE_REGULAR | UNIX_RW_RW_RW;
	hdr->unix_gid = 0;
	hdr->unix_uid = 0;

	if (hdr->header_level == 0) {
		extend_size = header_size - name_length -22;
		if (extend_size < 0) {
			if (extend_size == -2) {
				hdr->extend_type = EXTEND_GENERIC;
				hdr->has_crc = FALSE;
			} else {
				fatal_error(_T("Unkonwn header (lha file?)"));
				return FALSE;
			}
		} else {
			hdr->has_crc = TRUE;
			hdr->crc = get_word ();
		}

		if (extend_size >= 1) {
			hdr->extend_type = get_byte ();
			extend_size--;
		}
		if (hdr->extend_type == EXTEND_UNIX || hdr->extend_type == EXTEND_AMIGAOS) {
			if (extend_size >= 11) {
				hdr->minor_version = get_byte ();
				hdr->unix_last_modified_stamp = (time_t) get_longword();
				hdr->unix_mode = get_word ();
				hdr->unix_uid = get_word ();
				hdr->unix_gid = get_word ();
				extend_size -= 11;
			} else {
				hdr->extend_type = EXTEND_GENERIC;
			}
		}
		while (extend_size-- > 0)
			dmy = get_byte ();
		if (hdr->extend_type == EXTEND_UNIX)
			return TRUE;
	} else if (hdr->header_level == 1) {
		hdr->has_crc = TRUE;
		extend_size = header_size - name_length-25;
		hdr->crc = get_word ();
		hdr->extend_type = get_byte ();
		while (extend_size-- > 0)
			dmy = get_byte ();
	} else { /* level 2 */
		hdr->has_crc = TRUE;
		hdr->crc = get_word ();
		hdr->extend_type = get_byte ();
	}

	if (hdr->header_level > 0) {
		/* Extend Header */
		if (hdr->header_level != 2)
			setup_get(data + hdr->header_size);
		ptr = get_ptr;
		while ((header_size = get_word ()) != 0) {
			if (hdr->header_level != 2 &&
			((data + LZHEADER_STRAGE - get_ptr < header_size) ||
			 zfile_fread(get_ptr, sizeof(char), header_size, fp) < header_size)) {
				fatal_error(_T("Invalid header (LHa file ?)"));
				return FALSE;
			}
			switch (get_byte ()) {
			case 0:
				/*
				 * header crc
				 */
				setup_get(get_ptr + header_size - 3);
				break;
			case 1:
				/*
				 * filename
				 */
				if (header_size >= 256)
				    return FALSE;
				for (i = 0; i < header_size - 3; i++)
					hdr->name[i] = (char) get_byte ();
				hdr->name[header_size - 3] = '\0';
				name_length = header_size - 3;
				break;
			case 2:
				/*
				 * directory
				 */
				if (header_size >= FILENAME_LENGTH)
				    return FALSE;
				for (i = 0; i < header_size - 3; i++)
					dirname[i] = (char) get_byte ();
				dirname[header_size - 3] = '\0';
				convdelim((unsigned char*)dirname, DELIM);
				dir_length = header_size - 3;
				break;
			case 0x40:
				/*
				 * MS-DOS attribute
				 */
				if (hdr->extend_type == EXTEND_MSDOS ||
				    hdr->extend_type == EXTEND_HUMAN ||
				    hdr->extend_type == EXTEND_AMIGAOS ||
				    hdr->extend_type == EXTEND_GENERIC)
					hdr->attribute = get_word ();
				break;
			case 0x50:
				/*
				 * UNIX permission
				 */
				if (hdr->extend_type == EXTEND_UNIX)
					hdr->unix_mode = get_word ();
				break;
			case 0x51:
				/*
				 * UNIX gid and uid
				 */
				if (hdr->extend_type == EXTEND_UNIX) {
					hdr->unix_gid = get_word ();
					hdr->unix_uid = get_word ();
				}
				break;
			case 0x52:
				/*
				 * UNIX group name
				 */
				setup_get(get_ptr + header_size - 3);
				break;
			case 0x53:
				/*
				 * UNIX user name
				 */
				setup_get(get_ptr + header_size - 3);
				break;
			case 0x54:
				/*
				 * UNIX last modified time
				 */
				if (hdr->extend_type == EXTEND_UNIX || hdr->extend_type == EXTEND_AMIGAOS)
					hdr->unix_last_modified_stamp = (time_t) get_longword();
				break;
			default:
				/*
				 * other headers
				 */
				setup_get(get_ptr + header_size - 3);
				break;
			}
		}
		if (hdr->header_level != 2 && get_ptr - ptr != 2) {
			hdr->packed_size -= get_ptr - ptr - 2;
			hdr->header_size += get_ptr - ptr - 2;
		}
	}

	switch (hdr->extend_type) {
	case EXTEND_MSDOS:
		msdos_to_unix_filename(hdr->name, name_length);
		msdos_to_unix_filename(dirname, dir_length);
	case EXTEND_HUMAN:
		if (hdr->header_level == 2)
			hdr->unix_last_modified_stamp = hdr->last_modified_stamp;
		else
			hdr->unix_last_modified_stamp =
				generic_to_unix_stamp(hdr->last_modified_stamp);
		break;

#ifdef OSK
	case EXTEND_OS68K:
	case EXTEND_XOSK:
#endif
	case EXTEND_AMIGAOS:
	case EXTEND_UNIX:
		break;

	case EXTEND_MACOS:
		macos_to_unix_filename(hdr->name, name_length);
		/* macos_to_unix_filename(dirname, dir_length); */
		hdr->unix_last_modified_stamp =
			generic_to_unix_stamp(hdr->last_modified_stamp);
		break;

	default:
		generic_to_unix_filename(hdr->name, name_length);
		generic_to_unix_filename(dirname, dir_length);
		if (hdr->header_level == 2)
			hdr->unix_last_modified_stamp = hdr->last_modified_stamp;
		else
			hdr->unix_last_modified_stamp =
				generic_to_unix_stamp(hdr->last_modified_stamp);
	}

	if (dir_length) {
		strcat(dirname, hdr->name);
		strcpy(hdr->name, dirname);
		name_length += dir_length;
	}

	return TRUE;
}

/* ------------------------------------------------------------------------ */
void init_header(char *name, struct stat *v_stat, LzHeader *hdr)
{
	int             len;

	if (compress_method == LZHUFF5_METHOD_NUM)  /* Changed N.Watazaki */
		bcopy(LZHUFF5_METHOD, hdr->method, METHOD_TYPE_STRAGE);
	else if (compress_method)
		bcopy(LZHUFF1_METHOD, hdr->method, METHOD_TYPE_STRAGE);
	else
		bcopy(LZHUFF0_METHOD, hdr->method, METHOD_TYPE_STRAGE);

	hdr->packed_size = 0;
	hdr->original_size = v_stat->st_size;
	hdr->last_modified_stamp = unix_to_generic_stamp(v_stat->st_mtime);
	hdr->attribute = GENERIC_ATTRIBUTE;
	hdr->header_level = header_level;
	strcpy(hdr->name, name);
	len = strlen(name);
	hdr->crc = 0x0000;
	hdr->extend_type = EXTEND_UNIX;
	hdr->unix_last_modified_stamp = v_stat->st_mtime;
	/* since 00:00:00 JAN.1.1970 */
#ifdef NOT_COMPATIBLE_MODE
	/* Please need your modification in this space. */
#else
	hdr->unix_mode = v_stat->st_mode;
#endif

	hdr->unix_uid = v_stat->st_uid;
	hdr->unix_gid = v_stat->st_gid;

	if (is_directory(v_stat)) {
		bcopy(LZHDIRS_METHOD, hdr->method, METHOD_TYPE_STRAGE);
		hdr->attribute = GENERIC_DIRECTORY_ATTRIBUTE;
		hdr->original_size = 0;
		if (len > 0 && hdr->name[len - 1] != '/')
			strcpy(&hdr->name[len++], "/");
	}

#ifdef S_IFLNK
	if (is_symlink(v_stat)) {
		char	lkname[257];
		int		len;
		bcopy(LZHDIRS_METHOD, hdr->method, METHOD_TYPE_STRAGE);
		hdr->attribute = GENERIC_DIRECTORY_ATTRIBUTE;
		hdr->original_size = 0;
		len = readlink(name, lkname, 256);
		lkname[len] = (char)'\0';
		sprintf(hdr->name, "%s|%s", hdr->name, lkname);
	}
#endif
	if (generic_format)
		unix_to_generic_filename(hdr->name, len);
}
