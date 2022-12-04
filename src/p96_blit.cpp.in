
#if BLT_SIZE == 3
static void NOINLINE BLT_NAME(unsigned int w, unsigned int h, uae_u8 *src, uae_u8 *dst, int srcpitch, int dstpitch, uae_u32 rgbmask)
{
	uae_u8 *src2 = src;
	uae_u8 *dst2 = dst;
	uae_u32 *src2_32 = (uae_u32*)src;
	uae_u32 *dst2_32 = (uae_u32*)dst;
	unsigned int y, x, ww, xxd;
	w *= BLT_SIZE;
	ww = w / 4;
	xxd = w - (ww * 4);
	if (src < dst && src + h * srcpitch > dst) {
		dst2 += h * dstpitch + w;
		src2 += h * srcpitch + w;
		for (y = 0; y < h; y++) {
			dst2 -= dstpitch;
			src2 -= srcpitch;
			uae_u8 *src_8;
			uae_u8 *dst_8;
			src_8 = (uae_u8*)src2;
			dst_8 = (uae_u8*)dst2;
			for (x = 0; x < xxd; x++) {
				src_8--;
				dst_8--;
				uae_u32 sv = *src_8;
				uae_u32 dv = *dst_8;
				BLT_FUNC(&sv, &dv);
				*dst_8 = (uae_u8)dv;
			}
			uae_u32 *src_32 = (uae_u32*)src_8;
			uae_u32 *dst_32 = (uae_u32*)dst_8;
			for (x = 0; x < ww; x++) {
				src_32--; dst_32--;
				BLT_FUNC(src_32, dst_32);
			}
		}
	} else {
		for (y = 0; y < h; y++) {
			uae_u8 *src_8;
			uae_u8 *dst_8;
			uae_u32 *src_32 = (uae_u32*)src2;
			uae_u32 *dst_32 = (uae_u32*)dst2;
			for (x = 0; x < ww; x++) {
				BLT_FUNC(src_32, dst_32);
				src_32++; dst_32++;
			}
			src_8 = (uae_u8 *)src_32;
			dst_8 = (uae_u8 *)dst_32;
			for (x = 0; x < xxd; x++) {
				uae_u32 sv = *src_8;
				uae_u32 dv = *dst_8;
				BLT_FUNC(&sv, &dv);
				*dst_8 = (uae_u8)dv;
				src_8++;
				dst_8++;
			}
			dst2 += dstpitch;
			src2 += srcpitch;
		}
	}
}
#else
static void NOINLINE BLT_NAME(unsigned int w, unsigned int h, uae_u8 *src, uae_u8 *dst, int srcpitch, int dstpitch, uae_u32 rgbmask)
{
	uae_u8 *src2 = src;
	uae_u8 *dst2 = dst;
	uae_u32 *src2_32 = (uae_u32*)src;
	uae_u32 *dst2_32 = (uae_u32*)dst;
	unsigned int y, x, ww, xxd;

	if (w < 8 * BLT_MULT) {
		ww = w / BLT_MULT;
		if (src2 < dst2 && src2 + h * srcpitch > dst2) {
			dst2 += h * dstpitch + w * BLT_SIZE;
			src2 += h * srcpitch + w * BLT_SIZE;
			for (y = 0; y < h; y++) {
				dst2 -= dstpitch;
				src2 -= srcpitch;
#if BLT_SIZE == 2
				if (w & 1) {
					dst2 -= 2;
					src2 -= 2;
					uae_u16 *src_16 = (uae_u16*)src2;
					uae_u16 *dst_16 = (uae_u16*)dst2;
					BLT_FUNC(src_16, dst_16);
				}
#elif BLT_SIZE == 1
				{
					int wb = w & 3;
					while (wb--) {
						src2--;
						dst2--;
						uae_u8 *src_8 = (uae_u8*)src2;
						uae_u8 *dst_8 = (uae_u8*)dst2;
						BLT_FUNC(src_8, dst_8);
					}
				}
#endif
				uae_u32 *src_32 = (uae_u32*)src2;
				uae_u32 *dst_32 = (uae_u32*)dst2;
				for (x = 0; x < ww; x++) {
					src_32--; dst_32--;
					BLT_FUNC(src_32, dst_32);
				}
			}
		} else {
			for (y = 0; y < h; y++) {
				uae_u32 *src_32 = (uae_u32*)src2;
				uae_u32 *dst_32 = (uae_u32*)dst2;
				for (x = 0; x < ww; x++) {
					BLT_FUNC(src_32, dst_32);
					src_32++; dst_32++;
				}
#if BLT_SIZE == 2
				if (w & 1) {
					uae_u16 *src_16 = (uae_u16*)src_32;
					uae_u16 *dst_16 = (uae_u16*)dst_32;
					BLT_FUNC(src_16, dst_16);
				}
#elif BLT_SIZE == 1
				{
					int wb = w & 3;
					uae_u8 *src_8 = (uae_u8*)src_32;
					uae_u8 *dst_8 = (uae_u8*)dst_32;
					while (wb--) {
						BLT_FUNC(src_8, dst_8);
						src_8++;
						dst_8++;
					}
				}
#endif
				dst2 += dstpitch;
				src2 += srcpitch;
			}
		}
		return;
	}

	ww = w / (8 * BLT_MULT);
	xxd = (w - ww * (8 * BLT_MULT)) / BLT_MULT;
	if (src2 < dst2 && src2 + h * srcpitch > dst2) {
		dst2 += h * dstpitch + w * BLT_SIZE;
		src2 += h * srcpitch + w * BLT_SIZE;
		for (y = 0; y < h; y++) {
			dst2 -= dstpitch;
			src2 -= srcpitch;
#if BLT_SIZE == 2
			if (w & 1) {
				src2 -= 2;
				dst2 -= 2;
				uae_u16 *src_16 = (uae_u16*)src2;
				uae_u16 *dst_16 = (uae_u16*)dst2;
				BLT_FUNC(src_16, dst_16);
			}
#elif BLT_SIZE == 1
			{
				int wb = w & 3;
				while (wb--) {
					src2--;
					dst2--;
					uae_u8 *src_8 = (uae_u8*)src2;
					uae_u8 *dst_8 = (uae_u8*)dst2;
					BLT_FUNC(src_8, dst_8);
				}
			}
#endif
			uae_u32 *src_32 = (uae_u32*)src2;
			uae_u32 *dst_32 = (uae_u32*)dst2;
			for (x = 0; x < xxd; x++) {
				src_32--; dst_32--;
				BLT_FUNC(src_32, dst_32);
			}
			for (x = 0; x < ww; x++) {
				src_32--; dst_32--;
				BLT_FUNC(src_32, dst_32);
				src_32--; dst_32--;
				BLT_FUNC(src_32, dst_32);
				src_32--; dst_32--;
				BLT_FUNC(src_32, dst_32);
				src_32--; dst_32--;
				BLT_FUNC(src_32, dst_32);
				src_32--; dst_32--;
				BLT_FUNC(src_32, dst_32);
				src_32--; dst_32--;
				BLT_FUNC(src_32, dst_32);
				src_32--; dst_32--;
				BLT_FUNC(src_32, dst_32);
				src_32--; dst_32--;
				BLT_FUNC(src_32, dst_32);
			}
		}
	} else {
		for (y = 0; y < h; y++) {
			uae_u32 *src_32 = (uae_u32*)src2;
			uae_u32 *dst_32 = (uae_u32*)dst2;
			for (x = 0; x < ww; x++) {
				BLT_FUNC(src_32, dst_32);
				src_32++; dst_32++;
				BLT_FUNC(src_32, dst_32);
				src_32++; dst_32++;
				BLT_FUNC(src_32, dst_32);
				src_32++; dst_32++;
				BLT_FUNC(src_32, dst_32);
				src_32++; dst_32++;
				BLT_FUNC(src_32, dst_32);
				src_32++; dst_32++;
				BLT_FUNC(src_32, dst_32);
				src_32++; dst_32++;
				BLT_FUNC(src_32, dst_32);
				src_32++; dst_32++;
				BLT_FUNC(src_32, dst_32);
				src_32++; dst_32++;
			}
			for (x = 0; x < xxd; x++) {
				BLT_FUNC(src_32, dst_32);
				src_32++; dst_32++;
			}
#if BLT_SIZE == 2
			if (w & 1) {
				uae_u16 *src_16 = (uae_u16*)src_32;
				uae_u16 *dst_16 = (uae_u16*)dst_32;
				BLT_FUNC(src_16, dst_16);
			}
#elif BLT_SIZE == 1
			{
				int wb = w & 3;
				uae_u8 *src_8 = (uae_u8*)src_32;
				uae_u8 *dst_8 = (uae_u8*)dst_32;
				while (wb--) {
					BLT_FUNC(src_8, dst_8);
					src_8++;
					dst_8++;
				}
			}
#endif
			dst2 += dstpitch;
			src2 += srcpitch;
		}
	}
}
#endif

#if BLT_SIZE == 1
static void NOINLINE BLT_NAME_MASK(unsigned int w, unsigned int h, uae_u8 *src, uae_u8 *dst, int srcpitch, int dstpitch, uae_u8 mask)
{
	uae_u8 *src2 = src;
	uae_u8 *dst2 = dst;
	unsigned int y, x;
	uae_u32 mask32 = mask * 0x01010101;

	if (src < dst && src + h * srcpitch > dst) {
		dst2 += h * dstpitch + w;
		src2 += h * srcpitch + w;
		for (y = 0; y < h; y++) {
			dst2 -= dstpitch;
			src2 -= srcpitch;
			uae_u32 *src_32 = (uae_u32*)src2;
			uae_u32 *dst_32 = (uae_u32*)dst2;
			for (x = 0; x < (w & ~3); x += 4) {
				src_32--;
				dst_32--;
				BLT_FUNC_MASK(src_32, dst_32, mask32);
			}
			uae_u8 *src_8 = (uae_u8*)src_32;
			uae_u8 *dst_8 = (uae_u8*)dst_32;
			for (x = 0; x < (w & 3); x++) {
				src_8--;
				dst_8--;
				BLT_FUNC_MASK(src_8, dst_8, mask);
			}
		}
	} else {
		for (y = 0; y < h; y++) {
			uae_u32 *src_32 = (uae_u32*)src2;
			uae_u32 *dst_32 = (uae_u32*)dst2;
			for (x = 0; x < (w & ~3); x += 4) {
				BLT_FUNC_MASK(src_32, dst_32, mask32);
				src_32++;
				dst_32++;
			}
			uae_u8 *src_8 = (uae_u8*)src_32;
			uae_u8 *dst_8 = (uae_u8*)dst_32;
			for (x = 0; x < (w & 3); x++) {
				BLT_FUNC_MASK(src_8, dst_8, mask);
				src_8++;
				dst_8++;
			}
			dst2 += dstpitch;
			src2 += srcpitch;
		}
	}
}
#endif

#undef BLT_NAME
#undef BLT_NAME_MASK
#undef BLT_FUNC
#undef BLT_FUNC_MASK


