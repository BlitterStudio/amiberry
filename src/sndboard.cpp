/*
* UAE - The Un*x Amiga Emulator
*
* Toccata Z2
* Prelude and Prelude A1200
*
* Copyright 2014-2015 Toni Wilen
*
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "debug.h"
#include "sndboard.h"
#include "audio.h"
#include "autoconf.h"
#include "pci_hw.h"
#include "qemuvga/qemuaudio.h"
#include "rommgr.h"
#include "devices.h"

#define DEBUG_SNDDEV 0
#define DEBUG_SNDDEV_READ 0
#define DEBUG_SNDDEV_WRITE 0
#define DEBUG_SNDDEV_FIFO 0

static void snd_init(void);
static void sndboard_rethink(void);
static uae_u8 *sndboard_get_buffer(int *frames);
static void sndboard_release_buffer(uae_u8 *buffer, int frames);
static void sndboard_free_capture(void);
static bool sndboard_init_capture(int freq);
static void uaesndboard_reset(int hardreset);
static void sndboard_reset(int hardreset);

static float base_event_clock;

extern addrbank uaesndboard_bank_z2, uaesndboard_bank_z3;

#define MAX_DUPLICATE_SOUND_BOARDS 1
#define MAX_SNDDEVS 3

#define MAX_UAE_CHANNELS 8
#define MAX_UAE_STREAMS 8
struct uaesndboard_stream
{
	int streamid;
	int play;
	int ch;
	int bitmode;
	int volume;
	uae_u32 flags;
	uae_u16 format;
	uae_u32 freq;
	int event_time;
	uaecptr baseaddr;
	uaecptr setaddr;
	uaecptr address, original_address;
	uaecptr next;
	uaecptr repeat;
	uaecptr indirect_ptr;
	uaecptr indirect_address;
	int indirect_len;
	int lastlen;
	int len, original_len;
	int replen;
	int repcnt;
	int first;
	bool repeating;
	uae_u8 chmode;
	uae_s16 panx, pany;
	uae_u8 intenamask;
	uae_u8 intreqmask;
	uae_u8 masterintenamask;
	int framesize;
	uae_u8 io[256];
	uae_u16 wordlatch;
	int timer_cnt;
	int timer_event_time;
	int sample[MAX_UAE_CHANNELS];
};
struct uaesndboard_data
{
	bool enabled;
	bool z3;
	int configured;
	uae_u32 streammask;
	uae_u32 streamallocmask;
	uae_u32 streamintenamask;
	uae_u32 streamintreqmask;
	uae_u8 acmemory[128];
	int streamcnt;
	int volume[MAX_UAE_CHANNELS];
	struct uaesndboard_stream stream[MAX_UAE_STREAMS];
	uae_u8 info[256];
};

static struct uaesndboard_data uaesndboard[MAX_DUPLICATE_SOUND_BOARDS];

/*
	autoconfig data:

	manufacturer 6502
	product 2
	Z2 64k board or Z3 16M board, no boot rom. May have optional boot rom in the future.

	Z2 board has 32k of RAM (0x8000-0xffff). Z3 board has 8M of RAM. (Upper 8M of board)
	It can be used as 1:1 physical/logical mapped sample set or sample data storage. 

	uaesnd sample set structure, can be located anywhere in Amiga address space.
	It is never modified by UAE. Must be long aligned.

	 0.W flags. Must be zero.
	 2.W format. Must be zero. (non-zero values reserved for compressed/non-PCM formats. If needed in the future..)

	 4.L sample address pointer
	 8.L sample length (in frames, negative=play backwards, set address after last sample.). 0 = repeat previous sample until length is updated.

	If sample length equals 0x80000000, sample address pointer becomes pointer to array that has one or more sample pointer and sample length pairs.
	All sample pointers will played sequentially, NULL pointer ends the chain.
	(sample ptr1, sample len1, ptr2, len2, ptr3, len3, NULL) Can be useful if using non-1:1 physical to logical address mapping.

	12.L playback frequency in Hz * 256 (bits 8-27: integer part, bits 0 to 7 fractional part). In Paula periods if high word is FFFF (0xFFFFxxxx).
	16.L repeat sample address pointer (Ignored if repeat address=0 and length=0)
	20.L repeat sample length (negative=play backwards, 0 = repeat previous sample until length is updated.)
	24.W repeat count (0=no repeat, -1=repeat forever)
	26.W volume (0 to 32768)
	28.L next sample set address (0=end, this was last set)
	32.B number of channels. (interleaved samples if 2 or more channels)
	33.B sample type (bit 0: bits per sample, 0=8,1=16,2=24,3=32 bit 6=signed, bit 7=little-endian)
	34.B bit 0: interrupt when set starts, bit 1: interrupt when set ends (last sample played), bit 2: interrupt when repeat starts, bit 3: after each sample.
	35.B if mono stream, bit mask that selects output channels. (0=default, redirect to left and right channels)
	(Can be used for example when playing tracker modules by using 4 single channel streams)
	stereo or higher: channel mode.
	36.W horizontal panning (-32767 to 32767). Not yet implemented  Must be zero.
	38.W front to back panning (-32767 to 32767). Not yet implemented. Must be zero.
	40.L original address (RO)
	44.L original length (RO)

	Channel mode = 0

	2: left, right
	4: left, right, left back, right back
	6: left, right, center, lfe, left back, right back, left surround
	8: left, right, center, lfe, left back, right back, right surround

	Channel mode = 1 (alternate channel order)

	2: left, right (no change)
	4: left, right, center, back
	6: left, right, left back, right back, center, lfe
	8: left, right, left back, right back, left surround, right surround, center, lfe

	Hardware addresses relative to uaesnd autoconfig board start:

	Read-only hardware information:

	$0080.L uae version (ver.rev.subrev)
	$0084.W uae sndboard "hardware" version
	$0086.W uae sndboard "hardware" revision
	$0088.L preferred frequency (=user configured mixing/resampling rate)
	$008C.B max number of channels/stream (>=6: 5.1 fully supported, 7.1 also supported but last 2 surround channels are currently muted)
	$008D.B active number of channels (mirrors selected UAE sound mode. inactive channels are muted but can be still be used normally.)
	$008E.B max number of simultaneous audio streams (currently 8)
	$0090.L offset to RAM from board base address (or zero if no RAM)
	$0094.L size of RAM (or zero if no RAM)

	$00E0.L allocated streams, bit mask. Hardware level stream allocation feature, use single test and set/clear instruction or
			disable interrupts before allocating/freeing streams. If stream bit is not set, stream's address range is inactive.
			byte access to $00E3 is also allowed.

	$00E4.L Stream latch bit. Write-only. Bit set = latch stream, bit not set = does nothing. ($00E6.W and $00E7.B also allowed)
	$00E8.L Stream unlatch bit. Same behavior as $00E4.L. ($00EA.W and $00EB.B also allowed)

	$00F0.L stream enable bit mask. RW. Can be used to start or stop multiple streams simultaneously. $00F3.B is also allowed.
			Changing stream mask always clears stream's interrupt status register.
	$00F4.L stream master interrupt enable bit mask. RW.
	$00F8.L stream master interrupt request bit mask. RO.

	$0100 stream 1

	$0100-$013f: Current sample set structure. RW.
	$0140-$017f: Latched sample set structure. RW.
	$0180.L Current sample set pointer. RW.
	$0184.B Reserved
	$0185.B Reserved
	$0186.B Reserved
	$0187.B Interrupt status. 7: set when interrupt active. 0,1,2,3: same as 34.B bit, always set when condition matches,
			even if 34.B bit is not set. If also 34.B bit is set: bit 7 set and interrupt is activated.
			bit 4 = timer interrupt. Reading clears interrupt. RO.
	$0188.B Reserved
	$0189.B Reserved
	$018A.B Reserved
	$018B.B Alternate interrupt status. Same as $187.B but reading does not clear interrupt. RO.
	$018C.B Reserved
	$018D.B Reserved
	$018E.B Reserved
	$018F.B Stream master interrupt enable (same bits as $0187.B)
	$0190.B Reserved
	$0191.B Reserved
	$0192.B Reserved
	$0193.B Status (Read: bit 0 = play active, bit 1 = output stream allocated, bit 2 = repeating, Write: reload sample set if not playing or repeating)
	$0194.L Timer frequency (same format as 12.L), sets interrupt bit 4. Zero = disabled. WO.
	$0198.L
	$019C.L
	$01a0.L Sample set pointer, force load. WO.

	$0200 stream 2

	...

	$0800 stream 8

	Writing non-zero to sample set pointer ($180) starts audio if not already started and stream enable ($F0) bit is set.
	Writing zero stops the stream immediately. Also clears automatically when stream ends.

	Long wide registers have special feature when read using word wide reads (68000/10 CPU), when high word is read,
	low word is copied to internal register. Following low word read comes from register copy.
	This prevents situation where sound emulation can modify the register between high and low word reads.
	68020+ aligned long accesses are always guaranteed safe.

	Set structure is copied to emulator side internal address space when set starts.
	This data can be always read and written in real time by accessing $0100-$013f space.
	Writes are nearly immediate, values are updated during next sample period.
	(It probably is not a good idea to do on the fly change number of channels or bits per sample values..)

	Use hardware current sample set structure to detect current sample address, length and repeat count. 

	Repeat address pointer and length values are (re-)fetched from set structure each time when new repeat starts.

	Next sample set pointer is fetched when current set finishes.

	Reading interrupt status register will also clear interrupts.
	Interrupt is level 6 (EXTER).

	Sample set parameters are validated, any error causes audio to stop immediately and log message will be printed.

	During non-repeat playback sample address increases and sample length decreases. When length becomes zero,
	repeat count is checked, if it is non-zero, repeat address and repeat length are loaded from memory and start
	counting (address increases, length decreases). Note that sample address and length won't change anymore.
	when repeat counter becomes zero (or it was already zero), next sample set address is loaded and started.

	Hardware word and long accesses must be aligned (unaligned write is ignored, unaligned read will return 0xffff or 0xffffffff)

	Timer is usable when stream is allocated. Active audio play is not required.

*/

#define STREAM_STRUCT_SIZE 40

static bool audio_state_sndboard_uae(int streamid, void *params);

extern addrbank uaesndboard_ram_bank;
MEMORY_FUNCTIONS(uaesndboard_ram);
addrbank uaesndboard_ram_bank = {
	uaesndboard_ram_lget, uaesndboard_ram_wget, uaesndboard_ram_bget,
	uaesndboard_ram_lput, uaesndboard_ram_wput, uaesndboard_ram_bput,
	uaesndboard_ram_xlate, uaesndboard_ram_check, NULL, _T("*"), _T("UAESND memory"),
	uaesndboard_ram_lget, uaesndboard_ram_wget,
	ABFLAG_RAM | ABFLAG_THREADSAFE, 0, 0
};

static bool uaesnd_rethink(void)
{
	bool irq = false;
	for (int j = 0; j < MAX_DUPLICATE_SOUND_BOARDS; j++) {
		struct uaesndboard_data *data = &uaesndboard[j];
		if (data->enabled) {
			for (int i = 0; i < MAX_UAE_STREAMS; i++) {
				if (data->streamintenamask & (1 << i)) {
					struct uaesndboard_stream *s = &uaesndboard[j].stream[i];
					if (s->intreqmask & 0x80) {
						data->streamintreqmask |= 1 << i;
						irq = true;
						break;
					}
				}
				if (!irq) {
					data->streamintreqmask &= ~(1 << i);
				}
			}
		}
	}
	return irq;
}

static void uaesndboard_stop(struct uaesndboard_data *data, struct uaesndboard_stream *s)
{
	if (!s->play)
		return;

#if DEBUG_SNDDEV
	write_log("UAESND %d: STOP\n", s - data->stream);
#endif

	s->play = 0;
	s->next = 0;
	data->streammask &= ~(1 << (s - data->stream));
	audio_enable_stream(false, s->streamid, 0, NULL, NULL);
	s->streamid = 0;
	data->streamcnt--;
}

static void uaesndboard_maybe_alloc_stream(struct uaesndboard_data *data, struct uaesndboard_stream *s)
{
	if (!s->play)
		return;
	if (s->streamid > 0)
		return;
	if (s->event_time == MAX_EV || s->ch == 0) {
		uaesndboard_stop(data, s);
		return;
	} else {
		data->streamcnt++;
		s->streamid = audio_enable_stream(true, -1, MAX_UAE_CHANNELS, audio_state_sndboard_uae, NULL);

#if DEBUG_SNDDEV
		write_log("UAESND %d: Stream allocated %d\n", s - data->stream, s->streamid);
#endif

		if (!s->streamid) {
			uaesndboard_stop(data, s);
		}
	}
}

#define UAESND_MAX_FREQ 200000

static void uaesnd_setfreq(struct uaesndboard_data *data, struct uaesndboard_stream *s)
{
	if (s->freq == 0) {
		s->event_time = MAX_EV;
	} else if ((s->freq >> 16) == 0xffff) {
		s->event_time = s->freq & 65535;
	} else {
		if (s->freq < 1 * 256)
			s->freq = 1 * 256;
		if (s->freq > UAESND_MAX_FREQ * 256)
			s->freq = UAESND_MAX_FREQ * 256;
		s->event_time = (uae_s64)base_event_clock * CYCLE_UNIT * 256 / s->freq;
	}
	uaesndboard_maybe_alloc_stream(data, s);
}

static struct uaesndboard_stream *uaesnd_addr(uaecptr addr)
{
	if (addr < 0x100)
		return NULL;
	int stream = (addr - 0x100) / 0x100;
	if (stream >= MAX_UAE_STREAMS)
		return NULL;
	if (!(uaesndboard[0].streamallocmask & (1 << stream)))
		return NULL;
	return &uaesndboard[0].stream[stream];
}

static struct uaesndboard_stream *uaesnd_get(uaecptr addr)
{
	struct uaesndboard_stream *s = uaesnd_addr(addr);
	if (!s)
		return NULL;

	put_word_host(s->io +  0, s->flags);
	put_word_host(s->io +  2, s->format);
	put_long_host(s->io +  4, s->address);
	put_long_host(s->io +  8, s->len);
	put_long_host(s->io + 12, s->freq);
	put_long_host(s->io + 16, s->repeat);
	put_long_host(s->io + 20, s->replen);
	put_word_host(s->io + 24, s->repcnt);
	put_word_host(s->io + 26, s->volume);
	put_long_host(s->io + 28, s->next);
	put_byte_host(s->io + 32, s->ch);
	put_byte_host(s->io + 33, s->bitmode);
	put_byte_host(s->io + 34, s->intenamask);
	put_byte_host(s->io + 35, s->chmode);
	put_word_host(s->io + 36, s->panx);
	put_word_host(s->io + 38, s->pany);
	put_long_host(s->io + 0x80, s->setaddr);
	put_long_host(s->io + 0x84, s->intreqmask);
	put_long_host(s->io + 0x88, s->intreqmask);
	put_long_host(s->io + 0x8c, s->masterintenamask);

	return s;
}

static void uaesndboard_start(struct uaesndboard_data *data, struct uaesndboard_stream *s)
{
	if (s->play)
		return;

	if (!(data->streammask & (1 << (s - data->stream))))
		return;

#if DEBUG_SNDDEV
	write_log("UAESND %d: PLAY\n", s - data->stream);
#endif

	s->play = 1;
	for (int i = 0; i < MAX_UAE_CHANNELS; i++) {
		s->sample[i] = 0;
	}
	uaesnd_setfreq(data, s);
}

static bool get_indirect(struct uaesndboard_stream *s, uaecptr saddr)
{
	if (!valid_address(saddr, 8)) {
		write_log(_T("UAESND: invalid indirect pointer %08x\n"), saddr);
		return false;
	}
	s->indirect_ptr = saddr;
	s->indirect_address = get_long(saddr);
	s->indirect_len = get_long(saddr + 4);
	if (!s->indirect_address)
		return true;
	if (!valid_address(s->indirect_address, abs(s->indirect_len) * s->framesize)) {
		write_log(_T("UAESND: invalid indirect sample pointer range %08x - %08x\n"), s->indirect_address, s->indirect_address + s->indirect_len * s->framesize);
		return false;
	}
	return true;
}

static bool uaesnd_validate(struct uaesndboard_data *data, struct uaesndboard_stream *s)
{
	int samplebits = (s->bitmode & 1) ? 16 : 8;

	s->framesize = samplebits * s->ch / 8;

	if (s->flags != 0) {
		write_log(_T("UAESND: Flags must be zero (%08x)\n"), s->flags);
		return false;
	}
	if (s->format != 0) {
		write_log(_T("UAESND: Only format=0 supported (%04x)\n"), s->format);
		return false;
	}
	if (s->ch < 0 || s->ch > MAX_UAE_CHANNELS || s->ch == 3 || s->ch == 5 || s->ch == 7) {
		write_log(_T("UAESND: unsupported number of channels %d\n"), s->ch);
		return false;
	}
	if (s->freq != 0) {
		if ((s->freq >> 16) == 0xffff) {
			if ((s->freq & 65535) == 0) {
				write_log(_T("UAESND: zero period is not allowed\n"));
				return false;
			}
		} else if (s->freq < 1 * 256 || s->freq > UAESND_MAX_FREQ * 256) {
			write_log(_T("UAESND: unsupported frequency %d\n"), s->freq);
			return false;
		}
	}
	if (s->volume < 0 || s->volume > 32768) {
		write_log(_T("UAESND: unsupported volume %d\n"), s->volume);
		return false;
	}
	if (s->next && ((s->next & 1) || !valid_address(s->next, 32))) {
		write_log(_T("UAESND: invalid next sample set pointer %08x\n"), s->next);
		return false;
	}
	uaecptr saddr = s->address;
	if (s->len == 0x80000000) {
		if (!get_indirect(s, saddr))
			return false;
	} else {
		if (s->len < 0)
			saddr -= abs(s->len) * s->framesize;
		if (!valid_address(saddr, abs(s->len) * s->framesize)) {
			write_log(_T("UAESND: invalid sample pointer range %08x - %08x\n"), saddr, saddr + s->len * s->framesize);
			return false;
		}
	}
	if (s->repcnt) {
		uaecptr repeat = s->repeat;
		if (s->replen < 0)
			repeat -= abs(s->replen) * s->framesize;
		if (s->repeat && !valid_address(repeat, abs(s->replen) * s->framesize)) {
			write_log(_T("UAESND: invalid sample repeat pointer range %08x - %08x\n"), repeat, repeat + s->replen * s->framesize);
			return false;
		}
	}
	if (s->panx || s->pany) {
		write_log(_T("UAESND: Panning values must be zeros\n"));
		return false;
	}
	uaesnd_setfreq(data, s);
	for (int i = s->ch; i < MAX_UAE_CHANNELS; i++) {
		s->sample[i] = 0;
	}
	return true;
}

static void uaesnd_load(struct uaesndboard_stream *s, uaecptr addr)
{
	s->flags =		get_word(addr +  0);
	s->format =		get_word(addr +  2);
	s->address =	get_long(addr +  4);
	s->len =		get_long(addr +  8);
	s->freq =		get_long(addr + 12);
	s->repeat =		get_long(addr + 16);
	s->replen =		get_long(addr + 20);
	s->repcnt =		get_word(addr + 24);
	s->volume =		get_word(addr + 26);
	s->next =		get_long(addr + 28);
	s->ch =			get_byte(addr + 32);
	s->bitmode =	get_byte(addr + 33);
	s->intenamask = get_byte(addr + 34);
	s->chmode =		get_byte(addr + 35);
	s->panx =		get_word(addr + 36);
	s->pany =		get_word(addr + 38);

	if (s->len)
		s->lastlen = s->len;

	s->original_address = s->address;
	s->original_len = s->len;
	put_long_host(s->io + 40, s->original_address);
	put_long_host(s->io + 44, s->original_len);

#if DEBUG_SNDDEV
	write_log(_T("PTR = %08x\n"), addr);
	write_log(_T("Flags = %04x\n"), s->flags);
	write_log(_T("Format = %04x\n"), s->format);
	write_log(_T("Address = %08x\n"), s->address);
	write_log(_T("Length = %08x\n"), s->len);
	write_log(_T("Frequency = %d.%d\n"), s->freq >> 8, s->freq & 255);
	write_log(_T("Repeat = %08x\n"), s->repeat);
	write_log(_T("Replen = %08x\n"), s->replen);
	write_log(_T("Repcnt = %04x\n"), s->repcnt);
	write_log(_T("Volume = %04x\n"), s->volume);
	write_log(_T("Next = %08x\n"), s->next);
	write_log(_T("CH = %02x\n"), s->ch);
	write_log(_T("Bitmode = %02x\n"), s->bitmode);
	write_log(_T("Intena = %02x\n"), s->intenamask);
	write_log(_T("CHmode = %02x\n"), s->chmode);
	write_log(_T("PanX = %04x\n"), s->panx);
	write_log(_T("PanY = %04x\n"), s->pany);
#endif
}

static bool uaesnd_directload(struct uaesndboard_data *data, struct uaesndboard_stream *s, int reg)
{
	if (reg < 0 || reg == 4) {
		s->address = get_long_host(s->io + 4);
	}
	if (reg < 0 || reg == 8) {
		s->len = get_long_host(s->io + 8);
		if (s->len)
			s->lastlen = s->len;
	}
	if (reg < 0 || reg == 12) {
		s->freq = get_long_host(s->io + 12);
	}
	if (reg < 0 || reg == 16) {
		s->repeat = get_long_host(s->io + 16);
	}
	if (reg < 0 || reg == 20) {
		s->replen = get_long_host(s->io + 20);
	}
	return uaesnd_validate(data, s);
}

static bool uaesnd_next(struct uaesndboard_data *data, struct uaesndboard_stream *s, uaecptr addr)
{
	if ((addr & 3) || !valid_address(addr, STREAM_STRUCT_SIZE) || addr < 0x100) {
		write_log(_T("UAESND: invalid sample set pointer %08x\n"), addr);
		return false;
	}

	s->setaddr = addr;
	uaesnd_load(s, addr);
	s->first = 10;
	s->repeating = false;

	return uaesnd_validate(data, s);
}

static void uaesnd_stream_start(struct uaesndboard_data *data, struct uaesndboard_stream *s, bool always)
{
	if ((!s->play || always) && s->next) {
		if (data->streammask & (1 << (s - data->stream))) {
#if DEBUG_SNDDEV
			write_log(_T("UAESND %d start (old=%d,forced=%d,next=%08x)\n"), s - data->stream, s->play, always, s->next);
#endif
			if (uaesnd_next(data, s, s->next)) {
				uaesndboard_start(data, s);
				return;
			}
		}
	}
	if (s->play && !s->next) {
		uaesndboard_stop(data, s);
	}
}

static void uaesnd_irq(struct uaesndboard_stream *s, uae_u8 mask)
{
	uae_u8 enablemask = s->masterintenamask;
	uae_u8 intenamask = s->intenamask | 0x10 | 0x20 | 0x40;
	s->intreqmask |= mask;
	if ((intenamask & mask) && (enablemask & mask)) {
		s->intreqmask |= 0x80;
		devices_rethink_all(sndboard_rethink);
	}
}

static void uaesnd_streammask(struct uaesndboard_data *data, uae_u32 m)
{
	uae_u32 old = data->streammask;
	data->streammask = m;
	data->streammask &= (1 << MAX_UAE_STREAMS) - 1;
	for (int i = 0; i < MAX_UAE_STREAMS; i++) {
		if ((old ^ data->streammask) & (1 << i)) {
			struct uaesndboard_stream *s = &data->stream[i];
			s->intreqmask = 0;
			if (data->streammask & (1 << i)) {
				uaesnd_stream_start(data, s, false);
			} else {
				uaesndboard_stop(data, s);
			}
		}
	}
}

static bool audio_state_sndboard_uae(int streamid, void *params)
{
	struct uaesndboard_data *data = &uaesndboard[0];
	struct uaesndboard_stream *s = NULL;

	for (int i = 0; i < MAX_UAE_STREAMS; i++) {
		if (data->stream[i].streamid == streamid) {
			s = &data->stream[i];
			break;
		}
	}
	if (!s)
		return false;
	int highestch = s->ch;
	int streamnum = (int)(s - data->stream);
	if (s->play && (data->streammask & (1 << streamnum))) {
		uaecptr addr;
		int len;
		if (s->indirect_ptr) {
			addr = s->indirect_address;
			len = s->indirect_len;
		} else {
			len = s->repeating ? s->replen : s->len;
			addr = s->repeating ? s->repeat : s->address;
		}
		int st = (s->bitmode & 7);
		bool le = (s->bitmode & 0x80) != 0;
		bool sign = (s->bitmode & 0x40) != 0;
		bool len_nonzero = len != 0;
		if (len_nonzero) {
			if (len < 0)
				addr -= s->framesize;
			for (int i = 0; i < s->ch; i++) {
				uae_u16 sample = 0;
				switch (st)
				{
				case 3: // 32-bit (last 2 bytes ignored)
					sample = get_word(le ? (addr + 2) : (addr + 0));
					addr += 4;
					break;
				case 2: // 24-bit (last byte ignored)
					sample = get_word(le ? (addr + 1) : (addr + 0));
					addr += 3;
					break;
				case 1: // 16-bit
					sample = get_word(addr);
					addr += 2;
					break;
				case 0: // 8-bit
					sample = get_byte(addr);
					sample = (sample << 8) | sample;
					addr += 1;
					break;
				}
				if (le)
					sample = (sample >> 8) | (sample << 8);
				if (sign)
					sample -= 0x8000;
				uae_s16 samples = (uae_s16)sample;
				s->sample[i] = samples * ((s->volume + 1) / 2 + (data->volume[i] + 1) / 2) / 32768;
			}
			if (len < 0)
				addr -= s->framesize;
			if (s->repeating) {
				s->repeat = addr;
				if (s->replen > 0)
					s->replen--;
				else
					s->replen++;
				len = s->replen;
			} else {
				s->address = addr;
				if (s->indirect_len) {
					if (s->indirect_len > 0)
						s->indirect_len--;
					else
						s->indirect_len++;
					len = s->indirect_len;
				} else {
					if (s->len > 0)
						s->len--;
					else
						s->len++;
					len = s->len;
				}
			}
			uaesnd_irq(s, 8);
		}
		if (s->first > 0) {
			s->first--;
			if (!s->first) {
				uaesnd_irq(s, 1);
			}
		}
		// if len was zero when called: do nothing.
		if (len == 0 && len_nonzero) {
			bool end = true;
			// sample ended
			if (s->repcnt) {
				if (s->repcnt != 0xffff)
					s->repcnt--;
				s->repeat = get_long(s->setaddr + 16);
				if (s->repeat) {
					s->replen = get_long(s->setaddr + 20);
					if (s->replen == 0) {
						s->replen = s->lastlen;
					} else {
						s->lastlen = s->replen;
					}
				} else {
					s->repeat = 0;
				}
				s->repeating = true;
				uaesnd_irq(s, 4);
				end = false;
			} else if (s->indirect_ptr) {
				s->indirect_ptr += 8;
				if (!get_indirect(s, s->indirect_ptr)) {
					uaesndboard_stop(data, s);
				}
				if (!s->indirect_address) {
					s->indirect_ptr = 0;
				}
			}
			if (end) {
				// set ended
				uaesnd_irq(s, 2);
				s->next = get_long(s->setaddr + 28);
				if (s->next) {
					if (!uaesnd_next(data, s, s->next)) {
						uaesndboard_stop(data, s);
					}
				} else {
					uaesndboard_stop(data, s);
				}
			}
		}
	}
	if (s->ch == 1 && s->chmode) {
		int smp = s->sample[0];
		for (int i = 1; i < MAX_UAE_CHANNELS; i++) {
			if ((1 << i) & s->chmode) {
				s->sample[i] = smp;
				if (i > highestch)
					highestch = i;
			}
		}
	} else if (s->ch == 4 && s->chmode == 1) {
		s->sample[2] = s->sample[4];
		s->sample[3] = s->sample[5];
	} else if (s->ch == 6 && s->chmode == 1) {
		int c = s->sample[2];
		int lfe = s->sample[3];
		s->sample[2] = s->sample[4];
		s->sample[3] = s->sample[5];
		s->sample[4] = c;
		s->sample[5] = lfe;
	} else if (s->ch == 8 && s->chmode == 1) {
		int c = s->sample[2];
		int lfe = s->sample[3];
		s->sample[2] = s->sample[4];
		s->sample[3] = s->sample[5];
		s->sample[4] = s->sample[6];
		s->sample[5] = s->sample[7];
		s->sample[6] = c;
		s->sample[7] = lfe;
	}
	audio_state_stream_state(streamid, s->sample, highestch, s->event_time);
	return true;
}

static void uaesnd_latch(struct uaesndboard_data *data, struct uaesndboard_stream *s)
{
	memcpy(s->io + 0x40, s->io + 0x00, 0x40);
}
static void uaesnd_latch_back(struct uaesndboard_data *data, struct uaesndboard_stream *s)
{
	memcpy(s->io + 0x00, s->io + 0x40, 0x40);
	if (!uaesnd_directload(data, s, -1)) {
		uaesndboard_stop(data, s);
	} else if (!s->play) {
		uaesndboard_start(data, s);
	}
}

static void uaesnd_latch_mask(struct uaesndboard_data *data, uae_u32 mask)
{
	for (int i = 0; i < MAX_UAE_STREAMS; i++) {
		if ((mask & (1 << i)) && (data->streammask & (1 << i))) {
			uaesnd_latch(data, &data->stream[i]);
		}
	}
}

static void uaesnd_unlatch_mask(struct uaesndboard_data *data, uae_u32 mask)
{
	for (int i = 0; i < MAX_UAE_STREAMS; i++) {
		if ((mask & (1 << i)) && (data->streammask & (1 << i))) {
			uaesnd_latch_back(data, &data->stream[i]);
		}
	}
}

static int uaesnd_timer_period(uae_u32 v)
{
	if (v == 0) {
		v = 0;
	} else if ((v >> 16) == 0xffff) {
		v = v & 65535;
	} else {
		v = (int)(((uae_s64)base_event_clock * CYCLE_UNIT * 256) / v);
	}
	return v;
}

static void uaesnd_timer(uae_u32 v)
{
	struct uaesndboard_data *data = &uaesndboard[v >> 16];
	struct uaesndboard_stream *s = &data->stream[v & 65535];
	s->timer_cnt = get_long_host(s->io + 0x94);
	if (s->timer_cnt > 0 && data->enabled) {
		s->timer_event_time = uaesnd_timer_period(s->timer_cnt);
		if (s->timer_event_time > 0) {
			event2_newevent_xx(-1, s->timer_event_time, (int)(s - &data->stream[0]), uaesnd_timer);
			uaesnd_irq(s, 0x10);
		}
	}
}

static void uaesnd_put(struct uaesndboard_data *data, struct uaesndboard_stream *s, int reg)
{
	if (reg == 0x80) { // set pointer write?
		uaecptr setaddr = get_long_host(s->io + 0x80);
		s->next = setaddr;
		uaesnd_stream_start(data, s, false);
	} else if (reg == 0xa0) { // force set pointer write?
		uaecptr setaddr = get_long_host(s->io + 0xa0);
		s->next = setaddr;
		uaesnd_stream_start(data, s, true);
	} else if (reg >= 0x8c && reg <= 0x8f) {
		s->masterintenamask = get_long_host(s->io + 0x8c);
	} else if (reg == 0x94) { // timer
		int timer_cnt = get_long_host(s->io + 0x94);
#if DEBUG_SNDDEV
		write_log(_T("uaesnd timer %d: %d -> %d\n"), s - &data->stream[0], s->timer_cnt, timer_cnt);
#endif
		if (timer_cnt != s->timer_cnt) {
			s->timer_cnt = 0;
			if (timer_cnt > 0) {
				s->timer_event_time = uaesnd_timer_period(timer_cnt);
				if (s->timer_event_time > 0) {
					s->timer_cnt = timer_cnt;
					event2_newevent_xx(-1, s->timer_event_time, (((int)(data - &uaesndboard[0])) << 16) | ((int)(s - &data->stream[0])), uaesnd_timer);
				}
			}
		}
	} else if (reg == 0x93) { // status strobe
		uae_u8 b = get_byte_host(s->io + 0x93);
		if (b & 1) {
			// new sample queued, do not repeat or if already repeating: start immediately
			s->repcnt = 0;
			if (s->repeating) {
				s->replen = 1;
			}
		}
	} else if (reg < 0x40) {
		if (!uaesnd_directload(data, s, reg)) {
			uaesndboard_stop(data, s);
		}
	}
}

static void uaesnd_configure(struct uaesndboard_data *data)
{
	data->configured = 1;
	for (int i = 0; i < MAX_UAE_STREAMS; i++) {
		data->stream[i].baseaddr = expamem_board_pointer + 0x100 + 0x100 * i;
	}
}

static uae_u32 REGPARAM2 uaesndboard_bget(uaecptr addr)
{
	uae_u8 v = 0;
	struct uaesndboard_data *data = &uaesndboard[0];
	addr &= 65535;
	if (addr < 0x80) {
		return data->acmemory[addr];
	} else if (data->configured) {
		struct uaesndboard_stream *s = uaesnd_get(addr);
		if (s) {
			int reg = addr & 255;
			if (reg == 0x87)
				s->intreqmask = 0;
			s->io[0x93] = (s->play ? 1 : 0) | (s->streamid > 0 ? 2 : 0) | (s->repeating ? 4 : 0);
			v = get_byte_host(s->io + reg);
		} else if (addr >= 0x80 && addr < 0xe0) {
			v = get_byte_host(data->info + (addr & 0x7f));
		} else if (addr >= 0xe0 && addr <= 0xe3) {
			v = data->streamallocmask >> (8 * (3 - (addr - 0xe0)));
		} else if (addr >= 0xf0 && addr <= 0xf3) {
			v = data->streammask >> ((3 - (addr - 0xf0)) * 8);
		} else if (addr >= 0xf4 && addr <= 0xf7) {
			v = data->streamintenamask >> ((3 - (addr - 0xf0)) * 8);
		} else if (addr >= 0xf8 && addr <= 0xfb) {
			v = data->streamintreqmask >> ((3 - (addr - 0xf0)) * 8);
		}
	}
#if DEBUG_SNDDEV_READ
	write_log(_T("uaesnd_bget %08x = %02x\n"), addr, v);
#endif
	return v;
}
static uae_u32 REGPARAM2 uaesndboard_wget(uaecptr addr)
{
	struct uaesndboard_data *data = &uaesndboard[0];
	uae_u16 v = 0;
	addr &= 65535;
	if (addr < 0x80) {
		return (uaesndboard_bget(addr) << 8) | uaesndboard_bget(addr + 1);
	} else if (data->configured) {
		if (addr & 1)
			return 0xffff;
		struct uaesndboard_stream *s = uaesnd_get(addr);
		if (s) {
			int reg = addr & 255;
			int reg4 = reg / 4;
			if (reg4 <= 4 || reg4 == 6) {
				if (!(reg & 2)) {
					s->wordlatch = get_word_host(s->io + ((reg + 2) & 255));
				} else {
					return s->wordlatch;
				}
			}
			if (reg == 0x86) {
				s->intreqmask = 0;
			}
			v = get_word_host(s->io + reg);
		} else if (addr >= 0x80 && addr < 0xe0 - 1) {
			v = get_word_host(data->info + (addr & 0x7f));
		} else if (addr == 0xe0) {
			v = data->streamallocmask >> 16;
		} else if (addr == 0xe2) {
			v = data->streamallocmask >> 0;
		} else if (addr == 0xf0) {
			v = data->streammask >> 16;
		} else if (addr == 0xf2) {
			v = data->streammask;
		} else if (addr == 0xf4) {
			v = data->streamintenamask >> 16;
		} else if (addr == 0xf6) {
			v = data->streamintenamask;
		} else if (addr == 0xf8) {
			v = data->streamintreqmask >> 16;
		} else if (addr == 0xfa) {
			v = data->streamintreqmask;
		}
	}
#if DEBUG_SNDDEV_READ
	write_log(_T("uaesnd_wget %08x = %04x\n"), addr, v);
#endif
	return v;
}
static uae_u32 REGPARAM2 uaesndboard_lget(uaecptr addr)
{
	struct uaesndboard_data *data = &uaesndboard[0];
	uae_u32 v = 0;
	addr &= 65535;
	if (addr < 0x80) {
		return (uaesndboard_wget(addr) << 16) | uaesndboard_wget(addr + 2);
	} else if (data->configured) {
		if (addr & 3)
			return 0xffffffff;
		struct uaesndboard_stream *s = uaesnd_get(addr);
		if (s) {
			int reg = addr & 255;
			if (reg == 0x84)
				s->intreqmask = 0;
			v = get_long_host(s->io + reg);
		} else if (addr >= 0x80 && addr < 0xe0 - 3) {
			v = get_long_host(data->info + (addr & 0x7f));
		} else if (addr == 0xe0) {
			v = data->streamallocmask;
		} else if (addr == 0xf0) {
			v = data->streammask;
		} else if (addr == 0xf4) {
			v = data->streamintenamask;
		} else if (addr == 0xf8) {
			v = data->streamintreqmask;
		}
	}
#if DEBUG_SNDDEV_READ
	write_log(_T("uaesnd_lget %08x = %08x\n"), addr, v);
#endif
	return v;
}
static void REGPARAM2 uaesndboard_bput(uaecptr addr, uae_u32 b)
{
	struct uaesndboard_data *data = &uaesndboard[0];
	addr &= 65535;
	if (!data->configured) {
		switch (addr) {
			case 0x48:
			if (!data->z3) {
				uae_u32 ram_start = expamem_board_pointer;
				uae_u32 ram_size = 65536;
				uaesndboard_ram_bank.start = ram_start;
				uaesndboard_ram_bank.reserved_size = ram_size;
				uaesndboard_ram_bank.mask = ram_size - 1;
				mapped_malloc(&uaesndboard_ram_bank);
				map_banks_z2(&uaesndboard_bank_z2, expamem_board_pointer >> 16, 65536 >> 16);
				uaesnd_configure(data);
				expamem_next(&uaesndboard_bank_z2, NULL);
			}
			break;
			case 0x4c:
			data->configured = -1;
			expamem_shutup(&uaesndboard_bank_z2);
			break;
		}
		return;
	} else {
		struct uaesndboard_stream *s = uaesnd_addr(addr);
#if DEBUG_SNDDEV_WRITE
		write_log(_T("uaesnd_bput %08x = %02x\n"), addr, b);
#endif
		if (s) {
			int reg = addr & 255;
			put_byte_host(s->io + reg, b);
			uaesnd_put(data, s, reg);
		} else if (addr >= 0xe0 && addr <= 0xe3) {
			uae_u32 v = data->streamallocmask;
			int shift = 8 * (3 - (addr - 0xe0));
			uae_u32 mask = 0xff;
			v &= ~(mask << shift);
			v |= b << shift;
			uaesnd_streammask(data, data->streammask & v);
			data->streamallocmask &= ~(mask << shift);
			data->streamallocmask |= v << shift;
		} else if (addr >= 0xf0 && addr <= 0xf3) {
			b <<= 8 * (3 - (addr - 0xf0));
			uaesnd_streammask(data, b);
		}
	}
}
static void REGPARAM2 uaesndboard_wput(uaecptr addr, uae_u32 b)
{
	struct uaesndboard_data *data = &uaesndboard[0];
	addr &= 65535;
	if (!data->configured) {
		switch (addr) {
			case 0x44:
			if (data->z3) {
				uae_u32 ram_start = expamem_board_pointer + 8 * 1024 * 1024;
				uae_u32 ram_size = 8 * 1024 * 1024;
				map_banks_z3(&uaesndboard_bank_z3, expamem_board_pointer >> 16, ram_size >> 16);
				uaesndboard_ram_bank.start = ram_start;
				uaesndboard_ram_bank.reserved_size = ram_size;
				uaesndboard_ram_bank.mask = ram_size - 1;
				mapped_malloc(&uaesndboard_ram_bank);
				map_banks_z3(&uaesndboard_ram_bank, ram_start >> 16, ram_size >> 16);
				uaesnd_configure(data);
				expamem_next(&uaesndboard_bank_z3, NULL);
			}
			break;
		}
		return;
	} else {
		if (addr & 1)
			return;
#if DEBUG_SNDDEV_WRITE
		write_log(_T("uaesnd_wput %08x = %04x\n"), addr, b);
#endif
		struct uaesndboard_stream *s = uaesnd_addr(addr);
		if (s) {
			int reg = addr & 255;
			put_word_host(s->io + reg, b);
			uaesnd_put(data, s, reg);
		} else if (addr == 0xe4 + 2) {
			uaesnd_latch_mask(data, b);
		} else if (addr == 0xe8 + 2) {
			uaesnd_unlatch_mask(data, b);
		} else if (addr == 0xf0) {
			uaesnd_streammask(data, b << 16);
		} else if (addr == 0xf2) {
			uaesnd_streammask(data, b);
		}
	}
}
static void REGPARAM2 uaesndboard_lput(uaecptr addr, uae_u32 b)
{
	struct uaesndboard_data *data = &uaesndboard[0];
	addr &= 65535;
	if (data->configured) {
		if (addr & 3)
			return;
#if DEBUG_SNDDEV_WRITE
		write_log(_T("uaesnd_lput %08x = %08x\n"), addr, b);
#endif
		struct uaesndboard_stream *s = uaesnd_addr(addr);
		if (s) {
			int reg = addr & 255;
			put_long_host(s->io + reg, b);
			uaesnd_put(data, s, reg);
		} else if (addr == 0xe0) {
			uaesnd_streammask(data, data->streammask & b);
			data->streamallocmask = b;
		} else if (addr == 0xe4) {
			uaesnd_latch_mask(data, b);
		} else if (addr == 0xe8) {
			uaesnd_unlatch_mask(data, b);
		} else if (addr == 0xf0) {
			uaesnd_streammask(data, b);
		} else if (addr == 0xf4) {
			data->streamintenamask = b;
		} else if (addr == 0xf8) {
			data->streamintreqmask = b;
		}
	}
}

addrbank uaesndboard_sub_bank_z2 = {
	uaesndboard_lget, uaesndboard_wget, uaesndboard_bget,
	uaesndboard_lput, uaesndboard_wput, uaesndboard_bput,
	default_xlate, default_check, NULL, NULL, _T("uaesnd z2"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO, S_READ, S_WRITE
};

static addrbank_sub uaesndz2_sub_banks[] = {
	{ &uaesndboard_sub_bank_z2, 0x0000 },
	{ &uaesndboard_ram_bank, 0x8000 },
	{ NULL }
};

addrbank uaesndboard_bank_z3 = {
	uaesndboard_lget, uaesndboard_wget, uaesndboard_bget,
	uaesndboard_lput, uaesndboard_wput, uaesndboard_bput,
	default_xlate, default_check, NULL, NULL, _T("uaesnd z3"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO, S_READ, S_WRITE
};

addrbank uaesndboard_bank_z2 = {
	sub_bank_lget, sub_bank_wget, sub_bank_bget,
	sub_bank_lput, sub_bank_wput, sub_bank_bput,
	default_xlate, default_check, NULL, NULL, _T("uaesnd z2"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO, S_READ, S_WRITE, uaesndz2_sub_banks
};

static void ew(uae_u8 *acmemory, int addr, uae_u32 value)
{
	addr &= 0xffff;
	if (addr == 00 || addr == 02 || addr == 0x40 || addr == 0x42) {
		acmemory[addr] = (value & 0xf0);
		acmemory[addr + 2] = (value & 0x0f) << 4;
	} else {
		acmemory[addr] = ~(value & 0xf0);
		acmemory[addr + 2] = ~((value & 0x0f) << 4);
	}
}

bool uaesndboard_init (struct autoconfig_info *aci, int z)
{
	struct uaesndboard_data *data = &uaesndboard[0];

	device_add_reset(uaesndboard_reset);

	if (aci->doinit)
		snd_init();

	data->configured = 0;
	data->enabled = true;
	data->z3 = z == 3;
	memset(data->acmemory, 0xff, sizeof data->acmemory);
	const struct expansionromtype *ert = get_device_expansion_rom(z == 3 ? ROMTYPE_UAESNDZ3 : ROMTYPE_UAESNDZ2);
	if (!ert)
		return false;
	data->streammask = 0;
	data->volume[0] = 32768;
	data->volume[1] = 32768;
	put_long_host(data->info + 0, (UAEMAJOR << 16) | (UAEMINOR << 8) | (UAESUBREV));
	put_long_host(data->info + 4, (1 << 16) | (0));
	put_long_host(data->info + 8, currprefs.sound_freq);
	put_byte_host(data->info + 12, MAX_UAE_CHANNELS);
	put_byte_host(data->info + 13, get_audio_nativechannels(currprefs.sound_stereo));
	put_byte_host(data->info + 14, MAX_UAE_STREAMS);
	put_long_host(data->info + 16, data->z3 ? 8 * 1024 * 1024 : 0x8000);
	put_long_host(data->info + 20, data->z3 ? 8 * 1024 * 1024 : 0x8000);
	for (int i = 0; i < 16; i++) {
		uae_u8 b = ert->autoconfig[i];
		ew(data->acmemory, i * 4, b);
	}

	memcpy(aci->autoconfig_raw, data->acmemory, sizeof data->acmemory);
	aci->addrbank = data->z3 ? &uaesndboard_bank_z3 : &uaesndboard_bank_z2;
	return true;
}

bool uaesndboard_init_z2(struct autoconfig_info *aci)
{
	return uaesndboard_init(aci, 2);
}
bool uaesndboard_init_z3(struct autoconfig_info *aci)
{
	return uaesndboard_init(aci, 3);
}

static void uaesndboard_free(void)
{
	for (int j = 0; j < MAX_DUPLICATE_SOUND_BOARDS; j++) {
		struct uaesndboard_data *data = &uaesndboard[j];
		data->enabled = false;
	}
	mapped_free(&uaesndboard_ram_bank);
	sndboard_rethink();
}

static void uaesndboard_reset(int hardreset)
{
	for (int j = 0; j < MAX_DUPLICATE_SOUND_BOARDS; j++) {
		struct uaesndboard_data *data = &uaesndboard[j];
		if (data->enabled) {
			for (int i = 0; i < MAX_UAE_STREAMS; i++) {
				if (data->stream[i].streamid) {
					audio_enable_stream(false, data->stream[i].streamid, 0, NULL, NULL);
					memset(&data->stream[i], 0, sizeof(struct uaesndboard_stream));
				}
			}
		}
		data->streammask = 0;
	}
	mapped_free(&uaesndboard_ram_bank);
	sndboard_rethink();
}


// PMX

struct pmx_data
{
	bool enabled;
	int configured;
	uae_u8 acmemory[128];
	int streamid;
	struct romconfig *rc;
	int reset_delay;
	uae_u16 status;
	bool dreq;
	uae_u16 regs[16];
};
static struct pmx_data pmx[MAX_DUPLICATE_SOUND_BOARDS];

static void pmx_reset_chip(struct pmx_data *data)
{
	for (int i = 0; i < 16; i++) {
		data->regs[i] = 0;
	}
	data->regs[0] = 0x4000;
	data->regs[1] = 0x000c;
}

static void REGPARAM2 pmx_bput(uaecptr addr, uae_u32 v)
{
	struct pmx_data *data = &pmx[0];
	v &= 0xff;
	write_log(_T("PMXBPUT %08x %02x %08x\n"), addr, v, M68K_GETPC);
}

static void REGPARAM2 pmx_wput(uaecptr addr, uae_u32 v)
{
	struct pmx_data *data = &pmx[0];
	int reg = -1;
	v &= 0xffff;
	if (addr & 0x8000) {
		reg = (addr >> 2) & 15;
		data->regs[reg] = v;
	} else {
		data->status = v;
		if (v & 0x8000) {
			data->dreq = true;
			data->reset_delay = 10;
		}
	}
	write_log(_T("PMXWPUT %d %08x %04x %08x\n"), reg, addr, v, M68K_GETPC);
}

static void REGPARAM2 pmx_lput(uaecptr addr, uae_u32 v)
{
	write_log(_T("PMXLPUT %08x %08x %08x\n"), addr, v, M68K_GETPC);
}

static uae_u32 REGPARAM2 pmx_bget(uaecptr addr)
{
	struct pmx_data *data = &pmx[0];
	uae_u8 v = 0;
	data->dreq = !data->dreq;
	if (!data->dreq)
		v |= 1 << 3;
	write_log(_T("PMXBGET %08x %02x %08x\n"), addr, v, M68K_GETPC);
	return v;
}
static uae_u32 REGPARAM2 pmx_wget(uaecptr addr)
{
	struct pmx_data *data = &pmx[0];
	uae_u16 v = 0;
	int reg = -1;
	if (addr & 0x8000) {
		reg = (addr >> 2) & 15;
		v = data->regs[reg];
		if (reg == 1) {
			v &= ~0x03f0;
			v |= 0x0060; ;//revision
		}
	} else {
		v = data->status;
	}
	write_log(_T("PMXWGET %d %08x %04x %08x\n"), reg, addr, v, M68K_GETPC);
	return v;
}
static uae_u32 REGPARAM2 pmx_lget(uaecptr addr)
{
	write_log(_T("PMXLGET %08x %08x\n"), addr, M68K_GETPC);
	return 0;
}

static addrbank pmx_bank = {
	pmx_lget, pmx_wget, pmx_bget,
	pmx_lput, pmx_wput, pmx_bput,
	default_xlate, default_check, NULL, _T("*"), _T("PMX"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO | ABFLAG_SAFE, S_READ, S_WRITE
};

bool pmx_init (struct autoconfig_info *aci)
{
	struct pmx_data *data = &pmx[0];
	const struct expansionromtype *ert = get_device_expansion_rom(ROMTYPE_PMX);
	if (!ert)
		return false;

	aci->addrbank = &pmx_bank;
	aci->autoconfig_automatic = true;
	device_add_reset(sndboard_reset);

	if (!aci->doinit) {
		aci->autoconfigp = ert->autoconfig;
		return true;
	}

	snd_init();

	data->configured = 0;
	data->streamid = 0;
	memset(data->acmemory, 0xff, sizeof data->acmemory);
	data->rc = aci->rc;
	data->enabled = true;
	for (int i = 0; i < 16; i++) {
		uae_u8 b = ert->autoconfig[i];
		ew(data->acmemory, i * 4, b);
	}
	memcpy(aci->autoconfig_raw, data->acmemory, sizeof data->acmemory);
	return true;
}

static void pmx_free(void)
{
	for (int j = 0; j < MAX_DUPLICATE_SOUND_BOARDS; j++) {
		struct pmx_data *data = &pmx[j];
		data->enabled = false;
	}
	sndboard_rethink();
}

static void pmx_reset(int hardreset)
{
	for (int j = 0; j < MAX_DUPLICATE_SOUND_BOARDS; j++) {
		struct pmx_data *data = &pmx[j];
		if (data->enabled) {
		}
	}
	sndboard_rethink();
}

// TOCCATA/PRELUDE

#define SNDDEV_TOCCATA 0
#define SNDDEV_PRELUDE 1
#define SNDDEV_PRELUDE1200 2

#define BOARD_SIZE 65536
#define BOARD_MASK (BOARD_SIZE - 1)

#define FIFO_SIZE_MAX 1024

struct snddev_data {
	bool enabled;
	int type;
	uae_u8 acmemory[128];
	int configured;
	uae_u32 baseaddress;
	uae_u32 baseaddress_mask, baseaddress_value;
	uae_u8 ad1848_index;
	uae_u8 ad1848_index_mask;
	uae_u8 ad1848_regs[32];
	uae_u8 ad1848_status;
	int autocalibration;
	uae_u8 snddev_status;
	int snddev_irq;
	int fifo_read_index;
	int fifo_write_index;
	int data_in_fifo;
	int fifo_size;
	uae_u8 fifo[FIFO_SIZE_MAX];
	bool fifo_play_byteswap, fifo_record_byteswap;

	int fifo_record_read_index;
	int fifo_record_write_index;
	int data_in_record_fifo;
	uae_u8 record_fifo[FIFO_SIZE_MAX];

	int streamid;
	int ch_sample[2];

	uae_u16 codec_reg1_mask;
	uae_u16 codec_reg1_addr;
	uae_u16 codec_reg2_mask;
	uae_u16 codec_reg2_addr;
	uae_u16 codec_fifo_mask;
	uae_u16 codec_fifo_addr;

	int fifo_half;
	int snddev_active;
	int left_volume, right_volume;

	int freq, freq_adjusted;
	int play_channels, play_samplebits;
	int record_channels, record_samplebits;
	int event_time, record_event_time;
	int record_event_counter;
	int play_bytespersample, record_bytespersample;

	int capture_buffer_size;
	int capture_read_index, capture_write_index;
	uae_u8 *capture_buffer;

	struct romconfig *rc;
	addrbank *bank;
};

static struct snddev_data snddev[MAX_SNDDEVS];

extern addrbank toccata_bank;

#define STATUS_ACTIVE 1
#define STATUS_RESET 2
#define STATUS_FIFO_CODEC 4
#define STATUS_FIFO_RECORD 8
#define STATUS_FIFO_PLAY 0x10
#define STATUS_RECORD_INTENA 0x40
#define STATUS_PLAY_INTENA 0x80

#define STATUS_READ_INTREQ 128
#define STATUS_READ_PLAY_HALF 8
#define STATUS_READ_RECORD_HALF 4


void update_sndboard_sound (float clk)
{
	base_event_clock = clk;
}

static void process_fifo(struct snddev_data *data)
{
	int prev_data_in_fifo = data->data_in_fifo;
	if (data->data_in_fifo >= data->play_bytespersample) {
		uae_s16 v;
		if (data->play_samplebits == 8) {
			v = data->fifo[data->fifo_read_index] << 8;
			v |= data->fifo[data->fifo_read_index];
			data->ch_sample[0] = v;
			if (data->play_channels == 2) {
				v = data->fifo[data->fifo_read_index + 1] << 8;
				v |= data->fifo[data->fifo_read_index + 1];
			}
			data->ch_sample[1] = v;
		} else if (data->play_samplebits == 16) {
			if (data->fifo_play_byteswap) {
				v = data->fifo[data->fifo_read_index + 0] << 8;
				v |= data->fifo[data->fifo_read_index + 1];
			} else {
				v = data->fifo[data->fifo_read_index + 1] << 8;
				v |= data->fifo[data->fifo_read_index + 0];
			}
			data->ch_sample[0] = v;
			if (data->play_channels == 2) {
				if (data->fifo_play_byteswap) {
					v = data->fifo[data->fifo_read_index + 2] << 8;
					v |= data->fifo[data->fifo_read_index + 3];
				} else {
					v = data->fifo[data->fifo_read_index + 3] << 8;
					v |= data->fifo[data->fifo_read_index + 2];
				}
			}
			data->ch_sample[1] = v;
		}
		data->data_in_fifo -= data->play_bytespersample;
		data->fifo_read_index += data->play_bytespersample;
		data->fifo_read_index = data->fifo_read_index % data->fifo_size;
	} else if (data->data_in_fifo > 0) {
		data->data_in_fifo = 0;
	}
	data->ch_sample[0] = data->ch_sample[0] * data->left_volume / 32768;
	data->ch_sample[1] = data->ch_sample[1] * data->right_volume / 32768;

	if (data->data_in_fifo < data->fifo_size / 2 && prev_data_in_fifo >= data->fifo_size / 2)
		data->fifo_half |= STATUS_FIFO_PLAY;
}

static bool audio_state_sndboard_toccata(int streamid, void *cb)
{
	struct snddev_data *data = (struct snddev_data*)cb;
	if (!data->snddev_active)
		return false;
	if (data->streamid != streamid)
		return false;
	if ((data->snddev_active & STATUS_FIFO_PLAY)) {
		// get all bytes at once to prevent fifo going out of sync
		// if fifo has for example 3 bytes remaining but we need 4.
		process_fifo(data);
	}
	if (data->type == SNDDEV_TOCCATA) {
		int old = data->snddev_irq;
		if (data->snddev_active && (data->snddev_status & STATUS_FIFO_CODEC)) {
			if ((data->fifo_half & STATUS_FIFO_PLAY) && (data->snddev_status & STATUS_PLAY_INTENA) && (data->snddev_status & STATUS_FIFO_PLAY)) {
				data->snddev_irq |= STATUS_READ_PLAY_HALF;
			}
			if ((data->fifo_half & STATUS_FIFO_RECORD) && (data->snddev_status & STATUS_RECORD_INTENA) && (data->snddev_status & STATUS_FIFO_RECORD)) {
				data->snddev_irq |= STATUS_READ_RECORD_HALF;
			}
		}
		if (old != data->snddev_irq) {
			devices_rethink_all(sndboard_rethink);
#if DEBUG_SNDDEV > 2
			write_log(_T("SNDDEV IRQ\n"));
#endif
		}
	}
	audio_state_stream_state(streamid, data->ch_sample, 2, data->event_time);
	return true;
}

static int get_volume(uae_u8 v)
{
	int out;
	if (v & 0x80) // Mute bit
		return 0;
	out = v & 63;
	out = 64 - out;
	out *= 32768 / 64;
	return out;
}

static int get_volume_in(uae_u8 v)
{
	int out;
	if (v & 0x80) // Mute bit
		return 0;
	out = v & 31;
	out = 32 - out;
	out *= 32768 / 32;
	return out;
}

static void calculate_volume_toccata(struct snddev_data *data)
{
	data->left_volume = (100 - currprefs.sound_volume_board) * 32768 / 100;
	data->right_volume = (100 - currprefs.sound_volume_board) * 32768 / 100;

	data->left_volume = get_volume(data->ad1848_regs[6]) * data->left_volume / 32768;
	data->right_volume = get_volume(data->ad1848_regs[7]) * data->right_volume / 32768;

	if (data->rc->device_settings & 1) {
		sound_paula_volume[0] = get_volume_in(data->ad1848_regs[4]);
		sound_paula_volume[1] = get_volume_in(data->ad1848_regs[5]);

		sound_cd_volume[0] = get_volume_in(data->ad1848_regs[2]);
		sound_cd_volume[1] = get_volume_in(data->ad1848_regs[3]);
	}
}

static const int freq_crystals[] = {
	// AD1848 documentation says 24.576MHz but photo of board shows 24.582MHz
	// Added later: It seems there are boards that have correct crystal and
	// also boards with wrong crystal..
	// So we can use correct one in emulation.
	24576000,
	16934400
};
static const int freq_dividers[] = {
	3072,
	1536,
	896,
	768,
	448,
	384,
	512,
	2560
};

static void codec_setup(struct snddev_data *data)
{
	uae_u8 c = data->ad1848_regs[8];

	data->play_channels = (c & 0x10) ? 2 : 1;
	data->play_samplebits = (c & 0x40) ? 16 : 8;
	data->freq = freq_crystals[c & 1] / freq_dividers[(c >> 1) & 7];
	data->freq_adjusted = ((data->freq + 49) / 100) * 100;
	data->play_bytespersample = (data->play_samplebits / 8) * data->play_channels;

	data->record_channels = data->play_channels;
	data->record_samplebits = data->play_samplebits;
	if (data->ad1848_regs[12] & 0x40) {
		uae_u8 r = data->ad1848_regs[28];
		data->record_channels = (r & 0x10) ? 2 : 1;
		data->record_samplebits = (r & 0x40) ? 16 : 8;
		data->fifo_record_byteswap = (r & 0x80) != 0;
	}
	data->record_bytespersample = (data->record_samplebits / 8) * data->record_channels;

	write_log(_T("SNDDEV start %s freq=%d bits=%d channels=%d\n"),
		((data->snddev_active & (STATUS_FIFO_PLAY | STATUS_FIFO_RECORD)) == (STATUS_FIFO_PLAY | STATUS_FIFO_RECORD)) ? _T("Play+Record") :
		(data->snddev_active & STATUS_FIFO_PLAY) ? _T("Play") : _T("Record"),
		data->freq, data->play_samplebits, data->play_channels);
}

static void codec_start(struct snddev_data *data)
{
	data->snddev_active  = (data->ad1848_regs[9] & 1) ? STATUS_FIFO_PLAY : 0;
	data->snddev_active |= (data->ad1848_regs[9] & 2) ? STATUS_FIFO_RECORD : 0;

	codec_setup(data);

	data->event_time = (int)(base_event_clock * CYCLE_UNIT / data->freq);
	data->record_event_time = (int)(base_event_clock * CYCLE_UNIT / (data->freq_adjusted * data->record_bytespersample));
	data->record_event_counter = 0;

	if (data->snddev_active & STATUS_FIFO_PLAY) {
		data->streamid = audio_enable_stream(true, -1, 2, audio_state_sndboard_toccata, data);
	}
	if (data->snddev_active & STATUS_FIFO_RECORD) {
#ifdef _WIN32
		data->capture_buffer_size = 48000 * 2 * 2; // 1s at 48000/stereo/16bit
		data->capture_buffer = xcalloc(uae_u8, data->capture_buffer_size);
		sndboard_init_capture(data->freq_adjusted);
#endif
	}
}

static void codec_stop(struct snddev_data *data)
{
	if (!data->snddev_active)
		return;
	write_log(_T("CODEC stop\n"));
	data->snddev_active = 0;
#ifdef _WIN32
	sndboard_free_capture();
#endif
	int streamid = data->streamid;
	data->streamid = 0;
	audio_enable_stream(false, streamid, 0, NULL, NULL);
	xfree(data->capture_buffer);
	data->capture_buffer = NULL;
}

static void sndboard_rethink(void)
{
	for (int i = 0; i < MAX_SNDDEVS; i++) {
		if (snddev[i].enabled) {
			struct snddev_data *data = &snddev[i];
			bool irq = data->snddev_irq != 0;
			if (irq) {
				safe_interrupt_set(IRQ_SOURCE_SOUND, 0, true);
			}
		}
	}
	if (uaesndboard[0].enabled) {
		bool irq = uaesnd_rethink();
		if (irq) {
			safe_interrupt_set(IRQ_SOURCE_SOUND, 1, true);
		}
	}
}

static void sndboard_process_capture(struct snddev_data *data)
{
#ifdef _WIN32
	int frames;
	uae_u8 *buffer = sndboard_get_buffer(&frames);
	if (buffer && frames) {
		uae_u8 *p = buffer;
		int bytes = frames * 4;
		if (bytes >= data->capture_buffer_size - data->capture_write_index) {
			memcpy(data->capture_buffer + data->capture_write_index, p, data->capture_buffer_size - data->capture_write_index);
			p += data->capture_buffer_size - data->capture_write_index;
			bytes -= data->capture_buffer_size - data->capture_write_index;
			data->capture_write_index = 0;
		}
		if (bytes > 0 && bytes < data->capture_buffer_size - data->capture_write_index) {
			memcpy(data->capture_buffer + data->capture_write_index, p, bytes);
			data->capture_write_index += bytes;
		}
	}
	sndboard_release_buffer(buffer, frames);
#endif
}

static void check_prelude_interrupt(struct snddev_data *data)
{
	int oldirq = data->snddev_irq;
	if (data->data_in_fifo < data->fifo_size / 2 && data->data_in_fifo > 0) {
		data->snddev_irq |= STATUS_READ_PLAY_HALF;
	} else {
		data->snddev_irq &= ~STATUS_READ_PLAY_HALF;
	}
	if (oldirq != data->snddev_irq) {
		devices_rethink_all(sndboard_rethink);
	}
}

static void sndboard_hsync(void)
{
	for (int i = 0; i < MAX_SNDDEVS; i++) {
		struct snddev_data *data = &snddev[i];
		static int capcnt[MAX_SNDDEVS];

		if (!data->configured)
			continue;
		if (data->autocalibration > 0)
			data->autocalibration--;

		if (data->type == SNDDEV_PRELUDE || data->type == SNDDEV_PRELUDE1200) {
			check_prelude_interrupt(data);
		}

		if (data->snddev_active & STATUS_FIFO_RECORD) {

			capcnt[i]--;
			if (capcnt[i] <= 0) {
				sndboard_process_capture(data);
				capcnt[i] = data->record_event_time * 312 / (maxhpos * CYCLE_UNIT);
			}

			data->record_event_counter += maxhpos * CYCLE_UNIT;
			int bytes = data->record_event_counter / data->record_event_time;
			bytes &= ~3;
			if (bytes < 64 || data->capture_read_index == data->capture_write_index)
				return;

			int oldfifo = data->data_in_record_fifo;
			int oldbytes = bytes;
			int size = data->fifo_size - data->data_in_record_fifo;
			while (size > 0 && data->capture_read_index != data->capture_write_index && bytes > 0) {
				uae_u8 *fifop = &data->fifo[data->fifo_record_write_index];
				uae_u8 *bufp = &data->capture_buffer[data->capture_read_index];

				if (data->record_samplebits == 8) {
					fifop[0] = bufp[1];
					data->fifo_record_write_index++;
					data->data_in_record_fifo++;
					size--;
					bytes--;
					if (data->record_channels == 2) {
						fifop[1] = bufp[3];
						data->fifo_record_write_index++;
						data->data_in_record_fifo++;
						size--;
						bytes--;
					}
				} else if (data->record_samplebits == 16) {
					if (data->fifo_record_byteswap) {
						fifop[0] = bufp[0];
						fifop[1] = bufp[1];
					} else {
						fifop[0] = bufp[1];
						fifop[1] = bufp[0];
					}
					data->fifo_record_write_index += 2;
					data->data_in_record_fifo += 2;
					size -= 2;
					bytes -= 2;
					if (data->record_channels == 2) {
						if (data->fifo_record_byteswap) {
							fifop[2] = bufp[2];
							fifop[3] = bufp[3];
						} else {
							fifop[2] = bufp[3];
							fifop[3] = bufp[2];
						}
						data->fifo_record_write_index += 2;
						data->data_in_record_fifo += 2;
						size -= 2;
						bytes -= 2;
					}
				}

				data->fifo_record_write_index %= data->fifo_size;
				data->capture_read_index += 4;
				if (data->capture_read_index >= data->capture_buffer_size)
					data->capture_read_index = 0;
			}

			//write_log(_T("%d %d %d %d\n"), capture_read_index, capture_write_index, size, bytes);

			if (data->data_in_record_fifo > data->fifo_size / 2 && oldfifo <= data->fifo_size / 2) {
				data->fifo_half |= STATUS_FIFO_RECORD;
				//audio_state_sndboard(-1, -1);
			}
			data->record_event_counter -= oldbytes * data->record_event_time;
		}
	}
}

static void sndboard_vsync_toccata(struct snddev_data *data)
{
	if (data->snddev_active) {
		calculate_volume_toccata(data);
		audio_activate();
	}
}

static void toccata_put(struct snddev_data *data, uaecptr addr, uae_u8 v)
{
	int idx = data->ad1848_index & data->ad1848_index_mask;
	bool hit = true;

#if DEBUG_SNDDEV > 2
	if ((addr & data->codec_fifo_mask) != data->codec_fifo_addr)
		write_log(_T("SNDDEV PUT %08x %02x %d PC=%08X\n"), addr, v, idx, M68K_GETPC);
#endif

	if (idx >= 16 && !(data->ad1848_regs[12] & 0x40))
		return;

	if ((addr & data->codec_reg1_mask) == data->codec_reg1_addr) {
		// AD1848 register 0
		data->ad1848_index = v;
	} else 	if ((addr & data->codec_reg2_mask) == data->codec_reg2_addr) {
		// AD1848 register 1
		uae_u8 old = data->ad1848_regs[idx];
		
		switch(idx)
		{
			case 12:
			// revision (AD1848) or revision + mode (CS4231A)
			if (data->type == SNDDEV_TOCCATA) {
				v = 0x0a;
			} else if (data->type == SNDDEV_PRELUDE || data->type == SNDDEV_PRELUDE1200) {
				v &= 0xf0;
				v |= 0x8a;
			}
			break;
			case 8:
			if (data->ad1848_regs[12] & 0x40) {
				// CS4231A only big endian 16-bit data format?
				data->fifo_play_byteswap = (v >> 5) == 6;
			} else {
				data->fifo_play_byteswap = false;
				v &= ~0x80;
			}
			data->fifo_record_byteswap = data->fifo_record_byteswap;
			break;
		} 

		data->ad1848_regs[idx] = v;
#if DEBUG_SNDDEV > 0
		write_log(_T("SNDDEV PUT reg %d = %02x PC=%08x\n"), idx, v, M68K_GETPC);
#endif
		switch(idx)
		{
			case 9:
			if (v & 8) // ACI enabled
				data->autocalibration = 50;
			if (!(old & 3) && (v & 3))
				codec_start(data);
			else if ((old & 3) && !(v & 3))
				codec_stop(data);
			break;

			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
				calculate_volume_toccata(data);
			break;

		}
	} else if ((addr & data->codec_fifo_mask) == data->codec_fifo_addr) {
#if DEBUG_SNDDEV_FIFO
		write_log(_T("SNDDEV FIFO PUT %02x (%d %d) PC=%08x\n"), v, data->fifo_write_index, data->data_in_fifo, M68K_GETPC);
#endif
		// FIFO input
		if ((data->snddev_status & STATUS_FIFO_PLAY) || (data->type == SNDDEV_PRELUDE || data->type == SNDDEV_PRELUDE1200)) {
			// 7202LA datasheet says fifo can't overflow
			if (data->data_in_fifo < data->fifo_size) {
				data->fifo[data->fifo_write_index] = v;
				data->fifo_write_index++;
				data->fifo_write_index %= data->fifo_size;
				data->data_in_fifo++;
			}
			if (data->type == SNDDEV_PRELUDE || data->type == SNDDEV_PRELUDE1200)
				check_prelude_interrupt(data);
		}
		data->snddev_irq &= ~STATUS_READ_PLAY_HALF;
		data->fifo_half &= ~STATUS_FIFO_PLAY;
	} else if (data->type == SNDDEV_TOCCATA) {
		if ((addr & 0x6800) == 0x0000) {
			// Board status
			if (v & STATUS_RESET) {
				codec_stop(data);
				data->snddev_status = 0;
				data->snddev_irq = 0;
				v = 0;
			}
			if (v == STATUS_ACTIVE) {
				data->fifo_write_index = 0;
				data->fifo_read_index = 0;
				data->data_in_fifo = 0;
				data->snddev_status = 0;
				data->snddev_irq = 0;
				data->fifo_half = 0;
			}
			data->snddev_status = v;
#if DEBUG_SNDDEV > 0
			write_log(_T("TOCCATA PUT STATUS %08x %02x %d PC=%08X\n"), addr, v, idx, M68K_GETPC);
#endif
		} else {
			hit = false;
		}
	} else if (data->type == SNDDEV_PRELUDE) {
		if ((addr & 0x00ff) == 0x8A) { // Reset FIFOs and CS4231A
			codec_stop(data);
			data->snddev_status = 0;
			data->snddev_irq = 0;
			data->fifo_write_index = 0;
			data->fifo_read_index = 0;
			data->data_in_fifo = 0;
			data->fifo_half = 0;
		} else if (addr < 0x90 || addr >= 0xc0) {
			hit = false;
		}
	} else if (data->type == SNDDEV_PRELUDE1200) {

		if ((addr & data->baseaddress_mask) == data->baseaddress_value) {
			if ((addr & 0x00ff) == 0x15) { // Reset FIFOs and CS4231A
				codec_stop(data);
				data->snddev_status = 0;
				data->snddev_irq = 0;
				data->fifo_write_index = 0;
				data->fifo_read_index = 0;
				data->data_in_fifo = 0;
				data->fifo_half = 0;
			} else if ((addr & 0x00ff) == 0x19) { // ?
				;
			} else {
				hit = false;
			}
		} else {
			hit = false;
		}
	} else {
		hit = false;
	}
	if (!hit) {
		write_log(_T("SNDDEV PUT UNKNOWN %08x PC=%08x\n"), addr, M68K_GETPC);
	}
}

static uae_u8 toccata_get(struct snddev_data *data, uaecptr addr)
{
	int idx = data->ad1848_index & data->ad1848_index_mask;
	uae_u8 v = 0;
	bool hit = true;

	if (idx >= 16 && !(data->ad1848_regs[12] & 0x40))
		return v;

	if ((addr & data->codec_reg1_mask) == data->codec_reg1_addr) {
		// AD1848 register 0
		v = data->ad1848_index;
	} else 	if ((addr & data->codec_reg2_mask) == data->codec_reg2_addr) {
		// AD1848 register 1
		v = data->ad1848_regs[idx];
#if DEBUG_SNDDEV > 0
		write_log(_T("SNDDEV GET reg %d = %02x PC=%08x\n"), idx, v, M68K_GETPC);
#endif
		switch (idx)
		{
			case 11:
			if (data->autocalibration > 10 && data->autocalibration < 30)
				data->ad1848_regs[11] |= 0x20;
			else
				data->ad1848_regs[11] &= ~0x20;
			break;
		}
	} else if ((addr & data->codec_fifo_mask) == data->codec_fifo_addr) {
		// FIFO output
		v = data->fifo[data->fifo_record_read_index];
#if DEBUG_SNDDEV_FIFO
		write_log(_T("SNDDEV FIFO GET %02x (%d %d) PC=%08x\n"), v, data->fifo_record_read_index, data->data_in_fifo, M68K_GETPC);
#endif
		if ((data->snddev_status & STATUS_FIFO_RECORD) || (data->type == SNDDEV_PRELUDE || data->type == SNDDEV_PRELUDE1200)) {
			if (data->data_in_record_fifo > 0) {
				data->fifo_record_read_index++;
				data->fifo_record_read_index %= data->fifo_size;
				data->data_in_record_fifo--;
			}
			if (data->type == SNDDEV_PRELUDE || data->type == SNDDEV_PRELUDE1200) {
				check_prelude_interrupt(data);
			}
		}
		data->snddev_irq &= ~STATUS_READ_RECORD_HALF;
		data->fifo_half &= ~STATUS_FIFO_RECORD;
	} else if (data->type == SNDDEV_TOCCATA) {
		if ((addr & 0x6800) == 0x0000) {
			// Board status
			v = STATUS_READ_INTREQ; // active low
			if (data->snddev_irq) {
				v &= ~STATUS_READ_INTREQ;
				v |= data->snddev_irq;
				data->snddev_irq = 0;
			}
#if DEBUG_SNDDEV > 0
			write_log(_T("TOCCATA GET STATUS %08x %02x %d PC=%08X\n"), addr, v, idx, M68K_GETPC);
#endif
		} else {
			hit = false;
		}
	} else if (data->type == SNDDEV_PRELUDE) {

		if ((addr & 0x00ff) == 0x8e) { // DMA busy flag
			v = 0xff;
		} else if ((addr & 0x00ff) == 0x8c) { // FIFO Status
			if (data->data_in_fifo >= data->fifo_size / 2)
				v |= 0x80;
			if (!data->data_in_fifo)
				v |= 0x40;
			if (data->data_in_record_fifo >= data->fifo_size / 2)
				v |= 0x20;
			if (data->data_in_record_fifo >= data->fifo_size)
				v |= 0x10;
			v ^= 0xff;
		} else if ((addr >= 0x80 && addr <= 0x90) || addr >= 0xc0) {
			hit = false;
		}

	} else if (data->type == SNDDEV_PRELUDE1200) {

		if ((addr & data->baseaddress_mask) == data->baseaddress_value) {
			if ((addr & 0x00ff) == 0x15) { // FIFO Status
				if (data->data_in_fifo >= data->fifo_size / 2)
					v |= 0x08;
				if (!data->data_in_fifo)
					v |= 0x04;
				if (data->data_in_record_fifo >= data->fifo_size / 2)
					v |= 0x02;
				if (data->data_in_record_fifo >= data->fifo_size)
					v |= 0x01;
				v ^= 0xff;
			} else if ((addr & 0x00ff) == 0x1d) { // id?
				v = 0x05;
			} else if ((addr & 0x00ff) == 0x19) { // ?
				v = 0x00;
			} else {
				hit = false;
			}
		} else {
			hit = false;
		}

	} else {
		hit = false;
	}
	if (!hit) {
		write_log(_T("SNDDEV GET UNKNOWN %08x PC=%08x\n"), addr, M68K_GETPC);
	}

#if DEBUG_SNDDEV > 2
	write_log(_T("SNDDEV GET %08x %02x %d PC=%08X\n"), addr, v, idx, M68K_GETPC);
#endif
	return v;
}

static struct snddev_data *getsnddev(uaecptr addr)
{
	for (int i = 0; i < MAX_SNDDEVS; i++) {
		struct snddev_data *data = &snddev[i];
		if (data->bank) {
			if (!data->configured)
				return data;
			if (data->baseaddress == (addr & 0xffff0000))
				return data;
		}
	}
	return NULL;
}

static void REGPARAM2 toccata_bput(uaecptr addr, uae_u32 b)
{
	addr &= 0xffffff;
	struct snddev_data *data = getsnddev(addr);
	if (!data)
		return;
	b &= 0xff;
	addr &= BOARD_MASK;
	if (!data->configured) {
		switch (addr)
		{
			case 0x48:
			map_banks_z2(&toccata_bank, expamem_board_pointer >> 16, BOARD_SIZE >> 16);
			data->configured = 1;
			data->baseaddress = expamem_board_pointer;
			expamem_next(&toccata_bank, NULL);
			break;
			case 0x4c:
			data->configured = -1;
			expamem_shutup(&toccata_bank);
			break;
		}
		return;
	}
	if (data->configured > 0)
		toccata_put(data, addr, b);
}

static void REGPARAM2 toccata_wput(uaecptr addr, uae_u32 b)
{
	toccata_bput(addr + 0, b >> 8);
	toccata_bput(addr + 1, b >> 0);
}

static void REGPARAM2 toccata_lput(uaecptr addr, uae_u32 b)
{
	toccata_bput(addr + 0, b >> 24);
	toccata_bput(addr + 1, b >> 16);
	toccata_bput(addr + 2, b >>  8);
	toccata_bput(addr + 3, b >>  0);
}

static uae_u32 REGPARAM2 toccata_bget(uaecptr addr)
{
	uae_u8 v = 0;
	addr &= 0xffffff;
	struct snddev_data *data = getsnddev(addr);
	if (!data)
		return v;
	addr &= BOARD_MASK;
	if (!data->configured) {
		if (addr >= sizeof data->acmemory)
			return 0;
		return data->acmemory[addr];
	}
	if (data->configured > 0)
		v = toccata_get(data, addr);
	return v;
}
static uae_u32 REGPARAM2 toccata_wget(uaecptr addr)
{
	uae_u16 v;
	v = toccata_bget(addr) << 8;
	v |= toccata_bget(addr + 1) << 0;
	return v;
}
static uae_u32 REGPARAM2 toccata_lget(uaecptr addr)
{
	uae_u32 v;
	v = toccata_bget(addr) << 24;
	v |= toccata_bget(addr + 1) << 16;
	v |= toccata_bget(addr + 2) << 8;
	v |= toccata_bget(addr + 3) << 0;
	return v;
}

addrbank toccata_bank = {
	toccata_lget, toccata_wget, toccata_bget,
	toccata_lput, toccata_wput, toccata_bput,
	default_xlate, default_check, NULL, _T("*"), _T("Toccata"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO, S_READ, S_WRITE
};

addrbank prelude_bank = {
	toccata_lget, toccata_wget, toccata_bget,
	toccata_lput, toccata_wput, toccata_bput,
	default_xlate, default_check, NULL, _T("*"), _T("Prelude"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO, S_READ, S_WRITE
};

addrbank prelude1200_bank = {
	toccata_lget, toccata_wget, toccata_bget,
	toccata_lput, toccata_wput, toccata_bput,
	default_xlate, default_check, NULL, _T("*"), _T("Prelude1200"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO, S_READ, S_WRITE
};;

static void ad1848_init(struct snddev_data *data)
{
	memset(data->ad1848_regs, 0, sizeof data->ad1848_regs);
	data->ad1848_regs[2] = 0x80;
	data->ad1848_regs[3] = 0x80;
	data->ad1848_regs[4] = 0x80;
	data->ad1848_regs[5] = 0x80;
	data->ad1848_regs[6] = 0x80;
	data->ad1848_regs[7] = 0x80;
	data->ad1848_regs[9] = 0x10;
	data->ad1848_regs[12] = 0x0a;
	data->ad1848_regs[25] = 0xa0;
	data->ad1848_status = 0xcc;
	data->ad1848_index = 0x40;
	data->ad1848_index_mask = 15;
	data->fifo_play_byteswap = false;
	data->fifo_record_byteswap = false;
}

bool prelude1200_init(struct autoconfig_info *aci)
{
	struct snddev_data *data = &snddev[2];

	aci->addrbank = &prelude1200_bank;
	aci->start = 0xd80000;
	aci->size = 0x10000;
	if (aci->devnum > 0) {
		aci->start = 0xd80000 + (aci->devnum - 1) * 0x4000;
		aci->size = 0x4000;
	}
	device_add_reset(sndboard_reset);

	if (!aci->doinit)
		return true;

	snd_init();

	data->configured = 1;
	data->baseaddress = 0xd80000;
	data->baseaddress_mask = 0;
	data->baseaddress_value = 0;
	if (aci->devnum > 0) {
		data->baseaddress_mask = 0xc000;
		data->baseaddress_value = (aci->devnum - 1) * 0x4000;
	}
	data->type = SNDDEV_PRELUDE1200;
	data->fifo_size = 1024;
	data->codec_reg1_mask = 0x00ff;
	data->codec_reg1_addr = 0x0001;
	data->codec_reg2_mask = 0x00ff;
	data->codec_reg2_addr = 0x0005;
	data->codec_fifo_mask = 0x00ff;
	data->codec_fifo_addr = 0x0011;
	data->streamid = 0;
	memset(data->acmemory, 0xff, sizeof data->acmemory);
	data->rc = aci->rc;
	data->enabled = true;
	data->bank = &prelude1200_bank;
	mapped_malloc(data->bank);
	map_banks(data->bank, data->baseaddress >> 16, 1, 1);
	ad1848_init(data);
	data->ad1848_regs[12] = 0x8a;
	data->ad1848_index_mask = 31;
	calculate_volume_toccata(data);
	return true;
}

bool prelude_init(struct autoconfig_info *aci)
{
	const struct expansionromtype *ert = get_device_expansion_rom(ROMTYPE_PRELUDE);
	if (!ert)
		return false;

	aci->addrbank = &prelude_bank;
	device_add_reset(sndboard_reset);

	if (!aci->doinit) {
		aci->autoconfigp = ert->autoconfig;
		return true;
	}

	snd_init();

	struct snddev_data *data = &snddev[1];

	data->configured = 0;
	data->type = SNDDEV_PRELUDE;
	data->fifo_size = 1024;
	data->codec_reg1_mask = 0x00ff;
	data->codec_reg1_addr = 0x0080;
	data->codec_reg2_mask = 0x00ff;
	data->codec_reg2_addr = 0x0082;
	data->codec_fifo_mask = 0x00ff;
	data->codec_fifo_addr = 0x0088;
	data->streamid = 0;
	memset(data->acmemory, 0xff, sizeof data->acmemory);
	data->rc = aci->rc;
	data->enabled = true;
	for (int i = 0; i < 16; i++) {
		uae_u8 b = ert->autoconfig[i];
		ew(data->acmemory, i * 4, b);
	}
	data->bank = &prelude_bank;
	mapped_malloc(data->bank);
	ad1848_init(data);
	data->ad1848_regs[12] = 0x8a;
	data->ad1848_index_mask = 31;
	calculate_volume_toccata(data);
	return true;
}

bool toccata_init(struct autoconfig_info *aci)
{
	const struct expansionromtype *ert = get_device_expansion_rom(ROMTYPE_TOCCATA);
	if (!ert)
		return false;

	aci->addrbank = &toccata_bank;
	device_add_reset(sndboard_reset);

	if (!aci->doinit) {
		aci->autoconfigp = ert->autoconfig;
		return true;
	}

	snd_init();

	struct snddev_data *data = &snddev[0];

	data->configured = 0;
	data->type = SNDDEV_TOCCATA;
	data->fifo_size = 1024;
	data->codec_reg1_mask = 0x6801;
	data->codec_reg1_addr = 0x6001;
	data->codec_reg2_mask = 0x6801;
	data->codec_reg2_addr = 0x6801;
	data->codec_fifo_mask = 0x6800;
	data->codec_fifo_addr = 0x2000;
	data->streamid = 0;
	memset(data->acmemory, 0xff, sizeof data->acmemory);
	data->rc = aci->rc;
	data->enabled = true;
	for (int i = 0; i < 16; i++) {
		uae_u8 b = ert->autoconfig[i];
		ew(data->acmemory, i * 4, b);
	}
	data->bank = &toccata_bank;
	mapped_malloc(data->bank);
	ad1848_init(data);
	calculate_volume_toccata(data);
	return true;
}

static void sndboard_reset(int hardreset)
{
	for (int i = 0; i < MAX_SNDDEVS; i++) {
		struct snddev_data *data = &snddev[i];
		data->ch_sample[0] = 0;
		data->ch_sample[1] = 0;
		codec_stop(data);
		data->snddev_irq = 0;
		if (data->streamid > 0)
			audio_enable_stream(false, data->streamid, 0, NULL, data);
		data->streamid = 0;
		if (data->bank)
			mapped_free(data->bank);
		data->bank = NULL;
	}
	sndboard_rethink();
}

static void sndboard_free(void)
{
	sndboard_reset(1);
	for (int i = 0; i < MAX_SNDDEVS; i++) {
		struct snddev_data *data = &snddev[i];
		data->rc = NULL;
	}
}

struct fm801_data
{
	struct pci_board_state *pcibs;
	uaecptr play_dma[2], play_dma2[2];
	uae_u16 play_len, play_len2;
	uae_u16 play_control;
	uae_u16 interrupt_control;
	uae_u16 interrupt_status;
	int dmach;
	int freq;
	int bits;
	int ch;
	int bytesperframe;
	bool play_on;
	int left_volume, right_volume;
	int ch_sample[2];
	int event_time;
	int streamid;
};
static struct fm801_data fm801;
static bool fm801_active;
static const int fm801_freq[16] = { 5500, 8000, 9600, 11025, 16000, 19200, 22050, 32000, 38400, 44100, 48000 };

static void calculate_volume_fm801(void)
{
	struct fm801_data *data = &fm801;
	data->left_volume = (100 - currprefs.sound_volume_board) * 32768 / 100;
	data->right_volume = (100 - currprefs.sound_volume_board) * 32768 / 100;
}

static void sndboard_vsync_fm801(void)
{
	audio_activate();
	calculate_volume_fm801();
}

static void fm801_stop(struct fm801_data *data)
{
	write_log(_T("FM801 STOP\n"));
	data->play_on = false;
	int streamid = data->streamid;
	data->streamid = 0;
	audio_enable_stream(false, streamid, 0, NULL, NULL);
}

static void fm801_swap_buffers(struct fm801_data *data)
{
	data->dmach = data->dmach ? 0 : 1;
	data->play_dma2[data->dmach] = data->play_dma[data->dmach];
	data->play_len2 = data->play_len;
	// stop at the end of buffer
	if (!(data->play_control & 0x20) && !(data->play_control & 0x80))
		fm801_stop(data);
}

static void fm801_interrupt(struct fm801_data *data)
{
	if ((data->interrupt_status & 0x100) && !(data->interrupt_control & 1)) {
		data->pcibs->irq_callback(data->pcibs, true);
	} else {
		data->pcibs->irq_callback(data->pcibs, false);
	}
}

static bool audio_state_sndboard_fm801(int streamid, void *params)
{
	struct fm801_data *data = &fm801;

	if (!fm801_active)
		return false;
	if (data->streamid != streamid)
		return false;
	if (data->play_on) {
		uae_u8 sample[2 * 6] = { 0 };
		pci_read_dma(data->pcibs, data->play_dma2[data->dmach], sample, data->bytesperframe);
		for (int i = 0; i < data->ch; i++) {
			uae_s16 smp;
			int vol;
			if (data->bits == 8) {
				smp = (sample[i] << 8) | (sample[i]);
			} else {
				smp = (sample[i * 2 + 1] << 8) | sample[i * 2 + 0];
			}
			if (i == 0 || i == 4)
				vol = data->left_volume;
			else if (i == 1 || i == 5)
				vol = data->right_volume;
			else
				vol = (data->left_volume + data->right_volume) / 2;
			data->ch_sample[i] = smp * vol / 32768;
		}
		data->play_len2 -= data->bytesperframe;
		data->play_dma2[data->dmach] += data->bytesperframe;
		if (data->play_len2 == 0xffff) {
			fm801_swap_buffers(data);
			data->interrupt_status |= 0x100;
			fm801_interrupt(data);
		}
	}
	audio_state_stream_state(streamid, data->ch_sample, data->ch, data->event_time);
	return true;
}

static void fm801_hsync_handler(struct pci_board_state *pcibs)
{
}

static void fm801_play(struct fm801_data *data)
{
	uae_u16 control = data->play_control;
	int f = (control >> 8) & 15;
	data->freq = fm801_freq[f];
	if (!data->freq)
		data->freq = 44100;
	data->event_time = (int)(base_event_clock * CYCLE_UNIT / data->freq);
	data->bits = (control & 0x4000) ? 16 : 8;
	f = (control >> 12) & 3;
	switch (f)
	{
		case 0:
		data->ch = (control & 0x8000) ? 2 : 1;
		break;
		case 1:
		data->ch = 4;
		break;
		case 2:
		data->ch = 6;
		break;
		case 3:
		data->ch = 6;
		break;
	}
	data->bytesperframe = data->bits * data->ch / 8;
	data->play_on = true;

	data->dmach = 1;
	fm801_swap_buffers(data);

	calculate_volume_fm801();

	write_log(_T("FM801 PLAY: freq=%d ch=%d bits=%d\n"), data->freq, data->ch, data->bits);

	data->streamid = audio_enable_stream(true, -1, data->ch, audio_state_sndboard_fm801, NULL);
}

static void fm801_pause(struct fm801_data *data, bool pause)
{
	write_log(_T("FM801 PAUSED %d\n"), pause);
}

static void fm801_control(struct fm801_data *data, uae_u16 control)
{
	uae_u16 old_control = data->play_control;
	data->play_control = control;
	data->play_control &= ~(8 | 16);
	if ((data->play_control & 0x20) && !(old_control & 0x20)) {
		fm801_play(data);
	} else if (!(data->play_control & 0x20) && (old_control & 0x20)) {
		if (data->play_control & 0x80)
			fm801_stop(data);
	} else if (data->play_control & 0x20) {
		if ((data->play_control & 0x40) && !(old_control & 0x40)) {
			fm801_pause(data, true);
		} else if (!(data->play_control & 0x40) && (old_control & 0x40)) {
			fm801_pause(data, true);
		}
	}

}

static void REGPARAM2 fm801_bput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	struct fm801_data *data = &fm801;
}
static void REGPARAM2 fm801_wput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	struct fm801_data *data = &fm801;
	switch (addr)
	{
		case 0x08:
		fm801_control(data, b);
		break;
		case 0x0a:
		data->play_len = b;
		break;
		case 0x56:
		data->interrupt_control = b;
		fm801_interrupt(data);
		break;
		case 0x5a:
		data->interrupt_status &= ~b;
		fm801_interrupt(data);
		break;
	}
}
static void REGPARAM2 fm801_lput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	struct fm801_data *data = &fm801;
	switch (addr)
	{
		case 0x0c:
		data->play_dma[0] = b;
		break;
		case 0x10:
		data->play_dma[1] = b;
		break;
	}
}
static uae_u32 REGPARAM2 fm801_bget(struct pci_board_state *pcibs, uaecptr addr)
{
	struct fm801_data *data = &fm801;
	uae_u32 v = 0;
	return v;
}
static uae_u32 REGPARAM2 fm801_wget(struct pci_board_state *pcibs, uaecptr addr)
{
	struct fm801_data *data = &fm801;
	uae_u32 v = 0;
	switch (addr) {
		case 0x08:
		v = data->play_control;
		break;
		case 0x0a:
		v = data->play_len2;
		break;
		case 0x56:
		v = data->interrupt_control;
		break;
		case 0x5a:
		v = data->interrupt_status;
		break;

	}
	return v;
}
static uae_u32 REGPARAM2 fm801_lget(struct pci_board_state *pcibs, uaecptr addr)
{
	struct fm801_data *data = &fm801;
	uae_u32 v = 0;
	switch(addr)
	{
		case 0x0c:
		v = data->play_dma2[data->dmach];
		break;
		break;
		case 0x10:
		v = data->play_dma2[data->dmach];
	}
	return v;
}

static void fm801_reset(struct pci_board_state *pcibs)
{
	struct fm801_data *data = &fm801;
	data->play_control = 0xca00;
	data->interrupt_control = 0x00df;
}

static void fm801_free(struct pci_board_state *pcibs)
{
	struct fm801_data *data = &fm801;
	fm801_active = false;
	fm801_stop(data);
}

static bool fm801_init(struct pci_board_state *pcibs, struct autoconfig_info *aci)
{
	struct fm801_data *data = &fm801;
	memset(data, 0, sizeof(struct fm801_data));
	data->pcibs = pcibs;
	fm801_active = true;
	return false;
}

static void fm801_config(struct pci_board_state *pcibs, uae_u8 *cfg)
{
	// Status register is read-only
	cfg[4] = 0x02;
	cfg[5] = 0x80;
	// Clear command register reserved
	cfg[6] &= 3;
}

static const struct pci_config fm801_pci_config =
{
	0x1319, 0x0801, 0, 0x280, 0xb2, 0x040100, 0x80, 0x1319, 0x1319, 1, 0x04, 0x28, { 128 | 1, 0, 0, 0, 0, 0, 0 }
};
static const struct pci_config fm801_pci_config_func1 =
{
	0x1319, 0x0802, 0, 0x280, 0xb2, 0x098000, 0x80, 0x1319, 0x1319, 0, 0x04, 0x28, { 16 | 1, 0, 0, 0, 0, 0, 0 }
};

const struct pci_board fm801_pci_board =
{
	_T("FM801"),
	&fm801_pci_config, fm801_init, fm801_free, fm801_reset, fm801_hsync_handler, fm801_config,
	{
		{ fm801_lget, fm801_wget, fm801_bget, fm801_lput, fm801_wput, fm801_bput },
		{ NULL },
		{ NULL },
		{ NULL },
		{ NULL },
		{ NULL },
		{ NULL },
		{ NULL }
	}
};

const struct pci_board fm801_pci_board_func1 =
{
	_T("FM801-2"),
	&fm801_pci_config_func1, NULL, NULL, NULL, NULL, fm801_config,
	{
		{ fm801_lget, fm801_wget, fm801_bget, fm801_lput, fm801_wput, fm801_bput },
		{ NULL },
		{ NULL },
		{ NULL },
		{ NULL },
		{ NULL },
		{ NULL },
		{ NULL }
	}
};

static void solo1_reset(struct pci_board_state *pcibs)
{
}

static void solo1_free(struct pci_board_state *pcibs)
{
}

static bool solo1_init(struct pci_board_state *pcibs, struct autoconfig_info *aci)
{
	return true;
}

static void solo1_sb_put(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
}
static uae_u32 solo1_sb_get(struct pci_board_state *pcibs, uaecptr addr)
{
	uae_u32 v = 0;
	return v;
}

static void solo1_put(struct pci_board_state *pcibs, int bar, uaecptr addr, uae_u32 b)
{
	if (bar == 1)
		solo1_sb_put(pcibs, addr, b);
}
static uae_u32 solo1_get(struct pci_board_state *pcibs, int bar, uaecptr addr)
{
	uae_u32 v = 0;
	if (bar == 1)
		v = solo1_sb_get(pcibs, addr);
	return v;
}

static void REGPARAM2 solo1_bput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	write_log(_T("SOLO1 BPUT %08x=%08x %d\n"), addr, b, pcibs->selected_bar);
	solo1_put(pcibs, pcibs->selected_bar, addr + 0, b >> 24);
	solo1_put(pcibs, pcibs->selected_bar, addr + 1, b >> 16);
	solo1_put(pcibs, pcibs->selected_bar, addr + 2, b >>  8);
	solo1_put(pcibs, pcibs->selected_bar, addr + 3, b >>  0);
}
static void REGPARAM2 solo1_wput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	write_log(_T("SOLO1 WPUT %08x=%08x %d\n"), addr, b, pcibs->selected_bar);
	solo1_put(pcibs, pcibs->selected_bar, addr + 0, b >> 8);
	solo1_put(pcibs, pcibs->selected_bar, addr + 1, b >> 0);
}
static void REGPARAM2 solo1_lput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	write_log(_T("SOLO1 LPUT %08x=%08x %d\n"), addr, b, pcibs->selected_bar);
	solo1_put(pcibs, pcibs->selected_bar, addr, b);
}
static uae_u32 REGPARAM2 solo1_bget(struct pci_board_state *pcibs, uaecptr addr)
{
	uae_u32 v = 0;
	v = solo1_get(pcibs, pcibs->selected_bar, addr);
	write_log(_T("SOLO1 BGET %08x %d\n"), addr, pcibs->selected_bar);
	return v;
}
static uae_u32 REGPARAM2 solo1_wget(struct pci_board_state *pcibs, uaecptr addr)
{
	uae_u32 v = 0;
	write_log(_T("SOLO1 WGET %08x %d\n"), addr, pcibs->selected_bar);
	return v;
}
static uae_u32 REGPARAM2 solo1_lget(struct pci_board_state *pcibs, uaecptr addr)
{
	uae_u32 v = 0;
	write_log(_T("SOLO1 LGET %08x %d\n"), addr, pcibs->selected_bar);
	return v;
}

static const struct pci_config solo1_pci_config =
{
	0x125d, 0x1969, 0, 0, 0, 0x040100, 0, 0x125d, 0x1818, 1, 2, 0x18, { 16 | 1, 16 | 1, 16 | 1, 4 | 1, 4 | 1, 0, 0 }
};
const struct pci_board solo1_pci_board =
{
	_T("SOLO1"),
	&solo1_pci_config, solo1_init, solo1_free, solo1_reset, NULL, NULL,
	{
		{ solo1_lget, solo1_wget, solo1_bget, solo1_lput, solo1_wput, solo1_bput },
		{ solo1_lget, solo1_wget, solo1_bget, solo1_lput, solo1_wput, solo1_bput },
		{ solo1_lget, solo1_wget, solo1_bget, solo1_lput, solo1_wput, solo1_bput },
		{ solo1_lget, solo1_wget, solo1_bget, solo1_lput, solo1_wput, solo1_bput },
		{ solo1_lget, solo1_wget, solo1_bget, solo1_lput, solo1_wput, solo1_bput },
		{ NULL },
		{ NULL },
		{ NULL }
	}
};

static SWVoiceOut *qemu_voice_out;

static bool audio_state_sndboard_qemu(int streamid, void *params)
{
	SWVoiceOut *out = qemu_voice_out;

	if (!out || !out->active)
		return false;
	if (streamid != out->streamid)
		return false;
	if (out->active) {
		uae_s16 l, r;
		if (out->samplebuf_index >= out->samplebuf_total) {
			int maxsize = sizeof(out->samplebuf);
			int size = 128 * out->bytesperframe;
			if (size > maxsize)
				size = maxsize;
			out->callback(out->opaque, size);
			out->samplebuf_index = 0;
		}
		uae_u8 *p = out->samplebuf + out->samplebuf_index;
		if (out->bits == 8) {
			if (out->ch == 1) {
				p[1] = p[0];
				p[2] = p[0];
				p[3] = p[0];
			} else {
				p[2] = p[1];
				p[3] = p[1];
				p[1] = p[0];
			}
		} else {
			if (out->ch == 1) {
				p[2] = p[0];
				p[3] = p[1];
			}
		}
		l = (p[1] << 8) | p[0];
		r = (p[3] << 8) | p[2];
		out->ch_sample[0] = l;
		out->ch_sample[1] = r;
		out->ch_sample[0] = out->ch_sample[0] * out->left_volume / 32768;
		out->ch_sample[1] = out->ch_sample[1] * out->right_volume / 32768;
		out->samplebuf_index += out->bytesperframe;
	}
	audio_state_stream_state(streamid, out->ch_sample, out->ch, out->event_time);
	return true;
}

static void calculate_volume_qemu(void)
{
	SWVoiceOut *out = qemu_voice_out;
	if (!out)
		return;
	out->left_volume = (100 - currprefs.sound_volume_board) * 32768 / 100;
	out->right_volume = (100 - currprefs.sound_volume_board) * 32768 / 100;
}

void AUD_close_in(QEMUSoundCard *card, SWVoiceIn *sw)
{
}
int AUD_read(SWVoiceIn *sw, void *pcm_buf, int size)
{
	return size;
}
int AUD_write(SWVoiceOut *sw, void *pcm_buf, int size)
{
	memcpy(sw->samplebuf, pcm_buf, size);
	sw->samplebuf_total = size;
	return sw->samplebuf_total;
}
void AUD_set_active_out(SWVoiceOut *sw, int on)
{
	sw->active = on != 0;
	sw->event_time = (int)(base_event_clock * CYCLE_UNIT / sw->freq);
	sw->samplebuf_index = 0;
	sw->samplebuf_total = 0;
	calculate_volume_qemu();
	audio_enable_stream(false, sw->streamid, 2, NULL, NULL);
	sw->streamid = 0;
	if (on) {
		sw->streamid = audio_enable_stream(true, -1, 2, audio_state_sndboard_qemu, NULL);
	}
}
void AUD_set_active_in(SWVoiceIn *sw, int on)
{
}
int  AUD_is_active_in(SWVoiceIn *sw)
{
	return 0;
}
void AUD_close_out(QEMUSoundCard *card, SWVoiceOut *sw)
{
	qemu_voice_out = NULL;
	if (sw) {
		audio_enable_stream(false, sw->streamid, 0, NULL, NULL);
		sw->streamid = 0;
		xfree(sw);
	}
}
SWVoiceIn *AUD_open_in(
	QEMUSoundCard *card,
	SWVoiceIn *sw,
	const char *name,
	void *callback_opaque,
	audio_callback_fn callback_fn,
struct audsettings *settings)
{
	return NULL;
}
SWVoiceOut *AUD_open_out(
	QEMUSoundCard *card,
	SWVoiceOut *sw,
	const char *name,
	void *callback_opaque,
	audio_callback_fn callback_fn,
	struct audsettings *settings)
{
	SWVoiceOut *out = sw;
	if (!sw)
		out = xcalloc(SWVoiceOut, 1);
	int bits = 8;

	if (settings->fmt >= AUD_FMT_U16)
		bits = 16;
	if (settings->fmt >= AUD_FMT_U32)
		bits = 32;

	out->callback = callback_fn;
	out->opaque = callback_opaque;
	out->bits = bits;
	out->freq = settings->freq;
	out->ch = settings->nchannels;
	out->fmt = settings->fmt;
	out->bytesperframe = out->ch * bits / 8;

	TCHAR *name2 = au(name);
	write_log(_T("QEMU AUDIO: freq=%d ch=%d bits=%d (fmt=%d) '%s'\n"), out->freq, out->ch, bits, settings->fmt, name2);
	xfree(name2);

	qemu_voice_out = out;

	return out;
}

static void sndboard_vsync_qemu(void)
{
	audio_activate();
}

static void sndboard_vsync(void)
{
	if (snddev[0].snddev_active)
		sndboard_vsync_toccata(&snddev[0]);
	if (fm801_active)
		sndboard_vsync_fm801();
	if (qemu_voice_out && qemu_voice_out->active)
		sndboard_vsync_qemu();
}

void sndboard_ext_volume(void)
{
	if (snddev[0].snddev_active)
		calculate_volume_toccata(&snddev[0]);
	if (fm801_active)
		calculate_volume_fm801();
	if (qemu_voice_out && qemu_voice_out->active)
		calculate_volume_qemu();
}

static void snd_init(void)
{
	device_add_hsync(sndboard_hsync);
	device_add_vsync_post(sndboard_vsync);
	device_add_rethink(sndboard_rethink);
	device_add_exit(sndboard_free, NULL);
	device_add_exit(uaesndboard_free, NULL);
}


#ifdef _WIN32

#include <mmdeviceapi.h>
#include <Audioclient.h>

#define REFTIMES_PER_SEC  10000000

static const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
static const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
static const IID IID_IAudioClient = __uuidof(IAudioClient);
static const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

#define EXIT_ON_ERROR(hres) if (FAILED(hres)) { goto Exit; }
#define SAFE_RELEASE(punk)  if ((punk) != NULL) { (punk)->Release(); (punk) = NULL; }

static IMMDeviceEnumerator *pEnumerator = NULL;
static IMMDevice *pDevice = NULL;
static IAudioClient *pAudioClient = NULL;
static IAudioCaptureClient *pCaptureClient = NULL;
static bool capture_started;

static uae_u8 *sndboard_get_buffer(int *frames)
{	
	HRESULT hr;
	UINT32 numFramesAvailable;
	BYTE *pData;
	DWORD flags = 0;

	*frames = -1;
	if (!capture_started)
		return NULL;
	hr = pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, NULL, NULL);
	if (FAILED(hr)) {
		write_log(_T("GetBuffer failed %08x\n"), hr);
		return NULL;
	}
	*frames = numFramesAvailable;
	return pData;
}

static void sndboard_release_buffer(uae_u8 *buffer, int frames)
{
	HRESULT hr;
	if (!capture_started || frames < 0)
		return;
	hr = pCaptureClient->ReleaseBuffer(frames);
	if (FAILED(hr)) {
		write_log(_T("ReleaseBuffer failed %08x\n"), hr);
	}
}

static void sndboard_free_capture(void)
{
	if (capture_started)
		pAudioClient->Stop();
	capture_started = false;
	SAFE_RELEASE(pEnumerator)
	SAFE_RELEASE(pDevice)
	SAFE_RELEASE(pAudioClient)
	SAFE_RELEASE(pCaptureClient)
}

static bool sndboard_init_capture(int freq)
{
	HRESULT hr;
	WAVEFORMATEX wavfmtsrc;
	WAVEFORMATEX *wavfmt2;
	WAVEFORMATEX *wavfmt;
	bool init = false;

	wavfmt2 = NULL;

	hr = CoCreateInstance(
		CLSID_MMDeviceEnumerator, NULL,
		CLSCTX_ALL, IID_IMMDeviceEnumerator,
		(void**)&pEnumerator);
	EXIT_ON_ERROR(hr)

	hr = pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &pDevice);
	EXIT_ON_ERROR(hr)

	hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
	EXIT_ON_ERROR(hr)

	memset (&wavfmtsrc, 0, sizeof wavfmtsrc);
	wavfmtsrc.nChannels = 2;
	wavfmtsrc.nSamplesPerSec = freq;
	wavfmtsrc.wBitsPerSample = 16;
	wavfmtsrc.wFormatTag = WAVE_FORMAT_PCM;
	wavfmtsrc.cbSize = 0;
	wavfmtsrc.nBlockAlign = wavfmtsrc.wBitsPerSample / 8 * wavfmtsrc.nChannels;
	wavfmtsrc.nAvgBytesPerSec = wavfmtsrc.nBlockAlign * wavfmtsrc.nSamplesPerSec;

	AUDCLNT_SHAREMODE exc;
	for (int mode = 0; mode < 2; mode++) {
		exc = mode == 0 ? AUDCLNT_SHAREMODE_EXCLUSIVE : AUDCLNT_SHAREMODE_SHARED;
		int time = mode == 0 ? 0 : REFTIMES_PER_SEC / 50;

		wavfmt = &wavfmtsrc;
		hr = pAudioClient->IsFormatSupported(exc, &wavfmtsrc, &wavfmt2);
		if (SUCCEEDED(hr)) {
			hr = pAudioClient->Initialize(exc, 0, time, 0, wavfmt, NULL);
			if (SUCCEEDED(hr)) {
				init = true;
				break;
			}
		}
	
		if (hr == S_FALSE && wavfmt2) {
			wavfmt = wavfmt2;
			hr = pAudioClient->Initialize(exc, 0, time, 0, wavfmt, NULL);
			if (SUCCEEDED(hr)) {
				init = true;
				break;
			}
		}
	}

	if (!init) {
		write_log(_T("sndboard capture init, freq=%d, failed\n"), freq);
		goto Exit;
	}


	hr = pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pCaptureClient);
	EXIT_ON_ERROR(hr)
		
	hr = pAudioClient->Start();
	EXIT_ON_ERROR(hr)
	capture_started = true;

	CoTaskMemFree(wavfmt2);

	write_log(_T("sndboard capture started: freq=%d mode=%s\n"), freq, exc == AUDCLNT_SHAREMODE_EXCLUSIVE ? _T("exclusive") : _T("shared"));

	return true;
Exit:;
	CoTaskMemFree(wavfmt2);
	write_log(_T("sndboard capture init failed %08x\n"), hr);
	sndboard_free_capture();
	return false;
}

#endif
