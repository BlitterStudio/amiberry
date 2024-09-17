/*
 * QEMU Cirrus CLGD 54xx VGA Emulator.
 *
 * Copyright (c) 2004 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

STATIC_INLINE void glue(rop_8_,ROP_NAME)(uint8_t *dst, uint8_t src)
{
    *dst = ROP_FN(*dst, src);
}

STATIC_INLINE void glue(rop_16_,ROP_NAME)(uint16_t *dst, uint16_t src)
{
    *dst = ROP_FN(*dst, src);
}

STATIC_INLINE void glue(rop_32_,ROP_NAME)(uint32_t *dst, uint32_t src)
{
    *dst = ROP_FN(*dst, src);
}

#define ROP_OP(d, s) glue(rop_8_,ROP_NAME)(d, s)
#define ROP_OP_16(d, s) glue(rop_16_,ROP_NAME)(d, s)
#define ROP_OP_32(d, s) glue(rop_32_,ROP_NAME)(d, s)
#undef ROP_FN

static void
glue(cirrus_bitblt_rop_fwd_, ROP_NAME)(CirrusVGAState *s,
                            uint8_t *dst, uint32_t dstaddr, uint32_t dstmask,
							const uint8_t *src, uint32_t srcaddr, uint32_t srcmask,
                            int dstpitch,int srcpitch,
                            int bltwidth,int bltheight)
{
    int x,y;

	BLTCHECK_FWD

    dstpitch -= bltwidth;
    srcpitch -= bltwidth;

    for (y = 0; y < bltheight; y++) {
  		for (x = 0; x < (bltwidth & ~3); x += 4) {
			ROP_OP_32((uint32_t*)dst, *((uint32_t*)src));
			dst += 4;
			src += 4;
		}
		for (; x < bltwidth; x++) {
            ROP_OP(dst, *src);
            dst++;
            src++;
        }
        dst += dstpitch;
        src += srcpitch;
    }
}

static void
glue(cirrus_bitblt_rop_bkwd_, ROP_NAME)(CirrusVGAState *s,
										uint8_t *dst, uint32_t dstaddr, uint32_t dstmask,
										const uint8_t *src, uint32_t srcaddr, uint32_t srcmask,
										int dstpitch,int srcpitch,
                                        int bltwidth,int bltheight)
{
	BLTCHECK_BKWD
	
	int x,y;

	dstpitch += bltwidth;
    srcpitch += bltwidth;

	for (y = 0; y < bltheight; y++) {
  		for (x = 0; x < (bltwidth & ~3); x += 4) {
			dst -= 3;
			src -= 3;
			ROP_OP_32((uint32_t*)dst, *((uint32_t*)src));
			dst -= 1;
			src -= 1;
		}
        for (; x < bltwidth; x++) {
            ROP_OP(dst, *src);
            dst--;
            src--;
        }
        dst += dstpitch;
        src += srcpitch;
    }
}

static void
glue(glue(cirrus_bitblt_rop_fwd_transp_, ROP_NAME),_8)(CirrusVGAState *s,
								uint8_t *dst, uint32_t dstaddr, uint32_t dstmask,
								const uint8_t *src, uint32_t srcaddr, uint32_t srcmask,
								int dstpitch,int srcpitch,
								int bltwidth,int bltheight)
{
	BLTCHECK_FWD

	int x,y;
    uint8_t p;

	dstpitch -= bltwidth;
    srcpitch -= bltwidth;
    
	for (y = 0; y < bltheight; y++) {
        for (x = 0; x < bltwidth; x++) {
	    p = *dst;
            ROP_OP(&p, *src);
	    if (p != s->vga.gr[0x34]) *dst = p;
            dst++;
            src++;
        }
        dst += dstpitch;
        src += srcpitch;
    }
}

static void
glue(glue(cirrus_bitblt_rop_bkwd_transp_, ROP_NAME),_8)(CirrusVGAState *s,
							uint8_t *dst, uint32_t dstaddr, uint32_t dstmask,
							const uint8_t *src, uint32_t srcaddr, uint32_t srcmask,
							int dstpitch,int srcpitch,
							int bltwidth,int bltheight)
{
	BLTCHECK_BKWD
	
	int x,y;
    uint8_t p;

	dstpitch += bltwidth;
    srcpitch += bltwidth;
    
	for (y = 0; y < bltheight; y++) {
        for (x = 0; x < bltwidth; x++) {
	    p = *dst;
            ROP_OP(&p, *src);
	    if (p != s->vga.gr[0x34]) *dst = p;
            dst--;
            src--;
        }
        dst += dstpitch;
        src += srcpitch;
    }
}

static void
glue(glue(cirrus_bitblt_rop_fwd_transp_, ROP_NAME),_16)(CirrusVGAState *s,
							uint8_t *dst, uint32_t dstaddr, uint32_t dstmask,
							const uint8_t *src, uint32_t srcaddr, uint32_t srcmask,
							int dstpitch,int srcpitch,
							int bltwidth,int bltheight)
{
	BLTCHECK_FWD
		
	int x,y;
    uint8_t p1, p2;
    
	dstpitch -= bltwidth;
    srcpitch -= bltwidth;

	for (y = 0; y < bltheight; y++) {
        for (x = 0; x < bltwidth; x+=2) {
	    p1 = *dst;
	    p2 = *(dst+1);
            ROP_OP(&p1, *src);
            ROP_OP(&p2, *(src + 1));
	    if ((p1 != s->vga.gr[0x34]) || (p2 != s->vga.gr[0x35])) {
		*dst = p1;
		*(dst+1) = p2;
	    }
            dst+=2;
            src+=2;
        }
        dst += dstpitch;
        src += srcpitch;
    }
}

static void
glue(glue(cirrus_bitblt_rop_bkwd_transp_, ROP_NAME),_16)(CirrusVGAState *s,
							uint8_t *dst, uint32_t dstaddr, uint32_t dstmask,
							const uint8_t *src, uint32_t srcaddr, uint32_t srcmask,
							int dstpitch,int srcpitch,
							int bltwidth,int bltheight)
{
	BLTCHECK_BKWD

	int x,y;
    uint8_t p1, p2;
    
	dstpitch += bltwidth;
    srcpitch += bltwidth;
    
	for (y = 0; y < bltheight; y++) {
        for (x = 0; x < bltwidth; x+=2) {
	    p1 = *(dst-1);
	    p2 = *dst;
            ROP_OP(&p1, *(src - 1));
            ROP_OP(&p2, *src);
	    if ((p1 != s->vga.gr[0x34]) || (p2 != s->vga.gr[0x35])) {
		*(dst-1) = p1;
		*dst = p2;
	    }
            dst-=2;
            src-=2;
        }
        dst += dstpitch;
        src += srcpitch;
    }
}

#define DEPTH 8
#include "cirrus_vga_rop2.h"

#define DEPTH 16
#include "cirrus_vga_rop2.h"

#define DEPTH 24
#include "cirrus_vga_rop2.h"

#define DEPTH 32
#include "cirrus_vga_rop2.h"

#undef ROP_NAME
#undef ROP_OP
#undef ROP_OP_16
#undef ROP_OP_32
