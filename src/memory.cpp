 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Memory management
  *
  * (c) 1995 Bernd Schmidt
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "include/memory.h"
#include "rommgr.h"
#include "ersatz.h"
#include "zfile.h"
#include "custom.h"
#include "newcpu.h"
#include "autoconf.h"
#include "savestate.h"
#include "crc32.h"
#include "gui.h"
#include "akiko.h"
#include "gfxboard.h"

#ifdef JIT
/* Set by each memory handler that does not simply access real memory. */
int special_mem;
#endif
static int mem_hardreset;

static size_t bootrom_filepos, chip_filepos, bogo_filepos, rom_filepos;

/* Set if we notice during initialization that settings changed,
   and we must clear all memory to prevent bogus contents from confusing
   the Kickstart.  */
static bool need_hardreset;

/* The address space setting used during the last reset.  */
static bool last_address_space_24;

addrbank *mem_banks[MEMORY_BANKS];


int addr_valid(const TCHAR *txt, uaecptr addr, uae_u32 len)
{
  addrbank *ab = &get_mem_bank(addr);
  if (ab == 0 || !(ab->flags & (ABFLAG_RAM | ABFLAG_ROM)) || addr < 0x100 || len < 0 || len > 16777215 || !valid_address(addr, len)) {
		write_log (_T("corrupt %s pointer %x (%d) detected!\n"), txt, addr, len);
  	return 0;
  }
  return 1;
}

/* A dummy bank that only contains zeros */

static uae_u32 REGPARAM3 dummy_lget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 dummy_wget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 dummy_bget (uaecptr) REGPARAM;
static void REGPARAM3 dummy_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 dummy_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 dummy_bput (uaecptr, uae_u32) REGPARAM;
static int REGPARAM3 dummy_check (uaecptr addr, uae_u32 size) REGPARAM;

#define NONEXISTINGDATA 0
//#define NONEXISTINGDATA 0xffffffff

STATIC_INLINE uae_u32 dummy_get (uaecptr addr, int size, bool inst)
{
	uae_u32 v = NONEXISTINGDATA;

	if (currprefs.cpu_model >= 68040)
		return v;
	if (!currprefs.cpu_compatible)
		return v;
	if (currprefs.address_space_24)
		addr &= 0x00ffffff;
	if (addr >= 0x10000000)
		return v;
	if ((currprefs.cpu_model <= 68010) || (currprefs.cpu_model == 68020 && (currprefs.chipset_mask & CSMASK_AGA) && currprefs.address_space_24)) {
	  if (size == 4) {
    	v = (regs.irc << 16) | regs.irc;
	  } else if (size == 2) {
		  v = regs.irc & 0xffff;
	  } else {
		  v = regs.irc;
		  v = (addr & 1) ? (v & 0xff) : ((v >> 8) & 0xff);
		}
	 }
	return v;
}

static uae_u32 REGPARAM2 dummy_lget (uaecptr addr)
{
#ifdef JIT
  special_mem |= S_READ;
#endif
	return dummy_get (addr, 4, false);
}
uae_u32 REGPARAM2 dummy_lgeti (uaecptr addr)
{
#ifdef JIT
  special_mem |= S_READ;
#endif
	return dummy_get (addr, 4, true);
}

static uae_u32 REGPARAM2 dummy_wget (uaecptr addr)
{
#ifdef JIT
  special_mem |= S_READ;
#endif
	return dummy_get (addr, 2, false);
}
uae_u32 REGPARAM2 dummy_wgeti (uaecptr addr)
{
#ifdef JIT
  special_mem |= S_READ;
#endif
	return dummy_get (addr, 2, true);
}

static uae_u32 REGPARAM2 dummy_bget (uaecptr addr)
{
#ifdef JIT
  special_mem |= S_READ;
#endif
	return dummy_get (addr, 1, false);
}

static void REGPARAM2 dummy_lput (uaecptr addr, uae_u32 l)
{
#ifdef JIT
  special_mem |= S_WRITE;
#endif
}

static void REGPARAM2 dummy_wput (uaecptr addr, uae_u32 w)
{
#ifdef JIT
  special_mem |= S_WRITE;
#endif
}

static void REGPARAM2 dummy_bput (uaecptr addr, uae_u32 b)
{
#ifdef JIT
  special_mem |= S_WRITE;
#endif
}

static int REGPARAM2 dummy_check (uaecptr addr, uae_u32 size)
{
#ifdef JIT
  special_mem |= S_READ;
#endif
  return 0;
}

/* Chip memory */

uae_u32 chipmem_full_mask;

static int REGPARAM3 chipmem_check (uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *REGPARAM3 chipmem_xlate (uaecptr addr) REGPARAM;

uae_u32 REGPARAM2 chipmem_lget (uaecptr addr)
{
  uae_u32 *m;

  addr &= chipmem_bank.mask;
  m = (uae_u32 *)(chipmem_bank.baseaddr + addr);
  return do_get_mem_long (m);
}

static uae_u32 REGPARAM2 chipmem_wget (uaecptr addr)
{
  uae_u16 *m, v;

  addr &= chipmem_bank.mask;
  m = (uae_u16 *)(chipmem_bank.baseaddr + addr);
  v = do_get_mem_word (m);
  return v;
}

static uae_u32 REGPARAM2 chipmem_bget (uaecptr addr)
{
	uae_u8 v;
  addr &= chipmem_bank.mask;
	v = chipmem_bank.baseaddr[addr];
	return v;
}

void REGPARAM2 chipmem_lput (uaecptr addr, uae_u32 l)
{
  uae_u32 *m;
  addr &= chipmem_bank.mask;
  m = (uae_u32 *)(chipmem_bank.baseaddr + addr);
  do_put_mem_long(m, l);
}

void REGPARAM2 chipmem_wput (uaecptr addr, uae_u32 w)
{
 uae_u16 *m;
 addr &= chipmem_bank.mask;
 m = (uae_u16 *)(chipmem_bank.baseaddr + addr);
 do_put_mem_word (m, w);
}

void REGPARAM2 chipmem_bput (uaecptr addr, uae_u32 b)
{
  addr &= chipmem_bank.mask;
	chipmem_bank.baseaddr[addr] = b;
}

void REGPARAM2 chipmem_agnus_wput (uaecptr addr, uae_u32 w)
{
  uae_u16 *m;

  addr &= chipmem_full_mask;
  if (addr >= chipmem_bank.allocated)
	  return;
  m = (uae_u16 *)(chipmem_bank.baseaddr + addr);
  do_put_mem_word (m, w);
}

static int REGPARAM2 chipmem_check (uaecptr addr, uae_u32 size)
{
  addr &= chipmem_bank.mask;
  return (addr + size) <= chipmem_bank.allocated;
}

static uae_u8 *REGPARAM2 chipmem_xlate (uaecptr addr)
{
	addr &= chipmem_bank.mask;
  return chipmem_bank.baseaddr + addr;
}

/* Slow memory */

MEMORY_FUNCTIONS(bogomem);

/* Kick memory */

uae_u16 kickstart_version;

static void REGPARAM3 kickmem_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 kickmem_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 kickmem_bput (uaecptr, uae_u32) REGPARAM;

MEMORY_BGET(kickmem);
MEMORY_WGET(kickmem);
MEMORY_LGET(kickmem);
MEMORY_CHECK(kickmem);
MEMORY_XLATE(kickmem);

static void REGPARAM2 kickmem_lput (uaecptr addr, uae_u32 b)
{
#ifdef JIT
  special_mem |= S_WRITE;
#endif
}

static void REGPARAM2 kickmem_wput (uaecptr addr, uae_u32 b)
{
#ifdef JIT
  special_mem |= S_WRITE;
#endif
}

static void REGPARAM2 kickmem_bput (uaecptr addr, uae_u32 b)
{
#ifdef JIT
  special_mem |= S_WRITE;
#endif
}

/* CD32/CDTV extended kick memory */

static int extendedkickmem_type;

#define EXTENDED_ROM_CD32 1
#define EXTENDED_ROM_CDTV 2
#define EXTENDED_ROM_KS 3
#define EXTENDED_ROM_ARCADIA 4

static void REGPARAM3 extendedkickmem_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 extendedkickmem_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 extendedkickmem_bput (uaecptr, uae_u32) REGPARAM;

MEMORY_BGET(extendedkickmem);
MEMORY_WGET(extendedkickmem);
MEMORY_LGET(extendedkickmem);
MEMORY_CHECK(extendedkickmem);
MEMORY_XLATE(extendedkickmem);

static void REGPARAM2 extendedkickmem_lput (uaecptr addr, uae_u32 b)
{
#ifdef JIT
  special_mem |= S_WRITE;
#endif
}
static void REGPARAM2 extendedkickmem_wput (uaecptr addr, uae_u32 b)
{
#ifdef JIT
  special_mem |= S_WRITE;
#endif
}
static void REGPARAM2 extendedkickmem_bput (uaecptr addr, uae_u32 b)
{
#ifdef JIT
  special_mem |= S_WRITE;
#endif
}


static void REGPARAM3 extendedkickmem2_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 extendedkickmem2_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 extendedkickmem2_bput (uaecptr, uae_u32) REGPARAM;

MEMORY_BGET(extendedkickmem2);
MEMORY_WGET(extendedkickmem2);
MEMORY_LGET(extendedkickmem2);
MEMORY_CHECK(extendedkickmem2);
MEMORY_XLATE(extendedkickmem2);

static void REGPARAM2 extendedkickmem2_lput (uaecptr addr, uae_u32 b)
{
#ifdef JIT
  special_mem |= S_WRITE;
#endif
}
static void REGPARAM2 extendedkickmem2_wput (uaecptr addr, uae_u32 b)
{
#ifdef JIT
  special_mem |= S_WRITE;
#endif
}
static void REGPARAM2 extendedkickmem2_bput (uaecptr addr, uae_u32 b)
{
#ifdef JIT
  special_mem |= S_WRITE;
#endif
}

/* Default memory access functions */

int REGPARAM2 default_check (uaecptr a, uae_u32 b)
{
  return 0;
}

static int be_cnt;

uae_u8 *REGPARAM2 default_xlate (uaecptr a)
{
  if (quit_program == 0) {
    /* do this only in 68010+ mode, there are some tricky A500 programs.. */
    if(currprefs.cpu_model > 68000 || !currprefs.cpu_compatible) {
			if (be_cnt < 3) {
        write_log (_T("Your Amiga program just did something terribly stupid %08X PC=%08X\n"), a, M68K_GETPC);
      }
			be_cnt++;
			if (regs.s || be_cnt > 1000) {
				cpu_halt (3);
				be_cnt = 0;
			} else {
				regs.panic = 4;
				regs.panic_pc = m68k_getpc ();
				regs.panic_addr = a;
				set_special (SPCFLAG_BRK);
			}
    }
  }
  return kickmem_xlate (2);	/* So we don't crash. */
}

/* Address banks */

addrbank dummy_bank = {
  dummy_lget, dummy_wget, dummy_bget,
  dummy_lput, dummy_wput, dummy_bput,
  default_xlate, dummy_check, NULL, NULL,
  dummy_lgeti, dummy_wgeti, ABFLAG_NONE
};

addrbank chipmem_bank = {
  chipmem_lget, chipmem_wget, chipmem_bget,
  chipmem_lput, chipmem_wput, chipmem_bput,
	chipmem_xlate, chipmem_check, NULL, _T("Chip memory"),
  chipmem_lget, chipmem_wget, ABFLAG_RAM
};

addrbank bogomem_bank = {
  bogomem_lget, bogomem_wget, bogomem_bget,
  bogomem_lput, bogomem_wput, bogomem_bput,
	bogomem_xlate, bogomem_check, NULL, _T("Slow memory"),
  bogomem_lget, bogomem_wget, ABFLAG_RAM
};

addrbank kickmem_bank = {
  kickmem_lget, kickmem_wget, kickmem_bget,
  kickmem_lput, kickmem_wput, kickmem_bput,
	kickmem_xlate, kickmem_check, NULL, _T("Kickstart ROM"),
  kickmem_lget, kickmem_wget, ABFLAG_ROM
};

addrbank extendedkickmem_bank = {
  extendedkickmem_lget, extendedkickmem_wget, extendedkickmem_bget,
  extendedkickmem_lput, extendedkickmem_wput, extendedkickmem_bput,
	extendedkickmem_xlate, extendedkickmem_check, NULL, _T("Extended Kickstart ROM"),
  extendedkickmem_lget, extendedkickmem_wget, ABFLAG_ROM
};
addrbank extendedkickmem2_bank = {
  extendedkickmem2_lget, extendedkickmem2_wget, extendedkickmem2_bget,
  extendedkickmem2_lput, extendedkickmem2_wput, extendedkickmem2_bput,
	extendedkickmem2_xlate, extendedkickmem2_check, NULL, _T("Extended 2nd Kickstart ROM"),
  extendedkickmem2_lget, extendedkickmem2_wget, ABFLAG_ROM
};


static uae_char *kickstring = "exec.library";
static int read_kickstart (struct zfile *f, uae_u8 *mem, int size, int dochecksum, int noalias)
{
  uae_char buffer[20];
  int i, j, oldpos;
  int cr = 0, kickdisk = 0;

  if (size < 0) {
  	zfile_fseek (f, 0, SEEK_END);
  	size = zfile_ftell (f) & ~0x3ff;
  	zfile_fseek (f, 0, SEEK_SET);
  }
  oldpos = zfile_ftell (f);
  i = zfile_fread (buffer, 1, 11, f);
  if (!memcmp(buffer, "KICK", 4)) {
    zfile_fseek (f, 512, SEEK_SET);
    kickdisk = 1;
#if 0
  } else if (size >= ROM_SIZE_512 && !memcmp (buffer, "AMIG", 4)) {
  	/* ReKick */
  	zfile_fseek (f, oldpos + 0x6c, SEEK_SET);
  	cr = 2;
#endif
  } else if (memcmp ((uae_char*)buffer, "AMIROMTYPE1", 11) != 0) {
    zfile_fseek (f, oldpos, SEEK_SET);
  } else {
  	cloanto_rom = 1;
  	cr = 1;
  }

  memset (mem, 0, size);
  for (i = 0; i < 8; i++)
  	mem[size - 16 + i * 2 + 1] = 0x18 + i;
  mem[size - 20] = size >> 24;
  mem[size - 19] = size >> 16;
  mem[size - 18] = size >>  8;
  mem[size - 17] = size >>  0;

  i = zfile_fread (mem, 1, size, f);

  if (kickdisk && i > ROM_SIZE_256)
    i = ROM_SIZE_256;
#if 0
  if (i >= ROM_SIZE_256 && (i != ROM_SIZE_256 && i != ROM_SIZE_512 && i != ROM_SIZE_512 * 2 && i != ROM_SIZE_512 * 4)) {
    notify_user (NUMSG_KSROMREADERROR);
    return 0;
  }
#endif
  if (i < size - 20)
  	kickstart_fix_checksum (mem, size);

  j = 1;
  while (j < i)
  	j <<= 1;
  i = j;

  if (!noalias && i == size / 2)
    memcpy (mem + size / 2, mem, size / 2);

  if (cr) {
    if(!decode_rom (mem, size, cr, i))
      return 0;
  }

  for (j = 0; j < 256 && i >= ROM_SIZE_256; j++) {
  	if (!memcmp (mem + j, kickstring, strlen (kickstring) + 1))
	    break;
  }

  if (j == 256 || i < ROM_SIZE_256)
  	dochecksum = 0;
  if (dochecksum)
  	kickstart_checksum (mem, size);
  return i;
}

static bool load_extendedkickstart (const TCHAR *romextfile, int type)
{
  struct zfile *f;
  int size, off;
	bool ret = false;

  if (_tcslen (romextfile) == 0)
    return false;
  f = read_rom_name (romextfile);
  if (!f) {
	  notify_user (NUMSG_NOEXTROM);
	  return false;
  }
  zfile_fseek (f, 0, SEEK_END);
  size = zfile_ftell (f);
	extendedkickmem_bank.allocated = ROM_SIZE_512;
  off = 0;
	if (type == 0) {
		if (currprefs.cs_cd32cd) {
			extendedkickmem_type = EXTENDED_ROM_CD32;
    } else if (size > 300000) {
    	extendedkickmem_type = EXTENDED_ROM_CD32;
    } else if (need_uae_boot_rom () != 0xf00000) {
	    extendedkickmem_type = EXTENDED_ROM_CDTV;
    } 
  } else {
		extendedkickmem_type = type;
	}
	if (extendedkickmem_type) {
    zfile_fseek (f, off, SEEK_SET);
    switch (extendedkickmem_type) {
    case EXTENDED_ROM_CDTV:
      extendedkickmem_bank.baseaddr = mapped_malloc (extendedkickmem_bank.allocated, _T("rom_f0"));
      extendedkickmem_bank.start = 0xf00000;
	    break;
    case EXTENDED_ROM_CD32:
	    extendedkickmem_bank.baseaddr = mapped_malloc (extendedkickmem_bank.allocated, _T("rom_e0"));
      extendedkickmem_bank.start = 0xe00000;
	    break;
    }
		if (extendedkickmem_bank.baseaddr) {
      read_kickstart (f, extendedkickmem_bank.baseaddr, extendedkickmem_bank.allocated, 0, 1);
      extendedkickmem_bank.mask = extendedkickmem_bank.allocated - 1;
			ret = true;
		}
	}
  zfile_fclose (f);
  return ret;
}

/* disable incompatible drivers */
static int patch_residents (uae_u8 *kickmemory, int size)
{
  int i, j, patched = 0;
  uae_char *residents[] = { "NCR scsi.device", "scsi.device", "carddisk.device", "card.resource", 0 };
  // "scsi.device", "carddisk.device", "card.resource" };
  uaecptr base = size == ROM_SIZE_512 ? 0xf80000 : 0xfc0000;

  for (i = 0; i < size - 100; i++) {
  	if (kickmemory[i] == 0x4a && kickmemory[i + 1] == 0xfc) {
	    uaecptr addr;
	    addr = (kickmemory[i + 2] << 24) | (kickmemory[i + 3] << 16) | (kickmemory[i + 4] << 8) | (kickmemory[i + 5] << 0);
	    if (addr != i + base)
    		continue;
	    addr = (kickmemory[i + 14] << 24) | (kickmemory[i + 15] << 16) | (kickmemory[i + 16] << 8) | (kickmemory[i + 17] << 0);
	    if (addr >= base && addr < base + size) {
    		j = 0;
    		while (residents[j]) {
  		    if (!memcmp (residents[j], kickmemory + addr - base, strlen (residents[j]) + 1)) {
						TCHAR *s = au (residents[j]);
						write_log (_T("KSPatcher: '%s' at %08X disabled\n"), s, i + base);
						xfree (s);
      			kickmemory[i] = 0x4b; /* destroy RTC_MATCHWORD */
      			patched++;
      			break;
  		    }
  		    j++;
    		}
	    }	
  	}
  }
  return patched;
}

static void patch_kick(void)
{
  int patched = 0;
  patched += patch_residents (kickmem_bank.baseaddr, kickmem_bank.allocated);
  if (extendedkickmem_bank.baseaddr) {
    patched += patch_residents (extendedkickmem_bank.baseaddr, extendedkickmem_bank.allocated);
    if (patched)
      kickstart_fix_checksum (extendedkickmem_bank.baseaddr, extendedkickmem_bank.allocated);
  }
  if (patched)
  	kickstart_fix_checksum (kickmem_bank.baseaddr, kickmem_bank.allocated);
}

extern unsigned char arosrom[];
extern unsigned int arosrom_len;
static bool load_kickstart_replacement (void)
{
	struct zfile *f;
	
	f = zfile_fopen_data (_T("aros.gz"), arosrom_len, arosrom);
	if (!f)
		return false;
	f = zfile_gunzip (f);
	if (!f)
		return false;

	extendedkickmem_bank.allocated = ROM_SIZE_512;
	extendedkickmem_bank.mask = ROM_SIZE_512 - 1;
	extendedkickmem_type = EXTENDED_ROM_KS;
	extendedkickmem_bank.baseaddr = mapped_malloc (extendedkickmem_bank.allocated, _T("rom_e0"));
	read_kickstart (f, extendedkickmem_bank.baseaddr, ROM_SIZE_512, 0, 1);

	kickmem_bank.allocated = ROM_SIZE_512;
	kickmem_bank.mask = ROM_SIZE_512 - 1;
	read_kickstart (f, kickmem_bank.baseaddr, ROM_SIZE_512, 1, 0);
	zfile_fclose (f);
	return true;
}

static int load_kickstart (void)
{
  struct zfile *f;
  TCHAR tmprom[MAX_DPATH], tmprom2[MAX_DPATH];
  int patched = 0;

  cloanto_rom = 0;
	if (!_tcscmp (currprefs.romfile, _T(":AROS")))
	  return load_kickstart_replacement ();
  f = read_rom_name (currprefs.romfile);
  _tcscpy (tmprom, currprefs.romfile);
  if (f == NULL) {
  	_stprintf (tmprom2, _T("%s%s"), start_path_data, currprefs.romfile);
  	f = rom_fopen (tmprom2, _T("rb"), ZFD_NORMAL);
  	if (f == NULL) {
	    _stprintf (currprefs.romfile, _T("%sroms/kick.rom"), start_path_data);
    	f = rom_fopen (currprefs.romfile, _T("rb"), ZFD_NORMAL);
    	if (f == NULL) {
    		_stprintf (currprefs.romfile, _T("%skick.rom"), start_path_data);
	      f = rom_fopen( currprefs.romfile, _T("rb"), ZFD_NORMAL);
				if (f == NULL)
					f = read_rom_name_guess (tmprom);
      }
  	} else {
    _tcscpy (currprefs.romfile, tmprom2);
    }
  }
  addkeydir (currprefs.romfile);
	if (f == NULL) /* still no luck */
	  goto err;

  if (f != NULL) {
	  int filesize, size, maxsize;
  	int kspos = ROM_SIZE_512;
  	int extpos = 0;

	  maxsize = ROM_SIZE_512;
	  zfile_fseek (f, 0, SEEK_END);
	  filesize = zfile_ftell (f);
	  zfile_fseek (f, 0, SEEK_SET);
	  if (filesize == 1760 * 512) {
      filesize = ROM_SIZE_256;
      maxsize = ROM_SIZE_256;
	  }
	  if (filesize == ROM_SIZE_512 + 8) {
	    /* GVP 0xf0 kickstart */
	    zfile_fseek (f, 8, SEEK_SET);
	  }
	  if (filesize >= ROM_SIZE_512 * 2) {
      struct romdata *rd = getromdatabyzfile(f);
      zfile_fseek (f, kspos, SEEK_SET);
	  }
	  if (filesize >= ROM_SIZE_512 * 4) {
	    kspos = ROM_SIZE_512 * 3;
	    extpos = 0;
	    zfile_fseek (f, kspos, SEEK_SET);
	  }
	  size = read_kickstart (f, kickmem_bank.baseaddr, maxsize, 1, 0);
    if (size == 0)
    	goto err;
    kickmem_bank.mask = size - 1;
  	kickmem_bank.allocated = size;
  	if (filesize >= ROM_SIZE_512 * 2 && !extendedkickmem_type) {
	    extendedkickmem_bank.allocated = ROM_SIZE_512;
	    extendedkickmem_type = EXTENDED_ROM_KS;
	    extendedkickmem_bank.baseaddr = mapped_malloc (extendedkickmem_bank.allocated, _T("rom_e0"));
			extendedkickmem_bank.start = 0xe00000;
	    zfile_fseek (f, extpos, SEEK_SET);
	    read_kickstart (f, extendedkickmem_bank.baseaddr, extendedkickmem_bank.allocated, 0, 1);
	    extendedkickmem_bank.mask = extendedkickmem_bank.allocated - 1;
  	}
  	if (filesize > ROM_SIZE_512 * 2) {
	    extendedkickmem2_bank.allocated = ROM_SIZE_512 * 2;
	    extendedkickmem2_bank.baseaddr = mapped_malloc (extendedkickmem2_bank.allocated, _T("rom_a8"));
	    zfile_fseek (f, extpos + ROM_SIZE_512, SEEK_SET);
	    read_kickstart (f, extendedkickmem2_bank.baseaddr, ROM_SIZE_512, 0, 1);
	    zfile_fseek (f, extpos + ROM_SIZE_512 * 2, SEEK_SET);
	    read_kickstart (f, extendedkickmem2_bank.baseaddr + ROM_SIZE_512, ROM_SIZE_512, 0, 1);
	    extendedkickmem2_bank.mask = extendedkickmem2_bank.allocated - 1;
			extendedkickmem2_bank.start = 0xa80000;
  	}
  }

  kickstart_version = (kickmem_bank.baseaddr[12] << 8) | kickmem_bank.baseaddr[13];
  if (kickstart_version == 0xffff)
  	kickstart_version = 0;
  zfile_fclose (f);
  return 1;

err:
  _tcscpy (currprefs.romfile, tmprom);
  zfile_fclose (f);
  return 0;
}


static void init_mem_banks (void)
{
  int i;
  for (i = 0; i < MEMORY_BANKS; i++) {
    mem_banks[i] = &dummy_bank;
  }
}

static bool singlebit (uae_u32 v)
{
	while (v && !(v & 1))
		v >>= 1;
	return (v & ~1) == 0;
}

static void allocate_memory (void)
{
  if (chipmem_bank.allocated != currprefs.chipmem_size) {
    int memsize;
    mapped_free (chipmem_bank.baseaddr);
		chipmem_bank.baseaddr = 0;
	  if (currprefs.chipmem_size > 2 * 1024 * 1024)
	    free_fastmemory ();
		
	  memsize = chipmem_bank.allocated = currprefs.chipmem_size;
  	chipmem_full_mask = chipmem_bank.mask = chipmem_bank.allocated - 1;
		chipmem_bank.start = chipmem_start_addr;
		if (!currprefs.cachesize && memsize < 0x100000)
			memsize = 0x100000;
	  if (memsize > 0x100000 && memsize < 0x200000)
      memsize = 0x200000;
    chipmem_bank.baseaddr = mapped_malloc (memsize, _T("chip"));
		if (chipmem_bank.baseaddr == 0) {
			write_log (_T("Fatal error: out of memory for chipmem.\n"));
			chipmem_bank.allocated = 0;
    } else {
			need_hardreset = true;
	    if (memsize > chipmem_bank.allocated)
		    memset (chipmem_bank.baseaddr + chipmem_bank.allocated, 0xff, memsize - chipmem_bank.allocated);
    }
    currprefs.chipset_mask = changed_prefs.chipset_mask;
    chipmem_full_mask = chipmem_bank.allocated - 1;
		if ((currprefs.chipset_mask & CSMASK_ECS_AGNUS) && !currprefs.cachesize) {
			if (chipmem_bank.allocated < 0x100000)
				chipmem_full_mask = 0x100000 - 1;
			if (chipmem_bank.allocated > 0x100000 && chipmem_bank.allocated < 0x200000)
				chipmem_full_mask = chipmem_bank.mask = 0x200000 - 1;
		}
  }

  if (bogomem_bank.allocated != currprefs.bogomem_size) {
    mapped_free (bogomem_bank.baseaddr);
		bogomem_bank.baseaddr = NULL;
		
		if(currprefs.bogomem_size > 0x1c0000)
      currprefs.bogomem_size = 0x1c0000;
    if (currprefs.bogomem_size > 0x180000 && ((changed_prefs.chipset_mask & CSMASK_AGA) || (currprefs.cpu_model >= 68020)))
      currprefs.bogomem_size = 0x180000;

	  bogomem_bank.allocated = currprefs.bogomem_size;
		if (bogomem_bank.allocated >= 0x180000)
			bogomem_bank.allocated = 0x200000;
		bogomem_bank.mask = bogomem_bank.allocated - 1;
		bogomem_bank.start = bogomem_start_addr;

		if (bogomem_bank.allocated) {
			bogomem_bank.baseaddr = mapped_malloc (bogomem_bank.allocated, _T("bogo"));
	    if (bogomem_bank.baseaddr == 0) {
				write_log (_T("Out of memory for bogomem.\n"));
    		bogomem_bank.allocated = 0;
	    }
		}
	  need_hardreset = true;
	}

  if (savestate_state == STATE_RESTORE) {
    if (bootrom_filepos) {
			protect_roms (false);
  	  restore_ram (bootrom_filepos, rtarea);
			protect_roms (true);
    }
    restore_ram (chip_filepos, chipmem_bank.baseaddr);
    if (bogomem_bank.allocated > 0)
	    restore_ram (bogo_filepos, bogomem_bank.baseaddr);
  }
  bootrom_filepos = 0;
  chip_filepos = 0;
  bogo_filepos = 0;
}

void map_overlay (int chip)
{
  int size;
  addrbank *cb;
  int currPC = m68k_getpc();

  size = chipmem_bank.allocated >= 0x180000 ? (chipmem_bank.allocated >> 16) : 32;
  cb = &chipmem_bank;
  if (chip) {
		map_banks (&dummy_bank, 0, 32, 0);
  	map_banks (cb, 0, size, chipmem_bank.allocated);
  } else {
  	addrbank *rb = NULL;
  	if (size < 32)
	    size = 32;
  	cb = &get_mem_bank (0xf00000);
  	if (!rb && cb && (cb->flags & ABFLAG_ROM) && get_word (0xf00000) == 0x1114)
	    rb = cb;
  	cb = &get_mem_bank (0xe00000);
  	if (!rb && cb && (cb->flags & ABFLAG_ROM) && get_word (0xe00000) == 0x1114)
	    rb = cb;
  	if (!rb)
	    rb = &kickmem_bank;
  	map_banks (rb, 0, size, 0x80000);
  }
  if (!isrestore () && valid_address (regs.pc, 4))
    m68k_setpc_normal (currPC);
}

uae_s32 getz2size (struct uae_prefs *p)
{
	uae_u32 start;
	start = p->fastmem_size;
	if (p->rtgmem_size && !gfxboard_is_z3 (p->rtgmem_type)) {
		while (start & (p->rtgmem_size - 1) && start < 8 * 1024 * 1024)
			start += 1024 * 1024;
		if (start + p->rtgmem_size > 8 * 1024 * 1024)
			return -1;
	}
	start += p->rtgmem_size;
	return start;
}

uae_u32 getz2endaddr (void)
{
	uae_u32 start;
	start = currprefs.fastmem_size;
	if (currprefs.rtgmem_size && !gfxboard_is_z3 (currprefs.rtgmem_type)) {
		if (!start)
			start = 0x00200000;
		while (start & (currprefs.rtgmem_size - 1) && start < 4 * 1024 * 1024)
			start += 1024 * 1024;
	}
	return start + 2 * 1024 * 1024;
}

void memory_clear (void)
{
	mem_hardreset = 0;
	if (savestate_state == STATE_RESTORE)
		return;
	if (chipmem_bank.baseaddr)
		memset (chipmem_bank.baseaddr, 0, chipmem_bank.allocated);
	if (bogomem_bank.baseaddr)
		memset (bogomem_bank.baseaddr, 0, bogomem_bank.allocated);
	expansion_clear ();
}

void memory_reset (void)
{
  int bnk, bnk_end;
	bool gayleorfatgary;

	need_hardreset = false;
	/* Use changed_prefs, as m68k_reset is called later.  */
	if (last_address_space_24 != changed_prefs.address_space_24)
		need_hardreset = true;
	last_address_space_24 = changed_prefs.address_space_24;

	if (mem_hardreset > 2)
		memory_init ();

	be_cnt = 0;
  currprefs.chipmem_size = changed_prefs.chipmem_size;
  currprefs.bogomem_size = changed_prefs.bogomem_size;

	gayleorfatgary = (currprefs.chipset_mask & CSMASK_AGA);

  init_mem_banks ();
  allocate_memory ();

  if (mem_hardreset > 1
		|| _tcscmp (currprefs.romfile, changed_prefs.romfile) != 0
  	|| _tcscmp (currprefs.romextfile, changed_prefs.romextfile) != 0)
  {
		protect_roms (false);
		write_log (_T("ROM loader.. (%s)\n"), currprefs.romfile);
    kickstart_rom = 1;

  	memcpy (currprefs.romfile, changed_prefs.romfile, sizeof currprefs.romfile);
  	memcpy (currprefs.romextfile, changed_prefs.romextfile, sizeof currprefs.romextfile);
		need_hardreset = true;
    mapped_free (extendedkickmem_bank.baseaddr);
  	extendedkickmem_bank.baseaddr = NULL;
    extendedkickmem_bank.allocated = 0;
  	extendedkickmem2_bank.baseaddr = NULL;
  	extendedkickmem2_bank.allocated = 0;
  	extendedkickmem_type = 0;
    load_extendedkickstart (currprefs.romextfile, 0);
  	kickmem_bank.mask = ROM_SIZE_512 - 1;
  	if (!load_kickstart ()) {
	    if (_tcslen (currprefs.romfile) > 0) {
				error_log (_T("Failed to open '%s'\n"), currprefs.romfile);
    		notify_user (NUMSG_NOROM);
      }
			load_kickstart_replacement ();
  	} else {
      struct romdata *rd = getromdatabydata (kickmem_bank.baseaddr, kickmem_bank.allocated);
  	  if (rd) {
				write_log (_T("Known ROM '%s' loaded\n"), rd->name);
				if ((rd->cpu & 8) && changed_prefs.cpu_model < 68030) {
					notify_user (NUMSG_KS68030PLUS);
					uae_restart (-1, NULL);
				} else if ((rd->cpu & 3) == 3 && changed_prefs.cpu_model != 68030) {
		      notify_user (NUMSG_KS68030);
		      uae_restart (-1, NULL);
  		  } else if ((rd->cpu & 3) == 1 && changed_prefs.cpu_model < 68020) {
  	      notify_user (NUMSG_KS68EC020);
		      uae_restart (-1, NULL);
	  	  } else if ((rd->cpu & 3) == 2 && (changed_prefs.cpu_model < 68020 || changed_prefs.address_space_24)) {
		      notify_user (NUMSG_KS68020);
		      uae_restart (-1, NULL);
	  	  }
	  	  if (rd->cloanto)
	  	    cloanto_rom = 1;
 	    	kickstart_rom = 0;
				if ((rd->type & (ROMTYPE_SPECIALKICK | ROMTYPE_KICK)) == ROMTYPE_KICK)
	  	    kickstart_rom = 1;
  	  } else {
				write_log (_T("Unknown ROM '%s' loaded\n"), currprefs.romfile);
      }
    }
	  patch_kick ();
		write_log (_T("ROM loader end\n"));
		protect_roms (true);
  }

  map_banks (&custom_bank, 0xC0, 0xE0 - 0xC0, 0);
  map_banks (&cia_bank, 0xA0, 32, 0);

  /* D80000 - DDFFFF not mapped (A1000 or A2000 = custom chips) */
  map_banks (&dummy_bank, 0xD8, 6, 0);

  /* map "nothing" to 0x200000 - 0x9FFFFF (0xBEFFFF if Gayle or Fat Gary) */
  bnk = chipmem_bank.allocated >> 16;
  if (bnk < 0x20 + (currprefs.fastmem_size >> 16))
     bnk = 0x20 + (currprefs.fastmem_size >> 16);
  bnk_end = gayleorfatgary ? 0xBF : 0xA0;
  map_banks (&dummy_bank, bnk, bnk_end - bnk, 0);
  if (gayleorfatgary) {
	  // a3000 or a4000 = custom chips from 0xc0 to 0xd0
     map_banks (&dummy_bank, 0xc0, 0xd8 - 0xc0, 0);
  }

  if (bogomem_bank.baseaddr) {
	  int t = currprefs.bogomem_size >> 16;
	  if (t > 0x1C)
		  t = 0x1C;
	  if (t > 0x18 && ((currprefs.chipset_mask & CSMASK_AGA) || (currprefs.cpu_model >= 68020 && !currprefs.address_space_24)))
		  t = 0x18;
    map_banks (&bogomem_bank, 0xC0, t, 0);
  }
  map_banks (&clock_bank, 0xDC, 1, 0);
#ifdef CD32
	if (currprefs.cs_cd32c2p || currprefs.cs_cd32cd || currprefs.cs_cd32nvram) {
		map_banks (&akiko_bank, AKIKO_BASE >> 16, 1, 0);
	}
#endif

  map_banks (&kickmem_bank, 0xF8, 8, 0);
  /* map beta Kickstarts at 0x200000/0xC00000/0xF00000 */
  if (kickmem_bank.baseaddr[0] == 0x11 && kickmem_bank.baseaddr[2] == 0x4e && kickmem_bank.baseaddr[3] == 0xf9 && kickmem_bank.baseaddr[4] == 0x00) {
     uae_u32 addr = kickmem_bank.baseaddr[5];
    if (addr == 0x20 && chipmem_bank.allocated <= 0x200000 && fastmem_bank.allocated == 0)
      map_banks (&kickmem_bank, addr, 8, 0);
	  if (addr == 0xC0 && bogomem_bank.allocated == 0)
      map_banks (&kickmem_bank, addr, 8, 0);
	  if (addr == 0xF0)
      map_banks (&kickmem_bank, addr, 8, 0);
  }

#ifdef AUTOCONFIG
  map_banks (&expamem_bank, 0xE8, 1, 0);
#endif

  /* Map the chipmem into all of the lower 8MB */
  map_overlay (1);

  switch (extendedkickmem_type) {
    case EXTENDED_ROM_KS:
      map_banks (&extendedkickmem_bank, 0xE0, 8, 0);
      break;
#ifdef CD32
	  case EXTENDED_ROM_CD32:
		  map_banks (&extendedkickmem_bank, 0xE0, 8, 0);
		  break;
#endif
  }
#ifdef AUTOCONFIG
  if (need_uae_boot_rom ())
  	map_banks (&rtarea_bank, rtarea_base >> 16, 1, 0);
#endif

  if ((cloanto_rom) && !extendedkickmem_type)
    map_banks (&kickmem_bank, 0xE0, 8, 0);

	if (currprefs.chipset_mask & CSMASK_AGA) {
		if (extendedkickmem_type == EXTENDED_ROM_CD32 || extendedkickmem_type == EXTENDED_ROM_KS)
			map_banks (&extendedkickmem_bank, 0xb0, 8, 0);
		else
			map_banks (&kickmem_bank, 0xb0, 8, 0);
		map_banks (&kickmem_bank, 0xa8, 8, 0);
	}

	if (mem_hardreset) {
		memory_clear ();
	}
  write_log (_T("memory init end\n"));
}

void memory_init (void)
{
	init_mem_banks ();
	virtualdevice_init ();

  chipmem_bank.allocated = 0;
  bogomem_bank.allocated = 0;
  kickmem_bank.baseaddr = NULL;
  extendedkickmem_bank.baseaddr = NULL;
  extendedkickmem_bank.allocated = 0;
  extendedkickmem2_bank.baseaddr = NULL;
  extendedkickmem2_bank.allocated = 0;
  extendedkickmem_type = 0;
  chipmem_bank.baseaddr = 0;
  bogomem_bank.baseaddr = NULL;

	kickmem_bank.baseaddr = mapped_malloc (ROM_SIZE_512, _T("kick"));
	memset (kickmem_bank.baseaddr, 0, ROM_SIZE_512);
	_tcscpy (currprefs.romfile, _T("<none>"));
  currprefs.romextfile[0] = 0;
}

void memory_cleanup (void)
{
	mapped_free (bogomem_bank.baseaddr);
	mapped_free (kickmem_bank.baseaddr);
	mapped_free (chipmem_bank.baseaddr);
  mapped_free (extendedkickmem_bank.baseaddr);
  mapped_free (extendedkickmem2_bank.baseaddr);
  
  bogomem_bank.baseaddr = NULL;
  kickmem_bank.baseaddr = NULL;
  chipmem_bank.baseaddr = NULL;
  extendedkickmem_bank.baseaddr = NULL;
  extendedkickmem2_bank.baseaddr = NULL;
  
  init_mem_banks ();
}

void memory_hardreset (int mode)
{
	if (mode + 1 > mem_hardreset)
		mem_hardreset = mode + 1;
}

static void map_banks2 (addrbank *bank, int start, int size, int realsize, int quick)
{
  int bnr;
  unsigned long int hioffs = 0, endhioffs = 0x100;
  addrbank *orgbank = bank;
  uae_u32 realstart = start;

  flush_icache (0, 3);		/* Sure don't want to keep any old mappings around! */

  if (!realsize)
    realsize = size << 16;

  if ((size << 16) < realsize) {
		write_log (_T("Broken mapping, size=%x, realsize=%x\nStart is %x\n"),
	    size, realsize, start);
  }

#ifndef ADDRESS_SPACE_24BIT
  if (start >= 0x100) {
    for (bnr = start; bnr < start + size; bnr++) {
      mem_banks[bnr] = bank;
    }
    return;
  }
#endif
  if (last_address_space_24)
	  endhioffs = 0x10000;
#ifdef ADDRESS_SPACE_24BIT
  endhioffs = 0x100;
#endif
  for (hioffs = 0; hioffs < endhioffs; hioffs += 0x100) {
    for (bnr = start; bnr < start + size; bnr++) {
      mem_banks[bnr + hioffs] = bank;
    }
  }
}

void map_banks (addrbank *bank, int start, int size, int realsize)
{
	map_banks2 (bank, start, size, realsize, 0);
}
void map_banks_quick (addrbank *bank, int start, int size, int realsize)
{
	map_banks2 (bank, start, size, realsize, 1);
}

#ifdef SAVESTATE

/* memory save/restore code */

uae_u8 *save_bootrom(int *len)
{
  if (!uae_boot_rom)
  	return 0;
  *len = uae_boot_rom_size;
  return rtarea;
}

uae_u8 *save_cram (int *len)
{
  *len = chipmem_bank.allocated;
  return chipmem_bank.baseaddr;
}

uae_u8 *save_bram (int *len)
{
  *len = bogomem_bank.allocated;
  return bogomem_bank.baseaddr;
}

void restore_bootrom (int len, size_t filepos)
{
  bootrom_filepos = filepos;
}

void restore_cram (int len, size_t filepos)
{
  chip_filepos = filepos;
  changed_prefs.chipmem_size = len;
}

void restore_bram (int len, size_t filepos)
{
  bogo_filepos = filepos;
  changed_prefs.bogomem_size = len;
}

uae_u8 *restore_rom (uae_u8 *src)
{
  uae_u32 crc32, mem_start, mem_size, mem_type, version;
  TCHAR *s, *romn;
  int i, crcdet;
	struct romlist *rl = romlist_getit ();

  mem_start = restore_u32 ();
  mem_size = restore_u32 ();
  mem_type = restore_u32 ();
  version = restore_u32 ();
  crc32 = restore_u32 ();
  romn = restore_string ();
  crcdet = 0;
  for (i = 0; i < romlist_count (); i++) {
    if (rl[i].rd->crc32 == crc32 && crc32) {
      if (zfile_exists (rl[i].path)) {
	      switch (mem_type)
        {
        case 0:
        	_tcsncpy (changed_prefs.romfile, rl[i].path, 255);
          break;
	      case 1:
        	_tcsncpy (changed_prefs.romextfile, rl[i].path, 255);
	        break;
        }
			  write_log (_T("ROM '%s' = '%s'\n"), romn, rl[i].path);
        crcdet = 1;
      } else {
			  write_log (_T("ROM '%s' = '%s' invalid rom scanner path!"), romn, rl[i].path);
      }
      break;
    }
  }
  s = restore_string ();
  if (!crcdet) {
    if(zfile_exists (s)) {
      switch (mem_type)
      {
      case 0:
        _tcsncpy (changed_prefs.romfile, s, 255);
        break;
      case 1:
        _tcsncpy (changed_prefs.romextfile, s, 255);
        break;
      }
			write_log (_T("ROM detected (path) as '%s'\n"), s);
      crcdet = 1;
      }
  }
  xfree (s);
  if (!crcdet)
		write_log (_T("WARNING: ROM '%s' %d.%d (CRC32=%08x %08x-%08x) not found!\n"),
			romn, version >> 16, version & 0xffff, crc32, mem_start, mem_start + mem_size - 1);
  xfree (romn);

  return src;
}

uae_u8 *save_rom (int first, int *len, uae_u8 *dstptr)
{
  static int count;
  uae_u8 *dst, *dstbak;
  uae_u8 *mem_real_start;
  uae_u32 version;
  TCHAR *path;
  int mem_start, mem_size, mem_type, saverom;
  int i;
  TCHAR tmpname[1000];

  version = 0;
  saverom = 0;
  if (first)
  	count = 0;
  for (;;) {
  	mem_type = count;
  	mem_size = 0;
  	switch (count) {
  	case 0:		/* Kickstart ROM */
	    mem_start = 0xf80000;
	    mem_real_start = kickmem_bank.baseaddr;
	    mem_size = kickmem_bank.allocated;
	    path = currprefs.romfile;
	    /* 256KB or 512KB ROM? */
	    for (i = 0; i < mem_size / 2 - 4; i++) {
    		if (longget (i + mem_start) != longget (i + mem_start + mem_size / 2))
  		    break;
	    }
	    if (i == mem_size / 2 - 4) {
    		mem_size /= 2;
    		mem_start += ROM_SIZE_256;
	    }
	    version = longget (mem_start + 12); /* version+revision */
			_stprintf (tmpname, _T("Kickstart %d.%d"), wordget (mem_start + 12), wordget (mem_start + 14));
	    break;
	  case 1: /* Extended ROM */
	    if (!extendedkickmem_type)
		    break;
	    mem_start = extendedkickmem_bank.start;
	    mem_real_start = extendedkickmem_bank.baseaddr;
	    mem_size = extendedkickmem_bank.allocated;
	    path = currprefs.romextfile;
			version = longget (mem_start + 12); /* version+revision */
			if (version == 0xffffffff)
				version = longget (mem_start + 16);
			_stprintf (tmpname, _T("Extended"));
	    break;
	  default:
	    return 0;
	  }
	  count++;
	  if (mem_size)
	    break;
  }
  if (dstptr)
  	dstbak = dst = dstptr;
  else
    dstbak = dst = xmalloc (uae_u8, 4 + 4 + 4 + 4 + 4 + 256 + 256 + mem_size);
  save_u32 (mem_start);
  save_u32 (mem_size);
  save_u32 (mem_type);
  save_u32 (version);
  save_u32 (get_crc32 (mem_real_start, mem_size));
  save_string (tmpname);
  save_string (path);
  if (saverom) {
    for (i = 0; i < mem_size; i++)
      *dst++ = byteget (mem_start + i);
  }
  *len = dst - dstbak;
  return dstbak;
}

#endif /* SAVESTATE */

/* memory helpers */

void memcpyha_safe (uaecptr dst, const uae_u8 *src, int size)
{
	if (!addr_valid (_T("memcpyha"), dst, size))
  	return;
  while (size--)
  	put_byte (dst++, *src++);
}
void memcpyha (uaecptr dst, const uae_u8 *src, int size)
{
    while (size--)
	put_byte (dst++, *src++);
}
void memcpyah_safe (uae_u8 *dst, uaecptr src, int size)
{
	if (!addr_valid (_T("memcpyah"), src, size))
  	return;
  while (size--)
  	*dst++ = get_byte(src++);
}
void memcpyah (uae_u8 *dst, uaecptr src, int size)
{
  while (size--)
  	*dst++ = get_byte(src++);
}
uae_char *strcpyah_safe (uae_char *dst, uaecptr src, int maxsize)
{
	uae_char *res = dst;
  uae_u8 b;
	dst[0] = 0;
  do {
		if (!addr_valid (_T("_tcscpyah"), src, 1))
	    return res;
  	b = get_byte(src++);
  	*dst++ = b;
		*dst = 0;
  	maxsize--;
		if (maxsize <= 1)
	    break;
  } while (b);
  return res;
}
uaecptr strcpyha_safe (uaecptr dst, const uae_char *src)
{
  uaecptr res = dst;
  uae_u8 b;
  do {
		if (!addr_valid (_T("_tcscpyha"), dst, 1))
	    return res;
  	b = *src++;
  	put_byte (dst++, b);
  } while (b);
  return res;
}
