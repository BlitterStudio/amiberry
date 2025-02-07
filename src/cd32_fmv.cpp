/*
* UAE - The Un*x Amiga Emulator
*
* CD32 FMV cartridge
*
* Copyright 2008-2014 Toni Wilen
*
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "rommgr.h"
#include "custom.h"
#include "newcpu.h"
#include "zfile.h"
#include "cd32_fmv.h"
#include "uae.h"
#include "debug.h"
#include "custom.h"
#include "audio.h"
#include "devices.h"
#include "threaddep/thread.h"

#include "cda_play.h"
#include "archivers/mp2/kjmp2.h"

#ifdef USE_LIBMPEG2
#if (!defined _WIN32 && !defined ANDROID)
extern "C" {
#include "mpeg2dec/mpeg2.h"
#include "mpeg2dec/mpeg2convert.h"
}
#else
#include "mpeg2.h"
#include "mpeg2convert.h"
#endif

#define FMV_DEBUG 0
static int fmv_audio_debug = 0;
static int fmv_video_debug = 0;
#define DUMP_VIDEO 0

/*
 0x200000 - 0x23FFFF ROM (256k)
 0x240000 io/status (word)
 0x2500xx L64111 audio decoder (word registers)
 0x260000 CL450 data port (word)
 0x2700xx CL450 video decoder (word registers)
 0x280000 - 0x2FFFFF RAM (512k)
*/

#define ROM_BASE		0x000000
#define IO_BASE			0x040000
#define L64111_BASE		0x050000
#define CL450_DATA		0x060000
#define CL450_BASE		0x070000
#define RAM_BASE		0x080000

#define BANK_MASK		0x0F0000

// IO_BASE bits (read)
#define IO_CL450_IRQ			0x8000 // CL450INT_
#define IO_L64111_IRQ			0x4000 // L64111INT_
#define IO_CL450_FIFO_STATUS	0x0800 // CL450CFLEVEL
#define IO_L64111_FALF			0x0400 // L64111FALF
#define IO_L64111_FALE			0x0400 // L64111FALE
#define IO_L64111_FEMP			0x0400 // L64111FEMP

// IO_BASE bits (write)
#define IO_CL450_VIDEO			0x4000
#define IO_UNK2					0x2000
#define IO_UNK3					0x1000
// above three are set/cleared by ROM code
#define IO_L64111_MUTE			0x0200

// L64111 registers
#define A_DATA			 0
#define A_CONTROL1		 1
#define A_CONTROL2		 2
#define A_CONTROL3		 3
#define A_INT1			 4
#define A_INT2			 5
#define A_TCR			 6
#define A_TORH			 7
#define A_TORL			 8
#define A_PARAM1		 9
#define A_PARAM2		10
#define A_PARAM3		11
#define A_PRESENT1		12
#define A_PRESENT2		13
#define A_PRESENT3		14
#define A_PRESENT4		15
#define A_PRESENT5		16
#define A_FIFO			17
#define A_CB_STATUS		18
#define A_CB_WRITE		19
#define A_CB_READ		20
// L641111 status register 1
#define ANC_DATA_VALID		0x80
#define ANC_DATA_FIFO_OVFL	0x40
#define ANC_DATA_FIO_HFF	0x10
#define ERR_BUF_OVFL		0x08
// L641111 status register 2
#define SYNTAX_ERR_DET	0x40
#define PTS_AVAILABLE	0x20
#define SYNC_AUD		0x10
#define SYNC_SYS		0x08
#define FRAME_DETECT_IN	0x04
#define CRC_ERR			0x02
#define NEWLAST_FRAME	0x01
#define NEW_FRAME_S		0x01
#define LAST_FRAME_S	0x80
// status register 2 mask bits
#define NEW_FRAME		0x02
#define LAST_FRAME		0x01

// CL450 direct access registers
#define CMEM_control	0x80
#define CMEM_data		0x02
#define CMEM_dmactrl	0x84
#define CMEM_status		0x82
#define CPU_control		0x20
#define CPU_iaddr		0x3e
#define CPU_imem		0x42
#define CPU_int			0x54
#define CPU_intenb		0x26
#define CPU_pc			0x22
#define CPU_taddr		0x38
#define CPU_tmem		0x46
#define DRAM_refcnt		0xac
#define HOST_control	0x90
#define HOST_intvecr	0x9c
#define HOST_intvecw	0x98
#define HOST_newcmd		0x56
#define HOST_raddr		0x88
#define HOST_rdata		0x8c
#define HOST_scr0		0x92
#define HOST_scr1		0x94
#define HOST_scr2		0x96
#define VID_control		0xec
#define VID_regdata		0xee
#define VID_chrom		0x0a
#define VID_y			0x00
// CL450 indirect access registers
#define VID_sela		0x0
#define VID_selactive	0x8
#define VID_selaux		0xc
#define VID_selb		0x1
#define VID_selbor		0x9
#define VID_selGB		0xb
#define VID_selmode		0x7
#define VID_selR		0xA
// CL450 commands
#define CL_SetBlank				0x030f
#define CL_SetBorder			0x0407
#define CL_SetColorMode			0x0111
#define CL_SetInterruptMask		0x0104
#define CL_SetThresHold			0x0103
#define CL_SetVideoFormat		0x0105
#define CL_SetWindow			0x0406
#define CL_DisplayStill			0x000c
#define CL_Pause				0x000e
#define CL_Play					0x000d
#define CL_Scan					0x000a
#define CL_SingleStep			0x000b
#define CL_SlowMotion			0x0109
#define CL_AccessSCR			0x8312
#define CL_FlushBitStream		0x8102
#define CL_InquireBufferFullness 0x8001
#define CL_NewPacket			0x0408
#define CL_Reset				0x8000
// CL450 interrupts
#define CL_INT_RDY		(1 << 10)
#define CL_INT_END_D	(1 <<  5)
#define CL_INT_ERR		(1 <<  0)
#define CL_INT_PIC_D	(1 <<  6)
#define CL_INT_SEQ_D	(1 <<  9)
#define CL_INT_SCN		(1 << 11)
#define CL_INT_UND		(1 <<  8)
#define CL_INT_END_V	(1 <<  4)
#define CL_INT_GOP		(1 <<  2)
#define CL_INT_PIC_V	(1 <<  1)
#define CL_INT_SEQ_V	(1 <<  3)
// CL450 DRAM resident variables (WORD address)
#define CL_DRAM_SEQ_SEM			0x10
#define CL_DRAM_SEQ_CONTROL		0x11
#define CL_DRAM_H_SIZE			0x12
#define CL_DRAM_V_SIZE			0x13
#define CL_DRAM_PICTURE_RATE	0x14
#define CL_DRAM_FLAGS			0x15
#define CL_DRAM_PIC_SEM			0x16
#define CL_DRAM_TIME_CODE_0		0x17
#define CL_DRAM_TIME_CODE_1		0x18
#define CL_DRAM_TEMPORAL_REF	0x19
#define CL_DRAM_VER				0xa0 // 0x0200
#define CL_DRAM_PID				0xa1 // 0x0002
#define CL_DRAM_CPU_PC			0xa2

static uae_u8 *audioram;
static const int fmv_rom_size = 262144;
static const int fmv_ram_size = 524288;
static const uaecptr fmv_start = 0x00200000;
static const int fmv_board_size = 1048576;

#define CL_HMEM_INT_STATUS		0x0a
#define CL450_MPEG_BUFFER		0x10000
#define CL450_MPEG_BUFFER_SIZE  65536
#define CL450_MPEG_DECODE_BUFFER  (CL450_MPEG_BUFFER + CL450_MPEG_BUFFER_SIZE)
#define CL450_MPEG_DECODE_BUFFER_SIZE (fmv_ram_size - CL450_MPEG_DECODE_BUFFER)

#define CL450_VIDEO_BUFFERS 8
#define CL450_VIDEO_BUFFER_SIZE (352 * 288 * 4)

static uae_u16 cl450_regs[256];
static float cl450_scr;
#define CL450_IMEM_WORDS (2 * 512)
#define CL450_TMEM_WORDS 128
#define CL450_HMEM_WORDS 16
static uae_u16 cl450_imem[CL450_IMEM_WORDS];
static uae_u16 cl450_tmem[CL450_TMEM_WORDS];
static uae_u16 cl450_hmem[CL450_HMEM_WORDS];
#define CL450_VID_REGS 16
static uae_u16 cl450_vid[CL450_VID_REGS];
static int cl450_play;
static int cl450_blank;
static uae_u32 cl450_border_color;
static uae_u16 cl450_interruptmask;
static uae_u16 cl450_pending_interrupts;
static uae_u16 cl450_threshold;
static int cl450_buffer_offset;
static int cl450_buffer_empty_cnt;
static int libmpeg_offset;
static uae_sem_t play_sem;
static volatile bool fmv_bufon[2];
static float fmv_syncadjust;
static struct cd_audio_state cas;

struct cl450_videoram
{
	int width;
	int height;
	int depth;
	uae_u8 data[CL450_VIDEO_BUFFER_SIZE];
};
static struct cl450_videoram *videoram;

static bool cl450_newpacket_mode;
// Real CL450 has command buffer but we don't need to care,
// non-NewPacket commands can be emulated immediately.
#define CL450_NEWPACKET_BUFFER_SIZE 32
struct cl450_newpacket
{
	uae_u16 length;
	uae_u64 pts;
	bool pts_valid;
};
static struct cl450_newpacket cl450_newpacket_buffer[CL450_NEWPACKET_BUFFER_SIZE];
static int cl450_newpacket_offset_write;
static int cl450_newpacket_offset_read;

static int cl450_frame_rate, cl450_frame_pixbytes;
static int cl450_frame_width, cl450_frame_height;
static int cl450_video_hsync_wait;
static int cl450_videoram_read;
static int cl450_videoram_write;
static int cl450_videoram_cnt;
static int cl450_frame_cnt;

static uae_u16 l64111_regs[32];
static uae_u16 l64111intmask[2], l64111intstatus[2];
#define L64111_CHANNEL_BUFFERS 128

static uae_u16 mpeg_io_reg;

static mpeg2dec_t *mpeg_decoder;
static const mpeg2_info_t *mpeg_info;

#if FMV_DEBUG
static int isdebug(uaecptr addr)
{
#if FMV_DEBUG > 2
	if (M68K_GETPC >= 0x200100)
		return 1;
	return 0;
#endif
#if FMV_DEBUG == 2
	if (M68K_GETPC >= 0x200100 && (addr & fmv_mask) >= VRAM_BASE)
		return 1;
	return 0;
#endif
	return 0;
}
#endif

static void do_irq(void)
{
	safe_interrupt_set(IRQ_SOURCE_CD32CDTV, 1, false);
}

static bool l64111_checkint(bool enabled)
{
	bool irq = false;
	if (l64111intstatus[0] & l64111intmask[0])
		irq = true;
	if (((l64111intstatus[1] << 1) | (l64111intstatus[1] >> 7)) & l64111intmask[1])
		irq = true;
	if (irq && enabled)
		do_irq();
	return irq;
}

static bool cl450_checkint(bool enabled)
{
	bool irq = false;
	if (!(cl450_regs[CPU_control] & 1))
		return false;
	if (!(cl450_regs[HOST_control] & 0x80))
		irq = true;
	if (irq && enabled)
		do_irq();
	return irq;
}

DECLARE_MEMORY_FUNCTIONS(fmv);
static addrbank fmv_bank = {
	fmv_lget, fmv_wget, fmv_bget,
	fmv_lput, fmv_wput, fmv_bput,
	default_xlate, default_check, NULL, NULL, _T("CD32 FMV IO"),
	fmv_lget, fmv_wget,
	ABFLAG_IO, S_READ, S_WRITE
};

DECLARE_MEMORY_FUNCTIONS(fmv_rom);
static addrbank fmv_rom_bank = {
	fmv_rom_lget, fmv_rom_wget, fmv_rom_bget,
	fmv_rom_lput, fmv_rom_wput, fmv_rom_bput,
	fmv_rom_xlate, fmv_rom_check, NULL, _T("*"), _T("CD32 FMV ROM"),
	fmv_rom_lget, fmv_rom_wget,
	ABFLAG_ROM, S_READ, S_WRITE
};

DECLARE_MEMORY_FUNCTIONS(fmv_ram);
static addrbank fmv_ram_bank = {
	fmv_ram_lget, fmv_ram_wget, fmv_ram_bget,
	fmv_ram_lput, fmv_ram_wput, fmv_ram_bput,
	fmv_ram_xlate, fmv_ram_check, NULL, _T("*"), _T("CD32 FMV RAM"),
	fmv_ram_lget, fmv_ram_wget,
	ABFLAG_RAM, S_READ, S_WRITE
};

MEMORY_FUNCTIONS(fmv_rom);
MEMORY_FUNCTIONS(fmv_ram);

static void rethink_cd32fmv(void)
{
	if (!fmv_ram_bank.baseaddr)
		return;
	cl450_checkint(true);
	l64111_checkint(true);
}

#define L64111_FIFO_LOOKUP 96
#define L64111_FIFO_BYTES 128
static uae_u8 l64111_fifo[L64111_FIFO_BYTES];
static int l64111_fifo_cnt;
static int audio_frame_cnt, audio_frame_size;
static int audio_frame_detect, audio_head_detect;
static int l64111_cb_mask;
#define L64111_CHANNEL_BUFFER_SIZE 2048

static kjmp2_context_t mp2;
#define PCM_SECTORS 4
static cda_audio *cda;
static int audio_data_remaining;
static int audio_skip_size;

struct zfile *fdump;

struct fmv_pcmaudio
{
	bool ready;
	signed short pcm[KJMP2_SAMPLES_PER_FRAME * 2];
};
static struct fmv_pcmaudio *pcmaudio;

static void l64111_set_status(int num, uae_u16 mask)
{
	num--;
	l64111intstatus[num] |= mask;
	l64111_checkint(true);
}

static void l64111_setvolume(void)
{
	int volume = 32768;
	if (l64111_regs[A_CONTROL2] & (1 << 5) || (mpeg_io_reg & IO_L64111_MUTE))
		volume = 0;
	if (!pcmaudio)
		return;
	write_log(_T("L64111 mute %d\n"), volume ? 0 : 1);
	if (cda) {
		audio_cda_volume(&cas, volume, volume);
	}
}

static int l64111_get_frame(uae_u8 *data, int remaining)
{
	int size, offset;
	uae_u8 *memdata;

	if (!audio_frame_size || !audio_data_remaining || !remaining)
		return 0;
	size = audio_frame_size - audio_frame_cnt > remaining ? remaining : audio_frame_size - audio_frame_cnt;
	if (audio_data_remaining >= 0 && size > audio_data_remaining)
		size = audio_data_remaining;
	offset = l64111_regs[A_CB_WRITE] & l64111_cb_mask;
	memdata = audioram + offset * L64111_CHANNEL_BUFFER_SIZE;
	memcpy(memdata + audio_frame_cnt, data, size);
	audio_frame_cnt += size;
	if (audio_data_remaining >= 0)
		audio_data_remaining -= size;
	if (audio_frame_cnt == audio_frame_size) {
		int bytes;

		if (pcmaudio[offset].ready) {
			write_log(_T("L64111 buffer overflow!\n"));
		}
#if 0
		if (!fdump)
			fdump = zfile_fopen(_T("c:\\temp\\1.mp2"), _T("wb"));

		zfile_fwrite(memdata, 1, audio_frame_size, fdump);
#endif
		bytes = kjmp2_decode_frame(&mp2, memdata, pcmaudio[offset].pcm);
		if (bytes < 4 || bytes > KJMP2_MAX_FRAME_SIZE) {
			write_log(_T("mp2 decoding error\n"));
			memset(pcmaudio[offset].pcm, 0, KJMP2_SAMPLES_PER_FRAME * 4);
		}
		pcmaudio[offset].ready = true;

		audio_frame_size = 0;
		audio_frame_cnt = 0;
		l64111_set_status(2, NEW_FRAME_S);
		//write_log(_T("Audio frame %d (%d %d)\n"), offset, l64111_regs[A_CB_STATUS], bytes);
		offset++;
		l64111_regs[A_CB_WRITE] = offset & l64111_cb_mask;
		l64111_regs[A_CB_STATUS]++;

	}
	return size;
}

static const int mpa_bitrates[] = {
	-1, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, -1,
	-1, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, -1,
	-1, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1,
	-1, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, -1,
	-1, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, -1
};
static const int mpa_frequencies[] = {
	44100, 48000, 32000, 0,
	22050, 24000, 16000, 0,
	11025, 12000, 8000, 0
};
static const int mpa_samplesperframe[] = {
	384, 384, 384,
	1152, 1152, 1152,
	1152, 576, 576
};

static bool parse_mp2_frame(uae_u8 *header)
{
	int ver, layer, bitrate, freq, padding, bitindex, iscrc;
	int samplerate, bitrateidx, channelmode;
	int isstereo;

	audio_frame_cnt = 0;
	audio_frame_size = 0;

	ver = (header[1] >> 3) & 3;
	if (ver != 3) // not MPEG-1
		return false;
	ver = 0;
	layer = 4 - ((header[1] >> 1) & 3);
	if (layer != 2) // not layer-2
		return false;
	iscrc = ((header[1] >> 0) & 1) ? 0 : 2;
	bitrateidx = (header[2] >> 4) & 15;
	if (bitrateidx == 0 || bitrateidx == 15)
		return false; // invalid value
	freq = mpa_frequencies[(header[2] >> 2) & 3];
	if (!freq)
		return false; // invalid value
	channelmode = (header[3] >> 6) & 3;
	isstereo = channelmode != 3;
	if (ver == 0) {
		bitindex = layer - 1;
	} else {
		if (layer == 1)
			bitindex = 3;
		else
			bitindex = 4;
	}
	bitrate = mpa_bitrates[bitindex * 16 + bitrateidx] * 1000;
	if (bitrate <= 0) // invalid value
		return false;
	padding = (header[2] >> 1) & 1;
	samplerate = mpa_samplesperframe[(layer - 1) * 3 + ver];
	audio_frame_size = ((samplerate / 8 * bitrate) / freq) + padding;
	audio_frame_cnt = 0;
	l64111_cb_mask = layer == 2 ? 63 : 127;

	audio_frame_detect++;

	l64111_regs[A_PARAM1] = ((header[1] << 4) | (header[2] >> 4)) & 0xff;
	l64111_regs[A_PARAM2] = ((header[2] << 4) | (header[3] >> 4)) & 0xff;
	l64111_regs[A_PARAM3] = ((header[3] << 4)) & 0xff;

	l64111_set_status(2, FRAME_DETECT_IN | (audio_frame_detect == 3 ? SYNC_AUD : 0));

	return true;
}

static void l64111_init(void)
{
	audio_head_detect = 0;
	audio_frame_detect = 0;
	audio_data_remaining = 0;
	audio_skip_size = 0;
	audio_frame_cnt = 0;
	audio_frame_size = 0;
	l64111_fifo_cnt = 0;
}

static void l64111_reset(void)
{
	memset(l64111_regs, 0, sizeof l64111_regs);
	l64111intmask[0] = l64111intmask[1] = 0;
	l64111intstatus[0] = l64111intstatus[1] = 0;
	l64111_regs[A_CONTROL3] = 1 << 7; // AUDIO STREAM_ID_IGNORE=1
	l64111_init();
	l64111_setvolume();
	if (pcmaudio) {
		memset(pcmaudio, 0, sizeof(struct fmv_pcmaudio) * L64111_CHANNEL_BUFFERS);
		write_log(_T("L64111 reset\n"));
	}
}

static uae_u8 *parse_audio_header(uae_u8 *p)
{
	bool ptsdts;
	int cnt;
	uae_u16 psize;
	uae_u64 pts = 0;

	p += 4;
	psize = (p[0] << 8) | p[1];
	//write_log(_T("audio stream header size %d\n"), psize);
	p += 2;
	if (audio_head_detect < 3) {
		audio_head_detect++;
		if (audio_head_detect == 3)
			l64111_set_status(2, SYNC_SYS);
	}
	cnt = 16;
	while (p[0] == 0xff) {
		cnt--;
		if (cnt < 0)
			return p;
		p++;
		psize--;
	}
	if (p[0] == 0x0f) {
		p++;
		psize--;
	} else {
		if ((p[0] & 0xc0) == 0x40) {
			// STD
			p += 2;
			psize -= 2;
		}
		ptsdts = false;
		if ((p[0] & 0xf0) == 0x20 || (p[0] & 0xf0) == 0x30) {
			if ((p[0] & 0xf0) == 0x30)
				ptsdts = true;
			// PTS
			pts = (((uae_u64)(p[0] >> 1) & 7) << 30);
			pts |= ((p[1] >> 0) << 22);
			pts |= ((p[2] >> 1) << 15);
			pts |= ((p[3] >> 0) << 7);
			pts |= ((p[4] >> 1) << 0);
			p += 5;
			psize -= 5;
			if (audio_head_detect >= 3) {
				l64111_regs[A_PRESENT1] = (uae_u16)(pts >> 0);
				l64111_regs[A_PRESENT2] = (uae_u16)(pts >> 8);
				l64111_regs[A_PRESENT3] = (uae_u16)(pts >> 16);
				l64111_regs[A_PRESENT4] = (uae_u16)(pts >> 24);
				l64111_regs[A_PRESENT5] = (uae_u16)(pts >> 32);
				//write_log(_T("audio PTS %09llx SCR %09llx\n"), pts, (uae_u64)cl450_scr);
				l64111_set_status(2, PTS_AVAILABLE);
			}
		}
		if (ptsdts && (p[0] & 0xf0) == 0x10) {
			// DTS
			p += 5;
			psize -= 5;
		}
	}
	audio_data_remaining = psize;
	return p;
}

static void l64111_parse(void)
{
	bool audio_only = (l64111_regs[A_CONTROL2] & 0x08) != 0;
	uae_u8 *p = l64111_fifo;

	if (!(l64111_regs[A_CONTROL1] & 1)) { // START bit set?
		l64111_fifo_cnt = 0;
		return;
	}

	if (audio_only) {
		audio_data_remaining = -1;
		audio_skip_size = 0;
	}

	if (audio_skip_size) {
		int size = audio_skip_size > L64111_FIFO_BYTES ? L64111_FIFO_BYTES : audio_skip_size;
		p += size;
		audio_skip_size -= size;
	}

	if (audio_frame_size && audio_data_remaining)
		p += l64111_get_frame(p, L64111_FIFO_BYTES - addrdiff(p, l64111_fifo));

	while (p - l64111_fifo < L64111_FIFO_LOOKUP || (addrdiff(p, l64111_fifo) & 1)) {
		uae_u8 *op = p;
		int size = 0;

		if (!audio_only) {
			// check system stream packets
			uae_u8 marker = p[3];
			if (p[0] == 0 && p[1] == 0 && p[2] == 1 && (marker & 0x80)) {
				int size = (p[4] << 8) | p[5];
				if (marker >= 0xc0 && marker <= 0xdf) {
					// audio stream 0 to 31
					bool ignore_stream_id = (l64111_regs[A_CONTROL3] & 0x80) != 0;
					if (ignore_stream_id || (marker - 0xc0) == (l64111_regs[A_CONTROL3] & 31))
						p = parse_audio_header(p);
					else
						p += 4;
				} else if (marker == 0xba) {
					//write_log(_T("L64111: Pack header\n"));
					p += 12;
				} else if (marker == 0xbb) {
					write_log(_T("L64111: System header, size %d\n"), size);
					p += 6;
					audio_skip_size = size;
				} else if (marker == 0xbe) {
					//write_log(_T("L64111: Padding packet size %d\n"), size);
					p += 6;
					audio_skip_size = size;
				} else {
					write_log(_T("L64111: Packet %02X, size %d\n"), marker, size);
					p += 6;
				}
			}
		}

		if (audio_skip_size) {
			int size = audio_skip_size > L64111_FIFO_BYTES - addrdiff(p, l64111_fifo) ? L64111_FIFO_BYTES - addrdiff(p, l64111_fifo) : audio_skip_size;
			p += size;
			audio_skip_size -= size;
		}
		if (L64111_FIFO_BYTES - (p - l64111_fifo) > 0 && audio_data_remaining) {
			if (audio_frame_size) {
				p += l64111_get_frame(p, L64111_FIFO_BYTES - addrdiff(p, l64111_fifo));
			} else if (p[0] == 0xff && (p[1] & (0x80 | 0x40 | 0x20)) == (0x80 | 0x40 | 0x20)) {
				if (parse_mp2_frame(p))
					p += l64111_get_frame(p, L64111_FIFO_BYTES - addrdiff(p, l64111_fifo));
			}
		}

		if (p == op) {
			p++;
			if (audio_data_remaining > 0)
				audio_data_remaining--;
		}
	}
	l64111_fifo_cnt = L64111_FIFO_BYTES - addrdiff(p, l64111_fifo);
	if (l64111_fifo_cnt < 0)
		l64111_fifo_cnt = 0;
	if (l64111_fifo_cnt > 0)
		memmove(l64111_fifo, p, l64111_fifo_cnt);
}

static uae_u16 l64111_wget (uaecptr addr)
{
	uae_u16 v;

	addr >>= 1;
	addr &= 31;

	if (fmv_audio_debug) {
		if (!(addr == A_INT1 || addr == A_INT2 || addr == A_CB_STATUS))
			write_log(_T("L64111 read reg %d -> %04x\n"), addr, l64111_regs[addr]);
	}
	v = l64111_regs[addr];
	if (addr == A_INT1) {
		v = l64111intstatus[0];
		l64111intstatus[0] = 0;
	} else if (addr == A_INT2) {
		v = l64111intstatus[1] & 0x7f;
		l64111intstatus[1] = 0;
	}
	return v;
}
static void l64111_wput (uaecptr addr, uae_u16 v)
{
	addr >>= 1;
	addr &= 31;

	if (fmv_audio_debug) {
		if (addr != A_DATA)
			write_log(_T("L64111 write reg %d = %04x\n"), addr, v);
	}

	switch (addr)
	{
	case A_CONTROL1:
		if ((v & 1) != (l64111_regs[addr] & 1))
			l64111_init();
		if ((v & 2))
			l64111_reset();
		if (v & 4) {
			l64111_regs[A_CB_WRITE] = 0;
			l64111_regs[A_CB_READ] = 0;
			l64111_regs[A_CB_STATUS] = 0;
			memset(pcmaudio, 0, sizeof(struct fmv_pcmaudio) * L64111_CHANNEL_BUFFERS);
			write_log(_T("L64111 buffer reset\n"));
		}
		break;
	case A_CONTROL2:
		l64111_setvolume();
		break;
	case A_DATA:
		if (l64111_fifo_cnt < 0 || l64111_fifo_cnt > L64111_FIFO_BYTES) {
			write_log(_T("L641111 fifo overflow!\n"));
			l64111_fifo_cnt = 0;
			return;
		}
		l64111_fifo[l64111_fifo_cnt++] = v >> 8;
		l64111_fifo[l64111_fifo_cnt++] = (uae_u8)v;
		if (l64111_fifo_cnt == L64111_FIFO_BYTES) {
			l64111_parse();
		}
		break;
	case A_INT1:
		l64111intmask[0] = v;
		return;
	case A_INT2:
		l64111intmask[1] = v;
		return;
	}

	l64111_regs[addr] = v;

}

static uae_u8 l64111_bget(uaecptr addr)
{
	return (uae_u8)l64111_wget(addr);
}
static void l64111_bput(uaecptr addr, uae_u8 v)
{
	l64111_wput(addr, v);
}

static void cl450_set_status(uae_u16 mask)
{
	cl450_pending_interrupts |= mask & cl450_interruptmask;
	if (cl450_hmem[CL_HMEM_INT_STATUS] == 0) {
		cl450_hmem[CL_HMEM_INT_STATUS] = cl450_pending_interrupts;
		cl450_pending_interrupts = 0;
		cl450_regs[HOST_control] &= ~0x80;
		cl450_checkint(true);
	}
}

static void cl450_write_dram(int addr, uae_u16 w)
{
	if (!fmv_ram_bank.baseaddr)
		return;
	fmv_ram_bank.baseaddr[addr * 2 + 0] = w >> 8;
	fmv_ram_bank.baseaddr[addr * 2 + 1] = (uae_u8)w;
}

#if DUMP_VIDEO
static struct zfile *videodump;
#endif

static void cl450_parse_frame(void)
{
	for (;;) {
		mpeg2_state_t mpeg_state = mpeg2_parse(mpeg_decoder);
		switch (mpeg_state)
		{
			case STATE_BUFFER:
			{
				int bufsize = cl450_buffer_offset;
				if (bufsize == 0)
					return;
				while (bufsize > 0 && cl450_newpacket_mode) {
					struct cl450_newpacket *np = &cl450_newpacket_buffer[cl450_newpacket_offset_read];
					if (cl450_newpacket_offset_read == cl450_newpacket_offset_write)
						return;
					int size = np->length > bufsize ? bufsize : np->length;

					if (np->length == 0) {
						write_log(_T("CL450 no matching newpacket!?\n"));
						return;
					}

					np->length -= size;
					bufsize -= size;
					if (np->length > 0)
						break;
					//write_log(_T("CL450: NewPacket %d done\n"), cl450_newpacket_offset_read);
					cl450_newpacket_offset_read++;
					cl450_newpacket_offset_read &= CL450_NEWPACKET_BUFFER_SIZE - 1;
				}
#if DUMP_VIDEO
				if (!videodump)
					videodump = zfile_fopen(_T("c:\\temp\\1.mpg"), _T("wb"));
				zfile_fwrite(&ram[CL450_MPEG_BUFFER], 1, cl450_buffer_offset, videodump);
#endif
				memcpy(&fmv_ram_bank.baseaddr[CL450_MPEG_DECODE_BUFFER] + libmpeg_offset, &fmv_ram_bank.baseaddr[CL450_MPEG_BUFFER], cl450_buffer_offset);
				mpeg2_buffer(mpeg_decoder, &fmv_ram_bank.baseaddr[CL450_MPEG_DECODE_BUFFER] + libmpeg_offset, &fmv_ram_bank.baseaddr[CL450_MPEG_DECODE_BUFFER] + libmpeg_offset + cl450_buffer_offset);
				libmpeg_offset += cl450_buffer_offset;
				if (libmpeg_offset >= CL450_MPEG_DECODE_BUFFER_SIZE - CL450_MPEG_BUFFER_SIZE)
					libmpeg_offset = 0;
				cl450_buffer_offset = 0;
			}
			break;
			case STATE_SEQUENCE:
				cl450_frame_pixbytes = currprefs.color_mode != 5 ? 2 : 4;
				mpeg2_convert(mpeg_decoder, cl450_frame_pixbytes == 2 ? mpeg2convert_rgb16 : mpeg2convert_rgb32, NULL);
				cl450_set_status(CL_INT_SEQ_V);
				cl450_frame_rate = mpeg_info->sequence->frame_period ? 27000000 / mpeg_info->sequence->frame_period : 0;
				cl450_frame_width = mpeg_info->sequence->width;
				cl450_frame_height = mpeg_info->sequence->height;
				cl450_write_dram(CL_DRAM_PICTURE_RATE, cl450_frame_rate);
				cl450_write_dram(CL_DRAM_H_SIZE, cl450_frame_width);
				cl450_write_dram(CL_DRAM_V_SIZE, cl450_frame_height);
				break;
			case STATE_PICTURE:
				break;
			case STATE_GOP:
				cl450_write_dram(CL_DRAM_TIME_CODE_0, (mpeg_info->gop->hours << 6) | (mpeg_info->gop->minutes));
				cl450_write_dram(CL_DRAM_TIME_CODE_1, (mpeg_info->gop->seconds << 6) | (mpeg_info->gop->pictures));
				break;
			case STATE_SLICE:
			case STATE_END:
				if (mpeg_info->display_fbuf) {
					memcpy(videoram[cl450_videoram_write].data, mpeg_info->display_fbuf->buf[0], cl450_frame_width * cl450_frame_height * cl450_frame_pixbytes);
					videoram[cl450_videoram_write].width = cl450_frame_width;
					videoram[cl450_videoram_write].height = cl450_frame_height;
					videoram[cl450_videoram_write].depth = cl450_frame_pixbytes;
					cl450_videoram_write++;
					cl450_videoram_write &= CL450_VIDEO_BUFFERS - 1;
					cl450_videoram_cnt++;
					//write_log(_T("%d\n"), cl450_videoram_cnt);
				}
				return;
			default:
				break;
		}
	}
}

static void cl450_reset(void)
{
	cl450_play = 0;
	cl450_pending_interrupts = 0;
	cl450_interruptmask = 0;
	cl450_blank = 0;
	cl450_border_color = 0;
	cl450_threshold = 4096;
	cl450_buffer_offset = 0;
	cl450_buffer_empty_cnt = 0;
	libmpeg_offset = 0;
	cl450_newpacket_mode = false;
	cl450_newpacket_offset_write = 0;
	cl450_newpacket_offset_read = 0;
	cl450_videoram_write = 0;
	cl450_videoram_read = 0;
	cl450_videoram_cnt = 0;
	memset(cl450_regs, 0, sizeof cl450_regs);
	if (mpeg_decoder)
		mpeg2_reset(mpeg_decoder, 1);
	if (fmv_ram_bank.baseaddr) {
		memset(fmv_ram_bank.baseaddr, 0, 0x100);
		write_log(_T("CL450 reset\n"));
	}
	cl450_write_dram(CL_DRAM_VER, 0x0200);
	cl450_write_dram(CL_DRAM_PID, 0x0002);
}

static void cl450_init(void)
{
	write_log(_T("CL450 CPU enabled\n"));
	cl450_hmem[15] = 0;
	cl450_regs[HOST_newcmd] = 0;
	cl450_regs[CMEM_control] = 2;
	cl450_regs[HOST_control] = 0x0080 | 0x0001;
	cl450_regs[VID_sela] = 0x8000 | 0x2000 | 0x0800 | 0x00c0 | 0x0006;
	cl450_regs[VID_selb] = 0x4000 | 0x0900 | 0x0060 | 0x0007;
	cl450_regs[HOST_scr2] = 0x1000 | 0x0d00 | 0x00e0;
	cl450_regs[HOST_scr1] = 0;
	cl450_regs[HOST_scr0] = 0;
	cl450_scr = 0;
	cl450_write_dram(CL_DRAM_VER, 0x0200);
	cl450_write_dram(CL_DRAM_PID, 0x0002);
	memset(fmv_ram_bank.baseaddr + 0x10, 0, 0x100 - 0x10);
}

static void cl450_newpacket(void)
{
	struct cl450_newpacket *np = &cl450_newpacket_buffer[cl450_newpacket_offset_write];

	cl450_newpacket_mode = true;

	np->length = cl450_hmem[1];
	np->pts = 0;
	np->pts_valid = false;
	if (cl450_hmem[2] & 0x8000) {
		uae_u64 v;
		v = ((uae_u64)cl450_regs[HOST_scr0] & 7) << 30;
		v |= (cl450_regs[HOST_scr1] & 0x7fff) << 15;
		v |= cl450_regs[HOST_scr2] & 0x7fff;
		np->pts = v;
		np->pts_valid = true;
	}

	if (fmv_video_debug & 1)
		write_log(_T("CL450 NewPacket %d: size=%d pts=%09llx v=%d\n"), cl450_newpacket_offset_write, np->length, np->pts, np->pts_valid);

	cl450_newpacket_offset_write++;
	cl450_newpacket_offset_write &= CL450_NEWPACKET_BUFFER_SIZE - 1;
}

static void cl450_from_scr(void)
{
	uae_u64 v = (uae_u64)cl450_scr;
	cl450_regs[HOST_scr0] &= ~7;
	cl450_regs[HOST_scr0] |= (v >> 30) & 7;
	cl450_regs[HOST_scr1] = (v >> 15) & 0x7fff;
	cl450_regs[HOST_scr2] = v & 0x7fff;
}

static void cl450_to_scr(void)
{
	uae_u64 v;
	v = ((uae_u64)cl450_regs[HOST_scr0] & 7) << 30;
	v |= (cl450_regs[HOST_scr1] & 0x7fff) << 15;
	v |= cl450_regs[HOST_scr2] & 0x7fff;
	cl450_scr = (float)v;
}

static void cl450_reset_cmd(void)
{
	cl450_blank = 1;
	cl450_play = 0;
	cl450_newpacket_offset_read = 0;
	cl450_newpacket_offset_write = 0;
	cl450_newpacket_mode = false;
	cl450_interruptmask = 0;
	cl450_buffer_offset = 0;
	cl450_buffer_empty_cnt = 0;
}

static void cl450_newcmd(void)
{
//	write_log(_T("* CL450 Command %04x\n"), cl450_hmem[0]);
//	for (int i = 1; i <= 4; i++)
//		write_log(_T("%02d: %04x\n"), i, cl450_hmem[i]);
	switch (cl450_hmem[0])
	{
		case CL_Play:
			cl450_play = 1;
			write_log(_T("CL450 PLAY\n"));
			break;
		case CL_Pause:
			if (cl450_play > 0) {
				// pause clears SCR
				cl450_scr = 0;
				cl450_to_scr();
			}
			cl450_play = -cl450_play;
			write_log(_T("CL450 PAUSE\n"));
			break;
		case CL_NewPacket:
			cl450_newpacket();
			break;
		case CL_InquireBufferFullness:
			cl450_hmem[0x0b] = cl450_buffer_offset;
			if (fmv_video_debug & 1)
				write_log(_T("CL450 InquireBufferFullness (%d)\n"), cl450_buffer_offset);
			break;
		case CL_SetBlank:
			cl450_blank = cl450_hmem[1] & 1;
			write_log(_T("CL450 blank = %d\n"), cl450_blank);
			break;
		case CL_SetBorder:
			cl450_border_color = ((cl450_hmem[3] & 0xff) << 16) | cl450_hmem[4];
			write_log(_T("CL450 SetBorder %08x\n"), cl450_border_color);
			cd32_fmv_new_border_color(cl450_border_color);
			break;
		case CL_SetColorMode:
			write_log(_T("CL450 SetColorMode\n"));
			break;
		case CL_SetInterruptMask:
			cl450_interruptmask = cl450_hmem[1];
			write_log(_T("CL450 SetInterruptMask %04x\n"), cl450_interruptmask);
			break;
		case CL_SetThresHold:
			cl450_threshold = cl450_hmem[1];
			write_log(_T("CL450 SetThresHold %d\n"), cl450_threshold);
			break;
		case CL_SetVideoFormat:
			write_log(_T("CL450 SetVideoFormat\n"));
			break;
		case CL_SetWindow:
			write_log(_T("CL450 SetWindow\n"));
			break;
		case CL_AccessSCR:
			if (cl450_hmem[1] & 0x8000) {
				cl450_from_scr();
				cl450_hmem[1] = 0x8000 | (cl450_regs[HOST_scr0] & 7);
				cl450_hmem[2] = cl450_regs[HOST_scr1] & 0x7fff;
				cl450_hmem[3] = cl450_regs[HOST_scr2] & 0x7fff;
			} else {
				cl450_regs[HOST_scr0] = cl450_hmem[1] & 7;
				cl450_regs[HOST_scr1] = cl450_hmem[2] & 0x7fff;
				cl450_regs[HOST_scr2] = cl450_hmem[3] & 0x7fff;
				cl450_to_scr();
			}
			if (fmv_video_debug & 1)
				write_log(_T("CL450 AccessSCR %c %09llx (%04x %04x %04x)\n"),
					(cl450_hmem[1] & 0x8000) ? 'R' : 'W', (uae_u64)cl450_scr,
					cl450_regs[HOST_scr0], cl450_regs[HOST_scr1], cl450_regs[HOST_scr2]);
			break;
		case CL_Reset:
			write_log(_T("CL450 Reset\n"));
			cl450_reset_cmd();
			break;
		case CL_FlushBitStream:
			write_log(_T("CL450 CL_FlushBitStream\n"));
			cl450_buffer_offset = 0;
			memset(cl450_newpacket_buffer, 0, sizeof cl450_newpacket_buffer);
			cl450_newpacket_offset_read = cl450_newpacket_offset_write = 0;
			break;
		default:
			write_log(_T("CL450 unsupported command %04x\n"), cl450_hmem[0]);
			break;
	}

	cl450_regs[HOST_newcmd] = 0;
}

static uae_u16 cl450_wget (uaecptr addr)
{
	uae_u16 v = 0;
	addr &= 0xfe;

	switch (addr)
	{
	case CMEM_dmactrl:
		write_log(_T("CL450 CMEM_dmactrl\n"));
		break;
	case HOST_intvecr:
		v = cl450_regs[HOST_intvecr];
		break;
	case HOST_control:
		v = cl450_regs[HOST_control];
		break;
	case HOST_raddr:
		v = cl450_regs[HOST_raddr];
		break;
	case HOST_rdata:
		v = cl450_hmem[cl450_regs[HOST_raddr]];
		break;
	case HOST_newcmd:
		v = cl450_regs[HOST_newcmd];
		break;
	case CPU_iaddr:
		v = cl450_regs[CPU_iaddr];
		break;
	case CPU_taddr:
		v = cl450_regs[CPU_taddr];
		break;
	case CPU_pc:
		v = cl450_regs[CPU_pc];
		break;
	case CPU_control:
		v = cl450_regs[CPU_control];
		break;
	case VID_control:
		v = cl450_regs[VID_control];
		break;
	case VID_regdata:
		v = cl450_vid[cl450_regs[VID_control] >> 1];
		break;
	default:
		write_log(_T("CL450 unknown register %02x read\n"), addr);
		return v;
	}

	if (fmv_video_debug & 2)
		write_log (_T("CL450 read reg %02x %04x\n"), addr, v);
	return v;
}

static void cl450_data_wput(uae_u16 v)
{
	fmv_ram_bank.baseaddr[CL450_MPEG_BUFFER + cl450_buffer_offset + 0] = v >> 8;
	fmv_ram_bank.baseaddr[CL450_MPEG_BUFFER + cl450_buffer_offset + 1] = (uae_u8)v;
	if (cl450_buffer_offset < CL450_MPEG_BUFFER_SIZE - 2)
		cl450_buffer_offset += 2;
}

static void cl450_wput(uaecptr addr, uae_u16 v)
{
	addr &= 0xfe;

	if (fmv_video_debug & 2)
		write_log(_T("CL450 write reg %02x %04x\n"), addr, v);

	switch (addr)
	{
	case CMEM_data:
		cl450_data_wput(v);
		break;
	case CMEM_control:
		cl450_regs[CMEM_control] = v;
		if (v & 0x40)
			cl450_reset();
		write_log(_T("CL450 CMEM_control %04x\n"), v);
		break;
	case CMEM_dmactrl:
		cl450_regs[CMEM_dmactrl] = v;
		write_log(_T("CL450 CMEM_dmactrl %04x\n"), v);
		break;
	case HOST_intvecw:
		cl450_regs[HOST_intvecr] = v;
		write_log(_T("CL450 HOST_intvecw %04x\n"), v);
		break;
	case HOST_control:
		cl450_regs[HOST_control] = v;
		//write_log(_T("CL450 HOST_control %04x\n"), v);
		break;
	case HOST_raddr:
		cl450_regs[HOST_raddr] = v & 15;
		//write_log(_T("CL450 HOST_raddr %04x\n"), v);
		break;
	case HOST_rdata:
		cl450_hmem[cl450_regs[HOST_raddr]] = v;
		//write_log(_T("CL450 HOST_rdata %d %04x\n"), cl450_regs[HOST_raddr], v);
		cl450_regs[HOST_raddr]++;
		cl450_regs[HOST_raddr] &= CL450_HMEM_WORDS - 1;
		break;
	case HOST_newcmd:
		cl450_regs[HOST_newcmd] = v;
		cl450_newcmd();
		break;
	case CPU_pc:
		cl450_regs[CPU_pc] = v;
		write_log(_T("CL450 CPU_pc %04x\n"), v);
		break;
	case CPU_control:
		write_log(_T("CL450 CPU_control %04x\n"), v);
		if (!(cl450_regs[CPU_control] & 1) && (v & 1)) {
			cl450_init();
		}
		cl450_regs[CPU_control] = v & 1;
		break;
	case DRAM_refcnt:
		cl450_regs[DRAM_refcnt] = v;
		write_log(_T("CL450 DRAM_refcnt %04x\n"), v);
		break;
	case CPU_imem:
		cl450_regs[CPU_iaddr] &= CL450_IMEM_WORDS - 1;
		cl450_imem[CPU_iaddr] = v;
		cl450_regs[CPU_iaddr]++;
		cl450_regs[CPU_iaddr] &= CL450_IMEM_WORDS - 1;
		break;
	case CPU_iaddr:
		cl450_regs[CPU_iaddr] = v & (CL450_IMEM_WORDS - 1);
		write_log(_T("CL450 CPU_iaddr %04x\n"), v);
		break;
	case CPU_tmem:
		cl450_regs[CPU_taddr] &= CL450_TMEM_WORDS - 1;
		cl450_tmem[CPU_taddr] = v;
		cl450_regs[CPU_taddr]++;
		cl450_regs[CPU_taddr] &= CL450_TMEM_WORDS - 1;
		break;
	case CPU_taddr:
		cl450_regs[CPU_taddr] = v & (CL450_TMEM_WORDS - 1);
		write_log(_T("CL450 CPU_taddr %04x\n"), v);
		break;
	case VID_control:
		cl450_regs[VID_control] = v & ((CL450_VID_REGS - 1) << 1);
		break;
	case VID_regdata:
		cl450_vid[cl450_regs[VID_control] >> 1] = v;
		write_log(_T("CL450 vid reg %02x = %04x\n"), cl450_regs[VID_control] >> 1, v);
		break;
	case HOST_scr0:
		cl450_regs[HOST_scr0] = v;
		break;
	case HOST_scr1:
		cl450_regs[HOST_scr1] = v;
		break;
	case HOST_scr2:
		cl450_regs[HOST_scr2] = v;
		break;
	default:
		write_log(_T("CL450 write unknown register %02x = %04x\n"), addr, v);
		return;
	}
}

static uae_u8 cl450_bget(uaecptr addr)
{
	return (uae_u8)cl450_wget(addr);
}
static void cl450_bput(uaecptr addr, uae_u8 v)
{
	cl450_wput(addr, v);
}

static uae_u8 io_bget(uaecptr addr)
{
	addr &= 0xffff;
	write_log(_T("FMV: IO byte read access %08x!\n"), addr);
	return 0;
}
static uae_u16 io_wget(uaecptr addr)
{
	uae_u16 v = 0;
	addr &= 0xffff;
	if (addr != 0)
		return 0;
	v |= IO_CL450_IRQ | IO_L64111_IRQ | IO_CL450_FIFO_STATUS;
	if (cl450_checkint(false))
		v &= ~IO_CL450_IRQ;
	if (l64111_checkint(false))
		v &= ~IO_L64111_IRQ;
	return v;
}
static void io_bput(uaecptr addr, uae_u8 v)
{
	addr &= 0xffff;
	write_log(_T("FMV: IO byte write access %08x!\n"), addr);
}
static void io_wput(uaecptr addr, uae_u16 v)
{
	addr &= 0xffff;
	if (addr != 0)
		return;
	write_log(_T("FMV: IO=%04x\n"), v);
	mpeg_io_reg = v;
	l64111_setvolume();
	cd32_fmv_state((mpeg_io_reg & IO_CL450_VIDEO) ? 1 : 0);
}

static uae_u32 REGPARAM2 fmv_wget (uaecptr addr)
{
	uae_u32 v = 0;
	addr -= fmv_start & fmv_bank.mask;
	addr &= fmv_bank.mask;
	int mask = addr & BANK_MASK;
	if (mask == L64111_BASE)
		v = l64111_wget (addr);
	else if (mask == CL450_BASE)
		v = cl450_wget (addr);
	else if (mask == IO_BASE)
		v = io_wget (addr);

#if FMV_DEBUG
	if (isdebug (addr))
		write_log (_T("fmv_wget %08X=%04X PC=%08X\n"), addr, v, M68K_GETPC);
#endif
	return v;
}

static uae_u32 REGPARAM2 fmv_lget (uaecptr addr)
{
	uae_u32 v;
	v = (fmv_wget (addr) << 16) | (fmv_wget (addr + 2) << 0);
#if FMV_DEBUG
	if (isdebug (addr))
		write_log (_T("fmv_lget %08X=%08X PC=%08X\n"), addr, v, M68K_GETPC);
#endif
	return v;
}

static uae_u32 REGPARAM2 fmv_bget (uaecptr addr)
{
	uae_u32 v = 0;
	addr -= fmv_start & fmv_bank.mask;
	addr &= fmv_bank.mask;
	int mask = addr & BANK_MASK;
	if (mask == L64111_BASE)
		v = l64111_bget (addr);
	else if (mask == CL450_BASE)
		v = cl450_bget (addr);
	else if (mask == IO_BASE)
		v = io_bget (addr);
	return v;
}

static void REGPARAM2 fmv_wput (uaecptr addr, uae_u32 w)
{
	addr -= fmv_start & fmv_bank.mask;
	addr &= fmv_bank.mask;
#if FMV_DEBUG
	if (isdebug (addr))
		write_log (_T("fmv_wput %04X=%04X PC=%08X\n"), addr, w & 65535, M68K_GETPC);
#endif
	int mask = addr & BANK_MASK;
	if (mask == L64111_BASE)
		l64111_wput (addr, w);
	else if (mask == CL450_BASE)
		cl450_wput (addr, w);
	else if (mask == IO_BASE)
		io_wput(addr, w);
	else if (mask == CL450_DATA)
		cl450_data_wput(w);
}

static void REGPARAM2 fmv_lput (uaecptr addr, uae_u32 w)
{
#if FMV_DEBUG
	if (isdebug (addr))
		write_log (_T("fmv_lput %08X=%08X PC=%08X\n"), addr, w, M68K_GETPC);
#endif
	fmv_wput (addr + 0, w >> 16);
	fmv_wput (addr + 2, w >>  0);
}

static void REGPARAM2 fmv_bput (uaecptr addr, uae_u32 w)
{
	addr -= fmv_start & fmv_bank.mask;
	addr &= fmv_bank.mask;
	int mask = addr & BANK_MASK;
	if (mask == L64111_BASE)
		l64111_bput (addr, w);
	else if (mask == CL450_BASE)
		cl450_bput (addr, w);
	else if (mask == IO_BASE)
		io_bput (addr, w);
}

static float max_sync_vpos;
static float remaining_sync_vpos;

void cd32_fmv_set_sync(float svpos, float adjust)
{
	max_sync_vpos = svpos / adjust;
	fmv_syncadjust = adjust;
}

static void fmv_next_cd_audio_buffer_callback(int bufnum, void *param)
{
	uae_sem_wait(&play_sem);
	if (bufnum >= 0) {
		fmv_bufon[bufnum] = 0;
		bufnum = 1 - bufnum;
		if (fmv_bufon[bufnum])
			audio_cda_new_buffer(&cas, (uae_s16*)cda->buffers[bufnum], PCM_SECTORS * KJMP2_SAMPLES_PER_FRAME, bufnum, fmv_next_cd_audio_buffer_callback, param);
		else
			bufnum = -1;
	}
	if (bufnum < 0) {
		audio_cda_new_buffer(&cas, NULL, 0, -1, NULL, NULL);
	}
	uae_sem_post(&play_sem);
}

void cd32_fmv_vsync_handler(void)
{
}

static void cd32_fmv_audio_handler(void)
{
	int bufnum;
	int offset, needsectors;
	bool play0, play1;

	if (!fmv_ram_bank.baseaddr)
		return;

	if (cd_audio_mode_changed || (cl450_play && !cda)) {
		cd_audio_mode_changed = false;
		if (cl450_play) {
			audio_cda_new_buffer(&cas, NULL, -1, -1, NULL, NULL);
			fmv_bufon[0] = fmv_bufon[1] = 0;
			delete cda;
			cda = new cda_audio(PCM_SECTORS, KJMP2_SAMPLES_PER_FRAME * 4, 44100);
			l64111_setvolume();
		}
	}

	if (cl450_buffer_offset == 0) {
		if (cl450_buffer_empty_cnt >= 2)
			cl450_set_status(CL_INT_UND);
		else
			cl450_buffer_empty_cnt++;
	} else {
		cl450_buffer_empty_cnt = 0;
	}

	if (!cda || !(l64111_regs[A_CONTROL1] & 1))
		return;
	play0 = fmv_bufon[0];
	play1 = fmv_bufon[1];
	needsectors = PCM_SECTORS;
	if (!play0 && !play1) {
		needsectors *= 2;
		write_log(_T("L64111 buffer underflow\n"));
	}
	offset = l64111_regs[A_CB_READ] & l64111_cb_mask;
	for (int i = 0; i < needsectors; i++) {
		int offset2 = (offset + i) & l64111_cb_mask;
		if (!pcmaudio[offset2].ready)
			return;
	}

	bufnum = 0;
	if (play0) {
		if (play1)
			return;
		bufnum = 1;
	}
	for (int i = 0; i < PCM_SECTORS; i++) {
		int offset2 = (offset + i) & l64111_cb_mask;
		memcpy(cda->buffers[bufnum] + i * KJMP2_SAMPLES_PER_FRAME * 4, pcmaudio[offset2].pcm, KJMP2_SAMPLES_PER_FRAME * 4);
		pcmaudio[offset2].ready = false;
	}
	if (!play0 && !play1) {
		fmv_bufon[bufnum] = 1;
		fmv_next_cd_audio_buffer_callback(1 - bufnum, NULL);
	}
	fmv_bufon[bufnum] = 1;
	offset += PCM_SECTORS;
	offset &= l64111_cb_mask;
	l64111_regs[A_CB_READ] = offset;
	l64111_regs[A_CB_STATUS] -= PCM_SECTORS;
}

static void cd32_fmv_hsync_handler(void)
{
	if (!fmv_ram_bank.baseaddr)
		return;

	if (cl450_play > 0)
		cl450_scr += 90000.0f / (hblank_hz / fmv_syncadjust);

	if (cl450_video_hsync_wait > 0)
		cl450_video_hsync_wait--;
	if (cl450_video_hsync_wait == 0) {
		cl450_set_status(CL_INT_PIC_D);
		if (cl450_videoram_cnt > 0) {
			cd32_fmv_new_image(videoram[cl450_videoram_read].width, videoram[cl450_videoram_read].height, 
				videoram[cl450_videoram_read].depth, cl450_blank ? NULL : videoram[cl450_videoram_read].data);
			cl450_videoram_read++;
			cl450_videoram_read &= CL450_VIDEO_BUFFERS - 1;
			cl450_videoram_cnt--;
		}
		cl450_video_hsync_wait = (int)max_sync_vpos;
		while (remaining_sync_vpos >= 1.0) {
			cl450_video_hsync_wait++;
			remaining_sync_vpos -= 1.0;
		}
		remaining_sync_vpos += max_sync_vpos - cl450_video_hsync_wait;
		if (cl450_frame_rate < 40)
			cl450_video_hsync_wait *= 2;
	}

	if ((vpos & 63) == 0)
		cd32_fmv_audio_handler();

	if (vpos & 7)
		return;

	if (cl450_play > 0) {
		if (cl450_newpacket_mode && cl450_buffer_offset < cl450_threshold) {
			int newpacket_len = 0;
			for (int i = 0; i < CL450_NEWPACKET_BUFFER_SIZE; i++)
				newpacket_len += cl450_newpacket_buffer[i].length;
			if (cl450_buffer_offset >= newpacket_len - 6)
				cl450_set_status(CL_INT_RDY);
		}

		if (cl450_buffer_offset >= 512 && cl450_videoram_cnt < CL450_VIDEO_BUFFERS - 1) {
			cl450_parse_frame();
		}
	}
}


static void cd32_fmv_reset(int hardreset)
{
	if (fmv_ram_bank.baseaddr)
		memset(fmv_ram_bank.baseaddr, 0, fmv_ram_bank.allocated_size);
	cd32_fmv_state(0);
}

static void cd32_fmv_free(void)
{
	mapped_free(&fmv_rom_bank);
	mapped_free(&fmv_ram_bank);
	xfree(audioram);
	audioram = NULL;
	xfree(videoram);
	videoram = NULL;
	if (cda) {
		fmv_next_cd_audio_buffer_callback(-1, NULL);
		delete cda;
	}
	cda = NULL;
	uae_sem_destroy(&play_sem);
	xfree(pcmaudio);
	pcmaudio = NULL;
	if (mpeg_decoder)
		mpeg2_close(mpeg_decoder);
	mpeg_decoder = NULL;
	cl450_reset();
	l64111_reset();
}

addrbank *cd32_fmv_init (struct autoconfig_info *aci)
{
	device_add_reset_imm(cd32_fmv_reset);
	cd32_fmv_free();
	write_log (_T("CD32 FMV mapped @$%x\n"), expamem_board_pointer);
	if (expamem_board_pointer != fmv_start) {
		write_log(_T("CD32 FMV unexpected base address!\n"));
	}
	if (!validate_banks_z2(&fmv_bank, expamem_board_pointer >> 16, expamem_board_size >> 16))
		return &expamem_null;

	fmv_rom_bank.start = expamem_board_pointer;
	fmv_ram_bank.start = fmv_rom_bank.start + 0x80000;

	fmv_rom_bank.mask = fmv_rom_size - 1;
	fmv_rom_bank.reserved_size = fmv_rom_size;
	fmv_ram_bank.mask = fmv_ram_size - 1;
	fmv_ram_bank.reserved_size = fmv_ram_size;

	if (mapped_malloc(&fmv_rom_bank)) {
		load_rom_rc(aci->rc, ROMTYPE_CD32CART, 262144, 0, fmv_rom_bank.baseaddr, 262144, 0);
	}

	if (!fmv_rom_bank.baseaddr) {
		write_log(_T("CD32 FMV without ROM is not supported.\n"));
		return &expamem_null;
	}
	if (!audioram)
		audioram = xmalloc(uae_u8, 262144);
	if (!videoram)
		videoram = xmalloc(struct cl450_videoram, CL450_VIDEO_BUFFERS);
	mapped_malloc(&fmv_ram_bank);
	if (!pcmaudio)
		pcmaudio = xcalloc(struct fmv_pcmaudio, L64111_CHANNEL_BUFFERS);
	kjmp2_init(&mp2);
	if (!mpeg_decoder) {
		mpeg_decoder = mpeg2_init();
		mpeg_info = mpeg2_info(mpeg_decoder);
	}
	memset(&cas, 0, sizeof(cas));
	fmv_bank.mask = fmv_board_size - 1;
	map_banks(&fmv_rom_bank, (fmv_start + ROM_BASE) >> 16, fmv_rom_size >> 16, 0);
	map_banks(&fmv_ram_bank, (fmv_start + RAM_BASE) >> 16, fmv_ram_size >> 16, 0);
	map_banks(&fmv_bank, (fmv_start + IO_BASE) >> 16, (RAM_BASE - IO_BASE) >> 16, 0);
	uae_sem_init(&play_sem, 0, 1);
	cd32_fmv_reset(1);

	device_add_hsync(cd32_fmv_hsync_handler);
	device_add_vsync_pre(cd32_fmv_vsync_handler);
	device_add_exit(cd32_fmv_free, NULL);
	device_add_rethink(rethink_cd32fmv);

	return &fmv_rom_bank;
}
#endif
