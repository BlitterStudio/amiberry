/*
 *
 * Support for reading .SCP (Supercard Pro) disk flux dumps.
 * 
 * By Keir Fraser in 2014.
 *
 * This file is free and unencumbered software released into the public domain.
 * For more information, please refer to <http://unlicense.org/>
 */

#include "sysconfig.h"
#include "sysdeps.h"

#include "scp.h"
#include "zfile.h"
#include "gui.h"
#include "uae.h"
#include "uae/endian.h"

#include <stdint.h>
#if defined __MACH__
#include <machine/endian.h>
#include <libkern/OSByteOrder.h>
#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)
#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)
#endif

#define MAX_REVS 5

enum pll_mode {
    PLL_fixed_clock, /* Fixed clock, snap phase to flux transitions. */
    PLL_variable_clock, /* Variable clock, snap phase to flux transitions. */
    PLL_authentic /* Variable clock, do not snap phase to flux transition. */
};

struct scpdrive {
    struct zfile *zf;

    /* Current track number. */
    unsigned int track;

    /* Raw track data. */
    uint16_t *dat;
    unsigned int datsz;

    unsigned int revs;       /* stored disk revolutions */
    unsigned int dat_idx;    /* current index into dat[] */
    unsigned int index_pos;  /* next index offset */
    unsigned int rev;

    int total_ticks;         /* total ticks to final index pulse */
    int acc_ticks;           /* accumulated ticks so far */

    unsigned int index_off[MAX_REVS]; /* data offsets of each index */

    /* Accumulated read latency in nanosecs. */
    uint64_t latency;

    /* Flux-based streams: Authentic emulation of FDC PLL behaviour? */
    enum pll_mode pll_mode;

    /* Flux-based streams. */
    int flux;                /* Nanoseconds to next flux reversal */
    int clock, clock_centre; /* Clock base value in nanoseconds */
    unsigned int clocked_zeros;
};
static struct scpdrive drive[4];

#define CLOCK_CENTRE  2000   /* 2000ns = 2us */
#define CLOCK_MAX_ADJ 10     /* +/- 10% adjustment */
#define CLOCK_MIN(_c) (((_c) * (100 - CLOCK_MAX_ADJ)) / 100)
#define CLOCK_MAX(_c) (((_c) * (100 + CLOCK_MAX_ADJ)) / 100)

#define SCK_NS_PER_TICK (25u)

int scp_open(struct zfile *zf, int drv, int *num_tracks)
{
    struct scpdrive *d = &drive[drv];
	uint8_t header[0x10] = { 0 };

    scp_close(drv);

    zfile_fread(header, sizeof(header), 1, zf);

    if (memcmp(header, "SCP", 3) != 0) {
        write_log(_T("SCP file header missing\n"));
		return 0;
    }

    if (header[5] == 0) {
        write_log(_T("SCP file has invalid revolution count (%u)\n"), header[5]);
        return 0;
    }

    if (header[9] != 0 && header[9] != 16) {
        write_log(_T("SCP file has unsupported bit cell time width (%u)\n"),
                  header[9]);
        return 0;
    }

    d->zf = zf;
    d->revs = min((int)header[5], MAX_REVS);
	*num_tracks = header[7] + 1;

    return 1;
}

void scp_close(int drv)
{
    struct scpdrive *d = &drive[drv];
    if (!d->revs)
        return;
    xfree(d->dat);
    memset(d, 0, sizeof(*d));
}

int scp_loadtrack(
    uae_u16 *mfmbuf, uae_u16 *tracktiming, int drv,
    int track, int *tracklength, int *multirev,
    int *gapoffset, int *nextrev, bool setrev)
{
    struct scpdrive *d = &drive[drv];
    uint8_t trk_header[4];
    uint32_t longwords[3];
    unsigned int rev, trkoffset[MAX_REVS];
    uint32_t hdr_offset, tdh_offset;

    *multirev = 1;
    *gapoffset = -1;

    xfree(d->dat);
    d->dat = NULL;
    d->datsz = 0;
    
    hdr_offset = 0x10 + track*sizeof(uint32_t);

    zfile_fseek(d->zf, hdr_offset, SEEK_SET);

    zfile_fread(longwords, sizeof(uint32_t), 1, d->zf);
    tdh_offset = le32toh(longwords[0]);

    zfile_fseek(d->zf, tdh_offset, SEEK_SET);

    zfile_fread(trk_header, sizeof(trk_header), 1, d->zf);
    if (memcmp(trk_header, "TRK", 3) != 0)
        return 0;

    if (trk_header[3] != track)
        return 0;

    d->total_ticks = 0;
    for (rev = 0 ; rev < d->revs ; rev++) {
        zfile_fread(longwords, sizeof(longwords), 1, d->zf);
        trkoffset[rev] = tdh_offset + le32toh(longwords[2]);
        d->index_off[rev] = le32toh(longwords[1]);
        d->total_ticks += le32toh(longwords[0]);
        d->datsz += d->index_off[rev];
    }

    d->dat = xmalloc(uint16_t, d->datsz * sizeof(d->dat[0]));
    d->datsz = 0;

    for (rev = 0 ; rev < d->revs ; rev++) {
        zfile_fseek(d->zf, trkoffset[rev], SEEK_SET);
        zfile_fread(&d->dat[d->datsz],
                    d->index_off[rev] * sizeof(d->dat[0]), 1,
                    d->zf);
        d->datsz += d->index_off[rev];
        d->index_off[rev] = d->datsz;
    }

    d->track = track;
    d->pll_mode = PLL_authentic;
    d->dat_idx = 0;
    d->index_pos = d->index_off[0];
    d->clock = d->clock_centre = CLOCK_CENTRE;
    d->rev = 0;
    d->flux = 0;
    d->clocked_zeros = 0;
    d->acc_ticks = 0;

    scp_loadrevolution(mfmbuf, drv, tracktiming, tracklength);
    return 1;
}

static int scp_next_flux(struct scpdrive *d)
{
    uint32_t val = 0, flux, t;

    if (d->rev == d->revs) {
        d->rev = d->dat_idx = 0;
        /* We are wrapping back to the start of the dump. Unless a flux
         * reversal sits exactly on the index we have some time to donate to 
         * the first reversal of the first revolution. */
        val = d->total_ticks - d->acc_ticks;
        d->acc_ticks = 0 - val;
    }

    for (;;) {
        if (d->dat_idx >= d->index_pos) {
            d->index_pos = d->index_off[++d->rev % d->revs];
            return -1;
        }

        t = be16toh(d->dat[d->dat_idx++]);

        if (t == 0) { /* overflow */
            val += 0x10000;
            continue;
        }

        val += t;
        break;
    }

    d->acc_ticks += val;

    flux = val * SCK_NS_PER_TICK;
    return (int)flux;
}

static int flux_next_bit(struct scpdrive *d)
{
    int new_flux;

    while (d->flux < (d->clock/2)) {
        if ((new_flux = scp_next_flux(d)) == -1)
            return -1;
        d->flux += new_flux;
        d->clocked_zeros = 0;
    }

    d->latency += d->clock;
    d->flux -= d->clock;

    if (d->flux >= (d->clock/2)) {
        d->clocked_zeros++;
        return 0;
    }

    if (d->pll_mode != PLL_fixed_clock) {
        /* PLL: Adjust clock frequency according to phase mismatch. */
        if ((d->clocked_zeros >= 1) && (d->clocked_zeros <= 3)) {
            /* In sync: adjust base clock by 10% of phase mismatch. */
            int diff = d->flux / (int)(d->clocked_zeros + 1);
            d->clock += diff / 10;
        } else {
            /* Out of sync: adjust base clock towards centre. */
            d->clock += (d->clock_centre - d->clock) / 10;
        }

        /* Clamp the clock's adjustment range. */
        d->clock = max(CLOCK_MIN(d->clock_centre),
                          min(CLOCK_MAX(d->clock_centre), d->clock));
    } else {
        d->clock = d->clock_centre;
    }

    /* Authentic PLL: Do not snap the timing window to each flux transition. */
    new_flux = (d->pll_mode == PLL_authentic) ? d->flux / 2 : 0;
    d->latency += d->flux - new_flux;
    d->flux = new_flux;

    return 1;
}

void scp_loadrevolution(
    uae_u16 *mfmbuf, int drv, uae_u16 *tracktiming,
    int *tracklength)
{
    struct scpdrive *d = &drive[drv];
    uint64_t prev_latency;
    uint32_t av_latency;
    unsigned int i, j;
    int b;

    d->latency = prev_latency = 0;
    for (i = 0; (b = flux_next_bit(d)) != -1; i++) {
        if ((i & 15) == 0)
            mfmbuf[i>>4] = 0;
        if (b)
            mfmbuf[i>>4] |= 0x8000u >> (i&15);
        if ((i & 7) == 7) {
            tracktiming[i>>3] = (uae_u16)(d->latency - prev_latency);
            prev_latency = d->latency;
        }
    }

    if (i & 7)
        tracktiming[i>>3] = (uae_u16)(((d->latency - prev_latency) * 8) / (i & 7));

    av_latency = (uint32_t)(prev_latency / (i>>3));
    for (j = 0; j < (i+7)>>3; j++)
        tracktiming[j] = ((uint32_t)tracktiming[j] * 1000u) / av_latency;

    *tracklength = i;
}
