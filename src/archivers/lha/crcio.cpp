/* ------------------------------------------------------------------------ */
/* LHa for UNIX    															*/
/*				crcio.c -- crc input / output								*/
/*																			*/
/*		Modified          		Nobutaka Watazaki							*/
/*																			*/
/*	Ver. 1.14 	Source All chagned				1995.01.14	N.Watazaki		*/
/* ------------------------------------------------------------------------ */
#include "lha.h"

/* ------------------------------------------------------------------------ */
static unsigned short crctable[UCHAR_MAX + 1];
static unsigned char subbitbuf, bitcount;
#ifdef EUC
static int      putc_euc_cache;
#endif
static int      getc_euc_cache;

/* ------------------------------------------------------------------------ */
void
make_crctable( /* void */ )
{
	unsigned int    i, j, r;

	for (i = 0; i <= UCHAR_MAX; i++) {
		r = i;
		for (j = 0; j < CHAR_BIT; j++)
			if (r & 1)
				r = (r >> 1) ^ CRCPOLY;
			else
				r >>= 1;
		crctable[i] = r;
	}
}

/* ------------------------------------------------------------------------ */
#ifdef NEED_INCREMENTAL_INDICATOR
static void
put_indicator(long int count)
{
	if (!quiet && indicator_threshold) {
		while (count > indicator_count) {
			putchar('o');
			fflush(stdout);
			indicator_count += indicator_threshold;
		}
	}
}
#endif

/* ------------------------------------------------------------------------ */
unsigned short
calccrc(unsigned char  *p, unsigned int n)
{
	reading_size += n;
#ifdef NEED_INCREMENTAL_INDICATOR
	put_indicator(reading_size);
#endif
	while (n-- > 0)
		UPDATE_CRC(*p++);
	return crc;
}

/* ------------------------------------------------------------------------ */
void
fillbuf(unsigned char n)			/* Shift bitbuf n bits left, read n bits */
{
	while (n > bitcount) {
		n -= bitcount;
		lhabitbuf = (lhabitbuf << bitcount) + (subbitbuf >> (CHAR_BIT - bitcount));
		if (compsize != 0) {
			compsize--;
			subbitbuf = (unsigned char) zfile_getc(infile);
		}
		else
			subbitbuf = 0;
		bitcount = CHAR_BIT;
	}
	bitcount -= n;
	lhabitbuf = (lhabitbuf << n) + (subbitbuf >> (CHAR_BIT - n));
	subbitbuf <<= n;
}

/* ------------------------------------------------------------------------ */
unsigned short
getbits(unsigned char n)
{
	unsigned short  x;

	x = lhabitbuf >> (2 * CHAR_BIT - n);
	fillbuf(n);
	return x;
}
#if 0
/* ------------------------------------------------------------------------ */
void
putcode(unsigned char n, unsigned short x)			/* Write rightmost n bits of x */
{
	while (n >= bitcount) {
		n -= bitcount;
		subbitbuf += x >> (USHRT_BIT - bitcount);
		x <<= bitcount;
		if (compsize < origsize) {
			if (fwrite(&subbitbuf, 1, 1, outfile) == 0) {
				/* fileerror(WTERR, outfile); */
			    fatal_error("Write error in crcio.c(putcode)\n");
				/* exit(errno); */
			}
			compsize++;
		}
		else
			unpackable = 1;
		subbitbuf = 0;
		bitcount = CHAR_BIT;
	}
	subbitbuf += x >> (USHRT_BIT - bitcount);
	bitcount -= n;
}

/* ------------------------------------------------------------------------ */
void
putbits(unsigned char n, unsigned short x)			/* Write rightmost n bits of x */
{
	x <<= USHRT_BIT - n;
	while (n >= bitcount) {
		n -= bitcount;
		subbitbuf += x >> (USHRT_BIT - bitcount);
		x <<= bitcount;
		if (compsize < origsize) {
			if (fwrite(&subbitbuf, 1, 1, outfile) == 0) {
				/* fileerror(WTERR, outfile); */
			    fatal_error("Write error in crcio.c(putbits)\n");
				/* exit(errno); */
			}
			compsize++;
		}
		else
			unpackable = 1;
		subbitbuf = 0;
		bitcount = CHAR_BIT;
	}
	subbitbuf += x >> (USHRT_BIT - bitcount);
	bitcount -= n;
}
#endif
/* ------------------------------------------------------------------------ */
int
fread_crc(unsigned char  *p, int n, struct zfile   *fp)
{
	n = zfile_fread(p, 1, n, fp);

	calccrc(p, n);
	return n;
}

/* ------------------------------------------------------------------------ */
void
fwrite_crc(unsigned char  *p, int n, struct zfile   *fp)
{
	calccrc(p, n);
	if (verify_mode)
		return;

	if (fp) {
	    zfile_fwrite(p, 1, n, fp);
	}
}

/* ------------------------------------------------------------------------ */
void
init_code_cache( /* void */ )
{				/* called from copyfile() in util.c */
#ifdef EUC
	putc_euc_cache = EOF;
#endif
	getc_euc_cache = EOF;
}

void
init_getbits( /* void */ )
{
	lhabitbuf = 0;
	subbitbuf = 0;
	bitcount = 0;
	fillbuf(2 * CHAR_BIT);
#ifdef EUC
	putc_euc_cache = EOF;
#endif
}

/* ------------------------------------------------------------------------ */
void
init_putbits( /* void */ )
{
	bitcount = CHAR_BIT;
	subbitbuf = 0;
	getc_euc_cache = EOF;
}

/* ------------------------------------------------------------------------ */
#ifdef EUC
void
putc_euc(int  c, FILE *fd)
{
	int             d;

	if (putc_euc_cache == EOF) {
		if (!euc_mode || c < 0x81 || c > 0xFC) {
			putc(c, fd);
			return;
		}
		if (c >= 0xA0 && c < 0xE0) {
			putc(0x8E, fd);	/* single shift */
			putc(c, fd);
			return;
		}
		putc_euc_cache = c;	/* save first byte */
		return;
	}
	d = putc_euc_cache;
	putc_euc_cache = EOF;
	if (d >= 0xA0)
		d -= 0xE0 - 0xA0;
	if (c > 0x9E) {
		c = c - 0x9F + 0x21;
		d = (d - 0x81) * 2 + 0x22;
	}
	else {
		if (c > 0x7E)
			c--;
		c -= 0x1F;
		d = (d - 0x81) * 2 + 0x21;
	}
	putc(0x80 | d, fd);
	putc(0x80 | c, fd);
}
#endif

/* ------------------------------------------------------------------------ */
int
fwrite_txt(unsigned char  *p, int n, FILE *fp)
{
	while (--n >= 0) {
		if (*p != '\015' && *p != '\032') {
#ifdef EUC
			putc_euc(*p, fp);
#else
			putc(*p, fp);
#endif
		}

		prev_char = *p++;
	}
	return (ferror(fp));
}

/* ------------------------------------------------------------------------ */
int
fread_txt(unsigned char  *p, int n, FILE *fp)
{
	int             c;
	int             cnt = 0;

	while (cnt < n) {
		if (getc_euc_cache != EOF) {
			c = getc_euc_cache;
			getc_euc_cache = EOF;
		}
		else {
			if ((c = fgetc(fp)) == EOF)
				break;
			if (c == '\n') {
				getc_euc_cache = c;
				++origsize;
				c = '\r';
			}
#ifdef EUC
			else if (euc_mode && (c == 0x8E || 0xA0 < c && c < 0xFF)) {
				int             d = fgetc(fp);
				if (d == EOF) {
					*p++ = c;
					cnt++;
					break;
				}
				if (c == 0x8E) {	/* single shift (KANA) */
					if ((0x20 < d && d < 0x7F) || (0xA0 < d && d < 0xFF))
						c = d | 0x80;
					else
						getc_euc_cache = d;
				}
				else {
					if (0xA0 < d && d < 0xFF) {	/* if GR */
						c &= 0x7F;	/* convert to MS-kanji */
						d &= 0x7F;
						if (!(c & 1)) {
							c--;
							d += 0x7F - 0x21;
						}
						if ((d += 0x40 - 0x21) > 0x7E)
							d++;
						if ((c = (c >> 1) + 0x71) >= 0xA0)
							c += 0xE0 - 0xA0;
					}
					getc_euc_cache = d;
				}
			}
#endif
		}
		*p++ = c;
		cnt++;
	}
	return cnt;
}

/* ------------------------------------------------------------------------ */
unsigned short
calc_header_crc(unsigned char  *p, unsigned int n)		/* Thanks T.Okamoto */
{
	crc = 0;
	while (n-- > 0)
		UPDATE_CRC(*p++);
	return crc;
}
