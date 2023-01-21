#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "custom.h"
#include "memory.h"
#include "blitter.h"
#include "blitfunc.h"

void blitdofast_0 (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
int i,j;
uae_u32 totald = 0;
uae_u16 ashift = bltcon0 >> 12;
uae_u16 bshift = bltcon1 >> 12;
uae_u32 dstd=0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = (0) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd += 2; }
	}
	if (ptd) ptd += b->bltdmod;
}
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_desc_0 (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
uae_u32 totald = 0;
int i,j;
uae_u16 ashift = 16 - (bltcon0 >> 12);
uae_u16 bshift = 16 - (bltcon1 >> 12);
uae_u32 dstd = 0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = (0) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd -= 2; }
	}
	if (ptd) ptd -= b->bltdmod;
}
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_a (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
int i,j;
uae_u32 totald = 0;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = bltcon0 >> 12;
uae_u16 bshift = bltcon1 >> 12;
uae_u32 dstd=0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc += 2; }
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta += 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)b->bltaold << 16) | bltadat) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((~srca & srcc)) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd += 2; }
	}
	if (pta) pta += b->bltamod;
	if (ptc) ptc += b->bltcmod;
	if (ptd) ptd += b->bltdmod;
}
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_desc_a (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
uae_u32 totald = 0;
int i,j;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = 16 - (bltcon0 >> 12);
uae_u16 bshift = 16 - (bltcon1 >> 12);
uae_u32 dstd = 0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc -= 2; }
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta -= 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)bltadat << 16) | b->bltaold) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((~srca & srcc)) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd -= 2; }
	}
	if (pta) pta -= b->bltamod;
	if (ptc) ptc -= b->bltcmod;
	if (ptd) ptd -= b->bltdmod;
}
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_2a (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
int i,j;
uae_u32 totald = 0;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = bltcon0 >> 12;
uae_u16 bshift = bltcon1 >> 12;
uae_u32 dstd=0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc += 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb += 2;
			srcb = (((uae_u32)b->bltbold << 16) | bltbdat) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta += 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)b->bltaold << 16) | bltadat) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srcc & ~(srca & srcb))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd += 2; }
	}
	if (pta) pta += b->bltamod;
	if (ptb) ptb += b->bltbmod;
	if (ptc) ptc += b->bltcmod;
	if (ptd) ptd += b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_desc_2a (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
uae_u32 totald = 0;
int i,j;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = 16 - (bltcon0 >> 12);
uae_u16 bshift = 16 - (bltcon1 >> 12);
uae_u32 dstd = 0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc -= 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb -= 2;
			srcb = ((bltbdat << 16) | b->bltbold) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta -= 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)bltadat << 16) | b->bltaold) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srcc & ~(srca & srcb))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd -= 2; }
	}
	if (pta) pta -= b->bltamod;
	if (ptb) ptb -= b->bltbmod;
	if (ptc) ptc -= b->bltcmod;
	if (ptd) ptd -= b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_30 (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
int i,j;
uae_u32 totald = 0;
uae_u32 srcb = b->bltbhold;
uae_u16 ashift = bltcon0 >> 12;
uae_u16 bshift = bltcon1 >> 12;
uae_u32 dstd=0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb += 2;
			srcb = (((uae_u32)b->bltbold << 16) | bltbdat) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta += 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)b->bltaold << 16) | bltadat) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srca & ~srcb)) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd += 2; }
	}
	if (pta) pta += b->bltamod;
	if (ptb) ptb += b->bltbmod;
	if (ptd) ptd += b->bltdmod;
}
b->bltbhold = srcb;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_desc_30 (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
uae_u32 totald = 0;
int i,j;
uae_u32 srcb = b->bltbhold;
uae_u16 ashift = 16 - (bltcon0 >> 12);
uae_u16 bshift = 16 - (bltcon1 >> 12);
uae_u32 dstd = 0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb -= 2;
			srcb = ((bltbdat << 16) | b->bltbold) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta -= 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)bltadat << 16) | b->bltaold) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srca & ~srcb)) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd -= 2; }
	}
	if (pta) pta -= b->bltamod;
	if (ptb) ptb -= b->bltbmod;
	if (ptd) ptd -= b->bltdmod;
}
b->bltbhold = srcb;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_3a (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
int i,j;
uae_u32 totald = 0;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = bltcon0 >> 12;
uae_u16 bshift = bltcon1 >> 12;
uae_u32 dstd=0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc += 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb += 2;
			srcb = (((uae_u32)b->bltbold << 16) | bltbdat) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta += 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)b->bltaold << 16) | bltadat) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srcb ^ (srca | (srcb ^ srcc)))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd += 2; }
	}
	if (pta) pta += b->bltamod;
	if (ptb) ptb += b->bltbmod;
	if (ptc) ptc += b->bltcmod;
	if (ptd) ptd += b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_desc_3a (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
uae_u32 totald = 0;
int i,j;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = 16 - (bltcon0 >> 12);
uae_u16 bshift = 16 - (bltcon1 >> 12);
uae_u32 dstd = 0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc -= 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb -= 2;
			srcb = ((bltbdat << 16) | b->bltbold) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta -= 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)bltadat << 16) | b->bltaold) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srcb ^ (srca | (srcb ^ srcc)))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd -= 2; }
	}
	if (pta) pta -= b->bltamod;
	if (ptb) ptb -= b->bltbmod;
	if (ptc) ptc -= b->bltcmod;
	if (ptd) ptd -= b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_3c (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
int i,j;
uae_u32 totald = 0;
uae_u32 srcb = b->bltbhold;
uae_u16 ashift = bltcon0 >> 12;
uae_u16 bshift = bltcon1 >> 12;
uae_u32 dstd=0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb += 2;
			srcb = (((uae_u32)b->bltbold << 16) | bltbdat) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta += 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)b->bltaold << 16) | bltadat) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srca ^ srcb)) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd += 2; }
	}
	if (pta) pta += b->bltamod;
	if (ptb) ptb += b->bltbmod;
	if (ptd) ptd += b->bltdmod;
}
b->bltbhold = srcb;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_desc_3c (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
uae_u32 totald = 0;
int i,j;
uae_u32 srcb = b->bltbhold;
uae_u16 ashift = 16 - (bltcon0 >> 12);
uae_u16 bshift = 16 - (bltcon1 >> 12);
uae_u32 dstd = 0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb -= 2;
			srcb = ((bltbdat << 16) | b->bltbold) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta -= 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)bltadat << 16) | b->bltaold) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srca ^ srcb)) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd -= 2; }
	}
	if (pta) pta -= b->bltamod;
	if (ptb) ptb -= b->bltbmod;
	if (ptd) ptd -= b->bltdmod;
}
b->bltbhold = srcb;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_4a (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
int i,j;
uae_u32 totald = 0;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = bltcon0 >> 12;
uae_u16 bshift = bltcon1 >> 12;
uae_u32 dstd=0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc += 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb += 2;
			srcb = (((uae_u32)b->bltbold << 16) | bltbdat) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta += 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)b->bltaold << 16) | bltadat) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srcc ^ (srca & (srcb | srcc)))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd += 2; }
	}
	if (pta) pta += b->bltamod;
	if (ptb) ptb += b->bltbmod;
	if (ptc) ptc += b->bltcmod;
	if (ptd) ptd += b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_desc_4a (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
uae_u32 totald = 0;
int i,j;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = 16 - (bltcon0 >> 12);
uae_u16 bshift = 16 - (bltcon1 >> 12);
uae_u32 dstd = 0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc -= 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb -= 2;
			srcb = ((bltbdat << 16) | b->bltbold) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta -= 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)bltadat << 16) | b->bltaold) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srcc ^ (srca & (srcb | srcc)))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd -= 2; }
	}
	if (pta) pta -= b->bltamod;
	if (ptb) ptb -= b->bltbmod;
	if (ptc) ptc -= b->bltcmod;
	if (ptd) ptd -= b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_6a (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
int i,j;
uae_u32 totald = 0;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = bltcon0 >> 12;
uae_u16 bshift = bltcon1 >> 12;
uae_u32 dstd=0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc += 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb += 2;
			srcb = (((uae_u32)b->bltbold << 16) | bltbdat) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta += 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)b->bltaold << 16) | bltadat) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srcc ^ (srca & srcb))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd += 2; }
	}
	if (pta) pta += b->bltamod;
	if (ptb) ptb += b->bltbmod;
	if (ptc) ptc += b->bltcmod;
	if (ptd) ptd += b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_desc_6a (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
uae_u32 totald = 0;
int i,j;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = 16 - (bltcon0 >> 12);
uae_u16 bshift = 16 - (bltcon1 >> 12);
uae_u32 dstd = 0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc -= 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb -= 2;
			srcb = ((bltbdat << 16) | b->bltbold) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta -= 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)bltadat << 16) | b->bltaold) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srcc ^ (srca & srcb))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd -= 2; }
	}
	if (pta) pta -= b->bltamod;
	if (ptb) ptb -= b->bltbmod;
	if (ptc) ptc -= b->bltcmod;
	if (ptd) ptd -= b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_8a (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
int i,j;
uae_u32 totald = 0;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = bltcon0 >> 12;
uae_u16 bshift = bltcon1 >> 12;
uae_u32 dstd=0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc += 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb += 2;
			srcb = (((uae_u32)b->bltbold << 16) | bltbdat) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta += 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)b->bltaold << 16) | bltadat) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srcc & (~srca | srcb))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd += 2; }
	}
	if (pta) pta += b->bltamod;
	if (ptb) ptb += b->bltbmod;
	if (ptc) ptc += b->bltcmod;
	if (ptd) ptd += b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_desc_8a (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
uae_u32 totald = 0;
int i,j;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = 16 - (bltcon0 >> 12);
uae_u16 bshift = 16 - (bltcon1 >> 12);
uae_u32 dstd = 0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc -= 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb -= 2;
			srcb = ((bltbdat << 16) | b->bltbold) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta -= 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)bltadat << 16) | b->bltaold) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srcc & (~srca | srcb))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd -= 2; }
	}
	if (pta) pta -= b->bltamod;
	if (ptb) ptb -= b->bltbmod;
	if (ptc) ptc -= b->bltcmod;
	if (ptd) ptd -= b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_8c (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
int i,j;
uae_u32 totald = 0;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = bltcon0 >> 12;
uae_u16 bshift = bltcon1 >> 12;
uae_u32 dstd=0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc += 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb += 2;
			srcb = (((uae_u32)b->bltbold << 16) | bltbdat) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta += 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)b->bltaold << 16) | bltadat) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srcb & (~srca | srcc))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd += 2; }
	}
	if (pta) pta += b->bltamod;
	if (ptb) ptb += b->bltbmod;
	if (ptc) ptc += b->bltcmod;
	if (ptd) ptd += b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_desc_8c (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
uae_u32 totald = 0;
int i,j;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = 16 - (bltcon0 >> 12);
uae_u16 bshift = 16 - (bltcon1 >> 12);
uae_u32 dstd = 0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc -= 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb -= 2;
			srcb = ((bltbdat << 16) | b->bltbold) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta -= 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)bltadat << 16) | b->bltaold) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srcb & (~srca | srcc))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd -= 2; }
	}
	if (pta) pta -= b->bltamod;
	if (ptb) ptb -= b->bltbmod;
	if (ptc) ptc -= b->bltcmod;
	if (ptd) ptd -= b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_9a (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
int i,j;
uae_u32 totald = 0;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = bltcon0 >> 12;
uae_u16 bshift = bltcon1 >> 12;
uae_u32 dstd=0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc += 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb += 2;
			srcb = (((uae_u32)b->bltbold << 16) | bltbdat) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta += 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)b->bltaold << 16) | bltadat) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srcc ^ (srca & ~srcb))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd += 2; }
	}
	if (pta) pta += b->bltamod;
	if (ptb) ptb += b->bltbmod;
	if (ptc) ptc += b->bltcmod;
	if (ptd) ptd += b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_desc_9a (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
uae_u32 totald = 0;
int i,j;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = 16 - (bltcon0 >> 12);
uae_u16 bshift = 16 - (bltcon1 >> 12);
uae_u32 dstd = 0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc -= 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb -= 2;
			srcb = ((bltbdat << 16) | b->bltbold) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta -= 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)bltadat << 16) | b->bltaold) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srcc ^ (srca & ~srcb))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd -= 2; }
	}
	if (pta) pta -= b->bltamod;
	if (ptb) ptb -= b->bltbmod;
	if (ptc) ptc -= b->bltcmod;
	if (ptd) ptd -= b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_a8 (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
int i,j;
uae_u32 totald = 0;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = bltcon0 >> 12;
uae_u16 bshift = bltcon1 >> 12;
uae_u32 dstd=0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc += 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb += 2;
			srcb = (((uae_u32)b->bltbold << 16) | bltbdat) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta += 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)b->bltaold << 16) | bltadat) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srcc & (srca | srcb))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd += 2; }
	}
	if (pta) pta += b->bltamod;
	if (ptb) ptb += b->bltbmod;
	if (ptc) ptc += b->bltcmod;
	if (ptd) ptd += b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_desc_a8 (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
uae_u32 totald = 0;
int i,j;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = 16 - (bltcon0 >> 12);
uae_u16 bshift = 16 - (bltcon1 >> 12);
uae_u32 dstd = 0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc -= 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb -= 2;
			srcb = ((bltbdat << 16) | b->bltbold) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta -= 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)bltadat << 16) | b->bltaold) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srcc & (srca | srcb))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd -= 2; }
	}
	if (pta) pta -= b->bltamod;
	if (ptb) ptb -= b->bltbmod;
	if (ptc) ptc -= b->bltcmod;
	if (ptd) ptd -= b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_aa (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
int i,j;
uae_u32 totald = 0;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = bltcon0 >> 12;
uae_u16 bshift = bltcon1 >> 12;
uae_u32 dstd=0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc += 2; }
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = (srcc) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd += 2; }
	}
	if (ptc) ptc += b->bltcmod;
	if (ptd) ptd += b->bltdmod;
}
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_desc_aa (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
uae_u32 totald = 0;
int i,j;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = 16 - (bltcon0 >> 12);
uae_u16 bshift = 16 - (bltcon1 >> 12);
uae_u32 dstd = 0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc -= 2; }
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = (srcc) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd -= 2; }
	}
	if (ptc) ptc -= b->bltcmod;
	if (ptd) ptd -= b->bltdmod;
}
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_b1 (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
int i,j;
uae_u32 totald = 0;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = bltcon0 >> 12;
uae_u16 bshift = bltcon1 >> 12;
uae_u32 dstd=0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc += 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb += 2;
			srcb = (((uae_u32)b->bltbold << 16) | bltbdat) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta += 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)b->bltaold << 16) | bltadat) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = (~(srca ^ (srcc | (srca ^ srcb)))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd += 2; }
	}
	if (pta) pta += b->bltamod;
	if (ptb) ptb += b->bltbmod;
	if (ptc) ptc += b->bltcmod;
	if (ptd) ptd += b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_desc_b1 (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
uae_u32 totald = 0;
int i,j;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = 16 - (bltcon0 >> 12);
uae_u16 bshift = 16 - (bltcon1 >> 12);
uae_u32 dstd = 0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc -= 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb -= 2;
			srcb = ((bltbdat << 16) | b->bltbold) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta -= 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)bltadat << 16) | b->bltaold) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = (~(srca ^ (srcc | (srca ^ srcb)))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd -= 2; }
	}
	if (pta) pta -= b->bltamod;
	if (ptb) ptb -= b->bltbmod;
	if (ptc) ptc -= b->bltcmod;
	if (ptd) ptd -= b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_ca (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
int i,j;
uae_u32 totald = 0;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = bltcon0 >> 12;
uae_u16 bshift = bltcon1 >> 12;
uae_u32 dstd=0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc += 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb += 2;
			srcb = (((uae_u32)b->bltbold << 16) | bltbdat) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta += 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)b->bltaold << 16) | bltadat) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srcc ^ (srca & (srcb ^ srcc)))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd += 2; }
	}
	if (pta) pta += b->bltamod;
	if (ptb) ptb += b->bltbmod;
	if (ptc) ptc += b->bltcmod;
	if (ptd) ptd += b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_desc_ca (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
uae_u32 totald = 0;
int i,j;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = 16 - (bltcon0 >> 12);
uae_u16 bshift = 16 - (bltcon1 >> 12);
uae_u32 dstd = 0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc -= 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb -= 2;
			srcb = ((bltbdat << 16) | b->bltbold) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta -= 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)bltadat << 16) | b->bltaold) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srcc ^ (srca & (srcb ^ srcc)))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd -= 2; }
	}
	if (pta) pta -= b->bltamod;
	if (ptb) ptb -= b->bltbmod;
	if (ptc) ptc -= b->bltcmod;
	if (ptd) ptd -= b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_cc (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
int i,j;
uae_u32 totald = 0;
uae_u32 srcb = b->bltbhold;
uae_u16 ashift = bltcon0 >> 12;
uae_u16 bshift = bltcon1 >> 12;
uae_u32 dstd=0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb += 2;
			srcb = (((uae_u32)b->bltbold << 16) | bltbdat) >> bshift;
			b->bltbold = bltbdat;
		}
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = (srcb) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd += 2; }
	}
	if (ptb) ptb += b->bltbmod;
	if (ptd) ptd += b->bltdmod;
}
b->bltbhold = srcb;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_desc_cc (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
uae_u32 totald = 0;
int i,j;
uae_u32 srcb = b->bltbhold;
uae_u16 ashift = 16 - (bltcon0 >> 12);
uae_u16 bshift = 16 - (bltcon1 >> 12);
uae_u32 dstd = 0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb -= 2;
			srcb = ((bltbdat << 16) | b->bltbold) >> bshift;
			b->bltbold = bltbdat;
		}
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = (srcb) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd -= 2; }
	}
	if (ptb) ptb -= b->bltbmod;
	if (ptd) ptd -= b->bltdmod;
}
b->bltbhold = srcb;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_d8 (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
int i,j;
uae_u32 totald = 0;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = bltcon0 >> 12;
uae_u16 bshift = bltcon1 >> 12;
uae_u32 dstd=0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc += 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb += 2;
			srcb = (((uae_u32)b->bltbold << 16) | bltbdat) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta += 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)b->bltaold << 16) | bltadat) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srca ^ (srcc & (srca ^ srcb)))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd += 2; }
	}
	if (pta) pta += b->bltamod;
	if (ptb) ptb += b->bltbmod;
	if (ptc) ptc += b->bltcmod;
	if (ptd) ptd += b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_desc_d8 (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
uae_u32 totald = 0;
int i,j;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = 16 - (bltcon0 >> 12);
uae_u16 bshift = 16 - (bltcon1 >> 12);
uae_u32 dstd = 0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc -= 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb -= 2;
			srcb = ((bltbdat << 16) | b->bltbold) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta -= 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)bltadat << 16) | b->bltaold) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srca ^ (srcc & (srca ^ srcb)))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd -= 2; }
	}
	if (pta) pta -= b->bltamod;
	if (ptb) ptb -= b->bltbmod;
	if (ptc) ptc -= b->bltcmod;
	if (ptd) ptd -= b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_e2 (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
int i,j;
uae_u32 totald = 0;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = bltcon0 >> 12;
uae_u16 bshift = bltcon1 >> 12;
uae_u32 dstd=0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc += 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb += 2;
			srcb = (((uae_u32)b->bltbold << 16) | bltbdat) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta += 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)b->bltaold << 16) | bltadat) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srcc ^ (srcb & (srca ^ srcc)))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd += 2; }
	}
	if (pta) pta += b->bltamod;
	if (ptb) ptb += b->bltbmod;
	if (ptc) ptc += b->bltcmod;
	if (ptd) ptd += b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_desc_e2 (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
uae_u32 totald = 0;
int i,j;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = 16 - (bltcon0 >> 12);
uae_u16 bshift = 16 - (bltcon1 >> 12);
uae_u32 dstd = 0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc -= 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb -= 2;
			srcb = ((bltbdat << 16) | b->bltbold) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta -= 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)bltadat << 16) | b->bltaold) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srcc ^ (srcb & (srca ^ srcc)))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd -= 2; }
	}
	if (pta) pta -= b->bltamod;
	if (ptb) ptb -= b->bltbmod;
	if (ptc) ptc -= b->bltcmod;
	if (ptd) ptd -= b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_ea (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
int i,j;
uae_u32 totald = 0;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = bltcon0 >> 12;
uae_u16 bshift = bltcon1 >> 12;
uae_u32 dstd=0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc += 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb += 2;
			srcb = (((uae_u32)b->bltbold << 16) | bltbdat) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta += 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)b->bltaold << 16) | bltadat) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srcc | (srca & srcb))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd += 2; }
	}
	if (pta) pta += b->bltamod;
	if (ptb) ptb += b->bltbmod;
	if (ptc) ptc += b->bltcmod;
	if (ptd) ptd += b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_desc_ea (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
uae_u32 totald = 0;
int i,j;
uae_u32 srcb = b->bltbhold;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = 16 - (bltcon0 >> 12);
uae_u16 bshift = 16 - (bltcon1 >> 12);
uae_u32 dstd = 0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc -= 2; }
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb -= 2;
			srcb = ((bltbdat << 16) | b->bltbold) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta -= 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)bltadat << 16) | b->bltaold) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srcc | (srca & srcb))) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd -= 2; }
	}
	if (pta) pta -= b->bltamod;
	if (ptb) ptb -= b->bltbmod;
	if (ptc) ptc -= b->bltcmod;
	if (ptd) ptd -= b->bltdmod;
}
b->bltbhold = srcb;
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_f0 (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
int i,j;
uae_u32 totald = 0;
uae_u16 ashift = bltcon0 >> 12;
uae_u16 bshift = bltcon1 >> 12;
uae_u32 dstd=0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta += 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)b->bltaold << 16) | bltadat) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = (srca) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd += 2; }
	}
	if (pta) pta += b->bltamod;
	if (ptd) ptd += b->bltdmod;
}
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_desc_f0 (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
uae_u32 totald = 0;
int i,j;
uae_u16 ashift = 16 - (bltcon0 >> 12);
uae_u16 bshift = 16 - (bltcon1 >> 12);
uae_u32 dstd = 0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta -= 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)bltadat << 16) | b->bltaold) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = (srca) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd -= 2; }
	}
	if (pta) pta -= b->bltamod;
	if (ptd) ptd -= b->bltdmod;
}
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_fa (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
int i,j;
uae_u32 totald = 0;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = bltcon0 >> 12;
uae_u16 bshift = bltcon1 >> 12;
uae_u32 dstd=0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc += 2; }
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta += 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)b->bltaold << 16) | bltadat) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srca | srcc)) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd += 2; }
	}
	if (pta) pta += b->bltamod;
	if (ptc) ptc += b->bltcmod;
	if (ptd) ptd += b->bltdmod;
}
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_desc_fa (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
uae_u32 totald = 0;
int i,j;
uae_u32 srcc = b->bltcdat;
uae_u16 ashift = 16 - (bltcon0 >> 12);
uae_u16 bshift = 16 - (bltcon1 >> 12);
uae_u32 dstd = 0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptc) { srcc = chipmem_wget_indirect(ptc); ptc -= 2; }
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta -= 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)bltadat << 16) | b->bltaold) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srca | srcc)) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd -= 2; }
	}
	if (pta) pta -= b->bltamod;
	if (ptc) ptc -= b->bltcmod;
	if (ptd) ptd -= b->bltdmod;
}
b->bltcdat = srcc;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_fc (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
int i,j;
uae_u32 totald = 0;
uae_u32 srcb = b->bltbhold;
uae_u16 ashift = bltcon0 >> 12;
uae_u16 bshift = bltcon1 >> 12;
uae_u32 dstd=0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb += 2;
			srcb = (((uae_u32)b->bltbold << 16) | bltbdat) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta += 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)b->bltaold << 16) | bltadat) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srca | srcb)) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd += 2; }
	}
	if (pta) pta += b->bltamod;
	if (ptb) ptb += b->bltbmod;
	if (ptd) ptd += b->bltdmod;
}
b->bltbhold = srcb;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
void blitdofast_desc_fc (uaecptr pta, uaecptr ptb, uaecptr ptc, uaecptr ptd, struct bltinfo *b)
{
uae_u32 totald = 0;
int i,j;
uae_u32 srcb = b->bltbhold;
uae_u16 ashift = 16 - (bltcon0 >> 12);
uae_u16 bshift = 16 - (bltcon1 >> 12);
uae_u32 dstd = 0;
uaecptr dstp = 0;
for (j = 0; j < b->vblitsize; j++) {
	for (i = 0; i < b->hblitsize; i++) {
		uae_u32 bltadat, srca;
		if (ptb) {
			uae_u32 bltbdat = b->bltbdat = chipmem_wget_indirect(ptb); ptb -= 2;
			srcb = ((bltbdat << 16) | b->bltbold) >> bshift;
			b->bltbold = bltbdat;
		}
		if (pta) { bltadat = b->bltadat = chipmem_wget_indirect(pta); pta -= 2; } else { bltadat = b->bltadat; }
		bltadat &= blit_masktable[i];
		srca = (((uae_u32)bltadat << 16) | b->bltaold) >> ashift;
		b->bltaold = bltadat;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
		dstd = ((srca | srcb)) & 0xFFFF;
		totald |= dstd;
		if (ptd) { dstp = ptd; ptd -= 2; }
	}
	if (pta) pta -= b->bltamod;
	if (ptb) ptb -= b->bltbmod;
	if (ptd) ptd -= b->bltdmod;
}
b->bltbhold = srcb;
		if (dstp) chipmem_wput_indirect(dstp, dstd);
if (totald != 0) b->blitzero = 0;
}
