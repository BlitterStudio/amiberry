 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Emulate very basic Amiga-like environment for XFD slaves
  *
  * All this only because there is no portable XFD replacement..
  *
  * (c) 2007 Toni Wilen
  *
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sysconfig.h"
#include "sysdeps.h"
#include "zfile.h"
#include "fsdb.h"
#include "uae.h"
#include "custom.h"
#include "memory.h"
#include "newcpu.h"

#include "cpu_small.h"
#include "cputbl_small.h"

/* memory structure
 *
 * 0x000676 execbase
 * 0x000ffc current start of free memory
 * 0x001000 allocmem code (exec)
 * 0x001080 freemem code, dummy (exec)
 * 0x001f00 XFD buffer info
 * 0x002000 first XFD slave
 * 0x?????? next XFD slave
 * ....
 * 0x?????? decrunched data
 * 0x?????? free memory (available for slave)
 * 0xxxf000 end of decompression buffer
 * end      stack
 */

static uaecptr exec = 0x676;
static uaecptr freememaddr = 0x0ffc;
static uaecptr allocmem = 0x1000;
static uaecptr freemem = 0x1080;
static uaecptr bufferinfo = 0x1f00;
static uaecptr stacksize = 0x1000;
struct xfdslave
{
    struct xfdslave *next;
    uaecptr start;
    char *name;
};

static struct xfdslave *xfdslaves;

#define FAKEMEM_SIZE 524288

static uae_u32 gl(uae_u8 *p)
{
    uae_u32 v;

    v = p[0] << 24;
    v |= p[1] << 16;
    v |= p[2] << 8;
    v |= p[3] << 0;
    return v;
}
static uae_u16 gw(uae_u8 *p)
{
    uae_u16 v;

    v = p[0] << 8;
    v |= p[1] << 0;
    return v;
}
static void pl(uae_u8 *p, uae_u32 v)
{
    p[0] = v >> 24;
    p[1] = v >> 16;
    p[2] = v >> 8;
    p[3] = v >> 0;
}

static uae_u8 *codeptr, *xfdmemory;
static uaecptr codememory;
static uae_u32 xfdmem_mask;

static int load_xfd(char *path)
{
    struct zfile *z;
    uae_u8 *buf, *p;
    int size;
    int first, last, numhunks;
    int hunksize;
    uae_u8 *prevhunk;
    int i, j;
    uaecptr hunks[10];
    uaecptr startaddr;

    z = zfile_fopen(path, "rb");
    if (!z)
	return 0;
    zfile_fseek(z, 0, SEEK_END);
    size = zfile_ftell(z);
    zfile_fseek(z, 0, SEEK_SET);
    buf = (uae_u8*)xmalloc(size);
    zfile_fread(buf, size, 1, z);
    zfile_fclose(z);
    p = buf;
    if (gl(p) != 0x3f3)
	goto end;
    p += 4 + 8;
    first = gl(p);
    p += 4;
    last = gl(p);
    p += 4;
    numhunks = last - first + 1;
    prevhunk = 0;
    startaddr = codememory;

    for (i = 0; i < numhunks; i++) {
	uaecptr out = codememory;
	hunksize = gl(p) * 4;
	p += 4;
	pl(codeptr + out, hunksize);
	if (prevhunk) {
	    pl(prevhunk, out);
	    prevhunk = codeptr + out;
	}
	hunks[i] = out + 8;
	codememory += hunksize + 8;
    }
    for (i = 0; i < numhunks; i++) {
	uae_u32 htype = gl(p);
	uae_u32 hsize = gl(p + 4) * 4;
	uaecptr haddr = hunks[i];
	uaecptr srchunk;
	uae_u32 relocnum, relochunknum;

	p += 8;
	if (htype == 0x3e9 || htype == 0x3ea) {
	    memcpy (codeptr + haddr, p, hsize);
	    p += hsize;
	} else if (htype != 0x3eb) {
	    write_log ("RELOC: unknown hunk %08X\n", htype);
	    goto end;
	}
	htype = gl(p);
	p += 4;
	if (htype == 0x3f2)
	    continue;
	if (htype != 0x3ec) {
	    write_log ("RELOC: expected 000003EC but got %08X\n", htype);
	    goto end;
	}
	relocnum = gl(p);
	p += 4;
	relochunknum = gl(p);
	p += 4;
	srchunk = hunks[relochunknum];
	for (j = 0; j < relocnum; j++) {
	    uae_u32 off = gl(p);
	    p += 4;
	    pl(codeptr + haddr + off, gl(codeptr + haddr + off) + srchunk);
	}
    }
    write_log ("XFD slave '%s' loaded and relocated @%08X (%d bytes) succesfully\n", path, startaddr, codememory - startaddr);
    p = codeptr + startaddr + 8;
    if (gl(p + 4) != 'XFDF') {
	write_log ("XFD header corrupt\n");
	goto end;
    }
    p = codeptr + gl(p + 20);
    for (;;) {
	int version = gw(p + 4);
	int mversion = gw(p + 6);
	uaecptr name = gl(p + 8);
	int flags = gw(p + 12);
	int minsize = gl(p + 28);
	uae_u8 *nameptr = codeptr + name;
	struct xfdslave *xfds;

	write_log ("- '%s' ver %d, master ver %d, minsize %d\n",
	    nameptr, version, mversion, minsize);
	xfds = (struct xfdslave *)xcalloc(sizeof(struct xfdslave), 1);
	xfds->name = (char *)nameptr;
	xfds->start = p - codeptr;
	if (!xfdslaves) {
	    xfdslaves = xfds;
	} else {
	    struct xfdslave *x = xfdslaves;
	    while(x->next)
		x = x->next;
	    x->next = xfds;
	}
	if (!gl(p))
	    break;
	p = codeptr + gl(p);
    }
    return 1;
end:
    return 0;
}

static void initexec(void)
{

    pl(codeptr + 4, exec);

    // fake memory allocation functions
    pl(codeptr + exec - 198, allocmem);
    pl(codeptr + exec - 210, freemem);

    // lea -4(pc),a0
    pl(codeptr + allocmem + 0, 0x41fafffa);
    // move.l (a0),d1; add.l d0,(a0)
    pl(codeptr + allocmem + 4, 0x2210d190);
    // move.l d1,d0; rts
    pl(codeptr + allocmem + 8, 0x20014e75);

    pl(codeptr + freemem, 0x4e750000); // rts
}

int init_xfd(void)
{
    static int init;
    char tmp[MAX_DPATH];
    void *d;

    if (init > 0)
	return 1;
    if (init < 0)
	return 0;
    init = -1;

    init_cpu_small();
    codememory = 0x2000;
    codeptr = (uae_u8 *)malloc (FAKEMEM_SIZE);
    sprintf (tmp, "%splugins%cxfd", start_path_data, FSDB_DIR_SEPARATOR);
    d = my_opendir(tmp);
    if (d) {
	while(my_readdir(d, tmp)) {
	    char tmp2[MAX_DPATH];
	    sprintf (tmp2, "%splugins%cxfd%c%s", start_path_data, FSDB_DIR_SEPARATOR, FSDB_DIR_SEPARATOR, tmp);
	    load_xfd(tmp2);
	}
	my_closedir(d);
    }
    initexec();
    codeptr = (uae_u8 *)realloc(codeptr, codememory);
    xfdmemory = (uae_u8 *)malloc (FAKEMEM_SIZE);
    init = 1;
    return 1;
}

void xop_illg (uae_u32 opcode)
{
    write_log("minicpu illegal opcode %04.4x\n", opcode);
    xm68k_setpc(0);
}

static int execute68k(void)
{
    uaecptr stack = xm68k_areg(7);
    xm68k_areg(7) = stack - 4;
    for (;;) {
	uae_u32 pc;
	uae_u32 opcode = xget_iword (0);
	(*xcpufunctbl[opcode])(opcode);
	if (xm68k_areg(7) == stack)
	    return 1;
	pc = xm68k_getpc();
	if (pc <= 0x100 || pc >= FAKEMEM_SIZE) {
	    write_log("minicpu crash, pc=%x\n", pc);
	    return 0;
	}
    }
}

static struct zfile *decomp(struct zfile *zf, struct xfdslave *xfds, uae_u32 size)
{
    uae_u8 *p;
    uae_u32 decompsize;
    uaecptr decompaddr;
    struct zfile *zfout;

    p = xfdmemory + bufferinfo;
    memset(p, 0, 20 * 4);
    xregs.regs[8] = bufferinfo; // A0
    decompsize = gl (p + 16 * 4);
    if (decompsize <= 0)
	return 0;
    decompaddr = FAKEMEM_SIZE - stacksize - decompsize;
    pl (p + 6 * 4, decompaddr); // TargetBuffer
    pl (p + 8 * 4, decompsize);
    if (!execute68k())
	return 0;
    if (!xregs.regs[0])
	return 0;
    decompsize = gl (p + 16 * 4);
    zfout = zfile_fopen_empty (zfile_getname(zf), decompsize);
    zfile_fwrite (xfdmemory + decompaddr, decompsize, 1, zf);
    return zfout;
}

uae_u32 xget_long (uaecptr addr)
{
    uae_u32 *m;
    addr &= xfdmem_mask;
    m = (uae_u32 *)(xfdmemory + addr);
    return do_get_mem_long (m);
}
uae_u32 xget_word (uaecptr addr)
{
    uae_u16 *m;
    addr &= xfdmem_mask;
    m = (uae_u16 *)(xfdmemory + addr);
    return do_get_mem_word (m);
}
uae_u32 xget_byte (uaecptr addr)
{
    addr &= xfdmem_mask;
    return xfdmemory[addr];
}
void xput_long (uaecptr addr, uae_u32 l)
{
    uae_u32 *m;
    addr &= xfdmem_mask;
    m = (uae_u32 *)(xfdmemory + addr);
    do_put_mem_long (m, l);
}
void xput_word (uaecptr addr, uae_u32 w)
{
    uae_u16 *m;
    addr &= xfdmem_mask;
    m = (uae_u16 *)(xfdmemory + addr);
    do_put_mem_word (m, w);
}
void xput_byte (uaecptr addr, uae_u32 b)
{
    addr &= xfdmem_mask;
    xfdmemory[addr] = b;
}

struct zfile *decompress_zfd(struct zfile *z)
{
    unsigned int size;
    uae_u8 *p;
    struct xfdslave *xfds;
    struct zfile *zfout = z;

    if (!init_xfd())
	return zfout;
    memset (xfdmemory, 0, FAKEMEM_SIZE);
    memcpy (xfdmemory, codeptr, codememory);
    xfdmem_mask = FAKEMEM_SIZE - 1;

    p = xfdmemory + codememory;
    zfile_fseek (z, 0, SEEK_END);
    size = zfile_ftell (z);
    zfile_fseek (z, 0, SEEK_SET);
    zfile_fread (p, size, 1, z);

    xfds = xfdslaves;
    while (xfds) {
	uaecptr start = xfds->start;
	memset(&xregs, 0, sizeof xregs);
	pl(codeptr + freememaddr, codememory + size); // reset start of "free memory"
	xregs.regs[0] = size; // D0
	xregs.regs[8] = codememory; // A0
	xregs.regs[9] = bufferinfo; // A1
	xregs.regs[15] = FAKEMEM_SIZE; // A7
	pl(xfdmemory + bufferinfo + 0x00, codememory); // SourceBuffer
	pl(xfdmemory + bufferinfo + 0x04, size); // SourceBufLen
	xm68k_setpc(gl(xfdmemory + start + 16)); // recog code
	if (xregs.pc) {
	    if (execute68k()) {
		if (xregs.regs[0]) {
		    write_log("XFD slave '%s' recognised the compressed data\n", xfds->name);
		    xm68k_setpc(gl(xfdmemory + start + 20)); // decomp code
		    if (xregs.pc) {
			struct zfile *zz = decomp(z, xfds, size);
			if (zz) {
			    zfout = zz;
			    break;
			}
		    }
		}
	    }
	}
	xfds = xfds->next;
    }

    return zfout;
}
