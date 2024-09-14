/*
 * QEMU ES1370 emulation
 *
 * Copyright (c) 2005 Vassili Karpov (malc)
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

#include "sysconfig.h"
#include "sysdeps.h"

#include "qemuuaeglue.h"
#include "queue.h"
#include "qemuaudio.h"

#include "options.h"
#include "memory.h"
#include "pci_hw.h"

#ifdef __GNUC__
#define audio_MIN(a, b) ( __extension__ ({      \
    __typeof (a) ta = a;                        \
    __typeof (b) tb = b;                        \
    ((ta)>(tb)?(tb):(ta));                      \
}))

#define audio_MAX(a, b) ( __extension__ ({      \
    __typeof (a) ta = a;                        \
    __typeof (b) tb = b;                        \
    ((ta)<(tb)?(tb):(ta));                      \
}))
#else
#define audio_MIN(a, b) ((a)>(b)?(b):(a))
#define audio_MAX(a, b) ((a)<(b)?(b):(a))
#endif

#ifdef HOST_WORDS_BIGENDIAN
#define AUDIO_HOST_ENDIANNESS 1
#else
#define AUDIO_HOST_ENDIANNESS 0
#endif

//#define DEBUG_ES1370
//#define VERBOSE_ES1370
#define SILENT_ES1370

//#include "hw/hw.h"
//#include "hw/audio/audio.h"
//#include "audio/audio.h"
//#include "hw/pci/pci.h"
//#include "sysemu/dma.h"

/* Missing stuff:
   SCTRL_P[12](END|ST)INC
   SCTRL_P1SCTRLD
   SCTRL_P2DACSEN
   CTRL_DAC_SYNC
   MIDI
   non looped mode
   surely more
*/

/*
  Following macros and samplerate array were copied verbatim from
  Linux kernel 2.4.30: drivers/sound/es1370.c

  Copyright (C) 1998-2001, 2003 Thomas Sailer (t.sailer@alumni.ethz.ch)
*/

/* Start blatant GPL violation */

#define ES1370_REG_CONTROL        0x00
#define ES1370_REG_STATUS         0x04
#define ES1370_REG_UART_DATA      0x08
#define ES1370_REG_UART_STATUS    0x09
#define ES1370_REG_UART_CONTROL   0x09
#define ES1370_REG_UART_TEST      0x0a
#define ES1370_REG_MEMPAGE        0x0c
#define ES1370_REG_CODEC          0x10
#define ES1370_REG_SERIAL_CONTROL 0x20
#define ES1370_REG_DAC1_SCOUNT    0x24
#define ES1370_REG_DAC2_SCOUNT    0x28
#define ES1370_REG_ADC_SCOUNT     0x2c

#define ES1370_REG_DAC1_FRAMEADR    0xc30
#define ES1370_REG_DAC1_FRAMECNT    0xc34
#define ES1370_REG_DAC2_FRAMEADR    0xc38
#define ES1370_REG_DAC2_FRAMECNT    0xc3c
#define ES1370_REG_ADC_FRAMEADR     0xd30
#define ES1370_REG_ADC_FRAMECNT     0xd34
#define ES1370_REG_PHANTOM_FRAMEADR 0xd38
#define ES1370_REG_PHANTOM_FRAMECNT 0xd3c

static const unsigned dac1_samplerate[] = { 5512, 11025, 22050, 44100 };

#define DAC2_SRTODIV(x) (((1411200+(x)/2)/(x))-2)
#define DAC2_DIVTOSR(x) (1411200/((x)+2))

#define CTRL_ADC_STOP   0x80000000  /* 1 = ADC stopped */
#define CTRL_XCTL1      0x40000000  /* electret mic bias */
#define CTRL_OPEN       0x20000000  /* no function, can be read and written */
#define CTRL_PCLKDIV    0x1fff0000  /* ADC/DAC2 clock divider */
#define CTRL_SH_PCLKDIV 16
#define CTRL_MSFMTSEL   0x00008000  /* MPEG serial data fmt: 0 = Sony, 1 = I2S */
#define CTRL_M_SBB      0x00004000  /* DAC2 clock: 0 = PCLKDIV, 1 = MPEG */
#define CTRL_WTSRSEL    0x00003000  /* DAC1 clock freq: 0=5512, 1=11025, 2=22050, 3=44100 */
#define CTRL_SH_WTSRSEL 12
#define CTRL_DAC_SYNC   0x00000800  /* 1 = DAC2 runs off DAC1 clock */
#define CTRL_CCB_INTRM  0x00000400  /* 1 = CCB "voice" ints enabled */
#define CTRL_M_CB       0x00000200  /* recording source: 0 = ADC, 1 = MPEG */
#define CTRL_XCTL0      0x00000100  /* 0 = Line in, 1 = Line out */
#define CTRL_BREQ       0x00000080  /* 1 = test mode (internal mem test) */
#define CTRL_DAC1_EN    0x00000040  /* enable DAC1 */
#define CTRL_DAC2_EN    0x00000020  /* enable DAC2 */
#define CTRL_ADC_EN     0x00000010  /* enable ADC */
#define CTRL_UART_EN    0x00000008  /* enable MIDI uart */
#define CTRL_JYSTK_EN   0x00000004  /* enable Joystick port (presumably at address 0x200) */
#define CTRL_CDC_EN     0x00000002  /* enable serial (CODEC) interface */
#define CTRL_SERR_DIS   0x00000001  /* 1 = disable PCI SERR signal */

#define STAT_INTR       0x80000000  /* wired or of all interrupt bits */
#define STAT_CSTAT      0x00000400  /* 1 = codec busy or codec write in progress */
#define STAT_CBUSY      0x00000200  /* 1 = codec busy */
#define STAT_CWRIP      0x00000100  /* 1 = codec write in progress */
#define STAT_VC         0x00000060  /* CCB int source, 0=DAC1, 1=DAC2, 2=ADC, 3=undef */
#define STAT_SH_VC      5
#define STAT_MCCB       0x00000010  /* CCB int pending */
#define STAT_UART       0x00000008  /* UART int pending */
#define STAT_DAC1       0x00000004  /* DAC1 int pending */
#define STAT_DAC2       0x00000002  /* DAC2 int pending */
#define STAT_ADC        0x00000001  /* ADC int pending */

#define USTAT_RXINT     0x80        /* UART rx int pending */
#define USTAT_TXINT     0x04        /* UART tx int pending */
#define USTAT_TXRDY     0x02        /* UART tx ready */
#define USTAT_RXRDY     0x01        /* UART rx ready */

#define UCTRL_RXINTEN   0x80        /* 1 = enable RX ints */
#define UCTRL_TXINTEN   0x60        /* TX int enable field mask */
#define UCTRL_ENA_TXINT 0x20        /* enable TX int */
#define UCTRL_CNTRL     0x03        /* control field */
#define UCTRL_CNTRL_SWR 0x03        /* software reset command */

#define SCTRL_P2ENDINC    0x00380000  /*  */
#define SCTRL_SH_P2ENDINC 19
#define SCTRL_P2STINC     0x00070000  /*  */
#define SCTRL_SH_P2STINC  16
#define SCTRL_R1LOOPSEL   0x00008000  /* 0 = loop mode */
#define SCTRL_P2LOOPSEL   0x00004000  /* 0 = loop mode */
#define SCTRL_P1LOOPSEL   0x00002000  /* 0 = loop mode */
#define SCTRL_P2PAUSE     0x00001000  /* 1 = pause mode */
#define SCTRL_P1PAUSE     0x00000800  /* 1 = pause mode */
#define SCTRL_R1INTEN     0x00000400  /* enable interrupt */
#define SCTRL_P2INTEN     0x00000200  /* enable interrupt */
#define SCTRL_P1INTEN     0x00000100  /* enable interrupt */
#define SCTRL_P1SCTRLD    0x00000080  /* reload sample count register for DAC1 */
#define SCTRL_P2DACSEN    0x00000040  /* 1 = DAC2 play back last sample when disabled */
#define SCTRL_R1SEB       0x00000020  /* 1 = 16bit */
#define SCTRL_R1SMB       0x00000010  /* 1 = stereo */
#define SCTRL_R1FMT       0x00000030  /* format mask */
#define SCTRL_SH_R1FMT    4
#define SCTRL_P2SEB       0x00000008  /* 1 = 16bit */
#define SCTRL_P2SMB       0x00000004  /* 1 = stereo */
#define SCTRL_P2FMT       0x0000000c  /* format mask */
#define SCTRL_SH_P2FMT    2
#define SCTRL_P1SEB       0x00000002  /* 1 = 16bit */
#define SCTRL_P1SMB       0x00000001  /* 1 = stereo */
#define SCTRL_P1FMT       0x00000003  /* format mask */
#define SCTRL_SH_P1FMT    0

/* End blatant GPL violation */

#define NB_CHANNELS 3
#define DAC1_CHANNEL 0
#define DAC2_CHANNEL 1
#define ADC_CHANNEL 2

#define IO_READ_PROTO(n) \
static uint32_t n (void *opaque, uint32_t addr)
#define IO_WRITE_PROTO(n) \
static void n (void *opaque, uint32_t addr, uint32_t val)

static void es1370_dac1_callback (void *opaque, int free);
static void es1370_dac2_callback (void *opaque, int free);
static void es1370_adc_callback (void *opaque, int avail);

#ifdef DEBUG_ES1370

#define ldebug(...) AUD_log ("es1370", __VA_ARGS__)

static void print_ctl (uint32_t val)
{
    char buf[1024];

    buf[0] = '\0';
#define a(n) if (val & CTRL_##n) strcat (buf, " "#n)
    a (ADC_STOP);
    a (XCTL1);
    a (OPEN);
    a (MSFMTSEL);
    a (M_SBB);
    a (DAC_SYNC);
    a (CCB_INTRM);
    a (M_CB);
    a (XCTL0);
    a (BREQ);
    a (DAC1_EN);
    a (DAC2_EN);
    a (ADC_EN);
    a (UART_EN);
    a (JYSTK_EN);
    a (CDC_EN);
    a (SERR_DIS);
#undef a
    AUD_log ("es1370", "ctl - PCLKDIV %d(DAC2 freq %d), freq %d,%s\n",
             (val & CTRL_PCLKDIV) >> CTRL_SH_PCLKDIV,
             DAC2_DIVTOSR ((val & CTRL_PCLKDIV) >> CTRL_SH_PCLKDIV),
             dac1_samplerate[(val & CTRL_WTSRSEL) >> CTRL_SH_WTSRSEL],
             buf);
}

static void print_sctl (uint32_t val)
{
    static const char *fmt_names[] = {"8M", "8S", "16M", "16S"};
    char buf[1024];

    buf[0] = '\0';

#define a(n) if (val & SCTRL_##n) strcat (buf, " "#n)
#define b(n) if (!(val & SCTRL_##n)) strcat (buf, " "#n)
    b (R1LOOPSEL);
    b (P2LOOPSEL);
    b (P1LOOPSEL);
    a (P2PAUSE);
    a (P1PAUSE);
    a (R1INTEN);
    a (P2INTEN);
    a (P1INTEN);
    a (P1SCTRLD);
    a (P2DACSEN);
    if (buf[0]) {
        strcat (buf, "\n        ");
    }
    else {
        buf[0] = ' ';
        buf[1] = '\0';
    }
#undef b
#undef a
    AUD_log ("es1370",
             "%s"
             "p2_end_inc %d, p2_st_inc %d, r1_fmt %s, p2_fmt %s, p1_fmt %s\n",
             buf,
             (val & SCTRL_P2ENDINC) >> SCTRL_SH_P2ENDINC,
             (val & SCTRL_P2STINC) >> SCTRL_SH_P2STINC,
             fmt_names [(val >> SCTRL_SH_R1FMT) & 3],
             fmt_names [(val >> SCTRL_SH_P2FMT) & 3],
             fmt_names [(val >> SCTRL_SH_P1FMT) & 3]
        );
}
#else
#define ldebug(...)
#define print_ctl(...)
#define print_sctl(...)
#endif

#ifdef VERBOSE_ES1370
#define dolog(...) write_log(__VA_ARGS__)
#else
#define dolog(...)
#endif

#ifndef SILENT_ES1370
#define lwarn(...) write_log(__VA_ARGS__)
#else
#define lwarn(...)
#endif

struct chan {
    uint32_t shift;
    uint32_t leftover;
    uint32_t scount;
    uint32_t frame_addr;
    uint32_t frame_cnt;
};

typedef struct ES1370State {

	QEMUSoundCard card;
	struct pci_board_state *dev;
	const struct pci_board *device;

//	PCIDevice dev;
//   QEMUSoundCard card;
//    MemoryRegion io;
    struct chan chan[NB_CHANNELS];
	SWVoiceOut *dac_voice[2];
    SWVoiceIn *adc_voice;

    uint32_t ctl;
    uint32_t status;
    uint32_t mempage;
    uint32_t codec;
    uint32_t sctl;
	pci_dev_irq irq_callback;
} ES1370State;

struct chan_bits {
    uint32_t ctl_en;
    uint32_t stat_int;
    uint32_t sctl_pause;
    uint32_t sctl_inten;
    uint32_t sctl_fmt;
    uint32_t sctl_sh_fmt;
    uint32_t sctl_loopsel;
    void (*calc_freq) (ES1370State *s, uint32_t ctl,
                       uint32_t *old_freq, uint32_t *new_freq);
};

static ES1370State es1370state;

static void es1370_dac1_calc_freq (ES1370State *s, uint32_t ctl,
                                   uint32_t *old_freq, uint32_t *new_freq);
static void es1370_dac2_and_adc_calc_freq (ES1370State *s, uint32_t ctl,
                                           uint32_t *old_freq,
                                           uint32_t *new_freq);

static const struct chan_bits es1370_chan_bits[] = {
    {CTRL_DAC1_EN, STAT_DAC1, SCTRL_P1PAUSE, SCTRL_P1INTEN,
     SCTRL_P1FMT, SCTRL_SH_P1FMT, SCTRL_P1LOOPSEL,
     es1370_dac1_calc_freq},

    {CTRL_DAC2_EN, STAT_DAC2, SCTRL_P2PAUSE, SCTRL_P2INTEN,
     SCTRL_P2FMT, SCTRL_SH_P2FMT, SCTRL_P2LOOPSEL,
     es1370_dac2_and_adc_calc_freq},

    {CTRL_ADC_EN, STAT_ADC, 0, SCTRL_R1INTEN,
     SCTRL_R1FMT, SCTRL_SH_R1FMT, SCTRL_R1LOOPSEL,
     es1370_dac2_and_adc_calc_freq}
};

static void es1370_update_status (ES1370State *s, uint32_t new_status)
{
    uint32_t level = new_status & (STAT_DAC1 | STAT_DAC2 | STAT_ADC);

    if (level) {
        s->status = new_status | STAT_INTR;
    } else {
        s->status = new_status & ~STAT_INTR;
    }
	es1370state.irq_callback(es1370state.dev, !!level);
//    pci_set_irq(&s->dev, !!level);
}

static void es1370_reset (ES1370State *s)
{
    size_t i;

    s->ctl = 1;
    s->status = 0x60;
    s->mempage = 0;
    s->codec = 0;
    s->sctl = 0;

    for (i = 0; i < NB_CHANNELS; ++i) {
        struct chan *d = &s->chan[i];
        d->scount = 0;
        d->leftover = 0;
		if (i == ADC_CHANNEL) {
            AUD_close_in (&s->card, s->adc_voice);
            s->adc_voice = NULL;
        } else {
            AUD_close_out (&s->card, s->dac_voice[i]);
            s->dac_voice[i] = NULL;
        }
	}
	es1370state.irq_callback(es1370state.dev, false);
//    pci_irq_deassert(&s->dev);
}

static void es1370_maybe_lower_irq (ES1370State *s, uint32_t sctl)
{
    uint32_t new_status = s->status;

    if (!(sctl & SCTRL_P1INTEN) && (s->sctl & SCTRL_P1INTEN)) {
        new_status &= ~STAT_DAC1;
    }

    if (!(sctl & SCTRL_P2INTEN) && (s->sctl & SCTRL_P2INTEN)) {
        new_status &= ~STAT_DAC2;
    }

    if (!(sctl & SCTRL_R1INTEN) && (s->sctl & SCTRL_R1INTEN)) {
        new_status &= ~STAT_ADC;
    }

    if (new_status != s->status) {
        es1370_update_status (s, new_status);
    }
}

static void es1370_dac1_calc_freq (ES1370State *s, uint32_t ctl,
                                   uint32_t *old_freq, uint32_t *new_freq)

{
    *old_freq = dac1_samplerate[(s->ctl & CTRL_WTSRSEL) >> CTRL_SH_WTSRSEL];
    *new_freq = dac1_samplerate[(ctl & CTRL_WTSRSEL) >> CTRL_SH_WTSRSEL];
}

static void es1370_dac2_and_adc_calc_freq (ES1370State *s, uint32_t ctl,
                                           uint32_t *old_freq,
                                           uint32_t *new_freq)

{
    uint32_t old_pclkdiv, new_pclkdiv;

    new_pclkdiv = (ctl & CTRL_PCLKDIV) >> CTRL_SH_PCLKDIV;
    old_pclkdiv = (s->ctl & CTRL_PCLKDIV) >> CTRL_SH_PCLKDIV;
    *new_freq = DAC2_DIVTOSR (new_pclkdiv);
    *old_freq = DAC2_DIVTOSR (old_pclkdiv);
}

static void es1370_update_voices (ES1370State *s, uint32_t ctl, uint32_t sctl)
{
    size_t i;
    uint32_t old_freq, new_freq, old_fmt, new_fmt;

    for (i = 0; i < NB_CHANNELS; ++i) {
        struct chan *d = &s->chan[i];
        const struct chan_bits *b = &es1370_chan_bits[i];

        new_fmt = (sctl & b->sctl_fmt) >> b->sctl_sh_fmt;
        old_fmt = (s->sctl & b->sctl_fmt) >> b->sctl_sh_fmt;

        b->calc_freq (s, ctl, &old_freq, &new_freq);

        if ((old_fmt != new_fmt) || (old_freq != new_freq)) {
            d->shift = (new_fmt & 1) + (new_fmt >> 1);
            ldebug ("channel %zu, freq = %d, nchannels %d, fmt %d, shift %d\n",
                    i,
                    new_freq,
                    1 << (new_fmt & 1),
                    (new_fmt & 2) ? AUD_FMT_S16 : AUD_FMT_U8,
                    d->shift);
            if (new_freq) {
                struct audsettings as;

                as.freq = new_freq;
                as.nchannels = 1 << (new_fmt & 1);
                as.fmt = (new_fmt & 2) ? AUD_FMT_S16 : AUD_FMT_U8;
                as.endianness = 0;

                if (i == ADC_CHANNEL) {
                    s->adc_voice =
                        AUD_open_in (
                            &s->card,
                            s->adc_voice,
                            "es1370.adc",
                            s,
                            es1370_adc_callback,
                            &as
                            );
                } else {
                    s->dac_voice[i] =
                        AUD_open_out (
                            &s->card,
                            s->dac_voice[i],
                            i ? "es1370.dac2" : "es1370.dac1",
                            s,
                            i ? es1370_dac2_callback : es1370_dac1_callback,
                            &as
                            );
                }
            }
        }

        if (((ctl ^ s->ctl) & b->ctl_en)
            || ((sctl ^ s->sctl) & b->sctl_pause)) {
            int on = (ctl & b->ctl_en) && !(sctl & b->sctl_pause);

			// TW: this was missing from original code.
			// Without this fix upper half of scount contained value from
			// last playback, breaking buffer switch sync in next playback.
			d->scount = (d->scount & 0xffff) | ((d->scount & 0xffff) << 16);
			
			if (i == ADC_CHANNEL) {
                AUD_set_active_in (s->adc_voice, on);
            } else {
				AUD_set_active_out (s->dac_voice[i], on);
            }
        }
    }

    s->ctl = ctl;
    s->sctl = sctl;
}

static inline uint32_t es1370_fixup (ES1370State *s, uint32_t addr)
{
    addr &= 0xff;
    if (addr >= 0x30 && addr <= 0x3f)
        addr |= s->mempage << 8;
    return addr;
}

IO_WRITE_PROTO (es1370_writeb)
{
    ES1370State *s = (ES1370State*)opaque;
    uint32_t shift, mask;

    addr = es1370_fixup (s, addr);

    switch (addr) {
    case ES1370_REG_CONTROL:
    case ES1370_REG_CONTROL + 1:
    case ES1370_REG_CONTROL + 2:
    case ES1370_REG_CONTROL + 3:
        shift = (addr - ES1370_REG_CONTROL) << 3;
        mask = 0xff << shift;
        val = (s->ctl & ~mask) | ((val & 0xff) << shift);
        es1370_update_voices (s, val, s->sctl);
        print_ctl (val);
        break;
    case ES1370_REG_MEMPAGE:
        s->mempage = val;
        break;
    case ES1370_REG_SERIAL_CONTROL:
    case ES1370_REG_SERIAL_CONTROL + 1:
    case ES1370_REG_SERIAL_CONTROL + 2:
    case ES1370_REG_SERIAL_CONTROL + 3:
        shift = (addr - ES1370_REG_SERIAL_CONTROL) << 3;
        mask = 0xff << shift;
        val = (s->sctl & ~mask) | ((val & 0xff) << shift);
        es1370_maybe_lower_irq (s, val);
        es1370_update_voices (s, s->ctl, val);
        print_sctl (val);
        break;
    default:
        lwarn ("writeb %#x <- %#x\n", addr, val);
        break;
    }
}

IO_WRITE_PROTO (es1370_writew)
{
    ES1370State *s = (ES1370State*)opaque;
    addr = es1370_fixup (s, addr);
    uint32_t shift, mask;
    struct chan *d = &s->chan[0];

    switch (addr) {
    case ES1370_REG_CODEC:
        dolog ("ignored codec write address %#x, data %#x\n",
               (val >> 8) & 0xff, val & 0xff);
        s->codec = val;
        break;

    case ES1370_REG_CONTROL:
    case ES1370_REG_CONTROL + 2:
        shift = (addr != ES1370_REG_CONTROL) << 4;
        mask = 0xffff << shift;
        val = (s->ctl & ~mask) | ((val & 0xffff) << shift);
        es1370_update_voices (s, val, s->sctl);
        print_ctl (val);
        break;

    case ES1370_REG_ADC_SCOUNT:
        d++;
    case ES1370_REG_DAC2_SCOUNT:
        d++;
    case ES1370_REG_DAC1_SCOUNT:
        d->scount = (d->scount & ~0xffff) | (val & 0xffff);
        break;

    default:
        lwarn ("writew %#x <- %#x\n", addr, val);
        break;
    }
}

IO_WRITE_PROTO (es1370_writel)
{
    ES1370State *s = (ES1370State*)opaque;
    struct chan *d = &s->chan[0];

    addr = es1370_fixup (s, addr);

    switch (addr) {
    case ES1370_REG_CONTROL:
        es1370_update_voices (s, val, s->sctl);
        print_ctl (val);
        break;

    case ES1370_REG_MEMPAGE:
        s->mempage = val & 0xf;
        break;

    case ES1370_REG_SERIAL_CONTROL:
        es1370_maybe_lower_irq (s, val);
        es1370_update_voices (s, s->ctl, val);
        print_sctl (val);
        break;

    case ES1370_REG_ADC_SCOUNT:
        d++;
    case ES1370_REG_DAC2_SCOUNT:
        d++;
    case ES1370_REG_DAC1_SCOUNT:
        d->scount = (val & 0xffff) | (d->scount & ~0xffff);
        ldebug ("chan %td CURR_SAMP_CT %d, SAMP_CT %d\n",
                d - &s->chan[0], val >> 16, (val & 0xffff));
        break;

    case ES1370_REG_ADC_FRAMEADR:
        d++;
    case ES1370_REG_DAC2_FRAMEADR:
        d++;
    case ES1370_REG_DAC1_FRAMEADR:
        d->frame_addr = val;
        ldebug ("chan %td frame address %#x\n", d - &s->chan[0], val);
        break;

    case ES1370_REG_PHANTOM_FRAMECNT:
        lwarn ("writing to phantom frame count %#x\n", val);
        break;
    case ES1370_REG_PHANTOM_FRAMEADR:
        lwarn ("writing to phantom frame address %#x\n", val);
        break;

    case ES1370_REG_ADC_FRAMECNT:
        d++;
    case ES1370_REG_DAC2_FRAMECNT:
        d++;
    case ES1370_REG_DAC1_FRAMECNT:
        d->frame_cnt = val;
        d->leftover = 0;
        ldebug ("chan %td frame count %d, buffer size %d\n",
                d - &s->chan[0], val >> 16, val & 0xffff);
        break;

    default:
        lwarn ("writel %#x <- %#x\n", addr, val);
        break;
    }
}

IO_READ_PROTO (es1370_readb)
{
    ES1370State *s = (ES1370State*)opaque;
    uint32_t val;

    addr = es1370_fixup (s, addr);

    switch (addr) {
    case 0x1b:                  /* Legacy */
        lwarn ("Attempt to read from legacy register\n");
        val = 5;
        break;
    case ES1370_REG_MEMPAGE:
        val = s->mempage;
        break;
    case ES1370_REG_CONTROL + 0:
    case ES1370_REG_CONTROL + 1:
    case ES1370_REG_CONTROL + 2:
    case ES1370_REG_CONTROL + 3:
        val = s->ctl >> ((addr - ES1370_REG_CONTROL) << 3);
        break;
    case ES1370_REG_STATUS + 0:
    case ES1370_REG_STATUS + 1:
    case ES1370_REG_STATUS + 2:
    case ES1370_REG_STATUS + 3:
        val = s->status >> ((addr - ES1370_REG_STATUS) << 3);
        break;
    default:
        val = ~0;
        lwarn ("readb %#x -> %#x\n", addr, val);
        break;
    }
    return val;
}

IO_READ_PROTO (es1370_readw)
{
    ES1370State *s = (ES1370State*)opaque;
    struct chan *d = &s->chan[0];
    uint32_t val;

    addr = es1370_fixup (s, addr);

    switch (addr) {
    case ES1370_REG_ADC_SCOUNT + 2:
        d++;
    case ES1370_REG_DAC2_SCOUNT + 2:
        d++;
    case ES1370_REG_DAC1_SCOUNT + 2:
        val = d->scount >> 16;
        break;

    case ES1370_REG_ADC_FRAMECNT:
        d++;
    case ES1370_REG_DAC2_FRAMECNT:
        d++;
    case ES1370_REG_DAC1_FRAMECNT:
        val = d->frame_cnt & 0xffff;
        break;

    case ES1370_REG_ADC_FRAMECNT + 2:
        d++;
    case ES1370_REG_DAC2_FRAMECNT + 2:
        d++;
    case ES1370_REG_DAC1_FRAMECNT + 2:
        val = d->frame_cnt >> 16;
        break;

    default:
        val = ~0;
        lwarn ("readw %#x -> %#x\n", addr, val);
        break;
    }

    return val;
}

IO_READ_PROTO (es1370_readl)
{
    ES1370State *s = (ES1370State*)opaque;
    uint32_t val;
    struct chan *d = &s->chan[0];

    addr = es1370_fixup (s, addr);

    switch (addr) {
    case ES1370_REG_CONTROL:
        val = s->ctl;
        break;
    case ES1370_REG_STATUS:
        val = s->status;
        break;
    case ES1370_REG_MEMPAGE:
        val = s->mempage;
        break;
    case ES1370_REG_CODEC:
        val = s->codec;
        break;
    case ES1370_REG_SERIAL_CONTROL:
        val = s->sctl;
        break;

    case ES1370_REG_ADC_SCOUNT:
        d++;
    case ES1370_REG_DAC2_SCOUNT:
        d++;
    case ES1370_REG_DAC1_SCOUNT:
        val = d->scount;
#ifdef DEBUG_ES1370
        {
            uint32_t curr_count = d->scount >> 16;
            uint32_t count = d->scount & 0xffff;

            curr_count <<= d->shift;
            count <<= d->shift;
            dolog ("read scount curr %d, total %d\n", curr_count, count);
        }
#endif
        break;

    case ES1370_REG_ADC_FRAMECNT:
        d++;
    case ES1370_REG_DAC2_FRAMECNT:
        d++;
    case ES1370_REG_DAC1_FRAMECNT:
        val = d->frame_cnt;
#ifdef DEBUG_ES1370
        {
            uint32_t size = ((d->frame_cnt & 0xffff) + 1) << 2;
            uint32_t curr = ((d->frame_cnt >> 16) + 1) << 2;
            if (curr > size) {
                dolog ("read framecnt curr %d, size %d %d\n", curr, size,
                       curr > size);
            }
        }
#endif
        break;

    case ES1370_REG_ADC_FRAMEADR:
        d++;
    case ES1370_REG_DAC2_FRAMEADR:
        d++;
    case ES1370_REG_DAC1_FRAMEADR:
        val = d->frame_addr;
        break;

    case ES1370_REG_PHANTOM_FRAMECNT:
        val = ~0U;
        lwarn ("reading from phantom frame count\n");
        break;
    case ES1370_REG_PHANTOM_FRAMEADR:
        val = ~0U;
        lwarn ("reading from phantom frame address\n");
        break;

    default:
        val = ~0U;
        lwarn ("readl %#x -> %#x\n", addr, val);
        break;
    }
    return val;
}

static void es1370_transfer_audio (ES1370State *s, struct chan *d, int loop_sel,
                                   int max, int *irq)
{
    uint8_t tmpbuf[4096];
    uint32_t addr = d->frame_addr;
    int sc = d->scount & 0xffff;
    int csc = d->scount >> 16;
    int csc_bytes = (csc + 1) << d->shift;
    int cnt = d->frame_cnt >> 16;
    int size = d->frame_cnt & 0xffff;
    int left = ((size - cnt + 1) << 2) + d->leftover;
    int transferred = 0;
    int temp = audio_MIN (max, audio_MIN (left, csc_bytes));
    int index = (int)(d - &s->chan[0]);

    addr += (cnt << 2) + d->leftover;

    if (index == ADC_CHANNEL) {
        while (temp) {
            int acquired, to_copy;

            to_copy = audio_MIN ((size_t) temp, sizeof (tmpbuf));
            acquired = AUD_read (s->adc_voice, tmpbuf, to_copy);
            if (!acquired)
                break;

			pci_write_dma(s->dev, addr, tmpbuf, acquired);

            temp -= acquired;
            addr += acquired;
            transferred += acquired;
        }
    } else {
        SWVoiceOut *voice = s->dac_voice[index];

        while (temp) {
            int copied, to_copy;

            to_copy = audio_MIN ((size_t) temp, sizeof (tmpbuf));

			//write_log("t=%d tc=%d cnt=%d size=%d lo=%d csc=% d %08x-%08x\n", temp, to_copy, cnt, size, d->leftover, csc_bytes, addr, addr + to_copy - 1);

			pci_read_dma(s->dev, addr, tmpbuf, to_copy);
            copied = AUD_write (voice, tmpbuf, to_copy);
            if (!copied)
                break;
            temp -= copied;
            addr += copied;
            transferred += copied;
        }
    }

    if (csc_bytes == transferred) {
        *irq = 1;
        d->scount = sc | (sc << 16);
        ldebug ("sc = %d, rate = %f\n",
                (sc + 1) << d->shift,
                (sc + 1) / (double) 44100);
    } else {
        *irq = 0;
        d->scount = sc | (((csc_bytes - transferred - 1) >> d->shift) << 16);
    }

    cnt += (transferred + d->leftover) >> 2;

    if (s->sctl & loop_sel) {
        /* Bah, how stupid is that having a 0 represent true value?
           i just spent few hours on this shit */
        //AUD_log ("es1370: warning", "non looping mode\n");
    } else {
        d->frame_cnt = size;

        if ((uint32_t) cnt <= d->frame_cnt)
            d->frame_cnt |= cnt << 16;
    }

    d->leftover = (transferred + d->leftover) & 3;
}

static void es1370_run_channel (ES1370State *s, size_t chan, int free_or_avail)
{
    uint32_t new_status = s->status;
    int max_bytes, irq;
    struct chan *d = &s->chan[chan];
    const struct chan_bits *b = &es1370_chan_bits[chan];

    if (!(s->ctl & b->ctl_en) || (s->sctl & b->sctl_pause)) {
        return;
    }

    max_bytes = free_or_avail;
    max_bytes &= ~((1 << d->shift) - 1);
    if (!max_bytes) {
        return;
    }

    es1370_transfer_audio (s, d, b->sctl_loopsel, max_bytes, &irq);

    if (irq) {
        if (s->sctl & b->sctl_inten) {
            new_status |= b->stat_int;
        }
    }

    if (new_status != s->status) {
        es1370_update_status (s, new_status);
    }
}

static void es1370_dac1_callback (void *opaque, int free)
{
    ES1370State *s = (ES1370State*)opaque;

    es1370_run_channel (s, DAC1_CHANNEL, free);
}

static void es1370_dac2_callback (void *opaque, int free)
{
    ES1370State *s = (ES1370State*)opaque;

    es1370_run_channel (s, DAC2_CHANNEL, free);
}

static void es1370_adc_callback (void *opaque, int avail)
{
    ES1370State *s = (ES1370State*)opaque;

    es1370_run_channel (s, ADC_CHANNEL, avail);
}

static uint64_t es1370_read(void *opaque, hwaddr addr,
                            unsigned size)
{
    switch (size) {
    case 1:
        return es1370_readb(opaque, addr);
    case 2:
        return es1370_readw(opaque, addr);
    case 4:
        return es1370_readl(opaque, addr);
    default:
        return -1;
    }
}

static void es1370_write(void *opaque, hwaddr addr, uint64_t val64,
                      unsigned size)
{
    uint32_t val = (uint32_t)val64;
    switch (size) {
    case 1:
        es1370_writeb(opaque, addr, val);
        break;
    case 2:
        es1370_writew(opaque, addr, val);
        break;
    case 4:
        es1370_writel(opaque, addr, val);
        break;
    }
}

#if 0
static const MemoryRegionOps es1370_io_ops = {
    .read = es1370_read,
    .write = es1370_write,
    .impl = {
        .min_access_size = 1,
        .max_access_size = 4,
    },
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static const VMStateDescription vmstate_es1370_channel = {
    .name = "es1370_channel",
    .version_id = 2,
    .minimum_version_id = 2,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32 (shift, struct chan),
        VMSTATE_UINT32 (leftover, struct chan),
        VMSTATE_UINT32 (scount, struct chan),
        VMSTATE_UINT32 (frame_addr, struct chan),
        VMSTATE_UINT32 (frame_cnt, struct chan),
        VMSTATE_END_OF_LIST ()
    }
};
#endif

static int es1370_post_load (void *opaque, int version_id)
{
    uint32_t ctl, sctl;
    ES1370State *s = (ES1370State*)opaque;
    size_t i;

    for (i = 0; i < NB_CHANNELS; ++i) {
        if (i == ADC_CHANNEL) {
            if (s->adc_voice) {
                AUD_close_in (&s->card, s->adc_voice);
                s->adc_voice = NULL;
            }
        } else {
            if (s->dac_voice[i]) {
                AUD_close_out (&s->card, s->dac_voice[i]);
                s->dac_voice[i] = NULL;
            }
        }
    }

    ctl = s->ctl;
    sctl = s->sctl;
    s->ctl = 0;
    s->sctl = 0;
    es1370_update_voices (s, ctl, sctl);
    return 0;
}

#if 0
static const VMStateDescription vmstate_es1370 = {
    .name = "es1370",
    .version_id = 2,
    .minimum_version_id = 2,
    .post_load = es1370_post_load,
    .fields = (VMStateField[]) {
        VMSTATE_PCI_DEVICE (dev, ES1370State),
        VMSTATE_STRUCT_ARRAY (chan, ES1370State, NB_CHANNELS, 2,
                              vmstate_es1370_channel, struct chan),
        VMSTATE_UINT32 (ctl, ES1370State),
        VMSTATE_UINT32 (status, ES1370State),
        VMSTATE_UINT32 (mempage, ES1370State),
        VMSTATE_UINT32 (codec, ES1370State),
        VMSTATE_UINT32 (sctl, ES1370State),
        VMSTATE_END_OF_LIST ()
    }
};
#endif

static void es1370_on_reset (void *opaque)
{
    ES1370State *s = (ES1370State*)opaque;
    es1370_reset (s);
}

#if 0
static void es1370_realize(PCIDevice *dev, Error **errp)
{
    ES1370State *s = DO_UPCAST (ES1370State, dev, dev);
    uint8_t *c = s->dev.config;

    c[PCI_STATUS + 1] = PCI_STATUS_DEVSEL_SLOW >> 8;

#if 0
    c[PCI_CAPABILITY_LIST] = 0xdc;
    c[PCI_INTERRUPT_LINE] = 10;
    c[0xdc] = 0x00;
#endif

    c[PCI_INTERRUPT_PIN] = 1;
    c[PCI_MIN_GNT] = 0x0c;
    c[PCI_MAX_LAT] = 0x80;

    memory_region_init_io (&s->io, OBJECT(s), &es1370_io_ops, s, "es1370", 256);
    pci_register_bar (&s->dev, 0, PCI_BASE_ADDRESS_SPACE_IO, &s->io);
    qemu_register_reset (es1370_on_reset, s);

    AUD_register_card ("es1370", &s->card);
    es1370_reset (s);
}

static int es1370_init (PCIBus *bus)
{
    pci_create_simple (bus, -1, "ES1370");
    return 0;
}

static void es1370_class_init (ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS (klass);
    PCIDeviceClass *k = PCI_DEVICE_CLASS (klass);

    k->realize = es1370_realize;
    k->vendor_id = PCI_VENDOR_ID_ENSONIQ;
    k->device_id = PCI_DEVICE_ID_ENSONIQ_ES1370;
    k->class_id = PCI_CLASS_MULTIMEDIA_AUDIO;
    k->subsystem_vendor_id = 0x4942;
    k->subsystem_id = 0x4c4c;
    set_bit(DEVICE_CATEGORY_SOUND, dc->categories);
    dc->desc = "ENSONIQ AudioPCI ES1370";
    dc->vmsd = &vmstate_es1370;
}

static const TypeInfo es1370_info = {
    .name          = "ES1370",
    .parent        = TYPE_PCI_DEVICE,
    .instance_size = sizeof (ES1370State),
    .class_init    = es1370_class_init,
};

static void es1370_register_types (void)
{
    type_register_static (&es1370_info);
    pci_register_soundhw("es1370", "ENSONIQ AudioPCI ES1370", es1370_init);
}

type_init (es1370_register_types)

#endif

static void REGPARAM2 es1370_bput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	//write_log(_T("ES1370 BPUT %08x = %02x\n"), addr, b & 0xff);
	es1370_writeb(&es1370state, addr, b & 0xff);
}
static void REGPARAM2 es1370_wput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	//write_log(_T("ES1370 WPUT %08x = %04x\n"), addr, b & 0xffff);
	es1370_writew(&es1370state, addr, b & 0xffff);
}
static void REGPARAM2 es1370_lput(struct pci_board_state *pcibs, uaecptr addr, uae_u32 b)
{
	//write_log(_T("ES1370 LPUT %08x = %08x\n"), addr, b);
	es1370_writel(&es1370state, addr, b);
}
static uae_u32 REGPARAM2 es1370_bget(struct pci_board_state *pcibs, uaecptr addr)
{
	uae_u32 v = es1370_readb(&es1370state, addr);
	//write_log(_T("ES1370 BGET %08x = %02x\n"), addr, v);
	return v;
}
static uae_u32 REGPARAM2 es1370_wget(struct pci_board_state *pcibs, uaecptr addr)
{
	uae_u32 v = es1370_readw(&es1370state, addr);
	//write_log(_T("ES1370 WGET %08x = %04x\n"), addr, v);
	return v;
}
static uae_u32 REGPARAM2 es1370_lget(struct pci_board_state *pcibs, uaecptr addr)
{
	uae_u32 v = es1370_readl(&es1370state, addr);
	//write_log(_T("ES1370 LGET %08x = %08x\n"), addr, v);
	return v;
}

static void init(struct pci_board_state *pcibs)
{
	memset(&es1370state, 0, sizeof es1370state);
	es1370state.dev = pcibs;
	es1370state.device = pcibs->board;
}

static void es1370_reset(struct pci_board_state *pcibs)
{
	if (!es1370state.dev)
		return;
	es1370_on_reset(&es1370state);
}

static void es1370_free(struct pci_board_state *pcibs)
{
	if (!es1370state.dev)
		return;
}

static bool es1370_init(struct pci_board_state *pcibs, struct autoconfig_info *aci)
{
	init(pcibs);
	es1370state.irq_callback = pcibs->irq_callback;
	es1370_reset(&es1370state);
	return true;
}


static const struct pci_config es1370_pci_config =
{
	0x1274, 0x5000, 0, 0, 0, 0x040100, 0, 0x4942, 0x4c4c, 1, 0x0c, 0x3f, { 0x40 | 1, 0, 0, 0, 0, 0, 0 }
};

const struct pci_board es1370_pci_board =
{
	_T("ES1370"),
	&es1370_pci_config, es1370_init, es1370_free, es1370_reset, NULL, NULL,
	{
		{ es1370_lget, es1370_wget, es1370_bget, es1370_lput, es1370_wput, es1370_bput },
		{ NULL },
		{ NULL },
		{ NULL },
		{ NULL },
        { NULL },
        { NULL },
        { NULL }
    }
};
