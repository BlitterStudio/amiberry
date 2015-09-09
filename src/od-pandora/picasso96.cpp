/*
 * UAE - The U*nix Amiga Emulator
 *
 * Picasso96 Support Module
 *
 * Copyright 1997-2001 Brian King <Brian_King@Mitel.com, Brian_King@Cloanto.com>
 * Copyright 2000-2001 Bernd Roesch
 * Copyright 2003-2005 Richard Drummond
 *
 * Theory of operation:
 * On the Amiga side, a Picasso card consists mainly of a memory area that
 * contains the frame buffer.  On the UAE side, we allocate a block of memory
 * that will hold the frame buffer.  This block is in normal memory, it is
 * never directly on the graphics card. All graphics operations, which are
 * mainly reads and writes into this block and a few basic operations like
 * filling a rectangle, operate on this block of memory.
 * Since the memory is not on the graphics card, some work must be done to
 * synchronize the display with the data in the Picasso frame buffer.  There
 * are various ways to do this. One possibility is to allocate a second
 * buffer of the same size, and perform all write operations twice.  Since
 * we never read from the second buffer, it can actually be placed in video
 * memory.  The X11 driver could be made to use the Picasso frame buffer as
 * the data buffer of an XImage, which could then be XPutImage()d from time
 * to time.  Another possibility is to translate all Picasso accesses into
 * Xlib (or GDI, or whatever your graphics system is) calls.  This possibility
 * is a bit tricky, since there is a risk of generating very many single pixel
 * accesses which may be rather slow.
 *
 * TODO:
 * - we want to add a manual switch to override SetSwitch for hardware banging
 *   programs started from a Picasso workbench.
 */

#include "sysconfig.h"
#include "sysdeps.h"

#include "config.h"
#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "xwin.h"
#include "savestate.h"
#include "autoconf.h"
#include "traps.h"

#ifdef PICASSO96

#include "picasso96.h"
#include <SDL.h>

int have_done_picasso; /* For the JIT compiler */
static int p96syncrate;

int palette_changed;

static uae_u32 REGPARAM2 gfxmem_lget (uaecptr) REGPARAM;
static uae_u32 REGPARAM2 gfxmem_wget (uaecptr) REGPARAM;
static uae_u32 REGPARAM2 gfxmem_bget (uaecptr) REGPARAM;
static void REGPARAM2 gfxmem_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM2 gfxmem_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM2 gfxmem_bput (uaecptr, uae_u32) REGPARAM;
static int REGPARAM2 gfxmem_check (uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *REGPARAM2 gfxmem_xlate (uaecptr addr) REGPARAM;

static uae_u8 all_ones_bitmap, all_zeros_bitmap; /* yuk */

struct picasso96_state_struct picasso96_state;
struct picasso_vidbuf_description picasso_vidinfo;
static struct PicassoResolution *newmodes = NULL;

/* These are the maximum resolutions... They are filled in by GetSupportedResolutions() */
/* have to fill this in, otherwise problems occur on the Amiga side P96 s/w which expects
/* data here. */
static struct ScreenResolution planar = { 320, 240 };
static struct ScreenResolution chunky = { 640, 480 };
static struct ScreenResolution hicolour = { 640, 480 };
static struct ScreenResolution truecolour = { 640, 480 };
static struct ScreenResolution alphacolour = { 640, 480 };

#define UAE_RTG_LIBRARY_VERSION 40
#define UAE_RTG_LIBRARY_REVISION 3994
static void checkrtglibrary(void)
{
  uae_u32 v;
  static int checked = 0;

  if (checked)
	  return;
  v = get_long (4); // execbase
  v += 378; // liblist
  while ((v = get_long(v))) {
  	uae_u32 v2 = get_long(v + 10); // name
  	uae_u8 *p;
  	addrbank *b = &get_mem_bank(v2);
  	if (!b || !b->check (v2, 12))
	    continue;
  	p = b->xlateaddr(v2);
  	if (!memcmp(p, "rtg.library\0", 12)) {
	    uae_u16 ver = get_word(v + 20);
	    uae_u16 rev = get_word(v + 22);
	    if (ver * 10000 + rev < UAE_RTG_LIBRARY_VERSION * 10000 + UAE_RTG_LIBRARY_REVISION) {
    		char msg[2000];
        write_log("installed rtg.library: %d.%d, required: %d.%d\n", ver, rev, UAE_RTG_LIBRARY_VERSION, UAE_RTG_LIBRARY_REVISION);
	    } else {
    		write_log("P96: rtg.library %d.%d detected\n", ver, rev);
	    }
	    checked = 1;
  	}
  }
}

struct PicassoResolution DisplayModes[MAX_PICASSO_MODES];

static int mode_count = 0;

static uae_u32 p2ctab[256][2];
static int set_gc_called = 0, init_picasso_screen_called = 0;


static uae_u8 GetBytesPerPixel (uae_u32 RGBfmt)
{
    static int bFailure = 0;

    switch (RGBfmt) {
    case RGBFB_CLUT:
	return 1;

    case RGBFB_A8R8G8B8:
    case RGBFB_A8B8G8R8:
    case RGBFB_R8G8B8A8:
    case RGBFB_B8G8R8A8:
	return 4;

    case RGBFB_B8G8R8:
    case RGBFB_R8G8B8:
	return 3;

    case RGBFB_R5G5B5:
    case RGBFB_R5G6B5:
    case RGBFB_R5G6B5PC:
    case RGBFB_R5G5B5PC:
    case RGBFB_B5G6R5PC:
    case RGBFB_B5G5R5PC:
	return 2;
    default:
	write_log ("ERROR - GetBytesPerPixel() was unsuccessful with 0x%x?!\n", RGBfmt);
	if( !bFailure )
	{
	    bFailure = 1;
	    return GetBytesPerPixel(picasso_vidinfo.rgbformat);
	}
	else
	{
	    abort();
	}
    }
	return 0;
}

/*
 * Amiga <-> native structure conversion functions
 */

static int CopyRenderInfoStructureA2U (uaecptr amigamemptr, struct RenderInfo *ri)
{
    uaecptr memp = get_long (amigamemptr + PSSO_RenderInfo_Memory);

    if (valid_address (memp, PSSO_RenderInfo_sizeof)) {
	ri->Memory = get_real_address (memp);
	ri->BytesPerRow = get_word (amigamemptr + PSSO_RenderInfo_BytesPerRow);
	ri->RGBFormat = (RGBFTYPE)get_long (amigamemptr + PSSO_RenderInfo_RGBFormat);
	return 1;
    }
    write_log ("ERROR - Invalid RenderInfo memory area...\n");
    return 0;
}

static int CopyPatternStructureA2U (uaecptr amigamemptr, struct Pattern *pattern)
{
    uaecptr memp = get_long (amigamemptr + PSSO_Pattern_Memory);
    if (valid_address (memp, PSSO_Pattern_sizeof)) {
	pattern->Memory = (char *)get_real_address (memp);
	pattern->XOffset = get_word (amigamemptr + PSSO_Pattern_XOffset);
	pattern->YOffset = get_word (amigamemptr + PSSO_Pattern_YOffset);
	pattern->FgPen = get_long (amigamemptr + PSSO_Pattern_FgPen);
	pattern->BgPen = get_long (amigamemptr + PSSO_Pattern_BgPen);
	pattern->Size = get_byte (amigamemptr + PSSO_Pattern_Size);
	pattern->DrawMode = get_byte (amigamemptr + PSSO_Pattern_DrawMode);
	return 1;
    }
    write_log ("ERROR - Invalid Pattern memory area...\n");
    return 0;
}

static void CopyColorIndexMappingA2U (uaecptr amigamemptr, struct ColorIndexMapping *cim)
{
    int i;
    cim->ColorMask = get_long (amigamemptr);
    for (i = 0; i < 256; i++, amigamemptr += 4)
	cim->Colors[i] = get_long (amigamemptr + 4);
}

static int CopyBitMapStructureA2U (uaecptr amigamemptr, struct BitMap *bm)
{
    int i;

    bm->BytesPerRow = get_word (amigamemptr + PSSO_BitMap_BytesPerRow);
    bm->Rows = get_word (amigamemptr + PSSO_BitMap_Rows);
    bm->Flags = get_byte (amigamemptr + PSSO_BitMap_Flags);
    bm->Depth = get_byte (amigamemptr + PSSO_BitMap_Depth);
  
    /* ARGH - why is THIS happening? */
    if( bm->Depth > 8 )
	bm->Depth = 8;

    for (i = 0; i < bm->Depth; i++) {
	uaecptr plane = get_long (amigamemptr + PSSO_BitMap_Planes + i * 4);
	switch (plane) {
	case 0:
	    bm->Planes[i] = &all_zeros_bitmap;
	    break;
	case 0xFFFFFFFF:
	    bm->Planes[i] = &all_ones_bitmap;
	    break;
	default:
	    if (valid_address (plane, bm->BytesPerRow * bm->Rows))
		bm->Planes[i] = get_real_address (plane);
	    else
		return 0;
	    break;
	}
    }
    return 1;
}

static int CopyTemplateStructureA2U (uaecptr amigamemptr, struct Template *tmpl)
{
    uaecptr memp = get_long (amigamemptr + PSSO_Template_Memory);

    if (valid_address (memp, sizeof(struct Template) )) {
	tmpl->Memory = (char *)get_real_address (memp);
	tmpl->BytesPerRow = get_word (amigamemptr + PSSO_Template_BytesPerRow);
	tmpl->XOffset = get_byte (amigamemptr + PSSO_Template_XOffset);
	tmpl->DrawMode = get_byte (amigamemptr + PSSO_Template_DrawMode);
	tmpl->FgPen = get_long (amigamemptr + PSSO_Template_FgPen);
	tmpl->BgPen = get_long (amigamemptr + PSSO_Template_BgPen);
	return 1;
    }
    write_log ("ERROR - Invalid Template memory area...\n");
    return 0;
}

/* list is Amiga address of list, in correct endian format for UAE
 * node is Amiga address of node, in correct endian format for UAE */
static void AmigaListAddTail (uaecptr l, uaecptr n)
{
    put_long (n + 0, l + 4); // n->ln_Succ = (struct Node *)&l->lh_Tail;
    put_long (n + 4, get_long (l + 8)); // n->ln_Pred = l->lh_TailPred;
    put_long (get_long (l + 8) + 0, n); // l->lh_TailPred->ln_Succ = n;
    put_long (l + 8, n); // l->lh_TailPred = n;
}

/*
 * Functions to perform an action on the real screen
 */

/*
 * Fill a rectangle on the screen.  src points to the start of a line of the
 * filled rectangle in the frame buffer; it can be used as a memcpy source if
 * there is no OS specific function to fill the rectangle.
 */

static void do_fillrect (uae_u8 *src, unsigned int x, unsigned int y, 
        unsigned int width, unsigned int height,
			  uae_u32 pen, int Bpp, RGBFTYPE rgbtype)
{
  uae_u8 *dst;

  /* Try OS specific fillrect function here; and return if successful.  Make sure we adjust for
   * the pen values if we're doing 8-bit display-emulation on a 16-bit or higher screen. */
  if( picasso_vidinfo.rgbformat == picasso96_state.RGBFormat )
  {
   	if( DX_Fill( x, y, width, height, pen, rgbtype ) )
      return;
  }
  else
  {   
    if(DX_Fill(x, y, width, height, picasso_vidinfo.clut[pen], rgbtype))
      return;
  }

  if (y + height > picasso_vidinfo.height)
  	height = picasso_vidinfo.height - y;
  if (x + width > picasso_vidinfo.width)
	  width = picasso_vidinfo.width - x;

  DX_Invalidate (x, y, width, height);
  if (!picasso_vidinfo.extra_mem)
   	return;

  dst = gfx_lock_picasso ();

  if (!dst)
	  goto out;

  width *= Bpp;
  dst += y * picasso_vidinfo.rowbytes + x * picasso_vidinfo.pixbytes;
  if (picasso_vidinfo.rgbformat == picasso96_state.RGBFormat) 
  {
	  if (Bpp == 1) 
    {
	    while (height-- > 0) 
      {
		    memset (dst, pen, width);
		    dst += picasso_vidinfo.rowbytes;
	    }
	  } 
    else 
    {
      // Set first line
      if (Bpp == 2) {
        for(int i=0; i<width; i += 2)
          *(uae_u16*)(dst + i) = pen;
      } else {
        for(int i=0; i<width; i += 4)
          *(uae_u32*)(dst + i) = pen;
      }
      --height; // First line already done
      dst += picasso_vidinfo.rowbytes;
      
	    while (height-- > 0) 
      {
		    memcpy (dst, src, width);
		    dst += picasso_vidinfo.rowbytes;
	    }
	  }
  } 
  else 
  {
	  int psiz = GetBytesPerPixel (picasso_vidinfo.rgbformat);
	  if (picasso96_state.RGBFormat != RGBFB_CHUNKY)
	  {
	    write_log ("ERROR - do_fillrect() failure1 (%d)\n", picasso96_state.RGBFormat);
	    abort ();
	  }

  	while (height-- > 0) 
    {
	    unsigned int i;
	    switch (psiz) 
      {
	      case 2:
		      for (i = 0; i < width; i++)
		        *((uae_u16 *) dst + i) = picasso_vidinfo.clut[src[i]];
		      break;
	      case 4:
		      for (i = 0; i < width; i++)
		        *((uae_u32 *) dst + i) = picasso_vidinfo.clut[src[i]];
		      break;
	      default:
      		write_log ("ERROR - do_fillrect() failure2 (%d), psize\n");
		      abort ();
		      break;
	    }
	    dst += picasso_vidinfo.rowbytes;
	  }
  }
out:
  gfx_unlock_picasso ();
}

/*
 * This routine modifies the real screen buffer after a blit has been
 * performed in the save area. If can_do_blit is nonzero, the blit can
 * be performed within the real screen buffer; otherwise, this routine
 * must do it by hand using the data in the frame-buffer, calculated using
 * the RenderInfo data and our coordinates.
 */

static void do_blit (struct RenderInfo *ri, int Bpp, 
      unsigned int srcx, unsigned int srcy, unsigned int dstx, unsigned int dsty, 
      unsigned int width, unsigned int height, BLIT_OPCODE opcode, int can_do_blit)
{
  uae_u8 *dstp, *srcp;

  dstx = dstx - picasso96_state.XOffset;
  dsty = dsty - picasso96_state.YOffset;
  if ((int)dstx <= 0) {
	  srcx=srcx-dstx;
	  dstx = 0;
  }
  if ((int)dsty <= 0) {
	  srcy=srcy-dsty;
	  dsty = 0;
  }

  /* Is our x/y origin on-screen? */
  if( dsty >= picasso_vidinfo.height )
	  return;
  if( dstx >= picasso_vidinfo.width )
	  return;

  /* Is our area in-range? */
  if (dsty + height >= picasso_vidinfo.height)
  	height = picasso_vidinfo.height - dsty;
  if (dstx + width >= picasso_vidinfo.width)
	  width = picasso_vidinfo.width - dstx;

  srcp = ri->Memory + srcx*Bpp + srcy*ri->BytesPerRow;

  DX_Invalidate (dstx, dsty, width, height);
  if (!picasso_vidinfo.extra_mem)
  {
    return;
  }

  dstp = gfx_lock_picasso ();
  if (dstp == 0)
  {
	  goto out;
  }
 
  /* The areas can't overlap: the source is always in the Picasso frame buffer,
   * and the destination is a different buffer owned by the graphics code.  */
  dstp += dsty * picasso_vidinfo.rowbytes + dstx * picasso_vidinfo.pixbytes;

  if (picasso_vidinfo.rgbformat == picasso96_state.RGBFormat) 
  {
  	while (height-- > 0) 
    {
	    switch (Bpp)
	    {
	      case 2:
	        pixcpy_swap16((uae_u16*)dstp, (uae_u16*)srcp, width);
	        break;
	      case 4:
	        pixcpy_swap32((uae_u32*)dstp, (uae_u32*)srcp, width);
	        break;
	      default:  
  	      memcpy (dstp, srcp, width);
	    }
	    srcp += ri->BytesPerRow;
	    dstp += picasso_vidinfo.rowbytes;
	  }
  } 
  else 
  {
	  int psiz = GetBytesPerPixel (picasso_vidinfo.rgbformat);
	  if (picasso96_state.RGBFormat != RGBFB_CHUNKY)
    {
	    write_log ("ERROR: do_blit() failure, %d!\n", picasso96_state.RGBFormat);
	    abort ();
	  }
	  while (height-- > 0) 
    {
	    unsigned int i;
	    switch (psiz) 
      {
	      case 2:
		      for (i = 0; i < width; i++)
		        *((uae_u16 *) dstp + i) = picasso_vidinfo.clut[srcp[i]];
		      break;
	      case 4:
		      for (i = 0; i < width; i++)
		        *((uae_u32 *) dstp + i) = picasso_vidinfo.clut[srcp[i]];
		      break;
	      default:
      		write_log ("ERROR - do_blit() failure2, %d!\n", psiz);
		      abort ();
		      break;
	    }
	    srcp += ri->BytesPerRow;
	    dstp += picasso_vidinfo.rowbytes;
	  }
  }
out:
  gfx_unlock_picasso ();
}

/*
* Invert a rectangle on the screen.  a render-info is given,
* so that do_blit can be used if
* there is no OS specific function to invert the rectangle.
 */

static void do_invertrect (struct RenderInfo *ri, int Bpp, int x, int y, int width, int height)
{
  do_blit (ri, Bpp, x, y, x, y, width, height, BLIT_SRC, 0);
}

static uaecptr wgfx_linestart;
static uaecptr wgfx_lineend;
static uaecptr wgfx_min, wgfx_max;
static unsigned long wgfx_y;

static void wgfx_do_flushline (void)
{
  uae_u8 *src, *dstp;

  /* Mark these lines as "dirty" */
  DX_Invalidate (0, wgfx_y, picasso_vidinfo.width, 1);

  if (!picasso_vidinfo.extra_mem) /* The "out" will flush the dirty lines directly */
  	goto out;

  dstp = gfx_lock_picasso ();
  if (dstp == 0)
  	goto out;

  src = gfxmemory + wgfx_min;

  if (picasso_vidinfo.rgbformat == picasso96_state.RGBFormat) 
  {
  	dstp += wgfx_y * picasso_vidinfo.rowbytes + wgfx_min - wgfx_linestart;
//  	memcpy (dstp, src, wgfx_max - wgfx_min);
    switch (GetBytesPerPixel(picasso_vidinfo.rgbformat))
    {
      case 2:
        pixcpy_swap16((uae_u16*)dstp, (uae_u16*)src, wgfx_max - wgfx_min);
        break;
      case 4:
        pixcpy_swap32((uae_u32*)dstp, (uae_u32*)src, wgfx_max - wgfx_min);
        break;
      default:  
	      memcpy (dstp, src, wgfx_max - wgfx_min);
    }
  } 
  else 
  {
    int width = wgfx_max - wgfx_min;
    int i;
    int psiz = GetBytesPerPixel (picasso_vidinfo.rgbformat);

	  if (picasso96_state.RGBFormat != RGBFB_CHUNKY)
	  {
	    write_log ("ERROR - wgfx_do_flushline() failure, %d!\n", picasso96_state.RGBFormat);
	    abort ();
	  }

	  dstp += wgfx_y * picasso_vidinfo.rowbytes + (wgfx_min - wgfx_linestart) * psiz;
	  switch (psiz) {
	    case 2:
	      for (i = 0; i < width; i++)
		      *((uae_u16 *) dstp + i) = picasso_vidinfo.clut[src[i]];
	      break;
	    case 4:
	      for (i = 0; i < width; i++)
		      *((uae_u32 *) dstp + i) = picasso_vidinfo.clut[src[i]];
	      break;
	    default:
  	    write_log ("ERROR - wgfx_do_flushline() failure2, %d!\n", psiz);
	      abort ();
	      break;
	  }
  }

out:
  gfx_unlock_picasso ();
  wgfx_linestart = 0xFFFFFFFF;
}

STATIC_INLINE void wgfx_flushline (void)
{
  if (wgfx_linestart == 0xFFFFFFFF || !picasso_on)
  	return;
  wgfx_do_flushline ();
}

static int renderinfo_is_current_screen (struct RenderInfo *ri)
{
  if (!picasso_on)
  	return 0;
  if (ri->Memory != gfxmemory + (picasso96_state.Address - gfxmem_start))
	  return 0;

  return 1;
}

/*
* Fill a rectangle in the screen.
*/
STATIC_INLINE void do_fillrect_frame_buffer (struct RenderInfo *ri, int X, int Y, 
  int Width, int Height, uae_u32 Pen, int Bpp, 
  RGBFTYPE RGBFormat)
{
  int cols;
  uae_u8 *start, *oldstart;
  uae_u8 *src, *dst;
  int lines;

  /* Do our virtual frame-buffer memory.  First, we do a single line fill by hand */
  oldstart = start = src = ri->Memory + X * Bpp + Y * ri->BytesPerRow;

  switch (Bpp) 
  {
	  case 1:
	    memset (start, Pen, Width);
	    break;
	  case 2:
	    for (cols = 0; cols < Width; cols++) 
      {
		    do_put_mem_word ((uae_u16 *)start, (uae_u16)Pen);
		    start += 2;
	    }
	    break;
	  case 3:
	    for (cols = 0; cols < Width; cols++) 
      {
		    do_put_mem_byte (start, (uae_u8)Pen);
		    start++;
		    *(uae_u16 *)(start) = (Pen & 0x00FFFF00) >> 8;
		    start+=2;
	    }
	    break;
	  case 4:
	    for (cols = 0; cols < Width; cols++) 
      {
	      /**start = Pen; */
		    do_put_mem_long ((uae_u32 *)start, Pen);
		    start += 4;
	    }
	    break;
  }

  src = oldstart;
  dst = src + ri->BytesPerRow;

  /* next, we do the remaining line fills via memcpy() for > 1 BPP, otherwise some more memset() calls */
  if (Bpp > 1) 
  {
	  for (lines = 0; lines < Height - 1; lines++, dst += ri->BytesPerRow)
	    memcpy (dst, src, Width * Bpp);
  } 
  else 
  {
	  for (lines = 0; lines < Height - 1; lines++, dst += ri->BytesPerRow)
	    memset( dst, Pen, Width );
  }
}

void picasso_handle_vsync (void)
{
  DX_Invalidate(-1, -1, -1, -1); //so a flushpixel is done every vsync if pixel are in buffer

	if (uae_boot_rom)
    rtarea[get_long (rtarea_base + 36) + 12 - 1]++;

  if (palette_changed) {
	  DX_SetPalette (0, 256);
	  palette_changed = 0;
  }

  flush_block ();
}

static int set_panning_called = 0;

/* Clear our screen, since we've got a new Picasso screen-mode, and refresh with the proper contents
 * This is called on several occasions:
 * 1. Amiga-->Picasso transition, via SetSwitch()
 * 2. Picasso-->Picasso transition, via SetPanning().
 * 3. whenever the graphics code notifies us that the screen contents have been lost.
 */
extern unsigned int new_beamcon0;

void picasso_refresh ( int call_setpalette )
{
    struct RenderInfo ri;
    static int beamcon0_before, p96refresh_was;

    if (!picasso_on)
	return;

    have_done_picasso=1;

    /* Make sure that the first time we show a Picasso video mode, we don't blit any crap.
     * We can do this by checking if we have an Address yet.  */
    if (picasso96_state.Address) {
	unsigned int width, height;
	/* blit the stuff from our static frame-buffer to the gfx-card */
	ri.Memory = gfxmemory + (picasso96_state.Address - gfxmem_start);
	ri.BytesPerRow = picasso96_state.BytesPerRow;
	ri.RGBFormat = (RGBFTYPE)picasso96_state.RGBFormat;

	if (set_panning_called) 
  {
	    width = ( picasso96_state.VirtualWidth < picasso96_state.Width ) ?
		picasso96_state.VirtualWidth : picasso96_state.Width;
	    height = ( picasso96_state.VirtualHeight < picasso96_state.Height ) ?
		picasso96_state.VirtualHeight : picasso96_state.Height;
	} 
  else 
  {
	    width = picasso96_state.Width;
	    height = picasso96_state.Height;
	}

	do_blit (&ri, picasso96_state.BytesPerPixel, 0, 0, 0, 0, width, height, BLIT_SRC, 0);
    } 
    else
    {
	write_log ("ERROR - picasso_refresh() can't refresh!\n");
    }
}

/*
* Functions to perform an action on the frame-buffer
*/
STATIC_INLINE void do_blitrect_frame_buffer (struct RenderInfo *ri, struct 
  RenderInfo *dstri, unsigned long srcx, unsigned long srcy,
	unsigned long dstx, unsigned long dsty, unsigned long width, unsigned 
  long height, uae_u8 mask, BLIT_OPCODE opcode)
{
  uae_u8 *src, *dst, *tmp, *tmp2, *tmp3;
  uae_u8 Bpp = GetBytesPerPixel (ri->RGBFormat);
  unsigned long total_width = width * Bpp;
  unsigned long linewidth = (total_width + 15) & ~15;
  unsigned long lines;
  int can_do_visible_blit = 0;

  src = ri->Memory + srcx*Bpp + srcy*ri->BytesPerRow;
  dst = dstri->Memory + dstx*Bpp + dsty*dstri->BytesPerRow;

  if (mask != 0xFF && Bpp > 1) 
  {
  	write_log ("WARNING - BlitRect() has mask 0x%x with Bpp %d.\n", mask, Bpp);
  }

  if (mask == 0xFF || Bpp > 1) 
  {
	  if( opcode == BLIT_SRC ) 
    {
	    /* handle normal case efficiently */
	    if (ri->Memory == dstri->Memory && dsty == srcy) 
      {
		    unsigned long i;
		    for (i = 0; i < height; i++, src += ri->BytesPerRow, dst += dstri->BytesPerRow)
		      memmove (dst, src, total_width);
	    } 
      else if (dsty < srcy) 
      {
		    unsigned long i;
		    for (i = 0; i < height; i++, src += ri->BytesPerRow, dst += dstri->BytesPerRow)
		      memcpy (dst, src, total_width);
	    } 
      else 
      {
		    unsigned long i;
		    src += (height-1) * ri->BytesPerRow;
		    dst += (height-1) * dstri->BytesPerRow;
		    for (i = 0; i < height; i++, src -= ri->BytesPerRow, dst -= dstri->BytesPerRow)
		    memcpy (dst, src, total_width);
	    }
	    return;
	  } 
    else 
    {
	    uae_u8  *src2 = src;
	    uae_u8  *dst2 = dst;
	    uae_u32 *src2_32 = (uae_u32*)src;
	    uae_u32 *dst2_32 = (uae_u32*)dst;
	    unsigned int y;

	    for (y = 0; y < height; y++) /* Vertical lines */
      {
		    uae_u8* bound = src + total_width - 4;
		    //copy now the longs
		    for( src2_32 = (uae_u32 *)src, dst2_32 = (uae_u32 *)dst; src2_32 < (uae_u32 *)bound; src2_32++, dst2_32++ ) /* Horizontal bytes */
        {
			    switch( opcode ) 
          {
		      case BLIT_FALSE:
				*dst2_32 = 0;
				break;
			    case BLIT_NOR:
				*dst2_32 = ~(*src2_32 | *dst2_32);
				break;
			    case BLIT_ONLYDST:
				*dst2_32 = *dst2_32 & ~(*src2_32);
				break;
			    case BLIT_NOTSRC:
				*dst2_32 = ~(*src2_32);
				break;
			    case BLIT_ONLYSRC:
				*dst2_32 = *src2_32 & ~(*dst2_32);
				break;
			    case BLIT_NOTDST:
				*dst2_32 = ~(*dst2_32);
				break;
			    case BLIT_EOR:
				*dst2_32 = *src2_32 ^ *dst2_32;
				break;
			    case BLIT_NAND:
				*dst2_32 = ~(*src2_32 & *dst2_32);
				break;
			    case BLIT_AND:
				*dst2_32 = *src2_32 & *dst2_32;
				break;
			    case BLIT_NEOR:
				*dst2_32 = ~(*src2_32 ^ *dst2_32);
				break;
			    case BLIT_DST:
				write_log( "do_blitrect_frame_buffer shouldn't get BLIT_DST!\n");
				break;
			    case BLIT_NOTONLYSRC:
				*dst2_32 = ~(*src2_32) | *dst2_32;
				break;
			    case BLIT_SRC:
				write_log( "do_blitrect_frame_buffer shouldn't get BLIT_SRC!\n");
				break;
			    case BLIT_NOTONLYDST:
				*dst2_32 = ~(*dst2_32) | *src2_32;
				break;
			    case BLIT_OR:
				*dst2_32 = *src2_32 | *dst2_32;
				break;
			    case BLIT_TRUE:
				*dst2_32 = 0xFFFFFFFF;
				break;
			    case 30: //code for swap source with dest in byte
				{
				    uae_u32    temp;
				    temp =     *src2_32;
				    *src2_32 = *dst2_32;
				    *dst2_32 = temp;
				}
				break;
			    case BLIT_LAST:
				write_log( "do_blitrect_frame_buffer shouldn't get BLIT_LAST!\n");
				break;
			} /* switch opcode */
		    } // for end
		    //now copy the rest few bytes
		    for (src2 = (uae_u8 *)src2_32, dst2 = (uae_u8 *)dst2_32; src2 < src + total_width; src2++, dst2++ ) /* Horizontal bytes */
        {
			switch( opcode ) 
      {
			    case BLIT_FALSE:
				*dst2 = 0;
				break;
			    case BLIT_NOR:
				*dst2 = ~(*src2 | *dst2);
				break;
			    case BLIT_ONLYDST:
				*dst2 = *dst2 & ~(*src2);
				break;
			    case BLIT_NOTSRC:
				*dst2 = ~(*src2);
				break;
			    case BLIT_ONLYSRC:
				*dst2 = *src2 & ~(*dst2);
				break;
			    case BLIT_NOTDST:
				*dst2 = ~(*dst2);
				break;
			    case BLIT_EOR:
				*dst2 = *src2 ^ *dst2;
				break;
			    case BLIT_NAND:
				*dst2 = ~(*src2 & *dst2);
				break;
			    case BLIT_AND:
				*dst2 = *src2 & *dst2;
				break;
			    case BLIT_NEOR:
				*dst2 = ~(*src2 ^ *dst2);
				break;
			    case BLIT_DST:
				write_log( "do_blitrect_frame_buffer shouldn't get BLIT_DST!\n");
				break;
			    case BLIT_NOTONLYSRC:
				*dst2 = ~(*src2) | *dst2;
				break;
			    case BLIT_SRC:
				write_log( "do_blitrect_frame_buffer shouldn't get BLIT_SRC!\n");
				break;
			    case BLIT_NOTONLYDST:
				*dst2 = ~(*dst2) | *src2;
				break;
			    case BLIT_OR:
				*dst2 = *src2 | *dst2;
				break;
			    case BLIT_TRUE:
				*dst2 = 0xFF;
				break;
			    case BLIT_LAST:
				write_log( "do_blitrect_frame_buffer shouldn't get BLIT_LAST!\n");
				break;
			    case 30: //code for swap source with dest in long
				{
				    uae_u8 temp;
				    temp = *src2;
				    *src2 = *dst2;
				    *dst2 = temp;
				}
				break;
			} /* switch opcode */
		    } /* for width */
		src += ri->BytesPerRow;
		dst += dstri->BytesPerRow;
	    } /* for height */
	}
	return;
    }

    tmp3 = tmp2 = tmp = (uae_u8 *)xmalloc (linewidth * height); /* allocate enough memory for the src-rect */
    if (!tmp)
	  return;

    /* copy the src-rect into our temporary buffer space */
    for (lines = 0; lines < height; lines++, src += ri->BytesPerRow, tmp2 += linewidth)
    {
	memcpy (tmp2, src, total_width);
    }

    /* copy the temporary buffer to the destination */
    for (lines = 0; lines < height; lines++, dst += dstri->BytesPerRow, tmp += linewidth) 
    {
	unsigned long cols;
	for (cols = 0; cols < width; cols++) 
  {
	    dst[cols] &= ~mask;
	    dst[cols] |= tmp[cols] & mask;
	}
    }
    /* free the temp-buf */
    free (tmp3);
}

/*
DrawLine: 
Synopsis: DrawLine(bi, ri, line, Mask, RGBFormat); 
Inputs: a0: struct BoardInfo *bi 
a1: struct RenderInfo *ri 
a2: struct Line *line 
d0.b: Mask 
d7.l: RGBFormat 

This function is used to paint a line on the board memory possibly using the blitter. It is called by Draw
and obeyes the destination RGBFormat as well as ForeGround and BackGround pens and draw modes. 
*/
uae_u32 REGPARAM2 picasso_DrawLine (struct regstruct *regs)
{
  return 0;
}

/*
 * BOOL FindCard(struct BoardInfo *bi);       and
 *
 * FindCard is called in the first stage of the board initialisation and
 * configuration and is used to look if there is a free and unconfigured
 * board of the type the driver is capable of managing. If it finds one,
 * it immediately reserves it for use by Picasso96, usually by clearing
 * the CDB_CONFIGME bit in the flags field of the ConfigDev struct of
 * this expansion card. But this is only a common example, a driver can
 * do whatever it wants to mark this card as used by the driver. This
 * mechanism is intended to ensure that a board is only configured and
 * used by one driver. FindBoard also usually fills some fields of the
 * BoardInfo struct supplied by the caller, the rtg.library, for example
 * the MemoryBase, MemorySize and RegisterBase fields.
 */
uae_u32 REGPARAM2 picasso_FindCard (struct regstruct *regs)
{
    uaecptr AmigaBoardInfo = m68k_areg (regs, 0);
    /* NOTES: See BoardInfo struct definition in Picasso96 dev info */

    if (allocated_gfxmem && !picasso96_state.CardFound) {
	/* Fill in MemoryBase, MemorySize */
	put_long (AmigaBoardInfo + PSSO_BoardInfo_MemoryBase, gfxmem_start);
	put_long (AmigaBoardInfo + PSSO_BoardInfo_MemorySize, allocated_gfxmem);

	picasso96_state.CardFound = 1;	/* mark our "card" as being found */

	return -1;
    } else
	return 0;
}

static void FillBoardInfo (uaecptr amigamemptr, struct LibResolution *res, struct PicassoResolution *dm)
{
    int i;

    switch (dm->depth) {
    case 1:
	res->Modes[CHUNKY] = amigamemptr;
	break;
    case 2:
	res->Modes[HICOLOR] = amigamemptr;
	break;
    case 3:
	res->Modes[TRUECOLOR] = amigamemptr;
	break;
    default:
	res->Modes[TRUEALPHA] = amigamemptr;
	break;
    }
    for (i = 0; i < PSSO_ModeInfo_sizeof; i++)
	put_byte (amigamemptr + i, 0);

    put_word (amigamemptr + PSSO_ModeInfo_Width, dm->res.width);
    put_word (amigamemptr + PSSO_ModeInfo_Height, dm->res.height);
    put_byte (amigamemptr + PSSO_ModeInfo_Depth, dm->depth * 8);
    put_byte (amigamemptr + PSSO_ModeInfo_Flags, 0);
    put_word (amigamemptr + PSSO_ModeInfo_HorTotal, dm->res.width);
    put_word (amigamemptr + PSSO_ModeInfo_HorBlankSize, 0);
    put_word (amigamemptr + PSSO_ModeInfo_HorSyncStart, 0);
    put_word (amigamemptr + PSSO_ModeInfo_HorSyncSize, 0);
    put_byte (amigamemptr + PSSO_ModeInfo_HorSyncSkew, 0);
    put_byte (amigamemptr + PSSO_ModeInfo_HorEnableSkew, 0);

    put_word (amigamemptr + PSSO_ModeInfo_VerTotal, dm->res.height);
    put_word (amigamemptr + PSSO_ModeInfo_VerBlankSize, 0);
    put_word (amigamemptr + PSSO_ModeInfo_VerSyncStart, 0);
    put_word (amigamemptr + PSSO_ModeInfo_VerSyncSize, 0);

    put_byte (amigamemptr + PSSO_ModeInfo_first_union, 98);
    put_byte (amigamemptr + PSSO_ModeInfo_second_union, 14);

    put_long (amigamemptr + PSSO_ModeInfo_PixelClock, 
      dm->res.width * dm->res.height * (currprefs.gfx_refreshrate ? abs (currprefs.gfx_refreshrate) : 50));
}

struct modeids {
    int width, height;
    int id;
};
static struct modeids mi[] =
{
/* "original" modes */

    320, 200, 0,
    320, 240, 1,
    640, 400, 2,
    640, 480, 3,
    800, 600, 4,
   1024, 768, 5,
   1152, 864, 6,
   1280,1024, 7,
   1600,1280, 8,

/* new modes */

    704, 480, 129,
    704, 576, 130,
    720, 480, 131,
    720, 576, 132,
    768, 483, 133,
    768, 576, 134,
    800, 480, 135,
    848, 480, 136,
    854, 480, 137,
    948, 576, 138,
   1024, 576, 139,
   1152, 768, 140,
   1152, 864, 141,
   1280, 720, 142,
   1280, 768, 143,
   1280, 800, 144,
   1280, 854, 145,
   1280, 960, 146,
   1366, 768, 147,
   1440, 900, 148,
   1440, 960, 149,
   1600,1200, 150,
   1680,1050, 151,
   1920,1080, 152,
   1920,1200, 153,
   2048,1152, 154,
   2048,1536, 155,
   2560,1600, 156,
   2560,2048, 157,
    400, 300, 158,
    512, 384, 159,
    640, 432, 160,
   1360, 768, 161,
   1360,1024, 162,
   1400,1050, 163,
   1792,1344, 164,
   1800,1440, 165,
   1856,1392, 166,
   1920,1440, 167,
    480, 360, 168,
    640, 350, 169,
   1600, 900, 170,
    960, 600, 171,
   1088, 612, 172,
   -1,-1,0
};

static int AssignModeID(int dm, int count, int *unkcnt)
{
    int i, w, h;

    w = newmodes[dm].res.width;
    h = newmodes[dm].res.height;
    for (i = 0; mi[i].width > 0; i++) {
	if (w == mi[i].width && h == mi[i].height)
	    return 0x50001000 | (mi[i].id * 0x10000);
    }
    (*unkcnt)++;
    write_log("P96: Non-unique mode %dx%d\n", w, h);
    return 0x51000000 - (*unkcnt) * 0x10000;
}

static uaecptr picasso96_amem, picasso96_amemend;

static void CopyLibResolutionStructureU2A (struct LibResolution *libres, uaecptr amigamemptr)
{
    int i;

    for (i = 0; i < PSSO_LibResolution_sizeof; i++)
	put_byte (amigamemptr + i, 0);
    for (i = 0; i < strlen (libres->P96ID); i++)
	put_byte (amigamemptr + PSSO_LibResolution_P96ID + i, libres->P96ID[i]);
    put_long (amigamemptr + PSSO_LibResolution_DisplayID, libres->DisplayID);
    put_word (amigamemptr + PSSO_LibResolution_Width, libres->Width);
    put_word (amigamemptr + PSSO_LibResolution_Height, libres->Height);
    put_word (amigamemptr + PSSO_LibResolution_Flags, libres->Flags);
    for (i = 0; i < MAXMODES; i++)
	put_long (amigamemptr + PSSO_LibResolution_Modes + i * 4, libres->Modes[i]);
    put_long (amigamemptr + 10, amigamemptr + PSSO_LibResolution_P96ID);
    put_long (amigamemptr + PSSO_LibResolution_BoardInfo, libres->BoardInfo);
}

static int missmodes[] = { 640, 400, 640, 480, 800, 480, -1 };

void picasso96_alloc (TrapContext *ctx)
{
    int i, j, size, cnt;
    int misscnt;

    if(newmodes != 0)
      xfree (newmodes);
    newmodes = NULL;
    picasso96_amem = picasso96_amemend = 0;
    if (currprefs.gfxmem_size == 0)
	return;
    misscnt = 0;
    cnt = 0;
    newmodes = (struct PicassoResolution*) xmalloc (sizeof (struct PicassoResolution) * MAX_PICASSO_MODES);
    size = 0;
    i = 0;
    while (DisplayModes[i].depth >= 0) {
	for (j = 0; missmodes[j * 2] >= 0; j++) {
	    if (DisplayModes[i].res.width == missmodes[j * 2 + 0] && DisplayModes[i].res.height == missmodes[j * 2 + 1]) {
		missmodes[j * 2 + 0] = 0;
		missmodes[j * 2 + 1] = 0;
	    }
	}
	i++;
    }

    i = 0;
    while (DisplayModes[i].depth >= 0) {
	j = i;
	size += PSSO_LibResolution_sizeof;
	while (missmodes[misscnt * 2] == 0)
	    misscnt++;
	if (missmodes[misscnt * 2] >= 0) {
	    int oldi = i;
	    if (DisplayModes[i].res.width > missmodes[misscnt * 2 + 0] || DisplayModes[i].res.height > missmodes[misscnt * 2 + 1]) {
		int w = DisplayModes[i].res.width;
		int h = DisplayModes[i].res.height;
		do {
		    struct PicassoResolution *pr = &newmodes[cnt];
		    memcpy (pr, &DisplayModes[i], sizeof (struct PicassoResolution));
		    pr->res.width = missmodes[misscnt * 2 + 0];
		    pr->res.height = missmodes[misscnt * 2 + 1];
		    sprintf (pr->name, "%dx%dx%d FAKE", pr->res.width, pr->res.height, pr->depth * 8);
		    size += PSSO_ModeInfo_sizeof;
		    i++;
		    cnt++;
		} while (DisplayModes[i].depth >= 0
		    && w == DisplayModes[i].res.width
		    && h == DisplayModes[i].res.height);
	    
		i = oldi;
		misscnt++;
		continue;
	    }
	}
	do {
	    memcpy (&newmodes[cnt], &DisplayModes[i], sizeof (struct PicassoResolution));
	    size += PSSO_ModeInfo_sizeof;
	    i++;
	    cnt++;
	} while (DisplayModes[i].depth >= 0
	    && DisplayModes[i].res.width == DisplayModes[j].res.width
	    && DisplayModes[i].res.height == DisplayModes[j].res.height);
    }
    newmodes[cnt].depth = -1;

  	for (i = 0; i < cnt; i++) {
//  	  sprintf (DisplayModes[i].name, "%dx%d, %d-bit, %d Hz",
//  		  DisplayModes[i].res.width, DisplayModes[i].res.height, DisplayModes[i].depth * 8, DisplayModes[i].refresh);
  	  switch (newmodes[i].depth) {
  	    case 1:
		      if (newmodes[i].res.width > chunky.width)
		        chunky.width = newmodes[i].res.width;
		      if (newmodes[i].res.height > chunky.height)
		        chunky.height = newmodes[i].res.height;
		      break;
	      case 2:
		      if (newmodes[i].res.width > hicolour.width)
		        hicolour.width = newmodes[i].res.width;
		      if (newmodes[i].res.height > hicolour.height)
		        hicolour.height = newmodes[i].res.height;
		      break;
	      case 3:
		      if (newmodes[i].res.width > truecolour.width)
		        truecolour.width = newmodes[i].res.width;
		      if (newmodes[i].res.height > truecolour.height)
		        truecolour.height = newmodes[i].res.height;
		      break;
	      case 4:
		      if (newmodes[i].res.width > alphacolour.width)
		        alphacolour.width = newmodes[i].res.width;
		      if (newmodes[i].res.height > alphacolour.height)
		        alphacolour.height = newmodes[i].res.height;
		      break;
	    }
	  }

    m68k_dreg (&ctx->regs, 0) = size;
    m68k_dreg (&ctx->regs, 1) = 65536 + 1;
    picasso96_amem = CallLib (ctx, get_long (4), -0xC6); /* AllocMem */
    picasso96_amemend = picasso96_amem + size;
}


/****************************************
* InitCard()
*
* a2: BoardInfo structure ptr - Amiga-based address in Intel endian-format
*
* Job - fill in the following structure members:
* gbi_RGBFormats: the pixel formats that the host-OS of UAE supports
*     If UAE is running in a window, it should ONLY report the pixel format of the host-OS desktop
*     If UAE is running full-screen, it should report ALL pixel formats that the host-OS can handle in full-screen
*     NOTE: If full-screen, and the user toggles to windowed-mode, all hell will break loose visually.  Must inform
*           user that they're doing something stupid (unless their desktop and full-screen colour modes match).
* gbi_SoftSpriteFlags: should be the same as above for now, until actual cursor support is added
* gbi_BitsPerCannon: could be 6 or 8 or ???, depending on the host-OS gfx-card
* gbi_MaxHorResolution: fill this in for all modes (even if you don't support them)
* gbi_MaxVerResolution: fill this in for all modes (even if you don't support them)
*/
uae_u32 REGPARAM2 picasso_InitCard (struct regstruct *regs)
{
  struct LibResolution res;
  int ModeInfoStructureCount = 1, LibResolutionStructureCount = 0;
  int i, j, unkcnt;

  uaecptr amem;
  uaecptr AmigaBoardInfo = m68k_areg (regs, 2);
    if (!picasso96_amem) {
	write_log ("P96: InitCard() but no resolution memory!\n");
	return 0;
    }
    amem = picasso96_amem;

  put_word (AmigaBoardInfo + PSSO_BoardInfo_BitsPerCannon, DX_BitsPerCannon ());
  put_word (AmigaBoardInfo + PSSO_BoardInfo_RGBFormats, picasso96_pixel_format);
  put_long (AmigaBoardInfo + PSSO_BoardInfo_BoardType, BT_uaegfx);
  put_word (AmigaBoardInfo + PSSO_BoardInfo_SoftSpriteFlags, picasso96_pixel_format);
  put_word (AmigaBoardInfo + PSSO_BoardInfo_MaxHorResolution + 0, planar.width);
  put_word (AmigaBoardInfo + PSSO_BoardInfo_MaxHorResolution + 2, chunky.width);
  put_word (AmigaBoardInfo + PSSO_BoardInfo_MaxHorResolution + 4, hicolour.width);
  put_word (AmigaBoardInfo + PSSO_BoardInfo_MaxHorResolution + 6, truecolour.width);
  put_word (AmigaBoardInfo + PSSO_BoardInfo_MaxHorResolution + 8, alphacolour.width);
  put_word (AmigaBoardInfo + PSSO_BoardInfo_MaxVerResolution + 0, planar.height);
  put_word (AmigaBoardInfo + PSSO_BoardInfo_MaxVerResolution + 2, chunky.height);
  put_word (AmigaBoardInfo + PSSO_BoardInfo_MaxVerResolution + 4, hicolour.height);
  put_word (AmigaBoardInfo + PSSO_BoardInfo_MaxVerResolution + 6, truecolour.height);
  put_word (AmigaBoardInfo + PSSO_BoardInfo_MaxVerResolution + 8, alphacolour.height);

  i = 0;
  unkcnt = 0;
  while (newmodes[i].depth >= 0) {    
  	j = i;
	  /* Add a LibResolution structure to the ResolutionsList MinList in our BoardInfo */
	  res.DisplayID = AssignModeID (i, LibResolutionStructureCount, &unkcnt);
  	res.BoardInfo = AmigaBoardInfo;
  	res.Width = newmodes[i].res.width;
  	res.Height = newmodes[i].res.height;
  	res.Flags = P96F_PUBLIC;
  	memcpy (res.P96ID, "P96-0:", 6);
	  sprintf (res.Name, "uaegfx:%dx%d", res.Width, res.Height);
  	res.Modes[PLANAR] = 0;
  	res.Modes[CHUNKY] = 0;
  	res.Modes[HICOLOR] = 0;
  	res.Modes[TRUECOLOR] = 0;
  	res.Modes[TRUEALPHA] = 0;

  	do {
	    /* Handle this display mode's depth */
	    /* New: Only add the modes when there is enough P96 RTG memory to hold the bitmap */
	    if (allocated_gfxmem >= newmodes[i].res.width * newmodes[i].res.height * newmodes[i].depth) {
        ModeInfoStructureCount++;
    		FillBoardInfo (amem, &res, &newmodes[i]);
		    amem += PSSO_ModeInfo_sizeof;
	    }
	    i++;
	  } while (newmodes[i].depth >= 0
		  && newmodes[i].res.width == newmodes[j].res.width
		  && newmodes[i].res.height == newmodes[j].res.height);

    LibResolutionStructureCount++;
  	CopyLibResolutionStructureU2A (&res, amem);
  	AmigaListAddTail (AmigaBoardInfo + PSSO_BoardInfo_ResolutionsList, amem);
  	amem += PSSO_LibResolution_sizeof;
  }
    if (amem > picasso96_amemend)
	write_log ("P96: display resolution list corruption %08x<>%08x (%d)\n", amem, picasso96_amemend, i);

  return -1;
}

/*
 * SetSwitch:
 * a0:	struct BoardInfo
 * d0.w:	BOOL state
 * this function should set a board switch to let the Amiga signal pass
 * through when supplied with a 0 in d0 and to show the board signal if
 * a 1 is passed in d0. You should remember the current state of the
 * switch to avoid unneeded switching. If your board has no switch, then
 * simply supply a function that does nothing except a RTS.
 *
 * NOTE: Return the opposite of the switch-state. BDK
*/
uae_u32 REGPARAM2 picasso_SetSwitch (struct regstruct *regs)
{
    uae_u16 flag = m68k_dreg (regs, 0) & 0xFFFF;

    /* Do not switch immediately.  Tell the custom chip emulation about the
     * desired state, and wait for custom.c to call picasso_enablescreen
     * whenever it is ready to change the screen state.  */
    picasso_requested_on = flag;
    write_log ("SetSwitch() - trying to show %s screen\n", flag ? "picasso96":"amiga");

    /* Put old switch-state in D0 */
    return !flag;
}

static void init_picasso_screen(void);
void picasso_enablescreen (int on)
{
    if (!init_picasso_screen_called)
	init_picasso_screen();
    wgfx_linestart = 0xFFFFFFFF;
    picasso_refresh (1);
    write_log ("picasso_enablescreen() showing %s screen\n", on ? "picasso96": "amiga");
    checkrtglibrary();
}


/*
 * SetColorArray:
 * a0: struct BoardInfo
 * d0.w: startindex
 * d1.w: count
 * when this function is called, your driver has to fetch "count" color
 * values starting at "startindex" from the CLUT field of the BoardInfo
 * structure and write them to the hardware. The color values are always
 * between 0 and 255 for each component regardless of the number of bits
 * per cannon your board has. So you might have to shift the colors
 * before writing them to the hardware.
 */
uae_u32 REGPARAM2 picasso_SetColorArray (struct regstruct *regs)
{
    /* Fill in some static UAE related structure about this new CLUT setting
     * We need this for CLUT-based displays, and for mapping CLUT to hi/true colour */
    uae_u16 start = m68k_dreg (regs, 0);
    uae_u16 count = m68k_dreg (regs, 1);
    int i;
    uaecptr boardinfo = m68k_areg (regs, 0);
    uaecptr clut = boardinfo + PSSO_BoardInfo_CLUT + start * 3;

    for (i = start; i < start + count; i++) {
	int r = get_byte (clut);
	int g = get_byte (clut + 1);
	int b = get_byte (clut + 2);

	palette_changed |= (picasso96_state.CLUT[i].Red != r 
      || picasso96_state.CLUT[i].Green != g 
      || picasso96_state.CLUT[i].Blue != b);

	picasso96_state.CLUT[i].Red = r;
	picasso96_state.CLUT[i].Green = g;
	picasso96_state.CLUT[i].Blue = b;
	clut += 3;
    }
    return 1;
}

/*
 * SetDAC:
 * a0: struct BoardInfo
 * d7: RGBFTYPE RGBFormat
 * This function is called whenever the RGB format of the display changes,
 * e.g. from chunky to TrueColor. Usually, all you have to do is to set
 * the RAMDAC of your board accordingly.
 */
uae_u32 REGPARAM2 picasso_SetDAC (struct regstruct *regs)
{
    /* Fill in some static UAE related structure about this new DAC setting
     * Lets us keep track of what pixel format the Amiga is thinking about in our frame-buffer */

    return 1;
}

static void init_picasso_screen (void)
{
    if (set_panning_called) {
	picasso96_state.Extent = picasso96_state.Address + picasso96_state.BytesPerRow * picasso96_state.VirtualHeight;
    }

    if (set_gc_called) {
      gfx_set_picasso_modeinfo (picasso96_state.Width, picasso96_state.Height, 
        picasso96_state.GC_Depth, (RGBFTYPE) picasso96_state.RGBFormat);
    }    
    if( ( picasso_vidinfo.width == picasso96_state.Width ) &&
	( picasso_vidinfo.height == picasso96_state.Height ) &&
	( picasso_vidinfo.depth == (picasso96_state.GC_Depth >> 3) ) &&
	( picasso_vidinfo.selected_rgbformat == picasso96_state.RGBFormat) ) 
    {
      DX_SetPalette (0, 256);
      picasso_refresh (1);
    }
    init_picasso_screen_called = 1;
}

/*
 * SetGC:
 * a0: struct BoardInfo
 * a1: struct ModeInfo
 * d0: BOOL Border
 * This function is called whenever another ModeInfo has to be set. This
 * function simply sets up the CRTC and TS registers to generate the
 * timing used for that screen mode. You should not set the DAC, clocks
 * or linear start adress. They will be set when appropriate by their
 * own functions.
 */
uae_u32 REGPARAM2 picasso_SetGC (struct regstruct *regs)
{
    /* Fill in some static UAE related structure about this new ModeInfo setting */
    uae_u32 border   = m68k_dreg (regs, 0);
    uaecptr modeinfo = m68k_areg (regs, 1);

    picasso96_state.Width = get_word (modeinfo + PSSO_ModeInfo_Width);
    picasso96_state.VirtualWidth = picasso96_state.Width;	/* in case SetPanning doesn't get called */

    picasso96_state.Height = get_word (modeinfo + PSSO_ModeInfo_Height);
    picasso96_state.VirtualHeight = picasso96_state.Height; /* in case SetPanning doesn't get called */

    picasso96_state.GC_Depth = get_byte (modeinfo + PSSO_ModeInfo_Depth);
    picasso96_state.GC_Flags = get_byte (modeinfo + PSSO_ModeInfo_Flags);

    set_gc_called = 1;
    picasso96_state.HostAddress = NULL;

    init_picasso_screen ();
    init_hz_p96 ();
    return 1;
}

/*
 * SetPanning:
 * a0: struct BoardInfo
 * a1: UBYTE *Memory
 * d0: uae_u16 Width
 * d1: WORD XOffset
 * d2: WORD YOffset
 * d7: RGBFTYPE RGBFormat
 * This function sets the view origin of a display which might also be
 * overscanned. In register a1 you get the start address of the screen
 * bitmap on the Amiga side. You will have to subtract the starting
 * address of the board memory from that value to get the memory start
 * offset within the board. Then you get the offset in pixels of the
 * left upper edge of the visible part of an overscanned display. From
 * these values you will have to calculate the LinearStartingAddress
 * fields of the CRTC registers.

 * NOTE: SetPanning() can be used to know when a Picasso96 screen is
 * being opened.  Better to do the appropriate clearing of the
 * background here than in SetSwitch() derived functions,
 * because SetSwitch() is not called for subsequent Picasso screens.
 */
 
static void picasso_SetPanningInit(void)
{
    picasso96_state.XYOffset = picasso96_state.Address + (picasso96_state.XOffset * picasso96_state.BytesPerPixel)
	+ (picasso96_state.YOffset * picasso96_state.BytesPerRow);
    picasso96_state.BytesPerRow = picasso96_state.VirtualWidth * picasso96_state.BytesPerPixel;
}   

uae_u32 REGPARAM2 picasso_SetPanning (struct regstruct *regs)
{
    uae_u16 Width = m68k_dreg (regs, 0);
    uaecptr start_of_screen = m68k_areg (regs, 1);
    uaecptr bi = m68k_areg (regs, 0);
    uaecptr bmeptr = get_long (bi + PSSO_BoardInfo_BitMapExtra);        /* Get our BoardInfo ptr's BitMapExtra ptr */
    uae_u16 bme_width, bme_height;

    bme_width = get_word (bmeptr + PSSO_BitMapExtra_Width);
    bme_height = get_word (bmeptr + PSSO_BitMapExtra_Height);

    picasso96_state.Address = start_of_screen;	/* Amiga-side address */
    picasso96_state.XOffset = (uae_s16) (m68k_dreg (regs, 1) & 0xFFFF);
    picasso96_state.YOffset = (uae_s16) (m68k_dreg (regs, 2) & 0xFFFF);
    picasso96_state.VirtualWidth = bme_width;
    picasso96_state.VirtualHeight = bme_height;
    picasso96_state.RGBFormat = m68k_dreg (regs, 7);
    picasso96_state.BytesPerPixel = GetBytesPerPixel (picasso96_state.RGBFormat);
    picasso_SetPanningInit();
    set_panning_called = 1;
    init_picasso_screen ();
    set_panning_called = 0;

    return 1;
}

static void do_xor8 (uae_u8 * ptr, long len, uae_u32 val)
{
    int i;
    for (i = 0; i < len; i++, ptr++) {
	do_put_mem_byte (ptr, (uae_u8)(do_get_mem_byte (ptr) ^ val));
    }
}

/*
 * InvertRect:
 *
 * Inputs:
 * a0:struct BoardInfo *bi
 * a1:struct RenderInfo *ri
 * d0.w:X
 * d1.w:Y
 * d2.w:Width
 * d3.w:Height
 * d4.l:Mask
 * d7.l:RGBFormat
 *
 * This function is used to invert a rectangular area on the board. It is called by BltBitMap,
 * BltPattern and BltTemplate.
 */
uae_u32 REGPARAM2 picasso_InvertRect (struct regstruct *regs)
{
    uaecptr renderinfo = m68k_areg (regs, 1);
    unsigned long X = (uae_u16)m68k_dreg (regs, 0);
    unsigned long Y = (uae_u16)m68k_dreg (regs, 1);
    unsigned long Width = (uae_u16)m68k_dreg (regs, 2);
    unsigned long Height = (uae_u16)m68k_dreg (regs, 3);
    uae_u8 mask = (uae_u8)m68k_dreg (regs, 4);
    int Bpp = GetBytesPerPixel (m68k_dreg (regs, 7));
    uae_u32 xorval;
    unsigned int lines;
    struct RenderInfo ri;
    uae_u8 *uae_mem, *rectstart;
    unsigned long width_in_bytes;
    uae_u32 result = 0;

    wgfx_flushline ();

    if (CopyRenderInfoStructureA2U (renderinfo, &ri)) 
    {
	if (mask != 0xFF && Bpp > 1)
	{
	    mask = 0xFF;
	}

	xorval = 0x01010101 * (mask & 0xFF);
	width_in_bytes = Bpp * Width;
	rectstart = uae_mem = ri.Memory + Y*ri.BytesPerRow + X*Bpp;

	for (lines = 0; lines < Height; lines++, uae_mem += ri.BytesPerRow)
	    do_xor8 (uae_mem, width_in_bytes, xorval);

	if (renderinfo_is_current_screen (&ri)) {
	    if (mask == 0xFF)
		do_invertrect (&ri, Bpp, X, Y, Width, Height);
	    else
		do_blit( &ri, Bpp, X, Y, X, Y, Width, Height, BLIT_SRC, 0);
	}
	result = 1;
    }
    return result; /* 1 if supported, 0 otherwise */
}

/***********************************************************
FillRect:
***********************************************************
* a0: 	struct BoardInfo *
* a1:	struct RenderInfo *
* d0: 	WORD X
* d1: 	WORD Y
* d2: 	WORD Width
* d3: 	WORD Height
* d4:	uae_u32 Pen
* d5:	UBYTE Mask
* d7:	uae_u32 RGBFormat
***********************************************************/
uae_u32 REGPARAM2 picasso_FillRect (struct regstruct *regs)
{
  uaecptr renderinfo = m68k_areg (regs, 1);
  uae_u32 X = (uae_u16)m68k_dreg (regs, 0);
  uae_u32 Y = (uae_u16)m68k_dreg (regs, 1);
  uae_u32 Width = (uae_u16)m68k_dreg (regs, 2);
  uae_u32 Height = (uae_u16)m68k_dreg (regs, 3);
  uae_u32 Pen = m68k_dreg (regs, 4);
  uae_u8 Mask = (uae_u8)m68k_dreg (regs, 5);
  RGBFTYPE RGBFormat = (RGBFTYPE) m68k_dreg (regs, 7);

  uae_u8 *src;
  uae_u8 *oldstart;
  int Bpp;
  struct RenderInfo ri;
  uae_u32 result = 0;

    if (Width * Height <= 2500)
	return 0;

#ifdef JIT
  special_mem|=S_WRITE|S_READ;
#endif

  wgfx_flushline ();

  if (CopyRenderInfoStructureA2U (renderinfo, &ri) && Y != 0xFFFF) 
  {
	  if (ri.RGBFormat != RGBFormat)
	    write_log ("Weird Stuff!\n");

	  Bpp = GetBytesPerPixel (RGBFormat);

	  if (Bpp > 1)
	    Mask = 0xFF;

	  if (Mask == 0xFF) 
    {
	    /* Do the fill-rect in the frame-buffer */
	    do_fillrect_frame_buffer (&ri, X, Y, Width, Height, Pen, Bpp, RGBFormat);

	    /* Now we do the on-screen display, if renderinfo points to it */
	    if (renderinfo_is_current_screen (&ri)) 
      {
		    src = ri.Memory + X * Bpp + Y * ri.BytesPerRow;
		    X = X - picasso96_state.XOffset;
		    Y = Y - picasso96_state.YOffset;
    		if ((int)X < 0) { Width = Width + X; X = 0; }
    		if ((int)Width < 1) return 1;
    		if ((int)Y < 0) { Height = Height + Y; Y = 0; }
    		if ((int)Height < 1) return 1;
        
		    /* Argh - why does P96Speed do this to me, with FillRect only?! */
		    if ((X < picasso96_state.Width) && 
            (Y < picasso96_state.Height)) 
        {
		      if (X + Width > picasso96_state.Width)
			      Width = picasso96_state.Width - X;
		      if (Y+Height > picasso96_state.Height)
			      Height = picasso96_state.Height - Y;

		      do_fillrect (src, X, Y, Width, Height, Pen, Bpp, RGBFormat);
		    }
	    }
	    result = 1;
	  } 
    else 
    {
	    /* We get here only if Mask != 0xFF */
	    if (Bpp != 1) 
      {
		    write_log( "WARNING - FillRect() has unhandled mask 0x%x with Bpp %d. Using fall-back routine.\n", Mask, Bpp);
	    } 
      else 
      {
		    Pen &= Mask;
		    Mask = ~Mask;
		    oldstart = ri.Memory + Y * ri.BytesPerRow + X * Bpp;
		    {
  		    uae_u8 *start = oldstart;
  		    uae_u8 *end = start + Height * ri.BytesPerRow;
  		    for (; start != end; start += ri.BytesPerRow) 
          {
      			uae_u8 *p = start;
      			unsigned long cols;
      			for (cols = 0; cols < Width; cols++) 
            {
    			    uae_u32 tmpval = do_get_mem_byte (p + cols) & Mask;
     			    do_put_mem_byte (p + cols, (uae_u8)(Pen | tmpval));
      			}
  		    }
  		  }
  		  if (renderinfo_is_current_screen (&ri))
  		    do_blit( &ri, Bpp, X, Y, X, Y, Width, Height, BLIT_SRC, 0);
  		  result = 1;
  	  }
  	}
  }

  return result;
}

/*
* BlitRect() is a generic (any chunky pixel format) rectangle copier
* NOTE: If dstri is NULL, then we're only dealing with one RenderInfo area, and called from picasso_BlitRect()
*
* OpCodes:
* 0 = FALSE:    dst = 0
* 1 = NOR:      dst = ~(src | dst)
* 2 = ONLYDST:  dst = dst & ~src
* 3 = NOTSRC:   dst = ~src
* 4 = ONLYSRC:  dst = src & ~dst
* 5 = NOTDST:   dst = ~dst
* 6 = EOR:      dst = src^dst
* 7 = NAND:     dst = ~(src & dst)
* 8 = AND:      dst = (src & dst)
* 9 = NEOR:     dst = ~(src ^ dst)
*10 = DST:      dst = dst
*11 = NOTONLYSRC: dst = ~src | dst
*12 = SRC:      dst = src
*13 = NOTONLYDST: dst = ~dst | src
*14 = OR:       dst = src | dst
*15 = TRUE:     dst = 0xFF
*/
struct blitdata
{
    struct RenderInfo ri_struct;
    struct RenderInfo dstri_struct;
    struct RenderInfo *ri; /* Self-referencing pointers */
    struct RenderInfo *dstri;
    unsigned long srcx;
    unsigned long srcy;
    unsigned long dstx;
    unsigned long dsty;
    unsigned long width;
    unsigned long height;
    uae_u8 mask;
    BLIT_OPCODE opcode;
} blitrectdata;

STATIC_INLINE int BlitRectHelper( void )
{
    struct RenderInfo *ri    = blitrectdata.ri;
    struct RenderInfo *dstri = blitrectdata.dstri;
    unsigned long srcx =   blitrectdata.srcx;
    unsigned long srcy =   blitrectdata.srcy;
    unsigned long dstx =   blitrectdata.dstx;
    unsigned long dsty =   blitrectdata.dsty;
    unsigned long width =  blitrectdata.width;
    unsigned long height = blitrectdata.height;
    uae_u8 mask = blitrectdata.mask;
    BLIT_OPCODE opcode = blitrectdata.opcode;

    uae_u8 Bpp = GetBytesPerPixel (ri->RGBFormat);
    unsigned long total_width = width * Bpp;
    unsigned long linewidth = (total_width + 15) & ~15;
    int can_do_visible_blit = 0;

    if (opcode == BLIT_DST) 
    {
	write_log( "WARNING: BlitRect() being called with opcode of BLIT_DST\n" );
	return 1;
    }

    /*
     * If we have no destination RenderInfo, then we're dealing with a single-buffer action, called
     * from picasso_BlitRect().  The code in do_blitrect_frame_buffer() deals with the frame-buffer,
     * while the do_blit() code deals with the visible screen.
     *
     * If we have a destination RenderInfo, then we've been called from picasso_BlitRectNoMaskComplete()
     * and we need to put the results on the screen from the frame-buffer.
     */
    if (dstri == NULL || dstri->Memory == ri->Memory) 
    {
	if (mask != 0xFF && Bpp > 1) 
  {
	    mask = 0xFF;
	}
	dstri = ri;
	can_do_visible_blit = 1;
    }

    /* Do our virtual frame-buffer memory first */
    do_blitrect_frame_buffer (ri, dstri, srcx, srcy, dstx, dsty, width, height, mask, opcode);
    /* Now we do the on-screen display, if renderinfo points to it */

    if (renderinfo_is_current_screen (dstri)) 
    {
	if (mask == 0xFF || Bpp > 1) {
	    if (can_do_visible_blit)
		do_blit (dstri, Bpp, srcx, srcy, dstx, dsty, width, height, opcode, 1);
	    else
		do_blit (dstri, Bpp, dstx, dsty, dstx, dsty, width, height, opcode, 0);
	} else {
	    do_blit (dstri, Bpp, dstx, dsty, dstx, dsty, width, height, opcode, 0);
  }
    }

    return 1;
}

STATIC_INLINE int BlitRect (uaecptr ri, uaecptr dstri,
			    unsigned long srcx, unsigned long srcy, unsigned long dstx, unsigned long dsty,
			    unsigned long width, unsigned long height, uae_u8 mask, BLIT_OPCODE opcode )
{
    /* Set up the params */
    CopyRenderInfoStructureA2U( ri, &blitrectdata.ri_struct );
    blitrectdata.ri = &blitrectdata.ri_struct;

    if (dstri) 
    {
	CopyRenderInfoStructureA2U( dstri, &blitrectdata.dstri_struct );
	blitrectdata.dstri = &blitrectdata.dstri_struct;
    } 
    else
    {
	blitrectdata.dstri = NULL;
    }
    blitrectdata.srcx   = srcx;
    blitrectdata.srcy   = srcy;
    blitrectdata.dstx   = dstx;
    blitrectdata.dsty   = dsty;
    blitrectdata.width  = width;
    blitrectdata.height = height;
    blitrectdata.mask   = mask;
    blitrectdata.opcode = opcode;

    return BlitRectHelper();
}

/***********************************************************
BlitRect:
***********************************************************
* a0:   struct BoardInfo
* a1:   struct RenderInfo
* d0:   WORD SrcX
* d1:   WORD SrcY
* d2:   WORD DstX
* d3:   WORD DstY
* d4:   WORD Width
* d5:   WORD Height
* d6:   UBYTE Mask
* d7:   uae_u32 RGBFormat
***********************************************************/
uae_u32 REGPARAM2 picasso_BlitRect (struct regstruct *regs)
{
    uaecptr renderinfo   = m68k_areg (regs, 1);
    unsigned long srcx   = (uae_u16)m68k_dreg (regs, 0);
    unsigned long srcy   = (uae_u16)m68k_dreg (regs, 1);
    unsigned long dstx   = (uae_u16)m68k_dreg (regs, 2);
    unsigned long dsty   = (uae_u16)m68k_dreg (regs, 3);
    unsigned long width  = (uae_u16)m68k_dreg (regs, 4);
    unsigned long height = (uae_u16)m68k_dreg (regs, 5);
    uae_u8  Mask         = (uae_u8) m68k_dreg (regs, 6);
    uae_u32 result = 0;

#ifdef JIT
    special_mem|=S_WRITE|S_READ;
#endif

    wgfx_flushline ();

    result = BlitRect (renderinfo, (uaecptr)NULL, srcx, srcy, dstx, dsty, width, height, Mask, BLIT_SRC);

    return result;
}

/***********************************************************
BlitRectNoMaskComplete:
***********************************************************
* a0: 	struct BoardInfo
* a1:	struct RenderInfo (src)
* a2:   struct RenderInfo (dst)
* d0: 	WORD SrcX
* d1: 	WORD SrcY
* d2: 	WORD DstX
* d3: 	WORD DstY
* d4:   WORD Width
* d5:   WORD Height
* d6:	UBYTE OpCode
* d7:	uae_u32 RGBFormat
* NOTE: MUST return 0 in D0 if we're not handling this operation
*       because the RGBFormat or opcode aren't supported.
*       OTHERWISE return 1
***********************************************************/
uae_u32 REGPARAM2 picasso_BlitRectNoMaskComplete (struct regstruct *regs)
{
    uaecptr srcri = m68k_areg (regs, 1);
    uaecptr dstri = m68k_areg (regs, 2);
    unsigned long srcx =   (uae_u16)m68k_dreg (regs, 0);
    unsigned long srcy =   (uae_u16)m68k_dreg (regs, 1);
    unsigned long dstx =   (uae_u16)m68k_dreg (regs, 2);
    unsigned long dsty =   (uae_u16)m68k_dreg (regs, 3);
    unsigned long width =  (uae_u16)m68k_dreg (regs, 4);
    unsigned long height = (uae_u16)m68k_dreg (regs, 5);
    uae_u8 OpCode =  m68k_dreg (regs, 6);
    uae_u32 RGBFmt = m68k_dreg (regs, 7);
    uae_u32 result = 0;

#ifdef JIT
    special_mem|=S_WRITE|S_READ;
#endif

    wgfx_flushline ();

    result = BlitRect (srcri, dstri, srcx, srcy, dstx, dsty, width, height, 0xFF, (BLIT_OPCODE)OpCode);

    return result;
}

/* This utility function is used both by BlitTemplate() and BlitPattern() */
STATIC_INLINE void PixelWrite1 (uae_u8 * mem, int bits, uae_u32 fgpen, uae_u32 mask)
{
  if (mask != 0xFF)
  	fgpen = (fgpen & mask) | (do_get_mem_byte (mem + bits) & ~mask);
  do_put_mem_byte (mem + bits, fgpen);
}

STATIC_INLINE void PixelWrite2 (uae_u8 * mem, int bits, uae_u32 fgpen)
{
  do_put_mem_word (((uae_u16 *) mem) + bits, fgpen);
}

STATIC_INLINE void PixelWrite3 (uae_u8 * mem, int bits, uae_u32 fgpen)
{
  do_put_mem_byte (mem + bits * 3, fgpen & 0x000000FF);
  *(uae_u16 *) (mem + bits * 3 + 1) = (fgpen & 0x00FFFF00) >> 8;
}

STATIC_INLINE void PixelWrite4 (uae_u8 * mem, int bits, uae_u32 fgpen)
{
  do_put_mem_long (((uae_u32 *) mem) + bits, fgpen);
}

STATIC_INLINE void PixelWrite (uae_u8 * mem, int bits, uae_u32 fgpen, uae_u8 Bpp, uae_u32 mask)
{
  switch (Bpp) {
    case 1:
  	  if (mask != 0xFF)
  	    fgpen = (fgpen & mask) | (do_get_mem_byte (mem + bits) & ~mask);
  	  do_put_mem_byte (mem + bits, (uae_u8)fgpen);
  	  break;
    case 2:
  	  do_put_mem_word (((uae_u16 *) mem) + bits, (uae_u16)fgpen);
  	  break;
    case 3:
  	  do_put_mem_byte (mem + bits * 3, (uae_u8)fgpen);
  	  *(uae_u16 *) (mem + bits * 3 + 1) = (fgpen & 0x00FFFF00) >> 8;
  	  break;
    case 4:
  	  do_put_mem_long (((uae_u32 *) mem) + bits, fgpen);
  	  break;
  }
}

/*
 * BlitPattern:
 *
 * Synopsis:BlitPattern(bi, ri, pattern, X, Y, Width, Height, Mask, RGBFormat);
 * Inputs:
 * a0:struct BoardInfo *bi
 * a1:struct RenderInfo *ri
 * a2:struct Pattern *pattern
 * d0.w:X
 * d1.w:Y
 * d2.w:Width
 * d3.w:Height
 * d4.w:Mask
 * d7.l:RGBFormat
 *
 * This function is used to paint a pattern on the board memory using the blitter. It is called by
 * BltPattern, if a AreaPtrn is used with positive AreaPtSz. The pattern consists of a b/w image
 * using a single plane of image data which will be expanded repeatedly to the destination RGBFormat
 * using ForeGround and BackGround pens as well as draw modes. The width of the pattern data is
 * always 16 pixels (one word) and the height is calculated as 2^Size. The data must be shifted up
 * and to the left by XOffset and YOffset pixels at the beginning.
 */
uae_u32 REGPARAM2 picasso_BlitPattern (struct regstruct *regs)
{
    uaecptr rinf = m68k_areg (regs, 1);
    uaecptr pinf = m68k_areg (regs, 2);
    unsigned long X = (uae_u16)m68k_dreg (regs, 0);
    unsigned long Y = (uae_u16)m68k_dreg (regs, 1);
    unsigned long W = (uae_u16)m68k_dreg (regs, 2);
    unsigned long H = (uae_u16)m68k_dreg (regs, 3);
    uae_u8 Mask =     (uae_u8) m68k_dreg (regs, 4);
    uae_u32 RGBFmt = m68k_dreg (regs, 7);
    uae_u8 Bpp = GetBytesPerPixel (RGBFmt);
    int inversion = 0;
    struct RenderInfo ri;
    struct Pattern pattern;
    unsigned long rows;
    uae_u32 fgpen;
    uae_u8 *uae_mem;
    int xshift;
    unsigned long ysize_mask;
    uae_u32 result = 0;

#ifdef JIT
    special_mem|=S_WRITE|S_READ;
#endif

    wgfx_flushline ();

    if (CopyRenderInfoStructureA2U (rinf, &ri) && CopyPatternStructureA2U (pinf, &pattern)) 
    {
	Bpp = GetBytesPerPixel (ri.RGBFormat);
	uae_mem = ri.Memory + Y*ri.BytesPerRow + X*Bpp; /* offset with address */

	if (pattern.DrawMode & INVERS)
	    inversion = 1;

	pattern.DrawMode &= 0x03;

	if (Mask != 0xFF) 
  {
	    if( Bpp > 1 )
		Mask = 0xFF;

	    if( pattern.DrawMode == COMP)
	    {
		 write_log ("WARNING - BlitPattern() has unhandled mask 0x%x with COMP DrawMode. Using fall-back routine.\n", Mask);
	    }
	    else
	    {
		result = 1;
	    }
	} 
  else
	{
	    result = 1;
	}

	if (result) 
  {
	    ysize_mask = (1 << pattern.Size) - 1;
	    xshift = pattern.XOffset & 15;

	    for (rows = 0; rows < H; rows++, uae_mem += ri.BytesPerRow) {
		unsigned long prow = (rows + pattern.YOffset) & ysize_mask;
		unsigned int d = do_get_mem_word (((uae_u16 *)pattern.Memory) + prow);
		uae_u8 *uae_mem2 = uae_mem;
		unsigned long cols;

		if (xshift != 0)
		    d = (d << xshift) | (d >> (16 - xshift));

		for (cols = 0; cols < W; cols += 16, uae_mem2 += Bpp << 4) 
    {
		    long bits;
		    long max = W - cols;
		    unsigned int data = d;

		    if (max > 16)
			max = 16;

		    for (bits = 0; bits < max; bits++) 
        {
			int bit_set = data & 0x8000;
			data <<= 1;
			switch (pattern.DrawMode) {
			    case JAM1:
				if (inversion)
				    bit_set = !bit_set;
				if (bit_set)
				    PixelWrite (uae_mem2, bits, pattern.FgPen, Bpp, Mask);
				break;
			    case JAM2:
				if (inversion)
				    bit_set = !bit_set;
				if (bit_set)
				    PixelWrite (uae_mem2, bits, pattern.FgPen, Bpp, Mask);
				else
				    PixelWrite (uae_mem2, bits, pattern.BgPen, Bpp, Mask);
				break;
			    case COMP:
				if (bit_set) {
				    fgpen = pattern.FgPen;

				    switch (Bpp) {
					case 1:
					    {
						uae_u8 *addr = uae_mem2 + bits;
						do_put_mem_byte (addr, (uae_u8)(do_get_mem_byte (addr) ^ 0xff));
					    }
					    break;
					case 2:
					    {
						uae_u16 *addr = ((uae_u16 *)uae_mem2) + bits;
						do_put_mem_word (addr, (uae_u16)(do_get_mem_word (addr) ^ fgpen));
					    }
					    break;
					case 3:
					    {
						uae_u32 *addr = (uae_u32 *)(uae_mem2 + bits * 3);
						do_put_mem_long (addr, do_get_mem_long (addr) ^ (fgpen & 0x00FFFFFF));
					    }
					    break;
					case 4:
					    {
						uae_u32 *addr = ((uae_u32 *)uae_mem2) + bits;
						do_put_mem_long (addr, do_get_mem_long (addr) ^ fgpen);
					    }
					    break;
				    }
				}
				break;
			}
		    }
		}
	    }

	    /* If we need to update a second-buffer (extra_mem is set), then do it only if visible! */
	    if (picasso_vidinfo.extra_mem && renderinfo_is_current_screen (&ri)) {
		    do_blit( &ri, Bpp, X, Y, X, Y, W, H, BLIT_SRC, 0);
	    }

	    result = 1;
	}
    }
    return result;
}

/*************************************************
BlitTemplate:
**************************************************
* Synopsis: BlitTemplate(bi, ri, template, X, Y, Width, Height, Mask, RGBFormat);
* a0: struct BoardInfo *bi
* a1: struct RenderInfo *ri
* a2: struct Template *template
* d0.w: X
* d1.w: Y
* d2.w: Width
* d3.w: Height
* d4.w: Mask
* d7.l: RGBFormat
*
* This function is used to paint a template on the board memory using the blitter.
* It is called by BltPattern and BltTemplate. The template consists of a b/w image
* using a single plane of image data which will be expanded to the destination RGBFormat
* using ForeGround and BackGround pens as well as draw modes.
***********************************************************************************/
uae_u32 REGPARAM2 picasso_BlitTemplate (struct regstruct *regs)
{
  uae_u8 inversion = 0;
  uaecptr rinf = m68k_areg (regs, 1);
  uaecptr tmpl = m68k_areg (regs, 2);
  unsigned long X = (uae_u16)m68k_dreg (regs, 0);
  unsigned long Y = (uae_u16)m68k_dreg (regs, 1);
  unsigned long W = (uae_u16)m68k_dreg (regs, 2);
  unsigned long H = (uae_u16)m68k_dreg (regs, 3);
  uae_u16 Mask = (uae_u16)m68k_dreg (regs, 4);
  struct Template tmp;
  struct RenderInfo ri;
  unsigned long rows;
  int bitoffset;
  uae_u32 fgpen;
  uae_u8 *uae_mem, Bpp;
  uae_u8 *tmpl_base;
  uae_u32 result = 0;

#ifdef JIT
  special_mem|=S_WRITE|S_READ;
#endif

  wgfx_flushline ();

  if (CopyRenderInfoStructureA2U (rinf, &ri) && CopyTemplateStructureA2U (tmpl, &tmp)) 
  {
	  Bpp = GetBytesPerPixel (ri.RGBFormat);
	  uae_mem = ri.Memory + Y*ri.BytesPerRow + X*Bpp; /* offset into address */

	  if (tmp.DrawMode & INVERS)
	    inversion = 1;

	  tmp.DrawMode &= 0x03;

	  if (Mask != 0xFF) 
    {
	    if (Bpp > 1)
		    Mask = 0xFF;

	    if (tmp.DrawMode == COMP) 
      {
		    write_log ("WARNING - BlitTemplate() has unhandled mask 0x%x with COMP DrawMode. Using fall-back routine.\n", Mask);
		    return 0;
	    } 
      else
	    {
		    result = 1;
      }
	  } 
    else
	  {
	    result = 1;

	  }
#if 1
	  if (tmp.DrawMode == COMP) {
	    /* workaround, let native blitter handle COMP mode */
	    return 0;
	  }
#endif

	  if (result) 
    {
	    bitoffset = tmp.XOffset % 8;

	    tmpl_base = (uae_u8 *)tmp.Memory + tmp.XOffset / 8;

	    for (rows = 0; rows < H; rows++, uae_mem += ri.BytesPerRow, tmpl_base += tmp.BytesPerRow) {
		    unsigned long cols;
    		uae_u8 *tmpl_mem = tmpl_base;
    		uae_u8 *uae_mem2 = uae_mem;
    		unsigned int data = *tmpl_mem;

		    for (cols = 0; cols < W; cols += 8, uae_mem2 += Bpp << 3) {
  		    unsigned int byte;
  		    long bits;
  		    long max = W - cols;

  		    if (max > 8)
  			    max = 8;

  		    data <<= 8;
  		    data |= *++tmpl_mem;
  
  		    byte = data >> (8 - bitoffset);

  		    for (bits = 0; bits < max; bits++) {
    			int bit_set = (byte & 0x80);
    			byte <<= 1;
    			switch (tmp.DrawMode) {
    			  case JAM1:
    				  if (inversion)
    				    bit_set = !bit_set;
    				  if (bit_set) {
    				    fgpen = tmp.FgPen;
    				    PixelWrite (uae_mem2, bits, fgpen, Bpp, Mask);
    				  }
				      break;
			      case JAM2:
				      if (inversion)
				        bit_set = !bit_set;
				      fgpen = tmp.BgPen;
				      if (bit_set)
				        fgpen = tmp.FgPen;
				      PixelWrite (uae_mem2, bits, fgpen, Bpp, Mask);
				      break;
			      case COMP:
				      if (bit_set) {
				        fgpen = tmp.FgPen;

				        switch (Bpp) {
        					case 1:
        					  {
        						  uae_u8 *addr = uae_mem2 + bits;
        						  do_put_mem_byte (addr, (uae_u8) (do_get_mem_byte (addr) ^ 0xff));
        					  }
        					  break;
        					case 2:
        					  {
        						  uae_u16 *addr = ((uae_u16 *)uae_mem2) + bits;
        						  do_put_mem_word (addr, (uae_u16) (do_get_mem_word (addr) ^ fgpen));
        					  }
        					  break;
        					case 3:
        					  {
        						  uae_u32 *addr = (uae_u32 *)(uae_mem2 + bits * 3);
        						  do_put_mem_long (addr, do_get_mem_long (addr) ^ (fgpen & 0x00FFFFFF));
        					  }
        					  break;
        					case 4:
        					  {
        						  uae_u32 *addr = ((uae_u32 *)uae_mem2) + bits;
        						  do_put_mem_long (addr, do_get_mem_long (addr) ^ fgpen);
        					  }
        					  break;
        				}
        			}
				      break;
			    }
		    }
		  }
	  }

	  /* If we need to update a second-buffer (extra_mem is set), then do it only if visible! */
	  if (picasso_vidinfo.extra_mem && renderinfo_is_current_screen (&ri)) {
		  do_blit (&ri, Bpp, X, Y, X, Y, W, H, BLIT_SRC, 0);
	  }
	  
    result = 1;
	  }
  }

  return 1;
}

/*
 * CalculateBytesPerRow:
 * a0: 	struct BoardInfo
 * d0: 	uae_u16 Width
 * d7:	RGBFTYPE RGBFormat
 * This function calculates the amount of bytes needed for a line of
 * "Width" pixels in the given RGBFormat.
 */
uae_u32 REGPARAM2 picasso_CalculateBytesPerRow (struct regstruct *regs)
{
    uae_u16 width = m68k_dreg (regs, 0);
    uae_u32 type = m68k_dreg (regs, 7);

    width = GetBytesPerPixel (type) * width;
    return width;
}

/*
 * SetDisplay:
 * a0:	struct BoardInfo
 * d0:	BOOL state
 * This function enables and disables the video display.
 *
 * NOTE: return the opposite of the state
 */
uae_u32 REGPARAM2 picasso_SetDisplay (struct regstruct *regs)
{
    uae_u32 state = m68k_dreg (regs, 0);
    return !state;
}

void init_hz_p96 (void)
{
}

/* NOTE: Watch for those planeptrs of 0x00000000 and 0xFFFFFFFF for all zero / all one bitmaps !!!! */
static void PlanarToChunky (struct RenderInfo *ri, struct BitMap *bm,
			    unsigned long srcx, unsigned long srcy,
			    unsigned long dstx, unsigned long dsty, 
          unsigned long width, unsigned long height, 
          uae_u8 mask)
{
  int j;

  uae_u8 *PLANAR[8], *image = ri->Memory + dstx * GetBytesPerPixel (ri->RGBFormat) + dsty * ri->BytesPerRow;
  int Depth = bm->Depth;
  unsigned long rows, bitoffset = srcx & 7;
  long eol_offset;

  /* if (mask != 0xFF)
     write_log ("P2C - pixel-width = %d, bit-offset = %d\n", width, bitoffset); */

  /* Set up our bm->Planes[] pointers to the right horizontal offset */
  for (j = 0; j < Depth; j++) {
	  uae_u8 *p = bm->Planes[j];
	  if (p != &all_zeros_bitmap && p != &all_ones_bitmap)
	    p += srcx / 8 + srcy * bm->BytesPerRow;
	  PLANAR[j] = p;
	  if ((mask & (1 << j)) == 0)
	    PLANAR[j] = &all_zeros_bitmap;
  }
  eol_offset = (long) bm->BytesPerRow - (long) ((width + 7) >> 3);
  for (rows = 0; rows < height; rows++, image += ri->BytesPerRow) {
	  unsigned long cols;

	  for (cols = 0; cols < width; cols += 8) {
	    int k;
	    uae_u32 a = 0, b = 0;
	    unsigned int msk = 0xFF;
	    long tmp = cols + 8 - width;
	    if (tmp > 0) {
		    msk <<= tmp;
		    b = do_get_mem_long ((uae_u32 *) (image + cols + 4));
		    if (tmp < 4)
		      b &= 0xFFFFFFFF >> (32 - tmp * 8);
		    else if (tmp > 4) {
		      a = do_get_mem_long ((uae_u32 *) (image + cols));
		      a &= 0xFFFFFFFF >> (64 - tmp * 8);
		    }
	    }
	    for (k = 0; k < Depth; k++) {
		    unsigned int data;
		    if (PLANAR[k] == &all_zeros_bitmap)
		      data = 0;
		    else if (PLANAR[k] == &all_ones_bitmap)
		      data = 0xFF;
		    else {
		      data = (uae_u8) (do_get_mem_word ((uae_u16 *) PLANAR[k]) >> (8 - bitoffset));
		      PLANAR[k]++;
		    }
		    data &= msk;
		    a |= p2ctab[data][0] << k;
		    b |= p2ctab[data][1] << k;
	    }
	    do_put_mem_long ((uae_u32 *) (image + cols), a);
	    do_put_mem_long ((uae_u32 *) (image + cols + 4), b);
	  }
	  for (j = 0; j < Depth; j++) {
	    if (PLANAR[j] != &all_zeros_bitmap && PLANAR[j] != &all_ones_bitmap) {
		    PLANAR[j] += eol_offset;
	    }
	  }
  }
}

/*
 * BlitPlanar2Chunky:
 * a0: struct BoardInfo *bi
 * a1: struct BitMap *bm - source containing planar information and assorted details
 * a2: struct RenderInfo *ri - dest area and its details
 * d0.w: SrcX
 * d1.w: SrcY
 * d2.w: DstX
 * d3.w: DstY
 * d4.w: SizeX
 * d5.w: SizeY
 * d6.b: MinTerm - uh oh!
 * d7.b: Mask - uh oh!
 *
 * This function is currently used to blit from planar bitmaps within system memory to chunky bitmaps
 * on the board. Watch out for plane pointers that are 0x00000000 (represents a plane with all bits "0")
 * or 0xffffffff (represents a plane with all bits "1").
 */
uae_u32 REGPARAM2 picasso_BlitPlanar2Chunky (struct regstruct *regs)
{
  uaecptr bm = m68k_areg (regs, 1);
  uaecptr ri = m68k_areg (regs, 2);
  unsigned long srcx = (uae_u16) m68k_dreg (regs, 0);
  unsigned long srcy = (uae_u16) m68k_dreg (regs, 1);
  unsigned long dstx = (uae_u16) m68k_dreg (regs, 2);
  unsigned long dsty = (uae_u16) m68k_dreg (regs, 3);
  unsigned long width = (uae_u16) m68k_dreg (regs, 4);
  unsigned long height = (uae_u16) m68k_dreg (regs, 5);
  uae_u8 minterm = m68k_dreg (regs, 6) & 0xFF;
  uae_u8 mask = m68k_dreg (regs, 7) & 0xFF;
  struct RenderInfo local_ri;
  struct BitMap local_bm;
  uae_u32 result = 0;

#ifdef JIT
  special_mem|=S_WRITE|S_READ;
#endif

  wgfx_flushline ();

  if (minterm != 0x0C) {
  	write_log ("ERROR - BlitPlanar2Chunky() has minterm 0x%x, which I don't handle. Using fall-back routine.\n", 
      minterm);
  }
  else if (CopyRenderInfoStructureA2U (ri, &local_ri) && 
    CopyBitMapStructureA2U (bm, &local_bm))
  {
    PlanarToChunky (&local_ri, &local_bm, srcx, srcy, dstx, dsty, width, height, mask);
    if (renderinfo_is_current_screen (&local_ri))
	  {
	    do_blit (&local_ri, GetBytesPerPixel (local_ri.RGBFormat), dstx, dsty, dstx, dsty, width, height, BLIT_SRC, 0);
	  }
	  result = 1;
  }

  return result;
}

/* NOTE: Watch for those planeptrs of 0x00000000 and 0xFFFFFFFF for all zero / all one bitmaps !!!! */
static void PlanarToDirect (struct RenderInfo *ri, struct BitMap *bm,
			    unsigned long srcx, unsigned long srcy,
			    unsigned long dstx, unsigned long dsty,
			    unsigned long width, unsigned long height, uae_u8 mask, 
          struct ColorIndexMapping *cim)
{
  int j;
  int bpp = GetBytesPerPixel (ri->RGBFormat);
  uae_u8 *PLANAR[8];
  uae_u8 *image = ri->Memory + dstx * bpp + dsty * ri->BytesPerRow;
  int Depth = bm->Depth;
  unsigned long rows;
  long eol_offset;

  if( !bpp )
  	return;

  /* Set up our bm->Planes[] pointers to the right horizontal offset */
  for (j = 0; j < Depth; j++) {
	  uae_u8 *p = bm->Planes[j];
	  if (p != &all_zeros_bitmap && p != &all_ones_bitmap)
	    p += srcx / 8 + srcy * bm->BytesPerRow;
	  PLANAR[j] = p;
	  if ((mask & (1 << j)) == 0)
	    PLANAR[j] = &all_zeros_bitmap;
  }

  eol_offset = (long) bm->BytesPerRow - (long) ((width + (srcx & 7)) >> 3);
  for (rows = 0; rows < height; rows++, image += ri->BytesPerRow) {
  	unsigned long cols;
	  uae_u8 *image2 = image;
	  unsigned int bitoffs = 7 - (srcx & 7);
	  int i;

	  for (cols = 0; cols < width; cols++) {
	    int v = 0, k;
	    for (k = 0; k < Depth; k++) {
		    if (PLANAR[k] == &all_ones_bitmap)
		      v |= 1 << k;
		    else if (PLANAR[k] != &all_zeros_bitmap) {
		      v |= ((*PLANAR[k] >> bitoffs) & 1) << k;
		    }
	    }

	    switch (bpp) {
  	    case 2:
	      	do_put_mem_word ((uae_u16 *) image2, (uae_u16)(cim->Colors[v]));
		      image2 += 2;
		      break;
	      case 3:
      		do_put_mem_byte (image2++, (uae_u8)cim->Colors[v]);
      		do_put_mem_word ((uae_u16 *) image2, (uae_u16)((cim->Colors[v] & 0x00FFFF00) >> 8));
      		image2 += 2;
      		break;
  	    case 4:
	      	do_put_mem_long ((uae_u32 *) image2, cim->Colors[v]);
      		image2 += 4;
      		break;
	    }
	    bitoffs--;
	    bitoffs &= 7;
	    if (bitoffs == 7) {
    		int k;
		    for (k = 0; k < Depth; k++) {
		      if (PLANAR[k] != &all_zeros_bitmap && PLANAR[k] != &all_ones_bitmap) {
			      PLANAR[k]++;
		      }
		    }
	    }
  	}

  	for (i = 0; i < Depth; i++) {
	    if (PLANAR[i] != &all_zeros_bitmap && PLANAR[i] != &all_ones_bitmap) {
	    	PLANAR[i] += eol_offset;
	    }
	  }
  }
}

/*
 * BlitPlanar2Direct:
 *
 * Synopsis:
 * BlitPlanar2Direct(bi, bm, ri, cim, SrcX, SrcY, DstX, DstY, SizeX, SizeY, MinTerm, Mask);
 * Inputs:
 * a0:struct BoardInfo *bi
 * a1:struct BitMap *bm
 * a2:struct RenderInfo *ri
 * a3:struct ColorIndexMapping *cmi
 * d0.w:SrcX
 * d1.w:SrcY
 * d2.w:DstX
 * d3.w:DstY
 * d4.w:SizeX
 * d5.w:SizeY
 * d6.b:MinTerm
 * d7.b:Mask
 *
 * This function is currently used to blit from planar bitmaps within system memory to direct color
 * bitmaps (15, 16, 24 or 32 bit) on the board. Watch out for plane pointers that are 0x00000000 (represents
 * a plane with all bits "0") or 0xffffffff (represents a plane with all bits "1"). The ColorIndexMapping is
 * used to map the color index of each pixel formed by the bits in the bitmap's planes to a direct color value
 * which is written to the destination RenderInfo. The color mask and all colors within the mapping are words,
 * triple bytes or longwords respectively similar to the color values used in FillRect(), BlitPattern() or
 * BlitTemplate().
 */

uae_u32 REGPARAM2 picasso_BlitPlanar2Direct (struct regstruct *regs)
{
  uaecptr bm = m68k_areg (regs, 1);
  uaecptr ri = m68k_areg (regs, 2);
  uaecptr cim = m68k_areg (regs, 3);
  unsigned long srcx = (uae_u16)m68k_dreg (regs, 0);
  unsigned long srcy = (uae_u16)m68k_dreg (regs, 1);
  unsigned long dstx = (uae_u16)m68k_dreg (regs, 2);
  unsigned long dsty = (uae_u16)m68k_dreg (regs, 3);
  unsigned long width = (uae_u16)m68k_dreg (regs, 4);
  unsigned long height = (uae_u16)m68k_dreg (regs, 5);
  uae_u8 minterm = m68k_dreg (regs, 6);
  uae_u8 Mask = m68k_dreg (regs, 7);
  struct RenderInfo local_ri;
  struct BitMap local_bm;
  struct ColorIndexMapping local_cim;
  uae_u32 result = 0;

#ifdef JIT
  special_mem|=S_WRITE|S_READ;
#endif

  wgfx_flushline ();

  if (minterm != 0x0C) {
  	write_log ("WARNING - BlitPlanar2Direct() has unhandled op-code 0x%x. Using fall-back routine.\n", 
      minterm);
  } 
  else if (CopyRenderInfoStructureA2U (ri, &local_ri) && 
    CopyBitMapStructureA2U (bm, &local_bm)) 
  {
	  Mask = 0xFF;
	  CopyColorIndexMappingA2U (cim, &local_cim);

	  PlanarToDirect (&local_ri, &local_bm, srcx, srcy, dstx, dsty, width, height, Mask, &local_cim);
	  if (renderinfo_is_current_screen (&local_ri))
	    do_blit( &local_ri, GetBytesPerPixel( local_ri.RGBFormat ), dstx, dsty, dstx, dsty, width, height, BLIT_SRC, 0);
	  result = 1;
  }

  return result;
}

STATIC_INLINE void write_gfx_x (uaecptr addr, uae_u32 value, int size)
{
  int y;
  uaecptr oldaddr = addr;

  if (!picasso_on)
  	return;
  
  /*
   * Several writes to successive memory locations are a common access pattern.
   * Try to optimize it.
   */
  if (addr >= wgfx_linestart && addr + size <= wgfx_lineend) {
	  if (addr < wgfx_min)
	    wgfx_min = addr;
	  if (addr + size > wgfx_max)
	    wgfx_max = addr + size;
	  return;
  } else
	  wgfx_flushline ();

  addr += gfxmem_start;
  /* Check to see if this needs to be written through to the display, or was it an "offscreen" area? */
  if (addr < picasso96_state.Address || addr + size >= picasso96_state.Extent)
  	return;

  addr -= picasso96_state.Address+(picasso96_state.XOffset*picasso96_state.BytesPerPixel)
  	+(picasso96_state.YOffset*picasso96_state.BytesPerRow);
  	
  y = addr / picasso96_state.BytesPerRow;

  if (y >= picasso96_state.Height)
	  return;
  wgfx_linestart = picasso96_state.Address - gfxmem_start + y * picasso96_state.BytesPerRow;
  wgfx_lineend = wgfx_linestart + picasso96_state.BytesPerRow;
  wgfx_y = y;
  wgfx_min = oldaddr;
  wgfx_max = oldaddr + size;
}

static uae_u32 REGPARAM2 gfxmem_lget (uaecptr addr)
{
    uae_u32 *m;

#ifdef JIT
    special_mem |= S_READ;
#endif
    addr -= gfxmem_start;
    addr &= gfxmem_mask;
    m = (uae_u32 *)(gfxmemory + addr);
    return do_get_mem_long(m);
}

static uae_u32 REGPARAM2 gfxmem_wget (uaecptr addr)
{
    uae_u16 *m;
#ifdef JIT
    special_mem |= S_READ;
#endif
    addr -= gfxmem_start;
    addr &= gfxmem_mask;
    m = (uae_u16 *)(gfxmemory + addr);
    return do_get_mem_word(m);
}

static uae_u32 REGPARAM2 gfxmem_bget (uaecptr addr)
{
    uae_u8 *m;
#ifdef JIT
    special_mem |= S_READ;
#endif
    addr -= gfxmem_start;
    addr &= gfxmem_mask;
    m = (uae_u8 *)(gfxmemory + addr);
    return do_get_mem_byte(m);
}

static void REGPARAM2 gfxmem_lput (uaecptr addr, uae_u32 l)
{
    uae_u32 *m;
#ifdef JIT
    special_mem |= S_WRITE;
#endif
    addr -= gfxmem_start;
    addr &= gfxmem_mask;
    m = (uae_u32 *)(gfxmemory + addr);
	  do_put_mem_long(m, l);

    /* write the long-word to our displayable memory */
    write_gfx_x (addr, l, 4);
}

static void REGPARAM2 gfxmem_wput (uaecptr addr, uae_u32 w)
{
    uae_u16 *m;
#ifdef JIT
    special_mem |= S_WRITE;
#endif
    addr -= gfxmem_start;
    addr &= gfxmem_mask;
    m = (uae_u16 *)(gfxmemory + addr);
    do_put_mem_word(m, (uae_u16)w);
    
    /* write the word to our displayable memory */
    write_gfx_x (addr, (uae_u16) w, 2);
}

static void REGPARAM2 gfxmem_bput (uaecptr addr, uae_u32 b)
{
    uae_u8 *m;
#ifdef JIT
    special_mem |= S_WRITE;
#endif
    addr -= gfxmem_start;
    addr &= gfxmem_mask;
    m = (uae_u8 *)(gfxmemory + addr);
    do_put_mem_byte(m, b);
    
    /* write the byte to our displayable memory */
    write_gfx_x (addr, (uae_u8) b, 1);
}

static uae_u32 REGPARAM2 gfxmem_lgetx (uaecptr addr)
{
    uae_u32 *m;

    addr -= gfxmem_start & gfxmem_mask;
    addr &= gfxmem_mask;
    m = (uae_u32 *)(gfxmemory + addr);
    return do_get_mem_long(m);
}

static uae_u32 REGPARAM2 gfxmem_wgetx (uaecptr addr)
{
    uae_u16 *m;
    addr -= gfxmem_start & gfxmem_mask;
    addr &= gfxmem_mask;
    m = (uae_u16 *)(gfxmemory + addr);
    return do_get_mem_word(m);
}

static uae_u32 REGPARAM2 gfxmem_bgetx (uaecptr addr)
{
    addr -= gfxmem_start & gfxmem_mask;
    addr &= gfxmem_mask;
    return gfxmemory[addr];
}

static void REGPARAM2 gfxmem_lputx (uaecptr addr, uae_u32 l)
{
    uae_u32 *m;
    addr -= gfxmem_start & gfxmem_mask;
    addr &= gfxmem_mask;
    m = (uae_u32 *)(gfxmemory + addr);
    do_put_mem_long(m, l);
}

static void REGPARAM2 gfxmem_wputx (uaecptr addr, uae_u32 w)
{
    uae_u16 *m;
    addr -= gfxmem_start & gfxmem_mask;
    addr &= gfxmem_mask;
    m = (uae_u16 *)(gfxmemory + addr);
    do_put_mem_word(m, (uae_u16)w);
}

static void REGPARAM2 gfxmem_bputx (uaecptr addr, uae_u32 b)
{
    addr -= gfxmem_start & gfxmem_mask;
    addr &= gfxmem_mask;
    gfxmemory[addr] = b;
}

static int REGPARAM2 gfxmem_check (uaecptr addr, uae_u32 size)
{
    addr -= gfxmem_start /*& gfxmem_mask*/;
    addr &= gfxmem_mask;
    return (addr + size) < allocated_gfxmem;
}

static uae_u8 *REGPARAM2 gfxmem_xlate (uaecptr addr)
{
    addr -= gfxmem_start /*& gfxmem_mask*/;
    addr &= gfxmem_mask;
    return gfxmemory + addr;
}

addrbank gfxmem_bank = {
    gfxmem_lget, gfxmem_wget, gfxmem_bget,
    gfxmem_lput, gfxmem_wput, gfxmem_bput,
    gfxmem_xlate, gfxmem_check, NULL, "RTG RAM",
    dummy_lgeti, dummy_wgeti, ABFLAG_RAM
};

addrbank gfxmem_bankx = {
    gfxmem_lgetx, gfxmem_wgetx, gfxmem_bgetx,
    gfxmem_lputx, gfxmem_wputx, gfxmem_bputx,
    gfxmem_xlate, gfxmem_check, NULL, "RTG RAM",
    dummy_lgeti, dummy_wgeti, ABFLAG_RAM
};

static int resolution_compare (const void *a, const void *b)
{
    struct PicassoResolution *ma = (struct PicassoResolution *) a;
    struct PicassoResolution *mb = (struct PicassoResolution *) b;
    if (ma->res.width > mb->res.width)
	return -1;
    if (ma->res.width < mb->res.width)
	return 1;
    if (ma->res.height > mb->res.height)
	return -1;
    if (ma->res.height < mb->res.height)
	return 1;
    return ma->depth - mb->depth;
}

/* Call this function first, near the beginning of code flow
* Place in InitGraphics() which seems reasonable...
* Also put it in reset_drawing() for safe-keeping.  */
void InitPicasso96 (void)
{
	int i;

  have_done_picasso = 0;
  palette_changed = 0;
	memset (&picasso96_state, 0, sizeof (struct picasso96_state_struct));

	for (i = 0; i < 256; i++) {
  	p2ctab[i][0] = (((i & 128) ? 0x01000000 : 0)
  		| ((i & 64) ? 0x010000 : 0)
  		| ((i & 32) ? 0x0100 : 0)
  		| ((i & 16) ? 0x01 : 0));
  	p2ctab[i][1] = (((i & 8) ? 0x01000000 : 0)
  		| ((i & 4) ? 0x010000 : 0)
  		| ((i & 2) ? 0x0100 : 0)
  		| ((i & 1) ? 0x01 : 0));
  }
  mode_count = DX_FillResolutions (&picasso96_pixel_format);
  qsort (DisplayModes, mode_count, sizeof (struct PicassoResolution), resolution_compare);
}


uae_u8 *restore_p96 (uae_u8 *src)
{
    uae_u32 flags;
    if (restore_u32 () != 1)
	return src;
    InitPicasso96();
    flags = restore_u32();
    picasso_requested_on = !!(flags & 1);
    picasso96_state.SwitchState = picasso_requested_on;
    picasso_on = 0;
    init_picasso_screen_called = 0;
    set_gc_called = !!(flags & 2);
    set_panning_called = !!(flags & 4);
    changed_prefs.gfxmem_size = restore_u32(); 
    picasso96_state.Address = restore_u32();
    picasso96_state.RGBFormat = restore_u32();
    picasso96_state.Width = restore_u16();
    picasso96_state.Height = restore_u16();
    picasso96_state.VirtualWidth = restore_u16();
    picasso96_state.VirtualHeight = restore_u16();
    picasso96_state.XOffset = restore_u16();
    picasso96_state.YOffset = restore_u16();
    picasso96_state.GC_Depth = restore_u8();
    picasso96_state.GC_Flags = restore_u8();
    picasso96_state.BytesPerRow = restore_u16();
    picasso96_state.BytesPerPixel = restore_u8();
    picasso96_state.HostAddress = NULL;
    picasso_SetPanningInit();
    picasso96_state.Extent = picasso96_state.Address + picasso96_state.BytesPerRow * picasso96_state.VirtualHeight;
    if (set_gc_called) {
	init_picasso_screen ();
	init_hz_p96 ();
    }
    return src;
}

uae_u8 *save_p96 (int *len, uae_u8 *dstptr)
{
    uae_u8 *dstbak,*dst;

    if (currprefs.gfxmem_size == 0)
	return NULL;
    if (dstptr)
	dstbak = dst = dstptr;
    else
	dstbak = dst = (uae_u8 *)malloc (1000);
    save_u32 (1);
    save_u32 ((picasso_on ? 1 : 0) | (set_gc_called ? 2 : 0) | (set_panning_called ? 4 : 0));
    save_u32 (currprefs.gfxmem_size);
    save_u32 (picasso96_state.Address);
    save_u32 (picasso96_state.RGBFormat);
    save_u16 (picasso96_state.Width);
    save_u16 (picasso96_state.Height);
    save_u16 (picasso96_state.VirtualWidth);
    save_u16 (picasso96_state.VirtualHeight);
    save_u16 (picasso96_state.XOffset);
    save_u16 (picasso96_state.YOffset);
    save_u8 (picasso96_state.GC_Depth);
    save_u8 (picasso96_state.GC_Flags);
    save_u16 (picasso96_state.BytesPerRow);
    save_u8 (picasso96_state.BytesPerPixel);
    *len = dst - dstbak;
    return dstbak;
}

#endif //picasso96
