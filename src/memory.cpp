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
#include "memory.h"
#include "ersatz.h"
#include "zfile.h"
#include "custom.h"
#include "events.h"
#include "newcpu.h"
#include "autoconf.h"
#include "savestate.h"
#include "crc32.h"
#include "gui.h"

#ifdef ANDROIDSDL
#include <android/log.h> 
#endif

#ifdef JIT
/* Set by each memory handler that does not simply access real memory. */
int special_mem;
#endif

int ersatzkickfile;

uae_u32 allocated_chipmem;
uae_u32 allocated_fastmem;
uae_u32 allocated_bogomem;
uae_u32 allocated_gfxmem;
uae_u32 allocated_z3fastmem;

uae_u32 max_z3fastmem = 64 * 1024 * 1024;

static size_t bootrom_filepos, chip_filepos, bogo_filepos, rom_filepos, a3000lmem_filepos, a3000hmem_filepos;

/* Set if we notice during initialization that settings changed,
   and we must clear all memory to prevent bogus contents from confusing
   the Kickstart.  */
static int need_hardreset;

/* The address space setting used during the last reset.  */
static int last_address_space_24;

static struct romlist *rl;
static int romlist_cnt;

char *romlist_get (struct romdata *rd)
{
    int i;

    if (!rd)
	return 0;
    for (i = 0; i < romlist_cnt; i++) {
	if (rl[i].rd->id == rd->id)
	    return rl[i].path;
    }
    return 0;
}

static struct romlist *romlist_getrl (struct romdata *rd)
{
    int i;
    
    if (!rd)
	return 0;
    for (i = 0; i < romlist_cnt; i++) {
	if (rl[i].rd == rd)
	    return &rl[i];
    }
    return 0;
}

static void romlist_cleanup (void);
void romlist_add (char *path, struct romdata *rd)
{
    struct romlist *rl2;

    if (path == NULL || rd == NULL) {
	romlist_cleanup ();
	return;
    }
    romlist_cnt++;
    rl = (struct romlist *)realloc (rl, sizeof (struct romlist) * romlist_cnt);
    rl2 = rl + romlist_cnt - 1;
    rl2->path = my_strdup (path);
    rl2->rd = rd;
}


struct romdata *getromdatabypath(char *path)
{
    int i;
    for (i = 0; i < romlist_cnt; i++) {
	struct romdata *rd = rl[i].rd;
	if (rd->configname && path[0] == ':') {
	    if (!strcmp(path + 1, rd->configname))
		return rd;
	}
	if (!strcmp(rl[i].path, path))
	    return rl[i].rd;
    }
    return NULL;
}

#define NEXT_ROM_ID 73

static struct romheader romheaders[] = {
    { "Freezer Cartridges", 1 },
    { "Arcadia Games", 2 },
    { NULL, 0 }
};

#define ALTROM(id,grp,num,size,flags,crc32,a,b,c,d,e) \
    { "X", 0, 0, 0, 0, 0, size, id, 0, 0, flags, (grp << 16) | num, 0, NULL, crc32, a, b, c, d, e },
#define ALTROMPN(id,grp,num,size,flags,pn,crc32,a,b,c,d,e) \
    { "X", 0, 0, 0, 0, 0, size, id, 0, 0, flags, (grp << 16) | num, 0, pn, crc32, a, b, c, d, e },

static struct romdata roms[] = {
    { "Cloanto Amiga Forever ROM key", 0, 0, 0, 0, 0, 2069, 0, 0, 1, ROMTYPE_KEY, 0, 0, NULL,
	0x869ae1b1, 0x801bbab3,0x2e3d3738,0x6dd1636d,0x4f1d6fa7,0xe21d5874 },
    { "Cloanto Amiga Forever 2006 ROM key", 0, 0, 0, 0, 0, 750, 48, 0, 1, ROMTYPE_KEY, 0, 0, NULL,
	0xb01c4b56, 0xbba8e5cd,0x118b8d92,0xafed5693,0x5eeb9770,0x2a662d8f },

    { "KS ROM v1.0 (A1000)(NTSC)", 1, 0, 1, 0, "A1000\0", 262144, 1, 0, 0, ROMTYPE_KICK, 0, 0, NULL,
	0x299790ff, 0x00C15406,0xBEB4B8AB,0x1A16AA66,0xC05860E1,0xA7C1AD79 },
    { "KS ROM v1.1 (A1000)(NTSC)", 1, 1, 31, 34, "A1000\0", 262144, 2, 0, 0, ROMTYPE_KICK, 0, 0, NULL,
	0xd060572a, 0x4192C505,0xD130F446,0xB2ADA6BD,0xC91DAE73,0x0ACAFB4C},
    { "KS ROM v1.1 (A1000)(PAL)", 1, 1, 31, 34, "A1000\0", 262144, 3, 0, 0, ROMTYPE_KICK, 0, 0, NULL,
	0xec86dae2, 0x16DF8B5F,0xD524C5A1,0xC7584B24,0x57AC15AF,0xF9E3AD6D },
    { "KS ROM v1.2 (A1000)", 1, 2, 33, 166, "A1000\0", 262144, 4, 0, 0, ROMTYPE_KICK, 0, 0, NULL,
	0x9ed783d0, 0x6A7BFB5D,0xBD6B8F17,0x9F03DA84,0xD8D95282,0x67B6273B },
    { "KS ROM v1.2 (A500,A1000,A2000)", 1, 2, 33, 180, "A500\0A1000\0A2000\0", 262144, 5, 0, 0, ROMTYPE_KICK, 0, 0, "315093-01",
	0xa6ce1636, 0x11F9E62C,0xF299F721,0x84835B7B,0x2A70A163,0x33FC0D88 },
    { "KS ROM v1.3 (A500,A1000,A2000)", 1, 3, 34, 5, "A500\0A1000\0A2000\0", 262144, 6, 0, 0, ROMTYPE_KICK, 0, 0, "315093-02",
	0xc4f0f55f, 0x891E9A54,0x7772FE0C,0x6C19B610,0xBAF8BC4E,0xA7FCB785 },
    { "KS ROM v1.3 (A3000)(SK)", 1, 3, 34, 5, "A3000\0", 262144, 32, 0, 0, ROMTYPE_KICK, 0, 0, NULL,
	0xe0f37258, 0xC39BD909,0x4D4E5F4E,0x28C1411F,0x30869504,0x06062E87 },
    { "KS ROM v1.4 (A3000)", 1, 4, 36, 16, "A3000\0", 524288, 59, 3, 0, ROMTYPE_KICK, 0, 0, NULL,
	0xbc0ec13f, 0xF76316BF,0x36DFF14B,0x20FA349E,0xD02E4B11,0xDD932B07 },
    ALTROMPN(59, 1, 1, 262144, ROMTYPE_EVEN, "390629-02", 0x58327536,0xd1713d7f,0x31474a59,0x48e6d488,0xe3368606,0x1cf3d1e2)
    ALTROMPN(59, 1, 2, 262144, ROMTYPE_ODD , "390630-02", 0xfe2f7fb9,0xc05c9c52,0xd014c66f,0x9019152b,0x3f2a2adc,0x2c678794)
    { "KS ROM v2.04 (A500+)", 2, 4, 37, 175, "A500+\0", 524288, 7, 0, 0, ROMTYPE_KICK, 0, 0, "390979-01",
	0xc3bdb240, 0xC5839F5C,0xB98A7A89,0x47065C3E,0xD2F14F5F,0x42E334A1 },
    { "KS ROM v2.05 (A600)", 2, 5, 37, 299, "A600\0", 524288, 8, 0, 0, ROMTYPE_KICK, 0, 0, "391388-01",
	0x83028fb5, 0x87508DE8,0x34DC7EB4,0x7359CEDE,0x72D2E3C8,0xA2E5D8DB },
    { "KS ROM v2.05 (A600HD)", 2, 5, 37, 300, "A600HD\0A600\0", 524288, 9, 0, 0, ROMTYPE_KICK, 0, 0, "391304-01",
	0x64466c2a, 0xF72D8914,0x8DAC39C6,0x96E30B10,0x859EBC85,0x9226637B },
    { "KS ROM v2.05 (A600HD)", 2, 5, 37, 350, "A600HD\0A600\0", 524288, 10, 0, 0, ROMTYPE_KICK, 0, 0, "391304-02",
	0x43b0df7b, 0x02843C42,0x53BBD29A,0xBA535B0A,0xA3BD9A85,0x034ECDE4 },
    { "KS ROM v2.04 (A3000)", 2, 4, 37, 175, "A3000\0", 524288, 71, 3, 0, ROMTYPE_KICK, 0, 0, NULL,
	0x234a7233, 0xd82ebb59,0xafc53540,0xddf2d718,0x7ecf239b,0x7ea91590 },
    ALTROMPN(71, 1, 1, 262144, ROMTYPE_EVEN, "390629-03", 0x7db1332b,0x48f14b31,0x279da675,0x7848df6f,0xeb531881,0x8f8f576c)
    ALTROMPN(71, 1, 2, 262144, ROMTYPE_ODD , "390630-03", 0xa245dbdf,0x83bab8e9,0x5d378b55,0xb0c6ae65,0x61385a96,0xf638598f)

    { "KS ROM v3.0 (A1200)", 3, 0, 39, 106, "A1200\0", 524288, 11, 0, 0, ROMTYPE_KICK, 0, 0, NULL,
	0x6c9b07d2, 0x70033828,0x182FFFC7,0xED106E53,0x73A8B89D,0xDA76FAA5 },
    ALTROMPN(11, 1, 1, 262144, ROMTYPE_EVEN, "391523-01", 0xc742a412,0x999eb81c,0x65dfd07a,0x71ee1931,0x5d99c7eb,0x858ab186)
    ALTROMPN(11, 1, 2, 262144, ROMTYPE_ODD , "391524-01", 0xd55c6ec6,0x3341108d,0x3a402882,0xb5ef9d3b,0x242cbf3c,0x8ab1a3e9)
    { "KS ROM v3.0 (A4000)", 3, 0, 39, 106, "A4000\0", 524288, 12, 2 | 4, 0, ROMTYPE_KICK, 0, 0, NULL,
	0x9e6ac152, 0xF0B4E9E2,0x9E12218C,0x2D5BD702,0x0E4E7852,0x97D91FD7 },
    ALTROMPN(12, 1, 1, 262144, ROMTYPE_EVEN, "391513-02", 0x36f64dd0,0x196e9f3f,0x9cad934e,0x181c07da,0x33083b1f,0x0a3c702f)
    ALTROMPN(12, 1, 2, 262144, ROMTYPE_ODD , "391514-02", 0x17266a55,0x42fbed34,0x53d1f11c,0xcbde89a9,0x826f2d11,0x75cca5cc)
    { "KS ROM v3.1 (A4000)", 3, 1, 40, 70, "A4000\0", 524288, 13, 2 | 4, 0, ROMTYPE_KICK, 0, 0, NULL,
	0x2b4566f1, 0x81c631dd,0x096bbb31,0xd2af9029,0x9c76b774,0xdb74076c },
    ALTROM(13, 1, 1, 262144, ROMTYPE_EVEN, 0xf9cbecc9,0x138d8cb4,0x3b8312fe,0x16d69070,0xde607469,0xb3d4078e)
    ALTROM(13, 1, 2, 262144, ROMTYPE_ODD , 0xf8248355,0xc2379547,0x9fae3910,0xc185512c,0xa268b82f,0x1ae4fe05)
    { "KS ROM v3.1 (A500,A600,A2000)", 3, 1, 40, 63, "A500\0A600\0A2000\0", 524288, 14, 0, 0, ROMTYPE_KICK, 0, 0, NULL,
	0xfc24ae0d, 0x3B7F1493,0xB27E2128,0x30F989F2,0x6CA76C02,0x049F09CA },
    { "KS ROM v3.1 (A1200)", 3, 1, 40, 68, "A1200\0", 524288, 15, 1, 0, ROMTYPE_KICK, 0, 0, NULL,
	0x1483a091, 0xE2154572,0x3FE8374E,0x91342617,0x604F1B3D,0x703094F1 },
    ALTROMPN(15, 1, 1, 262144, ROMTYPE_EVEN, "391773-01", 0x08dbf275,0xb8800f5f,0x90929810,0x9ea69690,0xb1b8523f,0xa22ddb37)
    ALTROMPN(15, 1, 2, 262144, ROMTYPE_ODD , "391774-01", 0x16c07bf8,0x90e331be,0x1970b0e5,0x3f53a9b0,0x390b51b5,0x9b3869c2)
    { "KS ROM v3.1 (A3000)", 3, 1, 40, 68, "A3000\0", 524288, 61, 2, 0, ROMTYPE_KICK, 0, 0, NULL,
	0xefb239cc, 0xF8E210D7,0x2B4C4853,0xE0C9B85D,0x223BA20E,0x3D1B36EE },
    ALTROM(61, 1, 1, 262144, ROMTYPE_EVEN, 0x286b9a0d,0x6763a225,0x8ec493f7,0x408cf663,0x110dae9a,0x17803ad1)
    ALTROM(61, 1, 2, 262144, ROMTYPE_ODD , 0x0b8cde6a,0x5f02e97b,0x48ebbba8,0x7d516a56,0xb0400c6f,0xc3434d8d)
    { "KS ROM v3.1 (A4000)(Cloanto)", 3, 1, 40, 68, "A4000\0", 524288, 31, 2 | 4, 1, ROMTYPE_KICK, 0, 0, NULL,
	0x43b6dd22, 0xC3C48116,0x0866E60D,0x085E436A,0x24DB3617,0xFF60B5F9 },
    { "KS ROM v3.1 (A4000)", 3, 1, 40, 68, "A4000\0", 524288, 16, 2 | 4, 0, ROMTYPE_KICK, 0, 0, NULL,
	0xd6bae334, 0x5FE04842,0xD04A4897,0x20F0F4BB,0x0E469481,0x99406F49 },
    ALTROM(16, 1, 1, 262144, ROMTYPE_EVEN, 0xb2af34f8,0x24e52b5e,0xfc020495,0x17387ab7,0xb1a1475f,0xc540350e)
    ALTROM(16, 1, 2, 262144, ROMTYPE_ODD , 0xe65636a3,0x313c7cbd,0xa5779e56,0xf19a41d3,0x4e760f51,0x7626d882)
    { "KS ROM v3.1 (A4000T)", 3, 1, 40, 70, "A4000T\0", 524288, 17, 2 | 4, 0, ROMTYPE_KICK, 0, 0, NULL,
	0x75932c3a, 0xB0EC8B84,0xD6768321,0xE01209F1,0x1E6248F2,0xF5281A21 },
    ALTROMPN(17, 1, 1, 262144, ROMTYPE_EVEN, "391657-01", 0x0ca94f70,0xb3806eda,0xcb3362fc,0x16a154ce,0x1eeec5bf,0x5bc24789)
    ALTROMPN(17, 1, 2, 262144, ROMTYPE_ODD , "391658-01", 0xdfe03120,0xcd7a706c,0x431b04d8,0x7814d3a2,0xd8b39710,0x0cf44c0c)
    { "KS ROM v3.X (A4000)(Cloanto)", 3, 10, 45, 57, "A4000\0", 524288, 46, 2 | 4, 0, ROMTYPE_KICK, 0, 0, NULL,
	0x3ac99edc, 0x3cbfc9e1,0xfe396360,0x157bd161,0xde74fc90,0x1abee7ec },

    { "CD32 KS ROM v3.1", 3, 1, 40, 60, "CD32\0", 524288, 18, 1, 0, ROMTYPE_KICKCD32, 0, 0, NULL,
	0x1e62d4a5, 0x3525BE88,0x87F79B59,0x29E017B4,0x2380A79E,0xDFEE542D },
    { "CD32 extended ROM", 3, 1, 40, 60, "CD32\0", 524288, 19, 1, 0, ROMTYPE_EXTCD32, 0, 0, NULL,
	0x87746be2, 0x5BEF3D62,0x8CE59CC0,0x2A66E6E4,0xAE0DA48F,0x60E78F7F },

    /* plain CD32 rom */
    { "CD32 ROM (KS + extended)", 3, 1, 40, 60, "CD32\0", 2 * 524288, 64, 1, 0, ROMTYPE_KICKCD32 | ROMTYPE_EXTCD32 | ROMTYPE_CD32, 0, 0, NULL,
	0xf5d4f3c8, 0x9fa14825,0xc40a2475,0xa2eba5cf,0x325bd483,0xc447e7c1 },
    /* real CD32 rom dump 391640-03 */
    ALTROMPN(64, 1, 1, 2 * 524288, ROMTYPE_CD32, "391640-03", 0xa4fbc94a, 0x816ce6c5,0x07787585,0x0c7d4345,0x2230a9ba,0x3a2902db )
   
    { "CD32 MPEG Cartridge ROM", 3, 1, 40, 30, "CD32FMV\0", 262144, 23, 1, 0, ROMTYPE_CD32CART, 0, 0, NULL,
	0xc35c37bf, 0x03ca81c7,0xa7b259cf,0x64bc9582,0x863eca0f,0x6529f435 },

    { "CDTV extended ROM v1.00", 1, 0, 1, 0, "CDTV\0", 262144, 20, 0, 0, ROMTYPE_EXTCDTV, 0, 0, NULL,
	0x42baa124, 0x7BA40FFA,0x17E500ED,0x9FED041F,0x3424BD81,0xD9C907BE },
    ALTROMPN(20, 1, 1, 131072, ROMTYPE_EVEN | ROMTYPE_8BIT, "252606-01", 0x791cb14b,0x277a1778,0x92449635,0x3ffe56be,0x68063d2a,0x334360e4)
    ALTROMPN(20, 1, 2, 131072, ROMTYPE_ODD  | ROMTYPE_8BIT, "252607-01", 0xaccbbc2e,0x41b06d16,0x79c6e693,0x3c3378b7,0x626025f7,0x641ebc5c)
    { "CDTV extended ROM v2.07", 2, 7, 2, 7, "CDTV\0", 262144, 22, 0, 0, ROMTYPE_EXTCDTV, 0, 0, NULL,
	0xceae68d2, 0x5BC114BB,0xA29F60A6,0x14A31174,0x5B3E2464,0xBFA06846 },
    ALTROM(22, 1, 1, 131072, ROMTYPE_EVEN | ROMTYPE_8BIT, 0x36d73cb8,0x9574e546,0x4b390697,0xf28f9a43,0x4e604e5e,0xf5e5490a)
    ALTROM(22, 1, 2, 131072, ROMTYPE_ODD  | ROMTYPE_8BIT, 0x6e84dce7,0x01a0679e,0x895a1a0f,0x559c7253,0xf539606b,0xd447b54f)
    { "CDTV/A570 extended ROM v2.30", 2, 30, 2, 30, "CDTV\0", 262144, 21, 0, 0, ROMTYPE_EXTCDTV, 0, 0, "391298-01",
	0x30b54232, 0xED7E461D,0x1FFF3CDA,0x321631AE,0x42B80E3C,0xD4FA5EBB },
    ALTROM(21, 1, 1, 131072, ROMTYPE_EVEN | ROMTYPE_8BIT, 0x48e4d74f,0x54946054,0x2269e410,0x36018402,0xe1f6b855,0xfd89092b)
    ALTROM(21, 1, 2, 131072, ROMTYPE_ODD  | ROMTYPE_8BIT, 0x8a54f362,0x03df800f,0x032046fd,0x892f6e7e,0xec08b76d,0x33981e8c)

    { "A1000 bootstrap ROM", 0, 0, 0, 0, "A1000\0", 65536, 24, 0, 0, ROMTYPE_KICK, 0, 0, NULL,
	0x0b1ad2d0, 0xBA93B8B8,0x5CA0D83A,0x68225CC3,0x3B95050D,0x72D2FDD7 },
    ALTROM(24, 1, 1, 8192,           0, 0x62f11c04, 0xC87F9FAD,0xA4EE4E69,0xF3CCA0C3,0x6193BE82,0x2B9F5FE6)
    ALTROMPN(24, 2, 1, 4096, ROMTYPE_EVEN | ROMTYPE_8BIT, "252179-01", 0x42553bc4,0x8855a97f,0x7a44e3f6,0x2d1c88d9,0x38fee1f4,0xc606af5b)
    ALTROMPN(24, 2, 2, 4096, ROMTYPE_ODD  | ROMTYPE_8BIT, "252180-01", 0x8e5b9a37,0xd10f1564,0xb99f5ffe,0x108fa042,0x362e877f,0x569de2c3)

    { "The Diagnostic 2.0 (Logica)", 2, 0, 2, 0, "LOGICA\0", 524288, 72, 0, 0, ROMTYPE_KICK | ROMTYPE_SPECIALKICK, 0, 0, NULL,
	0x8484f426, 0xba10d161,0x66b2e2d6,0x177c979c,0x99edf846,0x2b21651e },

    { "Freezer: Action Replay Mk I v1.00", 1, 0, 1, 0, "AR\0", 65536, 52, 0, 0, ROMTYPE_AR, 0, 1, NULL,
	0x2d921771, 0x1EAD9DDA,0x2DAD2914,0x6441F5EF,0x72183750,0x22E01248 },
    ALTROM(52, 1, 1, 32768, ROMTYPE_EVEN | ROMTYPE_8BIT, 0x82d6eb87, 0x7c9bac11,0x28666017,0xeee6f019,0x63fb3890,0x7fbea355)
    ALTROM(52, 1, 2, 32768, ROMTYPE_ODD  | ROMTYPE_8BIT, 0x40ae490c, 0x81d8e432,0x01b73fd9,0x2e204ebd,0x68af8602,0xb62ce397)
    { "Freezer: Action Replay Mk I v1.50", 1, 50, 1, 50, "AR\0", 65536, 25, 0, 0, ROMTYPE_AR, 0, 1, NULL,
	0xd4ce0675, 0x843B433B,0x2C56640E,0x045D5FDC,0x854DC6B1,0xA4964E7C },
    ALTROM(25, 1, 1, 32768, ROMTYPE_EVEN | ROMTYPE_8BIT, 0x7fbd6de2, 0xb5f71a5c,0x09d65ecc,0xa8a3bc93,0x93558461,0xca190228)
    ALTROM(25, 1, 2, 32768, ROMTYPE_ODD  | ROMTYPE_8BIT, 0x43018069, 0xad8ff242,0xb2cbf125,0x1fc53a73,0x581cf57a,0xb69cee00)
    { "Freezer: Action Replay Mk II v2.05", 2, 5, 2, 5, "AR\0", 131072, 26, 0, 0, ROMTYPE_AR, 0, 1, NULL,
	0x1287301f, 0xF6601DE8,0x888F0050,0x72BF562B,0x9F533BBC,0xAF1B0074 },
    { "Freezer: Action Replay Mk II v2.12", 2, 12, 2, 12, "AR\0", 131072, 27, 0, 0, ROMTYPE_AR, 0, 1, NULL,
	0x804d0361, 0x3194A07A,0x0A82D8B5,0xF2B6AEFA,0x3CA581D6,0x8BA8762B },
    { "Freezer: Action Replay Mk II v2.14", 2, 14, 2, 14, "AR\0", 131072, 28, 0, 0, ROMTYPE_AR, 0, 1, NULL,
	0x49650e4f, 0x255D6DF6,0x3A4EAB0A,0x838EB1A1,0x6A267B09,0x59DFF634 },
    { "Freezer: Action Replay Mk III v3.09", 3, 9, 3, 9, "AR\0", 262144, 29, 0, 0, ROMTYPE_AR, 0, 1, NULL,
	0x0ed9b5aa, 0x0FF3170A,0xBBF0CA64,0xC9DD93D6,0xEC0C7A01,0xB5436824 },
    ALTROM(29, 1, 1, 131072, ROMTYPE_EVEN | ROMTYPE_8BIT, 0x2b84519f, 0x7841873b,0xf009d834,0x1dfa2794,0xb3751bac,0xf86adcc8)
    ALTROM(29, 1, 2, 131072, ROMTYPE_ODD  | ROMTYPE_8BIT, 0x1d35bd56, 0x6464be16,0x26b51949,0x9e76e4e3,0x409e8016,0x515d48b6)
    { "Freezer: Action Replay Mk III v3.17", 3, 17, 3, 17, "AR\0", 262144, 30, 0, 0, ROMTYPE_AR, 0, 1, NULL,
	0xc8a16406, 0x5D4987C2,0xE3FFEA8B,0x1B02E314,0x30EF190F,0x2DB76542 },
    { "Freezer: Action Replay 1200", 0, 0, 0, 0, "AR\0", 262144, 47, 0, 0, ROMTYPE_AR, 0, 1, NULL,
	0x8d760101, 0x0F6AB834,0x2810094A,0xC0642F62,0xBA42F78B,0xC0B07E6A },

    { "Freezer: Action Cartridge Super IV Professional", 0, 0, 0, 0, "SUPERIV\0", 0, 62, 0, 0, ROMTYPE_SUPERIV, 0, 1, NULL,
	0xffffffff, 0, 0, 0, 0, 0, "SuperIV" },
    { "Freezer: Action Cart. Super IV Pro (+ROM v4.3)", 4, 3, 4, 3, "SUPERIV\0", 170368, 60, 0, 0, ROMTYPE_SUPERIV, 0, 1, NULL,
	0xe668a0be, 0x633A6E65,0xA93580B8,0xDDB0BE9C,0x9A64D4A1,0x7D4B4801 },
    { "Freezer: X-Power Professional 500 v1.2", 1, 2, 1, 2, "XPOWER\0", 131072, 65, 0, 0, ROMTYPE_XPOWER, 0, 1, NULL,
	0x9e70c231, 0xa2977a1c,0x41a8ca7d,0x4af4a168,0x726da542,0x179d5963 },
    ALTROM(65, 1, 1, 65536, ROMTYPE_EVEN|ROMTYPE_SCRAMBLED|ROMTYPE_8BIT, 0xf98742e4,0xe8e683ba,0xd8b38d1f,0x79f3ad83,0xa9e67c6f,0xa91dc96c)
    ALTROM(65, 1, 2, 65536, ROMTYPE_ODD |ROMTYPE_SCRAMBLED|ROMTYPE_8BIT, 0xdfb9984b,0x8d6bdd49,0x469ec8e2,0x0143fbb3,0x72e92500,0x99f07910)
    { "Freezer: X-Power Professional 500 v1.3", 1, 2, 1, 2, "XPOWER\0", 131072, 68, 0, 0, ROMTYPE_XPOWER, 0, 1, NULL,
	0x31e057f0, 0x84650266,0x465d1859,0x7fd71dee,0x00775930,0xb7e450ee },
    ALTROM(68, 1, 1, 65536, ROMTYPE_EVEN|ROMTYPE_SCRAMBLED|ROMTYPE_8BIT, 0x0b2ce0c7,0x45ad5456,0x89192404,0x956f47ce,0xf66a5274,0x57ace33b)
    ALTROM(68, 1, 2, 65536, ROMTYPE_ODD |ROMTYPE_SCRAMBLED|ROMTYPE_8BIT, 0x34580c35,0x8ad42566,0x7364f238,0x978f4381,0x08f8d5ec,0x470e72ea)
    { "Freezer: Nordic Power v1.5", 1, 5, 1, 5, "NPOWER\0", 65536, 69, 0, 0, ROMTYPE_NORDIC, 0, 1, NULL,
	0x83b4b21c, 0xc56ced25,0x506a5aab,0x3fa13813,0x4fc9e5ae,0x0f9d3709 },
    ALTROM(69, 1, 1, 32768, ROMTYPE_EVEN|ROMTYPE_SCRAMBLED|ROMTYPE_8BIT, 0xdd207174,0xae67652d,0x64f5db20,0x0f4b2110,0xee59567f,0xfbd90a1b)
    ALTROM(69, 1, 2, 32768, ROMTYPE_ODD |ROMTYPE_SCRAMBLED|ROMTYPE_8BIT, 0x8f93d85d,0x73c62d21,0x40c0c092,0x6315b702,0xdd5d0f05,0x3dad7fab)
    { "Freezer: Nordic Power v2.0", 2, 0, 2, 0, "NPOWER\0", 65536, 67, 0, 0, ROMTYPE_NORDIC, 0, 1, NULL,
	0xa4db2906, 0x0aec68f7,0x25470c89,0x6b699ff4,0x6623dec5,0xc777466e },
    ALTROM(67, 1, 1, 32768, ROMTYPE_EVEN|ROMTYPE_SCRAMBLED|ROMTYPE_8BIT, 0xb21be46c,0x50dc607c,0xce976bbd,0x3841eaf0,0x591ddc7e,0xa1939ad2)
    ALTROM(67, 1, 2, 32768, ROMTYPE_ODD |ROMTYPE_SCRAMBLED|ROMTYPE_8BIT, 0x96057aed,0xdd9209e2,0x1d5edfc1,0xcdb52abe,0x93de0f35,0xc43da696)
    { "Freezer: Nordic Power v3.0", 3, 0, 3, 0, "NPOWER\0", 65536, 70, 0, 0, ROMTYPE_NORDIC, 0, 1, NULL,
	0x72850aef, 0x59c91d1f,0xa8f118f9,0x0bdba05a,0x9ae788d7,0x7a6cc7c9 },
    ALTROM(70, 1, 1, 32768, ROMTYPE_EVEN|ROMTYPE_SCRAMBLED|ROMTYPE_8BIT, 0xf3330e1f,0x3a597db2,0xb7d11b6c,0xb8e13496,0xc215f223,0x88c4ca3c)
    ALTROM(70, 1, 2, 32768, ROMTYPE_EVEN|ROMTYPE_SCRAMBLED|ROMTYPE_8BIT, 0xee58e0f9,0x4148f4cb,0xb42cec33,0x8ca144de,0xd4f54118,0xe0f185dd)
    { "Freezer: HRTMon v2.30 (built-in)", 0, 0, 0, 0, "HRTMON\0", 0, 63, 0, 0, ROMTYPE_HRTMON, 0, 1, NULL,
	0xffffffff, 0, 0, 0, 0, 0, "HRTMon" },

    { "A590/A2091 SCSI boot ROM", 6, 0, 6, 0, "A590\0A2091\0", 16384, 53, 0, 0, ROMTYPE_A2091BOOT, 0, 0, NULL,
	0x8396cf4e, 0x5E03BC61,0x8C862ABE,0x7BF79723,0xB4EEF4D2,0x1859A0F2 },
    ALTROM(53, 1, 1, 8192, ROMTYPE_ODD  | ROMTYPE_8BIT, 0xb0b8cf24,0xfcf40175,0x05f4d441,0x814b45d5,0x59c19eab,0x43816b30)
    ALTROM(53, 1, 2, 8192, ROMTYPE_EVEN | ROMTYPE_8BIT, 0x2e77bbff,0x8a098845,0x068f32cf,0xa4d34a27,0x8cd290f6,0x1d35a52c)
    { "A590/A2091 SCSI boot ROM", 6, 6, 6, 6, "A590\0A2091\0", 16384, 54, 0, 0, ROMTYPE_A2091BOOT, 0, 0, NULL,
	0x33e00a7a, 0x739BB828,0xE874F064,0x9360F59D,0x26B5ED3F,0xBC99BB66 },
    ALTROM(54, 1, 1, 8192, ROMTYPE_ODD  | ROMTYPE_8BIT, 0xe536bbb2,0xfd7f8a6d,0xa18c1b02,0xd07eb990,0xc2467a24,0x183ede12)
    ALTROM(54, 1, 2, 8192, ROMTYPE_EVEN | ROMTYPE_8BIT, 0xc0871d25,0xe155f18a,0xbb90cf82,0x0589c15e,0x70559d3b,0x6b391af8)
    { "A590/A2091 SCSI boot ROM", 7, 0, 7, 0, "A590\0A2091\0", 16384, 55, 0, 0, ROMTYPE_A2091BOOT, 0, 0, NULL,
	0x714a97a2, 0xE50F01BA,0xF2899892,0x85547863,0x72A82C33,0x3C91276E },
    ALTROM(55, 1, 1, 8192, ROMTYPE_ODD  | ROMTYPE_8BIT, 0xa9ccffed,0x149f5bd5,0x2e2d2990,0x4e3de483,0xb9ad7724,0x48e9278e)
    ALTROM(55, 1, 2, 8192, ROMTYPE_EVEN | ROMTYPE_8BIT, 0x2942747a,0xdbd7648e,0x79c75333,0x7ff3e4f4,0x91de224b,0xf05e6bb6)
    { "A590/A2091 SCSI Guru boot ROM", 6, 14, 6, 14, "A590\0A2091\0", 32768, 56, 0, 0, ROMTYPE_A2091BOOT, 0, 0, NULL,
	0x04e52f93, 0x6DA21B6F,0x5E8F8837,0xD64507CD,0x8A4D5CDC,0xAC4F426B },
    { "A4091 SCSI boot ROM", 40, 9, 40, 9, "A4091\0", 32768, 57, 0, 0, ROMTYPE_A4091BOOT, 0, 0, NULL,
	0x00000000, 0, 0, 0, 0, 0 },
    { "A4091 SCSI boot ROM", 40, 13, 40, 13, "A4091\0", 32768, 58, 0, 0, ROMTYPE_A4091BOOT, 0, 0, NULL,
	0x54cb9e85, 0x3CE66919,0xF6FD6797,0x4923A12D,0x91B730F1,0xFFB4A7BA },

    { "Arcadia OnePlay 2.11", 0, 0, 0, 0, "ARCADIA\0", 0, 49, 0, 0, ROMTYPE_ARCADIABIOS, 0, 0 },
    { "Arcadia TenPlay 2.11", 0, 0, 0, 0, "ARCADIA\0", 0, 50, 0, 0, ROMTYPE_ARCADIABIOS, 0, 0 },
    { "Arcadia OnePlay 3.00", 0, 0, 0, 0, "ARCADIA\0", 0, 51, 0, 0, ROMTYPE_ARCADIABIOS, 0, 0 },

    { "Arcadia SportTime Table Hockey", 0, 0, 0, 0, "ARCADIA\0", 0, 33, 0, 0, ROMTYPE_ARCADIAGAME, 0, 2 },
    { "Arcadia SportTime Bowling", 0, 0, 0, 0, "ARCADIA\0", 0, 34, 0, 0, ROMTYPE_ARCADIAGAME, 0, 2 },
    { "Arcadia World Darts", 0, 0, 0, 0, "ARCADIA\0", 0, 35, 0, 0, ROMTYPE_ARCADIAGAME, 0, 2 },
    { "Arcadia Magic Johnson's Fast Break", 0, 0, 0, 0, "ARCADIA\0", 0, 36, 0, 0, ROMTYPE_ARCADIAGAME, 0, 2 },
    { "Arcadia Leader Board Golf", 0, 0, 0, 0, "ARCADIA\0", 0, 37, 0, 0, ROMTYPE_ARCADIAGAME, 0, 2 },
    { "Arcadia Leader Board Golf (alt)", 0, 0, 0, 0, "ARCADIA\0", 0, 38, 0, 0, ROMTYPE_ARCADIAGAME, 0, 2 },
    { "Arcadia Ninja Mission", 0, 0, 0, 0, "ARCADIA\0", 0, 39, 0, 0, ROMTYPE_ARCADIAGAME, 0, 2 },
    { "Arcadia Road Wars", 0, 0, 0, 0, "ARCADIA\0", 0, 40, 0, 0, ROMTYPE_ARCADIAGAME, 0, 2 },
    { "Arcadia Sidewinder", 0, 0, 0, 0, "ARCADIA\0", 0, 41, 0, 0, ROMTYPE_ARCADIAGAME, 0, 2 },
    { "Arcadia Spot", 0, 0, 0, 0, "ARCADIA\0", 0, 42, 0, 0, ROMTYPE_ARCADIAGAME, 0, 2 },
    { "Arcadia Space Ranger", 0, 0, 0, 0, "ARCADIA\0", 0, 43, 0, 0, ROMTYPE_ARCADIAGAME, 0, 2 },
    { "Arcadia Xenon", 0, 0, 0, 0, "ARCADIA\0", 0, 44, 0, 0, ROMTYPE_ARCADIAGAME, 0, 2 },
    { "Arcadia World Trophy Soccer", 0, 0, 0, 0, "ARCADIA\0", 0, 45, 0, 0, ROMTYPE_ARCADIAGAME, 0, 2 },

    { NULL }
};

void romlist_clear (void)
{
    int i;
    int mask = 0;
    struct romdata *parent;
    char *pn;

    xfree (rl);
    rl = 0;
    romlist_cnt = 0;
    parent = 0;
    pn = NULL;
    for (i = 0; roms[i].name; i++) {
	struct romdata *rd = &roms[i];
	if (rd->group == 0) {
	    parent = rd;
	    mask = rd->type;
	    pn = parent->partnumber;
	} else {
	    rd->type &= ~ROMTYPE_MASK;
	    rd->type |= mask & ROMTYPE_MASK;
	    if (rd->partnumber && !pn) {
		char *newpn;
		if (parent->partnumber == NULL)
		    parent->partnumber = my_strdup ("");
		newpn = (char *)xcalloc (strlen (parent->partnumber) + 1 + strlen (rd->partnumber) + 1, 1);
		if (strlen (parent->partnumber) > 0) {
		    strcpy (newpn, parent->partnumber);
		    strcat (newpn, "/");
		}
		strcat (newpn, rd->partnumber);
		xfree (parent->partnumber);
		parent->partnumber = newpn;
	    }
	}
    }
}

/* remove rom entries that need 2 or more roms but not everyone is present */
static void romlist_cleanup (void)
{
    int i = 0;
    while (roms[i].name) {
	struct romdata *rd = &roms[i];
	int grp = rd->group >> 16;
        int ok = 1;
	int j = i;
	int k = i;
	while (rd->name && (rd->group >> 16) == grp && grp > 0) {
	    struct romlist *rl = romlist_getrl (rd);
	    if (!rl)
		ok = 0;
	    rd++;
	    j++;
	}
	if (ok == 0) {
	    while (i < j) {
		struct romlist *rl = romlist_getrl (&roms[i]);
		if (rl) {
		    int cnt = romlist_cnt - i - 1;
		    write_log ("%d '%s' removed from romlist\n", roms[k].name, rl->path);
		    xfree (rl->path);
		    if (cnt > 0)
			memmove (rl, rl + 1, cnt * sizeof (struct romlist));
		    romlist_cnt--;
		}
		i++;
	    }
	}
	i++;
    }
    for (i = 0; i < romlist_cnt; i++) {
	struct romlist *rll = &rl[i];
	//write_log ("%s (%s)\n", rll->rd->name, rll->path);
    }
}

struct romlist **getromlistbyident(int ver, int rev, int subver, int subrev, char *model, int all)
{
    int i, j, ok, out, max;
    struct romdata *rd;
    struct romlist **rdout, *rltmp;
    void *buf;
    static struct romlist rlstatic;
    
    for (i = 0; roms[i].name; i++);
    if (all)
	max = i;
    else
	max = romlist_cnt;
    buf = xmalloc((sizeof (struct romlist*) + sizeof (struct romlist)) * (i + 1));
    rdout = (struct romlist **) buf;
    rltmp = (struct romlist*)((uae_u8*)buf + (i + 1) * sizeof (struct romlist*));
    out = 0;
    for (i = 0; i < max; i++) {
        ok = 0;
	if (!all)
	    rd = rl[i].rd;
	else
	    rd = &roms[i];
	if (rd->group)
	    continue;
	if (model && !strcmpi(model, rd->name))
	    ok = 2;
	if (rd->ver == ver && (rev < 0 || rd->rev == rev)) {
	    if (subver >= 0) {
		if (rd->subver == subver && (subrev < 0 || rd->subrev == subrev) && rd->subver > 0)
		    ok = 1;
	    } else {
		ok = 1;
	    }
	}
	if (!ok)
	    continue;
	if (model && ok < 2) {
	    const char *p = rd->model;
	    ok = 0;
	    while (*p) {
		if (!strcmp(rd->model, model)) {
		    ok = 1;
		    break;
		}
		p = p + strlen(p) + 1;
	    }
	}
	if (!model && rd->type != ROMTYPE_KICK)
	    ok = 0;
	if (ok) {
	    if (all) {
		rdout[out++] = rltmp;
		rltmp->path = NULL;
		rltmp->rd = rd;
		rltmp++;
	    } else {
		rdout[out++] = &rl[i];
	    }
	}
    }
    if (out == 0) {
	xfree (rdout);
	return NULL;
    }
    for (i = 0; i < out; i++) {
	int v1 = rdout[i]->rd->subver * 1000 + rdout[i]->rd->subrev;
	for (j = i + 1; j < out; j++) {
	    int v2 = rdout[j]->rd->subver * 1000 + rdout[j]->rd->subrev;
	    if (v1 < v2) {
		struct romlist *rltmp = rdout[j];
		rdout[j] = rdout[i];
		rdout[i] = rltmp;
	    }
	}
    }
    rdout[out] = NULL;
    return rdout;
}

struct romdata *getarcadiarombyname (char *name)
{
    int i;
    for (i = 0; roms[i].name; i++) {
        if (roms[i].group == 0 && (roms[i].type == ROMTYPE_ARCADIAGAME || roms[i].type == ROMTYPE_ARCADIAGAME)) {
	    const char *p = roms[i].name;
	    p = p + strlen (p) + 1;
	    if (strlen (name) >= strlen (p) + 4) {
		char *p2 = name + strlen (name) - strlen (p) - 4;
		if (!memcmp (p, p2, strlen (p)) && !memcmp (p2 + strlen (p2) - 4, ".zip", 4))
		    return &roms[i];
	    }
	}
    }
    return NULL;
}

struct romlist **getarcadiaroms(void)
{
    int i, out, max;
    void *buf;
    struct romlist **rdout, *rltmp;

    max = 0;
    for (i = 0; roms[i].name; i++) {
	if (roms[i].group == 0 && (roms[i].type == ROMTYPE_ARCADIABIOS || roms[i].type == ROMTYPE_ARCADIAGAME))
	    max++;
    }
    buf = xmalloc((sizeof (struct romlist*) + sizeof (struct romlist)) * (max + 1));
    rdout = (struct romlist **)buf;
    rltmp = (struct romlist*)((uae_u8*)buf + (max + 1) * sizeof (struct romlist*));
    out = 0;
    for (i = 0; roms[i].name; i++) {
	if (roms[i].group == 0 && (roms[i].type == ROMTYPE_ARCADIABIOS || roms[i].type == ROMTYPE_ARCADIAGAME)) {
	    rdout[out++] = rltmp;
	    rltmp->path = NULL;
	    rltmp->rd = &roms[i];
	    rltmp++;
	}
    }
    rdout[out] = NULL;
    return rdout;
}

static int kickstart_checksum_do (uae_u8 *mem, int size)
{
    uae_u32 cksum = 0, prevck = 0;
    int i;
    for (i = 0; i < size; i+=4) {
	uae_u32 data = mem[i]*65536*256 + mem[i+1]*65536 + mem[i+2]*256 + mem[i+3];
	cksum += data;
	if (cksum < prevck)
	    cksum++;
	prevck = cksum;
    }
    return cksum == 0xffffffff;
}

#define ROM_KEY_NUM 3
struct rom_key {
    uae_u8 *key;
    int size;
};

static struct rom_key keyring[ROM_KEY_NUM];

static void addkey (uae_u8 *key, int size, const char *name)
{
    int i;

    if (key == NULL || size == 0) {
	xfree (key);
	return;
    }
    for (i = 0; i < ROM_KEY_NUM; i++) {
	if (keyring[i].key && keyring[i].size == size && !memcmp (keyring[i].key, key, size)) {
	    xfree (key);
	    return;
	}
    }
    for (i = 0; i < ROM_KEY_NUM; i++) {
	if (keyring[i].key == NULL)
	    break;
    }
    if (i == ROM_KEY_NUM) {
	xfree (key);
	return;
    }
    keyring[i].key = key;
    keyring[i].size = size;
    write_log ("ROM KEY '%s' %d bytes loaded\n", name, size);
}

static void addkeyfile (const char *path)
{
    struct zfile *f;
    int keysize;
    uae_u8 *keybuf;

    f = zfile_fopen (path, "rb");
    if (!f)
	return;
    zfile_fseek (f, 0, SEEK_END);
    keysize = zfile_ftell (f);
    if (keysize > 0) {
        zfile_fseek (f, 0, SEEK_SET);
        keybuf = (uae_u8 *)xmalloc (keysize);
        zfile_fread (keybuf, 1, keysize, f);
        addkey (keybuf, keysize, path);
    }
    zfile_fclose (f);
}

static void addkeydir (const char *path)
{
    char tmp[MAX_DPATH];

    strcpy (tmp, path);
    if (zfile_exists (tmp)) {
        int i;
        for (i = strlen (tmp) - 1; i > 0; i--) {
    	    if (tmp[i] == '\\' || tmp[i] == '/')
	        break;
	}
	tmp[i] = 0;
    }
    strcat (tmp, "/");
    strcat (tmp, "rom.key");
    addkeyfile (tmp);
}

int get_keyring (void)
{
    int i, num = 0;
    for (i = 0; i < ROM_KEY_NUM; i++) {
	if (keyring[i].key)
	    num++;
    }
    return num;
}

int load_keyring (struct uae_prefs *p, char *path)
{
    uae_u8 *keybuf;
    int keysize;
    char tmp[MAX_PATH], *d;
    int keyids[] = { 0, 48, -1 };
    int cnt, i;

    free_keyring();
    keybuf = target_load_keyfile(p, path, &keysize, tmp);
    addkey (keybuf, keysize, tmp);
    for (i = 0; keyids[i] >= 0; i++) {
	struct romdata *rd = getromdatabyid (keyids[i]);
	char *s;
	if (rd) {
	    s = romlist_get (rd);
	    if (s)
		addkeyfile (s);
	}
    }

    cnt = 0;
    for (;;) {
	keybuf = NULL;
	keysize = 0;
	tmp[0] = 0;
	switch (cnt)
	{
	case 0:
	  if (path)
    {
	    strcpy (tmp, path);
	    strcat (tmp, "rom.key");
	  }
	break;
	case 1:
	    if (p) {
		strcpy (tmp, p->path_rom);
		strcat (tmp, "rom.key");
	    }
	break;
	case 2:
	    strcpy (tmp, "roms/rom.key");
	break;
	case 3:
	    strcpy (tmp, start_path_data);
	    strcat (tmp, "rom.key");
	break;
	case 4:
	    sprintf (tmp, "%s../shared/rom/rom.key", start_path_data);
	break;
	case 5:
	    if (p) {
		for (i = 0; uae_archive_extensions[i]; i++) {
		    if (strstr(p->romfile, uae_archive_extensions[i]))
			break;
		}
		if (!uae_archive_extensions[i]) {
		    strcpy (tmp, p->romfile);
		    d = strrchr(tmp, '/');
		    if (!d)
			d = strrchr(tmp, '\\');
		    if (d)
			strcpy (d + 1, "rom.key");
		}
	    }
	break;
	case 6:
	return cnt;
	}
	cnt++;
	if (!tmp[0])
	    continue;
	addkeyfile (tmp);
    }
}
void free_keyring (void)
{
    int i;
    for (i = 0; i < ROM_KEY_NUM; i++)
	xfree (keyring[i].key);
    memset(keyring, 0, sizeof (struct rom_key) * ROM_KEY_NUM);
}

struct romdata *getromdatabyname (char *name)
{
    char tmp[MAX_PATH];
    int i = 0;
    while (roms[i].name) {
	    if (!roms[i].group) {
	      getromname (&roms[i], tmp);
	      if (!strcmp (tmp, name) || !strcmp (roms[i].name, name))
	          return &roms[i];
	    }
	    i++;
    }
    return 0;
}

struct romdata *getromdatabyid (int id)
{
    int i = 0;
    while (roms[i].name) {
	if (id == roms[i].id && roms[i].group == 0)
	    return &roms[i];
	i++;
    }
    return 0;
}

struct romdata *getromdatabyidgroup (int id, int group, int subitem)
{
    int i = 0;
    group = (group << 16) | subitem;
    while (roms[i].name) {
	if (id == roms[i].id && roms[i].group == group)
	    return &roms[i];
	i++;
    }
    return 0;
}

STATIC_INLINE int notcrc32(uae_u32 crc32)
{
    if (crc32 == 0xffffffff || crc32 == 0x00000000)
	return 1;
    return 0;
}

struct romdata *getromdatabycrc (uae_u32 crc32)
{
    int i = 0;
    while (roms[i].name) {
	if (roms[i].group == 0 && crc32 == roms[i].crc32 && !notcrc32(crc32))
	    return &roms[i];
	i++;
    }
    return 0;
}

static int cmpsha1(uae_u8 *s1, struct romdata *rd)
{
    int i;

    for (i = 0; i < SHA1_SIZE / 4; i++) {
	uae_u32 v1 = (s1[0] << 24) | (s1[1] << 16) | (s1[2] << 8) | (s1[3] << 0);
	uae_u32 v2 = rd->sha1[i];
	if (v1 != v2)
	    return -1;
	s1 += 4;
    }
    return 0;
}

static struct romdata *checkromdata(uae_u8 *sha1, int size, uae_u32 mask)
{
    int i = 0;
    while (roms[i].name) {
	if (!notcrc32(roms[i].crc32) && roms[i].size >= size) {
	    if (roms[i].type & mask) {
		if (!cmpsha1(sha1, &roms[i]))
		    return &roms[i];
	    }
	}
	i++;
    }
    return NULL;
}

int decode_cloanto_rom_do (uae_u8 *mem, int size, int real_size)
{
    int cnt, t, i;

    for (i = ROM_KEY_NUM - 1; i >= 0; i--) {
	uae_u8 sha1[SHA1_SIZE];
	struct romdata *rd;
	int keysize = keyring[i].size;
	uae_u8 *key = keyring[i].key;
	if (!key)
	    continue;
    for (t = cnt = 0; cnt < size; cnt++, t = (t + 1) % keysize)  {
        mem[cnt] ^= key[t];
        if (real_size == cnt + 1)
	    t = keysize - 1;
    }
	if ((mem[2] == 0x4e && mem[3] == 0xf9) || (mem[0] == 0x11 && (mem[1] == 0x11 || mem[1] == 0x14))) {
	    cloanto_rom = 1;
	    return 1;
	}
	get_sha1 (mem, size, sha1);
	rd = checkromdata (sha1, size, -1);
	if (rd) {
	    if (rd->cloanto)
		cloanto_rom = 1;
	    return 1;
	}
	if (i == 0)
	    break;
	for (t = cnt = 0; cnt < size; cnt++, t = (t + 1) % keysize)  {
	    mem[cnt] ^= key[t];
	    if (real_size == cnt + 1)
		t = keysize - 1;
	}
    }
    return 0;
}

static int decode_rekick_rom_do (uae_u8 *mem, int size, int real_size)
{
    uae_u32 d1 = 0xdeadfeed, d0;
    int i;

    for (i = 0; i < size / 8; i++) {
	d0 = ((mem[i * 8 + 0] << 24) | (mem[i * 8 + 1] << 16) | (mem[i * 8 + 2] << 8) | mem[i * 8 + 3]);
	d1 = d1 ^ d0;
	mem[i * 8 + 0] = d1 >> 24;
	mem[i * 8 + 1] = d1 >> 16;
	mem[i * 8 + 2] = d1 >> 8;
	mem[i * 8 + 3] = d1;
	d1 = ((mem[i * 8 + 4] << 24) | (mem[i * 8 + 5] << 16) | (mem[i * 8 + 6] << 8) | mem[i * 8 + 7]);
	d0 = d0 ^ d1;
	mem[i * 8 + 4] = d0 >> 24;
	mem[i * 8 + 5] = d0 >> 16;
	mem[i * 8 + 6] = d0 >> 8;
	mem[i * 8 + 7] = d0;
    }
    return 1;
}

static int decode_rom (uae_u8 *mem, int size, int mode, int real_size)
{
  if (mode == 1) {
	  if (!decode_cloanto_rom_do (mem, size, real_size)) {
#ifndef SINGLEFILE
    	notify_user (NUMSG_NOROMKEY);
#endif
    	return 0;
    }
    return 1;
  } else if (mode == 2) {
  	decode_rekick_rom_do (mem, size, real_size);
  	return 1;
  }
  return 0;
}

struct romdata *getromdatabydata (uae_u8 *rom, int size)
{
    uae_u8 sha1[SHA1_SIZE];
    uae_u8 tmp[4];
    uae_u8 *tmpbuf = NULL;
    struct romdata *ret = NULL;

    if (size > 11 && !memcmp (rom, "AMIROMTYPE1", 11)) {
	uae_u8 *tmpbuf = (uae_u8*)xmalloc (size);
	int tmpsize = size - 11;
	memcpy (tmpbuf, rom + 11, tmpsize);
	decode_rom (tmpbuf, tmpsize, 1, tmpsize);
	rom = tmpbuf;
	size = tmpsize;
    }
    get_sha1 (rom, size, sha1);
    ret = checkromdata(sha1, size, -1);
    if (!ret) {
	get_sha1 (rom, size / 2, sha1);
	ret = checkromdata (sha1, size / 2, -1);
	if (!ret) {
	    /* ignore AR IO-port range until we have full dump */
	    memcpy (tmp, rom, 4);
	    memset (rom, 0, 4);
	    get_sha1 (rom, size, sha1);
	    ret = checkromdata (sha1, size, ROMTYPE_AR);
	    memcpy (rom, tmp, 4);
	}
    }
    xfree (tmpbuf);
    return ret;
}

struct romdata *getromdatabyzfile (struct zfile *f)
{
    int pos, size;
    uae_u8 *p;
    struct romdata *rd;

    pos = zfile_ftell (f);
    zfile_fseek (f, 0, SEEK_END);
    size = zfile_ftell (f);
    p = (uae_u8 *) xmalloc (size);
    if (!p)
	return 0;
    memset (p, 0, size);
    zfile_fseek (f, 0, SEEK_SET);
    zfile_fread (p, 1, size, f);
    zfile_fseek (f, pos, SEEK_SET);        
    rd = getromdatabydata (p, size);
    xfree (p);
    return rd;
}

void getromname (struct romdata *rd, char *name)
{
    name[0] = 0;
    if (!rd)
	    return;
    while (rd->group)
	rd--;
    strcat (name, rd->name);
    if ((rd->subrev || rd->subver) && rd->subver != rd->ver)
	    sprintf (name + strlen (name), " rev %d.%d", rd->subver, rd->subrev);
    if (rd->size > 0)
      sprintf (name + strlen (name), " (%dk)", (rd->size + 1023) / 1024);
    if (rd->partnumber && strlen (rd->partnumber) > 0)
	sprintf (name + strlen (name), " [%s]", rd->partnumber);
}

struct romlist *getromlistbyromdata(struct romdata *rd)
{
    int ids[2];
    
    ids[0] = rd->id;
    ids[1] = 0;
    return getromlistbyids(ids);
}

struct romlist *getromlistbyids(int *ids)
{
    struct romdata *rd;
    int i, j;

    i = 0;
    while (ids[i] >= 0) {
	rd = getromdatabyid (ids[i]);
	if (rd) {
	    for (j = 0; j < romlist_cnt; j++) {
		if (rl[j].rd->id == rd->id)
		    return &rl[j];
	    }
	}
	i++;
    }
    return NULL;
}

void romwarning(int *ids)
{
}

addrbank *mem_banks[MEMORY_BANKS];

/* This has two functions. It either holds a host address that, when added
   to the 68k address, gives the host address corresponding to that 68k
   address (in which case the value in this array is even), OR it holds the
   same value as mem_banks, for those banks that have baseaddr==0. In that
   case, bit 0 is set (the memory access routines will take care of it).  */

uae_u8 *baseaddr[MEMORY_BANKS];

int addr_valid(char *txt, uaecptr addr, uae_u32 len)
{
    addrbank *ab = &get_mem_bank(addr);
    if (ab == 0 || !(ab->flags & ABFLAG_RAM) || addr < 0x100 || len < 0 || len > 16777215 || !valid_address(addr, len)) {
    	write_log("corrupt %s pointer %x (%d) detected!\n", txt, addr, len);
	return 0;
    }
    return 1;
}

uae_u32 chipmem_mask, chipmem_full_mask;
uae_u32 kickmem_mask, extendedkickmem_mask, extendedkickmem2_mask, bogomem_mask;
uae_u32 a3000lmem_mask, a3000hmem_mask, cardmem_mask;

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

static uae_u32 REGPARAM2 dummy_lget (uaecptr addr)
{
#ifdef JIT
  special_mem |= S_READ;
#endif
  if (currprefs.cpu_model >= 68020)
    return NONEXISTINGDATA;
  return (regs.irc << 16) | regs.irc;
}
uae_u32 REGPARAM2 dummy_lgeti (uaecptr addr)
{
#ifdef JIT
    special_mem |= S_READ;
#endif
    if (currprefs.cpu_model >= 68020)
	return NONEXISTINGDATA;
    return (regs.irc << 16) | regs.irc;
}

static uae_u32 REGPARAM2 dummy_wget (uaecptr addr)
{
#ifdef JIT
  special_mem |= S_READ;
#endif
  if (currprefs.cpu_model >= 68020)
    return NONEXISTINGDATA;
  return regs.irc;
}
uae_u32 REGPARAM2 dummy_wgeti (uaecptr addr)
{
#ifdef JIT
    special_mem |= S_READ;
#endif
    if (currprefs.cpu_model >= 68020)
	return NONEXISTINGDATA;
    return regs.irc;
}

static uae_u32 REGPARAM2 dummy_bget (uaecptr addr)
{
#ifdef JIT
  special_mem |= S_READ;
#endif
  if (currprefs.cpu_model >= 68020)
    return NONEXISTINGDATA;
  return (addr & 1) ? regs.irc : regs.irc >> 8;
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

uae_u8 *chipmemory;

static int REGPARAM3 chipmem_check (uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *REGPARAM3 chipmem_xlate (uaecptr addr) REGPARAM;

uae_u32 REGPARAM2 chipmem_lget (uaecptr addr)
{
    uae_u32 *m;
    addr &= chipmem_mask;
    m = (uae_u32 *)(chipmemory + addr);

    return do_get_mem_long(m);
}

static uae_u32 REGPARAM2 chipmem_wget (uaecptr addr)
{
   uae_u16 *m;
   addr &= chipmem_mask;
   m = (uae_u16 *)(chipmemory + addr);

   return do_get_mem_word (m);
}

static uae_u32 REGPARAM2 chipmem_bget (uaecptr addr)
{
	uae_u8 *m;
    addr &= chipmem_mask;
    m = (uae_u8 *)(chipmemory + addr);
    return do_get_mem_byte(m);
}

void REGPARAM2 chipmem_lput (uaecptr addr, uae_u32 l)
{
    uae_u32 *m;
    addr &= chipmem_mask;
    m = (uae_u32 *)(chipmemory + addr);
    do_put_mem_long(m, l);
}

void REGPARAM2 chipmem_wput (uaecptr addr, uae_u32 w)
{
   uae_u16 *m;
   addr &= chipmem_mask;
   m = (uae_u16 *)(chipmemory + addr);
   do_put_mem_word (m, w);
}

void REGPARAM2 chipmem_bput (uaecptr addr, uae_u32 b)
{
	uae_u8 *m;
    addr &= chipmem_mask;
    m = (uae_u8 *)(chipmemory + addr);
    do_put_mem_byte(m, b);
}

static uae_u32 REGPARAM2 chipmem_agnus_lget (uaecptr addr)
{
    uae_u32 *m;

    addr &= chipmem_full_mask;
    m = (uae_u32 *)(chipmemory + addr);
    return do_get_mem_long (m);
}

uae_u32 REGPARAM2 chipmem_agnus_wget (uaecptr addr)
{
   uae_u16 *m;

   addr &= chipmem_full_mask;
   m = (uae_u16 *)(chipmemory + addr);
   return do_get_mem_word (m);
}

static uae_u32 REGPARAM2 chipmem_agnus_bget (uaecptr addr)
{
	uae_u8 *m;
    addr &= chipmem_full_mask;
    m = (uae_u8 *)(chipmemory + addr);
    return do_get_mem_byte(m);
}

static void REGPARAM2 chipmem_agnus_lput (uaecptr addr, uae_u32 l)
{
    uae_u32 *m;

    addr &= chipmem_full_mask;
    if (addr >= allocated_chipmem)
	return;
    m = (uae_u32 *)(chipmemory + addr);
    do_put_mem_long (m, l);
}

void REGPARAM2 chipmem_agnus_wput (uaecptr addr, uae_u32 w)
{
   uae_u16 *m;

   addr &= chipmem_full_mask;
    if (addr >= allocated_chipmem)
	return;
   m = (uae_u16 *)(chipmemory + addr);
   do_put_mem_word (m, w);
}

static void REGPARAM2 chipmem_agnus_bput (uaecptr addr, uae_u32 b)
{
	uae_u8 *m;
    addr &= chipmem_full_mask;
    if (addr >= allocated_chipmem)
	return;
    m = (uae_u8 *)(chipmemory + addr);
    do_put_mem_byte(m, b);
}

static int REGPARAM2 chipmem_check (uaecptr addr, uae_u32 size)
{
    addr &= chipmem_mask;
    return (addr + size) <= allocated_chipmem;
}

static uae_u8 *REGPARAM2 chipmem_xlate (uaecptr addr)
{
	addr &= chipmem_mask;
  return chipmemory + addr;
}

/* Slow memory */

static uae_u8 *bogomemory;

static uae_u32 REGPARAM3 bogomem_lget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 bogomem_wget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 bogomem_bget (uaecptr) REGPARAM;
static void REGPARAM3 bogomem_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 bogomem_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 bogomem_bput (uaecptr, uae_u32) REGPARAM;
static int REGPARAM3 bogomem_check (uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *REGPARAM3 bogomem_xlate (uaecptr addr) REGPARAM;

static uae_u32 REGPARAM2 bogomem_lget (uaecptr addr)
{
    uae_u32 *m;
    addr &= bogomem_mask;
    m = (uae_u32 *)(bogomemory + addr);

    return do_get_mem_long (m);
}

static uae_u32 REGPARAM2 bogomem_wget (uaecptr addr)
{
    uae_u16 *m;
    addr &= bogomem_mask;
    m = (uae_u16 *)(bogomemory + addr);

    return do_get_mem_word (m);
}

static uae_u32 REGPARAM2 bogomem_bget (uaecptr addr)
{
    uae_u8 *m;
    addr &= bogomem_mask;
    m = (uae_u8 *)(bogomemory + addr);

    return do_get_mem_byte(m);
}

static void REGPARAM2 bogomem_lput (uaecptr addr, uae_u32 l)
{
    uae_u32 *m;
    addr &= bogomem_mask;
    m = (uae_u32 *)(bogomemory + addr);
    do_put_mem_long (m, l);
}

static void REGPARAM2 bogomem_wput (uaecptr addr, uae_u32 w)
{
    uae_u16 *m;
    addr &= bogomem_mask;
    m = (uae_u16 *)(bogomemory + addr);
    do_put_mem_word (m, w);
}

static void REGPARAM2 bogomem_bput (uaecptr addr, uae_u32 b)
{
    uae_u8 *m;
    addr &= bogomem_mask;
    m = (uae_u8 *)(bogomemory + addr);
    do_put_mem_byte(m, b);
}

static int REGPARAM2 bogomem_check (uaecptr addr, uae_u32 size)
{
    addr &= bogomem_mask;
    return (addr + size) <= allocated_bogomem;
}

static uae_u8 *REGPARAM2 bogomem_xlate (uaecptr addr)
{
    addr &= bogomem_mask;
    return bogomemory + addr;
}

/* Kick memory */

uae_u8 *kickmemory;
uae_u16 kickstart_version;
static int kickmem_size;

/*
 * A1000 kickstart RAM handling
 *
 * RESET instruction unhides boot ROM and disables write protection
 * write access to boot ROM hides boot ROM and enables write protection
 *
 */
static int a1000_kickstart_mode;
static uae_u8 *a1000_bootrom;
static void a1000_handle_kickstart (int mode)
{
    if (!a1000_bootrom)
	return;
    if (mode == 0) {
	      a1000_kickstart_mode = 0;
	      memcpy (kickmemory, kickmemory + 262144, 262144);
        kickstart_version = (kickmemory[262144 + 12] << 8) | kickmemory[262144 + 13];
    } else {
	      a1000_kickstart_mode = 1;
	      memcpy (kickmemory, a1000_bootrom, 262144);
        kickstart_version = 0;
    }
}

void a1000_reset (void)
{
	a1000_handle_kickstart (1);
}

static uae_u32 REGPARAM3 kickmem_lget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 kickmem_wget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 kickmem_bget (uaecptr) REGPARAM;
static void REGPARAM3 kickmem_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 kickmem_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 kickmem_bput (uaecptr, uae_u32) REGPARAM;
static int REGPARAM3 kickmem_check (uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *REGPARAM3 kickmem_xlate (uaecptr addr) REGPARAM;

static uae_u32 REGPARAM2 kickmem_lget (uaecptr addr)
{
   uae_u32 *m;
   addr &= kickmem_mask;
   m = (uae_u32 *)(kickmemory + addr);
   return do_get_mem_long(m);
}

static uae_u32 REGPARAM2 kickmem_wget (uaecptr addr)
{
   uae_u16 *m;
   addr &= kickmem_mask;
   m = (uae_u16 *)(kickmemory + addr);

   return do_get_mem_word (m);
}

static uae_u32 REGPARAM2 kickmem_bget (uaecptr addr)
{
    uae_u8 *m;
    addr &= kickmem_mask;
    m = (uae_u8 *)(kickmemory + addr);

    return do_get_mem_byte(m);
}

static void REGPARAM2 kickmem_lput (uaecptr addr, uae_u32 l)
{
   uae_u32 *m;
#ifdef JIT
    special_mem |= S_WRITE;
#endif
   if (a1000_kickstart_mode) {
      if (addr >= 0xfc0000) {
         addr &= kickmem_mask;
         m = (uae_u32 *)(kickmemory + addr);
         do_put_mem_long(m, l);
         return;
      } else
         a1000_handle_kickstart (0);
   }
}

static void REGPARAM2 kickmem_wput (uaecptr addr, uae_u32 w)
{
   uae_u16 *m;
#ifdef JIT
    special_mem |= S_WRITE;
#endif
   if (a1000_kickstart_mode) {
      if (addr >= 0xfc0000) {
         addr &= kickmem_mask;
         m = (uae_u16 *)(kickmemory + addr);
         do_put_mem_word (m, w);
         return;
      } else
         a1000_handle_kickstart (0);
   }
}

static void REGPARAM2 kickmem_bput (uaecptr addr, uae_u32 b)
{
   uae_u8 *m;
#ifdef JIT
    special_mem |= S_WRITE;
#endif
   if (a1000_kickstart_mode) {
      if (addr >= 0xfc0000) {
         addr &= kickmem_mask;
         m = (uae_u8 *)(kickmemory + addr);
         do_put_mem_byte (m, b);
         return;
      } else
         a1000_handle_kickstart (0);
   }
}

static void REGPARAM2 kickmem2_lput (uaecptr addr, uae_u32 l)
{
    uae_u32 *m;
#ifdef JIT
    special_mem |= S_WRITE;
#endif
    addr &= kickmem_mask;
    m = (uae_u32 *)(kickmemory + addr);
    do_put_mem_long (m, l);
}

static void REGPARAM2 kickmem2_wput (uaecptr addr, uae_u32 w)
{
    uae_u16 *m;
#ifdef JIT
    special_mem |= S_WRITE;
#endif
    addr &= kickmem_mask;
    m = (uae_u16 *)(kickmemory + addr);
    do_put_mem_word (m, w);
}

static void REGPARAM2 kickmem2_bput (uaecptr addr, uae_u32 b)
{
    uae_u8 *m;
#ifdef JIT
    special_mem |= S_WRITE;
#endif
    addr &= kickmem_mask;
    m = (uae_u8 *)(kickmemory + addr);
    do_put_mem_byte (m, b);
}

static int REGPARAM2 kickmem_check (uaecptr addr, uae_u32 size)
{
    addr &= kickmem_mask;
    return (addr + size) <= kickmem_size;
}

static uae_u8 *REGPARAM2 kickmem_xlate (uaecptr addr)
{
    addr &= kickmem_mask;
    return kickmemory + addr;
}

/* CD32/CDTV extended kick memory */

uae_u8 *extendedkickmemory, *extendedkickmemory2;
static int extendedkickmem_size, extendedkickmem2_size;
static uae_u32 extendedkickmem_start, extendedkickmem2_start;
static int extendedkickmem_type;

#define EXTENDED_ROM_CD32 1
#define EXTENDED_ROM_CDTV 2
#define EXTENDED_ROM_KS 3
#define EXTENDED_ROM_ARCADIA 4

static uae_u32 REGPARAM3 extendedkickmem_lget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 extendedkickmem_wget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 extendedkickmem_bget (uaecptr) REGPARAM;
static void REGPARAM3 extendedkickmem_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 extendedkickmem_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 extendedkickmem_bput (uaecptr, uae_u32) REGPARAM;
static int REGPARAM3 extendedkickmem_check (uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *REGPARAM3 extendedkickmem_xlate (uaecptr addr) REGPARAM;

static uae_u32 REGPARAM2 extendedkickmem_lget (uaecptr addr)
{
    uae_u32 *m;
    addr -= extendedkickmem_start;
    addr &= extendedkickmem_mask;
    m = (uae_u32 *)(extendedkickmemory + addr);
    return do_get_mem_long (m);
}

static uae_u32 REGPARAM2 extendedkickmem_wget (uaecptr addr)
{
    uae_u16 *m;
    addr -= extendedkickmem_start;
    addr &= extendedkickmem_mask;
    m = (uae_u16 *)(extendedkickmemory + addr);
    return do_get_mem_word (m);
}

static uae_u32 REGPARAM2 extendedkickmem_bget (uaecptr addr)
{
    addr -= extendedkickmem_start;
    addr &= extendedkickmem_mask;
    return extendedkickmemory[addr];
}

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

static int REGPARAM2 extendedkickmem_check (uaecptr addr, uae_u32 size)
{
    addr -= extendedkickmem_start;
    addr &= extendedkickmem_mask;
    return (addr + size) <= extendedkickmem_size;
}

static uae_u8 *REGPARAM2 extendedkickmem_xlate (uaecptr addr)
{
    addr -= extendedkickmem_start;
    addr &= extendedkickmem_mask;
    return extendedkickmemory + addr;
}

static uae_u32 REGPARAM3 extendedkickmem2_lget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 extendedkickmem2_wget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 extendedkickmem2_bget (uaecptr) REGPARAM;
static void REGPARAM3 extendedkickmem2_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 extendedkickmem2_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 extendedkickmem2_bput (uaecptr, uae_u32) REGPARAM;
static int REGPARAM3 extendedkickmem2_check (uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *REGPARAM3 extendedkickmem2_xlate (uaecptr addr) REGPARAM;
static uae_u32 REGPARAM2 extendedkickmem2_lget (uaecptr addr)
{
    uae_u32 *m;
    addr -= extendedkickmem2_start;
    addr &= extendedkickmem2_mask;
    m = (uae_u32 *)(extendedkickmemory2 + addr);
    return do_get_mem_long (m);
}
static uae_u32 REGPARAM2 extendedkickmem2_wget (uaecptr addr)
{
    uae_u16 *m;
    addr -= extendedkickmem2_start;
    addr &= extendedkickmem2_mask;
    m = (uae_u16 *)(extendedkickmemory2 + addr);
    return do_get_mem_word (m);
}
static uae_u32 REGPARAM2 extendedkickmem2_bget (uaecptr addr)
{
    addr -= extendedkickmem2_start;
    addr &= extendedkickmem2_mask;
    return extendedkickmemory2[addr];
}
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
static int REGPARAM2 extendedkickmem2_check (uaecptr addr, uae_u32 size)
{
    addr -= extendedkickmem2_start;
    addr &= extendedkickmem2_mask;
    return (addr + size) <= extendedkickmem2_size;
}
static uae_u8 *REGPARAM2 extendedkickmem2_xlate (uaecptr addr)
{
    addr -= extendedkickmem2_start;
    addr &= extendedkickmem2_mask;
    return extendedkickmemory2 + addr;
}


/* Default memory access functions */

int REGPARAM2 default_check (uaecptr a, uae_u32 b)
{
    return 0;
}

uae_u8 *REGPARAM2 default_xlate (uaecptr a)
{
    if (quit_program == 0) {
	    /* do this only in 68010+ mode, there are some tricky A500 programs.. */
      if(currprefs.cpu_model > 68000 || !currprefs.cpu_compatible) {
        write_log ("Your Amiga program just did something terribly stupid\n");
        uae_reset (0);
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
    chipmem_xlate, chipmem_check, NULL, "Chip memory",
    chipmem_lget, chipmem_wget, ABFLAG_RAM
};

addrbank bogomem_bank = {
    bogomem_lget, bogomem_wget, bogomem_bget,
    bogomem_lput, bogomem_wput, bogomem_bput,
    bogomem_xlate, bogomem_check, NULL, "Slow memory",
    bogomem_lget, bogomem_wget, ABFLAG_RAM

};

addrbank kickmem_bank = {
    kickmem_lget, kickmem_wget, kickmem_bget,
    kickmem_lput, kickmem_wput, kickmem_bput,
    kickmem_xlate, kickmem_check, NULL, "Kickstart ROM",
    kickmem_lget, kickmem_wget, ABFLAG_ROM
};

addrbank kickram_bank = {
    kickmem_lget, kickmem_wget, kickmem_bget,
    kickmem2_lput, kickmem2_wput, kickmem2_bput,
    kickmem_xlate, kickmem_check, NULL, "Kickstart Shadow RAM",
    kickmem_lget, kickmem_wget, ABFLAG_UNK | ABFLAG_SAFE
};

addrbank extendedkickmem_bank = {
    extendedkickmem_lget, extendedkickmem_wget, extendedkickmem_bget,
    extendedkickmem_lput, extendedkickmem_wput, extendedkickmem_bput,
    extendedkickmem_xlate, extendedkickmem_check, NULL, "Extended Kickstart ROM",
    extendedkickmem_lget, extendedkickmem_wget, ABFLAG_ROM
};
addrbank extendedkickmem2_bank = {
    extendedkickmem2_lget, extendedkickmem2_wget, extendedkickmem2_bget,
    extendedkickmem2_lput, extendedkickmem2_wput, extendedkickmem2_bput,
    extendedkickmem2_xlate, extendedkickmem2_check, NULL, "Extended 2nd Kickstart ROM",
    extendedkickmem2_lget, extendedkickmem2_wget, ABFLAG_ROM
};


static int kickstart_checksum (uae_u8 *mem, int size)
{
    if (!kickstart_checksum_do (mem, size)) {
#ifndef	SINGLEFILE
	    notify_user (NUMSG_KSROMCRCERROR);
#endif
      return 0;
    }
    return 1;
}

static int read_rom_file (uae_u8 *buf, struct romdata *rd)
{
    struct zfile *zf;
    struct romlist *rl = romlist_getrl (rd);
    char tmp[11];

    if (!rl || strlen (rl->path) == 0)
	return 0;
    zf = zfile_fopen (rl->path, "rb");
    if (!zf)
	return 0;
    addkeydir (rl->path);
    zfile_fread (tmp, sizeof tmp, 1, zf);
    if (!memcmp (tmp, "AMIROMTYPE1", sizeof tmp)) {
	zfile_fread (buf, rd->size, 1, zf);
        decode_cloanto_rom_do (buf, rd->size, rd->size);
    } else {
	memcpy (buf, tmp, sizeof tmp);
	zfile_fread (buf + sizeof tmp, rd->size - sizeof (tmp), 1, zf);
    }
    zfile_fclose (zf);
    return 1;
}

static void byteswap (uae_u8 *buf, int size)
{
    int i;
    for (i = 0; i < size; i += 2) {
	uae_u8 t = buf[i];
	buf[i] = buf[i + 1];
	buf[i + 1] = t;
    }
}
static void mergecd32 (uae_u8 *dst, uae_u8 *src, int size)
{
    int i, k;
    k = 0;
    for (i = 0; i < size / 2; i += 2) {
	int j = i + size / 2;
	dst[k + 1] = src[i + 0];
	dst[k + 0] = src[i + 1];
	dst[k + 3] = src[j + 0];
	dst[k + 2] = src[j + 1];
	k += 4;
    }
#if 0
    {
	struct zfile *f;
	f = zfile_fopen ("c:\\d\\1.rom","wb");
	zfile_fwrite (dst, 1, size, f);
	zfile_fclose(f);
    }
#endif
}

static void descramble (struct romdata *rd, uae_u8 *data, int size, int odd)
{
}

struct zfile *read_rom (struct romdata **prd)
{
    struct romdata *rd2 = *prd;
    struct romdata *rd = *prd;
    const char *name;
    int id = rd->id;
    uae_u32 crc32;
    int size;
    uae_u8 *buf, *buf2;

    /* find parent node */
    for (;;) {
	if (rd2 == &roms[0])
	    break;
	if (rd2[-1].id != id)
	    break;
	rd2--;
    }
    *prd = rd2;
    size = rd2->size;
    crc32 = rd2->crc32;
    name = rd->name;
    buf = (uae_u8 *)xmalloc (size * 2);
    if (!buf)
	return NULL;
    buf2 = buf + size;
    while (rd->id == id) {
	int i;
	int ok = 0;
	uae_u32 flags = rd->type;
        int odd = (flags & ROMTYPE_ODD) ? 1 : 0;

	for (i = 0; i < 2; i++) {
	    memset (buf, 0, size);
	    if (!(flags & (ROMTYPE_EVEN | ROMTYPE_ODD))) {
		read_rom_file (buf, rd);
		if (flags & ROMTYPE_CD32) {
		    memcpy (buf2, buf, size);
		    mergecd32 (buf, buf2, size);
		}
		rd++;
		i++;
	    } else {
		int romsize = size / 2;
		if (i)
		    odd = !odd;
		read_rom_file (buf2, rd);
		if (flags & ROMTYPE_BYTESWAP)
		    byteswap (buf2, romsize);
		if (flags & ROMTYPE_SCRAMBLED)
		    descramble (rd, buf2, romsize, odd);
		for (i = 0; i < size; i += 2)
		    buf[i + odd] = buf2[i / 2];
		read_rom_file (buf2, rd + 1);
		if (flags & ROMTYPE_BYTESWAP)
		    byteswap (buf2, romsize);
		if (flags & ROMTYPE_SCRAMBLED)
		    descramble (rd + 1, buf2, romsize, !odd);
		for (i = 0; i < size; i += 2)
		    buf[i + (1 - odd)] = buf2[i / 2];
		rd += 2;
	    }
	    if (get_crc32 (buf, size) == crc32) {
		ok = 1;
	    } else {
		/* perhaps it is byteswapped without byteswap entry? */
		byteswap (buf, size);
		if (get_crc32 (buf, size) == crc32)
		    ok = 1;
	    }
	    if (ok) {
    		struct zfile *zf = zfile_fopen_empty (name, size);
		if (zf) {
		    zfile_fwrite (buf, size, 1, zf);
		    zfile_fseek (zf, 0, SEEK_SET);
		}
		xfree (buf);
		return zf;
	    }
	}

    }
    xfree (buf);
    return NULL;
}

struct zfile *read_rom_name (const char *filename)
{
    int i;
    struct zfile *f;

    for (i = 0; i < romlist_cnt; i++) {
	if (!strcmpi (filename, rl[i].path)) {
	    struct romdata *rd = rl[i].rd;
	    f = read_rom (&rd);
	    if (f)
		return f;
	}
    }
    f = zfile_fopen (filename, "rb");
    if (f) {
	uae_u8 tmp[11];
	zfile_fread (tmp, sizeof tmp, 1, f);
	if (!memcmp (tmp, "AMIROMTYPE1", sizeof tmp)) {
	    struct zfile *df;
	    int size;
	    uae_u8 *buf;
	    addkeydir (filename);
	    zfile_fseek (f, 0, SEEK_END);
	    size = zfile_ftell (f) - sizeof tmp;
	    zfile_fseek (f, sizeof tmp, SEEK_SET);
	    buf = (uae_u8 *)xmalloc (size);
	    zfile_fread (buf, size, 1, f);
	    df = zfile_fopen_empty ("tmp.rom", size);
	    decode_cloanto_rom_do (buf, size, size);
	    zfile_fwrite (buf, size, 1, df);
	    zfile_fclose (f);
	    xfree (buf);
	    zfile_fseek (df, 0, SEEK_SET);
	    f = df;
	} else {
	    zfile_fseek (f, -((int)sizeof tmp), SEEK_CUR);
	}
    }
    return f;
}

static char *kickstring = "exec.library";
static int read_kickstart (struct zfile *f, uae_u8 *mem, int size, int dochecksum, int noalias)
{
    unsigned char buffer[20];
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
    } else if (strncmp ((char *) buffer, "AMIROMTYPE1", 11) != 0) {
	    zfile_fseek (f, oldpos, SEEK_SET);
    } else {
	cloanto_rom = 1;
	cr = 1;
    }

    i = zfile_fread (mem, 1, size, f);
    if (kickdisk && i > 262144)
	    i = 262144;
    if (i != 8192 && i != 65536 && i != 131072 && i != 262144 && i != 524288 && i != 524288 * 2 && i != 524288 * 4) {
	    notify_user (NUMSG_KSROMREADERROR);
	    return 0;
    }
    if (!noalias && i == size / 2)
	    memcpy (mem + size / 2, mem, size / 2);

    if (cr) {
	    if(!decode_rom (mem, size, cr, i))
	      return 0;
    }
    if (currprefs.cs_a1000ram) {
    	int off = 0;
	    a1000_bootrom = (uae_u8*)xcalloc (262144, 1);
	    while (off + i < 262144) {
	      memcpy (a1000_bootrom + off, kickmemory, i);
	      off += i;
	    }
      memset (kickmemory, 0, kickmem_size);
	    a1000_handle_kickstart (1);
    	dochecksum = 0;
    	i = 524288;
    }
    for (j = 0; j < 256 && i >= 262144; j++) {
	if (!memcmp (mem + j, kickstring, strlen (kickstring) + 1))
	    break;
    }
    if (j == 256 || i < 262144)
	dochecksum = 0;

    if (dochecksum)
	kickstart_checksum (mem, size);
    return i;
}

static int load_extendedkickstart (void)
{
  struct zfile *f;
  int size, off;

  if (strlen (currprefs.romextfile) == 0)
	  return 0;
  f = read_rom_name (currprefs.romextfile);
  if (!f) {
	  notify_user (NUMSG_NOEXTROM);
	  return 0;
  }

  zfile_fseek (f, 0, SEEK_END);
  size = zfile_ftell (f);
	extendedkickmem_size = 524288;
  off = 0;
    if (size > 300000) {
	extendedkickmem_type = EXTENDED_ROM_CD32;
    } else {
	extendedkickmem_type = EXTENDED_ROM_CDTV;
    }
  zfile_fseek (f, off, SEEK_SET);
  switch (extendedkickmem_type) {

    case EXTENDED_ROM_CDTV:
      extendedkickmem_start = 0xf00000;
      extendedkickmemory = (uae_u8 *) mapped_malloc (extendedkickmem_size, "rom_f0");
	    extendedkickmem_bank.baseaddr = (uae_u8 *) extendedkickmemory;
	    break;
    case EXTENDED_ROM_CD32:
      extendedkickmem_start = 0xe00000;
	    extendedkickmemory = (uae_u8 *) mapped_malloc (extendedkickmem_size, "rom_e0");
	    extendedkickmem_bank.baseaddr = (uae_u8 *) extendedkickmemory;
	    break;
  }
  
  read_kickstart (f, extendedkickmemory, extendedkickmem_size, 0, 1);
  extendedkickmem_mask = extendedkickmem_size - 1;
  zfile_fclose (f);
  return 1;
}

static void kickstart_fix_checksum (uae_u8 *mem, int size)
{
    uae_u32 cksum = 0, prevck = 0;
    int i, ch = size == 524288 ? 0x7ffe8 : 0x3e;
    
    mem[ch] = 0;
    mem[ch + 1] = 0;
    mem[ch + 2] = 0;
    mem[ch + 3] = 0;
    for (i = 0; i < size; i+=4) {
	uae_u32 data = (mem[i] << 24) | (mem[i + 1] << 16) | (mem[i + 2] << 8) | mem[i + 3];
	cksum += data;
	if (cksum < prevck)
	    cksum++;
	prevck = cksum;
    }
    cksum ^= 0xffffffff;
    mem[ch++] = cksum >> 24;
    mem[ch++] = cksum >> 16;
    mem[ch++] = cksum >> 8;
    mem[ch++] = cksum >> 0;
}

/* disable incompatible drivers */
static int patch_residents (uae_u8 *kickmemory, int size)
{
    int i, j, patched = 0;
    const char *residents[] = { "NCR scsi.device", "scsi.device", "carddisk.device", "card.resource", 0 };
    // "scsi.device", "carddisk.device", "card.resource" };
    uaecptr base = size == 524288 ? 0xf80000 : 0xfc0000;

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
			write_log ("KSPatcher: '%s' at %08X disabled\n", residents[j], i + base);
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
    patched += patch_residents (kickmemory, kickmem_size);
    if (extendedkickmemory) {
	    patched += patch_residents (extendedkickmemory, extendedkickmem_size);
	    if (patched)
	      kickstart_fix_checksum (extendedkickmemory, extendedkickmem_size);
    }
    if (patched)
    	kickstart_fix_checksum (kickmemory, kickmem_size);
}

static int load_kickstart (void)
{
    struct zfile *f;
    char tmprom[MAX_DPATH], tmprom2[MAX_DPATH];
    int patched = 0;

    cloanto_rom = 0;
    f = read_rom_name (currprefs.romfile);
    strcpy (tmprom, currprefs.romfile);
    if (f == NULL) {
    	sprintf (tmprom2, "%s%s", start_path_data, currprefs.romfile);
    	f = zfile_fopen (tmprom2, "rb");
    	if (f == NULL) {
  	    sprintf (currprefs.romfile, "%sroms/kick.rom", start_path_data);
      	f = zfile_fopen (currprefs.romfile, "rb");
      	if (f == NULL) {
        		sprintf (currprefs.romfile, "%skick.rom", start_path_data);
      	    f = zfile_fopen( currprefs.romfile, "rb" );
        }
    	} else {
    	    strcpy (currprefs.romfile, tmprom2);
      }
    }
    addkeydir (currprefs.romfile);
    if( f == NULL ) { /* still no luck */
#if defined(AMIGA)||defined(__POS__)
#define USE_UAE_ERSATZ "USE_UAE_ERSATZ"
	if( !getenv(USE_UAE_ERSATZ)) 
        {
	    write_log ("Using current ROM. (create ENV:%s to "
		"use uae's ROM replacement)\n",USE_UAE_ERSATZ);
	    memcpy(kickmemory,(char*)0x1000000-kickmem_size,kickmem_size);
	    kickstart_checksum (kickmemory, kickmem_size);
	    goto chk_sum;
	}
#else
	goto err;
#endif
    }

      if (f != NULL) {
	  int filesize, size, maxsize;
  	int kspos = 524288;
  	int extpos = 0;

	  maxsize = 524288;
	  zfile_fseek (f, 0, SEEK_END);
	  filesize = zfile_ftell (f);
	  zfile_fseek (f, 0, SEEK_SET);
	  if (filesize == 1760 * 512) {
	      filesize = 262144;
	      maxsize = 262144;
	  }
	  if (filesize == 524288 + 8) {
	    /* GVP 0xf0 kickstart */
	    zfile_fseek (f, 8, SEEK_SET);
	  }
	  if (filesize >= 524288 * 2) {
	      struct romdata *rd = getromdatabyzfile(f);
	      zfile_fseek (f, kspos, SEEK_SET);
	  }
	  if (filesize >= 524288 * 4) {
	    kspos = 524288 * 3;
	    extpos = 0;
	    zfile_fseek (f, kspos, SEEK_SET);
	  }
	  size = read_kickstart (f, kickmemory, maxsize, 1, 0);
    if (size == 0)
    	goto err;
      kickmem_mask = size - 1;
	kickmem_size = size;
	if (filesize >= 524288 * 2 && !extendedkickmem_type) {
	    extendedkickmem_size = 0x80000;
	    extendedkickmem_type = EXTENDED_ROM_KS;
	    extendedkickmemory = (uae_u8 *) mapped_malloc (extendedkickmem_size, "rom_e0");
	    extendedkickmem_bank.baseaddr = (uae_u8 *) extendedkickmemory;
	    zfile_fseek (f, extpos, SEEK_SET);
	    read_kickstart (f, extendedkickmemory, extendedkickmem_size, 0, 1);
	    extendedkickmem_mask = extendedkickmem_size - 1;
	}
	if (filesize > 524288 * 2) {
	    extendedkickmem2_size = 524288 * 2;
	    extendedkickmemory2 = mapped_malloc (extendedkickmem2_size, "rom_a8");
	    extendedkickmem2_bank.baseaddr = extendedkickmemory2;
	    zfile_fseek (f, extpos + 524288, SEEK_SET);
	    read_kickstart (f, extendedkickmemory2, 524288, 0, 1);
	    zfile_fseek (f, extpos + 524288 * 2, SEEK_SET);
	    read_kickstart (f, extendedkickmemory2 + 524288, 524288, 0, 1);
	    extendedkickmem2_mask = extendedkickmem2_size - 1;
	}
    }

    kickstart_version = (kickmemory[12] << 8) | kickmemory[13];
    zfile_fclose (f);
    return 1;

err:
    strcpy (currprefs.romfile, tmprom);
    zfile_fclose (f);
    return 0;
}



static void init_mem_banks (void)
{
  int i;
  for (i = 0; i < MEMORY_BANKS; i++)
	  put_mem_bank (i << 16, &dummy_bank, 0);
}

static void allocate_memory (void)
{
  if (allocated_chipmem != currprefs.chipmem_size) {
    int memsize;
   	if (chipmemory)
 	    mapped_free (chipmemory);
		chipmemory = 0;
	  if (currprefs.chipmem_size > 2 * 1024 * 1024)
	    free_fastmemory ();
		
  	memsize = allocated_chipmem = currprefs.chipmem_size;
  	chipmem_full_mask = chipmem_mask = allocated_chipmem - 1;
  	if (memsize < 0x100000)
  	    memsize = 0x100000;
	  if (memsize > 0x100000 && memsize < 0x200000)
	      memsize = 0x200000;
    chipmemory = mapped_malloc (memsize, "chip");
		
		if (chipmemory == 0) {
			write_log ("Fatal error: out of memory for chipmem.\n");
			allocated_chipmem = 0;
    } else {
	    need_hardreset = 1;
	    if (memsize != allocated_chipmem)
		    memset (chipmemory + allocated_chipmem, 0xff, memsize - allocated_chipmem);
    }
	
    currprefs.chipset_mask = changed_prefs.chipset_mask;
    chipmem_full_mask = allocated_chipmem - 1;
	  if (currprefs.chipset_mask & CSMASK_ECS_AGNUS) {
	    if (allocated_chipmem < 0x100000)
        chipmem_full_mask = 0x100000 - 1;
	    if (allocated_chipmem > 0x100000 && allocated_chipmem < 0x200000)
		  chipmem_full_mask = chipmem_mask = 0x200000 - 1;
	  }
  }

  if (allocated_bogomem != currprefs.bogomem_size) {
	  if (bogomemory)
	    mapped_free (bogomemory);
		bogomemory = 0;
		
		if(currprefs.bogomem_size > 0x1c0000)
      currprefs.bogomem_size = 0x1c0000;
    if (currprefs.bogomem_size > 0x180000 && ((changed_prefs.chipset_mask & CSMASK_AGA) || (currprefs.cpu_model >= 68020)))
      currprefs.bogomem_size = 0x180000;
      
	  allocated_bogomem = currprefs.bogomem_size;
		bogomem_mask = allocated_bogomem - 1;

		if (allocated_bogomem) {
	    bogomemory = mapped_malloc (allocated_bogomem, "bogo");
	    if (bogomemory == 0) {
		write_log ("Out of memory for bogomem.\n");
		allocated_bogomem = 0;
	    }
		}
	  need_hardreset = 1;
	}

    if (savestate_state == STATE_RESTORE) {
	    if (bootrom_filepos) {
    	  restore_ram (bootrom_filepos, rtarea);
	    }
	    restore_ram (chip_filepos, chipmemory);
	    if (allocated_bogomem > 0)
    	    restore_ram (bogo_filepos, bogomemory);
    }
	chipmem_bank.baseaddr = chipmemory;
	bogomem_bank.baseaddr = bogomemory;
  bootrom_filepos = 0;
  chip_filepos = 0;
  bogo_filepos = 0;
}

void map_overlay (int chip)
{
    int size;
    addrbank *cb;
    int currPC = m68k_getpc(&regs);

    size = allocated_chipmem >= 0x180000 ? (allocated_chipmem >> 16) : 32;
    cb = &chipmem_bank;
    if (chip) {
	map_banks (cb, 0, size, allocated_chipmem);
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
    if (savestate_state != STATE_RESTORE && valid_address (regs.pc, 4))
      m68k_setpc(&regs, currPC);
}

void memory_reset (void)
{
    int bnk, bnk_end;

    currprefs.chipmem_size = changed_prefs.chipmem_size;
    currprefs.bogomem_size = changed_prefs.bogomem_size;
    currprefs.cs_a1000ram = changed_prefs.cs_a1000ram;

    need_hardreset = 0;
    /* Use changed_prefs, as m68k_reset is called later.  */
    if (last_address_space_24 != changed_prefs.address_space_24)
	need_hardreset = 1;

    last_address_space_24 = changed_prefs.address_space_24;

    init_mem_banks ();
    allocate_memory ();

  if (strcmp (currprefs.romfile, changed_prefs.romfile) != 0
	|| strcmp (currprefs.romextfile, changed_prefs.romextfile) != 0)
    {
        kickstart_rom = 1;
	ersatzkickfile = 0;
	a1000_handle_kickstart (0);
	xfree (a1000_bootrom);
	a1000_bootrom = 0;
	a1000_kickstart_mode = 0;
	memcpy (currprefs.romfile, changed_prefs.romfile, sizeof currprefs.romfile);
	memcpy (currprefs.romextfile, changed_prefs.romextfile, sizeof currprefs.romextfile);
	need_hardreset = 1;
  mapped_free (extendedkickmemory);
	extendedkickmemory = 0;
  extendedkickmem_size = 0;
	extendedkickmemory2 = 0;
	extendedkickmem2_size = 0;
	extendedkickmem_type = 0;
  load_extendedkickstart ();
	kickmem_mask = 524288 - 1;
	if (!load_kickstart ()) {
	    if (strlen (currprefs.romfile) > 0) {
		    write_log ("Failed to open '%s'\n", currprefs.romfile);
    		notify_user (NUMSG_NOROM);
      }
#ifdef AUTOCONFIG
	  init_ersatz_rom (kickmemory);
	  kickmem_mask = 262144 - 1;
	  kickmem_size = 262144;
	  ersatzkickfile = 1;
#else
	  uae_restart (-1, NULL);
#endif
	} else {
    struct romdata *rd = getromdatabydata (kickmemory, kickmem_size);
	  if (rd) {
		if ((rd->cpu & 3) == 3 && changed_prefs.cpu_model != 68030) {
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
		if ((rd->type & ROMTYPE_SPECIALKICK | ROMTYPE_KICK) == ROMTYPE_KICK)
		    kickstart_rom = 1;
   }
  }
	patch_kick ();
    }

    map_banks (&custom_bank, 0xC0, 0xE0 - 0xC0, 0);
    map_banks (&cia_bank, 0xA0, 32, 0);
    if (!currprefs.cs_a1000ram)
      /* D80000 - DDFFFF not mapped (A1000 = custom chips) */
	    map_banks (&dummy_bank, 0xD8, 6, 0);

    /* map "nothing" to 0x200000 - 0x9FFFFF (0xBEFFFF if PCMCIA or AGA) */
    bnk = allocated_chipmem >> 16;
    if (bnk < 0x20 + (currprefs.fastmem_size >> 16))
       bnk = 0x20 + (currprefs.fastmem_size >> 16);
    bnk_end = ((currprefs.chipset_mask & CSMASK_AGA) ? 0xBF : 0xA0);
    map_banks (&dummy_bank, bnk, bnk_end - bnk, 0);
    if (currprefs.chipset_mask & CSMASK_AGA)
       map_banks (&dummy_bank, 0xc0, 0xd8 - 0xc0, 0);

    if (bogomemory != 0) {
      int t = allocated_bogomem >> 16;
	    map_banks (&bogomem_bank, 0xC0, t, 0);
    }
    map_banks (&clock_bank, 0xDC, 1, 0);

    map_banks (&kickmem_bank, 0xF8, 8, 0);
    /* map beta Kickstarts at 0x200000/0xC00000/0xF00000 */
    if (kickmemory[0] == 0x11 && kickmemory[2] == 0x4e && kickmemory[3] == 0xf9 && kickmemory[4] == 0x00) {
       uae_u32 addr = kickmemory[5];
    if (addr == 0x20 && allocated_chipmem <= 0x200000 && allocated_fastmem == 0)
      map_banks (&kickmem_bank, addr, 8, 0);
  	if (addr == 0xC0 && allocated_bogomem == 0)
	    map_banks (&kickmem_bank, addr, 8, 0);
  	if (addr == 0xF0)
	    map_banks (&kickmem_bank, addr, 8, 0);
    }

    if (a1000_bootrom)
       a1000_handle_kickstart (1);

#ifdef AUTOCONFIG
    map_banks (&expamem_bank, 0xE8, 1, 0);
#endif

    /* Map the chipmem into all of the lower 8MB */
    map_overlay (1);

    switch (extendedkickmem_type) {
        case EXTENDED_ROM_KS:
	        map_banks (&extendedkickmem_bank, 0xE0, 8, 0);
	        break;
    }
#ifdef AUTOCONFIG
    if (need_uae_boot_rom ())
	map_banks (&rtarea_bank, rtarea_base >> 16, 1, 0);
#endif

	  if ((cloanto_rom) && !extendedkickmem_type)
      map_banks (&kickmem_bank, 0xE0, 8, 0);
}

void memory_init (void)
{
    allocated_chipmem = 0;
    allocated_bogomem = 0;
    kickmemory = 0;
    extendedkickmemory = 0;
    extendedkickmem_size = 0;
    extendedkickmemory2 = 0;
    extendedkickmem2_size = 0;
    extendedkickmem_type = 0;
    chipmemory = 0;
    bogomemory = 0;

    init_mem_banks ();

    kickmemory = mapped_malloc (0x80000, "kick");
    memset (kickmemory, 0, 0x80000);
    kickmem_bank.baseaddr = kickmemory;
    strcpy (currprefs.romfile, "<none>");
    currprefs.romextfile[0] = 0;
}

void memory_cleanup (void)
{
    if (bogomemory)
	mapped_free (bogomemory);
    if (kickmemory)
	mapped_free (kickmemory);
    if (a1000_bootrom)
	xfree (a1000_bootrom);
    if (chipmemory)
	mapped_free (chipmemory);

    bogomemory = 0;
    kickmemory = 0;
    a1000_bootrom = 0;
    a1000_kickstart_mode = 0;
    chipmemory = 0;
}

void memory_hardreset(void)
{
    if (savestate_state == STATE_RESTORE)
	return;
    if (chipmemory)
	memset (chipmemory, 0, allocated_chipmem);
    if (bogomemory)
	memset (bogomemory, 0, allocated_bogomem);
    expansion_clear();
}

void map_banks (addrbank *bank, int start, int size, int realsize)
{
   int bnr;
   unsigned long int hioffs = 0, endhioffs = 0x100;
   addrbank *orgbank = bank;
   uae_u32 realstart = start;

   flush_icache (1);		/* Sure don't want to keep any old mappings around! */

   if (!realsize)
      realsize = size << 16;

   if ((size << 16) < realsize) {
	    write_log ("Broken mapping, size=%x, realsize=%x\nStart is %x\n",
	      size, realsize, start);
   }

#ifndef ADDRESS_SPACE_24BIT
   if (start >= 0x100) {
      int real_left = 0;
      for (bnr = start; bnr < start + size; bnr++) {
         if (!real_left) {
            realstart = bnr;
            real_left = realsize >> 16;
         }
         put_mem_bank (bnr << 16, bank, realstart << 16);
         real_left--;
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
      int real_left = 0;
      for (bnr = start; bnr < start + size; bnr++) {
         if (!real_left) {
            realstart = bnr + hioffs;
            real_left = realsize >> 16;
         }
         put_mem_bank ((bnr + hioffs) << 16, bank, realstart << 16);
         real_left--;
      }
   }
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
    *len = allocated_chipmem;
    return chipmemory;
}

uae_u8 *save_bram (int *len)
{
    *len = allocated_bogomem;
    return bogomemory;
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
    int i;
    mem_start = restore_u32 ();
    mem_size = restore_u32 ();
    mem_type = restore_u32 ();
    version = restore_u32 ();
    crc32 = restore_u32 ();
    for (i = 0; i < romlist_cnt; i++) {
	    if (rl[i].rd->crc32 == crc32 && crc32) {
	    switch (mem_type)
	    {
		    case 0:
	        strncpy (changed_prefs.romfile, rl[i].path, 255);
	        break;
		    case 1:
		      strncpy (changed_prefs.romextfile, rl[i].path, 255);
		      break;
	    }
	    break;
	    }
    }
    src += strlen ((char *)src) + 1;
    if (zfile_exists((char *)src)) {
	    switch (mem_type)
	    {
	      case 0:
	        strncpy (changed_prefs.romfile, (char *)src, 255);
	        break;
	      case 1:
	        strncpy (changed_prefs.romextfile, (char *)src, 255);
	        break;
	    }
    }
    src += strlen((char *)src) + 1;

    return src;
}

uae_u8 *save_rom (int first, int *len, uae_u8 *dstptr)
{
    static int count;
    uae_u8 *dst, *dstbak;
    uae_u8 *mem_real_start;
    uae_u32 version;
    char *path;
    int mem_start, mem_size, mem_type, saverom;
    int i;
    char tmpname[1000];

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
	    mem_real_start = kickmemory;
	    mem_size = kickmem_size;
	    path = currprefs.romfile;
	    /* 256KB or 512KB ROM? */
	    for (i = 0; i < mem_size / 2 - 4; i++) {
		if (longget (i + mem_start) != longget (i + mem_start + mem_size / 2))
		    break;
	    }
	    if (i == mem_size / 2 - 4) {
		mem_size /= 2;
		mem_start += 262144;
	    }
	    version = longget (mem_start + 12); /* version+revision */
	    sprintf (tmpname, "Kickstart %d.%d", wordget (mem_start + 12), wordget (mem_start + 14));
	    break;
	case 1: /* Extended ROM */
	    if (!extendedkickmem_type)
		break;
	    mem_start = extendedkickmem_start;
	    mem_real_start = extendedkickmemory;
	    mem_size = extendedkickmem_size;
	    path = currprefs.romextfile;
	    sprintf (tmpname, "Extended");
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
      dstbak = dst = (uae_u8 *)xmalloc (4 + 4 + 4 + 4 + 4 + 256 + 256 + mem_size);
    save_u32 (mem_start);
    save_u32 (mem_size);
    save_u32 (mem_type);
    save_u32 (version);
    save_u32 (get_crc32 (mem_real_start, mem_size));
    strcpy ((char *)dst, tmpname);
    dst += strlen ((char *)dst) + 1;
    strcpy ((char *)dst, path);/* rom image name */
    dst += strlen ((char *)dst) + 1;
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
    if (!addr_valid("memcpyha", dst, size))
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
    if (!addr_valid("memcpyah", src, size))
	return;
    while (size--)
	*dst++ = get_byte(src++);
}
void memcpyah (uae_u8 *dst, uaecptr src, int size)
{
    while (size--)
	*dst++ = get_byte(src++);
}
char *strcpyah_safe (char *dst, uaecptr src, int maxsize)
{
    char *res = dst;
    uae_u8 b;
    do {
	if (!addr_valid("strcpyah", src, 1))
	    return res;
	b = get_byte(src++);
	*dst++ = b;
	maxsize--;
	if (maxsize <= 1) {
	    *dst++= 0;
	    break;
	}
    } while (b);
    return res;
}
uaecptr strcpyha_safe (uaecptr dst, const char *src)
{
    uaecptr res = dst;
    uae_u8 b;
    do {
	if (!addr_valid("strcpyha", dst, 1))
	    return res;
	b = *src++;
	put_byte (dst++, b);
    } while (b);
    return res;
}
