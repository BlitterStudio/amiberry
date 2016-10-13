/*
* UAE - The Un*x Amiga Emulator
*
* CD32 Akiko emulation
*
* - C2P
* - NVRAM
* - CDROM
*
* Copyright 2001-2010 Toni Wilen
*
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "include/memory.h"
#include "newcpu.h"
#include "events.h"
#include "savestate.h"
#include "blkdev.h"
#include "zfile.h"
#include "threaddep/thread.h"
#include "akiko.h"
#include "gui.h"
#include "crc32.h"
#include "uae.h"
#include "custom.h"

#define AKIKO_DEBUG_NVRAM 0
#define AKIKO_DEBUG_IO 0
#define AKIKO_DEBUG_IO_CMD 0

// 43 48 49 4E 4F 4E 20 20 4F 2D 36 35 38 2D 32 20 32 34
#define FIRMWAREVERSION "CHINON  O-658-2 24"

static void irq (void)
{
#if AKIKO_DEBUG_IO > 1
	write_log (_T("Akiko Interrupt\n"));
#endif
	if (!(intreq & 8)) {
		INTREQ_0 (0x8000 | 0x0008);
	}
}

/*
* CD32 1Kb NVRAM (EEPROM) emulation
*
* NVRAM chip is 24C08 CMOS EEPROM (1024x8 bits = 1Kb)
* Chip interface is I2C (2 wire serial)
* Akiko addresses used:
* 0xb80030: bit 7 = SCL (clock), 6 = SDA (data)
* 0xb80032: 0xb80030 data direction register (0 = input, 1 = output)
*
* Because I don't have any experience on I2C, following code may be
* unnecessarily complex and not 100% correct..
*/

enum i2c { I2C_WAIT, I2C_START, I2C_DEVICEADDR,	I2C_WORDADDR, I2C_DATA };

/* size of EEPROM, don't try to change,
* (hardcoded in Kickstart)
*/
#define NVRAM_SIZE 1024
/* max size of one write request */
#define NVRAM_PAGE_SIZE 16

static uae_u8 cd32_nvram[NVRAM_SIZE], nvram_writetmp[NVRAM_PAGE_SIZE];
static int nvram_address, nvram_writeaddr;
static int nvram_rw;
static int bitcounter = -1, direction = -1;
static uae_u8 nvram_byte;
static int scl_out, scl_in, scl_dir, oscl, sda_out, sda_in, sda_dir, osda;
static int sda_dir_nvram;
static int state = I2C_WAIT;

static void nvram_write (int offset, int len)
{
	struct zfile *f;

	if (!currprefs.cs_cd32nvram)
		return;
	f = zfile_fopen (currprefs.flashfile, _T("rb+"), ZFD_NORMAL);
	if (!f) {
		f = zfile_fopen (currprefs.flashfile, _T("wb"), 0);
		if (!f) return;
		zfile_fwrite (cd32_nvram, NVRAM_SIZE, 1, f);
	}
	zfile_fseek (f, offset, SEEK_SET);
	zfile_fwrite (cd32_nvram + offset, len, 1, f);
	zfile_fclose (f);
}

static void nvram_read (void)
{
	struct zfile *f;

	if (!currprefs.cs_cd32nvram)
		return;
	f = zfile_fopen (currprefs.flashfile, _T("rb"), ZFD_NORMAL);
	memset (cd32_nvram, 0, NVRAM_SIZE);
	if (!f) return;
	zfile_fread (cd32_nvram, NVRAM_SIZE, 1, f);
	zfile_fclose (f);
}

static void i2c_do (void)
{
#if AKIKO_DEBUG_NVRAM
	int i;
#endif
	sda_in = 1;
	if (!sda_dir_nvram && scl_out && oscl) {
		if (!sda_out && osda) { /* START-condition? */
			state = I2C_DEVICEADDR;
			bitcounter = 0;
			direction = -1;
#if AKIKO_DEBUG_NVRAM
			write_log (_T("START\n"));
#endif
			return;
		} else if(sda_out && !osda) { /* STOP-condition? */
			state = I2C_WAIT;
			bitcounter = -1;
#if AKIKO_DEBUG_NVRAM
			write_log (_T("STOP\n"));
#endif
			if (direction > 0) {
				memcpy (cd32_nvram + (nvram_address & ~(NVRAM_PAGE_SIZE - 1)), nvram_writetmp, NVRAM_PAGE_SIZE);
				nvram_write (nvram_address & ~(NVRAM_PAGE_SIZE - 1), NVRAM_PAGE_SIZE);
				direction = -1;
				gui_flicker_led (LED_MD, 0, 2);
#if AKIKO_DEBUG_NVRAM
				write_log (_T("NVRAM write address %04X:"), nvram_address & ~(NVRAM_PAGE_SIZE - 1));
				for (i = 0; i < NVRAM_PAGE_SIZE; i++)
					write_log (_T("%02X"), nvram_writetmp[i]);
				write_log (_T("\n"));

#endif
			}
			return;
		}
	}
	if (bitcounter >= 0) {
		if (direction) {
			/* Amiga -> NVRAM */
			if (scl_out && !oscl) {
				if (bitcounter == 8) {
#if AKIKO_DEBUG_NVRAM
					write_log (_T("RB %02X "), nvram_byte, M68K_GETPC);
#endif
					sda_in = 0; /* ACK */
					if (direction > 0) {
						nvram_writetmp[nvram_writeaddr++] = nvram_byte;
						nvram_writeaddr &= 15;
						bitcounter = 0;
					} else {
						bitcounter = -1;
					}
				} else {
					//write_log (_T("NVRAM received bit %d, offset %d\n"), sda_out, bitcounter);
					nvram_byte <<= 1;
					nvram_byte |= sda_out;
					bitcounter++;
				}
			}
		} else {
			/* NVRAM -> Amiga */
			if (scl_out && !oscl && bitcounter < 8) {
				if (bitcounter == 0)
					nvram_byte = cd32_nvram[nvram_address];
				sda_dir_nvram = 1;
				sda_in = (nvram_byte & 0x80) ? 1 : 0;
				//write_log (_T("NVRAM sent bit %d, offset %d\n"), sda_in, bitcounter);
				nvram_byte <<= 1;
				bitcounter++;
				if (bitcounter == 8) {
#if AKIKO_DEBUG_NVRAM
					write_log (_T("NVRAM sent byte %02X address %04X PC=%08X\n"), cd32_nvram[nvram_address], nvram_address, M68K_GETPC);
#endif
					nvram_address++;
					nvram_address &= NVRAM_SIZE - 1;
					sda_dir_nvram = 0;
				}
			}
			if(!sda_out && sda_dir && !scl_out) /* ACK from Amiga */
				bitcounter = 0;
		}
		if (bitcounter >= 0) return;
	}
	switch (state)
	{
	case I2C_DEVICEADDR:
		if ((nvram_byte & 0xf0) != 0xa0) {
			write_log (_T("WARNING: I2C_DEVICEADDR: device address != 0xA0\n"));
			state = I2C_WAIT;
			return;
		}
		nvram_rw = (nvram_byte & 1) ? 0 : 1;
		if (nvram_rw) {
			/* 2 high address bits, only fetched if WRITE = 1 */
			nvram_address &= 0xff;
			nvram_address |= ((nvram_byte >> 1) & 3) << 8;
			state = I2C_WORDADDR;
			direction = -1;
		} else {
			state = I2C_DATA;
			direction = 0;
			sda_dir_nvram = 1;
		}
		bitcounter = 0;
#if AKIKO_DEBUG_NVRAM
		write_log (_T("I2C_DEVICEADDR: rw %d, address %02Xxx PC=%08X\n"), nvram_rw, nvram_address >> 8, M68K_GETPC);
#endif
		break;
	case I2C_WORDADDR:
		nvram_address &= 0x300;
		nvram_address |= nvram_byte;
#if AKIKO_DEBUG_NVRAM
		write_log (_T("I2C_WORDADDR: address %04X PC=%08X\n"), nvram_address, M68K_GETPC);
#endif
		if (direction < 0) {
			memcpy (nvram_writetmp, cd32_nvram + (nvram_address & ~(NVRAM_PAGE_SIZE - 1)), NVRAM_PAGE_SIZE);
			nvram_writeaddr = nvram_address & (NVRAM_PAGE_SIZE - 1);
			gui_flicker_led (LED_MD, 0, 1);
		}
		state = I2C_DATA;
		bitcounter = 0;
		direction = 1;
		break;
	}
}

static void akiko_nvram_write (int offset, uae_u32 v)
{
	int sda;
	switch (offset)
	{
	case 0:
		oscl = scl_out;
		scl_out = (v & 0x80) ? 1 : 0;
		osda = sda_out;
		sda_out = (v & 0x40) ? 1 : 0;
		break;
	case 2:
		scl_dir = (v & 0x80) ? 1 : 0;
		sda_dir = (v & 0x40) ? 1 : 0;
		break;
	default:
		return;
	}
	sda = sda_out;
	if (oscl != scl_out || osda != sda) {
		i2c_do ();
		oscl = scl_out;
		osda = sda;
	}
}

static uae_u32 akiko_nvram_read (int offset)
{
	uae_u32 v = 0;
	switch (offset)
	{
	case 0:
		if (!scl_dir)
			v |= scl_in ? 0x80 : 0x00;
		else
			v |= scl_out ? 0x80 : 0x00;
		if (!sda_dir)
			v |= sda_in ? 0x40 : 0x00;
		else
			v |= sda_out ? 0x40 : 0x00;
		break;
	case 2:
		v |= scl_dir ? 0x80 : 0x00;
		v |= sda_dir ? 0x40 : 0x00;
		break;
	}
	return v;
}

/* CD32 Chunky to Planar hardware emulation
* Akiko addresses used:
* 0xb80038-0xb8003b
*/

static uae_u32 akiko_buffer[8];
static int akiko_read_offset, akiko_write_offset;
static uae_u32 akiko_result[8];

#if 0
static void akiko_c2p_do (void)
{
	int i;

	for (i = 0; i < 8; i++)
		akiko_result[i] = 0;
	/* FIXME: better c2p algoritm than this piece of crap.... */
	for (i = 0; i < 8 * 32; i++) {
		if (akiko_buffer[7 - (i >> 5)] & (1 << (i & 31)))
			akiko_result[i & 7] |= 1 << (i >> 3);
	}
}
#else
/* Optimised Chunky-to-Planar algorithm by Mequa */
static uae_u32 akiko_precalc_shift[32];
static uae_u32 akiko_precalc_bytenum[32][8];
static void akiko_precalculate (void)
{
	uae_u32 i, j;
	for (i = 0; i < 32; i++) {
		akiko_precalc_shift  [(int)i] = 1 << i;
		for (j = 0; j < 8; j++) {
			akiko_precalc_bytenum[(int)i][(int)j] = (i >> 3) + ((7 - j) << 2);
		}
	}
}

static void akiko_c2p_do (void)
{
	int i;

	for (i = 0; i < 8; i++) {
		akiko_result[i]    =  (((akiko_buffer[0] & akiko_precalc_shift[i])    != 0) << (akiko_precalc_bytenum[i][0])   )
				    	   |  (((akiko_buffer[1] & akiko_precalc_shift[i])    != 0) << (akiko_precalc_bytenum[i][1])   )
				    	   |  (((akiko_buffer[2] & akiko_precalc_shift[i])    != 0) << (akiko_precalc_bytenum[i][2])   )
				    	   |  (((akiko_buffer[3] & akiko_precalc_shift[i])    != 0) << (akiko_precalc_bytenum[i][3])   )
				    	   |  (((akiko_buffer[4] & akiko_precalc_shift[i])    != 0) << (akiko_precalc_bytenum[i][4])   )
				    	   |  (((akiko_buffer[5] & akiko_precalc_shift[i])    != 0) << (akiko_precalc_bytenum[i][5])   )
				    	   |  (((akiko_buffer[6] & akiko_precalc_shift[i])    != 0) << (akiko_precalc_bytenum[i][6])   )
				    	   |  (((akiko_buffer[7] & akiko_precalc_shift[i])    != 0) << (akiko_precalc_bytenum[i][7])   )
						   |  (((akiko_buffer[0] & akiko_precalc_shift[i+8])  != 0) << (akiko_precalc_bytenum[i+8][0]) )
				    	   |  (((akiko_buffer[1] & akiko_precalc_shift[i+8])  != 0) << (akiko_precalc_bytenum[i+8][1]) )
				    	   |  (((akiko_buffer[2] & akiko_precalc_shift[i+8])  != 0) << (akiko_precalc_bytenum[i+8][2]) )
				    	   |  (((akiko_buffer[3] & akiko_precalc_shift[i+8])  != 0) << (akiko_precalc_bytenum[i+8][3]) )
				    	   |  (((akiko_buffer[4] & akiko_precalc_shift[i+8])  != 0) << (akiko_precalc_bytenum[i+8][4]) )
				    	   |  (((akiko_buffer[5] & akiko_precalc_shift[i+8])  != 0) << (akiko_precalc_bytenum[i+8][5]) )
				    	   |  (((akiko_buffer[6] & akiko_precalc_shift[i+8])  != 0) << (akiko_precalc_bytenum[i+8][6]) )
				    	   |  (((akiko_buffer[7] & akiko_precalc_shift[i+8])  != 0) << (akiko_precalc_bytenum[i+8][7]) )
						   |  (((akiko_buffer[0] & akiko_precalc_shift[i+16]) != 0) << (akiko_precalc_bytenum[i+16][0]))
				    	   |  (((akiko_buffer[1] & akiko_precalc_shift[i+16]) != 0) << (akiko_precalc_bytenum[i+16][1]))
				    	   |  (((akiko_buffer[2] & akiko_precalc_shift[i+16]) != 0) << (akiko_precalc_bytenum[i+16][2]))
				    	   |  (((akiko_buffer[3] & akiko_precalc_shift[i+16]) != 0) << (akiko_precalc_bytenum[i+16][3]))
				    	   |  (((akiko_buffer[4] & akiko_precalc_shift[i+16]) != 0) << (akiko_precalc_bytenum[i+16][4]))
				    	   |  (((akiko_buffer[5] & akiko_precalc_shift[i+16]) != 0) << (akiko_precalc_bytenum[i+16][5]))
				    	   |  (((akiko_buffer[6] & akiko_precalc_shift[i+16]) != 0) << (akiko_precalc_bytenum[i+16][6]))
				    	   |  (((akiko_buffer[7] & akiko_precalc_shift[i+16]) != 0) << (akiko_precalc_bytenum[i+16][7]))
						   |  (((akiko_buffer[0] & akiko_precalc_shift[i+24]) != 0) << (akiko_precalc_bytenum[i+24][0]))
				    	   |  (((akiko_buffer[1] & akiko_precalc_shift[i+24]) != 0) << (akiko_precalc_bytenum[i+24][1]))
				    	   |  (((akiko_buffer[2] & akiko_precalc_shift[i+24]) != 0) << (akiko_precalc_bytenum[i+24][2]))
				    	   |  (((akiko_buffer[3] & akiko_precalc_shift[i+24]) != 0) << (akiko_precalc_bytenum[i+24][3]))
				    	   |  (((akiko_buffer[4] & akiko_precalc_shift[i+24]) != 0) << (akiko_precalc_bytenum[i+24][4]))
				    	   |  (((akiko_buffer[5] & akiko_precalc_shift[i+24]) != 0) << (akiko_precalc_bytenum[i+24][5]))
				    	   |  (((akiko_buffer[6] & akiko_precalc_shift[i+24]) != 0) << (akiko_precalc_bytenum[i+24][6]))
				    	   |  (((akiko_buffer[7] & akiko_precalc_shift[i+24]) != 0) << (akiko_precalc_bytenum[i+24][7]));
	}
}
#endif

static void akiko_c2p_write (int offset, uae_u32 v)
{
	if (offset == 3)
		akiko_buffer[akiko_write_offset] = 0;
	akiko_buffer[akiko_write_offset] |= v << ( 8 * (3 - offset));
	if (offset == 0) {
		akiko_write_offset++;
		akiko_write_offset &= 7;
	}
	akiko_read_offset = 0;
}

static uae_u32 akiko_c2p_read (int offset)
{
	uae_u32 v;

	if (akiko_read_offset == 0 && offset == 3)
		akiko_c2p_do ();
	akiko_write_offset = 0;
	v = akiko_result[akiko_read_offset];
	if (offset == 0) {
		akiko_read_offset++;
		akiko_read_offset &= 7;
	}
	return v >> (8 * (3 - offset));
}

/* CD32 CDROM hardware emulation
* Akiko addresses used:
* 0xb80004-0xb80028
*/

#define CDINTERRUPT_SUBCODE		0x80000000
#define CDINTERRUPT_DRIVEXMIT	0x40000000 /* not used by ROM */
#define CDINTERRUPT_DRIVERECV	0x20000000 /* not used by ROM */
#define CDINTERRUPT_RXDMADONE	0x10000000
#define CDINTERRUPT_TXDMADONE	0x08000000
#define CDINTERRUPT_PBX			0x04000000
#define CDINTERRUPT_OVERFLOW	0x02000000

#define CDFLAG_SUBCODE			0x80000000
#define CDFLAG_TXD				0x40000000
#define CDFLAG_RXD				0x20000000
#define CDFLAG_CAS				0x10000000
#define CDFLAG_PBX				0x08000000
#define CDFLAG_ENABLE			0x04000000
#define CDFLAG_RAW				0x02000000
#define CDFLAG_MSB				0x01000000

#define CDS_ERROR 0x80
#define CDS_PLAYING 0x08
#define CDS_PLAYEND 0x00

#define CH_ERR_BADCOMMAND       0x80 // %10000000
#define CH_ERR_CHECKSUM         0x88 // %10001000
#define CH_ERR_DRAWERSTUCK      0x90 // %10010000
#define CH_ERR_DISKUNREADABLE   0x98 // %10011000
#define CH_ERR_INVALIDADDRESS   0xa0 // %10100000
#define CH_ERR_WRONGDATA        0xa8 // %10101000
#define CH_ERR_FOCUSERROR       0xc8 // %11001000
#define CH_ERR_SPINDLEERROR     0xd0 // %11010000
#define CH_ERR_TRACKINGERROR    0xd8 // %11011000
#define CH_ERR_SLEDERROR        0xe0 // %11100000
#define CH_ERR_TRACKJUMP        0xe8 // %11101000
#define CH_ERR_ABNORMALSEEK     0xf0 // %11110000
#define CH_ERR_NODISK           0xf8 // %11111000

static int subcodecounter;

#define MAX_SUBCODEBUFFER 36
static volatile int subcodebufferoffset, subcodebufferoffsetw;
static uae_u8 subcodebufferinuse[MAX_SUBCODEBUFFER];
static uae_u8 subcodebuffer[MAX_SUBCODEBUFFER * SUB_CHANNEL_SIZE];

static uae_u32 cdrom_intreq, cdrom_intena;
static uae_u8 cdrom_subcodeoffset;
static uae_u32 cdrom_addressdata, cdrom_addressmisc;
static uae_u32 subcode_address, cdrx_address, cdtx_address;
static uae_u32 cdrom_flags;
static uae_u32 cdrom_pbx;

static uae_u8 cdcomtxinx; /* 0x19 */
static uae_u8 cdcomrxinx; /* 0x1a */
static uae_u8 cdcomtxcmp; /* 0x1d */
static uae_u8 cdcomrxcmp; /* 0x1f */
static uae_u8 cdrom_result_buffer[32];
static uae_u8 cdrom_command_buffer[32];
static uae_u8 cdrom_command;

static int cdrom_toc_counter;
static uae_u32 cdrom_toc_crc;
static uae_u8 cdrom_toc_buffer[MAX_TOC_ENTRIES * 13];
static struct cd_toc_head cdrom_toc_cd_buffer;
static uae_u8 qcode_buf[SUBQ_SIZE];
static int qcode_valid;

static int cdrom_disk, cdrom_paused, cdrom_playing, cdrom_audiostatus;
static int cdrom_command_active;
static int cdrom_command_length;
static int cdrom_checksum_error, cdrom_unknown_command;
static int cdrom_data_offset, cdrom_speed, cdrom_sector_counter;
static int cdrom_current_sector, cdrom_seek_delay;
static int cdrom_data_end;
static int cdrom_audiotimeout;
static int cdrom_led;
static int cdrom_receive_length, cdrom_receive_offset;
static int cdrom_muted;
static int cd_initialized;
static int cdrom_tx_dma_delay;

static uae_u8 *sector_buffer_1, *sector_buffer_2;
static int sector_buffer_sector_1, sector_buffer_sector_2;
#define	SECTOR_BUFFER_SIZE 64
static uae_u8 *sector_buffer_info_1, *sector_buffer_info_2;

static int unitnum = -1;
static bool akiko_inited;
static volatile int mediachanged, mediacheckcounter;
static volatile int frame2counter;

static smp_comm_pipe requests;
static volatile int akiko_thread_running;
static uae_sem_t akiko_sem, sub_sem;

static void checkint (void)
{
	if (cdrom_intreq & cdrom_intena)
		irq ();
}

static void set_status (uae_u32 status)
{
#if AKIKO_DEBUG_IO
	if (!(cdrom_intreq & status))
		write_log (_T("Akiko INTREQ %08x (%08x)\n"), status, cdrom_intreq | status);
#endif
	cdrom_intreq |= status;
	checkint ();
	cdrom_led ^= LED_CD_ACTIVE2;
}

void rethink_akiko (void)
{
	checkint ();
}

static void cdaudiostop_do (void)
{
	qcode_valid = 0;
	if (unitnum < 0)
		return;
	sys_command_cd_pause (unitnum, 0);
	sys_command_cd_stop (unitnum);
}

static void cdaudiostop (void)
{
	cdrom_audiostatus = 0;
	cdrom_audiotimeout = 0;
	cdrom_paused = 0;
	cdrom_playing = 0;
	write_comm_pipe_u32 (&requests, 0x0104, 1);
}

static void subfunc (uae_u8 *data, int cnt)
{
	if (!(cdrom_flags & CDFLAG_SUBCODE))
		return;
	uae_sem_wait (&sub_sem);
#if 0
	int total = 0;
	for (int i = 0; i < MAX_SUBCODEBUFFER; i++) {
		if (subcodebufferinuse[i])
			total++;
	}
	write_log (_T("%d "), total);
#endif
	if (subcodebufferinuse[subcodebufferoffsetw]) {
		memset (subcodebufferinuse, 0,sizeof (subcodebufferinuse));
		subcodebufferoffsetw = subcodebufferoffset = 0;
		uae_sem_post (&sub_sem);
		//write_log (_T("CD32: subcode buffer overflow 1\n"));
		return;
	}
	int offset = subcodebufferoffsetw;
	while (cnt > 0) {
		if (subcodebufferinuse[offset]) {
			write_log (_T("CD32: subcode buffer overflow 2\n"));
			break;
		}
		subcodebufferinuse[offset] = 1;
		memcpy (&subcodebuffer[offset * SUB_CHANNEL_SIZE], data, SUB_CHANNEL_SIZE);
		data += SUB_CHANNEL_SIZE;
		offset++;
		if (offset >= MAX_SUBCODEBUFFER)
			offset = 0;
		cnt--;
	}
	subcodebufferoffsetw = offset;
	uae_sem_post (&sub_sem);
}

static int statusfunc (int status)
{
	if (status == -1)
		return 0;
	if (status == -2)
		return 10;
#if 1
	if (cdrom_audiostatus != status) {
		if (status == AUDIO_STATUS_IN_PROGRESS) {
			cdrom_playing = 1;
			cdrom_audiotimeout = 1;
		} 
		if (cdrom_playing && status != AUDIO_STATUS_IN_PROGRESS && status != AUDIO_STATUS_PAUSED && status != AUDIO_STATUS_NOT_SUPPORTED) {
			cdrom_audiotimeout = -1;
		}
	}
	cdrom_audiostatus = status;
#endif
	return 0;
}

static void cdaudioplay_do (void)
{
	uae_u32 startlsn = read_comm_pipe_u32_blocking (&requests);
	uae_u32 endlsn = read_comm_pipe_u32_blocking (&requests);
	uae_u32 scan = read_comm_pipe_u32_blocking (&requests);
	qcode_valid = 0;
	if (unitnum < 0)
		return;
	sys_command_cd_pause (unitnum, 0);
	sys_command_cd_play (unitnum, startlsn, endlsn, scan, statusfunc, subfunc);
}

static bool isaudiotrack (int startlsn)
{
	struct cd_toc *s = NULL;
	uae_u32 addr;
	int i;

	if (!cdrom_toc_cd_buffer.points)
		return false;
	for (i = 0; i < cdrom_toc_cd_buffer.points; i++) {
		s = &cdrom_toc_cd_buffer.toc[i];
		addr = s->paddress;
		if (s->track > 0 && s->track < 100 && addr >= startlsn)
			break;
		s++;
	}
	if (s && (s->control & 0x0c) == 0x04) {
		write_log (_T("tried to play data track %d!\n"), s->track);
		return false;
	}
	return true;
}

static struct cd_toc *get_track (int startlsn)
{
	for (int i = cdrom_toc_cd_buffer.first_track_offset + 1; i <= cdrom_toc_cd_buffer.last_track_offset; i++) {
		struct cd_toc *s = &cdrom_toc_cd_buffer.toc[i];
		uae_u32 addr = s->paddress;
		if (startlsn < addr)
			return s - 1;
	}
	return NULL;
}

static int last_play_end;
static int cd_play_audio (int startlsn, int endlsn, int scan)
{
	struct cd_toc *s = NULL;

	if (!cdrom_toc_cd_buffer.points)
		return 0;
	s = get_track (startlsn);
	if (s && (s->control & 0x0c) == 0x04) {
		s = get_track (startlsn + 150);
		if (s && (s->control & 0x0c) == 0x04) {
			write_log (_T("tried to play data track %d!\n"), s->track);
			s++;
			startlsn = s->paddress;
			s++;
			endlsn = s->paddress;
		}
		startlsn = s->paddress;
	}
	qcode_valid = 0;
	last_play_end = endlsn;
	cdrom_audiotimeout = 10;
	cdrom_paused = 0;
	write_comm_pipe_u32 (&requests, 0x0110, 0);
	write_comm_pipe_u32 (&requests, startlsn, 0);
	write_comm_pipe_u32 (&requests, endlsn, 0);
	write_comm_pipe_u32 (&requests, scan, 1);
	return 1;
}


/* read qcode */
static int last_play_pos;
static int cd_qcode (uae_u8 *d)
{
	uae_u8 *buf, *s, as;

	if (d)
		memset (d, 0, 11);
	last_play_pos = 0;
	buf = qcode_buf;
	as = buf[1];
	buf[2] = 0x80;
	if (!qcode_valid)
		return 0;
	if (cdrom_audiostatus != AUDIO_STATUS_IN_PROGRESS && cdrom_audiostatus != AUDIO_STATUS_PAUSED)
		return 0;
	s = buf + 4;
	last_play_pos = msf2lsn (fromlongbcd (s + 7));
	if (!d)
		return 0;
	buf[2] = 0;
	/* ??? */
	d[0] = 0;
	/* CtlAdr */
	d[1] = s[0];
	/* Track */
	d[2] = s[1];
	/* Index */
	d[3] = s[2];
	/* TrackPos */
	d[4] = s[3];
	d[5] = s[4];
	d[6] = s[5];
	/* DiskPos */
	d[7] = 0;
	d[8] = s[7];
	d[9] = s[8];
	d[10] = s[9];
	if (as == AUDIO_STATUS_IN_PROGRESS) {
		/* Make sure end of disc position is not missed.
		*/
		if (last_play_pos >= cdrom_toc_cd_buffer.lastaddress || cdrom_toc_cd_buffer.lastaddress - last_play_pos < 10) {
			int msf = lsn2msf (cdrom_toc_cd_buffer.lastaddress);
			d[8] = tobcd ((uae_u8)(msf >> 16));
			d[9] = tobcd ((uae_u8)(msf >> 8));
			d[10] = tobcd ((uae_u8)(msf >> 0));
		}
	}
//	write_log (_T("%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X\n"),
//		d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11]);
	return 0;
}

/* read toc */
static int get_cdrom_toc (void)
{
	int j;
	int datatrack = 0, secondtrack = 0;

	cdrom_toc_counter = -1;
	if (!sys_command_cd_toc (unitnum, &cdrom_toc_cd_buffer))
		return 1;
	memset (cdrom_toc_buffer, 0, MAX_TOC_ENTRIES * 13);
	cdrom_data_end = -1;
	for (j = 0; j < cdrom_toc_cd_buffer.points; j++) {
		struct cd_toc *s = &cdrom_toc_cd_buffer.toc[j];
		uae_u8 *d = &cdrom_toc_buffer[j * 13];
		int addr = s->paddress;
		int msf = lsn2msf (addr);
		if (s->point == 0xa0 || s->point == 0xa1)
			msf = s->track << 16;
		d[1] = (s->adr << 0) | (s->control << 4);
		d[3] = s->point < 100 ? tobcd (s->point) : s->point;
		d[8] = tobcd ((msf >> 16) & 0xff);
		d[9] = tobcd ((msf >> 8) & 0xff);
		d[10] = tobcd ((msf >> 0) & 0xff);
		if (s->point == 1 && (s->control & 0x0c) == 0x04)
			datatrack = 1;
		if (s->point == 2)
			secondtrack = addr;
	}
	cdrom_toc_crc = get_crc32 (cdrom_toc_buffer, cdrom_toc_cd_buffer.points * 13);
	if (datatrack) {
		if (secondtrack)
			cdrom_data_end = secondtrack;
		else
			cdrom_data_end = cdrom_toc_cd_buffer.lastaddress;
	}
	return 0;
}

/* open device */
static int sys_cddev_open (void)
{
	struct device_info di;
	unitnum = get_standard_cd_unit (CD_STANDARD_UNIT_CD32);
	sys_command_info (unitnum, &di, 0);
	write_log (_T("using drive %s (unit %d, media %d)\n"), di.label, unitnum, di.media_inserted);
	/* make sure CD audio is not playing */
	cdaudiostop_do ();
	return 0;
}

/* close device */
static void sys_cddev_close (void)
{
	if (unitnum >= 0) {
		cdaudiostop_do ();
		sys_command_close (unitnum);
	}
	unitnum = -1;
	
}

static int command_lengths[] = { 1,2,1,1,12,2,1,1,4,1,2,-1,-1,-1,-1,-1 };

static int cdrom_start_return_data (int len)
{
	if (cdrom_receive_length > 0)
		return 0;
	if (len <= 0)
		return -1;
	cdrom_receive_length = len;
	cdrom_receive_offset = -1;
	return 1;
}

/*
	RX DMA channel writes bytes to memory if DMA enabled, cdcomrxinx != cdcomrxcmp
	and there is data available from CDROM firmware code.
	
	Triggers CDINTERRUPT_RXDMADONE and stops transfer (even if there is
	more data available) when cdcomrxinx matches cdcomrxcmp
*/

static void cdrom_return_data (void)
{
	uae_u32 cmd_buf = cdrx_address;
	int i;
	uae_u8 checksum;

	if (!cdrom_receive_length)
		return;
	if (!(cdrom_flags & CDFLAG_RXD))
		return;
	if (cdcomrxinx == cdcomrxcmp)
		return;

#if AKIKO_DEBUG_IO
	write_log (_T("OUT IDX=0x%02X-0x%02X LEN=%d,%08x:"), cdcomrxinx, cdcomrxcmp, cdrom_receive_length, cmd_buf);
#endif

	if (cdrom_receive_offset < 0) {
	  checksum = 0xff;
		for (i = 0; i < cdrom_receive_length; i++) {
		  checksum -= cdrom_result_buffer[i];
#if AKIKO_DEBUG_IO
		  write_log (_T("%02X "), cdrom_result_buffer[i]);
#endif
	  }
#if AKIKO_DEBUG_IO
	  write_log (_T("(%02X)\n"), checksum);
#endif
		cdrom_result_buffer[cdrom_receive_length++] = checksum;
		cdrom_receive_offset = 0;
	} else {
#if AKIKO_DEBUG_IO
		write_log (_T("\n"));
#endif
	}
	while (cdrom_receive_offset < cdrom_receive_length && cdcomrxinx != cdcomrxcmp) {
		put_byte (cmd_buf + cdcomrxinx, cdrom_result_buffer[cdrom_receive_offset]);
		cdcomrxinx++;
		cdrom_receive_offset++;
	}
	if (cdcomrxinx == cdcomrxcmp) {
	  set_status (CDINTERRUPT_RXDMADONE);
#if AKIKO_DEBUG_IO
		write_log (_T("RXDMADONE %d/%d\n"), cdrom_receive_offset, cdrom_receive_length);
#endif
	}

	if (cdrom_receive_offset == cdrom_receive_length)
		cdrom_receive_length = 0;
}

static int cdrom_command_led (void)
{
	int v = cdrom_command_buffer[1];
	int old = cdrom_led;
	cdrom_led &= ~LED_CD_ACTIVE;
	cdrom_led |= (v & 1) ? LED_CD_ACTIVE : 0;
	if (cdrom_led != old)
		gui_flicker_led (LED_CD, 0, cdrom_led);
	if (v & 0x80) { // result wanted?
		cdrom_result_buffer[0] = cdrom_command;
		cdrom_result_buffer[1] = (cdrom_led & LED_CD_ACTIVE) ? 1 : 0;
		return 2;
	}
	return 0;
}

static int cdrom_command_idle_status (void)
{
	cdrom_result_buffer[0] = 0x0a;
	cdrom_result_buffer[1] = 0x70;
	return 2;
}

static int cdrom_command_media_status (void)
{
	cdrom_result_buffer[0] = 0x0a;
	cdrom_result_buffer[1] = sys_command_ismedia (unitnum, 0) > 0 ? 0x83: 0x80;
	return 2;
}

/* check if cd drive door is open or closed, return firmware info */
static int cdrom_command_status (void)
{
	cdrom_result_buffer[1] = 0x01;
	//cdrom_result_buffer[1] = 0x80; door open
	if (unitnum >= 0)
		get_cdrom_toc ();
	/* firmware info */
	memcpy (cdrom_result_buffer + 2, FIRMWAREVERSION, sizeof FIRMWAREVERSION);
	cdrom_result_buffer[0] = cdrom_command;
	cd_initialized = 1;
	return 20;
}

/* return one TOC entry, each TOC entry repeats 3 times */
#define TOC_REPEAT 3
static int cdrom_return_toc_entry (void)
{
	cdrom_result_buffer[0] = 6;
#if AKIKO_DEBUG_IO_CMD
	write_log (_T("CD32: TOC entry %d/%d\n"), cdrom_toc_counter / TOC_REPEAT, cdrom_toc_cd_buffer.points);
#endif
	if (cdrom_toc_cd_buffer.points == 0) {
		cdrom_result_buffer[1] = CDS_ERROR;
		return 15;
	}
	cdrom_result_buffer[1] = 0;
	memcpy (cdrom_result_buffer + 2, cdrom_toc_buffer + (cdrom_toc_counter / TOC_REPEAT) * 13, 13);
	cdrom_toc_counter++;
	if (cdrom_toc_counter / TOC_REPEAT >= cdrom_toc_cd_buffer.points)
		cdrom_toc_counter = -1;
	return 15;
}

static int checkerr (void)
{
	if (!cdrom_disk) {
		cdrom_result_buffer[1] = CH_ERR_NODISK;
		return 1;
	}
	return 0;
}

static int cdrom_command_stop (void)
{
	cdrom_audiotimeout = 0;
	cdrom_result_buffer[0] = cdrom_command;
	if (checkerr ())
		return 2;
	cdrom_result_buffer[1] = 0;
	cdaudiostop ();
	return 2;
}

/* pause CD audio */
static int cdrom_command_pause (void)
{
#if AKIKO_DEBUG_IO_CMD
	write_log (_T("CD32: pause: %d, %d\n"), cdrom_paused, cdrom_playing);
#endif
	cdrom_audiotimeout = 0;
	cdrom_toc_counter = -1;
	cdrom_result_buffer[0] = cdrom_command;
	if (checkerr ())
		return 2;
	cdrom_result_buffer[1] = cdrom_playing ? CDS_PLAYING : 0;
	if (cdrom_paused)
		return 2;
	cdrom_paused = 1;
	if (!cdrom_playing)
		return 2;
	write_comm_pipe_u32 (&requests, 0x0102, 1);
	return 2;
}

/* unpause CD audio */
static int cdrom_command_unpause (void)
{
#if AKIKO_DEBUG_IO_CMD
	write_log (_T("CD32: unpause: %d, %d\n"), cdrom_paused, cdrom_playing);
#endif
	cdrom_result_buffer[0] = cdrom_command;
	if (checkerr ())
		return 2;
	cdrom_result_buffer[1] = cdrom_playing ? CDS_PLAYING : 0;
	if (!cdrom_paused)
		return 2;
	cdrom_paused = 0;
	if (!cdrom_playing)
		return 2;
	write_comm_pipe_u32 (&requests, 0x0103, 1);
	return 2;
}

/* seek	head/play CD audio/read	data sectors */
static int cdrom_command_multi (void)
{
	int seekpos = msf2lsn (fromlongbcd (cdrom_command_buffer + 1));
	int endpos = msf2lsn (fromlongbcd (cdrom_command_buffer + 4));

	if (cdrom_playing)
		cdaudiostop ();
	cdrom_speed = (cdrom_command_buffer[8] & 0x40) ? 2 : 1;
	cdrom_result_buffer[0] = cdrom_command;
	cdrom_result_buffer[1] = 0;
	if (!cdrom_disk) {
		cdrom_result_buffer[1] = 1; // no disk
		return 2;
	}

	if (cdrom_command_buffer[7] == 0x80) { /* data read */
#if AKIKO_DEBUG_IO_CMD
		int cdrom_data_offset_end = endpos;
#endif
		cdrom_data_offset = seekpos;
		cdrom_seek_delay = abs (cdrom_current_sector - cdrom_data_offset);
		if (cdrom_seek_delay < 100) {
			cdrom_seek_delay = 1;
		} else {
			cdrom_seek_delay /= 1000;
			cdrom_seek_delay += 10;
			if (cdrom_seek_delay > 100)
				cdrom_seek_delay = 100;
		}
#if AKIKO_DEBUG_IO_CMD
		write_log (_T("READ DATA %06X (%d) - %06X (%d) SPD=%dx PC=%08X\n"),
			seekpos, cdrom_data_offset, endpos, cdrom_data_offset_end, cdrom_speed, M68K_GETPC);
#endif
		cdrom_result_buffer[1] |= 0x02;
	} else if (cdrom_command_buffer[10] & 4) { /* play audio */
#if AKIKO_DEBUG_IO_CMD
		int scan = 0;
		if (cdrom_command_buffer[7] & 0x04)
			scan = 1;
		else if (cdrom_command_buffer[7] & 0x08)
			scan = -1;
		write_log (_T("PLAY FROM %06X (%d) to %06X (%d) SCAN=%d\n"),
			seekpos, msf2lsn (seekpos), endpos, msf2lsn (endpos), scan);
#endif
		//cdrom_result_buffer[1] |= CDS_PLAYING;
		cdrom_playing = 1;
		if (!cd_play_audio (seekpos, endpos, 0)) {
			// play didn't start, report it in next status packet
			cdrom_audiotimeout = -3;
		}
	} else {
#if AKIKO_DEBUG_IO_CMD
		write_log (_T("SEEKTO %06X\n"),seekpos);
#endif
		if (seekpos < 150)
			cdrom_toc_counter = 0;
		else
			cdrom_toc_counter = -1;
	}
	return 2;
}

static int cdrom_playend_notify (int status)
{
	cdrom_result_buffer[0] = 4;
	if (status < 0)
		cdrom_result_buffer[1] = 0x80; // error
	else if (status == 0)
		cdrom_result_buffer[1] = 0x08; // play started
	else
		cdrom_result_buffer[1] = 0x00; // play ended
	return 2;
}

/* return subq entry */
static int cdrom_command_subq (void)
{
	cdrom_result_buffer[0] = cdrom_command;
	cdrom_result_buffer[1] = 0;
	cd_qcode (cdrom_result_buffer + 2);
	return 15;
}

/*
	TX DMA reads bytes from memory and sends them to
	CDROM hardware if TX DMA enabled, CDROM data transfer
	DMA not enabled and cdcomtxinx != cdcomtx.

	CDINTERRUPT_TXDMADONE triggered when cdromtxinx matches cdcomtx.
*/

static void cdrom_run_command (void)
{
	int i, cmd_len;
	uae_u8 checksum;
#if 0
	uae_u8 *pp = get_real_address (cdtx_address);
#endif

	if (!(cdrom_flags & CDFLAG_TXD))
		return;
	if ((cdrom_flags & CDFLAG_ENABLE))
		return;
	if (cdrom_command_active)
  	return;
	if (cdrom_receive_length)
			return;
	if (cdcomtxinx == cdcomtxcmp)
		return;
	if (cdrom_tx_dma_delay > 0)
		return;

	cdrom_command = get_byte (cdtx_address + cdcomtxinx);

	if (cdrom_command == 0) {
		cdcomtxinx++;
		return;
	}

	cdrom_checksum_error = 0;
	cdrom_unknown_command = 0;

	cmd_len = command_lengths[cdrom_command & 0x0f];
	if (cmd_len < 0) {
#if AKIKO_DEBUG_IO_CMD
		write_log (_T("unknown command %x\n"), cdrom_command & 0x0f);
#endif
		cdrom_unknown_command = 1;
		cdrom_command_active = 1;
		cdrom_command_length = 1;
		set_status (CDINTERRUPT_TXDMADONE);
		return;
	}

#if AKIKO_DEBUG_IO_CMD
	write_log (_T("IN CMD=%02X IDX=0x%02X-0x%02X LEN=%d:"), cdrom_command & 0x0f, cdcomtxinx, cdcomtxcmp, cmd_len);
#endif
	checksum = 0;
	for (i = 0; i < cmd_len + 1; i++) {
		cdrom_command_buffer[i] = get_byte (cdtx_address + ((cdcomtxinx + i) & 0xff));
		checksum += cdrom_command_buffer[i];
#if AKIKO_DEBUG_IO_CMD
		if (i == cmd_len)
			write_log (_T("(%02X) "), cdrom_command_buffer[i]); // checksum
		else
			write_log (_T("%02X "), cdrom_command_buffer[i]);
#endif
	}
	if (checksum != 0xff) {
#if AKIKO_DEBUG_IO_CMD
		write_log (_T(" checksum error"));
#endif
		cdrom_checksum_error = 1;
		//activate_debugger ();
	}
#if AKIKO_DEBUG_IO_CMD
	write_log (_T("\n"));
#endif
	cdrom_command_active = 1;
	cdrom_command_length = cmd_len;
	set_status (CDINTERRUPT_TXDMADONE);
}

static void cdrom_run_command_run (void)
{
	int len;

	cdcomtxinx = cdcomtxinx + cdrom_command_length + 1;
	memset (cdrom_result_buffer, 0, sizeof (cdrom_result_buffer));

	if (cdrom_checksum_error || cdrom_unknown_command) {
		cdrom_result_buffer[0] = (cdrom_command & 0xf0) | 5;
		if (cdrom_checksum_error)
			cdrom_result_buffer[1] |= CH_ERR_CHECKSUM;
		else if (cdrom_unknown_command)
			cdrom_result_buffer[1] |= CH_ERR_BADCOMMAND;
		len = 2;
		cdrom_start_return_data (len);
		return;
	}

	switch (cdrom_command & 0x0f)
	{
	case 0:
		len = 1;
		cdrom_result_buffer[0] = cdrom_command;
		break;
	case 1:
		len = cdrom_command_stop ();
		break;
	case 2:
		len = cdrom_command_pause ();
		break;
	case 3:
		len = cdrom_command_unpause ();
		break;
	case 4:
		len = cdrom_command_multi ();
		break;
	case 5:
		len = cdrom_command_led ();
		break;
	case 6:
		len = cdrom_command_subq ();
		break;
	case 7:
		len = cdrom_command_status ();
		break;
	default:
		len = 0;
		break;
	}
	if (len == 0)
		return;
	cdrom_start_return_data (len);
}

/* DMA transfer one CD sector */
static void cdrom_run_read (void)
{
	int i, sector, inc;
	int sec;
	int seccnt;

	if (!(cdrom_flags & CDFLAG_ENABLE))
		return;
	if (!cdrom_pbx) {
		set_status (CDINTERRUPT_OVERFLOW);
		return;
	}
	if (!(cdrom_flags & CDFLAG_PBX))
		return;
	if (cdrom_data_offset < 0)
		return;
	if (unitnum < 0)
		return;

	inc = 1;
	// always use highest available slot or Lotus 3 (Lotus Trilogy) fails to load
	for (seccnt = 15; seccnt >= 0; seccnt--) {
		if (cdrom_pbx & (1 << seccnt))
			break;
	}
	sector = cdrom_current_sector = cdrom_data_offset + cdrom_sector_counter;
	sec = sector - sector_buffer_sector_1;
	if (sector_buffer_sector_1 >= 0 && sec >= 0 && sec < SECTOR_BUFFER_SIZE) {
		if (sector_buffer_info_1[sec] != 0xff && sector_buffer_info_1[sec] != 0) {
			uae_u8 buf[2352];

			memcpy (buf, sector_buffer_1 + sec * 2352, 2352);
			buf[0] = 0;
			buf[1] = 0;
			buf[2] = 0;
			buf[3] = cdrom_sector_counter & 31;
			for (i = 0; i < 2352; i++)
				put_byte (cdrom_addressdata + seccnt * 4096 + i, buf[i]);
			for (i = 0; i < 73 * 2; i++)
				put_byte (cdrom_addressdata + seccnt * 4096 + 0xc00 + i, 0);
			cdrom_pbx &= ~(1 << seccnt);
			set_status (CDINTERRUPT_PBX);
		} else {
			inc = 0;
		}
		if (sector_buffer_info_1[sec] != 0xff)
			sector_buffer_info_1[sec]--;
#if AKIKO_DEBUG_IO_CMD
		write_log (_T("pbx=%04x sec=%d, scnt=%d -> %d. %d (%04x) %08X\n"),
			cdrom_pbx, cdrom_data_offset, cdrom_sector_counter, sector, seccnt, 1 << seccnt, cdrom_addressdata + seccnt * 4096);
#endif
	} else {
		inc = 0;
	}
	if (inc)
		cdrom_sector_counter++;
}

static int lastmediastate = 0;

static void akiko_handler (bool framesync)
{
	if (unitnum < 0)
		return;
	if (!cd_initialized || cdrom_receive_length)
		return;

	if (mediachanged) {
		if (cdrom_start_return_data (0) < 0) {
			cdrom_start_return_data (cdrom_command_media_status ());
			mediachanged = 0;
			get_cdrom_toc ();
			/* do not remove! first try may fail */
			get_cdrom_toc ();
		}
		return;
	}
	if (cdrom_audiotimeout > 1)
		cdrom_audiotimeout--;
	if (cdrom_audiotimeout == 1) { // play start
		cdrom_playing = 1;
		//cdrom_start_return_data (cdrom_playend_notify (0));
		cdrom_audiotimeout = 0;
	}
	if (cdrom_audiotimeout == -1) { // play finished (or disk end)
		if (cdrom_playing) {
			cdaudiostop ();
			cdrom_audiotimeout = -2;
		} else {
			cdrom_audiotimeout = 0;
		}
	}
	if (cdrom_audiotimeout == -2) { // play end notification
		cdrom_start_return_data (cdrom_playend_notify (1));
		cdrom_audiotimeout = 0;
	}
	 // play didn't start notification (illegal address)
	if (cdrom_audiotimeout == -3) { // return error status
		cdrom_start_return_data (cdrom_playend_notify (-1));
		cdrom_audiotimeout = 0;
	}

	/* one toc entry / frame */
	if (cdrom_toc_counter >= 0 && !cdrom_command_active && framesync) {
		if (cdrom_start_return_data (-1)) {
		  cdrom_start_return_data (cdrom_return_toc_entry ());
		}
	}
}

static void akiko_internal (void)
{
	if (!currprefs.cs_cd32cd)
		return;
	cdrom_return_data ();
	cdrom_run_command ();
	if (cdrom_command_active > 0) {
		cdrom_command_active--;
		if (!cdrom_command_active)
			cdrom_run_command_run ();
	}
}

void AKIKO_hsync_handler (void)
{
	bool framesync = false;
	if (!currprefs.cs_cd32cd || !akiko_inited)
		return;

	static float framecounter;
	framecounter--;
	if (framecounter <= 0) {
		if (cdrom_seek_delay <= 0) {
			cdrom_run_read ();
		} else {
			cdrom_seek_delay--;
		}
		framecounter += (float)maxvpos * vblank_hz / (75.0 * cdrom_speed);
		framesync = true;
	}

	if (cdrom_tx_dma_delay > 0)
		cdrom_tx_dma_delay--;

	subcodecounter--;
	if (subcodecounter <= 0) {
		if ((cdrom_flags & CDFLAG_SUBCODE) && cdrom_playing && subcodebufferoffset != subcodebufferoffsetw) {
			uae_sem_wait (&sub_sem);
			if (subcodebufferinuse[subcodebufferoffset]) {
				if (cdrom_subcodeoffset >= 128)
					cdrom_subcodeoffset = 0;
				else
					cdrom_subcodeoffset = 128;
				// 96 byte subchannel data
				for (int i = 0; i < SUB_CHANNEL_SIZE; i++)
					put_byte (subcode_address + cdrom_subcodeoffset + i, subcodebuffer[subcodebufferoffset * SUB_CHANNEL_SIZE + i]);
				put_long (subcode_address + cdrom_subcodeoffset + SUB_CHANNEL_SIZE, 0xffffffff);
				subcodebufferinuse[subcodebufferoffset] = 0;
				cdrom_subcodeoffset += 100;
				subcodebufferoffset++;
				if (subcodebufferoffset >= MAX_SUBCODEBUFFER)
					subcodebufferoffset -= MAX_SUBCODEBUFFER;
				set_status (CDINTERRUPT_SUBCODE);
				//write_log (_T("*"));
			}
			uae_sem_post (&sub_sem);
		}
		subcodecounter = maxvpos * vblank_hz / (75 * cdrom_speed) - 5;
	}

	if (frame2counter > 0)
		frame2counter--;
	if (mediacheckcounter > 0)
		mediacheckcounter--;

	akiko_internal ();
	akiko_handler (framesync);
}

/* cdrom data buffering thread */
static void *akiko_thread (void *null)
{
	int i;
	uae_u8 *tmp1;
	uae_u8 *tmp2;
	int tmp3;
	int sector;

	while (akiko_thread_running || comm_pipe_has_data (&requests)) {

		if (comm_pipe_has_data (&requests)) {
			uae_u32 b = read_comm_pipe_u32_blocking (&requests);
			switch (b)
			{
			case 0x0102: // pause
				sys_command_cd_pause (unitnum, 1);
				break;
			case 0x0103: // unpause
				sys_command_cd_pause (unitnum, 0);
				break;
			case 0x0104: // stop
				cdaudiostop_do ();
				break;
			case 0x0105: // mute change
				sys_command_cd_volume (unitnum, cdrom_muted ? 0 : 0x7fff, cdrom_muted ? 0 : 0x7fff);
				break;
			case 0x0110: // do_play!
				sys_command_cd_volume (unitnum, cdrom_muted ? 0 : 0x7fff, cdrom_muted ? 0 : 0x7fff);
				cdaudioplay_do ();
				break;
			}
		}

		if (frame2counter <= 0) {
			frame2counter = 312 * 50 / 2;
			if (unitnum >= 0 && sys_command_cd_qcode (unitnum, qcode_buf)) {
				qcode_valid = 1;
			}
		}

		if (mediacheckcounter <= 0) {
			mediacheckcounter = 312 * 50 * 2;
			int media = sys_command_ismedia (unitnum, 1);
			if (media < 0) {
				write_log (_T("CD32: device unit %d lost\n"), unitnum);
				media = lastmediastate = cdrom_disk = 0;
				mediachanged = 1;
				cdaudiostop_do ();
			} else if (media != lastmediastate) {
				if (!media && lastmediastate > 1) {
					// ignore missing media if statefile restored with cd present
					if (lastmediastate == 2)
						write_log (_T("CD32: CD missing but statefile was stored with CD inserted: faking media present\n"));
					lastmediastate = 3;
				} else {
				  write_log (_T("CD32: media changed = %d\n"), media);
				  lastmediastate = cdrom_disk = media;
				  mediachanged = 1;
				  cdaudiostop_do ();
			  }
		  }
		}

		uae_sem_wait (&akiko_sem);
		sector = cdrom_current_sector;
		for (i = 0; i < SECTOR_BUFFER_SIZE; i++) {
			if (sector_buffer_info_1[i] == 0xff)
				break;
		}
		if (cdrom_data_end > 0 && sector >= 0 &&
			(sector_buffer_sector_1 < 0 || sector < sector_buffer_sector_1 || sector >= sector_buffer_sector_1 + SECTOR_BUFFER_SIZE * 2 / 3 || i != SECTOR_BUFFER_SIZE)) {
				int blocks;
				memset (sector_buffer_info_2, 0, SECTOR_BUFFER_SIZE);
#if AKIKO_DEBUG_IO_CMD
				write_log (_T("filling buffer sector=%d (max=%d)\n"), sector, cdrom_data_end);
#endif
				sector_buffer_sector_2 = sector;
				if (sector + SECTOR_BUFFER_SIZE >= cdrom_data_end)
					blocks = cdrom_data_end - sector;
				else
					blocks = SECTOR_BUFFER_SIZE;
				int ok = sys_command_cd_rawread (unitnum, sector_buffer_2, sector, blocks, 2352);
				if (!ok) {
					int offset = 0;
				  while (offset < SECTOR_BUFFER_SIZE) {
					  int ok = 0;
					  if (sector < cdrom_data_end)
						  ok = sys_command_cd_rawread (unitnum, sector_buffer_2 + offset * 2352, sector, 1, 2352);
					  sector_buffer_info_2[offset] = ok ? 3 : 0;
					  offset++;
					  sector++;
					}
				} else {
					for (int i = 0; i < SECTOR_BUFFER_SIZE; i++)
						sector_buffer_info_2[i] = i < blocks ? 3 : 0;
				}
				tmp1 = sector_buffer_info_1;
				sector_buffer_info_1 = sector_buffer_info_2;
				sector_buffer_info_2 = tmp1;
				tmp2 = sector_buffer_1;
				sector_buffer_1 = sector_buffer_2;
				sector_buffer_2 = tmp2;
				tmp3 = sector_buffer_sector_1;
				sector_buffer_sector_1 = sector_buffer_sector_2;
				sector_buffer_sector_2 = tmp3;
		}
		uae_sem_post (&akiko_sem);
		sleep_millis (10);
	}
	akiko_thread_running = -1;
	return 0;
}

STATIC_INLINE uae_u8 akiko_get_long (uae_u32 v, int offset)
{
	return v >> ((3 - offset) * 8);
}

STATIC_INLINE void akiko_put_long (uae_u32 *p, int offset, int v)
{
	*p &= ~(0xff << ((3 - offset) * 8));
	*p |= v << ((3 - offset) * 8);
}

static uae_u32 REGPARAM3 akiko_lget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 akiko_wget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 akiko_bget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 akiko_lgeti (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 akiko_wgeti (uaecptr) REGPARAM;
static void REGPARAM3 akiko_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 akiko_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 akiko_bput (uaecptr, uae_u32) REGPARAM;

static uae_u32 akiko_bget2 (uaecptr addr, int msg)
{
	uae_u8 v = 0;

	addr &= 0xffff;

	switch (addr)
	{
		/* "CAFE" = Akiko identification.
		* Kickstart ignores Akiko C2P if this ID isn't correct */
	case 0x02:
		return 0xCA;
	case 0x03:
		return 0xFE;
		/* NVRAM */
	case 0x30:
	case 0x31:
	case 0x32:
	case 0x33:
		if (currprefs.cs_cd32nvram)
			v =  akiko_nvram_read (addr - 0x30);
		return v;

		/* C2P */
	case 0x38:
	case 0x39:
	case 0x3a:
	case 0x3b:
		if (currprefs.cs_cd32c2p)
			v = akiko_c2p_read (addr - 0x38);
		return v;
	}


	uae_sem_wait (&akiko_sem);
	switch (addr)
	{
		if (currprefs.cs_cd32cd) {
			/* CDROM control */
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
		v = akiko_get_long (cdrom_intreq, addr - 0x04);
		break;
	case 0x08:
	case 0x09:
	case 0x0a:
	case 0x0b:
		v = akiko_get_long (cdrom_intena, addr - 0x08);
		break;
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
		v = akiko_get_long (cdrom_addressdata, addr - 0x10);
		break;
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
		v = akiko_get_long (cdrom_addressmisc, addr - 0x14);
		break;
	case 0x18:
		v = cdrom_subcodeoffset;
		break;
	case 0x19:
		v = cdcomtxinx;
		break;
	case 0x1a:
		v = cdcomrxinx;
		break;
	case 0x1f:
		v = cdcomrxcmp;
		break;
	case 0x20:
	case 0x21:
		v = akiko_get_long (cdrom_pbx, addr - 0x20 + 2);
		break;
	case 0x24:
	case 0x25:
	case 0x26:
	case 0x27:
		v = akiko_get_long (cdrom_flags, addr - 0x24);
		break;
		} else if (addr < 0x30) {
			break;
		}

	default:
		write_log (_T("akiko_bget: unknown address %08X PC=%08X\n"), addr, M68K_GETPC);
		v = 0;
		break;
	}
	akiko_internal ();
	uae_sem_post (&akiko_sem);
	if (msg && addr < 0x30 && AKIKO_DEBUG_IO)
		write_log (_T("akiko_bget %08X: %08X %02X\n"), M68K_GETPC, addr, v & 0xff);
	return v;
}

static uae_u32 REGPARAM2 akiko_bget (uaecptr addr)
{
#ifdef JIT
	special_mem |= S_READ;
#endif
	return akiko_bget2 (addr, 1);
}

static uae_u32 REGPARAM2 akiko_wget (uaecptr addr)
{
	uae_u16 v;
#ifdef JIT
	special_mem |= S_READ;
#endif
	addr &= 0xffff;
	v = akiko_bget2 (addr + 1, 0);
	v |= akiko_bget2 (addr + 0, 0) << 8;
	if (addr < 0x30 && AKIKO_DEBUG_IO)
		write_log (_T("akiko_wget %08X: %08X %04X\n"), M68K_GETPC, addr, v & 0xffff);
	return v;
}

static uae_u32 REGPARAM2 akiko_lget (uaecptr addr)
{
	uae_u32 v;

#ifdef JIT
	special_mem |= S_READ;
#endif
	addr &= 0xffff;
	v = akiko_bget2 (addr + 3, 0);
	v |= akiko_bget2 (addr + 2, 0) << 8;
	v |= akiko_bget2 (addr + 1, 0) << 16;
	v |= akiko_bget2 (addr + 0, 0) << 24;
	if (addr < 0x30 && (addr != 4 && addr != 8) && AKIKO_DEBUG_IO)
		write_log (_T("akiko_lget %08X: %08X %08X\n"), M68K_GETPC, addr, v);
	return v;
}

static void akiko_bput2 (uaecptr addr, uae_u32 v, int msg)
{
	uae_u32 tmp;

	addr &= 0xffff;
	v &= 0xff;

	if(msg && addr < 0x30 && AKIKO_DEBUG_IO)
		write_log (_T("akiko_bput %08X: %08X=%02X\n"), M68K_GETPC, addr, v & 0xff);

	switch (addr)
	{
	case 0x30:
	case 0x31:
	case 0x32:
	case 0x33:
		if (currprefs.cs_cd32nvram)
			akiko_nvram_write (addr - 0x30, v);
		return;

	case 0x38:
	case 0x39:
	case 0x3a:
	case 0x3b:
		if (currprefs.cs_cd32c2p)
			akiko_c2p_write (addr - 0x38, v);
		return;
	}

	uae_sem_wait (&akiko_sem);
	switch (addr)
	{
		if (currprefs.cs_cd32cd) {
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
		akiko_put_long (&cdrom_intreq, addr - 0x04, v);
		break;
	case 0x08:
	case 0x09:
	case 0x0a:
	case 0x0b:
		akiko_put_long (&cdrom_intena, addr - 0x08, v);
		if (addr == 8)
			cdrom_intreq &= cdrom_intena;
		break;
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
		akiko_put_long (&cdrom_addressdata, addr - 0x10, v);
		cdrom_addressdata &= 0x00fff000;
		break;
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
		akiko_put_long (&cdrom_addressmisc, addr - 0x14, v);
		cdrom_addressmisc &= 0x00fffc00;
		subcode_address = cdrom_addressmisc | 0x100;
		cdrx_address = cdrom_addressmisc;
		cdtx_address = cdrom_addressmisc | 0x200;
		break;
	case 0x18:
		cdrom_intreq &= ~CDINTERRUPT_SUBCODE;
		break;
	case 0x1d:
		cdrom_intreq &= ~CDINTERRUPT_TXDMADONE;
		cdcomtxcmp = v;
		cdrom_tx_dma_delay = 5;
		break;
	case 0x1f:
		cdrom_intreq &= ~CDINTERRUPT_RXDMADONE;
		cdcomrxcmp = v;
		break;
	case 0x20:
	case 0x21:
		tmp = cdrom_pbx;
		akiko_put_long (&cdrom_pbx, addr - 0x20 + 2, v);
		cdrom_pbx |= tmp;
		cdrom_pbx &= 0xffff;
		cdrom_intreq &= ~CDINTERRUPT_PBX;
		break;
	case 0x24:
	case 0x25:
	case 0x26:
	case 0x27:
		tmp = cdrom_flags;
		akiko_put_long (&cdrom_flags, addr - 0x24, v);
		if ((cdrom_flags & CDFLAG_ENABLE) && !(tmp & CDFLAG_ENABLE))
			cdrom_sector_counter = 0;
		if (!(cdrom_flags & CDFLAG_PBX) && (tmp & CDFLAG_PBX))
			cdrom_pbx = 0;
		if ((cdrom_flags & CDFLAG_SUBCODE) && !(tmp & CDFLAG_SUBCODE)) {
			uae_sem_wait (&sub_sem);
			memset (subcodebufferinuse, 0, sizeof subcodebufferinuse);
			subcodebufferoffset = subcodebufferoffsetw = 0;
			uae_sem_post (&sub_sem);
		}
		break;
		} else if (addr < 0x30) {
			break;
		}

	default:
		write_log (_T("akiko_bput: unknown address %08X=%02X PC=%08X\n"), addr, v & 0xff, M68K_GETPC);
		break;
	}
	akiko_internal ();
	uae_sem_post (&akiko_sem);
}

static void REGPARAM2 akiko_bput (uaecptr addr, uae_u32 v)
{
#ifdef JIT
	special_mem |= S_WRITE;
#endif
	akiko_bput2 (addr, v, 1);
}

static void REGPARAM2 akiko_wput (uaecptr addr, uae_u32 v)
{
#ifdef JIT
	special_mem |= S_WRITE;
#endif
	addr &= 0xfff;
	if((addr < 0x30 && AKIKO_DEBUG_IO))
		write_log (_T("akiko_wput %08X: %08X=%04X\n"), M68K_GETPC, addr, v & 0xffff);
	akiko_bput2 (addr + 1, v & 0xff, 0);
	akiko_bput2 (addr + 0, v >> 8, 0);
}

static void REGPARAM2 akiko_lput (uaecptr addr, uae_u32 v)
{
#ifdef JIT
	special_mem |= S_WRITE;
#endif
	addr &= 0xffff;
	if(addr < 0x30 && AKIKO_DEBUG_IO)
		write_log (_T("akiko_lput %08X: %08X=%08X\n"), M68K_GETPC, addr, v);
	akiko_bput2 (addr + 3, (v >> 0) & 0xff, 0);
	akiko_bput2 (addr + 2, (v >> 8) & 0xff, 0);
	akiko_bput2 (addr + 1, (v >> 16) & 0xff, 0);
	akiko_bput2 (addr + 0, (v >> 24) & 0xff, 0);
}

addrbank akiko_bank = {
	akiko_lget, akiko_wget, akiko_bget,
	akiko_lput, akiko_wput, akiko_bput,
	default_xlate, default_check, NULL, _T("Akiko"),
	dummy_lgeti, dummy_wgeti, ABFLAG_IO
};

static const uae_u8 patchdata[]={0x0c,0x82,0x00,0x00,0x03,0xe8,0x64,0x00,0x00,0x46};
static void patchrom (void)
{
    int i;
	if (currprefs.cpu_model > 68020 || currprefs.cachesize || currprefs.m68k_speed != 0) {
		uae_u8 *p = extendedkickmem_bank.baseaddr;
		for (i = 0; i < 524288 - sizeof (patchdata); i++) {
			if (!memcmp (p + i, patchdata, sizeof(patchdata))) {
				protect_roms (false);
				p[i + 6] = 0x4e;
				p[i + 7] = 0x71;
				p[i + 8] = 0x4e;
				p[i + 9] = 0x71;
				protect_roms (true);
				write_log (_T("extended rom delay loop patched at 0x%p\n"), i + 6 + 0xe00000);
				return;
			}
		}
		write_log (_T("couldn't patch extended rom\n"));
	}
}

static void akiko_cdrom_free (void)
{
	sys_cddev_close ();
	xfree (sector_buffer_1);
	xfree (sector_buffer_2);
	xfree (sector_buffer_info_1);
	xfree (sector_buffer_info_2);
	sector_buffer_1 = 0;
	sector_buffer_2 = 0;
	sector_buffer_info_1 = 0;
	sector_buffer_info_2 = 0;
}

void akiko_reset (void)
{
	cdaudiostop_do ();
	nvram_read ();
	state = I2C_WAIT;
	bitcounter = -1;
	direction = -1;

	cdrom_speed = 1;
	cdrom_current_sector = -1;
	if (!savestate_state) {
		cdcomtxinx = 0;
		cdcomrxinx = 0;
		cdcomtxcmp = 0;
		lastmediastate = -1;
	}
	cdrom_led = 0;
	cdrom_receive_length = 0;
	cdrom_receive_offset = 0;
	cd_initialized = 0;

	if (akiko_thread_running > 0) {
		cdaudiostop ();
		akiko_thread_running = 0;
		while(akiko_thread_running == 0)
			sleep_millis (10);
		akiko_thread_running = 0;
	}
	akiko_cdrom_free ();
	mediacheckcounter = 0;
	akiko_inited = false;
}


void akiko_free (void)
{
	akiko_reset ();
	akiko_cdrom_free ();
}

int akiko_init (void)
{
	if (!currprefs.cs_cd32cd)
		return 0;
	akiko_free ();
	akiko_precalculate ();
	unitnum = -1;
	sys_cddev_open ();
	sector_buffer_1 = xmalloc (uae_u8, SECTOR_BUFFER_SIZE * 2352);
	sector_buffer_2 = xmalloc (uae_u8, SECTOR_BUFFER_SIZE * 2352);
	sector_buffer_info_1 = xmalloc (uae_u8, SECTOR_BUFFER_SIZE);
	sector_buffer_info_2 = xmalloc (uae_u8, SECTOR_BUFFER_SIZE);
	sector_buffer_sector_1 = -1;
	sector_buffer_sector_2 = -1;
	uae_sem_init (&akiko_sem, 0, 1);
	uae_sem_init (&sub_sem, 0, 1);
	if (!savestate_state) {
		cdrom_playing = cdrom_paused = 0;
		cdrom_data_offset = -1;
	}
	patchrom ();
	if (!akiko_thread_running) {
		akiko_thread_running = 1;
		init_comm_pipe (&requests, 100, 1);
		uae_start_thread (_T("akiko"), akiko_thread, 0, NULL);
	}
	gui_flicker_led (LED_HD, 0, -1);
	akiko_inited = true;
	return 1;
}

#ifdef SAVESTATE

uae_u8 *save_akiko (int *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;
	int i;

	if (!currprefs.cs_cd32cd)
		return NULL;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 1000);
	save_u16 (0);
	save_u16 (0xCAFE);
	save_u32 (cdrom_intreq);
	save_u32 (cdrom_intena);
	save_u32 (0);
	save_u32 (cdrom_addressdata);
	save_u32 (cdrom_addressmisc);
	save_u8 (cdrom_subcodeoffset);
	save_u8 (cdcomtxinx);
	save_u8 (cdcomrxinx);
	save_u8 (0);
	save_u8 (0);
	save_u8 (cdcomtxcmp);
	save_u8 (0);
	save_u8 (cdcomrxcmp);
	save_u16 ((uae_u16)cdrom_pbx);
	save_u16 (0);
	save_u32 (cdrom_flags);
	save_u32 (0);
	save_u32 (0);
	save_u32 ((scl_dir ? 0x8000 : 0) | (sda_dir ? 0x4000 : 0));
	save_u32 (0);
	save_u32 (0);

	for (i = 0; i < 8; i++)
		save_u32 (akiko_buffer[i]);
	save_u8 ((uae_u8)akiko_read_offset);
	save_u8 ((uae_u8)akiko_write_offset);

	save_u32 ((cdrom_playing ? 1 : 0) | (cdrom_paused ? 2 : 0) | (cdrom_disk ? 4 : 0));
	if (cdrom_playing)
		cd_qcode (0);
	save_u32 (lsn2msf (last_play_pos));
	save_u32 (lsn2msf (last_play_end));
	save_u8 ((uae_u8)cdrom_toc_counter);

	save_u8 (cdrom_speed);
	save_u8 (cdrom_current_sector);

	save_u32 (cdrom_toc_crc);
	save_u8 (cdrom_toc_cd_buffer.points);
	save_u32 (cdrom_toc_cd_buffer.lastaddress);

	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *restore_akiko (uae_u8 *src)
{
	uae_u32 v;
	int i;

	akiko_free ();
	if (!currprefs.cs_cd32cd) {
		changed_prefs.cs_cd32c2p = changed_prefs.cs_cd32cd = changed_prefs.cs_cd32nvram = true;
		currprefs.cs_cd32c2p = currprefs.cs_cd32cd = currprefs.cs_cd32nvram = true;
		akiko_init ();
	}

	restore_u16 ();
	restore_u16 ();
	cdrom_intreq = restore_u32 ();
	cdrom_intena = restore_u32 ();
	restore_u32 ();
	cdrom_addressdata = restore_u32 ();
	cdrom_addressmisc = restore_u32 ();
	subcode_address = cdrom_addressmisc | 0x100;
	cdrx_address = cdrom_addressmisc;
	cdtx_address = cdrom_addressmisc | 0x200;
	cdrom_subcodeoffset = restore_u8 ();
	cdcomtxinx = restore_u8 ();
	cdcomrxinx = restore_u8 ();
	restore_u8 ();
	restore_u8 ();
	cdcomtxcmp = restore_u8 ();
	restore_u8 ();
	cdcomrxcmp = restore_u8 ();
	cdrom_pbx = restore_u16 ();
	restore_u16 ();
	cdrom_flags = restore_u32 ();
	restore_u32 ();
	restore_u32 ();
	v = restore_u32 ();
	scl_dir = (v & 0x8000) ? 1 : 0;
	sda_dir = (v & 0x4000) ? 1 : 0;
	restore_u32 ();
	restore_u32 ();

	for (i = 0; i < 8; i++)
		akiko_buffer[i] = restore_u32 ();
	akiko_read_offset = restore_u8 ();
	akiko_write_offset = restore_u8 ();

	cdrom_playing = cdrom_paused = cdrom_disk = 0;
	v = restore_u32 ();
	if (v & 1)
		cdrom_playing = 1;
	if (v & 2)
		cdrom_paused = 1;
	if (v & 4)
		cdrom_disk = 1;
	lastmediastate = cdrom_disk ? 2 : 0;

	last_play_pos = msf2lsn (restore_u32 ());
	last_play_end = msf2lsn (restore_u32 ());
	cdrom_toc_counter = (uae_s8)restore_u8 ();
	cdrom_speed = restore_u8 ();
	cdrom_current_sector = (uae_s8)restore_u8 ();

	restore_u32 ();
	restore_u8 ();
	restore_u32 ();

	return src;
}

void restore_akiko_finish (void)
{
	if (!currprefs.cs_cd32cd)
		return;
	sys_cddev_close ();
	akiko_init ();
	akiko_c2p_do ();
	get_cdrom_toc ();
	write_comm_pipe_u32 (&requests, 0x0102, 1); // pause
	write_comm_pipe_u32 (&requests, 0x0104, 1); // stop
	write_comm_pipe_u32 (&requests, 0x0103, 1); // unpause
	if (cdrom_playing && isaudiotrack (last_play_pos)) {
		write_comm_pipe_u32 (&requests, 0x0103, 1); // unpause
		write_comm_pipe_u32 (&requests, 0x0110, 0); // play
		write_comm_pipe_u32 (&requests, last_play_pos, 0);
		write_comm_pipe_u32 (&requests, last_play_end, 0);
		write_comm_pipe_u32 (&requests, 0, 1);
	}
	cd_initialized = 1;
}

#endif

void akiko_mute (int muted)
{
	cdrom_muted = muted;
	if (unitnum >= 0)
		write_comm_pipe_u32 (&requests, 0x0105, 1);
}

