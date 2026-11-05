/*
 * Host clipboard access through SDL3, on the main thread.
 *
 * Text goes through SDL's text clipboard. Images are read as PNG, TIFF
 * (macOS) or BMP, whichever the host offers first, and written in all of
 * them. clipboard.cpp moves the content to and from the guest.
 */

#include "sysconfig.h"
#include "sysdeps.h"

#include <cstring>
#include <vector>

#include "options.h"
#include "clipboard.h"
#include "clipboard_host.h"
#include "clipboard_image.h"

namespace {

struct host_image_data {
	std::vector<uae_u8> png;
	std::vector<uae_u8> tiff;
	std::vector<uae_u8> bmp;
};

const void *SDLCALL provide_image(void *userdata, const char *mime_type, size_t *size)
{
	const auto *data = static_cast<const host_image_data *>(userdata);
	const std::vector<uae_u8> *payload = &data->bmp;
	if (mime_type && !strcmp(mime_type, "image/png") && !data->png.empty())
		payload = &data->png;
	else if (mime_type && (!strcmp(mime_type, "image/tiff") || !strcmp(mime_type, "public.tiff")) && !data->tiff.empty())
		payload = &data->tiff;
	*size = payload->size();
	return payload->data();
}

void SDLCALL free_image(void *userdata)
{
	delete static_cast<host_image_data *>(userdata);
}

bool read_clipboard_data(const char *mime_type, std::vector<uae_u8> *out)
{
	if (!SDL_HasClipboardData(mime_type))
		return false;
	size_t size = 0;
	void *data = SDL_GetClipboardData(mime_type, &size);
	if (!data)
		return false;
	const bool ok = size > 0 && size <= CLIP_SIZE_LIMIT;
	if (ok)
		out->assign(static_cast<const uae_u8 *>(data), static_cast<const uae_u8 *>(data) + size);
	SDL_free(data);
	return ok;
}

bool read_host_image(clipboard_bitmap *bitmap)
{
	std::vector<uae_u8> data;
	if (read_clipboard_data("image/png", &data) && clipboard_png_decode(data.data(), data.size(), bitmap))
		return true;
#ifdef AMIBERRY_MACOS
	for (const char *mime_type : { "image/tiff", "public.tiff" }) {
		if (read_clipboard_data(mime_type, &data) && clipboard_imageio_decode(data.data(), data.size(), bitmap))
			return true;
	}
#endif
	for (const char *mime_type : { "image/bmp", "image/x-bmp" }) {
		if (read_clipboard_data(mime_type, &data) && clipboard_bmp_decode(data.data(), data.size(), bitmap))
			return true;
	}
	return false;
}

void read_host_clipboard()
{
	clipboard_content content;
	if (SDL_HasClipboardText()) {
		char *text = SDL_GetClipboardText();
		if (!text)
			return;
		content.text = text;
		SDL_free(text);
		if (content.text.size() > CLIP_SIZE_LIMIT)
			return;
		content.type = clipboard_content::kind::text;
	} else if (read_host_image(&content.bitmap)) {
		content.type = clipboard_content::kind::bitmap;
	} else {
		return;
	}
	clipboard_set_host_content(std::move(content));
}

void write_host_image(const clipboard_bitmap &bitmap)
{
	auto *data = new host_image_data;
	if (!clipboard_bmp_encode(bitmap, &data->bmp)) {
		delete data;
		return;
	}
	std::vector<const char *> mime_types;
#ifdef _WIN32
	// SDL's Windows backend publishes only the first image type it is given:
	// BMP becomes CF_DIB, which every Windows program can paste.
	mime_types.push_back("image/bmp");
#else
	if (clipboard_png_encode(bitmap, &data->png))
		mime_types.push_back("image/png");
#ifdef AMIBERRY_MACOS
	if (clipboard_tiff_encode(bitmap, &data->tiff)) {
		mime_types.push_back("image/tiff");
		mime_types.push_back("public.tiff");
	}
#endif
	mime_types.push_back("image/bmp");
	mime_types.push_back("image/x-bmp");
#endif
	// SDL owns data from here on, also when it fails to publish it: it calls
	// free_image when the clipboard changes again.
	if (!SDL_SetClipboardData(provide_image, free_image, data, mime_types.data(), mime_types.size()))
		write_log(_T("clipboard: failed to write image to the host clipboard: %s\n"), SDL_GetError());
}

} // namespace

void clipboard_host_update_event(const SDL_ClipboardEvent &event)
{
	// owner: the update is our own write.
	if (event.owner || !currprefs.clipboard_sharing)
		return;
	read_host_clipboard();
}

void clipboard_host_flush()
{
	if (!currprefs.clipboard_sharing)
		return;
	if (clipboard_host_snapshot_wanted())
		read_host_clipboard();

	clipboard_content content;
	if (!clipboard_take_guest_content(&content))
		return;
	if (content.type == clipboard_content::kind::text) {
		if (!SDL_SetClipboardText(content.text.c_str()))
			write_log(_T("clipboard: failed to write text to the host clipboard: %s\n"), SDL_GetError());
	} else if (content.type == clipboard_content::kind::bitmap) {
		write_host_image(content.bitmap);
	}
}
