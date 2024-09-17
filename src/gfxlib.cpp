 /*
  * UAE - The Un*x Amiga Emulator
  *
  * graphics.library emulation
  *
  * Copyright 1996, 1997 Bernd Schmidt
  *
  * Ideas for this:
  * Rewrite layers completely. When there are lots of windows on the screen
  * it can take 3 minutes to update everything after resizing or moving one
  * (at least with Kick 1.3). Hide the internal structure of the layers as far
  * as possible, keep most of the data in emulator space so we save copying/
  * conversion time. Programs really shouldn't do anything directly with the
  * Layer or ClipRect structures.
  * This means that a lot of graphics.library functions will have to be
  * rewritten as well.
  * Once that's done, add support for non-planar bitmaps. Conveniently, the
  * struct Bitmap has an unused pad field which we could abuse as some sort of
  * type field. Need to add chunky<->planar conversion routines to get it
  * going, plus variants of all the drawing functions for speed reasons.
  *
  * When it becomes necessary to convert a structure from Amiga memory, make
  * a function with a name ending in ..FA, which takes a pointer to the
  * native structure and a uaecptr and returns the native pointer.
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include <assert.h>

#include "options.h"
#include "threaddep/thread.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "xwin.h"
#include "autoconf.h"
#include "osemu.h"

#ifdef USE_EXECLIB

/* Uniq list management. Should be in a separate file. */
struct uniq_head {
    struct uniq_head *next;
    uae_u32 uniq;
};

typedef struct {
    struct uniq_head *head;
    uae_u32 uniq;
} uniq_list;

#define UNIQ_INIT { NULL, 1 }

static void init_uniq(uniq_list *list)
{
    list->head = NULL;
    list->uniq = 1;
}

static struct uniq_head *find_uniq (uniq_list *a, uae_u32 uniq)
{
    struct uniq_head *b = a->head;
    while (b && b->uniq != uniq)
	b = b->next;
    if (!b)
	write_log (_T("Couldn't find structure. Bad\n"));
    return b;
}

static struct uniq_head *find_and_rem_uniq (uniq_list *a, uae_u32 uniq)
{
    struct uniq_head **b = &a->head, *c;
    while (*b && (*b)->uniq != uniq)
	b = &(*b)->next;
    c = *b;
    if (!c)
	write_log (_T("Couldn't find structure. Bad\n"));
    else
	*b = c->next;
    return c;
}

static void add_uniq (uniq_list *a, struct uniq_head *item, uaecptr amem)
{
    item->uniq = a->uniq++;
    put_long (amem, item->uniq);
    if (a->uniq == 0)
	a->uniq++;
    item->next = a->head;
    a->head = item;
}

/* Graphics stuff begins here */
#define CLIPRECT_SIZE 40
#define LAYER_SIZE 160
#define LINFO_SIZE 102

static uaecptr gfxbase, layersbase;

static void do_LockLayer(uaecptr layer)
{
#if 0 /* Later.. */
    uaecptr sigsem = layer + 72;
    m68k_areg (regs, 0) = sigsem;
    CallLib (get_long (4), -564);
#else
    m68k_areg (regs, 1) = layer;
    CallLib (layersbase, -96);
#endif
}

static void do_UnlockLayer(uaecptr layer)
{
    m68k_areg (regs, 0) = layer;
    CallLib (layersbase, -102);
}

static uae_u32 gfxlibname, layerslibname;

struct Rectangle {
    int MinX, MinY, MaxX, MaxY;
};

static int GFX_PointInRectangle(uaecptr rect, int x, int y)
{
    uae_s16 minx = get_word (rect);
    uae_s16 miny = get_word (rect+2);
    uae_s16 maxx = get_word (rect+4);
    uae_s16 maxy = get_word (rect+6);

    if (x < minx || x > maxx || y < miny || y > maxy)
	return 0;
    return 1;
}

static int GFX_RectContainsRect(struct Rectangle *r1, struct Rectangle *r2)
{
    return (r2->MinX >= r1->MinX && r2->MaxX <= r1->MaxX
	    && r2->MinY >= r1->MinY && r2->MaxY <= r1->MaxY);
}

static struct Rectangle *GFX_RectFA(struct Rectangle *rp, uaecptr rect)
{
    rp->MinX = (uae_s16)get_word (rect);
    rp->MinY = (uae_s16)get_word (rect+2);
    rp->MaxX = (uae_s16)get_word (rect+4);
    rp->MaxY = (uae_s16)get_word (rect+6);
    return rp;
}

static int GFX_Bitmap_WritePixel(uaecptr bitmap, int x, int y, uaecptr rp)
{
    int i, offs;
    unsigned int bpr = get_word (bitmap);
    unsigned int rows = get_word (bitmap + 2);
    uae_u16 mask;

    uae_u8 planemask = get_byte (rp + 24);
    uae_u8 fgpen = get_byte (rp + 25);
    uae_u8 bgpen = get_byte (rp + 26);
    uae_u8 drmd = get_byte (rp + 28);
    uae_u8 pen = drmd & 4 ? bgpen : fgpen;

    if (x < 0 || y < 0 || x >= 8*bpr || y >= rows)
	return -1;

    offs = y*bpr + (x & ~15)/8;

    for (i = 0; i < get_byte (bitmap + 5); i++) {
	uaecptr planeptr;
	uae_u16 data;

	if ((planemask & (1 << i)) == 0)
	    continue;

	planeptr = get_long (bitmap + 8 + i*4);
	data = get_word (planeptr + offs);

	mask = 0x8000 >> (x & 15);

	if (drmd & 2) {
	    if ((pen & (1 << i)) != 0)
		data ^=mask;
	} else {
	    data &= ~mask;
	    if ((pen & (1 << i)) != 0)
		data |= mask;
	}
	put_word (planeptr + offs, data);
    }
    return 0;
}

int GFX_WritePixel(uaecptr rp, int x, int y)
{
    int v;
    uaecptr layer = get_long (rp);
    uaecptr bitmap = get_long (rp + 4);
    uaecptr cliprect;
    int x2, y2;

    if (bitmap == 0) {
	write_log (_T("bogus RastPort in WritePixel\n"));
	return -1;
    }

    /* Easy case first */
    if (layer == 0) {
	return GFX_Bitmap_WritePixel(bitmap, x, y, rp);
    }
    do_LockLayer(layer);
    /*
     * Now, in theory we ought to obtain the semaphore.
     * Since we don't, the programs will happily write into the raster
     * even though we are currently moving the window around.
     * Not good.
     */

    x2 = x + (uae_s16)get_word (layer + 16);
    y2 = y + (uae_s16)get_word (layer + 18);

    if (!GFX_PointInRectangle (layer + 16, x2, y2)) {
	do_UnlockLayer(layer);
	return -1;
    }
    /* Find the right ClipRect */
    cliprect = get_long (layer + 8);
    while (cliprect != 0 && !GFX_PointInRectangle (cliprect + 16, x2, y2))
	cliprect = get_long (cliprect);
    if (cliprect == 0) {
	/* Don't complain: The "Dots" demo does this all the time. I
	 * suppose if we can't find a ClipRect, we aren't supposed to draw
	 * the dot.
	 */
	/*write_log (_T("Weirdness in WritePixel\n"));*/
	v = -1;
    } else if (get_long (cliprect + 8) == 0) {
	v = GFX_Bitmap_WritePixel(bitmap, x2, y2, rp);
    } else if (get_long (cliprect + 12) == 0) {
	/* I don't really know what to do here... */
	v = 0;
    } else {
	/* This appears to be normal for smart refresh layers which are obscured */
	v = GFX_Bitmap_WritePixel (get_long (cliprect + 12), x2 - (uae_s16)get_word (cliprect + 16),
				   y2 - (uae_s16)get_word (cliprect + 18), rp);
    }
    do_UnlockLayer(layer);
    return v;
}


static uae_u32 gfxl_WritePixel(void) { return GFX_WritePixel(m68k_areg (regs, 1), (uae_s16)m68k_dreg (regs, 0), (uae_s16)m68k_dreg (regs, 1)); }

static uae_u32 gfxl_BltClear(void)
{
    uaecptr mem=m68k_areg (regs, 1);
    uae_u8 *mptr = chipmem_bank.xlateaddr(m68k_areg (regs, 1));
    uae_u32 count=m68k_dreg (regs, 0);
    uae_u32 flags=m68k_dreg (regs, 1);
    unsigned int i;
    uae_u32 pattern;

    if ((flags & 2) == 2){
	/* count is given in Rows / Bytes per row */
	count=(count & 0xFFFF) * (count >> 16);
    }

    if ((mem & 1) != 0 || (count & 1) != 0)
	write_log (_T("gfx: BltClear called with odd parameters\n"));

    /* Bit 2 set means use pattern (V36+ only, but we might as well emulate
     * it always) */
    if ((flags & 4) == 0)
	pattern = 0;
    else
	pattern= ((flags >> 16) & 0xFFFF) | (flags & 0xFFFF0000ul);

    if ((pattern & 0xFF) == ((pattern >> 8) & 0xFF)) {
	memset(mptr, pattern, count);
	return 0;
    }

    for(i = 0; i < count; i += 4)
	chipmem_bank.lput(mem+i, pattern);

    if ((count & 3) != 0)
	chipmem_bank.wput(mem + i - 4, pattern);

    return 0;
}

static uae_u32 gfxl_BltBitmap(void)
{
    uaecptr srcbitmap = m68k_areg (regs, 0), dstbitmap = m68k_areg (regs, 1);
    int srcx = (uae_s16)m68k_dreg (regs, 0), srcy = (uae_s16)m68k_dreg (regs, 1);
    int dstx = (uae_s16)m68k_dreg (regs, 2), dsty = (uae_s16)m68k_dreg (regs, 3);
    int sizex = (uae_s16)m68k_dreg (regs, 4), sizey = (uae_s16)m68k_dreg (regs, 5);
    uae_u8 minterm = (uae_u8)m68k_dreg (regs, 6), mask = m68k_dreg (regs, 7);
    return 0; /* sam: a return was missing here ! */
}

static uaecptr amiga_malloc(int len)
{
    m68k_dreg (regs, 0) = len;
    m68k_dreg (regs, 1) = 1; /* MEMF_PUBLIC */
    return CallLib (get_long (4), -198); /* AllocMem */
}

static void amiga_free(uaecptr addr, int len)
{
    m68k_areg (regs, 1) = addr;
    m68k_dreg (regs, 0) = len;
    CallLib (get_long (4), -210); /* FreeMem */
}

/*
 * Region handling code
 *
 * General ideas stolen from xc/verylongpath/miregion.c
 *
 * The Clear code is untested. And and Or seem to work, Xor is only used
 * by the 1.3 Prefs program and seems to work, too.
 */

struct RegionRectangle {
    struct RegionRectangle *Next,*Prev;
    struct Rectangle bounds;
};

struct Region {
    struct Rectangle bounds;
    struct RegionRectangle *RegionRectangle;
};

struct RectList {
    int count;
    int space;
    struct Rectangle bounds;
    struct Rectangle *rects;
};

struct BandList {
    int count;
    int space;
    int *miny, *maxy;
};

static void init_bandlist(struct BandList *bl)
{
    bl->count = 0;
    bl->space = 20;
    bl->miny = (int *)malloc(20*sizeof(int));
    bl->maxy = (int *)malloc(20*sizeof(int));
}

static void dup_bandlist(struct BandList *to, struct BandList *from)
{
    to->count = from->count;
    to->space = to->count+4;
    to->miny = (int *)malloc (to->space*sizeof(int));
    to->maxy = (int *)malloc (to->space*sizeof(int));
    memcpy(to->miny, from->miny, to->count*sizeof(int));
    memcpy(to->maxy, from->maxy, to->count*sizeof(int));
}

STATIC_INLINE void add_band(struct BandList *bl, int miny, int maxy, int pos)
{
    if (bl->count == bl->space) {
	bl->space += 20;
	bl->miny = (int *)realloc(bl->miny, bl->space*sizeof(int));
	bl->maxy = (int *)realloc(bl->maxy, bl->space*sizeof(int));
    }
    memmove(bl->miny + pos + 1, bl->miny + pos, (bl->count - pos) * sizeof(int));
    memmove(bl->maxy + pos + 1, bl->maxy + pos, (bl->count - pos) * sizeof(int));
    bl->count++;
    bl->miny[pos] = miny;
    bl->maxy[pos] = maxy;
}

static void init_rectlist(struct RectList *rl)
{
    rl->count = 0;
    rl->space = 100;
    rl->bounds.MinX = rl->bounds.MinY = rl->bounds.MaxX = rl->bounds.MaxY = 0;
    rl->rects = (struct Rectangle *)malloc(100*sizeof(struct Rectangle));
}

static void dup_rectlist(struct RectList *to, struct RectList *from)
{
    to->count = from->count;
    to->space = to->count+4;
    to->bounds = from->bounds;
    to->rects = (struct Rectangle *)malloc (to->space*sizeof(struct Rectangle));
    memcpy(to->rects, from->rects, to->count*sizeof(struct Rectangle));
}

STATIC_INLINE void add_rect(struct RectList *rl, struct Rectangle r)
{
    if (rl->count == 0)
	rl->bounds = r;
    else {
	if (r.MinX < rl->bounds.MinX)
	    rl->bounds.MinX = r.MinX;
	if (r.MinY < rl->bounds.MinY)
	    rl->bounds.MinY = r.MinY;
	if (r.MaxX > rl->bounds.MaxX)
	    rl->bounds.MaxX = r.MaxX;
	if (r.MaxY > rl->bounds.MaxY)
	    rl->bounds.MaxY = r.MaxY;
    }
    if (rl->count == rl->space) {
	rl->space += 100;
	rl->rects = (struct Rectangle *)realloc(rl->rects, rl->space*sizeof(struct Rectangle));
    }
    rl->rects[rl->count++] = r;
}

STATIC_INLINE void rem_rect(struct RectList *rl, int num)
{
    rl->count--;
    if (num == rl->count)
	return;
    rl->rects[num] = rl->rects[rl->count];
}

static void free_rectlist(struct RectList *rl)
{
    free(rl->rects);
}

static void free_bandlist(struct BandList *bl)
{
    free(bl->miny);
    free(bl->maxy);
}

static int regionrect_cmpfn(const void *a, const void *b)
{
    struct Rectangle *ra = (struct Rectangle *)a;
    struct Rectangle *rb = (struct Rectangle *)b;

    if (ra->MinY < rb->MinY)
	return -1;
    if (ra->MinY > rb->MinY)
	return 1;
    if (ra->MinX < rb->MinX)
	return -1;
    if (ra->MinX > rb->MinX)
	return 1;
    if (ra->MaxX < rb->MaxX)
	return -1;
    return 1;
}

STATIC_INLINE int min(int x, int y)
{
    return x < y ? x : y;
}

STATIC_INLINE int max(int x, int y)
{
    return x > y ? x : y;
}

static void add_rect_to_bands(struct BandList *bl, struct Rectangle *rect)
{
    int j;
    struct Rectangle tmpr = *rect;

    for (j = 0; j < bl->count; j++) {
	/* Is the current band before the rectangle? */
	if (bl->maxy[j] < tmpr.MinY)
	    continue;
	/* Band already present? */
	if (bl->miny[j] == tmpr.MinY && bl->maxy[j] == tmpr.MaxY)
	    break;
	/* Completely new band? Add it */
	if (bl->miny[j] > tmpr.MaxY) {
	    add_band(bl, tmpr.MinY, tmpr.MaxY, j);
	    break;
	}
	/* Now we know that the bands are overlapping.
	 * See whether they match in one point */
	if (bl->miny[j] == tmpr.MinY) {
	    int t;
	    if (bl->maxy[j] < tmpr.MaxY) {
		/* Rectangle exceeds band */
		tmpr.MinY = bl->maxy[j]+1;
		continue;
	    }
	    /* Rectangle splits band */
	    t = bl->maxy[j];
	    bl->maxy[j] = tmpr.MaxY;
	    tmpr.MinY = bl->maxy[j] + 1;
	    tmpr.MaxY = t;
	    continue;
	} else if (bl->maxy[j] == tmpr.MaxY) {
	    int t;
	    if (bl->miny[j] > tmpr.MinY) {
		/* Rectangle exceeds band */
		t = bl->miny[j];
		bl->miny[j] = tmpr.MinY;
		bl->maxy[j] = t-1;
		tmpr.MinY = t;
		continue;
	    }
	    /* Rectangle splits band */
	    bl->maxy[j] = tmpr.MinY - 1;
	    continue;
	}
	/* Bands overlap and match in no points. Get a new band and align */
	if (bl->miny[j] > tmpr.MinY) {
	    /* Rectangle begins before band, so make a new band before
	     * and adjust rectangle */
	    add_band(bl, tmpr.MinY, bl->miny[j] - 1, j);
	    tmpr.MinY = bl->miny[j+1];
	} else {
	    /* Rectangle begins in band */
	    add_band(bl, bl->miny[j], tmpr.MinY - 1, j);
	    bl->miny[j+1] = tmpr.MinY;
	}
	continue;
    }
    if (j == bl->count)
	add_band(bl, tmpr.MinY, tmpr.MaxY, j);
}

static void region_addbands(struct RectList *rl, struct BandList *bl)
{
    int i,j;

    for (i = 0; i < rl->count; i++) {
	add_rect_to_bands(bl, rl->rects + i);
    }
}

static void merge_bands(struct BandList *dest, struct BandList *src)
{
    int i;
    for (i = 0; i < src->count; i++) {
	struct Rectangle tmp;
	tmp.MinY = src->miny[i];
	tmp.MaxY = src->maxy[i];
	add_rect_to_bands(dest, &tmp);
    }
}

static void region_splitrects_band(struct RectList *rl, struct BandList *bl)
{
    int i,j;
    for (i = 0; i < rl->count; i++) {
	for (j = 0; j < bl->count; j++) {
	    if (bl->miny[j] == rl->rects[i].MinY && bl->maxy[j] == rl->rects[i].MaxY)
		break;
	    if (rl->rects[i].MinY > bl->maxy[j])
		continue;
	    if (bl->miny[j] == rl->rects[i].MinY) {
		struct Rectangle tmpr;
		tmpr.MinX = rl->rects[i].MinX;
		tmpr.MaxX = rl->rects[i].MaxX;
		tmpr.MinY = bl->maxy[j] + 1;
		tmpr.MaxY = rl->rects[i].MaxY;
		add_rect(rl, tmpr); /* will be processed later */
		rl->rects[i].MaxY = bl->maxy[j];
		break;
	    }
	    write_log (_T("Foo..\n"));
	}
    }
    qsort(rl->rects, rl->count, sizeof (struct Rectangle), regionrect_cmpfn);
}

static void region_coalesce_rects(struct RectList *rl, int do_2nd_pass)
{
    int i,j;

    /* First pass: Coalesce horizontally */
    for (i = j = 0; i < rl->count;) {
	int offs = 1;
	while (i + offs < rl->count) {
	    if (rl->rects[i].MinY != rl->rects[i+offs].MinY
		|| rl->rects[i].MaxY != rl->rects[i+offs].MaxY
		|| rl->rects[i].MaxX+1 < rl->rects[i+offs].MinX)
		break;
	    rl->rects[i].MaxX = rl->rects[i+offs].MaxX;
	    offs++;
	}
	rl->rects[j++] = rl->rects[i];
	i += offs;
    }
    rl->count = j;

    if (!do_2nd_pass)
	return;

    /* Second pass: Coalesce bands */
    for (i = 0; i < rl->count;) {
	int match = 0;
	for (j = i + 1; j < rl->count; j++)
	    if (rl->rects[i].MinY != rl->rects[j].MinY)
		break;
	if (j < rl->count && rl->rects[i].MaxY + 1 == rl->rects[j].MinY) {
	    int k;
	    match = 1;
	    for (k = 0; i+k < j; k++) {
		if (j+k >= rl->count
		    || rl->rects[j+k].MinY != rl->rects[j].MinY)
		{
		    match = 0; break;
		}
		if (rl->rects[i+k].MinX != rl->rects[j+k].MinX
		    || rl->rects[i+k].MaxX != rl->rects[j+k].MaxX)
		{
		    match = 0;
		    break;
		}
	    }
	    if (j+k < rl->count && rl->rects[j+k].MinY == rl->rects[j].MinY)
		match = 0;
	    if (match) {
		for (k = 0; i+k < j; k++)
		    rl->rects[i+k].MaxY = rl->rects[j].MaxY;
		memmove(rl->rects + j, rl->rects + j + k, (rl->count - j - k)*sizeof(struct Rectangle));
		rl->count -= k;
	    }
	}
	if (!match)
	    i = j;
    }
}

static int copy_rects (uaecptr region, struct RectList *rl)
{
    uaecptr regionrect;
    int numrects = 0;
    struct Rectangle b;
    regionrect = get_long (region+8);
    b.MinX = get_word (region);
    b.MinY = get_word (region+2);
    b.MaxX = get_word (region+4);
    b.MaxY = get_word (region+6);

    while (regionrect != 0) {
	struct Rectangle tmpr;

	tmpr.MinX = (uae_s16)get_word (regionrect+8)  + b.MinX;
	tmpr.MinY = (uae_s16)get_word (regionrect+10) + b.MinY;
	tmpr.MaxX = (uae_s16)get_word (regionrect+12) + b.MinX;
	tmpr.MaxY = (uae_s16)get_word (regionrect+14) + b.MinY;
	add_rect(rl, tmpr);
	regionrect = get_long (regionrect);
	numrects++;
    }
    return numrects;
}

static int rect_in_region(struct RectList *rl, struct Rectangle *r)
{
    int i;
    int miny = r->MinY;

    for (i = 0; i < rl->count; i++) {
	int j;
	if (rl->rects[i].MaxY < miny)
	    continue;
	if (rl->rects[i].MinY > miny)
	    break;
	if (rl->rects[i].MaxX < r->MinX)
	    continue;
	if (rl->rects[i].MinX > r->MaxX)
	    break;
	/* Overlap! */
	j = i;
	for (;;) {
	    if (rl->rects[j].MaxX > r->MaxX) {
		miny = rl->rects[i].MaxY + 1;
		break;
	    }
	    j++;
	    if (j == rl->count)
		break;
	    if (rl->rects[j].MinX != rl->rects[j-1].MaxX+1)
		break;
	    if (rl->rects[i].MinY != rl->rects[j].MinY)
		break;
	}
	if (miny <= rl->rects[i].MaxY)
	    break;
    }
    return 0;
}

typedef void (*regionop)(struct RectList *,struct RectList *,struct RectList *);

static void region_do_ClearRegionRegion(struct RectList *rl1,struct RectList *rl2,
					struct RectList *rl3)
{
    int i,j;

    for (i = j = 0; i < rl2->count && j < rl1->count;) {
	struct Rectangle tmpr;

	while ((rl1->rects[j].MinY < rl2->rects[i].MinY
		|| (rl1->rects[j].MinY == rl2->rects[i].MinY
		    && rl1->rects[j].MaxX < rl2->rects[i].MinX))
	       && j < rl1->count)
	    j++;
	if (j >= rl1->count)
	    break;
	while ((rl1->rects[j].MinY > rl2->rects[i].MinY
		|| (rl1->rects[j].MinY == rl2->rects[i].MinY
		    && rl1->rects[j].MinX > rl2->rects[i].MaxX))
	       && i < rl2->count)
	{
	    add_rect(rl3, rl2->rects[i]);
	    i++;
	}
	if (i >= rl2->count)
	    break;

	tmpr = rl2->rects[i];

	while (i < rl2->count && j < rl1->count
	       && rl1->rects[j].MinY == tmpr.MinY
	       && rl2->rects[i].MinY == tmpr.MinY
	       && rl1->rects[j].MinX <= rl2->rects[i].MaxX
	       && rl1->rects[j].MaxX >= rl2->rects[i].MinX)
	{
	    int oldmin = tmpr.MinX;
	    int oldmax = tmpr.MaxX;
	    if (tmpr.MinX < rl1->rects[j].MinX) {
		tmpr.MaxX = rl1->rects[j].MinX - 1;
		add_rect(rl3, tmpr);
	    }
	    if (oldmax <= rl1->rects[j].MaxX) {
		i++;
		if (i < rl2->count && rl2->rects[i].MinY == tmpr.MinY)
		    tmpr = rl2->rects[i];
	    } else {
		tmpr.MinX = rl1->rects[j].MaxX + 1;
		tmpr.MaxX = oldmax;
		j++;
	    }
	}
    }
    for(; i < rl2->count; i++)
	add_rect(rl3, rl2->rects[i]);
}

static void region_do_AndRegionRegion(struct RectList *rl1,struct RectList *rl2,
				      struct RectList *rl3)
{
    int i,j;

    for (i = j = 0; i < rl2->count && j < rl1->count;) {
	while ((rl1->rects[j].MinY < rl2->rects[i].MinY
		|| (rl1->rects[j].MinY == rl2->rects[i].MinY
		    && rl1->rects[j].MaxX < rl2->rects[i].MinX))
	       && j < rl1->count)
	    j++;
	if (j >= rl1->count)
	    break;
	while ((rl1->rects[j].MinY > rl2->rects[i].MinY
		|| (rl1->rects[j].MinY == rl2->rects[i].MinY
		    && rl1->rects[j].MinX > rl2->rects[i].MaxX))
	       && i < rl2->count)
	    i++;
	if (i >= rl2->count)
	    break;
	if (rl1->rects[j].MinY == rl2->rects[i].MinY
	    && rl1->rects[j].MinX <= rl2->rects[i].MaxX
	    && rl1->rects[j].MaxX >= rl2->rects[i].MinX)
	{
	    /* We have an intersection! */
	    struct Rectangle tmpr;
	    tmpr = rl2->rects[i];
	    if (tmpr.MinX < rl1->rects[j].MinX)
		tmpr.MinX = rl1->rects[j].MinX;
	    if (tmpr.MaxX > rl1->rects[j].MaxX)
		tmpr.MaxX = rl1->rects[j].MaxX;
	    add_rect(rl3, tmpr);
	    if (rl1->rects[j].MaxX == rl2->rects[i].MaxX)
		i++, j++;
	    else if (rl1->rects[j].MaxX > rl2->rects[i].MaxX)
		i++;
	    else
		j++;
	}
    }
}

static void region_do_OrRegionRegion(struct RectList *rl1,struct RectList *rl2,
				     struct RectList *rl3)
{
    int i,j;

    for (i = j = 0; i < rl2->count && j < rl1->count;) {
	while ((rl1->rects[j].MinY < rl2->rects[i].MinY
		|| (rl1->rects[j].MinY == rl2->rects[i].MinY
		    && rl1->rects[j].MaxX < rl2->rects[i].MinX))
	       && j < rl1->count)
	{
	    add_rect(rl3, rl1->rects[j]);
	    j++;
	}
	if (j >= rl1->count)
	    break;
	while ((rl1->rects[j].MinY > rl2->rects[i].MinY
		|| (rl1->rects[j].MinY == rl2->rects[i].MinY
		    && rl1->rects[j].MinX > rl2->rects[i].MaxX))
	       && i < rl2->count)
	{
	    add_rect(rl3, rl2->rects[i]);
	    i++;
	}
	if (i >= rl2->count)
	    break;
	if (rl1->rects[j].MinY == rl2->rects[i].MinY
	    && rl1->rects[j].MinX <= rl2->rects[i].MaxX
	    && rl1->rects[j].MaxX >= rl2->rects[i].MinX)
	{
	    /* We have an intersection! */
	    struct Rectangle tmpr;
	    tmpr = rl2->rects[i];
	    if (tmpr.MinX > rl1->rects[j].MinX)
		tmpr.MinX = rl1->rects[j].MinX;
	    if (tmpr.MaxX < rl1->rects[j].MaxX)
		tmpr.MaxX = rl1->rects[j].MaxX;
	    i++; j++;
	    for (;;) {
		int cont = 0;
		if (j < rl1->count && rl1->rects[j].MinY == tmpr.MinY
		    && tmpr.MaxX+1 >= rl1->rects[j].MinX) {
		    if (tmpr.MaxX < rl1->rects[j].MaxX)
			tmpr.MaxX = rl1->rects[j].MaxX;
		    j++; cont = 1;
		}
		if (i < rl2->count && rl2->rects[i].MinY == tmpr.MinY
		    && tmpr.MaxX+1 >= rl2->rects[i].MinX) {
		    if (tmpr.MaxX < rl2->rects[i].MaxX)
			tmpr.MaxX = rl2->rects[i].MaxX;
		    i++; cont = 1;
		}
		if (!cont)
		    break;
	    }
	    add_rect(rl3, tmpr);
	}
    }
    for(; i < rl2->count; i++)
	add_rect(rl3, rl2->rects[i]);
    for(; j < rl1->count; j++)
	add_rect(rl3, rl1->rects[j]);
}

static void region_do_XorRegionRegion(struct RectList *rl1,struct RectList *rl2,
				      struct RectList *rl3)
{
    int i,j;

    for (i = j = 0; i < rl2->count && j < rl1->count;) {
	struct Rectangle tmpr1, tmpr2;

	while ((rl1->rects[j].MinY < rl2->rects[i].MinY
		|| (rl1->rects[j].MinY == rl2->rects[i].MinY
		    && rl1->rects[j].MaxX < rl2->rects[i].MinX))
	       && j < rl1->count)
	{
	    add_rect(rl3, rl1->rects[j]);
	    j++;
	}
	if (j >= rl1->count)
	    break;
	while ((rl1->rects[j].MinY > rl2->rects[i].MinY
		|| (rl1->rects[j].MinY == rl2->rects[i].MinY
		    && rl1->rects[j].MinX > rl2->rects[i].MaxX))
	       && i < rl2->count)
	{
	    add_rect(rl3, rl2->rects[i]);
	    i++;
	}
	if (i >= rl2->count)
	    break;

	tmpr2 = rl2->rects[i];
	tmpr1 = rl1->rects[j];

	while (i < rl2->count && j < rl1->count
	       && rl1->rects[j].MinY == tmpr1.MinY
	       && rl2->rects[i].MinY == tmpr1.MinY
	       && rl1->rects[j].MinX <= rl2->rects[i].MaxX
	       && rl1->rects[j].MaxX >= rl2->rects[i].MinX)
	{
	    int oldmin2 = tmpr2.MinX;
	    int oldmax2 = tmpr2.MaxX;
	    int oldmin1 = tmpr1.MinX;
	    int oldmax1 = tmpr1.MaxX;
	    int need_1 = 0, need_2 = 0;

	    if (tmpr2.MinX > tmpr1.MinX	&& tmpr2.MaxX < tmpr1.MaxX)
	    {
		/*
		 *    ###########
		 *       ****
		 */
		tmpr1.MaxX = tmpr2.MinX - 1;
		add_rect(rl3, tmpr1);
		tmpr1.MaxX = oldmax1;
		tmpr1.MinX = tmpr2.MaxX + 1;
		add_rect(rl3, tmpr1);
		need_2 = 1;
	    } else if (tmpr2.MinX > tmpr1.MinX && tmpr2.MaxX > tmpr1.MaxX) {
		/*
		 *    ##########
		 *       *********
		 */
		tmpr1.MaxX = tmpr2.MinX - 1;
		add_rect(rl3, tmpr1);
		tmpr2.MinX = oldmax1 + 1;
		add_rect(rl3, tmpr2);
		need_1 = 1;
	    } else if (tmpr2.MinX < tmpr1.MinX && tmpr2.MaxX < tmpr1.MaxX) {
		/*
		 *       ##########
		 *    *********
		 */
		tmpr2.MaxX = tmpr1.MinX - 1;
		add_rect(rl3, tmpr2);
		tmpr1.MinX = oldmax2 + 1;
		add_rect(rl3, tmpr1);
		need_2 = 1;
	    } else if (tmpr2.MinX < tmpr1.MinX && tmpr2.MaxX > tmpr1.MaxX) {
		/*
		 *       ###
		 *    *********
		 */
		tmpr2.MaxX = tmpr1.MinX - 1;
		add_rect(rl3, tmpr2);
		tmpr2.MaxX = oldmax2;
		tmpr2.MinX = tmpr1.MaxX + 1;
		add_rect(rl3, tmpr2);
		need_1 = 1;
	    } else if (tmpr1.MinX == tmpr2.MinX && tmpr2.MaxX < tmpr1.MaxX) {
		/*
		 *    #############
		 *    *********
		 */
		tmpr1.MinX = tmpr2.MaxX + 1;
		need_2 = 1;
	    } else if (tmpr1.MinX == tmpr2.MinX && tmpr2.MaxX > tmpr1.MaxX) {
		/*
		 *    #########
		 *    *************
		 */
		tmpr2.MinX = tmpr1.MaxX + 1;
		need_1 = 1;
	    } else if (tmpr1.MinX < tmpr2.MinX && tmpr2.MaxX == tmpr1.MaxX) {
		/*
		 *    #############
		 *        *********
		 */
		tmpr1.MaxX = tmpr2.MinX - 1;
		add_rect(rl3, tmpr1);
		need_2 = need_1 = 1;
	    } else if (tmpr1.MinX > tmpr2.MinX && tmpr2.MaxX == tmpr1.MaxX) {
		/*
		 *        #########
		 *    *************
		 */
		tmpr2.MaxX = tmpr1.MinX - 1;
		add_rect(rl3, tmpr2);
		need_2 = need_1 = 1;
	    } else {
		assert(tmpr1.MinX == tmpr2.MinX && tmpr2.MaxX == tmpr1.MaxX);
		need_1 = need_2 = 1;
	    }
	    if (need_1) {
		j++;
		if (j < rl1->count && rl1->rects[j].MinY == tmpr1.MinY)
		    tmpr1 = rl1->rects[j];
	    }
	    if (need_2) {
		i++;
		if (i < rl2->count && rl2->rects[i].MinY == tmpr2.MinY)
		    tmpr2 = rl2->rects[i];
	    }
	}
    }
    for(; i < rl2->count; i++)
	add_rect(rl3, rl2->rects[i]);
    for(; j < rl1->count; j++)
	add_rect(rl3, rl1->rects[j]);
}

static uae_u32 gfxl_perform_regionop(regionop op, int with_rect)
{
    int i,j,k;
    uaecptr reg1;
    uaecptr reg2;
    uaecptr tmp, rpp;
    struct RectList rl1, rl2, rl3;
    struct BandList bl;

    int retval = 0;
    int numrects2;

    init_rectlist(&rl1); init_rectlist(&rl2); init_rectlist(&rl3);

    if (with_rect) {
	struct Rectangle tmpr;
	reg2 = m68k_areg (regs, 0);
	numrects2 = copy_rects(reg2, &rl2);
	tmpr.MinX = get_word (m68k_areg (regs, 1));
	tmpr.MinY = get_word (m68k_areg (regs, 1) + 2);
	tmpr.MaxX = get_word (m68k_areg (regs, 1) + 4);
	tmpr.MaxY = get_word (m68k_areg (regs, 1) + 6);
	add_rect(&rl1, tmpr);
    } else {
	reg1 = m68k_areg (regs, 0);
	reg2 = m68k_areg (regs, 1);

	copy_rects(reg1, &rl1);
	numrects2 = copy_rects(reg2, &rl2);
    }

    init_bandlist(&bl);
    region_addbands(&rl1, &bl);
    region_addbands(&rl2, &bl);
    region_splitrects_band(&rl1, &bl);
    region_splitrects_band(&rl2, &bl);
    region_coalesce_rects(&rl1, 0);
    region_coalesce_rects(&rl2, 0);

    (*op)(&rl1, &rl2, &rl3);
    region_coalesce_rects(&rl3, 1);

    rpp = reg2 + 8;
    if (rl3.count < numrects2) {
	while (numrects2-- != rl3.count) {
	    tmp = get_long (rpp);
	    put_long (rpp, get_long (tmp));
	    amiga_free(tmp, 16);
	}
	if (rl3.count > 0)
	    put_long (get_long (rpp) + 4, rpp);
    } else if (rl3.count > numrects2) {
	while(numrects2++ != rl3.count) {
	    uaecptr prev = get_long (rpp);
	    tmp = amiga_malloc(16);
	    if (tmp == 0)
		goto done;
	    put_long (tmp, prev);
	    put_long (tmp + 4, rpp);
	    if (prev != 0)
		put_long (prev + 4, tmp);
	    put_long (rpp, tmp);
	}
    }

    if (rl3.count > 0) {
	rpp = reg2 + 8;
	for (i = 0; i < rl3.count; i++) {
	    uaecptr rr = get_long (rpp);
	    put_word (rr+8, rl3.rects[i].MinX - rl3.bounds.MinX);
	    put_word (rr+10, rl3.rects[i].MinY - rl3.bounds.MinY);
	    put_word (rr+12, rl3.rects[i].MaxX - rl3.bounds.MinX);
	    put_word (rr+14, rl3.rects[i].MaxY - rl3.bounds.MinY);
	    rpp = rr;
	}
	if (get_long (rpp) != 0)
	    write_log (_T("BUG\n"));
    }
    put_word (reg2+0, rl3.bounds.MinX);
    put_word (reg2+2, rl3.bounds.MinY);
    put_word (reg2+4, rl3.bounds.MaxX);
    put_word (reg2+6, rl3.bounds.MaxY);
    retval = 1;

    done:
    free_rectlist(&rl1); free_rectlist(&rl2); free_rectlist(&rl3);
    free_bandlist(&bl);

    return retval;
}

static uae_u32 gfxl_AndRegionRegion(void)
{
    return gfxl_perform_regionop(region_do_AndRegionRegion, 0);
}
static uae_u32 gfxl_XorRegionRegion(void)
{
    return gfxl_perform_regionop(region_do_XorRegionRegion, 0);
}
static uae_u32 gfxl_OrRegionRegion(void)
{
    return gfxl_perform_regionop(region_do_OrRegionRegion, 0);
}

static uae_u32 gfxl_ClearRectRegion(void)
{
    return gfxl_perform_regionop(region_do_ClearRegionRegion, 1);
}
static uae_u32 gfxl_OrRectRegion(void)
{
    return gfxl_perform_regionop(region_do_OrRegionRegion, 1);
}

static uae_u32 gfxl_AndRectRegion(void)
{
    return gfxl_perform_regionop(region_do_AndRegionRegion, 1);
}

static uae_u32 gfxl_XorRectRegion(void)
{
    return gfxl_perform_regionop(region_do_XorRegionRegion, 1);
}


/* Layers code */

static uae_u32 LY_TryLockLayer(uaecptr layer)
{
    uaecptr sigsem = layer + 72;

    m68k_areg (regs, 0) = sigsem;
    return CallLib (get_long (4), -576);
}

static void LY_LockLayer(uaecptr layer)
{
    uaecptr sigsem = layer + 72;

    m68k_areg (regs, 0) = sigsem;
    CallLib (get_long (4), -564);
}

static void LY_UnlockLayer(uaecptr layer)
{
    uaecptr sigsem = layer + 72;

    m68k_areg (regs, 0) = sigsem;
    CallLib (get_long (4), -570);
}

static void LY_LockLayerInfo(uaecptr li)
{
    uaecptr sigsem = li + 24;

    m68k_areg (regs, 0) = sigsem;
    CallLib (get_long (4), -564);
    put_byte (li+91, get_byte (li+91)+1);
}

static void LY_UnlockLayerInfo(uaecptr li)
{
    uaecptr sigsem = li + 24;

    put_byte (li+91, get_byte (li+91)-1);
    m68k_areg (regs, 0) = sigsem;
    CallLib (get_long (4), -570);
}

static void LY_LockLayers(uaecptr li)
{
    uaecptr l = get_long (li);
    LY_LockLayerInfo(li);
    while (l != 0) {
	LY_LockLayer(l);
	l = get_long (l);
    }
    LY_UnlockLayerInfo(li);
}

static void LY_UnlockLayers(uaecptr li)
{
    uaecptr l = get_long (li);
    LY_LockLayerInfo(li);
    while (l != 0) {
	LY_UnlockLayer(l);
	l = get_long (l);
    }
    LY_UnlockLayerInfo(li);
}

#define LAYER_CLUELESS	 0x8000 /* Indicates we know nothing about the layer's regions. */
#define LAYER_CR_CHANGED 0x4000 /* Indicates that the cliprects in Amiga memory need to be re-done */
#define LAYER_REDO	 0x2000 /* Indicates that we have regions, but they are bogus. */

static uae_u32 layer_uniq = 1;

struct MyLayerInfo {
    struct uniq_head head;
    uaecptr amigaos_linfo;
    uniq_list layer_list;
};

struct MyLayer {
    struct uniq_head head;
    uaecptr amigaos_layer, rastport;
    struct Rectangle bounds;
    struct RectList clipregion;
    struct RectList obscured;
    struct RectList visible;
    struct BandList big_bands; /* created by obscuring layers */
    struct BandList small_bands; /* big_bands + those from clipregion */
    struct RectList damage;
    struct BandList damage_bands;
    struct MyLayerInfo *mli;
};

static uniq_list MyLayerInfo_list = UNIQ_INIT;

static void LY_InitLayers(uaecptr li)
{
    memset (get_real_address (li), 0, 92);
    put_long (li + 0, 0); /* top layer */
    put_long (li+84, 0); /* uniq: */
    m68k_areg (regs, 0) = li + 24; CallLib (get_long (4), -558); /* InitSemaphore() */
    put_word (li+88, 0); /* flags (???) */
    put_byte (li+89, 0); /* fatten_count */
    /* @@@ How big can I assume the structure? What's all this 1.0/1.1 cruft? */
}

static void LY_FattenLayerInfo(uaecptr li)
{
    struct MyLayerInfo *mli;
    int fatten_count = get_byte (li + 89);
    if (fatten_count == 0) {
	mli = (struct MyLayerInfo *)malloc(sizeof(struct MyLayerInfo));
	add_uniq(&MyLayerInfo_list, &mli->head, li + 84);
	init_uniq(&mli->layer_list);
	mli->amigaos_linfo = li;
    }
    put_byte (li + 89, fatten_count + 1);
}

static void LY_ThinLayerInfo(uaecptr li)
{
    int fatten_count = get_byte (li + 89)-1;
    put_byte (li + 89, fatten_count);
    if (fatten_count == 0) {
	struct MyLayerInfo *mli = (struct MyLayerInfo *)find_and_rem_uniq(&MyLayerInfo_list, get_long (li+84));
	if (mli)
	    free(mli);
    }
}

static void build_cliprect (struct MyLayer *l, struct Rectangle *bounds,
			    int obscured, uaecptr *crp, uaecptr *prev)
{
    uaecptr cr = get_long (*crp);
    if (cr == 0) {
	put_long (*crp, cr = amiga_malloc(CLIPRECT_SIZE));
	put_long (cr, 0);
    }
    *prev = cr;
    *crp = cr;
    put_word (cr + 16, bounds->MinX);
    put_word (cr + 18, bounds->MinY);
    put_word (cr + 20, bounds->MaxX);
    put_word (cr + 22, bounds->MaxY);
    put_long (cr + 8, obscured ? l->amigaos_layer : 0); /* cheat */
    put_long (cr + 12, 0); /* no smart refresh yet */
}

static void build_cliprects (struct MyLayer *l)
{
    uaecptr layer = l->amigaos_layer;
    uaecptr cr = layer + 8;
    uaecptr prev = 0;
    uae_u16 flags = get_word (layer + 30);
    int i;

    if ((flags & LAYER_CR_CHANGED) == 0)
	return;
    put_word (layer + 30, flags & ~LAYER_CR_CHANGED);
    for (i = 0; i < l->obscured.count; i++) {
	build_cliprect (l, l->obscured.rects + i, 1, &cr, &prev);
    }
    for (i = 0; i < l->visible.count; i++) {
	build_cliprect (l, l->visible.rects + i, 1, &cr, &prev);
    }
    while ((prev = get_long (cr))) {
	put_long (cr, get_long (prev));
	amiga_free (prev, CLIPRECT_SIZE);
    }
}

static void propagate_clueless_redo (struct MyLayerInfo *mli)
{
    /* For all CLUELESS layers, set the REDO bit for all layers below it that overlap it
     * and delete the data associated with them. */
    uaecptr current_l = get_long (mli->amigaos_linfo);
    while (current_l) {
	struct MyLayer *l = (struct MyLayer *)find_uniq(&mli->layer_list, get_long (current_l + 24));
	if ((get_word (l->amigaos_layer + 32) & LAYER_CLUELESS) != 0) {
	    uaecptr next_l = get_long (current_l);
	    put_word (l->amigaos_layer + 32, get_word (l->amigaos_layer + 32) | LAYER_REDO);
	    while (next_l) {
		struct MyLayer *l2 = (struct MyLayer *)find_uniq(&mli->layer_list, get_long (next_l + 24));
		uae_u16 flags = get_word (l2->amigaos_layer + 32);
		if (l2->bounds.MinX <= l->bounds.MaxX && l->bounds.MinX <= l2->bounds.MaxX
		    && l2->bounds.MinY <= l->bounds.MaxY && l->bounds.MinY <= l2->bounds.MaxY)
		    put_word (l2->amigaos_layer + 32, flags | LAYER_REDO);
		if ((flags & (LAYER_REDO|LAYER_CLUELESS)) == 0) {
		    free_rectlist(&l->obscured);
		    free_rectlist(&l->visible);
		    free_bandlist(&l->big_bands);
		    free_bandlist(&l->small_bands);
		}
		next_l = get_long (next_l);
	    }
	}
	current_l = get_long (current_l);
    }
}

static void redo_layers(struct MyLayerInfo *mli, uaecptr bm)
{
    uaecptr current_l;
    struct RectList tmp_rl;

    propagate_clueless_redo(mli);
    current_l = get_long (mli->amigaos_linfo);

    while (current_l) {
	struct MyLayer *l = (struct MyLayer *)find_uniq(&mli->layer_list, get_long (current_l + 24));
	uae_u16 flags = get_word (l->amigaos_layer + 32);
	if ((flags & LAYER_REDO) != 0) {
	    uaecptr next_l = get_long (current_l+4);
	    int have_rects = 0;

	    init_rectlist(&l->obscured);
	    init_bandlist(&l->big_bands);
	    add_rect_to_bands(&l->big_bands, &l->bounds);

	    while (next_l) {
		struct MyLayer *l2 = (struct MyLayer *)find_uniq(&mli->layer_list, get_long (next_l + 24));
		if (l2->visible.bounds.MinX <= l->bounds.MaxX && l->bounds.MinX <= l2->visible.bounds.MaxX
		    && l2->visible.bounds.MinY <= l->bounds.MaxY && l->bounds.MinY <= l2->visible.bounds.MaxY
		    && !rect_in_region (&l->obscured, &l2->visible.bounds))
		{
		    add_rect_to_bands(&l->big_bands, &l2->visible.bounds);
		    add_rect(&l->obscured, l2->visible.bounds);
		    have_rects++;
		}
		next_l = get_long (next_l+4);
	    }
	    init_rectlist(&l->visible);
	    init_rectlist(&tmp_rl);
	    add_rect (&tmp_rl, l->bounds);

	    region_splitrects_band(&l->obscured, &l->big_bands);
	    region_splitrects_band(&tmp_rl, &l->big_bands);
	    region_do_ClearRegionRegion(&l->obscured, &tmp_rl, &l->visible);
	    flags |= LAYER_CR_CHANGED;
	}
	put_word (l->amigaos_layer + 32, flags & ~(LAYER_CLUELESS|LAYER_REDO));
	current_l = get_long (current_l);
    }
}

static struct MyLayer *LY_NewLayer(struct MyLayerInfo *mli, int x0, int x1, int y0, int y1,
				   uae_u16 flags, uaecptr bm, uaecptr sbm)
{
    struct MyLayer *l = (struct MyLayer *)malloc(sizeof (struct MyLayer));
    uaecptr layer = amiga_malloc(LAYER_SIZE);
    memset (get_real_address (layer), 0, LAYER_SIZE);
    l->amigaos_layer = layer;

    put_word (layer + 16, x0); /* bounds */
    put_word (layer + 18, y0);
    put_word (layer + 20, x1);
    put_word (layer + 22, y1);
    put_word (layer + 30, flags | LAYER_CLUELESS);
    put_long (layer + 32, flags & 4 ? sbm : 0); /* ClipRect */
    put_long (layer + 68, mli->amigaos_linfo);
    m68k_areg (regs, 0) = layer + 72; CallLib (get_long (4), -558); /* InitSemaphore() */
    add_uniq(&mli->layer_list, &l->head, layer + 24);
    l->mli = mli;

    l->bounds.MinX = x0;
    l->bounds.MaxX = x1;
    l->bounds.MinY = y0;
    l->bounds.MaxY = y1;
    return l;
}

static void LY_DeleteLayer(uaecptr layer)
{
    uaecptr cr;
    struct MyLayer *l = (struct MyLayer *)find_and_rem_uniq(&l->mli->layer_list, get_long (layer + 24));
    /* Free ClipRects */
    while ((cr = get_long (l->amigaos_layer + 8))) {
	put_long (l->amigaos_layer + 8, get_long (cr));
	amiga_free(cr, CLIPRECT_SIZE);
    }
    amiga_free (l->amigaos_layer, LAYER_SIZE);
    free(l);
}

static uaecptr find_behindlayer_position(uaecptr li, uae_u16 flags)
{
    uaecptr where = li;
    for (;;) {
	uaecptr other = get_long (where);
	/* End of list? */
	if (other == 0)
	    break;
	/* Backdrop? */
	if ((get_word (other + 30) & 0x40) > (flags & 0x40))
	    break;
	where = other;
    }
    return where;
}

static uaecptr LY_CreateLayer(uaecptr li, int x0, int x1, int y0, int y1,
			      uae_u16 flags, uaecptr bm, uaecptr sbm, uaecptr where)
{
    struct MyLayerInfo *mli = (struct MyLayerInfo *)find_uniq(&MyLayerInfo_list, get_long (li + 84));
    struct MyLayer *l;

    LY_LockLayerInfo(li);

    l = LY_NewLayer(mli, x0, x1, y0, y1, flags, bm, sbm);
    /* Chain into list */
    put_long (l->amigaos_layer, get_long (where));
    put_long (l->amigaos_layer + 4, where == li ? 0 : where);
    if (get_long (where) != 0)
	put_long (get_long (where) + 4, l->amigaos_layer);
    put_long (where, l->amigaos_layer);
    redo_layers(mli, bm);
    build_cliprects(l);
    LY_UnlockLayerInfo(li);
    return l->amigaos_layer;
}

static void LY_DisposeLayerInfo(uaecptr li)
{
    LY_ThinLayerInfo(li);
    amiga_free(li, LINFO_SIZE);
}

static uae_u32 layers_NewLayerInfo(void)
{
    uaecptr li = amiga_malloc(LINFO_SIZE);
    LY_InitLayers(li);
    LY_FattenLayerInfo(li);
    return li;
}
static uae_u32 layers_InitLayers(void) { LY_InitLayers (m68k_areg (regs, 0)); return 0; }
static uae_u32 layers_DisposeLayerInfo(void) { LY_DisposeLayerInfo (m68k_areg (regs, 0)); return 0; }

static uae_u32 layers_FattenLayerInfo(void) { LY_FattenLayerInfo(m68k_areg (regs, 0)); return 0; }
static uae_u32 layers_ThinLayerInfo(void) { LY_ThinLayerInfo(m68k_areg (regs, 0)); return 0; }

static uae_u32 layers_CreateUpfrontLayer(void)
{
    return LY_CreateLayer(m68k_areg (regs, 0), (uae_s32)m68k_dreg (regs, 0),
			  (uae_s32)m68k_dreg (regs, 1), (uae_s32)m68k_dreg (regs, 2),
			  (uae_s32)m68k_dreg (regs, 3),
			  m68k_dreg (regs, 4),
			  m68k_areg (regs, 1), m68k_areg (regs, 2), m68k_areg (regs, 0));
}
static uae_u32 layers_CreateBehindLayer(void)
{
    return LY_CreateLayer(m68k_areg (regs, 0), (uae_s32)m68k_dreg (regs, 0),
			  (uae_s32)m68k_dreg (regs, 1), (uae_s32)m68k_dreg (regs, 2),
			  (uae_s32)m68k_dreg (regs, 3),
			  m68k_dreg (regs, 4),
			  m68k_areg (regs, 1), m68k_areg (regs, 2),
			  find_behindlayer_position (m68k_areg (regs, 0), m68k_dreg (regs, 4)));
}
static uae_u32 layers_DeleteLayer(void) { LY_DeleteLayer (m68k_areg (regs, 1)); return 0; }

static void LY_LockLayer1(uaecptr layer)
{
    uaecptr li = get_long (layer + 68);
    struct MyLayerInfo *mli = (struct MyLayerInfo *)find_uniq (&MyLayerInfo_list, get_long (li + 84));
    struct MyLayer *l = (struct MyLayer *)find_uniq (&mli->layer_list, get_long (layer + 24));

    LY_LockLayer(layer);
    build_cliprects (l);
}
static uae_u32 LY_TryLockLayer1(uaecptr layer)
{
    uaecptr li = get_long (layer + 68);
    struct MyLayerInfo *mli = (struct MyLayerInfo *)find_uniq (&MyLayerInfo_list, get_long (li + 84));
    struct MyLayer *l = (struct MyLayer *)find_uniq (&mli->layer_list, get_long (layer + 24));

    if (!LY_TryLockLayer(layer))
	return 0;
    build_cliprects (l);
    return 1;
}
static uae_u32 gfx_TryLockLayer(void) { return LY_TryLockLayer1 (m68k_areg (regs, 5));  }
static uae_u32 gfx_LockLayer(void) { LY_LockLayer1 (m68k_areg (regs, 5)); return 0; }
static uae_u32 gfx_UnlockLayer(void) { LY_UnlockLayer(m68k_areg (regs, 5)); return 0; }
static uae_u32 layers_LockLayer(void) { LY_LockLayer1 (m68k_areg (regs, 1)); return 0; }
static uae_u32 layers_LockLayers(void) { LY_LockLayers(m68k_areg (regs, 0)); return 0; }
static uae_u32 layers_LockLayerInfo(void) { LY_LockLayerInfo(m68k_areg (regs, 0)); return 0; }
static uae_u32 layers_UnlockLayer(void) { LY_UnlockLayer(m68k_areg (regs, 0)); return 0; }
static uae_u32 layers_UnlockLayers(void) { LY_UnlockLayers(m68k_areg (regs, 0)); return 0; }
static uae_u32 layers_UnlockLayerInfo(void) { LY_UnlockLayerInfo(m68k_areg (regs, 0)); return 0; }

static uae_u32 layers_ScrollLayer(void)
{
    abort();
}

static uae_u32 layers_SizeLayer(void)
{
    abort();
}

static uae_u32 layers_MoveLayer(void)
{
    abort();
}

static uae_u32 layers_UpfrontLayer(void)
{
    abort();
}

static uae_u32 layers_BehindLayer(void)
{
    abort();
}

static uae_u32 layers_MoveLayerInFrontOf(void)
{
    abort();
}

static uae_u32 layers_BeginUpdate(void)
{
    return 1;
}

static uae_u32 layers_EndUpdate(void)
{
    return 0;
}

static uae_u32 layers_WhichLayer(void)
{
    abort();
}

static uae_u32 layers_InstallClipRegion(void)
{
    return 0;
}

static uae_u32 layers_SwapBitsRastPortClipRect(void)
{
    abort();
}

/*
 *  Initialization
 */
static uae_u32 gfxlib_init(void)
{
    uae_u32 old_arr;
    uaecptr sysbase=m68k_areg (regs, 6);
    int i=0;

    /* Install new routines */
    m68k_dreg (regs, 0)=0;
    m68k_areg (regs, 1)=gfxlibname;
    gfxbase=CallLib (sysbase, -408);  /* OpenLibrary */
    m68k_dreg (regs, 0)=0;
    m68k_areg (regs, 1)=layerslibname;
    layersbase=CallLib (sysbase, -408);  /* OpenLibrary */

    libemu_InstallFunctionFlags(gfxl_WritePixel, gfxbase, -324, TRAPFLAG_EXTRA_STACK, "");
    libemu_InstallFunctionFlags(gfxl_BltClear, gfxbase, -300, 0, "");
    libemu_InstallFunctionFlags(gfxl_AndRegionRegion, gfxbase, -624, TRAPFLAG_EXTRA_STACK, "");
    libemu_InstallFunctionFlags(gfxl_OrRegionRegion, gfxbase, -612, TRAPFLAG_EXTRA_STACK, "");
    libemu_InstallFunctionFlags(gfxl_XorRegionRegion, gfxbase, -618, TRAPFLAG_EXTRA_STACK, "");
    libemu_InstallFunctionFlags(gfxl_AndRectRegion, gfxbase, -504, TRAPFLAG_EXTRA_STACK, "");
    libemu_InstallFunctionFlags(gfxl_OrRectRegion, gfxbase, -510, TRAPFLAG_EXTRA_STACK, "");
    libemu_InstallFunctionFlags(gfxl_XorRectRegion, gfxbase, -558, TRAPFLAG_EXTRA_STACK, "");
    libemu_InstallFunctionFlags(gfxl_ClearRectRegion, gfxbase, -522, TRAPFLAG_EXTRA_STACK, "");

#if 0
#define MAYBE_FUNCTION(a) NULL
#else
#define MAYBE_FUNCTION(a) (a)
#endif
#if 0
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(gfx_TryLockLayer), gfxbase, -654, TRAPFLAG_EXTRA_STACK|TRAPFLAG_NO_RETVAL, "AttemptLockLayerRom");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(gfx_LockLayer), gfxbase, -432, TRAPFLAG_EXTRA_STACK|TRAPFLAG_NO_RETVAL, "LockLayerRom");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(gfx_UnlockLayer), gfxbase, -438, TRAPFLAG_EXTRA_STACK|TRAPFLAG_NO_RETVAL, "UnlockLayerRom");

    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_InitLayers), layersbase, -30, TRAPFLAG_EXTRA_STACK, "InitLayers");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_CreateUpfrontLayer), layersbase, -36, TRAPFLAG_EXTRA_STACK, "CreateUpfrontLayer");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_CreateBehindLayer), layersbase, -42, TRAPFLAG_EXTRA_STACK, "CreateBehindLayer");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_UpfrontLayer), layersbase, -48, TRAPFLAG_EXTRA_STACK, "UpfrontLayer");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_BehindLayer), layersbase, -54, TRAPFLAG_EXTRA_STACK, "BehindLayer");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_MoveLayer), layersbase, -60, TRAPFLAG_EXTRA_STACK, "MoveLayer");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_SizeLayer), layersbase, -66, TRAPFLAG_EXTRA_STACK, "SizeLayer");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_ScrollLayer), layersbase, -72, TRAPFLAG_EXTRA_STACK, "ScrollLayer");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_BeginUpdate), layersbase, -78, TRAPFLAG_EXTRA_STACK, "BeginUpdate");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_EndUpdate), layersbase, -84, TRAPFLAG_EXTRA_STACK, "EndUpdate");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_DeleteLayer), layersbase, -90, TRAPFLAG_EXTRA_STACK, "DeleteLayer");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_LockLayer), layersbase, -96, TRAPFLAG_EXTRA_STACK, "LockLayer");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_UnlockLayer), layersbase, -102, TRAPFLAG_EXTRA_STACK, "UnlockLayer");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_LockLayers), layersbase, -108, TRAPFLAG_EXTRA_STACK, "LockLayers");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_UnlockLayers), layersbase, -114, TRAPFLAG_EXTRA_STACK, "UnlockLayers");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_LockLayerInfo), layersbase, -120, TRAPFLAG_EXTRA_STACK, "LockLayerInfo");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_SwapBitsRastPortClipRect), layersbase, -126, TRAPFLAG_EXTRA_STACK, "SwapBitsRastPortClipRect");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_WhichLayer), layersbase, -132, TRAPFLAG_EXTRA_STACK, "WhichLayer");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_UnlockLayerInfo), layersbase, -138, TRAPFLAG_EXTRA_STACK, "UnlockLayerInfo");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_NewLayerInfo), layersbase, -144, TRAPFLAG_EXTRA_STACK, "NewLayerInfo");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_DisposeLayerInfo), layersbase, -150, TRAPFLAG_EXTRA_STACK, "DisposeLayerInfo");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_FattenLayerInfo), layersbase, -156, TRAPFLAG_EXTRA_STACK, "FattenLayerInfo");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_ThinLayerInfo), layersbase, -162, TRAPFLAG_EXTRA_STACK, "ThinLayerInfo");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_MoveLayerInFrontOf), layersbase, -168, TRAPFLAG_EXTRA_STACK, "MoveLayerInFrontOf");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_InstallClipRegion), layersbase, -174, TRAPFLAG_EXTRA_STACK, "InstallClipRegion");
#if 0
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_), layersbase, -180, TRAPFLAG_EXTRA_STACK, "MoveSizeLayer");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_), layersbase, -186, TRAPFLAG_EXTRA_STACK, "CreateUpfrontHookLayer");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_), layersbase, -192, TRAPFLAG_EXTRA_STACK, "CreateBehindHookLayer");
    libemu_InstallFunctionFlags(MAYBE_FUNCTION(layers_), layersbase, -198, TRAPFLAG_EXTRA_STACK, "InstallLayerHook");
#endif
#endif
    return 0;
}

/*
 *  Install the gfx-library-replacement
 */
void gfxlib_install(void)
{
    uae_u32 begin, end, resname, resid;
    int i;

    if (! currprefs.use_gfxlib)
	return;

    write_log (_T("Warning: you enabled the graphics.library replacement with -g\n")
	     "This may be buggy right now, and will not speed things up much.\n");

    resname = ds ("UAEgfxlib.resource");
    resid = ds ("UAE gfxlib 0.1");

    gfxlibname = ds ("graphics.library");
    layerslibname = ds ("layers.library");

    begin = here();
    dw(0x4AFC);             /* RTC_MATCHuae_s16 */
    dl(begin);              /* our start address */
    dl(0);                  /* Continue scan here */
    dw(0x0101);             /* RTF_COLDSTART; Version 1 */
    dw(0x0805);             /* NT_RESOURCE; pri 5 */
    dl(resname);            /* name */
    dl(resid);              /* ID */
    dl(here() + 4);         /* Init area: directly after this */

    calltrap(deftrap2(gfxlib_init, TRAPFLAG_EXTRA_STACK, "")); dw(RTS);

    end = here();
    org(begin + 6);
    dl(end);

    org(end);
}
#else

void gfxlib_install (void)
{
}

#endif
