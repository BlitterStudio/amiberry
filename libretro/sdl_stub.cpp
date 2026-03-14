#ifdef LIBRETRO
#include "sdl_compat.h"

#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <strings.h>
#include <unistd.h>
#endif

struct SDL_Thread {
	pthread_t thread;
	int (*fn)(void*);
	void* data;
	int status;
};

struct SDL_Mutex {
	pthread_mutex_t mutex;
};

struct SDL_Condition {
	pthread_cond_t cond;
};

struct SDL_Semaphore {
	sem_t sem;
};

struct SDL_Window {
	SDL_WindowID id;
	int w;
	int h;
	SDL_WindowFlags flags;
};

struct SDL_Renderer {
	SDL_Window* window;
	int logical_w;
	int logical_h;
};

struct SDL_Texture {
	Uint32 format;
	int access;
	int w;
	int h;
	Uint8 alpha;
	Uint8 cr;
	Uint8 cg;
	Uint8 cb;
};

struct SDL_Cursor {
	int dummy;
};

struct SDL_Gamepad {
	int index;
};

struct SDL_Joystick {
	int index;
};

struct SDL_AudioStream {
	SDL_AudioDeviceID devid;
	SDL_AudioSpec spec;
	SDL_AudioStreamCallback callback;
	void* userdata;
	int queued;
	float gain;
	bool paused;
};

static const char* sdl_last_error = "SDL stub";
static SDL_Window g_window{};
static SDL_Renderer g_renderer{};
static Uint32 g_next_window_id = 1;

static void sdl_set_error(const char* msg)
{
	sdl_last_error = msg ? msg : "SDL stub";
}

static void* sdl_thread_start(void* arg)
{
	SDL_Thread* t = static_cast<SDL_Thread*>(arg);
	if (t && t->fn) {
		t->status = t->fn(t->data);
	}
	return nullptr;
}

static const SDL_PixelFormatDetails* sdl_format_details(SDL_PixelFormat format)
{
	static SDL_PixelFormatDetails d;
	SDL_zero(d);
	d.format = format;
	d.bits_per_pixel = 32;
	d.bytes_per_pixel = 4;
	if (format == SDL_PIXELFORMAT_RGB565) {
		d.bits_per_pixel = 16;
		d.bytes_per_pixel = 2;
		d.Rmask = 0xF800;
		d.Gmask = 0x07E0;
		d.Bmask = 0x001F;
		d.Rbits = 5;
		d.Gbits = 6;
		d.Bbits = 5;
		d.Rshift = 11;
		d.Gshift = 5;
		d.Bshift = 0;
	} else if (format == SDL_PIXELFORMAT_RGB555 || format == SDL_PIXELFORMAT_XRGB1555) {
		d.bits_per_pixel = 16;
		d.bytes_per_pixel = 2;
		d.Rmask = 0x7C00;
		d.Gmask = 0x03E0;
		d.Bmask = 0x001F;
		d.Rbits = 5;
		d.Gbits = 5;
		d.Bbits = 5;
		d.Rshift = 10;
		d.Gshift = 5;
		d.Bshift = 0;
	} else if (format == SDL_PIXELFORMAT_ABGR8888) {
		d.Rmask = 0x000000FF;
		d.Gmask = 0x0000FF00;
		d.Bmask = 0x00FF0000;
		d.Amask = 0xFF000000;
		d.Rbits = d.Gbits = d.Bbits = d.Abits = 8;
		d.Rshift = 0;
		d.Gshift = 8;
		d.Bshift = 16;
		d.Ashift = 24;
	} else if (format == SDL_PIXELFORMAT_RGBA8888) {
		d.Rmask = 0xFF000000;
		d.Gmask = 0x00FF0000;
		d.Bmask = 0x0000FF00;
		d.Amask = 0x000000FF;
		d.Rbits = d.Gbits = d.Bbits = d.Abits = 8;
		d.Rshift = 24;
		d.Gshift = 16;
		d.Bshift = 8;
		d.Ashift = 0;
	} else if (format == SDL_PIXELFORMAT_BGRA8888) {
		d.Rmask = 0x0000FF00;
		d.Gmask = 0x00FF0000;
		d.Bmask = 0xFF000000;
		d.Amask = 0x000000FF;
		d.Rbits = d.Gbits = d.Bbits = d.Abits = 8;
		d.Rshift = 8;
		d.Gshift = 16;
		d.Bshift = 24;
		d.Ashift = 0;
	} else {
		d.Rmask = 0x00FF0000;
		d.Gmask = 0x0000FF00;
		d.Bmask = 0x000000FF;
		d.Amask = 0xFF000000;
		d.Rbits = d.Gbits = d.Bbits = d.Abits = 8;
		d.Rshift = 16;
		d.Gshift = 8;
		d.Bshift = 0;
		d.Ashift = 24;
	}
	return &d;
}

extern "C" {

const char* SDL_GetError(void) { return sdl_last_error; }

void SDL_Log(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
	va_end(ap);
}

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
size_t SDL_strlcpy(char* dst, const char* src, size_t dstsize)
{
	const char* s = src ? src : "";
	size_t slen = strlen(s);
	if (dst && dstsize > 0) {
		size_t n = (slen >= dstsize) ? dstsize - 1 : slen;
		memcpy(dst, s, n);
		dst[n] = '\0';
	}
	return slen;
}
size_t SDL_strlcat(char* dst, const char* src, size_t dstsize)
{
	const char* s = src ? src : "";
	size_t dlen = 0;
	if (dst) {
		dlen = strnlen(dst, dstsize);
	}
	if (!dst || dlen >= dstsize) {
		return dlen + strlen(s);
	}
	return dlen + SDL_strlcpy(dst + dlen, s, dstsize - dlen);
}
int SDL_snprintf(char* s, size_t n, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int r = vsnprintf(s, n, fmt, ap);
	va_end(ap);
	return r;
}
int SDL_vsnprintf(char* s, size_t n, const char* fmt, va_list ap) { return vsnprintf(s, n, fmt, ap); }
int SDL_tolower(int c) { return tolower(c); }
int SDL_toupper(int c) { return toupper(c); }
int SDL_isdigit(int c) { return isdigit(c); }
int SDL_isspace(int c) { return isspace(c); }
void* SDL_memset(void* dst, int c, size_t n) { return memset(dst, c, n); }
void SDL_free(void* p) { free(p); }
char* SDL_getenv(const char* name) { return getenv(name); }

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

Uint64 SDL_GetTicks(void) { return (Uint64)GetTickCount64(); }
void SDL_Delay(Uint32 ms) { Sleep(ms); }
#else
Uint64 SDL_GetPerformanceCounter(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (Uint64)ts.tv_sec * 1000000000ull + (Uint64)ts.tv_nsec;
}
Uint64 SDL_GetPerformanceFrequency(void) { return 1000000000ull; }
Uint64 SDL_GetTicks(void) { return SDL_GetPerformanceCounter() / 1000000ull; }
void SDL_Delay(Uint32 ms) { usleep((useconds_t)ms * 1000); }
#endif

int SDL_GetVersion(void) { return (3 << 16); }
const char* SDL_GetPlatform(void) { return "libretro"; }
const char* SDL_GetCurrentVideoDriver(void) { return "libretro"; }
int SDL_GetNumVideoDisplays(void) { return 1; }
SDL_DisplayID* SDL_GetDisplays(int* count)
{
	if (count) {
		*count = 1;
	}
	SDL_DisplayID* ids = (SDL_DisplayID*)calloc(2, sizeof(SDL_DisplayID));
	if (ids) {
		ids[0] = 1;
		ids[1] = 0;
	}
	return ids;
}
SDL_DisplayID SDL_GetPrimaryDisplay(void) { return 1; }
bool SDL_GetDisplayBounds(SDL_DisplayID displayID, SDL_Rect* rect)
{
	(void)displayID;
	if (rect) {
		rect->x = 0;
		rect->y = 0;
		rect->w = 1920;
		rect->h = 1080;
	}
	return true;
}
const SDL_DisplayMode* SDL_GetDesktopDisplayMode(SDL_DisplayID displayID)
{
	(void)displayID;
	static SDL_DisplayMode mode;
	mode.displayID = 1;
	mode.format = SDL_PIXELFORMAT_ARGB8888;
	mode.w = 1920;
	mode.h = 1080;
	mode.refresh_rate = 60.0f;
	mode.pixel_density = 1.0f;
	return &mode;
}
const SDL_DisplayMode* SDL_GetCurrentDisplayMode(SDL_DisplayID displayID) { return SDL_GetDesktopDisplayMode(displayID); }
const SDL_DisplayMode* SDL_GetDisplayMode(SDL_DisplayID displayID, int modeIndex)
{
	(void)modeIndex;
	return SDL_GetDesktopDisplayMode(displayID);
}
const SDL_DisplayMode* const* SDL_GetFullscreenDisplayModes(SDL_DisplayID displayID, int* count)
{
	(void)displayID;
	static const SDL_DisplayMode* list[2];
	list[0] = SDL_GetDesktopDisplayMode(1);
	list[1] = nullptr;
	if (count) {
		*count = 1;
	}
	return list;
}
bool SDL_GetDisplayDPI(SDL_DisplayID displayID, float* ddpi, float* hdpi, float* vdpi)
{
	(void)displayID;
	if (ddpi) *ddpi = 96.0f;
	if (hdpi) *hdpi = 96.0f;
	if (vdpi) *vdpi = 96.0f;
	return true;
}
float SDL_GetDisplayContentScale(SDL_DisplayID displayID)
{
	(void)displayID;
	return 1.0f;
}
const char* SDL_GetDisplayName(SDL_DisplayID displayID)
{
	(void)displayID;
	return "libretro";
}
int SDL_GetNumDisplayModes(SDL_DisplayID displayID)
{
	(void)displayID;
	return 1;
}
bool SDL_GetWindowDisplayMode(SDL_Window* window, SDL_DisplayMode* mode)
{
	(void)window;
	if (mode) {
		*mode = *SDL_GetDesktopDisplayMode(1);
	}
	return true;
}
SDL_DisplayID SDL_GetDisplayForWindow(SDL_Window* window)
{
	(void)window;
	return 1;
}
bool SDL_GetClosestFullscreenDisplayMode(SDL_DisplayID displayID, int w, int h, float refresh_rate, bool include_high_density_modes, SDL_DisplayMode* closest)
{
	(void)displayID;
	(void)w;
	(void)h;
	(void)refresh_rate;
	(void)include_high_density_modes;
	if (closest) {
		*closest = *SDL_GetDesktopDisplayMode(1);
	}
	return true;
}

int SDL_PointInRect(const SDL_Point* p, const SDL_Rect* r)
{
	if (!p || !r) {
		return SDL_FALSE;
	}
	return (p->x >= r->x && p->x < (r->x + r->w) && p->y >= r->y && p->y < (r->y + r->h)) ? SDL_TRUE : SDL_FALSE;
}

int SDL_IntersectRect(const SDL_Rect* a, const SDL_Rect* b, SDL_Rect* result)
{
	if (!a || !b || !result) {
		return SDL_FALSE;
	}
	const int x1 = (a->x > b->x) ? a->x : b->x;
	const int y1 = (a->y > b->y) ? a->y : b->y;
	const int x2 = ((a->x + a->w) < (b->x + b->w)) ? (a->x + a->w) : (b->x + b->w);
	const int y2 = ((a->y + a->h) < (b->y + b->h)) ? (a->y + a->h) : (b->y + b->h);
	if (x2 <= x1 || y2 <= y1) {
		return SDL_FALSE;
	}
	result->x = x1;
	result->y = y1;
	result->w = x2 - x1;
	result->h = y2 - y1;
	return SDL_TRUE;
}

SDL_Mutex* SDL_CreateMutex(void)
{
	SDL_Mutex* m = (SDL_Mutex*)calloc(1, sizeof(SDL_Mutex));
	if (!m) {
		return nullptr;
	}
	pthread_mutex_init(&m->mutex, nullptr);
	return m;
}
void SDL_DestroyMutex(SDL_Mutex* m)
{
	if (!m) return;
	pthread_mutex_destroy(&m->mutex);
	free(m);
}
void SDL_LockMutex(SDL_Mutex* m)
{
	if (m) {
		pthread_mutex_lock(&m->mutex);
	}
}
void SDL_UnlockMutex(SDL_Mutex* m)
{
	if (m) {
		pthread_mutex_unlock(&m->mutex);
	}
}
bool SDL_TryLockMutex(SDL_Mutex* m) { return m && pthread_mutex_trylock(&m->mutex) == 0; }

SDL_Condition* SDL_CreateCondition(void)
{
	SDL_Condition* c = (SDL_Condition*)calloc(1, sizeof(SDL_Condition));
	if (!c) {
		return nullptr;
	}
	pthread_cond_init(&c->cond, nullptr);
	return c;
}
void SDL_SignalCondition(SDL_Condition* c)
{
	if (c) {
		pthread_cond_signal(&c->cond);
	}
}
bool SDL_WaitConditionTimeout(SDL_Condition* c, SDL_Mutex* m, Sint32 ms)
{
	if (!c || !m) {
		return false;
	}
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += ms / 1000;
	ts.tv_nsec += (long)(ms % 1000) * 1000000L;
	if (ts.tv_nsec >= 1000000000L) {
		ts.tv_sec++;
		ts.tv_nsec -= 1000000000L;
	}
	return pthread_cond_timedwait(&c->cond, &m->mutex, &ts) == 0;
}

SDL_Semaphore* SDL_CreateSemaphore(Uint32 initial_value)
{
	SDL_Semaphore* s = (SDL_Semaphore*)calloc(1, sizeof(SDL_Semaphore));
	if (!s) {
		return nullptr;
	}
	if (sem_init(&s->sem, 0, initial_value) != 0) {
		free(s);
		return nullptr;
	}
	return s;
}
void SDL_DestroySemaphore(SDL_Semaphore* s)
{
	if (!s) return;
	sem_destroy(&s->sem);
	free(s);
}
void SDL_SignalSemaphore(SDL_Semaphore* s)
{
	if (s) {
		sem_post(&s->sem);
	}
}
void SDL_WaitSemaphore(SDL_Semaphore* s)
{
	if (!s) return;
	while (sem_wait(&s->sem) != 0 && errno == EINTR) {
	}
}
bool SDL_TryWaitSemaphore(SDL_Semaphore* s)
{
	return s && sem_trywait(&s->sem) == 0;
}
bool SDL_WaitSemaphoreTimeout(SDL_Semaphore* s, Sint32 ms)
{
	if (!s) {
		return false;
	}
#if defined(__APPLE__)
	Uint64 start = SDL_GetTicks();
	for (;;) {
		if (sem_trywait(&s->sem) == 0) {
			return true;
		}
		if (errno != EAGAIN && errno != EINTR) {
			return false;
		}
		if ((SDL_GetTicks() - start) >= (Uint64)ms) {
			return false;
		}
		SDL_Delay(1);
	}
#else
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += ms / 1000;
	ts.tv_nsec += (long)(ms % 1000) * 1000000L;
	if (ts.tv_nsec >= 1000000000L) {
		ts.tv_sec++;
		ts.tv_nsec -= 1000000000L;
	}
	return sem_timedwait(&s->sem, &ts) == 0;
#endif
}
Uint32 SDL_GetSemaphoreValue(SDL_Semaphore* s)
{
	if (!s) return 0;
	int v = 0;
	sem_getvalue(&s->sem, &v);
	return (Uint32)((v < 0) ? 0 : v);
}

SDL_Thread* SDL_CreateThread(SDL_ThreadFunction fn, const char* name, void* data)
{
	(void)name;
	SDL_Thread* t = (SDL_Thread*)calloc(1, sizeof(SDL_Thread));
	if (!t) {
		return nullptr;
	}
	t->fn = fn;
	t->data = data;
	if (pthread_create(&t->thread, nullptr, sdl_thread_start, t) != 0) {
		free(t);
		sdl_set_error("pthread_create failed");
		return nullptr;
	}
	return t;
}
void SDL_DetachThread(SDL_Thread* t)
{
	if (t) {
		pthread_detach(t->thread);
	}
}
void SDL_WaitThread(SDL_Thread* t, int* status)
{
	if (!t) return;
	pthread_join(t->thread, nullptr);
	if (status) {
		*status = t->status;
	}
	free(t);
}
SDL_ThreadID SDL_GetThreadID(SDL_Thread* t)
{
	if (t) {
		return (SDL_ThreadID)(uintptr_t)t->thread;
	}
	return (SDL_ThreadID)(uintptr_t)pthread_self();
}
int SDL_SetThreadPriority(SDL_ThreadPriority pri)
{
	(void)pri;
	return 0;
}
int SDL_SetCurrentThreadPriority(SDL_ThreadPriority pri)
{
	return SDL_SetThreadPriority(pri);
}

SDL_Surface* SDL_CreateSurface(int w, int h, SDL_PixelFormat format)
{
	if (w <= 0 || h <= 0) {
		return nullptr;
	}
	const SDL_PixelFormatDetails* fmt = sdl_format_details(format);
	SDL_Surface* s = (SDL_Surface*)calloc(1, sizeof(SDL_Surface));
	if (!s) {
		return nullptr;
	}
	s->format = format;
	s->w = w;
	s->h = h;
	s->pitch = w * fmt->bytes_per_pixel;
	s->pixels = calloc(1, (size_t)s->pitch * (size_t)h);
	if (!s->pixels) {
		free(s);
		return nullptr;
	}
	return s;
}

SDL_Surface* SDL_CreateSurfaceFrom(int w, int h, SDL_PixelFormat format, void* pixels, int pitch)
{
	if (w <= 0 || h <= 0 || !pixels || pitch <= 0) {
		return nullptr;
	}
	SDL_Surface* s = (SDL_Surface*)calloc(1, sizeof(SDL_Surface));
	if (!s) {
		return nullptr;
	}
	s->flags = SDL_PREALLOC;
	s->format = format;
	s->w = w;
	s->h = h;
	s->pitch = pitch;
	s->pixels = pixels;
	return s;
}

void SDL_DestroySurface(SDL_Surface* s)
{
	if (!s) return;
	if (!(s->flags & SDL_PREALLOC) && s->pixels) {
		free(s->pixels);
	}
	free(s);
}

const SDL_PixelFormatDetails* SDL_GetPixelFormatDetails(SDL_PixelFormat format)
{
	return sdl_format_details(format);
}

bool SDL_BlitSurface(SDL_Surface* src, const SDL_Rect* sr, SDL_Surface* dst, const SDL_Rect* dr)
{
	(void)src;
	(void)sr;
	(void)dst;
	(void)dr;
	return true;
}

bool SDL_FillSurfaceRect(SDL_Surface* dst, const SDL_Rect* rect, Uint32 color)
{
	(void)rect;
	if (!dst || !dst->pixels) {
		return false;
	}
	const SDL_PixelFormatDetails* fmt = sdl_format_details(dst->format);
	if (fmt->bytes_per_pixel == 4) {
		Uint32* p = (Uint32*)dst->pixels;
		size_t count = (size_t)dst->w * (size_t)dst->h;
		for (size_t i = 0; i < count; ++i) {
			p[i] = color;
		}
	} else {
		Uint16* p = (Uint16*)dst->pixels;
		size_t count = (size_t)dst->w * (size_t)dst->h;
		Uint16 c16 = (Uint16)color;
		for (size_t i = 0; i < count; ++i) {
			p[i] = c16;
		}
	}
	return true;
}

SDL_Texture* SDL_CreateTexture(SDL_Renderer* renderer, Uint32 format, int access, int w, int h)
{
	(void)renderer;
	SDL_Texture* t = (SDL_Texture*)calloc(1, sizeof(SDL_Texture));
	if (!t) return nullptr;
	t->format = format;
	t->access = access;
	t->w = w;
	t->h = h;
	t->alpha = 255;
	t->cr = t->cg = t->cb = 255;
	return t;
}
SDL_Texture* SDL_CreateTextureFromSurface(SDL_Renderer* renderer, SDL_Surface* surface)
{
	if (!surface) return nullptr;
	return SDL_CreateTexture(renderer, surface->format, SDL_TEXTUREACCESS_STREAMING, surface->w, surface->h);
}
void SDL_DestroyTexture(SDL_Texture* texture) { free(texture); }
bool SDL_QueryTexture(SDL_Texture* texture, Uint32* format, int* access, int* w, int* h)
{
	if (!texture) return false;
	if (format) *format = texture->format;
	if (access) *access = texture->access;
	if (w) *w = texture->w;
	if (h) *h = texture->h;
	return true;
}
bool SDL_GetTextureSize(SDL_Texture* texture, float* w, float* h)
{
	if (!texture) {
		return false;
	}
	if (w) *w = (float)texture->w;
	if (h) *h = (float)texture->h;
	return true;
}
bool SDL_UpdateTexture(SDL_Texture* texture, const SDL_Rect* rect, const void* pixels, int pitch)
{
	(void)texture;
	(void)rect;
	(void)pixels;
	(void)pitch;
	return true;
}
bool SDL_LockTexture(SDL_Texture* texture, const SDL_Rect* rect, void** pixels, int* pitch)
{
	(void)texture;
	(void)rect;
	if (pixels) *pixels = nullptr;
	if (pitch) *pitch = 0;
	return true;
}
void SDL_UnlockTexture(SDL_Texture* texture) { (void)texture; }
bool SDL_RenderTexture(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_FRect* sr, const SDL_FRect* dr)
{
	(void)renderer;
	(void)texture;
	(void)sr;
	(void)dr;
	return true;
}
bool SDL_RenderTextureRotated(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_FRect* sr, const SDL_FRect* dr, double angle, const SDL_FPoint* center, SDL_RendererFlip flip)
{
	(void)renderer;
	(void)texture;
	(void)sr;
	(void)dr;
	(void)angle;
	(void)center;
	(void)flip;
	return true;
}
bool SDL_RenderFillRect(SDL_Renderer* renderer, const SDL_FRect* rect)
{
	(void)renderer;
	(void)rect;
	return true;
}
bool SDL_SetRenderLogicalPresentation(SDL_Renderer* renderer, int w, int h, SDL_LogicalPresentation mode)
{
	(void)mode;
	if (renderer) {
		renderer->logical_w = w;
		renderer->logical_h = h;
	}
	return true;
}
bool SDL_GetRenderLogicalPresentation(SDL_Renderer* renderer, int* w, int* h, SDL_LogicalPresentation* mode, SDL_FRect* viewport)
{
	if (w) *w = renderer ? renderer->logical_w : 0;
	if (h) *h = renderer ? renderer->logical_h : 0;
	if (mode) *mode = SDL_LOGICAL_PRESENTATION_LETTERBOX;
	if (viewport) {
		viewport->x = 0.0f;
		viewport->y = 0.0f;
		viewport->w = renderer && renderer->window ? (float)renderer->window->w : 0.0f;
		viewport->h = renderer && renderer->window ? (float)renderer->window->h : 0.0f;
	}
	return true;
}
bool SDL_RenderSetIntegerScale(SDL_Renderer* renderer, SDL_bool enable)
{
	(void)renderer;
	(void)enable;
	return true;
}
bool SDL_RenderSetScale(SDL_Renderer* renderer, float scaleX, float scaleY)
{
	(void)renderer;
	(void)scaleX;
	(void)scaleY;
	return true;
}
bool SDL_SetRenderScale(SDL_Renderer* renderer, float scaleX, float scaleY)
{
	return SDL_RenderSetScale(renderer, scaleX, scaleY);
}
void SDL_RenderGetLogicalSize(SDL_Renderer* renderer, int* w, int* h)
{
	if (w) *w = renderer ? renderer->logical_w : 0;
	if (h) *h = renderer ? renderer->logical_h : 0;
}
bool SDL_GetCurrentRenderOutputSize(SDL_Renderer* renderer, int* w, int* h)
{
	if (w) *w = renderer && renderer->window ? renderer->window->w : 1920;
	if (h) *h = renderer && renderer->window ? renderer->window->h : 1080;
	return true;
}
bool SDL_RenderPresent(SDL_Renderer* renderer)
{
	(void)renderer;
	return true;
}
bool SDL_RenderClear(SDL_Renderer* renderer)
{
	(void)renderer;
	return true;
}
void SDL_DestroyRenderer(SDL_Renderer* renderer)
{
	if (renderer == &g_renderer) {
		SDL_zero(g_renderer);
	}
}
bool SDL_SetRenderDrawColor(SDL_Renderer* renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	(void)renderer;
	(void)r;
	(void)g;
	(void)b;
	(void)a;
	return true;
}
bool SDL_GetRenderDrawColor(SDL_Renderer* renderer, Uint8* r, Uint8* g, Uint8* b, Uint8* a)
{
	(void)renderer;
	if (r) *r = 0;
	if (g) *g = 0;
	if (b) *b = 0;
	if (a) *a = 255;
	return true;
}
bool SDL_SetRenderDrawBlendMode(SDL_Renderer* renderer, SDL_BlendMode blendMode)
{
	(void)renderer;
	(void)blendMode;
	return true;
}
bool SDL_SetSurfaceBlendMode(SDL_Surface* surface, SDL_BlendMode blendMode)
{
	(void)surface;
	(void)blendMode;
	return true;
}
bool SDL_SetTextureBlendMode(SDL_Texture* texture, SDL_BlendMode blendMode)
{
	(void)texture;
	(void)blendMode;
	return true;
}
bool SDL_SetTextureAlphaMod(SDL_Texture* texture, Uint8 alpha)
{
	if (texture) {
		texture->alpha = alpha;
	}
	return true;
}
bool SDL_SetTextureColorMod(SDL_Texture* texture, Uint8 r, Uint8 g, Uint8 b)
{
	if (texture) {
		texture->cr = r;
		texture->cg = g;
		texture->cb = b;
	}
	return true;
}
bool SDL_SetTextureScaleMode(SDL_Texture* texture, SDL_ScaleMode scaleMode)
{
	(void)texture;
	(void)scaleMode;
	return true;
}

SDL_Window* SDL_CreateWindow(const char* title, int w, int h, SDL_WindowFlags flags)
{
	(void)title;
	g_window.id = g_next_window_id++;
	g_window.w = (w > 0) ? w : 1920;
	g_window.h = (h > 0) ? h : 1080;
	g_window.flags = flags;
	return &g_window;
}
SDL_Renderer* SDL_CreateRenderer(SDL_Window* window, const char* name)
{
	(void)name;
	g_renderer.window = window ? window : &g_window;
	g_renderer.logical_w = g_renderer.window->w;
	g_renderer.logical_h = g_renderer.window->h;
	return &g_renderer;
}
SDL_Window* SDL_GetWindowFromID(SDL_WindowID id)
{
	return (g_window.id == id) ? &g_window : nullptr;
}
SDL_WindowID SDL_GetWindowID(SDL_Window* window) { return window ? window->id : 0; }
SDL_WindowFlags SDL_GetWindowFlags(SDL_Window* window) { return window ? window->flags : 0; }
void SDL_GetWindowPosition(SDL_Window* window, int* x, int* y)
{
	(void)window;
	if (x) *x = 0;
	if (y) *y = 0;
}
void SDL_GetWindowSize(SDL_Window* window, int* w, int* h)
{
	if (w) *w = window ? window->w : 0;
	if (h) *h = window ? window->h : 0;
}
void SDL_SetWindowGrab(SDL_Window* window, SDL_bool grabbed)
{
	(void)window;
	(void)grabbed;
}
void SDL_SetWindowMouseGrab(SDL_Window* window, SDL_bool grabbed)
{
	SDL_SetWindowGrab(window, grabbed);
}
void SDL_SetWindowKeyboardGrab(SDL_Window* window, SDL_bool grabbed)
{
	(void)window;
	(void)grabbed;
}
bool SDL_SetWindowFullscreenMode(SDL_Window* window, const SDL_DisplayMode* mode)
{
	(void)window;
	(void)mode;
	return true;
}
void SDL_SetWindowSize(SDL_Window* window, int w, int h)
{
	if (window) {
		window->w = w;
		window->h = h;
	}
}
void SDL_SetWindowPosition(SDL_Window* window, int x, int y)
{
	(void)window;
	(void)x;
	(void)y;
}
void SDL_RaiseWindow(SDL_Window* window) { (void)window; }
void SDL_MinimizeWindow(SDL_Window* window) { (void)window; }
void SDL_DestroyWindow(SDL_Window* window)
{
	if (window == &g_window) {
		SDL_zero(g_window);
	}
}
void SDL_SetWindowIcon(SDL_Window* window, SDL_Surface* icon)
{
	(void)window;
	(void)icon;
}

SDL_Window* SDL_GetKeyboardFocus(void) { return &g_window; }
const bool* SDL_GetKeyboardState(int* numkeys)
{
	static bool keys[SDL_SCANCODE_COUNT];
	if (numkeys) {
		*numkeys = SDL_SCANCODE_COUNT;
	}
	return keys;
}
SDL_Keycode SDL_GetKeyFromScancode(SDL_Scancode scancode, ...)
{
	return (SDL_Keycode)scancode;
}
SDL_Scancode SDL_GetScancodeFromName(const char* name)
{
	(void)name;
	return SDL_SCANCODE_UNKNOWN;
}
SDL_Keymod SDL_GetModState(void) { return KMOD_NONE; }

SDL_Cursor* SDL_GetCursor(void) { return nullptr; }
void SDL_ShowCursor(void) {}
void SDL_HideCursor(void) {}
SDL_Cursor* SDL_CreateColorCursor(SDL_Surface* surface, int hot_x, int hot_y)
{
	(void)surface;
	(void)hot_x;
	(void)hot_y;
	return nullptr;
}
SDL_Cursor* SDL_CreateSystemCursor(SDL_SystemCursor id)
{
	(void)id;
	return nullptr;
}
void SDL_DestroyCursor(SDL_Cursor* cursor) { (void)cursor; }
void SDL_SetCursor(SDL_Cursor* cursor) { (void)cursor; }
bool SDL_WarpMouseGlobal(float x, float y)
{
	(void)x;
	(void)y;
	return true;
}
void SDL_WarpMouseInWindow(SDL_Window* window, float x, float y)
{
	(void)window;
	(void)x;
	(void)y;
}
bool SDL_SetWindowRelativeMouseMode(SDL_Window* window, bool enabled)
{
	(void)window;
	(void)enabled;
	return true;
}

bool SDL_HasClipboardText(void) { return false; }
char* SDL_GetClipboardText(void) { return SDL_strdup(""); }
bool SDL_SetClipboardText(const char* text)
{
	(void)text;
	return true;
}

bool SDL_SetHint(const char* name, const char* value)
{
	(void)name;
	(void)value;
	return true;
}
bool SDL_GL_SetAttribute(SDL_GLattr attr, int value)
{
	(void)attr;
	(void)value;
	return true;
}
void SDL_GL_ResetAttributes(void) {}

bool SDL_PushEvent(SDL_Event* event)
{
	(void)event;
	return false;
}
bool SDL_PollEvent(SDL_Event* event)
{
	(void)event;
	return false;
}
void SDL_PumpEvents(void) {}
int SDL_PeepEvents(SDL_Event* events, int numevents, int action, Uint32 minType, Uint32 maxType)
{
	(void)events;
	(void)numevents;
	(void)action;
	(void)minType;
	(void)maxType;
	return 0;
}

SDL_Gamepad* SDL_OpenGamepad(int joystick_index)
{
	SDL_Gamepad* g = (SDL_Gamepad*)calloc(1, sizeof(SDL_Gamepad));
	if (g) {
		g->index = joystick_index;
	}
	return g;
}
void SDL_CloseGamepad(SDL_Gamepad* gamepad) { free(gamepad); }
SDL_Joystick* SDL_GetGamepadJoystick(SDL_Gamepad* gamepad)
{
	if (!gamepad) return nullptr;
	SDL_Joystick* j = (SDL_Joystick*)calloc(1, sizeof(SDL_Joystick));
	if (j) {
		j->index = gamepad->index;
	}
	return j;
}
const char* SDL_GetGamepadName(SDL_Gamepad* gamepad)
{
	(void)gamepad;
	return "Stub Gamepad";
}
const char* SDL_GetGamepadNameForID(SDL_JoystickID joystick_id)
{
	(void)joystick_id;
	return "Stub Gamepad";
}
const char* SDL_GetGamepadNameForIndex(int joystick_index)
{
	(void)joystick_index;
	return "Stub Gamepad";
}
char* SDL_GetGamepadMapping(SDL_Gamepad* gamepad)
{
	(void)gamepad;
	return SDL_strdup("stub");
}
const char* SDL_GetGamepadStringForAxis(SDL_GamepadAxis axis)
{
	static const char* names[] = {
		"leftx", "lefty", "rightx", "righty", "lefttrigger", "righttrigger"
	};
	if (axis < 0 || axis >= SDL_GAMEPAD_AXIS_COUNT) {
		return "invalid";
	}
	return names[axis];
}
const char* SDL_GetGamepadStringForButton(SDL_GamepadButton button)
{
	static const char* names[] = {
		"a", "b", "x", "y", "back", "guide", "start",
		"leftstick", "rightstick", "leftshoulder", "rightshoulder",
		"dpup", "dpdown", "dpleft", "dpright", "misc1",
		"rightpaddle1", "leftpaddle1", "rightpaddle2", "leftpaddle2",
		"touchpad", "misc2", "misc3", "misc4", "misc5", "misc6"
	};
	if (button < 0 || button >= SDL_GAMEPAD_BUTTON_COUNT) {
		return "invalid";
	}
	return names[button];
}
Uint8 SDL_GetGamepadButton(SDL_Gamepad* gamepad, SDL_GamepadButton button)
{
	(void)gamepad;
	(void)button;
	return 0;
}
SDL_GamepadButton SDL_GetGamepadButtonFromString(const char* str)
{
	(void)str;
	return SDL_GAMEPAD_BUTTON_INVALID;
}
Sint16 SDL_GetGamepadAxis(SDL_Gamepad* gamepad, SDL_GamepadAxis axis)
{
	(void)gamepad;
	(void)axis;
	return 0;
}
bool SDL_AddGamepadMappingsFromFile(const char* path)
{
	(void)path;
	return true;
}
bool SDL_IsGamepad(int joystick_index)
{
	(void)joystick_index;
	return false;
}
SDL_Gamepad* SDL_GetGamepadFromID(SDL_JoystickID joystick_id)
{
	(void)joystick_id;
	return nullptr;
}
bool SDL_GamepadConnected(SDL_Gamepad* gamepad)
{
	return gamepad != nullptr;
}
const SDL_GamepadBinding* SDL_GetGamepadBindings(SDL_Gamepad* gamepad, int* count)
{
	(void)gamepad;
	if (count) {
		*count = 0;
	}
	return nullptr;
}

SDL_Joystick* SDL_OpenJoystick(int device_index)
{
	SDL_Joystick* j = (SDL_Joystick*)calloc(1, sizeof(SDL_Joystick));
	if (j) {
		j->index = device_index;
	}
	return j;
}
void SDL_CloseJoystick(SDL_Joystick* joystick) { free(joystick); }
const char* SDL_GetJoystickName(SDL_Joystick* joystick)
{
	(void)joystick;
	return "Stub Joystick";
}
const char* SDL_GetJoystickNameForID(SDL_JoystickID joystick_id)
{
	(void)joystick_id;
	return "Stub Joystick";
}
const char* SDL_GetJoystickNameForIndex(int device_index)
{
	(void)device_index;
	return "Stub Joystick";
}
SDL_JoystickID SDL_GetJoystickID(SDL_Joystick* joystick)
{
	return joystick ? joystick->index : -1;
}
SDL_JoystickGUID SDL_GetJoystickGUID(SDL_Joystick* joystick)
{
	SDL_JoystickGUID g{};
	(void)joystick;
	return g;
}
void SDL_GetJoystickGUIDString(SDL_JoystickGUID guid, char* pszGUID, int cbGUID)
{
	(void)guid;
	if (pszGUID && cbGUID > 0) {
		pszGUID[0] = '\0';
	}
}
void SDL_GUIDToString(SDL_JoystickGUID guid, char* pszGUID, int cbGUID)
{
	SDL_GetJoystickGUIDString(guid, pszGUID, cbGUID);
}
int SDL_GetNumJoystickAxes(SDL_Joystick* joystick)
{
	(void)joystick;
	return 0;
}
int SDL_GetNumJoystickBalls(SDL_Joystick* joystick)
{
	(void)joystick;
	return 0;
}
int SDL_GetNumJoystickButtons(SDL_Joystick* joystick)
{
	(void)joystick;
	return 0;
}
int SDL_GetNumJoystickHats(SDL_Joystick* joystick)
{
	(void)joystick;
	return 0;
}
Sint16 SDL_GetJoystickAxis(SDL_Joystick* joystick, int axis)
{
	(void)joystick;
	(void)axis;
	return 0;
}
Uint8 SDL_GetJoystickButton(SDL_Joystick* joystick, int button)
{
	(void)joystick;
	(void)button;
	return 0;
}
int SDL_GetJoysticks(int** joysticks)
{
	if (joysticks) {
		*joysticks = nullptr;
	}
	return 0;
}
int SDL_NumJoysticks(void) { return 0; }

const char* SDL_GetAudioDeviceName(SDL_AudioDeviceID devid, ...)
{
	(void)devid;
	return "stub-audio";
}
SDL_AudioDeviceID* SDL_GetAudioPlaybackDevices(int* count)
{
	if (count) {
		*count = 1;
	}
	SDL_AudioDeviceID* ids = (SDL_AudioDeviceID*)calloc(2, sizeof(SDL_AudioDeviceID));
	if (ids) {
		ids[0] = 1;
		ids[1] = 0;
	}
	return ids;
}
SDL_AudioDeviceID* SDL_GetAudioRecordingDevices(int* count)
{
	if (count) {
		*count = 1;
	}
	SDL_AudioDeviceID* ids = (SDL_AudioDeviceID*)calloc(2, sizeof(SDL_AudioDeviceID));
	if (ids) {
		ids[0] = 2;
		ids[1] = 0;
	}
	return ids;
}
SDL_AudioStream* SDL_OpenAudioDeviceStream(SDL_AudioDeviceID devid, const SDL_AudioSpec* spec, SDL_AudioStreamCallback callback, void* userdata)
{
	SDL_AudioStream* s = (SDL_AudioStream*)calloc(1, sizeof(SDL_AudioStream));
	if (!s) {
		return nullptr;
	}
	s->devid = devid;
	if (spec) {
		s->spec = *spec;
	}
	s->callback = callback;
	s->userdata = userdata;
	s->queued = 0;
	s->gain = 1.0f;
	s->paused = false;
	return s;
}
bool SDL_PutAudioStreamData(SDL_AudioStream* stream, const void* buf, int len)
{
	(void)buf;
	if (!stream || len < 0) {
		return false;
	}
	stream->queued += len;
	return true;
}
int SDL_GetAudioStreamData(SDL_AudioStream* stream, void* buf, int len)
{
	(void)buf;
	if (!stream || len < 0) {
		return -1;
	}
	int out = (stream->queued < len) ? stream->queued : len;
	stream->queued -= out;
	return out;
}
bool SDL_ResumeAudioStreamDevice(SDL_AudioStream* stream)
{
	if (!stream) return false;
	stream->paused = false;
	return true;
}
bool SDL_PauseAudioStreamDevice(SDL_AudioStream* stream)
{
	if (!stream) return false;
	stream->paused = true;
	return true;
}
void SDL_DestroyAudioStream(SDL_AudioStream* stream)
{
	free(stream);
}
int SDL_GetAudioStreamQueued(SDL_AudioStream* stream)
{
	return stream ? stream->queued : 0;
}
int SDL_GetAudioStreamAvailable(SDL_AudioStream* stream)
{
	return stream ? stream->queued : 0;
}
void SDL_LockAudioStream(SDL_AudioStream* stream)
{
	(void)stream;
}
void SDL_UnlockAudioStream(SDL_AudioStream* stream)
{
	(void)stream;
}
bool SDL_SetAudioStreamGain(SDL_AudioStream* stream, float gain)
{
	if (!stream) return false;
	stream->gain = gain;
	return true;
}

bool SDL_Init(Uint32 flags)
{
	(void)flags;
	return true;
}
void SDL_Quit(void) {}

SDL_TimerID SDL_AddTimer(Uint32 interval, Uint32 (*callback)(Uint32, void*), void* param)
{
	(void)interval;
	(void)callback;
	(void)param;
	return 0;
}
SDL_bool SDL_RemoveTimer(SDL_TimerID id)
{
	(void)id;
	return SDL_TRUE;
}

SDL_AudioSpec* SDL_LoadWAV(const char* file, SDL_AudioSpec* spec, Uint8** audio_buf, Uint32* audio_len)
{
	(void)file;
	(void)spec;
	(void)audio_buf;
	(void)audio_len;
	return nullptr;
}
void SDL_FreeWAV(Uint8* audio_buf)
{
	(void)audio_buf;
}

}
#endif
