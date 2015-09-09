/* ------------------------------------------------------------------------ */
/* LHa for UNIX    															*/
/*				slice.c -- sliding dictionary with percolating update		*/
/*																			*/
/*		Modified          		Nobutaka Watazaki							*/
/*																			*/
/*	Ver. 1.14d 	Exchanging a search algorithm  1997.01.11    T.Okamoto      */
/* ------------------------------------------------------------------------ */

#if 0
#define DEBUG 1
#endif

#include "lha.h"


#ifdef DEBUG
FILE *fout = NULL;
static int noslide = 1;
#endif

/* ------------------------------------------------------------------------ */

static unsigned long encoded_origsize;

/* ------------------------------------------------------------------------ */

static unsigned int *hash;
static unsigned int *prev;

/* static unsigned char  *lha_text; */
unsigned char *too_flag;

#if 0
static struct encode_option encode_define[2] = {
#if 1 || defined(__STDC__) || defined(AIX)
	/* lh1 */
	{(void (*) ()) output_dyn,
		(void (*) ()) encode_start_fix,
	(void (*) ()) encode_end_dyn},
	/* lh4, 5,6 */
	{(void (*) ()) output_st1,
		(void (*) ()) encode_start_st1,
	(void (*) ()) encode_end_st1}
#else
	/* lh1 */
	{(int (*) ()) output_dyn,
		(int (*) ()) encode_start_fix,
	(int (*) ()) encode_end_dyn},
	/* lh4, 5,6 */
	{(int (*) ()) output_st1,
		(int (*) ()) encode_start_st1,
	(int (*) ()) encode_end_st1}
#endif
};
#endif

static struct decode_option decode_define[] = {
	/* lh1 */
	{decode_c_dyn, decode_p_st0, decode_start_fix},
	/* lh2 */
	{decode_c_dyn, decode_p_dyn, decode_start_dyn},
	/* lh3 */
	{decode_c_st0, decode_p_st0, decode_start_st0},
	/* lh4 */
	{decode_c_st1, decode_p_st1, decode_start_st1},
	/* lh5 */
	{decode_c_st1, decode_p_st1, decode_start_st1},
	/* lh6 */
	{decode_c_st1, decode_p_st1, decode_start_st1},
	/* lh7 */
	{decode_c_st1, decode_p_st1, decode_start_st1},
	/* lzs */
	{decode_c_lzs, decode_p_lzs, decode_start_lzs},
	/* lz5 */
	{decode_c_lz5, decode_p_lz5, decode_start_lz5}
};

static struct encode_option encode_set;
static struct decode_option decode_set;

#if 0
static node     pos, matchpos, avail, *position, *parent, *prev;
static int      remainder, matchlen;
static unsigned char *level, *childcount;
static unsigned long dicsiz;  /* t.okamoto */
static unsigned short max_hash_val;
static unsigned short hash1, hash2;
#endif

#ifdef SUPPORT_LH7
#define DICSIZ (1L << 16)
#define TXTSIZ (DICSIZ * 2L + MAXMATCH)
#else
#define DICSIZ (((unsigned long)1) << 15)
#define TXTSIZ (DICSIZ * 2 + MAXMATCH)
#endif

#define HSHSIZ (((unsigned long)1) <<15)
#define NIL 0
#define LIMIT 0x100	/* chain 長の limit */

static unsigned int txtsiz;

static unsigned long dicsiz;

static unsigned int hval;
static int matchlen;
static unsigned int matchpos;
static unsigned int pos;
static unsigned int remainder;

#if 0
/* ------------------------------------------------------------------------ */
int
encode_alloc(int method)
{
	if (method == LZHUFF1_METHOD_NUM) {	/* Changed N.Watazaki */
		encode_set = encode_define[0];
		maxmatch = 60;
		dicbit = 12;   /* 12 Changed N.Watazaki */
	} else { /* method LH4(12),LH5(13),LH6(15) */
		encode_set = encode_define[1];
		maxmatch = MAXMATCH;
		if (method == LZHUFF7_METHOD_NUM)
			dicbit = MAX_DICBIT; /* 16 bits */
		else if (method == LZHUFF6_METHOD_NUM)
			dicbit = MAX_DICBIT-1;		/* 15 bits */
		else /* LH5  LH4 is not used */
			dicbit = MAX_DICBIT - 3;	/* 13 bits */
	}

	dicsiz = (((unsigned long)1) << dicbit);
	txtsiz = dicsiz*2+maxmatch;

	if (hash) return method;

	if (alloc_buf() == NULL) exit(207); /* I don't know this 207. */

	hash = (unsigned int*)malloc(HSHSIZ * sizeof(unsigned int));
	prev = (unsigned int*)malloc(DICSIZ * sizeof(unsigned int));
	lha_text = (unsigned char*)malloc(TXTSIZ);
	too_flag = (unsigned char*)malloc(HSHSIZ);

	if (hash == NULL || prev == NULL || lha_text == NULL || too_flag == NULL)
		exit(207);

	return method;
}

/* ------------------------------------------------------------------------ */
/* ポインタの初期化 */

static void init_slide()
{
	unsigned int i;

	for (i = 0; i < HSHSIZ; i++) {
		hash[i] = NIL;
		too_flag[i] = 0;
	}
	/*
	for (i = 0; i < DICSIZ; i++) {
	    prev[i] = NIL;
	}
	*/
}

/* 辞書を DICSIZ 分 前にずらす */

static void update()
{
	unsigned int i, j;
	unsigned int k;
	long n;

#if 0
	memmove(&lha_text[0], &text[dicsiz], (unsigned)(txtsiz - dicsiz));
#else
	{
		int m;
		i = 0; j = dicsiz; m = txtsiz-dicsiz;
		while (m-- > 0) {
			lha_text[i++] = lha_text[j++];
		}
	}
#endif
	n = fread_crc(&lha_text[(unsigned)(txtsiz - dicsiz)], 
	                           (unsigned)dicsiz, infile);

	remainder += n;
	encoded_origsize += n;

	pos -= dicsiz;
	for (i = 0; i < HSHSIZ; i++) {
		j = hash[i];
		hash[i] = (j > dicsiz) ? j - dicsiz : NIL;
		too_flag[i] = 0;
	}
	for (i = 0; i < dicsiz; i++) {
		j = prev[i];
		prev[i] = (j > dicsiz) ? j - dicsiz : NIL;
	}
}


/* 現在の文字列をチェーンに追加する */

static void insert()
{
	prev[pos & (dicsiz - 1)] = hash[hval];
	hash[hval] = pos;
}


/* 現在の文字列と最長一致する文字列を検索し、チェーンに追加する */

static void match_insert()
{
	unsigned int scan_pos, scan_end, len;
	unsigned char *a, *b;
	unsigned int chain, off, h, max;

	max = maxmatch; /* MAXMATCH; */
	if (matchlen < THRESHOLD - 1) matchlen = THRESHOLD - 1;
	matchpos = pos;

	off = 0;
	for (h = hval; too_flag[h] && off < maxmatch - THRESHOLD; ) {
		h = ((h << 5) ^ lha_text[pos + (++off) + 2]) & (unsigned)(HSHSIZ - 1);
	}
	if (off == maxmatch - THRESHOLD) off = 0;
	for (;;) {
		chain = 0;
		scan_pos = hash[h];
		scan_end = (pos > dicsiz) ? pos + off - dicsiz : off;
		while (scan_pos > scan_end) {
			chain++;

			if (lha_text[scan_pos + matchlen - off] == lha_text[pos + matchlen]) {
				{
					a = lha_text + scan_pos - off;  b = lha_text + pos;
					for (len = 0; len < max && *a++ == *b++; len++);
				}

				if (len > matchlen) {
					matchpos = scan_pos - off;
					if ((matchlen = len) == max) {
						break;
					}
#ifdef DEBUG
					if (noslide) {
					  if (matchpos < dicsiz) {
						printf("matchpos=%u scan_pos=%u dicsiz=%u\n"
							   ,matchpos, scan_pos, dicsiz);
					  }
					}
#endif
				}
			}
			scan_pos = prev[scan_pos & (dicsiz - 1)];
		}

		if (chain >= LIMIT)
			too_flag[h] = 1;

		if (matchlen > off + 2 || off == 0)
			break;
		max = off + 2;
		off = 0;
		h = hval;
	}
	prev[pos & (dicsiz - 1)] = hash[hval];
	hash[hval] = pos;
}


/* ポインタを進め、辞書を更新し、ハッシュ値を更新する */

static void get_next()
{
	remainder--;
	if (++pos >= txtsiz - maxmatch) {
		update();
#ifdef DEBUG
		noslide = 0;
#endif
	}
	hval = ((hval << 5) ^ lha_text[pos + 2]) & (unsigned)(HSHSIZ - 1);
}

void encode(struct interfacing *lhinterface)
{
	int lastmatchlen;
	unsigned int lastmatchoffset;

#ifdef DEBUG
	unsigned int addr;

	addr = 0;

	fout = fopen("en", "wt");
	if (fout == NULL) exit(1);
#endif
	infile = lhinterface->infile;
	outfile = lhinterface->outfile;
	origsize = lhinterface->original;
	compsize = count = 0L;
	crc = unpackable = 0;

	/* encode_alloc(); */ /* allocate_memory(); */
	init_slide();  

	encode_set.encode_start();
	memset(&lha_text[0], ' ', (long)TXTSIZ);

	remainder = fread_crc(&lha_text[dicsiz], txtsiz-dicsiz, infile);
	encoded_origsize = remainder;
	matchlen = THRESHOLD - 1;

	pos = dicsiz;

	if (matchlen > remainder) matchlen = remainder;
	hval = ((((lha_text[dicsiz] << 5) ^ lha_text[dicsiz + 1]) << 5) 
	        ^ lha_text[dicsiz + 2]) & (unsigned)(HSHSIZ - 1);

	insert();
	while (remainder > 0 && ! unpackable) {
		lastmatchlen = matchlen;  lastmatchoffset = pos - matchpos - 1;
		--matchlen;
		get_next();  match_insert();
		if (matchlen > remainder) matchlen = remainder;
		if (matchlen > lastmatchlen || lastmatchlen < THRESHOLD) {
			encode_set.output(lha_text[pos - 1], 0);
#ifdef DEBUG
			fprintf(fout, "%u C %02X\n", addr, lha_text[pos-1]);
			addr++;
#endif
			count++;
		} else {
			encode_set.output(lastmatchlen + (UCHAR_MAX + 1 - THRESHOLD),
			   (lastmatchoffset) & (dicsiz-1) );
			--lastmatchlen;

#ifdef DEBUG
			fprintf(fout, "%u M %u %u ", addr,
					lastmatchoffset & (dicsiz-1), lastmatchlen+1);
			addr += lastmatchlen +1 ;

			{
			  int t,cc;
			for (t=0; t<lastmatchlen+1; t++) {
			  cc = lha_text[(pos-(lastmatchoffset)) & (dicsiz-1)];
			  fprintf(fout, "%02X ", cc);
			}
			fprintf(fout, "\n");
			}
#endif
			while (--lastmatchlen > 0) {
				get_next();  insert();
				count++;
			}
			get_next();
			matchlen = THRESHOLD - 1;
			match_insert();
			if (matchlen > remainder) matchlen = remainder;
		}
	}
	encode_set.encode_end();

	interface->packed = compsize;
	interface->original = encoded_origsize;
}

/* ------------------------------------------------------------------------ */

#endif

int
decode(struct interfacing *lhinterface)
{
	unsigned int i, j, k, c;
	unsigned int dicsiz1, offset;
	unsigned char *dtext;
	

#ifdef DEBUG
	fout = fopen("de", "wt");
	if (fout == NULL) exit(1);
#endif

	infile = lhinterface->infile;
	outfile = lhinterface->outfile;
	dicbit = lhinterface->dicbit;
	origsize = lhinterface->original;
	compsize = lhinterface->packed;
	decode_set = decode_define[lhinterface->method - 1];

	crc = 0;
	prev_char = -1;
	dicsiz = 1L << dicbit;
	dtext = (unsigned char *) malloc(dicsiz);
	if (dtext == NULL)
	    return 0;
	for (i=0; i<dicsiz; i++) dtext[i] = 0x20;
	decode_set.decode_start();
	dicsiz1 = dicsiz - 1;
	offset = (lhinterface->method == LARC_METHOD_NUM) ? 0x100 - 2 : 0x100 - 3;
	count = 0;
	loc = 0;
	while (count < origsize) {
		c = decode_set.decode_c();
		if (c <= UCHAR_MAX) {
#ifdef DEBUG
		  fprintf(fout, "%u C %02X\n", count, c);
#endif
			dtext[loc++] = c;
			if (loc == dicsiz) {
				fwrite_crc(dtext, dicsiz, outfile);
				loc = 0;
			}
			count++;
		}
		else {
			j = c - offset;
			i = (loc - decode_set.decode_p() - 1) & dicsiz1;
#ifdef DEBUG
			fprintf(fout, "%u M %u %u ", count, (loc-1-i) & dicsiz1, j);
#endif
			count += j;
			for (k = 0; k < j; k++) {
				c = dtext[(i + k) & dicsiz1];

#ifdef DEBUG
				fprintf(fout, "%02X ", c & 0xff);
#endif
				dtext[loc++] = c;
				if (loc == dicsiz) {
					fwrite_crc(dtext, dicsiz, outfile);
					loc = 0;
				}
			}
#ifdef DEBUG
			fprintf(fout, "\n");
#endif
		}
	}
	if (loc != 0) {
		fwrite_crc(dtext, loc, outfile);
	}

	free(dtext);
    return 1;
}

/* Local Variables: */
/* mode:c */
/* tab-width:4 */
/* End: */
