#ifdef USE_SDL2
#include <SDL.h>

static SDL_Window* window = NULL;
static SDL_Texture* texture = NULL;
static SDL_Texture* prescaled = NULL;
static SDL_Renderer* renderer = NULL;
int prescale_factor_x = 1;
int prescale_factor_y = 1;
int prescaled_width = 320;
int prescaled_height = 240;
int surface_width = 320;
int surface_height = 240;

static int x_offset;
static int y_offset;
static int scaled_height;
static int scaled_width;
static int display_width;
static int display_height;
extern int mainMenu_shader;
extern int mainMenu_displayedLines;
extern int mainMenu_displayHires;
extern int visibleAreaWidth;
extern int displaying_menu;
//uint32_t *texture_pixels=NULL;

extern void update_joycon_mode(void);

#define MIN(a,b) ((a) < (b) ? (a) : (b))

SDL_Surface *SDL_SetVideoMode(int w, int h, int bpp, int flags) {
	if (!renderer) {
		window = SDL_CreateWindow("uae4all2", 0, 0, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
		SDL_RenderClear(renderer);
		SDL_DisplayMode DM;
		SDL_GetCurrentDisplayMode(0, &DM);
		display_width = DM.w;
		display_height = DM.h;
	}
	SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 16, SDL_PIXELFORMAT_RGB565);
	surface_width = w;
	surface_height = h;
	return surface;
}

void SDL_SetVideoModeScaling(int x, int y, float sw, float sh) {
	if (mainMenu_shader != 0 && !displaying_menu) {
		x_offset = (x * display_width) / 960;
		y_offset = (y * display_height) / 544;
		scaled_width = (sw * display_width) / 960;
		scaled_height = (sh * display_height) / 544;
	} else {
		int screen_width = 320;
		int screen_height = 240;
		if (!displaying_menu) {
			screen_width = visibleAreaWidth;
			if (mainMenu_displayHires)
				screen_height = 2 * mainMenu_displayedLines;			
			else
				screen_height = mainMenu_displayedLines;
		}
		int scale_factor = MIN(display_height / screen_height, display_width / screen_width);
		scaled_height = (float) (scale_factor * screen_height);
		scaled_width = (float) (scale_factor * screen_width);
		x_offset = (display_width - scaled_width) / 2;
		y_offset = (display_height - scaled_height) / 2;
	}
	if (prescaled) {
		SDL_DestroyTexture(prescaled);
		prescaled = NULL;
	}
	if (mainMenu_shader == 1) {
		prescale_factor_x = scaled_width / surface_width;
		prescale_factor_y = scaled_height / surface_height;
		prescaled_width = prescale_factor_x * surface_width;
		prescaled_height = prescale_factor_y * surface_height;
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
		prescaled = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, prescaled_width, prescaled_height);
	}
}

void SDL_SetVideoModeBilinear(int value) {
	if (value && (mainMenu_shader != 3)) // no linear filtering when shader = SHADER_POINT
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	else
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
}

void SDL_SetVideoModeSync(int value) {
	if (1)
		SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	else
		SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");
}

void SDL_Flip(SDL_Surface *surface) {
	update_joycon_mode();
	if (surface && renderer && window) {
		if (texture) {
			SDL_DestroyTexture(texture);
			texture = NULL;
		}
		if (mainMenu_shader == 1 && !displaying_menu) {
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
			texture = SDL_CreateTextureFromSurface(renderer, surface);
			
			SDL_SetRenderTarget(renderer, prescaled);
			SDL_Rect dst_rect_prescale = { 0, 0, prescaled_width, prescaled_height };
			SDL_Rect *src_rect_prescale = NULL;
			SDL_RenderCopy(renderer, texture, src_rect_prescale, &dst_rect_prescale);

			SDL_SetRenderTarget(renderer, NULL);
			SDL_Rect dst_rect = { x_offset, y_offset, scaled_width , scaled_height };
			SDL_Rect *src_rect = NULL;
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
			SDL_RenderCopy(renderer, prescaled, src_rect, &dst_rect);
			SDL_RenderPresent(renderer);
		} else {
			texture = SDL_CreateTextureFromSurface(renderer, surface);
			SDL_Rect dst_rect = { x_offset, y_offset, scaled_width, scaled_height };
			SDL_Rect *src_rect = NULL;
			SDL_RenderCopy(renderer, texture, src_rect, &dst_rect);
			SDL_RenderPresent(renderer);
		}
	}
}
#endif