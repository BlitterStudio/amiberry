

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sysconfig.h"
#include "sysdeps.h"
#include "zfile.h"
#include "crc32.h"

/* based on http://libxad.cvs.sourceforge.net/libxad/libxad/portable/clients/ by Dirk Stoecker */

#define XADERR_ILLEGALDATA 1
#define XADERR_DECRUNCH 2
#define XADERR_NOMEMORY 3

struct rledata {
  uae_u32 rledata;
};

struct fout {
    struct zfile *zf;
    int xio_BitNum;
    int xio_BitBuf;
    int err;
};


static void putrle (uae_u8 data, struct zfile *out, struct rledata *rled)
{
    int num;
    uae_u32 a;

    if (!rled) {
	zfile_putc (data, out);
	return;
    }
    a = rled->rledata;
    if (a & 0x100) /* was RLE mode */
    {
	if (!data || (data == 1 && (a & 0x80000000))) {
	    a = 0x90; num = 1;
	} else {
	    a &= 0xFF; num = data - 1;
	}
    } else if (data == 0x90) {
	num = 0; a |= 0x100;
    } else {
	num = 1; a = data;
    }
    rled->rledata = a;
    while (num--)
	zfile_putc (a, out);
}

static uae_u32 xadIOGetBitsLow(struct fout *io, uae_u8 bits)
{
  uae_u32 x;

  io->err = 0;
  while(io->xio_BitNum < bits)
  {
      int v = zfile_getc (io->zf);
      if (v == -1) {
	  io->err = 1;
	  return 0;
      }
      io->xio_BitBuf |= v << io->xio_BitNum;
      io->xio_BitNum += 8;
  }
  x = io->xio_BitBuf & ((1<<bits)-1);
  io->xio_BitBuf >>= bits;
  io->xio_BitNum -= bits;
  return x;
}

#define ARCSQSPEOF   256                /* special endfile token */
#define ARCSQNUMVALS 257                /* 256 data values plus SPEOF */

static uae_s32 ARCunsqueeze(struct zfile *in, struct zfile *out, struct rledata *rled)
{
  uae_s32 err = 0;
  uae_s32 i, numnodes;
  uae_s16 *node;
  struct fout io;

  io.zf = in;
  io.xio_BitBuf = 0;
  io.xio_BitNum = 0;
  io.err = 0;

  if((node = (uae_s16 *) xcalloc(2*2*ARCSQNUMVALS, 1)))
  {
    numnodes = xadIOGetBitsLow(&io, 16);

    if(numnodes < 0 || numnodes >= ARCSQNUMVALS)
      err = XADERR_DECRUNCH;
    else
    {  /* initialize for possible empty tree (SPEOF only) */
      node[0] = node[1] = -(ARCSQSPEOF + 1);

      numnodes *= 2; i = 0;
      while(i < numnodes && !io.err)       /* get decoding tree from file */
      {
        node[i++] = xadIOGetBitsLow(&io, 16);
        node[i++] = xadIOGetBitsLow(&io, 16);
      }

      do
      {
        /* follow bit stream in tree to a leaf */
        i = 0;
        while(i >= 0 && !io.err)
          i = node[2*i + xadIOGetBitsLow(&io, 1)];

        i = -(i + 1); /* decode fake node index to original data value */

        if(i != ARCSQSPEOF)
          putrle (i, out, rled);
      } while(i != ARCSQSPEOF);
    }
    xfree(node);
  }
  else
    err = XADERR_NOMEMORY;

  return err;
}




#define UCOMPMAXCODE(n) (((uae_u32) 1 << (n)) - 1)
#define UCOMPBITS          16
#define UCOMPSTACKSIZE   8000
#define UCOMPFIRST        257           /* first free entry */
#define UCOMPCLEAR        256           /* table clear output code */
#define UCOMPINIT_BITS      9           /* initial number of bits/code */
#define UCOMPBIT_MASK    0x1f
#define UCOMPBLOCK_MASK  0x80

struct UCompData {
  uae_s16   clear_flg;
  uae_u16   n_bits;                 /* number of bits/code */
  uae_u16   maxbits;                /* user settable max # bits/code */
  uae_u32   maxcode;                /* maximum code, given n_bits */
  uae_u32   maxmaxcode;
  uae_s32   free_ent;
  uae_s32   offset;
  uae_s32   size;
  uae_u16  *tab_prefixof;
  uae_u8   *tab_suffixof;
  uae_u8    stack[UCOMPSTACKSIZE];
  uae_u8    buf[UCOMPBITS];
  int	    insize;
  struct    rledata *rled;
};


/* Read one code from input. If EOF, return -1. */
static uae_s32 UCompgetcode(struct zfile *in, struct UCompData *cd)
{
  uae_s32 code, r_off, bits;
  uae_u8 *bp = cd->buf;

  if(cd->clear_flg > 0 || cd->offset >= cd->size || cd->free_ent > cd->maxcode)
  {
    /*
     * If the next entry will be too big for the current code
     * size, then we must increase the size.  This implies reading
     * a new buffer full, too.
     */
    if(cd->free_ent > cd->maxcode)
    {
      if(++cd->n_bits == cd->maxbits)
        cd->maxcode = cd->maxmaxcode;   /* won't get any bigger now */
      else
        cd->maxcode = UCOMPMAXCODE(cd->n_bits);
    }
    if(cd->clear_flg > 0)
    {
      cd->maxcode = UCOMPMAXCODE(cd->n_bits = UCOMPINIT_BITS);
      cd->clear_flg = 0;
    }

    /* This reads maximum n_bits characters into buf */
    cd->size = 0;
    while(cd->size < cd->n_bits) {
	int v;
	if (cd->insize == 0)
	    break;
	v = zfile_getc (in);
	if (v == -1)
	    break;
	cd->insize--;
	cd->buf[cd->size++] = v;
    }
    if(cd->size <= 0)
      return -1;

    cd->offset = 0;
    /* Round size down to integral number of codes */
    cd->size = (cd->size << 3) - (cd->n_bits - 1);
  }

  r_off = cd->offset;
  bits = cd->n_bits;

  /* Get to the first byte. */
  bp += (r_off >> 3);
  r_off &= 7;

  /* Get first part (low order bits) */
  code = (*bp++ >> r_off);
  bits -= (8 - r_off);
  r_off = 8 - r_off;                    /* now, offset into code word */

  /* Get any 8 bit parts in the middle (<=1 for up to 16 bits). */
  if(bits >= 8)
  {
    code |= *bp++ << r_off;
    r_off += 8;
    bits -= 8;
  }

  /* high order bits. */
  code |= (*bp & ((1<<bits)-1)) << r_off;
  cd->offset += cd->n_bits;

  return code;
}

static uae_u32 xadIO_Compress(struct zfile *in, struct zfile *out, int insize, struct rledata *rled, uae_u8 bitinfo)
{
  int err = 0;
  struct UCompData *cd;

  if((bitinfo & UCOMPBIT_MASK) < UCOMPINIT_BITS)
    return XADERR_ILLEGALDATA;

  if((cd = (struct UCompData *) xcalloc(sizeof(struct UCompData), 1)))
  {
    int finchar, code, oldcode, incode, blockcomp;
    uae_u8 *stackp, *stack, *stackend;

    stackp = stack = cd->stack;
    stackend = stack + UCOMPSTACKSIZE;
    cd->maxbits = bitinfo & UCOMPBIT_MASK;
    blockcomp = bitinfo & UCOMPBLOCK_MASK;
    cd->maxmaxcode = 1 << cd->maxbits;
    cd->maxcode = UCOMPMAXCODE(cd->n_bits = UCOMPINIT_BITS);
    cd->free_ent = blockcomp ? UCOMPFIRST : 256;
    cd->insize = insize;
    cd->rled = rled;

    if((cd->tab_prefixof = (uae_u16 *) xcalloc(sizeof(uae_u16) * cd->maxmaxcode, 1)))
    {
      if((cd->tab_suffixof = (uae_u8 *)xcalloc(cd->maxmaxcode, 1)))
      {
        /* Initialize the first 256 entries in the table. */
        for(code = 255; code >= 0; code--)
          cd->tab_suffixof[code] = code;

        if((finchar = oldcode = UCompgetcode(in, cd)) == -1)
          err = XADERR_DECRUNCH;
        else
        {
	  putrle (finchar, out, cd->rled); /* first code must be 8 bits = uae_u8 */

          while((code = UCompgetcode(in, cd)) > -1)
          {
            if((code == UCOMPCLEAR) && blockcomp)
            {
              for(code = 255; code >= 0; code--)
                cd->tab_prefixof[code] = 0;
              cd->clear_flg = 1;
              cd->free_ent = UCOMPFIRST - 1;
              if((code = UCompgetcode(in, cd)) == -1)
                break;                                /* O, untimely death! */
            }
            incode = code;

            /* Special case for KwKwK string. */
            if(code >= cd->free_ent)
            {
              if(code > cd->free_ent)
              {
                err = XADERR_ILLEGALDATA;
                break;
              }
              *stackp++ = finchar;
              code = oldcode;
            }

            /* Generate output characters in reverse order */
            while(stackp < stackend && code >= 256)
            {
              *stackp++ = cd->tab_suffixof[code];
              code = cd->tab_prefixof[code];
            }
            if(stackp >= stackend)
            {
              err = XADERR_ILLEGALDATA;
              break;
            }
            *(stackp++) = finchar = cd->tab_suffixof[code];

            /* And put them out in forward order */
            do
            {
              putrle (*(--stackp), out, cd->rled);
            } while(stackp > stack);

            /* Generate the new entry. */
            if((code = cd->free_ent) < cd->maxmaxcode)
            {
              cd->tab_prefixof[code] = (uae_u16) oldcode;
              cd->tab_suffixof[code] = finchar;
              cd->free_ent = code+1;
            }
            /* Remember previous code. */
            oldcode = incode;
          }
        }
        xfree (cd->tab_suffixof);
      }
      else
        err = XADERR_NOMEMORY;
      xfree(cd->tab_prefixof);
    }
    else
      err = XADERR_NOMEMORY;
    xfree(cd);
  }
  else
    err = XADERR_NOMEMORY;

  return err;
}

static void MakeCRC16(uae_u16 *buf, uae_u16 ID)
{
  uae_u16 i, j, k;

  for(i = 0; i < 256; ++i)
  {
    k = i;

    for(j = 0; j < 8; ++j)
    {
      if(k & 1)
        k = (k >> 1) ^ ID;
      else
        k >>= 1;
    }
    buf[i] = k;
  }
}

static uae_u16 wrpcrc16 (uae_u16 *tab, uae_u8 *buf, int len)
{
    uae_u16 crc = 0;
    while (len-- > 0)
	crc = tab[(crc ^ *buf++) & 0xFF] ^ (crc >> 8);
    return crc;
}

static int iswrp (uae_u8 *data)
{
    if(data[0] == 'W' && data[1] == 'a' && data[2] == 'r' && data[3] == 'p'
	&& data[4] == ' ' && data[5] == 'v' && data[6] == '1' && data[7] == '.'
	&& data[8] == '1' && !data[9] && !data[18] && data[19] <= 3)
	    return 1;
    return 0;
}

#define COMPBUF 30000

struct zfile *unwarp(struct zfile *zf)
{
    int err = 0;
    uae_u8 buf[26] = { 0 };
    int algo, side, track;
    int pos, dstpos, olddstpos;
    uae_u16 crc;
    uae_u32 size;
    struct zfile *nf = NULL, *tmpf = NULL;
    uae_u8 *zero, *data;
    int outsize = 11 * 512;
    int outsize2 = 11 * (512 + 16);
    struct rledata rled;
    uae_u16 wrpcrc16table[256];

    MakeCRC16 (wrpcrc16table, 0xa001);

    zero = (uae_u8 *) xcalloc (outsize2, 1);
    olddstpos = 0;
    for (;;) {
	if (zfile_fread (buf, sizeof buf, 1, zf) == 0)
	    break;
	if (!iswrp (buf))
	    break;
	if (!nf) {
	    nf = zfile_fopen_empty ("zipped.wrp", 1760 * 512);
	    tmpf = zfile_fopen_empty ("tmp", outsize2);
	}
	track = (buf[10] << 8) | buf[11];
        algo = buf[19];
	side = -1;
	if (!memcmp (buf + 12, "BOT\0", 4))
	    side = 1;
	if (!memcmp (buf + 12, "TOP\0", 4))
	    side = 0;
	crc = (buf[20] << 8) | buf[21];
	pos = zfile_ftell (zf);
	dstpos = -1;
	if (side >= 0 && track >= 0 && track <= 79)
	    dstpos = track * 22 * 512 + (side * 11 * 512);
        zfile_fseek (tmpf, 0, SEEK_SET);
	zfile_fwrite (zero, outsize2, 1, tmpf);
        zfile_fseek (tmpf, 0, SEEK_SET);
	size = (buf[22] << 24) | (buf[23] << 16) | (buf[24] << 8) | buf[25];
	err = 0;
	memset (&rled, 0, sizeof rled);

	switch (algo)
	{
	    case 1:
		if (zfile_getc (zf) != 12)
		    err = XADERR_ILLEGALDATA;
		else
		    err = xadIO_Compress (zf, tmpf, size - 1, &rled, 12 | UCOMPBLOCK_MASK);
	    break;
	    case 2:
		err = ARCunsqueeze (zf, tmpf, &rled);
	    break;
	    case 0:
	    case 3:
	    {
		int i;
		for (i = 0; i < size; i++) {
		    uae_u8 v = zfile_getc (zf);
		    putrle (v, tmpf, algo == 3 ? &rled : NULL);
		}
	    }
	    break;
	    default:
		write_log ("WRP unknown compression method %d, track=%d,size=%d\n", algo, track, side);
		goto end;
	    break;
	}
	if (err) {
	    write_log ("WRP corrupt data, track=%d,side=%d,err=%d\n", track, side, err);
	} else {
	    uae_u16 crc2;
	    int os = zfile_ftell (tmpf);
	    data = (uae_u8 *) zfile_getdata (tmpf, 0, os);
	    crc2 = wrpcrc16 (wrpcrc16table, data, os);
	    if (crc != crc2)
	        write_log ("WRP crc error %04x<>%04x, track=%d,side=%d\n", crc, crc2, track, side);
	    xfree (data);
	}
	if (dstpos >= 0) {
	    zfile_fseek (nf, dstpos, SEEK_SET);
	    data = (uae_u8 *) zfile_getdata (tmpf, 0, outsize);
	    zfile_fwrite (data, outsize, 1, nf);
	}
        zfile_fseek (zf, pos + size, SEEK_SET);
    }
end:
    xfree (zero);
    zfile_fclose (tmpf);
    if (nf) {
	zfile_fclose (zf);
	zf = nf;
    }
    return zf;
}

