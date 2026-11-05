#pragma once

// Image file formats on the host clipboard, converted to and from the
// pixels the IFF ILBM codec in clipboard_common.cpp works on. Decoders reject
// images whose pixels would exceed CLIP_SIZE_LIMIT.

#include "uae/types.h"
#include "clipboard_common.h"

#include <cstddef>
#include <vector>

// BMP files as SDL's Windows backend produces them from CF_DIB/CF_DIBV5:
// 1, 4, 8, 16, 24 and 32 bits per pixel, BI_RGB and BI_BITFIELDS, any
// info header from BITMAPINFOHEADER to BITMAPV5HEADER. Alpha is ignored.
bool clipboard_bmp_decode(const uae_u8 *data, size_t size, clipboard_bitmap *bitmap);
// A 24-bit bottom-up BMP file.
bool clipboard_bmp_encode(const clipboard_bitmap &bitmap, std::vector<uae_u8> *bmp);

// PNG through libpng; transparent pixels are composited over white. Both
// return false when libpng lacks its simplified API.
bool clipboard_png_decode(const uae_u8 *data, size_t size, clipboard_bitmap *bitmap);
bool clipboard_png_encode(const clipboard_bitmap &bitmap, std::vector<uae_u8> *png);

#ifdef AMIBERRY_MACOS
// Any image ImageIO reads (macOS apps often put only TIFF on the pasteboard).
bool clipboard_imageio_decode(const uae_u8 *data, size_t size, clipboard_bitmap *bitmap);
bool clipboard_tiff_encode(const clipboard_bitmap &bitmap, std::vector<uae_u8> *tiff);
#endif
