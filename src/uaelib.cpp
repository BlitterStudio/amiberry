/*
 * UAE - The U*nix Amiga Emulator
 *
 * UAE Library v0.1
 *
 * (c) 1996 Tauno Taipaleenmaki <tataipal@raita.oulu.fi>
 *
 * Change UAE parameters and other stuff from inside the emulation.
 */

#include "sysconfig.h"
#include "sysdeps.h"

#include <assert.h>
#include <string.h>

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "xwin.h"
#include "autoconf.h"
#include "traps.h"
#include "disk.h"
#include "debug.h"
#include "gensound.h"
#include "picasso96.h"
#include "filesys.h"

/*
 * Returns UAE Version
 */
static uae_u32 emulib_GetVersion (void)
{
    return version;
}

/*
 * Resets your amiga
 */
static uae_u32 emulib_HardReset (void)
{
    uae_reset(0);
    return 0;
}

static uae_u32 emulib_Reset (void)
{
    uae_reset(0);
    return 0;
}

/*
 * Enables SOUND
 */
static uae_u32 emulib_EnableSound (uae_u32 val)
{
    if (!sound_available || currprefs.produce_sound == 2)
	return 0;

    currprefs.produce_sound = val;
    return 1;
}

/*
 * Enables FAKE JOYSTICK
 */
static uae_u32 emulib_EnableJoystick (uae_u32 val)
{
    return 0;
}

/*
 * Sets the framerate
 */
static uae_u32 emulib_SetFrameRate (uae_u32 val)
{
    if (val == 0)
	return 0;
    else if (val > 20)
	return 0;
    else {
	currprefs.gfx_framerate = val;
	return 1;
    }
}

/*
 * Changes keyboard language settings
 */
static uae_u32 emulib_ChangeLanguage (uae_u32 which)
{
  if(which > 0)	
    return 0;
  return 1;
}

/* The following ones don't work as we never realloc the arrays... */
/*
 * Changes chip memory size
 *  (reboots)
 */
static uae_u32 REGPARAM2 emulib_ChgCMemSize (struct regstruct *regs, uae_u32 memsize)
{
    if (memsize != 0x80000 && memsize != 0x100000 &&
	memsize != 0x200000) {
	memsize = 0x200000;
	write_log ("Unsupported chipmem size!\n");
    }
    m68k_dreg (regs, 0) = 0;

    currprefs.chipmem_size = memsize;
    uae_reset(0);
    return 1;
}

/*
 * Changes slow memory size
 *  (reboots)
 */
static uae_u32 REGPARAM2 emulib_ChgSMemSize (struct regstruct *regs, uae_u32 memsize)
{
    if (memsize != 0x80000 && memsize != 0x100000 &&
	memsize != 0x180000 && memsize != 0x1C0000) {
	memsize = 0;
	write_log ("Unsupported bogomem size!\n");
    }

    m68k_dreg (regs, 0) = 0;
    currprefs.bogomem_size = memsize;
    uae_reset (0);
    return 1;
}

/*
 * Changes fast memory size
 *  (reboots)
 */
static uae_u32 REGPARAM2 emulib_ChgFMemSize (struct regstruct *regs, uae_u32 memsize)
{
    if (memsize != 0x100000 && memsize != 0x200000 &&
	memsize != 0x400000 && memsize != 0x800000) {
	memsize = 0;
	write_log ("Unsupported fastmem size!\n");
    }
    m68k_dreg (regs, 0) = 0;
    currprefs.fastmem_size = memsize;
    uae_reset (0);
    return 0;
}

/*
 * Inserts a disk
 */
static uae_u32 emulib_InsertDisk (uaecptr name, uae_u32 drive)
{
    int i = 0;
    char real_name[256];

    if (drive > 3)
	return 0;

    while ((real_name[i] = get_byte (name + i)) != 0 && i++ != 254)
	;

    if (i == 255)
	return 0; /* ENAMETOOLONG */

    strcpy (changed_prefs.df[drive], real_name);

    return 1;
}

/*
 * Exits the emulator
 */
static uae_u32 emulib_ExitEmu (void)
{
    uae_quit ();
    return 1;
}

/*
 * Gets UAE Configuration
 */
static uae_u32 emulib_GetUaeConfig (uaecptr place)
{
    int i;

    put_long (place, version);
    put_long (place + 4, allocated_chipmem);
    put_long (place + 8, allocated_bogomem);
    put_long (place + 12, allocated_fastmem);
    put_long (place + 16, currprefs.gfx_framerate);
    put_long (place + 20, currprefs.produce_sound);
    put_long (place + 24, 0);
    put_long (place + 28, 0);
    if (disk_empty (0))
	put_byte (place + 32, 0);
    else
	put_byte (place + 32, 1);
    if (disk_empty (1))
	put_byte (place + 33, 0);
    else
	put_byte (place + 33, 1);
    if (disk_empty(2))
	put_byte (place + 34, 0);
    else
	put_byte (place + 34, 1);
    if (disk_empty(3))
	put_byte (place + 35, 0);
    else
	put_byte (place + 35, 1);

    for (i = 0; i < 256; i++) {
	put_byte ((place + 36 + i), currprefs.df[0][i]);
	put_byte ((place + 36 + i + 256), currprefs.df[1][i]);
	put_byte ((place + 36 + i + 512), currprefs.df[2][i]);
	put_byte ((place + 36 + i + 768), currprefs.df[3][i]);
    }
    return 1;
}

/*
 * Sets UAE Configuration
 *
 * NOT IMPLEMENTED YET
 */
static uae_u32 emulib_SetUaeConfig (uaecptr place)
{
    return 1;
}

/*
 * Gets the name of the disk in the given drive
 */
static uae_u32 emulib_GetDisk (uae_u32 drive, uaecptr name)
{
    int i;
    if (drive > 3)
	return 0;

    for (i = 0;i < 256; i++) {
	put_byte (name + i, currprefs.df[drive][i]);
    }
    return 1;
}

/*
 * Enter debugging state
 */
static uae_u32 emulib_Debug (void)
{
#ifdef DEBUGGER
    activate_debugger ();
    return 1;
#else
    return 0;
#endif
}

/* We simply find the first "text" hunk, get the offset of its actual code segment (20 bytes away)
 * and add that offset to the base address of the object.  Now we've got code to execute.
 *
 * @@@ Brian: does anything actually use this yet?
 * @@@ Bernd: Not yet.  It needs to get much better.  Should spawn off a seperate task to handle the
 *            function, and then somehow "signal" the Amiga caller that completion or error has
 *            occurred.  I don't know how to do that, so right now it is a synchronous call.  Yuck!
 *            Would be nice to implement jpg decompression functionality for the Amiga which used
 *            the UAE Host to do all the work, for example.
 * @@@ Brian: I disabled it to prevent people from starting to use it - if that happens, we're
 *            stuck with this.
 */
static uae_u32 FindFunctionInObject (uae_u8 *objectptr)
{
    uae_u8 *text_hdr;
    uae_u8 offset;
    text_hdr = (uae_u8 *)strstr ("text", (char *)objectptr);
    if (text_hdr != 0) {
	offset = *(text_hdr + 19);
	return (uae_u32)(objectptr + offset);
    }
    return 0;
}

#define CREATE_NATIVE_FUNC_PTR uae_u32 (* native_func)( uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, \
						 uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32)
#define SET_NATIVE_FUNC(x) native_func = (uae_u32 (*)(uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32))(x)
#define CALL_NATIVE_FUNC( d1,d2,d3,d4,d5,d6,d7,a1,a2,a3,a4,a5,a6 ) if(native_func) native_func( d1,d2,d3,d4,d5,d6,d7,a1,a2,a3,a4,a5,a6 )
/* A0 - Contains a ptr to the native .obj data.  This ptr is Amiga-based. */
/*      We simply find the first function in this .obj data, and execute it. */
static uae_u32 REGPARAM2 emulib_ExecuteNativeCode (struct regstruct *regs)
{
#if 0
    uaecptr object_AAM = m68k_areg (regs, 0);
    uae_u32 d1 = m68k_dreg (regs, 1);
    uae_u32 d2 = m68k_dreg (regs, 2);
    uae_u32 d3 = m68k_dreg (regs, 3);
    uae_u32 d4 = m68k_dreg (regs, 4);
    uae_u32 d5 = m68k_dreg (regs, 5);
    uae_u32 d6 = m68k_dreg (regs, 6);
    uae_u32 d7 = m68k_dreg (regs, 7);
    uae_u32 a1 = m68k_areg (regs, 1);
    uae_u32 a2 = m68k_areg (regs, 2);
    uae_u32 a3 = m68k_areg (regs, 3);
    uae_u32 a4 = m68k_areg (regs, 4);
    uae_u32 a5 = m68k_areg (regs, 5);
    uae_u32 a6 = m68k_areg (regs, 6);

    uae_u8* object_UAM = NULL;
    CREATE_NATIVE_FUNC_PTR;

    if( get_mem_bank( object_AAM ).check( object_AAM, 1 ) )
	object_UAM = get_mem_bank( object_AAM).xlateaddr( object_AAM );

    if(object_UAM) {
	SET_NATIVE_FUNC( FindFunctionInObject( object_UAM ) );
	CALL_NATIVE_FUNC( d1, d2, d3, d4, d5, d6, d7, a1, a2, a3, a4, a5, a6);
    }
    return 1;
#endif
    return 0;
}

static uae_u32 emulib_Minimize (void)
{
    return 0; // OSDEP_minimize_uae();
}

static int native_dos_op (uae_u32 mode, uae_u32 p1, uae_u32 p2, uae_u32 p3)
{
    char tmp[MAX_DPATH];
    int v, i;

    if (mode)
	return -1;
    /* receive native path from lock
     * p1 = dos.library:Lock, p2 = buffer, p3 = max buffer size 
     */
    v = get_native_path (p1, tmp);
    if (v)
	return v;
    for (i = 0; i <= strlen(tmp) && i < p3 - 1; i++) {
        put_byte (p2 + i, tmp[i]);
        put_byte (p2 + i + 1, 0);
    }
    return 0;
}

static uae_u32 REGPARAM2 uaelib_demux2 (TrapContext *context)
{
#define ARG0 (get_long (m68k_areg (&context->regs, 7) + 4))
#define ARG1 (get_long (m68k_areg (&context->regs, 7) + 8))
#define ARG2 (get_long (m68k_areg (&context->regs, 7) + 12))
#define ARG3 (get_long (m68k_areg (&context->regs, 7) + 16))
#define ARG4 (get_long (m68k_areg (&context->regs, 7) + 20))
#define ARG5 (get_long (m68k_areg (&context->regs, 7) + 24))

    switch (ARG0) 
    {
     case 0: return emulib_GetVersion ();
     case 1: return emulib_GetUaeConfig (ARG1);
     case 2: return emulib_SetUaeConfig (ARG1);
     case 3: return emulib_HardReset ();
     case 4: return emulib_Reset ();
     case 5: return emulib_InsertDisk (ARG1, ARG2);
     case 6: return emulib_EnableSound (ARG1);
     case 7: return emulib_EnableJoystick (ARG1);
     case 8: return emulib_SetFrameRate (ARG1);
     case 9: return emulib_ChgCMemSize (&context->regs, ARG1);
     case 10: return emulib_ChgSMemSize (&context->regs, ARG1);
     case 11: return emulib_ChgFMemSize (&context->regs, ARG1);
     case 12: return emulib_ChangeLanguage (ARG1);
	/* The next call brings bad luck */
     case 13: return emulib_ExitEmu ();
     case 14: return emulib_GetDisk (ARG1, ARG2);
     case 15: return emulib_Debug ();

     case 68: return emulib_Minimize ();
     case 69: return emulib_ExecuteNativeCode (&context->regs);

     case 70: return 0; /* RESERVED. Something uses this.. */

     case 80: return 0xffffffff;
     case 81: return 0;
     case 82: return 0;
     case 83: return 0;
#ifdef DEBUGGER
     case 84: return mmu_init (ARG1, ARG2, ARG3);
#endif
     case 85: return native_dos_op (ARG1, ARG2, ARG3, ARG4);
     case 86:
    	 if (valid_address(ARG1, 1))
    	    write_log("DBG: %s\n", get_real_address(ARG1));
    	 return 1; 
    }
    return 0;
}

static uae_u32 REGPARAM2 uaelib_demux (TrapContext *context)
{
    uae_u32 v;
    struct regstruct *r = &context->regs;

    if (ARG0 >= 16 && ARG0 <= 39) {
	write_log ("uaelib: obsolete Picasso96 uaelib hook called, call ignored\n");
	return 0;
    }
    v = uaelib_demux2 (context);
    return v;
}

/*
 * Installs the UAE LIBRARY
 */
void emulib_install (void)
{
    uaecptr a;
    if (!uae_boot_rom)
	return;
    a = here ();
    org (rtarea_base + 0xFF60);
#if 0
    dw (0x4eb9);
    dw ((rtarea_base >> 16) | get_word(rtarea_base + 36));
    dw (get_word(rtarea_base + 38) + 12);
#endif
    calltrap (deftrapres (uaelib_demux, 0, "uaelib_demux"));
    dw (RTS);
    org (a);
}
