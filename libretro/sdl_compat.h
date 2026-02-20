#ifndef LIBRETRO_SDL_COMPAT_H
#define LIBRETRO_SDL_COMPAT_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t Uint8;
typedef int8_t Sint8;
typedef uint16_t Uint16;
typedef int16_t Sint16;
typedef uint32_t Uint32;
typedef int32_t Sint32;
typedef uint64_t Uint64;
typedef int64_t Sint64;

typedef int SDL_bool;

#define SDL_FALSE 0
#define SDL_TRUE 1

#define SDL_DISABLE 0
#define SDL_ENABLE 1

#define SDL_PRESSED 1
#define SDL_RELEASED 0

typedef int SDL_Keycode;
typedef int SDL_Keymod;

#define KMOD_NONE 0x0000
#define KMOD_LSHIFT 0x0001
#define KMOD_RSHIFT 0x0002
#define KMOD_LCTRL 0x0040
#define KMOD_RCTRL 0x0080
#define KMOD_LALT 0x0100
#define KMOD_RALT 0x0200
#define KMOD_LGUI 0x0400
#define KMOD_RGUI 0x0800
#define KMOD_NUM 0x1000
#define KMOD_CAPS 0x2000
#define KMOD_MODE 0x4000
#define KMOD_RESERVED 0x8000
#define KMOD_SHIFT (KMOD_LSHIFT | KMOD_RSHIFT)
#define KMOD_CTRL (KMOD_LCTRL | KMOD_RCTRL)
#define KMOD_ALT (KMOD_LALT | KMOD_RALT)
#define KMOD_GUI (KMOD_LGUI | KMOD_RGUI)

typedef struct SDL_version {
	Uint8 major;
	Uint8 minor;
	Uint8 patch;
} SDL_version;

#define SDL_VERSION(x) do { (x)->major = 2; (x)->minor = 0; (x)->patch = 0; } while (0)
#define SDL_VERSION_ATLEAST(x, y, z) 1

typedef struct SDL_Point {
	int x;
	int y;
} SDL_Point;

typedef struct SDL_Rect {
	int x;
	int y;
	int w;
	int h;
} SDL_Rect;

typedef struct SDL_Color {
	Uint8 r;
	Uint8 g;
	Uint8 b;
	Uint8 a;
} SDL_Color;

typedef struct _TTF_Font TTF_Font;

int TTF_Init(void);
int TTF_SetFontSizeDPI(TTF_Font* font, int ptsize, unsigned int hdpi, unsigned int vdpi);

typedef struct SDL_DisplayMode {
	Uint32 format;
	int w;
	int h;
	int refresh_rate;
	void* driverdata;
} SDL_DisplayMode;

typedef struct SDL_PixelFormat {
	Uint32 format;
	Uint8 BitsPerPixel;
	Uint8 BytesPerPixel;
	Uint8 padding[2];
	Uint32 Rmask;
	Uint32 Gmask;
	Uint32 Bmask;
	Uint32 Amask;
	Uint8 Rshift;
	Uint8 Gshift;
	Uint8 Bshift;
	Uint8 Ashift;
	Uint32 refcount;
	void* palette;
} SDL_PixelFormat;

typedef struct SDL_Surface {
	Uint32 flags;
	SDL_PixelFormat* format;
	int w;
	int h;
	int pitch;
	void* pixels;
	void* userdata;
} SDL_Surface;

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_Cursor SDL_Cursor;
typedef struct SDL_GameController SDL_GameController;
typedef struct SDL_Joystick SDL_Joystick;
typedef struct SDL_Thread SDL_Thread;
typedef struct SDL_mutex SDL_mutex;
typedef struct SDL_cond SDL_cond;
typedef struct SDL_semaphore SDL_semaphore;
typedef struct SDL_semaphore SDL_sem;

typedef struct SDL_JoystickGUID {
	Uint8 data[16];
} SDL_JoystickGUID;

typedef Sint32 SDL_JoystickID;
typedef uintptr_t SDL_threadID;

typedef enum SDL_ThreadPriority {
	SDL_THREAD_PRIORITY_LOW,
	SDL_THREAD_PRIORITY_NORMAL,
	SDL_THREAD_PRIORITY_HIGH
} SDL_ThreadPriority;

typedef int (*SDL_ThreadFunction)(void*);

typedef void* SDL_GLContext;
typedef int SDL_GLattr;

typedef enum SDL_BlendMode {
	SDL_BLENDMODE_NONE = 0,
	SDL_BLENDMODE_BLEND = 1
} SDL_BlendMode;

typedef enum SDL_RendererFlip {
	SDL_FLIP_NONE = 0
} SDL_RendererFlip;

typedef Uint16 SDL_AudioFormat;
typedef void (*SDL_AudioCallback)(void*, Uint8*, int);

typedef struct SDL_AudioSpec {
	int freq;
	SDL_AudioFormat format;
	Uint8 channels;
	Uint8 silence;
	Uint16 samples;
	Uint32 size;
	SDL_AudioCallback callback;
	void* userdata;
} SDL_AudioSpec;

typedef Uint32 SDL_AudioDeviceID;
typedef int SDL_TimerID;

#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define SDL_BYTEORDER 4321
#else
#define SDL_BYTEORDER 1234
#endif

#define AUDIO_U8 0x0008
#define AUDIO_S8 0x8008
#define AUDIO_U16LSB 0x0010
#define AUDIO_S16LSB 0x8010
#define AUDIO_U16MSB 0x1010
#define AUDIO_S16MSB 0x9010
#if SDL_BYTEORDER == 1234
#define AUDIO_U16SYS AUDIO_U16LSB
#define AUDIO_S16SYS AUDIO_S16LSB
#else
#define AUDIO_U16SYS AUDIO_U16MSB
#define AUDIO_S16SYS AUDIO_S16MSB
#endif

typedef Sint64 SDL_TouchID;
typedef Sint64 SDL_FingerID;

typedef enum SDL_Scancode {
	SDL_SCANCODE_UNKNOWN = 0,
	SDL_SCANCODE_A = 4,
	SDL_SCANCODE_B = 5,
	SDL_SCANCODE_C = 6,
	SDL_SCANCODE_D = 7,
	SDL_SCANCODE_E = 8,
	SDL_SCANCODE_F = 9,
	SDL_SCANCODE_G = 10,
	SDL_SCANCODE_H = 11,
	SDL_SCANCODE_I = 12,
	SDL_SCANCODE_J = 13,
	SDL_SCANCODE_K = 14,
	SDL_SCANCODE_L = 15,
	SDL_SCANCODE_M = 16,
	SDL_SCANCODE_N = 17,
	SDL_SCANCODE_O = 18,
	SDL_SCANCODE_P = 19,
	SDL_SCANCODE_Q = 20,
	SDL_SCANCODE_R = 21,
	SDL_SCANCODE_S = 22,
	SDL_SCANCODE_T = 23,
	SDL_SCANCODE_U = 24,
	SDL_SCANCODE_V = 25,
	SDL_SCANCODE_W = 26,
	SDL_SCANCODE_X = 27,
	SDL_SCANCODE_Y = 28,
	SDL_SCANCODE_Z = 29,
	SDL_SCANCODE_1 = 30,
	SDL_SCANCODE_2 = 31,
	SDL_SCANCODE_3 = 32,
	SDL_SCANCODE_4 = 33,
	SDL_SCANCODE_5 = 34,
	SDL_SCANCODE_6 = 35,
	SDL_SCANCODE_7 = 36,
	SDL_SCANCODE_8 = 37,
	SDL_SCANCODE_9 = 38,
	SDL_SCANCODE_0 = 39,
	SDL_SCANCODE_RETURN = 40,
	SDL_SCANCODE_ESCAPE = 41,
	SDL_SCANCODE_BACKSPACE = 42,
	SDL_SCANCODE_TAB = 43,
	SDL_SCANCODE_SPACE = 44,
	SDL_SCANCODE_MINUS = 45,
	SDL_SCANCODE_EQUALS = 46,
	SDL_SCANCODE_LEFTBRACKET = 47,
	SDL_SCANCODE_RIGHTBRACKET = 48,
	SDL_SCANCODE_BACKSLASH = 49,
	SDL_SCANCODE_SEMICOLON = 51,
	SDL_SCANCODE_APOSTROPHE = 52,
	SDL_SCANCODE_GRAVE = 53,
	SDL_SCANCODE_COMMA = 54,
	SDL_SCANCODE_PERIOD = 55,
	SDL_SCANCODE_SLASH = 56,
	SDL_SCANCODE_CAPSLOCK = 57,
	SDL_SCANCODE_F1 = 58,
	SDL_SCANCODE_F2 = 59,
	SDL_SCANCODE_F3 = 60,
	SDL_SCANCODE_F4 = 61,
	SDL_SCANCODE_F5 = 62,
	SDL_SCANCODE_F6 = 63,
	SDL_SCANCODE_F7 = 64,
	SDL_SCANCODE_F8 = 65,
	SDL_SCANCODE_F9 = 66,
	SDL_SCANCODE_F10 = 67,
	SDL_SCANCODE_F11 = 68,
	SDL_SCANCODE_F12 = 69,
	SDL_SCANCODE_PRINTSCREEN = 70,
	SDL_SCANCODE_SCROLLLOCK = 71,
	SDL_SCANCODE_PAUSE = 72,
	SDL_SCANCODE_INSERT = 73,
	SDL_SCANCODE_HOME = 74,
	SDL_SCANCODE_PAGEUP = 75,
	SDL_SCANCODE_DELETE = 76,
	SDL_SCANCODE_END = 77,
	SDL_SCANCODE_PAGEDOWN = 78,
	SDL_SCANCODE_RIGHT = 79,
	SDL_SCANCODE_LEFT = 80,
	SDL_SCANCODE_DOWN = 81,
	SDL_SCANCODE_UP = 82,
	SDL_SCANCODE_NUMLOCKCLEAR = 83,
	SDL_SCANCODE_KP_DIVIDE = 84,
	SDL_SCANCODE_KP_MULTIPLY = 85,
	SDL_SCANCODE_KP_MINUS = 86,
	SDL_SCANCODE_KP_PLUS = 87,
	SDL_SCANCODE_KP_ENTER = 88,
	SDL_SCANCODE_KP_1 = 89,
	SDL_SCANCODE_KP_2 = 90,
	SDL_SCANCODE_KP_3 = 91,
	SDL_SCANCODE_KP_4 = 92,
	SDL_SCANCODE_KP_5 = 93,
	SDL_SCANCODE_KP_6 = 94,
	SDL_SCANCODE_KP_7 = 95,
	SDL_SCANCODE_KP_8 = 96,
	SDL_SCANCODE_KP_9 = 97,
	SDL_SCANCODE_KP_0 = 98,
	SDL_SCANCODE_KP_PERIOD = 99,
	SDL_SCANCODE_NONUSBACKSLASH = 100,
	SDL_SCANCODE_APPLICATION = 101,
	SDL_SCANCODE_KP_EQUALS = 103,
	SDL_SCANCODE_F13 = 104,
	SDL_SCANCODE_F14 = 105,
	SDL_SCANCODE_F15 = 106,
	SDL_SCANCODE_MENU = 118,
	SDL_SCANCODE_SYSREQ = 154,
	SDL_SCANCODE_KP_PLUSMINUS = 182,
	SDL_SCANCODE_KP_DECIMAL = 220,
	SDL_SCANCODE_LCTRL = 224,
	SDL_SCANCODE_LSHIFT = 225,
	SDL_SCANCODE_LALT = 226,
	SDL_SCANCODE_LGUI = 227,
	SDL_SCANCODE_RCTRL = 228,
	SDL_SCANCODE_RSHIFT = 229,
	SDL_SCANCODE_RALT = 230,
	SDL_SCANCODE_RGUI = 231,
	SDL_SCANCODE_AUDIOSTOP = 258,
	SDL_SCANCODE_AUDIOPLAY = 259,
	SDL_SCANCODE_AUDIOPREV = 260,
	SDL_SCANCODE_AUDIONEXT = 261
} SDL_Scancode;

#define SDL_NUM_SCANCODES 512

typedef struct SDL_Keysym {
	SDL_Scancode scancode;
	SDL_Keycode sym;
	SDL_Keymod mod;
	Uint32 unused;
} SDL_Keysym;

typedef struct SDL_WindowEvent {
	Uint32 type;
	Uint32 timestamp;
	Uint32 windowID;
	Uint8 event;
	Uint8 padding1;
	Uint8 padding2;
	Uint8 padding3;
	Sint32 data1;
	Sint32 data2;
} SDL_WindowEvent;

typedef struct SDL_KeyboardEvent {
	Uint32 type;
	Uint32 timestamp;
	Uint32 windowID;
	Uint8 state;
	Uint8 repeat;
	Uint8 padding2;
	Uint8 padding3;
	SDL_Keysym keysym;
} SDL_KeyboardEvent;

typedef struct SDL_ControllerButtonEvent {
	Uint32 type;
	Uint32 timestamp;
	SDL_JoystickID which;
	Uint8 button;
	Uint8 state;
	Uint8 padding1;
	Uint8 padding2;
} SDL_ControllerButtonEvent;

typedef struct SDL_ControllerAxisEvent {
	Uint32 type;
	Uint32 timestamp;
	SDL_JoystickID which;
	Uint8 axis;
	Sint16 value;
	Uint16 padding4;
} SDL_ControllerAxisEvent;

typedef struct SDL_JoyButtonEvent {
	Uint32 type;
	Uint32 timestamp;
	SDL_JoystickID which;
	Uint8 button;
	Uint8 state;
	Uint8 padding1;
	Uint8 padding2;
} SDL_JoyButtonEvent;

typedef struct SDL_JoyAxisEvent {
	Uint32 type;
	Uint32 timestamp;
	SDL_JoystickID which;
	Uint8 axis;
	Sint16 value;
	Uint16 padding4;
} SDL_JoyAxisEvent;

typedef struct SDL_JoyHatEvent {
	Uint32 type;
	Uint32 timestamp;
	SDL_JoystickID which;
	Uint8 hat;
	Uint8 value;
	Uint8 padding1;
	Uint8 padding2;
} SDL_JoyHatEvent;

typedef struct SDL_JoyDeviceEvent {
	Uint32 type;
	Uint32 timestamp;
	Sint32 which;
} SDL_JoyDeviceEvent;

#define SDL_TOUCH_MOUSEID ((Uint32)-1)

typedef struct SDL_MouseButtonEvent {
	Uint32 type;
	Uint32 timestamp;
	Uint32 windowID;
	Uint32 which;
	Uint8 button;
	Uint8 state;
	Uint8 clicks;
	Uint8 padding1;
	Sint32 x;
	Sint32 y;
} SDL_MouseButtonEvent;

typedef struct SDL_MouseMotionEvent {
	Uint32 type;
	Uint32 timestamp;
	Uint32 windowID;
	Uint32 which;
	Uint32 state;
	Sint32 x;
	Sint32 y;
	Sint32 xrel;
	Sint32 yrel;
} SDL_MouseMotionEvent;

typedef struct SDL_MouseWheelEvent {
	Uint32 type;
	Uint32 timestamp;
	Uint32 windowID;
	Uint32 which;
	Sint32 x;
	Sint32 y;
	Uint32 direction;
} SDL_MouseWheelEvent;

typedef struct SDL_TouchFingerEvent {
	Uint32 type;
	Uint32 timestamp;
	SDL_TouchID touchId;
	SDL_FingerID fingerId;
	float x;
	float y;
	float dx;
	float dy;
	float pressure;
	Uint32 windowID;
} SDL_TouchFingerEvent;

typedef struct SDL_DropEvent {
	Uint32 type;
	Uint32 timestamp;
	char* file;
	Uint32 windowID;
} SDL_DropEvent;

typedef union SDL_Event {
	Uint32 type;
	SDL_WindowEvent window;
	SDL_KeyboardEvent key;
	SDL_ControllerButtonEvent cbutton;
	SDL_ControllerAxisEvent caxis;
	SDL_JoyButtonEvent jbutton;
	SDL_JoyAxisEvent jaxis;
	SDL_JoyHatEvent jhat;
	SDL_JoyDeviceEvent jdevice;
	SDL_MouseButtonEvent button;
	SDL_MouseMotionEvent motion;
	SDL_MouseWheelEvent wheel;
	SDL_TouchFingerEvent tfinger;
	SDL_DropEvent drop;
} SDL_Event;

typedef enum SDL_GameControllerButton {
	SDL_CONTROLLER_BUTTON_INVALID = -1,
	SDL_CONTROLLER_BUTTON_A = 0,
	SDL_CONTROLLER_BUTTON_B,
	SDL_CONTROLLER_BUTTON_X,
	SDL_CONTROLLER_BUTTON_Y,
	SDL_CONTROLLER_BUTTON_BACK,
	SDL_CONTROLLER_BUTTON_GUIDE,
	SDL_CONTROLLER_BUTTON_START,
	SDL_CONTROLLER_BUTTON_LEFTSTICK,
	SDL_CONTROLLER_BUTTON_RIGHTSTICK,
	SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
	SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
	SDL_CONTROLLER_BUTTON_DPAD_UP,
	SDL_CONTROLLER_BUTTON_DPAD_DOWN,
	SDL_CONTROLLER_BUTTON_DPAD_LEFT,
	SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
	SDL_CONTROLLER_BUTTON_MAX
} SDL_GameControllerButton;

typedef enum SDL_GameControllerAxis {
	SDL_CONTROLLER_AXIS_INVALID = -1,
	SDL_CONTROLLER_AXIS_LEFTX = 0,
	SDL_CONTROLLER_AXIS_LEFTY,
	SDL_CONTROLLER_AXIS_RIGHTX,
	SDL_CONTROLLER_AXIS_RIGHTY,
	SDL_CONTROLLER_AXIS_TRIGGERLEFT,
	SDL_CONTROLLER_AXIS_TRIGGERRIGHT,
	SDL_CONTROLLER_AXIS_MAX
} SDL_GameControllerAxis;


enum {
	SDL_CONTROLLERAXISMOTION = 0x650,
	SDL_CONTROLLERBUTTONDOWN,
	SDL_CONTROLLERBUTTONUP,
	SDL_JOYAXISMOTION = 0x600,
	SDL_JOYBUTTONDOWN,
	SDL_JOYBUTTONUP,
	SDL_JOYDEVICEADDED,
	SDL_JOYDEVICEREMOVED,
	SDL_JOYHATMOTION,
	SDL_KEYDOWN = 0x300,
	SDL_KEYUP,
	SDL_MOUSEMOTION = 0x400,
	SDL_MOUSEBUTTONDOWN,
	SDL_MOUSEBUTTONUP,
	SDL_MOUSEWHEEL,
	SDL_FINGERDOWN = 0x700,
	SDL_FINGERUP,
	SDL_FINGERMOTION,
	SDL_DROPFILE = 0x1000,
	SDL_CLIPBOARDUPDATE = 0x900,
	SDL_QUIT = 0x100,
	SDL_APP_WILLENTERBACKGROUND = 0x103,
	SDL_APP_DIDENTERBACKGROUND = 0x104,
	SDL_APP_WILLENTERFOREGROUND = 0x105,
	SDL_APP_DIDENTERFOREGROUND = 0x106,
	SDL_WINDOWEVENT = 0x200
};

#define SDL_GETEVENT 0

enum {
	SDL_WINDOWEVENT_FOCUS_GAINED = 1,
	SDL_WINDOWEVENT_MINIMIZED,
	SDL_WINDOWEVENT_RESTORED,
	SDL_WINDOWEVENT_MOVED,
	SDL_WINDOWEVENT_ENTER,
	SDL_WINDOWEVENT_LEAVE,
	SDL_WINDOWEVENT_FOCUS_LOST,
	SDL_WINDOWEVENT_CLOSE,
	SDL_WINDOWEVENT_SIZE_CHANGED = 9,
	SDL_WINDOWEVENT_RESIZED = 10
};

#define SDL_BUTTON_LEFT 1
#define SDL_BUTTON_MIDDLE 2
#define SDL_BUTTON_RIGHT 3
#define SDL_BUTTON_X1 4
#define SDL_BUTTON_X2 5

#define SDL_HAT_UP 0x01
#define SDL_HAT_RIGHT 0x02
#define SDL_HAT_DOWN 0x04
#define SDL_HAT_LEFT 0x08

#define SDL_PIXELFORMAT_ARGB8888 0x16362004u
#define SDL_PIXELFORMAT_RGBA8888 0x16462004u
#define SDL_PIXELFORMAT_ABGR8888 0x16762004u
#define SDL_PIXELFORMAT_BGRA8888 0x16862004u
#define SDL_PIXELFORMAT_RGB565 0x15151002u
#define SDL_PIXELFORMAT_RGB555 0x15141002u

#if SDL_BYTEORDER == 1234
#define SDL_PIXELFORMAT_RGBA32 SDL_PIXELFORMAT_ABGR8888
#define SDL_PIXELFORMAT_ARGB32 SDL_PIXELFORMAT_BGRA8888
#define SDL_PIXELFORMAT_BGRA32 SDL_PIXELFORMAT_ARGB8888
#define SDL_PIXELFORMAT_ABGR32 SDL_PIXELFORMAT_RGBA8888
#else
#define SDL_PIXELFORMAT_RGBA32 SDL_PIXELFORMAT_RGBA8888
#define SDL_PIXELFORMAT_ARGB32 SDL_PIXELFORMAT_ARGB8888
#define SDL_PIXELFORMAT_BGRA32 SDL_PIXELFORMAT_BGRA8888
#define SDL_PIXELFORMAT_ABGR32 SDL_PIXELFORMAT_ABGR8888
#endif

#define SDL_PREALLOC 0x00000001u

#define SDL_RENDERER_ACCELERATED 0x00000002u
#define SDL_RENDERER_PRESENTVSYNC 0x00000004u

#define SDL_TEXTUREACCESS_STREAMING 1

#define SDL_WINDOW_FULLSCREEN 0x00000001u
#define SDL_WINDOW_OPENGL 0x00000002u
#define SDL_WINDOW_SHOWN 0x00000004u
#define SDL_WINDOW_HIDDEN 0x00000008u
#define SDL_WINDOW_BORDERLESS 0x00000010u
#define SDL_WINDOW_RESIZABLE 0x00000020u
#define SDL_WINDOW_MINIMIZED 0x00000040u
#define SDL_WINDOW_INPUT_FOCUS 0x00000200u
#define SDL_WINDOW_FULLSCREEN_DESKTOP 0x00001001u
#define SDL_WINDOW_ALLOW_HIGHDPI 0x00002000u
#define SDL_WINDOW_ALWAYS_ON_TOP 0x00008000u

#define SDL_SYSTEM_CURSOR_ARROW 0

#define SDL_MIX_MAXVOLUME 128
#define SDL_AUDIO_ALLOW_FORMAT_CHANGE 0x00000001u
#define SDL_INIT_EVERYTHING 0

#define SDL_HINT_ACCELEROMETER_AS_JOYSTICK "SDL_ACCELEROMETER_AS_JOYSTICK"
#define SDL_HINT_GRAB_KEYBOARD "SDL_GRAB_KEYBOARD"
#define SDL_HINT_RENDER_SCALE_QUALITY "SDL_RENDER_SCALE_QUALITY"
#define SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS "SDL_VIDEO_MINIMIZE_ON_FOCUS_LOSS"

#define SDL_GL_CONTEXT_PROFILE_COMPATIBILITY 0x0002
#define SDL_GL_CONTEXT_PROFILE_CORE 0x0001
#define SDL_GL_CONTEXT_PROFILE_ES 0x0004
#define SDL_GL_CONTEXT_FLAGS 0
#define SDL_GL_CONTEXT_PROFILE_MASK 0
#define SDL_GL_CONTEXT_MAJOR_VERSION 0
#define SDL_GL_CONTEXT_MINOR_VERSION 0
#define SDL_GL_DOUBLEBUFFER 0
#define SDL_GL_DEPTH_SIZE 0
#define SDL_GL_STENCIL_SIZE 0
#define SDL_GL_RED_SIZE 0
#define SDL_GL_GREEN_SIZE 0
#define SDL_GL_BLUE_SIZE 0
#define SDL_GL_ALPHA_SIZE 0

#define SDL_BYTESPERPIXEL(format) (((format) == SDL_PIXELFORMAT_RGB565 || (format) == SDL_PIXELFORMAT_RGB555) ? 2 : 4)
#define SDL_BITSPERPIXEL(format) (SDL_BYTESPERPIXEL(format) * 8)

#define SDL_zero(x) memset(&(x), 0, sizeof(x))

#define SDL_BlitSurface SDL_UpperBlit
#define SDL_mutexP(m) SDL_LockMutex(m)
#define SDL_mutexV(m) SDL_UnlockMutex(m)

int SDL_PointInRect(const SDL_Point* p, const SDL_Rect* r);
int SDL_IntersectRect(const SDL_Rect* a, const SDL_Rect* b, SDL_Rect* result);

const char* SDL_GetError(void);
void SDL_Log(const char* fmt, ...);

int SDL_atoi(const char* s);
double SDL_atof(const char* s);
size_t SDL_strlen(const char* s);
int SDL_strcmp(const char* a, const char* b);
int SDL_strncmp(const char* a, const char* b, size_t n);
int SDL_strcasecmp(const char* a, const char* b);
int SDL_strncasecmp(const char* a, const char* b, size_t n);
char* SDL_strchr(const char* s, int c);
char* SDL_strrchr(const char* s, int c);
char* SDL_strstr(const char* s, const char* f);
char* SDL_strdup(const char* s);
double SDL_strtod(const char* s, char** e);
long SDL_strtol(const char* s, char** e, int base);
Sint64 SDL_strtoll(const char* s, char** e, int base);
unsigned long SDL_strtoul(const char* s, char** e, int base);
int SDL_snprintf(char* s, size_t n, const char* fmt, ...);
int SDL_vsnprintf(char* s, size_t n, const char* fmt, va_list ap);
int SDL_tolower(int c);
int SDL_toupper(int c);
int SDL_isdigit(int c);
int SDL_isspace(int c);
void* SDL_memset(void* dst, int c, size_t n);
void SDL_free(void* p);

Uint64 SDL_GetPerformanceCounter(void);
Uint64 SDL_GetPerformanceFrequency(void);
Uint32 SDL_GetTicks(void);
void SDL_Delay(Uint32 ms);

void SDL_GetVersion(SDL_version* v);
const char* SDL_GetCurrentVideoDriver(void);
int SDL_GetNumVideoDisplays(void);
int SDL_GetDisplayBounds(int displayIndex, SDL_Rect* rect);
int SDL_GetDesktopDisplayMode(int displayIndex, SDL_DisplayMode* mode);
int SDL_GetCurrentDisplayMode(int displayIndex, SDL_DisplayMode* mode);
int SDL_GetDisplayMode(int displayIndex, int modeIndex, SDL_DisplayMode* mode);
int SDL_GetDisplayDPI(int displayIndex, float* ddpi, float* hdpi, float* vdpi);
const char* SDL_GetDisplayName(int displayIndex);
int SDL_GetNumDisplayModes(int displayIndex);
int SDL_GetWindowDisplayMode(SDL_Window* window, SDL_DisplayMode* mode);

SDL_mutex* SDL_CreateMutex(void);
void SDL_DestroyMutex(SDL_mutex* m);
int SDL_LockMutex(SDL_mutex* m);
int SDL_UnlockMutex(SDL_mutex* m);
int SDL_TryLockMutex(SDL_mutex* m);

SDL_cond* SDL_CreateCond(void);
int SDL_CondSignal(SDL_cond* c);
int SDL_CondWaitTimeout(SDL_cond* c, SDL_mutex* m, Uint32 ms);

SDL_sem* SDL_CreateSemaphore(Uint32 initial_value);
void SDL_DestroySemaphore(SDL_sem* s);
int SDL_SemPost(SDL_sem* s);
int SDL_SemWait(SDL_sem* s);
int SDL_SemTryWait(SDL_sem* s);
int SDL_SemWaitTimeout(SDL_sem* s, Uint32 ms);

SDL_Thread* SDL_CreateThread(SDL_ThreadFunction fn, const char* name, void* data);
void SDL_DetachThread(SDL_Thread* t);
void SDL_WaitThread(SDL_Thread* t, int* status);
SDL_threadID SDL_GetThreadID(SDL_Thread* t);
int SDL_SetThreadPriority(SDL_ThreadPriority pri);

SDL_Surface* SDL_CreateRGBSurfaceWithFormat(Uint32 flags, int w, int h, int depth, Uint32 format);
SDL_Surface* SDL_CreateRGBSurfaceWithFormatFrom(void* pixels, int w, int h, int depth, int pitch, Uint32 format);
SDL_Surface* SDL_CreateRGBSurface(Uint32 flags, int w, int h, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);
SDL_Surface* SDL_CreateRGBSurfaceFrom(void* pixels, int w, int h, int depth, int pitch, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);
void SDL_FreeSurface(SDL_Surface* s);
SDL_PixelFormat* SDL_AllocFormat(Uint32 format);
void SDL_FreeFormat(SDL_PixelFormat* fmt);
int SDL_UpperBlit(SDL_Surface* src, const SDL_Rect* sr, SDL_Surface* dst, SDL_Rect* dr);
int SDL_FillRect(SDL_Surface* dst, const SDL_Rect* rect, Uint32 color);

SDL_Texture* SDL_CreateTexture(SDL_Renderer* renderer, Uint32 format, int access, int w, int h);
SDL_Texture* SDL_CreateTextureFromSurface(SDL_Renderer* renderer, SDL_Surface* surface);
void SDL_DestroyTexture(SDL_Texture* texture);
int SDL_QueryTexture(SDL_Texture* texture, Uint32* format, int* access, int* w, int* h);
int SDL_UpdateTexture(SDL_Texture* texture, const SDL_Rect* rect, const void* pixels, int pitch);
int SDL_LockTexture(SDL_Texture* texture, const SDL_Rect* rect, void** pixels, int* pitch);
void SDL_UnlockTexture(SDL_Texture* texture);
int SDL_RenderCopy(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect* sr, const SDL_Rect* dr);
int SDL_RenderCopyEx(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect* sr, const SDL_Rect* dr, const double angle, const SDL_Point* center, const SDL_RendererFlip flip);
int SDL_RenderFillRect(SDL_Renderer* renderer, const SDL_Rect* rect);
int SDL_RenderSetLogicalSize(SDL_Renderer* renderer, int w, int h);
int SDL_RenderSetIntegerScale(SDL_Renderer* renderer, SDL_bool enable);
int SDL_RenderSetScale(SDL_Renderer* renderer, float scaleX, float scaleY);
void SDL_RenderGetLogicalSize(SDL_Renderer* renderer, int* w, int* h);
int SDL_GetRendererOutputSize(SDL_Renderer* renderer, int* w, int* h);
void SDL_RenderPresent(SDL_Renderer* renderer);
int SDL_RenderClear(SDL_Renderer* renderer);
void SDL_DestroyRenderer(SDL_Renderer* renderer);
int SDL_SetRenderDrawColor(SDL_Renderer* renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
int SDL_GetRenderDrawColor(SDL_Renderer* renderer, Uint8* r, Uint8* g, Uint8* b, Uint8* a);
int SDL_SetRenderDrawBlendMode(SDL_Renderer* renderer, SDL_BlendMode blendMode);
int SDL_SetSurfaceBlendMode(SDL_Surface* surface, SDL_BlendMode blendMode);
int SDL_SetTextureBlendMode(SDL_Texture* texture, SDL_BlendMode blendMode);
int SDL_SetTextureAlphaMod(SDL_Texture* texture, Uint8 alpha);

SDL_Window* SDL_CreateWindow(const char* title, int x, int y, int w, int h, Uint32 flags);
SDL_Renderer* SDL_CreateRenderer(SDL_Window* window, int index, Uint32 flags);
SDL_Window* SDL_GetWindowFromID(Uint32 id);
Uint32 SDL_GetWindowFlags(SDL_Window* window);
void SDL_GetWindowPosition(SDL_Window* window, int* x, int* y);
void SDL_GetWindowSize(SDL_Window* window, int* w, int* h);
void SDL_SetWindowGrab(SDL_Window* window, SDL_bool grabbed);
void SDL_SetWindowSize(SDL_Window* window, int w, int h);
void SDL_SetWindowPosition(SDL_Window* window, int x, int y);
void SDL_RaiseWindow(SDL_Window* window);
void SDL_MinimizeWindow(SDL_Window* window);
void SDL_DestroyWindow(SDL_Window* window);
void SDL_SetWindowIcon(SDL_Window* window, SDL_Surface* icon);

SDL_Window* SDL_GetKeyboardFocus(void);
const Uint8* SDL_GetKeyboardState(int* numkeys);
SDL_Keycode SDL_GetKeyFromScancode(SDL_Scancode scancode);
SDL_Scancode SDL_GetScancodeFromName(const char* name);
SDL_Keymod SDL_GetModState(void);

SDL_Cursor* SDL_GetCursor(void);
int SDL_ShowCursor(int toggle);
SDL_Cursor* SDL_CreateColorCursor(SDL_Surface* surface, int hot_x, int hot_y);
SDL_Cursor* SDL_CreateSystemCursor(int id);
void SDL_FreeCursor(SDL_Cursor* cursor);
void SDL_SetCursor(SDL_Cursor* cursor);
int SDL_WarpMouseGlobal(int x, int y);
void SDL_WarpMouseInWindow(SDL_Window* window, int x, int y);
int SDL_SetRelativeMouseMode(SDL_bool enabled);

SDL_bool SDL_HasClipboardText(void);
char* SDL_GetClipboardText(void);
int SDL_SetClipboardText(const char* text);

SDL_bool SDL_SetHint(const char* name, const char* value);
int SDL_GL_SetAttribute(SDL_GLattr attr, int value);
void SDL_GL_ResetAttributes(void);

int SDL_PushEvent(SDL_Event* event);
int SDL_PollEvent(SDL_Event* event);
void SDL_PumpEvents(void);
int SDL_PeepEvents(SDL_Event* events, int numevents, int action, Uint32 minType, Uint32 maxType);

SDL_GameController* SDL_GameControllerOpen(int joystick_index);
void SDL_GameControllerClose(SDL_GameController* gamecontroller);
SDL_Joystick* SDL_GameControllerGetJoystick(SDL_GameController* gamecontroller);
const char* SDL_GameControllerName(SDL_GameController* gamecontroller);
const char* SDL_GameControllerNameForIndex(int joystick_index);
char* SDL_GameControllerMapping(SDL_GameController* gamecontroller);
const char* SDL_GameControllerGetStringForAxis(SDL_GameControllerAxis axis);
const char* SDL_GameControllerGetStringForButton(SDL_GameControllerButton button);
Uint8 SDL_GameControllerGetButton(SDL_GameController* gamecontroller, SDL_GameControllerButton button);
SDL_GameControllerButton SDL_GameControllerGetButtonFromString(const char* str);
int SDL_GameControllerAddMappingsFromFile(const char* path);
SDL_bool SDL_IsGameController(int joystick_index);

SDL_Joystick* SDL_JoystickOpen(int device_index);
void SDL_JoystickClose(SDL_Joystick* joystick);
const char* SDL_JoystickName(SDL_Joystick* joystick);
const char* SDL_JoystickNameForIndex(int device_index);
SDL_JoystickID SDL_JoystickInstanceID(SDL_Joystick* joystick);
SDL_JoystickGUID SDL_JoystickGetGUID(SDL_Joystick* joystick);
void SDL_JoystickGetGUIDString(SDL_JoystickGUID guid, char* pszGUID, int cbGUID);
int SDL_JoystickNumAxes(SDL_Joystick* joystick);
int SDL_JoystickNumBalls(SDL_Joystick* joystick);
int SDL_JoystickNumButtons(SDL_Joystick* joystick);
int SDL_JoystickNumHats(SDL_Joystick* joystick);
Sint16 SDL_JoystickGetAxis(SDL_Joystick* joystick, int axis);
Uint8 SDL_JoystickGetButton(SDL_Joystick* joystick, int button);
int SDL_NumJoysticks(void);

const char* SDL_GetAudioDeviceName(int index, int iscapture);
SDL_AudioDeviceID SDL_OpenAudioDevice(const char* device, int iscapture, const SDL_AudioSpec* desired, SDL_AudioSpec* obtained, int allowed_changes);
void SDL_CloseAudioDevice(SDL_AudioDeviceID dev);
void SDL_PauseAudioDevice(SDL_AudioDeviceID dev, int pause_on);
int SDL_QueueAudio(SDL_AudioDeviceID dev, const void* data, Uint32 len);
Uint32 SDL_DequeueAudio(SDL_AudioDeviceID dev, void* data, Uint32 len);
Uint32 SDL_GetQueuedAudioSize(SDL_AudioDeviceID dev);
void SDL_LockAudioDevice(SDL_AudioDeviceID dev);
void SDL_UnlockAudioDevice(SDL_AudioDeviceID dev);
int SDL_GetNumAudioDevices(int iscapture);

int SDL_Init(Uint32 flags);
void SDL_Quit(void);

Uint32 SDL_GetWindowID(SDL_Window* window);

SDL_TimerID SDL_AddTimer(Uint32 interval, Uint32 (*callback)(Uint32, void*), void* param);
SDL_bool SDL_RemoveTimer(SDL_TimerID id);

SDL_AudioSpec* SDL_LoadWAV(const char* file, SDL_AudioSpec* spec, Uint8** audio_buf, Uint32* audio_len);
void SDL_FreeWAV(Uint8* audio_buf);

#ifdef __cplusplus
}
#endif

#endif
