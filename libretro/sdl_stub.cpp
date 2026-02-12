#ifdef LIBRETRO
#include "sdl_compat.h"

#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#ifndef _WIN32
#include <strings.h>
#endif

struct SDL_Thread {
	pthread_t thread;
	int (*fn)(void*);
	void* data;
	int status;
};

struct SDL_mutex {
	pthread_mutex_t mutex;
};

struct SDL_cond {
	pthread_cond_t cond;
};

struct SDL_semaphore {
	sem_t sem;
};

static const char* sdl_last_error = "SDL stub";

static void sdl_set_error(const char* msg)
{
	sdl_last_error = msg ? msg : "SDL stub";
}

static void sdl_fill_format(SDL_PixelFormat* fmt, Uint32 format)
{
	if (!fmt)
		return;
	memset(fmt, 0, sizeof(*fmt));
	fmt->format = format;
	fmt->BitsPerPixel = 32;
	fmt->BytesPerPixel = 4;
	if (format == SDL_PIXELFORMAT_RGB565) {
		fmt->BitsPerPixel = 16;
		fmt->BytesPerPixel = 2;
		fmt->Rmask = 0xF800;
		fmt->Gmask = 0x07E0;
		fmt->Bmask = 0x001F;
		fmt->Rshift = 11;
		fmt->Gshift = 5;
		fmt->Bshift = 0;
	} else if (format == SDL_PIXELFORMAT_RGB555) {
		fmt->BitsPerPixel = 16;
		fmt->BytesPerPixel = 2;
		fmt->Rmask = 0x7C00;
		fmt->Gmask = 0x03E0;
		fmt->Bmask = 0x001F;
		fmt->Rshift = 10;
		fmt->Gshift = 5;
		fmt->Bshift = 0;
	} else if (format == SDL_PIXELFORMAT_ABGR8888) {
		fmt->Rmask = 0x000000ff;
		fmt->Gmask = 0x0000ff00;
		fmt->Bmask = 0x00ff0000;
		fmt->Amask = 0xff000000;
		fmt->Rshift = 0;
		fmt->Gshift = 8;
		fmt->Bshift = 16;
		fmt->Ashift = 24;
	} else if (format == SDL_PIXELFORMAT_RGBA8888) {
		fmt->Rmask = 0xff000000;
		fmt->Gmask = 0x00ff0000;
		fmt->Bmask = 0x0000ff00;
		fmt->Amask = 0x000000ff;
		fmt->Rshift = 24;
		fmt->Gshift = 16;
		fmt->Bshift = 8;
		fmt->Ashift = 0;
	} else if (format == SDL_PIXELFORMAT_BGRA8888) {
		fmt->Rmask = 0x0000ff00;
		fmt->Gmask = 0x00ff0000;
		fmt->Bmask = 0xff000000;
		fmt->Amask = 0x000000ff;
		fmt->Rshift = 8;
		fmt->Gshift = 16;
		fmt->Bshift = 24;
		fmt->Ashift = 0;
	} else {
		// Default to ARGB8888
		fmt->Rmask = 0x00ff0000;
		fmt->Gmask = 0x0000ff00;
		fmt->Bmask = 0x000000ff;
		fmt->Amask = 0xff000000;
		fmt->Rshift = 16;
		fmt->Gshift = 8;
		fmt->Bshift = 0;
		fmt->Ashift = 24;
	}
	fmt->refcount = 1;
}

static void* sdl_thread_start(void* arg)
{
	SDL_Thread* t = static_cast<SDL_Thread*>(arg);
	if (t && t->fn) {
		t->status = t->fn(t->data);
	}
	return nullptr;
}

extern "C" {

const char* SDL_GetError(void) { return sdl_last_error; }
void SDL_Log(const char* fmt, ...) { va_list ap; va_start(ap, fmt); vfprintf(stderr, fmt, ap); fputc('\n', stderr); va_end(ap); }

int SDL_atoi(const char* s) { return s ? atoi(s) : 0; }
double SDL_atof(const char* s) { return s ? atof(s) : 0.0; }
size_t SDL_strlen(const char* s) { return s ? strlen(s) : 0; }
int SDL_strcmp(const char* a, const char* b) { return strcmp(a ? a : "", b ? b : ""); }
int SDL_strncmp(const char* a, const char* b, size_t n) { return strncmp(a ? a : "", b ? b : "", n); }
#ifdef _WIN32
int SDL_strcasecmp(const char* a, const char* b) { return _stricmp(a ? a : "", b ? b : ""); }
int SDL_strncasecmp(const char* a, const char* b, size_t n) { return _strnicmp(a ? a : "", b ? b : "", n); }
#else
int SDL_strcasecmp(const char* a, const char* b) { return strcasecmp(a ? a : "", b ? b : ""); }
int SDL_strncasecmp(const char* a, const char* b, size_t n) { return strncasecmp(a ? a : "", b ? b : "", n); }
#endif
char* SDL_strchr(const char* s, int c) { return const_cast<char*>(strchr(s ? s : "", c)); }
char* SDL_strrchr(const char* s, int c) { return const_cast<char*>(strrchr(s ? s : "", c)); }
char* SDL_strstr(const char* s, const char* f) { return const_cast<char*>(strstr(s ? s : "", f ? f : "")); }
char* SDL_strdup(const char* s) { return s ? strdup(s) : nullptr; }
double SDL_strtod(const char* s, char** e) { return s ? strtod(s, e) : 0.0; }
long SDL_strtol(const char* s, char** e, int base) { return s ? strtol(s, e, base) : 0; }
Sint64 SDL_strtoll(const char* s, char** e, int base) { return s ? (Sint64)strtoll(s, e, base) : 0; }
unsigned long SDL_strtoul(const char* s, char** e, int base) { return s ? strtoul(s, e, base) : 0; }
int SDL_snprintf(char* s, size_t n, const char* fmt, ...) { va_list ap; va_start(ap, fmt); int r = vsnprintf(s, n, fmt, ap); va_end(ap); return r; }
int SDL_vsnprintf(char* s, size_t n, const char* fmt, va_list ap) { return vsnprintf(s, n, fmt, ap); }
int SDL_tolower(int c) { return tolower(c); }
int SDL_toupper(int c) { return toupper(c); }
int SDL_isdigit(int c) { return isdigit(c); }
int SDL_isspace(int c) { return isspace(c); }
void* SDL_memset(void* dst, int c, size_t n) { return memset(dst, c, n); }
void SDL_free(void* p) { free(p); }

#ifdef _WIN32
Uint64 SDL_GetPerformanceCounter(void)
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return (Uint64)counter.QuadPart;
}

Uint64 SDL_GetPerformanceFrequency(void)
{
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	return (Uint64)freq.QuadPart;
}

Uint32 SDL_GetTicks(void)
{
	return (Uint32)GetTickCount();
}

void SDL_Delay(Uint32 ms) { Sleep(ms); }
#else
Uint64 SDL_GetPerformanceCounter(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (Uint64)ts.tv_sec * 1000000000ull + (Uint64)ts.tv_nsec;
}

Uint64 SDL_GetPerformanceFrequency(void) { return 1000000000ull; }
Uint32 SDL_GetTicks(void) { return (Uint32)(SDL_GetPerformanceCounter() / 1000000ull); }
void SDL_Delay(Uint32 ms) { usleep((useconds_t)ms * 1000); }
#endif

void SDL_GetVersion(SDL_version* v)
{
	if (!v) return;
	v->major = 2;
	v->minor = 0;
	v->patch = 0;
}

const char* SDL_GetCurrentVideoDriver(void) { return "libretro"; }

int SDL_GetNumVideoDisplays(void) { return 1; }
int SDL_GetDisplayBounds(int displayIndex, SDL_Rect* rect)
{
	(void)displayIndex;
	if (rect) { rect->x = 0; rect->y = 0; rect->w = 640; rect->h = 480; }
	return 0;
}
int SDL_GetDesktopDisplayMode(int displayIndex, SDL_DisplayMode* mode)
{
	(void)displayIndex;
	if (mode) { mode->w = 640; mode->h = 480; mode->refresh_rate = 60; mode->format = SDL_PIXELFORMAT_ARGB8888; }
	return 0;
}
int SDL_GetCurrentDisplayMode(int displayIndex, SDL_DisplayMode* mode)
{
	return SDL_GetDesktopDisplayMode(displayIndex, mode);
}
int SDL_GetDisplayMode(int displayIndex, int modeIndex, SDL_DisplayMode* mode)
{
	(void)modeIndex;
	return SDL_GetDesktopDisplayMode(displayIndex, mode);
}
int SDL_GetDisplayDPI(int displayIndex, float* ddpi, float* hdpi, float* vdpi)
{
	(void)displayIndex;
	if (ddpi) *ddpi = 96.0f;
	if (hdpi) *hdpi = 96.0f;
	if (vdpi) *vdpi = 96.0f;
	return 0;
}

const char* SDL_GetDisplayName(int displayIndex)
{
	(void)displayIndex;
	return "libretro";
}

int SDL_GetNumDisplayModes(int displayIndex)
{
	(void)displayIndex;
	return 1;
}

int SDL_GetWindowDisplayMode(SDL_Window* window, SDL_DisplayMode* mode)
{
	(void)window;
	return SDL_GetDesktopDisplayMode(0, mode);
}

int SDL_PointInRect(const SDL_Point* p, const SDL_Rect* r)
{
	if (!p || !r)
		return SDL_FALSE;
	return (p->x >= r->x && p->x < (r->x + r->w) && p->y >= r->y && p->y < (r->y + r->h)) ? SDL_TRUE : SDL_FALSE;
}

int SDL_IntersectRect(const SDL_Rect* a, const SDL_Rect* b, SDL_Rect* result)
{
	if (!a || !b || !result)
		return SDL_FALSE;
	const int x1 = (a->x > b->x) ? a->x : b->x;
	const int y1 = (a->y > b->y) ? a->y : b->y;
	const int x2 = ((a->x + a->w) < (b->x + b->w)) ? (a->x + a->w) : (b->x + b->w);
	const int y2 = ((a->y + a->h) < (b->y + b->h)) ? (a->y + a->h) : (b->y + b->h);
	if (x2 <= x1 || y2 <= y1)
		return SDL_FALSE;
	result->x = x1;
	result->y = y1;
	result->w = x2 - x1;
	result->h = y2 - y1;
	return SDL_TRUE;
}

SDL_mutex* SDL_CreateMutex(void)
{
	SDL_mutex* m = (SDL_mutex*)calloc(1, sizeof(SDL_mutex));
	if (!m) return nullptr;
	pthread_mutex_init(&m->mutex, nullptr);
	return m;
}
void SDL_DestroyMutex(SDL_mutex* m)
{
	if (!m) return;
	pthread_mutex_destroy(&m->mutex);
	free(m);
}
int SDL_LockMutex(SDL_mutex* m) { return m ? pthread_mutex_lock(&m->mutex) : -1; }
int SDL_UnlockMutex(SDL_mutex* m) { return m ? pthread_mutex_unlock(&m->mutex) : -1; }
int SDL_TryLockMutex(SDL_mutex* m) { return m ? pthread_mutex_trylock(&m->mutex) : -1; }

SDL_cond* SDL_CreateCond(void)
{
	SDL_cond* c = (SDL_cond*)calloc(1, sizeof(SDL_cond));
	if (!c) return nullptr;
	pthread_cond_init(&c->cond, nullptr);
	return c;
}
int SDL_CondSignal(SDL_cond* c) { return c ? pthread_cond_signal(&c->cond) : -1; }
int SDL_CondWaitTimeout(SDL_cond* c, SDL_mutex* m, Uint32 ms)
{
	if (!c || !m) return -1;
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += ms / 1000;
	ts.tv_nsec += (long)(ms % 1000) * 1000000L;
	if (ts.tv_nsec >= 1000000000L) { ts.tv_sec++; ts.tv_nsec -= 1000000000L; }
	int ret = pthread_cond_timedwait(&c->cond, &m->mutex, &ts);
	return ret == 0 ? 0 : -1;
}

SDL_sem* SDL_CreateSemaphore(Uint32 initial_value)
{
	SDL_sem* s = (SDL_sem*)calloc(1, sizeof(SDL_sem));
	if (!s) return nullptr;
	sem_init(&s->sem, 0, initial_value);
	return s;
}
void SDL_DestroySemaphore(SDL_sem* s)
{
	if (!s) return;
	sem_destroy(&s->sem);
	free(s);
}
int SDL_SemPost(SDL_sem* s) { return s ? sem_post(&s->sem) : -1; }
int SDL_SemWait(SDL_sem* s) { return s ? sem_wait(&s->sem) : -1; }
int SDL_SemTryWait(SDL_sem* s) { return s ? sem_trywait(&s->sem) : -1; }
int SDL_SemWaitTimeout(SDL_sem* s, Uint32 ms)
{
	if (!s) return -1;
#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += ms / 1000;
	ts.tv_nsec += (long)(ms % 1000) * 1000000L;
	if (ts.tv_nsec >= 1000000000L) { ts.tv_sec++; ts.tv_nsec -= 1000000000L; }
	return sem_timedwait(&s->sem, &ts);
#else
#ifdef _WIN32
	Sleep(ms);
#else
	usleep((useconds_t)ms * 1000);
#endif
	return sem_trywait(&s->sem);
#endif
}

SDL_Thread* SDL_CreateThread(SDL_ThreadFunction fn, const char* name, void* data)
{
	(void)name;
	SDL_Thread* t = (SDL_Thread*)calloc(1, sizeof(SDL_Thread));
	if (!t) return nullptr;
	t->fn = fn;
	t->data = data;
	if (pthread_create(&t->thread, nullptr, sdl_thread_start, t) != 0) {
		free(t);
		sdl_set_error("pthread_create failed");
		return nullptr;
	}
	return t;
}
void SDL_DetachThread(SDL_Thread* t) { if (t) pthread_detach(t->thread); }
void SDL_WaitThread(SDL_Thread* t, int* status)
{
	if (!t) return;
	pthread_join(t->thread, nullptr);
	if (status) *status = t->status;
	free(t);
}
SDL_threadID SDL_GetThreadID(SDL_Thread* t) { return t ? (SDL_threadID)t->thread : (SDL_threadID)pthread_self(); }
int SDL_SetThreadPriority(SDL_ThreadPriority pri) { (void)pri; return 0; }

SDL_Surface* SDL_CreateRGBSurfaceWithFormat(Uint32 flags, int w, int h, int depth, Uint32 format)
{
	(void)flags;
	(void)depth;
	if (w <= 0 || h <= 0) return nullptr;
	SDL_Surface* s = (SDL_Surface*)calloc(1, sizeof(SDL_Surface));
	if (!s) return nullptr;
	s->format = SDL_AllocFormat(format);
	if (!s->format) { free(s); return nullptr; }
	s->w = w;
	s->h = h;
	s->pitch = w * s->format->BytesPerPixel;
	s->pixels = calloc(1, (size_t)s->pitch * (size_t)h);
	if (!s->pixels) {
		SDL_FreeFormat(s->format);
		free(s);
		return nullptr;
	}
	return s;
}

SDL_Surface* SDL_CreateRGBSurfaceWithFormatFrom(void* pixels, int w, int h, int depth, int pitch, Uint32 format)
{
	SDL_Surface* s = (SDL_Surface*)calloc(1, sizeof(SDL_Surface));
	if (!s) return nullptr;
	s->format = SDL_AllocFormat(format);
	if (!s->format) { free(s); return nullptr; }
	s->flags |= SDL_PREALLOC;
	s->w = w;
	s->h = h;
	s->pitch = pitch;
	s->pixels = pixels;
	if (depth > 0) {
		s->format->BitsPerPixel = (Uint8)depth;
		s->format->BytesPerPixel = (Uint8)(depth / 8);
	}
	return s;
}

SDL_Surface* SDL_CreateRGBSurface(Uint32 flags, int w, int h, int depth,
	Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
	(void)Rmask; (void)Gmask; (void)Bmask; (void)Amask;
	Uint32 fmt = (depth == 32) ? SDL_PIXELFORMAT_ARGB8888 : SDL_PIXELFORMAT_ARGB8888;
	return SDL_CreateRGBSurfaceWithFormat(flags, w, h, depth, fmt);
}

SDL_Surface* SDL_CreateRGBSurfaceFrom(void* pixels, int w, int h, int depth,
	int pitch, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
	(void)Rmask; (void)Gmask; (void)Bmask; (void)Amask;
	SDL_Surface* s = (SDL_Surface*)calloc(1, sizeof(SDL_Surface));
	if (!s) return nullptr;
	s->format = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);
	if (!s->format) { free(s); return nullptr; }
	s->flags |= SDL_PREALLOC;
	s->w = w;
	s->h = h;
	s->pitch = pitch;
	s->pixels = pixels;
	s->format->BitsPerPixel = (Uint8)depth;
	s->format->BytesPerPixel = (Uint8)(depth / 8);
	return s;
}

void SDL_FreeSurface(SDL_Surface* s)
{
	if (!s) return;
	if (!(s->flags & SDL_PREALLOC) && s->pixels)
		free(s->pixels);
	if (s->format)
		SDL_FreeFormat(s->format);
	free(s);
}

SDL_PixelFormat* SDL_AllocFormat(Uint32 format)
{
	SDL_PixelFormat* fmt = (SDL_PixelFormat*)calloc(1, sizeof(SDL_PixelFormat));
	if (!fmt) return nullptr;
	sdl_fill_format(fmt, format);
	return fmt;
}

void SDL_FreeFormat(SDL_PixelFormat* fmt) { free(fmt); }

int SDL_UpperBlit(SDL_Surface* src, const SDL_Rect* sr, SDL_Surface* dst, SDL_Rect* dr)
{
	(void)src; (void)sr; (void)dst; (void)dr;
	return 0;
}

int SDL_FillRect(SDL_Surface* dst, const SDL_Rect* rect, Uint32 color)
{
	(void)dst; (void)rect; (void)color;
	return 0;
}

SDL_Texture* SDL_CreateTextureFromSurface(SDL_Renderer* renderer, SDL_Surface* surface)
{
	(void)renderer; (void)surface;
	return nullptr;
}
SDL_Texture* SDL_CreateTexture(SDL_Renderer* renderer, Uint32 format, int access, int w, int h)
{
	(void)renderer; (void)format; (void)access; (void)w; (void)h;
	return nullptr;
}
void SDL_DestroyTexture(SDL_Texture* texture) { (void)texture; }
int SDL_QueryTexture(SDL_Texture* texture, Uint32* format, int* access, int* w, int* h)
{
	(void)texture; if (format) *format = 0; if (access) *access = 0; if (w) *w = 0; if (h) *h = 0; return -1;
}
int SDL_UpdateTexture(SDL_Texture* texture, const SDL_Rect* rect, const void* pixels, int pitch)
{
	(void)texture; (void)rect; (void)pixels; (void)pitch; return 0;
}
int SDL_LockTexture(SDL_Texture* texture, const SDL_Rect* rect, void** pixels, int* pitch)
{
	(void)texture; (void)rect; if (pixels) *pixels = nullptr; if (pitch) *pitch = 0; return 0;
}
void SDL_UnlockTexture(SDL_Texture* texture) { (void)texture; }
int SDL_RenderCopy(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect* sr, const SDL_Rect* dr)
{
	(void)renderer; (void)texture; (void)sr; (void)dr; return 0;
}
int SDL_RenderCopyEx(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect* sr, const SDL_Rect* dr, const double angle, const SDL_Point* center, const SDL_RendererFlip flip)
{
	(void)renderer; (void)texture; (void)sr; (void)dr; (void)angle; (void)center; (void)flip; return 0;
}
int SDL_RenderFillRect(SDL_Renderer* renderer, const SDL_Rect* rect)
{
	(void)renderer; (void)rect; return 0;
}
int SDL_RenderSetLogicalSize(SDL_Renderer* renderer, int w, int h)
{
	(void)renderer; (void)w; (void)h; return 0;
}
int SDL_RenderSetIntegerScale(SDL_Renderer* renderer, SDL_bool enable)
{
	(void)renderer; (void)enable; return 0;
}
int SDL_RenderSetScale(SDL_Renderer* renderer, float scaleX, float scaleY)
{
	(void)renderer; (void)scaleX; (void)scaleY; return 0;
}
void SDL_RenderGetLogicalSize(SDL_Renderer* renderer, int* w, int* h)
{
	(void)renderer; if (w) *w = 0; if (h) *h = 0;
}
int SDL_GetRendererOutputSize(SDL_Renderer* renderer, int* w, int* h)
{
	(void)renderer; if (w) *w = 0; if (h) *h = 0; return 0;
}
void SDL_RenderPresent(SDL_Renderer* renderer) { (void)renderer; }
int SDL_RenderClear(SDL_Renderer* renderer) { (void)renderer; return 0; }
void SDL_DestroyRenderer(SDL_Renderer* renderer) { (void)renderer; }
int SDL_SetRenderDrawColor(SDL_Renderer* renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	(void)renderer; (void)r; (void)g; (void)b; (void)a; return 0;
}
int SDL_GetRenderDrawColor(SDL_Renderer* renderer, Uint8* r, Uint8* g, Uint8* b, Uint8* a)
{
	(void)renderer; if (r) *r = 0; if (g) *g = 0; if (b) *b = 0; if (a) *a = 0; return 0;
}
int SDL_SetRenderDrawBlendMode(SDL_Renderer* renderer, SDL_BlendMode blendMode)
{
	(void)renderer; (void)blendMode; return 0;
}
int SDL_SetSurfaceBlendMode(SDL_Surface* surface, SDL_BlendMode blendMode)
{
	(void)surface; (void)blendMode; return 0;
}
int SDL_SetTextureBlendMode(SDL_Texture* texture, SDL_BlendMode blendMode)
{
	(void)texture; (void)blendMode; return 0;
}
int SDL_SetTextureAlphaMod(SDL_Texture* texture, Uint8 alpha)
{
	(void)texture; (void)alpha; return 0;
}

SDL_Window* SDL_GetKeyboardFocus(void) { return nullptr; }
const Uint8* SDL_GetKeyboardState(int* numkeys)
{
	static Uint8 keys[512];
	if (numkeys) *numkeys = (int)(sizeof(keys) / sizeof(keys[0]));
	return keys;
}
SDL_Keycode SDL_GetKeyFromScancode(SDL_Scancode scancode) { return (SDL_Keycode)scancode; }
SDL_Scancode SDL_GetScancodeFromName(const char* name) { (void)name; return SDL_SCANCODE_UNKNOWN; }
SDL_Keymod SDL_GetModState(void) { return KMOD_NONE; }

SDL_Cursor* SDL_GetCursor(void) { return nullptr; }
int SDL_ShowCursor(int toggle) { (void)toggle; return 0; }
SDL_Cursor* SDL_CreateColorCursor(SDL_Surface* surface, int hot_x, int hot_y)
{
	(void)surface; (void)hot_x; (void)hot_y; return nullptr;
}
void SDL_FreeCursor(SDL_Cursor* cursor) { (void)cursor; }
void SDL_SetCursor(SDL_Cursor* cursor) { (void)cursor; }
int SDL_WarpMouseGlobal(int x, int y) { (void)x; (void)y; return 0; }
void SDL_WarpMouseInWindow(SDL_Window* window, int x, int y) { (void)window; (void)x; (void)y; }
int SDL_SetRelativeMouseMode(SDL_bool enabled) { (void)enabled; return 0; }

SDL_bool SDL_HasClipboardText(void) { return SDL_FALSE; }
char* SDL_GetClipboardText(void) { return SDL_strdup(""); }
int SDL_SetClipboardText(const char* text) { (void)text; return 0; }

SDL_Window* SDL_CreateWindow(const char* title, int x, int y, int w, int h, Uint32 flags)
{
	(void)title; (void)x; (void)y; (void)w; (void)h; (void)flags;
	return nullptr;
}
SDL_Renderer* SDL_CreateRenderer(SDL_Window* window, int index, Uint32 flags)
{
	(void)window; (void)index; (void)flags;
	return nullptr;
}
SDL_Window* SDL_GetWindowFromID(Uint32 id) { (void)id; return nullptr; }
Uint32 SDL_GetWindowID(SDL_Window* window) { (void)window; return 0; }
Uint32 SDL_GetWindowFlags(SDL_Window* window) { (void)window; return 0; }
void SDL_GetWindowPosition(SDL_Window* window, int* x, int* y) { (void)window; if (x) *x = 0; if (y) *y = 0; }
void SDL_GetWindowSize(SDL_Window* window, int* w, int* h) { (void)window; if (w) *w = 0; if (h) *h = 0; }
void SDL_SetWindowGrab(SDL_Window* window, SDL_bool grabbed) { (void)window; (void)grabbed; }
void SDL_SetWindowSize(SDL_Window* window, int w, int h) { (void)window; (void)w; (void)h; }
void SDL_SetWindowPosition(SDL_Window* window, int x, int y) { (void)window; (void)x; (void)y; }
void SDL_RaiseWindow(SDL_Window* window) { (void)window; }
void SDL_MinimizeWindow(SDL_Window* window) { (void)window; }
void SDL_DestroyWindow(SDL_Window* window) { (void)window; }
void SDL_SetWindowIcon(SDL_Window* window, SDL_Surface* icon) { (void)window; (void)icon; }

SDL_bool SDL_SetHint(const char* name, const char* value) { (void)name; (void)value; return SDL_TRUE; }
int SDL_GL_SetAttribute(SDL_GLattr attr, int value) { (void)attr; (void)value; return 0; }
void SDL_GL_ResetAttributes(void) {}

int SDL_PushEvent(SDL_Event* event) { (void)event; return -1; }
int SDL_PollEvent(SDL_Event* event) { (void)event; return 0; }
void SDL_PumpEvents(void) {}
int SDL_PeepEvents(SDL_Event* events, int numevents, int action, Uint32 minType, Uint32 maxType)
{
	(void)events; (void)numevents; (void)action; (void)minType; (void)maxType;
	return 0;
}

SDL_GameController* SDL_GameControllerOpen(int joystick_index) { (void)joystick_index; return nullptr; }
void SDL_GameControllerClose(SDL_GameController* gamecontroller) { (void)gamecontroller; }
SDL_Joystick* SDL_GameControllerGetJoystick(SDL_GameController* gamecontroller) { (void)gamecontroller; return nullptr; }
const char* SDL_GameControllerName(SDL_GameController* gamecontroller) { (void)gamecontroller; return nullptr; }
const char* SDL_GameControllerNameForIndex(int joystick_index) { (void)joystick_index; return nullptr; }
char* SDL_GameControllerMapping(SDL_GameController* gamecontroller) { (void)gamecontroller; return nullptr; }
const char* SDL_GameControllerGetStringForAxis(SDL_GameControllerAxis axis) { (void)axis; return nullptr; }
const char* SDL_GameControllerGetStringForButton(SDL_GameControllerButton button) { (void)button; return nullptr; }
Uint8 SDL_GameControllerGetButton(SDL_GameController* gamecontroller, SDL_GameControllerButton button)
{
	(void)gamecontroller; (void)button; return 0;
}
SDL_GameControllerButton SDL_GameControllerGetButtonFromString(const char* str)
{
	(void)str; return SDL_CONTROLLER_BUTTON_INVALID;
}
int SDL_GameControllerAddMappingsFromFile(const char* path) { (void)path; return 0; }
SDL_bool SDL_IsGameController(int joystick_index) { (void)joystick_index; return SDL_FALSE; }

SDL_Joystick* SDL_JoystickOpen(int device_index) { (void)device_index; return nullptr; }
void SDL_JoystickClose(SDL_Joystick* joystick) { (void)joystick; }
const char* SDL_JoystickName(SDL_Joystick* joystick) { (void)joystick; return nullptr; }
const char* SDL_JoystickNameForIndex(int device_index) { (void)device_index; return nullptr; }
SDL_JoystickID SDL_JoystickInstanceID(SDL_Joystick* joystick) { (void)joystick; return -1; }
SDL_JoystickGUID SDL_JoystickGetGUID(SDL_Joystick* joystick) { SDL_JoystickGUID g{}; (void)joystick; return g; }
void SDL_JoystickGetGUIDString(SDL_JoystickGUID guid, char* pszGUID, int cbGUID)
{
	(void)guid;
	if (pszGUID && cbGUID > 0) pszGUID[0] = '\0';
}
int SDL_JoystickNumAxes(SDL_Joystick* joystick) { (void)joystick; return 0; }
int SDL_JoystickNumBalls(SDL_Joystick* joystick) { (void)joystick; return 0; }
int SDL_JoystickNumButtons(SDL_Joystick* joystick) { (void)joystick; return 0; }
int SDL_JoystickNumHats(SDL_Joystick* joystick) { (void)joystick; return 0; }
Sint16 SDL_JoystickGetAxis(SDL_Joystick* joystick, int axis) { (void)joystick; (void)axis; return 0; }
Uint8 SDL_JoystickGetButton(SDL_Joystick* joystick, int button) { (void)joystick; (void)button; return 0; }
int SDL_NumJoysticks(void) { return 0; }

const char* SDL_GetAudioDeviceName(int index, int iscapture)
{
	(void)index; (void)iscapture; return nullptr;
}
SDL_AudioDeviceID SDL_OpenAudioDevice(const char* device, int iscapture,
	const SDL_AudioSpec* desired, SDL_AudioSpec* obtained, int allowed_changes)
{
	(void)device; (void)iscapture; (void)desired; (void)obtained; (void)allowed_changes;
	sdl_set_error("SDL audio not available");
	return 0;
}
void SDL_CloseAudioDevice(SDL_AudioDeviceID dev) { (void)dev; }
void SDL_PauseAudioDevice(SDL_AudioDeviceID dev, int pause_on) { (void)dev; (void)pause_on; }
int SDL_QueueAudio(SDL_AudioDeviceID dev, const void* data, Uint32 len)
{
	(void)dev; (void)data; (void)len;
	sdl_set_error("SDL audio not available");
	return -1;
}
Uint32 SDL_DequeueAudio(SDL_AudioDeviceID dev, void* data, Uint32 len)
{
	(void)dev; (void)data; (void)len; return 0;
}
Uint32 SDL_GetQueuedAudioSize(SDL_AudioDeviceID dev) { (void)dev; return 0; }
void SDL_LockAudioDevice(SDL_AudioDeviceID dev) { (void)dev; }
void SDL_UnlockAudioDevice(SDL_AudioDeviceID dev) { (void)dev; }

int SDL_GetNumAudioDevices(int iscapture)
{
	(void)iscapture;
	return 0;
}

int SDL_Init(Uint32 flags)
{
	(void)flags;
	return 0;
}

SDL_Cursor* SDL_CreateSystemCursor(int id) { (void)id; return nullptr; }

SDL_TimerID SDL_AddTimer(Uint32 interval, Uint32 (*callback)(Uint32, void*), void* param)
{
	(void)interval; (void)callback; (void)param;
	return 0;
}

SDL_bool SDL_RemoveTimer(SDL_TimerID id)
{
	(void)id;
	return SDL_TRUE;
}

SDL_AudioSpec* SDL_LoadWAV(const char* file, SDL_AudioSpec* spec, Uint8** audio_buf, Uint32* audio_len)
{
	(void)file; (void)spec; (void)audio_buf; (void)audio_len;
	return nullptr;
}

void SDL_FreeWAV(Uint8* audio_buf)
{
	(void)audio_buf;
}

void SDL_Quit(void) {}

}
#endif
