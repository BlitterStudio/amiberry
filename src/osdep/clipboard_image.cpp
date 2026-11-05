/*
 * Host clipboard image formats (BMP, PNG, and TIFF on macOS) to and from
 * clipboard_bitmap pixels (top-down 0x00RRGGBB).
 *
 * The BMP and PNG code follows WinUAE's od-unix/clipboard.cpp. The BMP
 * decoder also reads BI_BITFIELDS and the larger info headers, because SDL's
 * Windows backend hands out CF_DIBV5 clipboard images as 32-bit BI_BITFIELDS
 * files with a BITMAPV5HEADER.
 */

#include "clipboard_image.h"

#include <climits>
#include <cstring>

#include "png.h"

#ifdef AMIBERRY_MACOS
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#endif

static constexpr uae_u32 BMP_RGB = 0;
static constexpr uae_u32 BMP_BITFIELDS = 3;
static constexpr uae_u32 BMP_ALPHABITFIELDS = 6;
static constexpr size_t BMP_FILE_HEADER = 14;
static constexpr size_t BMP_INFO_HEADER = 40;

static uae_u16 read_le16(const uae_u8 *p)
{
	return (uae_u16)(p[0] | (p[1] << 8));
}

static uae_u32 read_le32(const uae_u8 *p)
{
	return (uae_u32)p[0] | ((uae_u32)p[1] << 8) | ((uae_u32)p[2] << 16) | ((uae_u32)p[3] << 24);
}

static void append_le16(std::vector<uae_u8> *out, uae_u16 value)
{
	out->push_back((uae_u8)value);
	out->push_back((uae_u8)(value >> 8));
}

static void append_le32(std::vector<uae_u8> *out, uae_u32 value)
{
	for (int i = 0; i < 4; i++)
		out->push_back((uae_u8)(value >> (i * 8)));
}

static bool fits_limit(uae_u64 width, uae_u64 height)
{
	return width > 0 && height > 0 && width <= INT_MAX && height <= INT_MAX
		&& width * height * 4 <= CLIP_SIZE_LIMIT;
}

static bool valid_bitmap(const clipboard_bitmap &bitmap)
{
	return fits_limit(bitmap.width > 0 ? bitmap.width : 0, bitmap.height > 0 ? bitmap.height : 0)
		&& bitmap.pixels.size() >= (size_t)bitmap.width * (size_t)bitmap.height;
}

// One colour channel described by a BI_BITFIELDS mask, scaled to 8 bits.
struct bmp_channel {
	uae_u32 mask = 0;
	int shift = 0;
	uae_u32 max = 0;

	explicit bmp_channel(uae_u32 m) : mask(m)
	{
		if (!mask)
			return;
		while (!((mask >> shift) & 1))
			shift++;
		max = mask >> shift;
	}

	// Masks must be one contiguous run of bits.
	bool valid() const
	{
		return !mask || (max & (max + 1)) == 0;
	}

	uae_u32 get(uae_u32 value) const
	{
		if (!mask)
			return 0;
		const uae_u32 v = (value & mask) >> shift;
		return (uae_u32)(((uae_u64)v * 255 + max / 2) / max);
	}
};

bool clipboard_bmp_decode(const uae_u8 *data, size_t size, clipboard_bitmap *bitmap)
{
	if (!bitmap || !data || size < BMP_FILE_HEADER + BMP_INFO_HEADER || data[0] != 'B' || data[1] != 'M')
		return false;

	const uae_u32 pixel_offset = read_le32(data + 10);
	const uae_u32 header_size = read_le32(data + 14);
	if (header_size < BMP_INFO_HEADER || header_size > size - BMP_FILE_HEADER)
		return false;
	const int32_t width = (int32_t)read_le32(data + 18);
	const int32_t raw_height = (int32_t)read_le32(data + 22);
	const uae_u16 planes = read_le16(data + 26);
	const uae_u16 bpp = read_le16(data + 28);
	const uae_u32 compression = read_le32(data + 30);
	if (planes != 1 || width <= 0 || raw_height == 0 || raw_height == INT32_MIN)
		return false;
	const bool top_down = raw_height < 0;
	const int32_t height = top_down ? -raw_height : raw_height;
	if (!fits_limit((uae_u64)width, (uae_u64)height))
		return false;
	if (bpp != 1 && bpp != 4 && bpp != 8 && bpp != 16 && bpp != 24 && bpp != 32)
		return false;

	uae_u32 masks[3];
	if (compression == BMP_BITFIELDS || compression == BMP_ALPHABITFIELDS) {
		if (bpp != 16 && bpp != 32)
			return false;
		// After a BITMAPINFOHEADER the masks follow it; the larger headers
		// hold them at the same offset.
		const size_t mask_offset = BMP_FILE_HEADER + BMP_INFO_HEADER;
		if (size - mask_offset < 12)
			return false;
		for (int i = 0; i < 3; i++)
			masks[i] = read_le32(data + mask_offset + i * 4);
	} else if (compression == BMP_RGB) {
		if (bpp == 16) {
			masks[0] = 0x7c00;
			masks[1] = 0x03e0;
			masks[2] = 0x001f;
		} else {
			masks[0] = 0xff0000;
			masks[1] = 0x00ff00;
			masks[2] = 0x0000ff;
		}
	} else {
		return false;
	}
	const bmp_channel red(masks[0]), green(masks[1]), blue(masks[2]);
	if (!red.valid() || !green.valid() || !blue.valid())
		return false;

	std::vector<uae_u32> palette;
	if (bpp <= 8) {
		uae_u32 colors = read_le32(data + 46);
		if (!colors || colors > (1u << bpp))
			colors = 1u << bpp;
		const size_t palette_offset = BMP_FILE_HEADER + header_size;
		if (palette_offset > size || (size - palette_offset) / 4 < colors)
			return false;
		palette.resize(colors);
		for (uae_u32 i = 0; i < colors; i++) {
			const uae_u8 *entry = data + palette_offset + (size_t)i * 4;
			palette[i] = ((uae_u32)entry[2] << 16) | ((uae_u32)entry[1] << 8) | entry[0];
		}
	}

	const size_t row_bytes = ((size_t)width * bpp + 31) / 32 * 4;
	if (pixel_offset > size || row_bytes > (size - pixel_offset) / (size_t)height)
		return false;

	bitmap->width = width;
	bitmap->height = height;
	bitmap->pixels.assign((size_t)width * (size_t)height, 0);
	for (int32_t y = 0; y < height; y++) {
		const uae_u8 *row = data + pixel_offset + (size_t)(top_down ? y : height - 1 - y) * row_bytes;
		uae_u32 *out = bitmap->pixels.data() + (size_t)y * (size_t)width;
		for (int32_t x = 0; x < width; x++) {
			uae_u32 pixel;
			if (bpp <= 8) {
				const int per_byte = 8 / bpp;
				const int shift = (per_byte - 1 - x % per_byte) * bpp;
				const uae_u32 index = (row[x / per_byte] >> shift) & ((1u << bpp) - 1);
				pixel = index < palette.size() ? palette[index] : 0;
			} else if (bpp == 24) {
				const uae_u8 *p = row + (size_t)x * 3;
				pixel = ((uae_u32)p[2] << 16) | ((uae_u32)p[1] << 8) | p[0];
			} else {
				const uae_u32 value = bpp == 16 ? read_le16(row + (size_t)x * 2) : read_le32(row + (size_t)x * 4);
				pixel = (red.get(value) << 16) | (green.get(value) << 8) | blue.get(value);
			}
			out[x] = pixel;
		}
	}
	return true;
}

bool clipboard_bmp_encode(const clipboard_bitmap &bitmap, std::vector<uae_u8> *bmp)
{
	if (!bmp || !valid_bitmap(bitmap))
		return false;

	const int width = bitmap.width;
	const int height = bitmap.height;
	const size_t row_bytes = ((size_t)width * 3 + 3) & ~(size_t)3;
	const uae_u32 image_size = (uae_u32)(row_bytes * (size_t)height);
	const uae_u32 file_size = BMP_FILE_HEADER + BMP_INFO_HEADER + image_size;

	bmp->clear();
	bmp->reserve(file_size);
	bmp->push_back('B');
	bmp->push_back('M');
	append_le32(bmp, file_size);
	append_le16(bmp, 0);
	append_le16(bmp, 0);
	append_le32(bmp, BMP_FILE_HEADER + BMP_INFO_HEADER);
	append_le32(bmp, BMP_INFO_HEADER);
	append_le32(bmp, (uae_u32)width);
	append_le32(bmp, (uae_u32)height);
	append_le16(bmp, 1);
	append_le16(bmp, 24);
	append_le32(bmp, BMP_RGB);
	append_le32(bmp, image_size);
	append_le32(bmp, 0);
	append_le32(bmp, 0);
	append_le32(bmp, 0);
	append_le32(bmp, 0);

	std::vector<uae_u8> row(row_bytes);
	for (int y = height - 1; y >= 0; y--) {
		const uae_u32 *in = bitmap.pixels.data() + (size_t)y * (size_t)width;
		for (int x = 0; x < width; x++) {
			row[(size_t)x * 3 + 0] = (uae_u8)in[x];
			row[(size_t)x * 3 + 1] = (uae_u8)(in[x] >> 8);
			row[(size_t)x * 3 + 2] = (uae_u8)(in[x] >> 16);
		}
		bmp->insert(bmp->end(), row.begin(), row.end());
	}
	return true;
}

#if defined(PNG_SIMPLIFIED_READ_SUPPORTED) && defined(PNG_SIMPLIFIED_WRITE_SUPPORTED)

bool clipboard_png_decode(const uae_u8 *data, size_t size, clipboard_bitmap *bitmap)
{
	if (!bitmap || !data || !size)
		return false;

	png_image image;
	memset(&image, 0, sizeof image);
	image.version = PNG_IMAGE_VERSION;
	if (!png_image_begin_read_from_memory(&image, data, size))
		return false;
	if (!fits_limit(image.width, image.height)) {
		png_image_free(&image);
		return false;
	}

	image.format = PNG_FORMAT_RGBA;
	std::vector<uae_u8> rgba(PNG_IMAGE_SIZE(image));
	if (!png_image_finish_read(&image, nullptr, rgba.data(), 0, nullptr)) {
		png_image_free(&image);
		return false;
	}

	bitmap->width = (int)image.width;
	bitmap->height = (int)image.height;
	bitmap->pixels.resize((size_t)image.width * (size_t)image.height);
	for (size_t i = 0; i < bitmap->pixels.size(); i++) {
		const uae_u8 *p = rgba.data() + i * 4;
		const uae_u32 alpha = p[3];
		const uae_u32 r = (p[0] * alpha + 255 * (255 - alpha) + 127) / 255;
		const uae_u32 g = (p[1] * alpha + 255 * (255 - alpha) + 127) / 255;
		const uae_u32 b = (p[2] * alpha + 255 * (255 - alpha) + 127) / 255;
		bitmap->pixels[i] = (r << 16) | (g << 8) | b;
	}
	png_image_free(&image);
	return true;
}

bool clipboard_png_encode(const clipboard_bitmap &bitmap, std::vector<uae_u8> *png)
{
	if (!png || !valid_bitmap(bitmap))
		return false;

	const size_t count = (size_t)bitmap.width * (size_t)bitmap.height;
	std::vector<uae_u8> rgb(count * 3);
	for (size_t i = 0; i < count; i++) {
		rgb[i * 3 + 0] = (uae_u8)(bitmap.pixels[i] >> 16);
		rgb[i * 3 + 1] = (uae_u8)(bitmap.pixels[i] >> 8);
		rgb[i * 3 + 2] = (uae_u8)bitmap.pixels[i];
	}

	png_image image;
	memset(&image, 0, sizeof image);
	image.version = PNG_IMAGE_VERSION;
	image.width = (png_uint_32)bitmap.width;
	image.height = (png_uint_32)bitmap.height;
	image.format = PNG_FORMAT_RGB;

	png_alloc_size_t png_size = 0;
	if (!png_image_write_to_memory(&image, nullptr, &png_size, 0, rgb.data(), 0, nullptr) || !png_size) {
		png_image_free(&image);
		return false;
	}
	png->resize(png_size);
	if (!png_image_write_to_memory(&image, png->data(), &png_size, 0, rgb.data(), 0, nullptr)) {
		png_image_free(&image);
		png->clear();
		return false;
	}
	png->resize(png_size);
	png_image_free(&image);
	return true;
}

#else

bool clipboard_png_decode(const uae_u8 *, size_t, clipboard_bitmap *)
{
	return false;
}

bool clipboard_png_encode(const clipboard_bitmap &, std::vector<uae_u8> *)
{
	return false;
}

#endif

#ifdef AMIBERRY_MACOS

bool clipboard_imageio_decode(const uae_u8 *data, size_t size, clipboard_bitmap *bitmap)
{
	if (!bitmap || !data || !size)
		return false;

	CFDataRef image_data = CFDataCreate(kCFAllocatorDefault, data, (CFIndex)size);
	if (!image_data)
		return false;
	CGImageSourceRef source = CGImageSourceCreateWithData(image_data, nullptr);
	CFRelease(image_data);
	if (!source)
		return false;
	CGImageRef image = CGImageSourceCreateImageAtIndex(source, 0, nullptr);
	CFRelease(source);
	if (!image)
		return false;

	const size_t width = CGImageGetWidth(image);
	const size_t height = CGImageGetHeight(image);
	if (!fits_limit(width, height)) {
		CGImageRelease(image);
		return false;
	}

	std::vector<uae_u8> rgba(width * height * 4);
	CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();
	CGContextRef context = CGBitmapContextCreate(rgba.data(), width, height, 8, width * 4,
		colorspace, kCGBitmapByteOrder32Big | kCGImageAlphaPremultipliedLast);
	CGColorSpaceRelease(colorspace);
	if (!context) {
		CGImageRelease(image);
		return false;
	}
	CGContextDrawImage(context, CGRectMake(0, 0, (CGFloat)width, (CGFloat)height), image);
	CGContextRelease(context);
	CGImageRelease(image);

	// The bitmap context's first row is the top of the image. Premultiplied
	// alpha: adding the missing white composites over white.
	bitmap->width = (int)width;
	bitmap->height = (int)height;
	bitmap->pixels.resize(width * height);
	for (size_t i = 0; i < bitmap->pixels.size(); i++) {
		const uae_u8 *p = rgba.data() + i * 4;
		const uae_u32 fill = 255 - p[3];
		const uae_u32 r = p[0] + fill, g = p[1] + fill, b = p[2] + fill;
		bitmap->pixels[i] = ((r > 255 ? 255 : r) << 16) | ((g > 255 ? 255 : g) << 8) | (b > 255 ? 255 : b);
	}
	return true;
}

bool clipboard_tiff_encode(const clipboard_bitmap &bitmap, std::vector<uae_u8> *tiff)
{
	if (!tiff || !valid_bitmap(bitmap))
		return false;

	const size_t width = (size_t)bitmap.width;
	const size_t height = (size_t)bitmap.height;
	std::vector<uae_u8> rgba(width * height * 4);
	for (size_t i = 0; i < width * height; i++) {
		rgba[i * 4 + 0] = (uae_u8)(bitmap.pixels[i] >> 16);
		rgba[i * 4 + 1] = (uae_u8)(bitmap.pixels[i] >> 8);
		rgba[i * 4 + 2] = (uae_u8)bitmap.pixels[i];
		rgba[i * 4 + 3] = 255;
	}

	CGDataProviderRef provider = CGDataProviderCreateWithData(nullptr, rgba.data(), rgba.size(), nullptr);
	if (!provider)
		return false;
	CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();
	CGImageRef image = CGImageCreate(width, height, 8, 32, width * 4, colorspace,
		kCGBitmapByteOrder32Big | kCGImageAlphaNoneSkipLast, provider, nullptr, false,
		kCGRenderingIntentDefault);
	CGColorSpaceRelease(colorspace);
	CGDataProviderRelease(provider);
	if (!image)
		return false;

	CFMutableDataRef image_data = CFDataCreateMutable(kCFAllocatorDefault, 0);
	CGImageDestinationRef destination = image_data
		? CGImageDestinationCreateWithData(image_data, CFSTR("public.tiff"), 1, nullptr) : nullptr;
	bool ok = false;
	if (destination) {
		CGImageDestinationAddImage(destination, image, nullptr);
		ok = CGImageDestinationFinalize(destination);
		CFRelease(destination);
	}
	CGImageRelease(image);
	if (ok) {
		const UInt8 *bytes = CFDataGetBytePtr(image_data);
		tiff->assign(bytes, bytes + CFDataGetLength(image_data));
	}
	if (image_data)
		CFRelease(image_data);
	return ok && !tiff->empty();
}

#endif
