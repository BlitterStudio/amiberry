#include "SDL_image.h"
#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

SDL_Surface* IMG_Load(const char* file)
{
	int width = 0;
	int height = 0;
	int channels = 0;
	unsigned char* data = stbi_load(file, &width, &height, &channels, 4);
	if (!data) {
		return nullptr;
	}

	SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
	if (!surface) {
		stbi_image_free(data);
		return nullptr;
	}

	memcpy(surface->pixels, data, static_cast<size_t>(width) * static_cast<size_t>(height) * 4u);
	stbi_image_free(data);
	return surface;
}
