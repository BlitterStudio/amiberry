#ifndef LIBRETRO_SDL_COMPAT_H
#define LIBRETRO_SDL_COMPAT_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SDLCALL
#define SDLCALL
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
typedef Uint32 SDL_WindowID;
typedef Uint32 SDL_DisplayID;
typedef Uint32 SDL_KeyboardID;
typedef Uint32 SDL_MouseID;

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

#define SDL_KMOD_NONE KMOD_NONE
#define SDL_KMOD_CAPS KMOD_CAPS
#define SDL_KMOD_NUM KMOD_NUM

typedef struct SDL_version {
	Uint8 major;
	Uint8 minor;
	Uint8 patch;
} SDL_version;

#define SDL_VERSION(x) do { (x)->major = 3; (x)->minor = 0; (x)->patch = 0; } while (0)
#define SDL_VERSION_ATLEAST(x, y, z) 1
#define SDL_VERSIONNUM_MAJOR(v) (((v) >> 24) & 0xFF)
#define SDL_VERSIONNUM_MINOR(v) (((v) >> 16) & 0xFF)
#define SDL_VERSIONNUM_MICRO(v) (((v) >> 8) & 0xFF)

typedef struct SDL_Point {
	int x;
	int y;
} SDL_Point;

typedef struct SDL_FPoint {
	float x;
	float y;
} SDL_FPoint;

typedef struct SDL_Rect {
	int x;
	int y;
	int w;
	int h;
} SDL_Rect;

typedef struct SDL_FRect {
	float x;
	float y;
	float w;
	float h;
} SDL_FRect;

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
	SDL_DisplayID displayID;
	Uint32 format;
	int w;
	int h;
	float pixel_density;
	float refresh_rate;
	int refresh_rate_numerator;
	int refresh_rate_denominator;
	void* internal;
} SDL_DisplayMode;

typedef Uint32 SDL_PixelFormat;

typedef struct SDL_PixelFormatDetails {
	SDL_PixelFormat format;
	Uint8 bits_per_pixel;
	Uint8 bytes_per_pixel;
	Uint8 padding[2];
	Uint32 Rmask;
	Uint32 Gmask;
	Uint32 Bmask;
	Uint32 Amask;
	Uint8 Rbits;
	Uint8 Gbits;
	Uint8 Bbits;
	Uint8 Abits;
	Uint8 Rshift;
	Uint8 Gshift;
	Uint8 Bshift;
	Uint8 Ashift;
} SDL_PixelFormatDetails;

typedef struct SDL_Surface {
	Uint32 flags;
	SDL_PixelFormat format;
	int w;
	int h;
	int pitch;
	void* pixels;
	void* reserved;
} SDL_Surface;

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_Cursor SDL_Cursor;
typedef struct SDL_Gamepad SDL_Gamepad;
typedef struct SDL_Joystick SDL_Joystick;
typedef struct SDL_Thread SDL_Thread;
typedef struct SDL_Mutex SDL_Mutex;
typedef struct SDL_Condition SDL_Condition;
typedef struct SDL_Semaphore SDL_Semaphore;
typedef struct SDL_AudioStream SDL_AudioStream;

typedef SDL_Mutex SDL_mutex;
typedef SDL_Condition SDL_cond;
typedef SDL_Semaphore SDL_semaphore;
typedef SDL_Semaphore SDL_sem;
typedef SDL_Gamepad SDL_GameController;

typedef struct SDL_JoystickGUID {
	Uint8 data[16];
} SDL_JoystickGUID;

typedef Sint32 SDL_JoystickID;
typedef Uint64 SDL_ThreadID;
typedef SDL_ThreadID SDL_threadID;

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
	SDL_FLIP_NONE = 0,
	SDL_FLIP_HORIZONTAL = 1,
	SDL_FLIP_VERTICAL = 2
} SDL_RendererFlip;

typedef Uint16 SDL_AudioFormat;
typedef void (*SDL_AudioCallback)(void*, Uint8*, int);
typedef void (*SDL_AudioStreamCallback)(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount);

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
#define SDL_AUDIO_S16 SDL_AUDIO_S16LE
#else
#define AUDIO_U16SYS AUDIO_U16MSB
#define AUDIO_S16SYS AUDIO_S16MSB
#define SDL_AUDIO_S16 SDL_AUDIO_S16BE
#endif

#define SDL_AUDIO_S16LE 0x8010
#define SDL_AUDIO_S16BE 0x9010
#define SDL_AUDIO_U8 0x0008

#define SDL_AUDIO_ALLOW_FORMAT_CHANGE 0x00000001u
#define SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK 0
#define SDL_AUDIO_DEVICE_DEFAULT_RECORDING 1

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

#define SDL_SCANCODE_MEDIA_STOP SDL_SCANCODE_AUDIOSTOP
#define SDL_SCANCODE_MEDIA_PLAY SDL_SCANCODE_AUDIOPLAY
#define SDL_SCANCODE_MEDIA_PREVIOUS_TRACK SDL_SCANCODE_AUDIOPREV
#define SDL_SCANCODE_MEDIA_NEXT_TRACK SDL_SCANCODE_AUDIONEXT

#define SDL_SCANCODE_COUNT 512
#define SDL_NUM_SCANCODES SDL_SCANCODE_COUNT

typedef struct SDL_Keysym {
	SDL_Scancode scancode;
	SDL_Keycode sym;
	SDL_Keymod mod;
	Uint32 unused;
} SDL_Keysym;

typedef struct SDL_WindowEvent {
	Uint32 type;
	Uint64 timestamp;
	SDL_WindowID windowID;
	Sint32 data1;
	Sint32 data2;
} SDL_WindowEvent;

typedef struct SDL_KeyboardEvent {
	Uint32 type;
	Uint64 timestamp;
	SDL_WindowID windowID;
	SDL_KeyboardID which;
	SDL_Scancode scancode;
	SDL_Keycode key;
	SDL_Keymod mod;
	Uint16 raw;
	SDL_bool down;
	SDL_bool repeat;
} SDL_KeyboardEvent;

typedef struct SDL_GamepadButtonEvent {
	Uint32 type;
	Uint64 timestamp;
	SDL_JoystickID which;
	Uint8 button;
	Uint8 down;
	Uint8 padding1;
	Uint8 padding2;
} SDL_GamepadButtonEvent;

typedef struct SDL_GamepadAxisEvent {
	Uint32 type;
	Uint64 timestamp;
	SDL_JoystickID which;
	Uint8 axis;
	Sint16 value;
	Uint16 padding;
} SDL_GamepadAxisEvent;

typedef struct SDL_JoyButtonEvent {
	Uint32 type;
	Uint64 timestamp;
	SDL_JoystickID which;
	Uint8 button;
	Uint8 down;
	Uint8 padding1;
	Uint8 padding2;
} SDL_JoyButtonEvent;

typedef struct SDL_JoyAxisEvent {
	Uint32 type;
	Uint64 timestamp;
	SDL_JoystickID which;
	Uint8 axis;
	Sint16 value;
	Uint16 padding;
} SDL_JoyAxisEvent;

typedef struct SDL_JoyHatEvent {
	Uint32 type;
	Uint64 timestamp;
	SDL_JoystickID which;
	Uint8 hat;
	Uint8 value;
	Uint8 padding1;
	Uint8 padding2;
} SDL_JoyHatEvent;

typedef struct SDL_JoyDeviceEvent {
	Uint32 type;
	Uint64 timestamp;
	Sint32 which;
} SDL_JoyDeviceEvent;

#define SDL_TOUCH_MOUSEID ((SDL_MouseID)-1)

typedef struct SDL_MouseButtonEvent {
	Uint32 type;
	Uint64 timestamp;
	SDL_WindowID windowID;
	SDL_MouseID which;
	Uint8 button;
	Uint8 down;
	Uint8 clicks;
	Uint8 padding;
	float x;
	float y;
} SDL_MouseButtonEvent;

typedef struct SDL_MouseMotionEvent {
	Uint32 type;
	Uint64 timestamp;
	SDL_WindowID windowID;
	SDL_MouseID which;
	Uint32 state;
	float x;
	float y;
	float xrel;
	float yrel;
} SDL_MouseMotionEvent;

typedef struct SDL_MouseWheelEvent {
	Uint32 type;
	Uint64 timestamp;
	SDL_WindowID windowID;
	SDL_MouseID which;
	float x;
	float y;
	Uint32 direction;
} SDL_MouseWheelEvent;

typedef struct SDL_TouchFingerEvent {
	Uint32 type;
	Uint64 timestamp;
	SDL_TouchID touchID;
	SDL_FingerID fingerID;
	float x;
	float y;
	float dx;
	float dy;
	float pressure;
	SDL_WindowID windowID;
} SDL_TouchFingerEvent;

typedef struct SDL_DropEvent {
	Uint32 type;
	Uint64 timestamp;
	char* data;
	SDL_WindowID windowID;
} SDL_DropEvent;

typedef union SDL_Event {
	Uint32 type;
	SDL_WindowEvent window;
	SDL_KeyboardEvent key;
	SDL_GamepadButtonEvent gbutton;
	SDL_GamepadAxisEvent gaxis;
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

#define cbutton gbutton
#define caxis gaxis

typedef enum SDL_GamepadButton {
	SDL_GAMEPAD_BUTTON_INVALID = -1,
	SDL_GAMEPAD_BUTTON_SOUTH = 0,
	SDL_GAMEPAD_BUTTON_EAST,
	SDL_GAMEPAD_BUTTON_WEST,
	SDL_GAMEPAD_BUTTON_NORTH,
	SDL_GAMEPAD_BUTTON_BACK,
	SDL_GAMEPAD_BUTTON_GUIDE,
	SDL_GAMEPAD_BUTTON_START,
	SDL_GAMEPAD_BUTTON_LEFT_STICK,
	SDL_GAMEPAD_BUTTON_RIGHT_STICK,
	SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,
	SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,
	SDL_GAMEPAD_BUTTON_DPAD_UP,
	SDL_GAMEPAD_BUTTON_DPAD_DOWN,
	SDL_GAMEPAD_BUTTON_DPAD_LEFT,
	SDL_GAMEPAD_BUTTON_DPAD_RIGHT,
	SDL_GAMEPAD_BUTTON_MISC1,
	SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1,
	SDL_GAMEPAD_BUTTON_LEFT_PADDLE1,
	SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2,
	SDL_GAMEPAD_BUTTON_LEFT_PADDLE2,
	SDL_GAMEPAD_BUTTON_TOUCHPAD,
	SDL_GAMEPAD_BUTTON_MISC2,
	SDL_GAMEPAD_BUTTON_MISC3,
	SDL_GAMEPAD_BUTTON_MISC4,
	SDL_GAMEPAD_BUTTON_MISC5,
	SDL_GAMEPAD_BUTTON_MISC6,
	SDL_GAMEPAD_BUTTON_COUNT
} SDL_GamepadButton;

typedef SDL_GamepadButton SDL_GameControllerButton;

typedef enum SDL_GamepadAxis {
	SDL_GAMEPAD_AXIS_INVALID = -1,
	SDL_GAMEPAD_AXIS_LEFTX = 0,
	SDL_GAMEPAD_AXIS_LEFTY,
	SDL_GAMEPAD_AXIS_RIGHTX,
	SDL_GAMEPAD_AXIS_RIGHTY,
	SDL_GAMEPAD_AXIS_LEFT_TRIGGER,
	SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,
	SDL_GAMEPAD_AXIS_COUNT
} SDL_GamepadAxis;

typedef SDL_GamepadAxis SDL_GameControllerAxis;

typedef enum SDL_GamepadBindingType {
	SDL_GAMEPAD_BINDTYPE_NONE = 0,
	SDL_GAMEPAD_BINDTYPE_BUTTON,
	SDL_GAMEPAD_BINDTYPE_AXIS,
	SDL_GAMEPAD_BINDTYPE_HAT
} SDL_GamepadBindingType;

typedef struct SDL_GamepadBinding {
	SDL_GamepadBindingType bindType;
	union {
		int button;
		struct {
			int axis;
			int axis_min;
			int axis_max;
		} axis;
		struct {
			int hat;
			int hat_mask;
		} hat;
	} value;
} SDL_GamepadBinding;

enum {
	SDL_EVENT_QUIT = 0x100,
	SDL_EVENT_WILL_ENTER_BACKGROUND = 0x103,
	SDL_EVENT_DID_ENTER_BACKGROUND = 0x104,
	SDL_EVENT_WILL_ENTER_FOREGROUND = 0x105,
	SDL_EVENT_DID_ENTER_FOREGROUND = 0x106,
	SDL_EVENT_WINDOW_FIRST = 0x200,
	SDL_EVENT_WINDOW_FOCUS_GAINED = 0x201,
	SDL_EVENT_WINDOW_FOCUS_LOST = 0x202,
	SDL_EVENT_WINDOW_MINIMIZED = 0x203,
	SDL_EVENT_WINDOW_RESTORED = 0x204,
	SDL_EVENT_WINDOW_MOVED = 0x205,
	SDL_EVENT_WINDOW_RESIZED = 0x206,
	SDL_EVENT_WINDOW_CLOSE_REQUESTED = 0x207,
	SDL_EVENT_WINDOW_MOUSE_ENTER = 0x208,
	SDL_EVENT_WINDOW_MOUSE_LEAVE = 0x209,
	SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED = 0x20A,
	SDL_EVENT_WINDOW_LAST = 0x2FF,
	SDL_EVENT_KEY_DOWN = 0x300,
	SDL_EVENT_KEY_UP,
	SDL_EVENT_MOUSE_MOTION = 0x400,
	SDL_EVENT_MOUSE_BUTTON_DOWN,
	SDL_EVENT_MOUSE_BUTTON_UP,
	SDL_EVENT_MOUSE_WHEEL,
	SDL_EVENT_JOYSTICK_AXIS_MOTION = 0x600,
	SDL_EVENT_JOYSTICK_BUTTON_DOWN,
	SDL_EVENT_JOYSTICK_BUTTON_UP,
	SDL_EVENT_JOYSTICK_ADDED,
	SDL_EVENT_JOYSTICK_REMOVED,
	SDL_EVENT_JOYSTICK_HAT_MOTION,
	SDL_EVENT_GAMEPAD_AXIS_MOTION = 0x650,
	SDL_EVENT_GAMEPAD_BUTTON_DOWN,
	SDL_EVENT_GAMEPAD_BUTTON_UP,
	SDL_EVENT_GAMEPAD_ADDED,
	SDL_EVENT_GAMEPAD_REMOVED,
	SDL_EVENT_FINGER_DOWN = 0x700,
	SDL_EVENT_FINGER_UP,
	SDL_EVENT_FINGER_MOTION,
	SDL_EVENT_CLIPBOARD_UPDATE = 0x900,
	SDL_EVENT_DROP_FILE = 0x1000
};

#define SDL_CONTROLLERAXISMOTION SDL_EVENT_GAMEPAD_AXIS_MOTION
#define SDL_CONTROLLERBUTTONDOWN SDL_EVENT_GAMEPAD_BUTTON_DOWN
#define SDL_CONTROLLERBUTTONUP SDL_EVENT_GAMEPAD_BUTTON_UP
#define SDL_JOYAXISMOTION SDL_EVENT_JOYSTICK_AXIS_MOTION
#define SDL_JOYBUTTONDOWN SDL_EVENT_JOYSTICK_BUTTON_DOWN
#define SDL_JOYBUTTONUP SDL_EVENT_JOYSTICK_BUTTON_UP
#define SDL_JOYDEVICEADDED SDL_EVENT_JOYSTICK_ADDED
#define SDL_JOYDEVICEREMOVED SDL_EVENT_JOYSTICK_REMOVED
#define SDL_JOYHATMOTION SDL_EVENT_JOYSTICK_HAT_MOTION
#define SDL_KEYDOWN SDL_EVENT_KEY_DOWN
#define SDL_KEYUP SDL_EVENT_KEY_UP
#define SDL_MOUSEMOTION SDL_EVENT_MOUSE_MOTION
#define SDL_MOUSEBUTTONDOWN SDL_EVENT_MOUSE_BUTTON_DOWN
#define SDL_MOUSEBUTTONUP SDL_EVENT_MOUSE_BUTTON_UP
#define SDL_MOUSEWHEEL SDL_EVENT_MOUSE_WHEEL
#define SDL_FINGERDOWN SDL_EVENT_FINGER_DOWN
#define SDL_FINGERUP SDL_EVENT_FINGER_UP
#define SDL_FINGERMOTION SDL_EVENT_FINGER_MOTION
#define SDL_DROPFILE SDL_EVENT_DROP_FILE
#define SDL_CLIPBOARDUPDATE SDL_EVENT_CLIPBOARD_UPDATE
#define SDL_QUIT SDL_EVENT_QUIT
#define SDL_APP_WILLENTERBACKGROUND SDL_EVENT_WILL_ENTER_BACKGROUND
#define SDL_APP_DIDENTERBACKGROUND SDL_EVENT_DID_ENTER_BACKGROUND
#define SDL_APP_WILLENTERFOREGROUND SDL_EVENT_WILL_ENTER_FOREGROUND
#define SDL_APP_DIDENTERFOREGROUND SDL_EVENT_DID_ENTER_FOREGROUND

#define SDL_GETEVENT 0

#define SDL_BUTTON_LEFT 1
#define SDL_BUTTON_MIDDLE 2
#define SDL_BUTTON_RIGHT 3
#define SDL_BUTTON_X1 4
#define SDL_BUTTON_X2 5

#define SDL_HAT_UP 0x01
#define SDL_HAT_RIGHT 0x02
#define SDL_HAT_DOWN 0x04
#define SDL_HAT_LEFT 0x08

#define SDL_PIXELFORMAT_UNKNOWN 0
#define SDL_PIXELFORMAT_ARGB8888 0x16362004u
#define SDL_PIXELFORMAT_RGBA8888 0x16462004u
#define SDL_PIXELFORMAT_ABGR8888 0x16762004u
#define SDL_PIXELFORMAT_BGRA8888 0x16862004u
#define SDL_PIXELFORMAT_RGB565 0x15151002u
#define SDL_PIXELFORMAT_RGB555 0x15141002u
#define SDL_PIXELFORMAT_XRGB1555 SDL_PIXELFORMAT_RGB555

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

typedef Uint64 SDL_WindowFlags;

#define SDL_WINDOW_FULLSCREEN 0x00000001u
#define SDL_WINDOW_OPENGL 0x00000002u
#define SDL_WINDOW_HIDDEN 0x00000008u
#define SDL_WINDOW_BORDERLESS 0x00000010u
#define SDL_WINDOW_RESIZABLE 0x00000020u
#define SDL_WINDOW_MINIMIZED 0x00000040u
#define SDL_WINDOW_INPUT_FOCUS 0x00000200u
#define SDL_WINDOW_FULLSCREEN_DESKTOP 0x00001001u
#define SDL_WINDOW_ALLOW_HIGHDPI 0x00002000u
#define SDL_WINDOW_HIGH_PIXEL_DENSITY SDL_WINDOW_ALLOW_HIGHDPI
#define SDL_WINDOW_ALWAYS_ON_TOP 0x00008000u

typedef enum SDL_SystemCursor {
	SDL_SYSTEM_CURSOR_ARROW = 0
} SDL_SystemCursor;

#define SDL_INIT_EVERYTHING 0
#define SDL_INIT_GAMEPAD 0x00002000u
#define SDL_INIT_GAMECONTROLLER SDL_INIT_GAMEPAD

#define SDL_HINT_ACCELEROMETER_AS_JOYSTICK "SDL_ACCELEROMETER_AS_JOYSTICK"
#define SDL_HINT_GRAB_KEYBOARD "SDL_GRAB_KEYBOARD"
#define SDL_HINT_RENDER_SCALE_QUALITY "SDL_RENDER_SCALE_QUALITY"
#define SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS "SDL_VIDEO_MINIMIZE_ON_FOCUS_LOSS"

#define SDL_GL_CONTEXT_PROFILE_COMPATIBILITY 0x0002
#define SDL_GL_CONTEXT_PROFILE_CORE 0x0001
#define SDL_GL_CONTEXT_PROFILE_ES 0x0004
#define SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG 0x0002
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

typedef enum SDL_LogicalPresentation {
	SDL_LOGICAL_PRESENTATION_DISABLED = 0,
	SDL_LOGICAL_PRESENTATION_LETTERBOX = 1,
	SDL_LOGICAL_PRESENTATION_OVERSCAN = 2,
	SDL_LOGICAL_PRESENTATION_STRETCH = 3,
	SDL_LOGICAL_PRESENTATION_INTEGER_SCALE = 4
} SDL_LogicalPresentation;

typedef enum SDL_ScaleMode {
	SDL_SCALEMODE_NEAREST = 0,
	SDL_SCALEMODE_LINEAR = 1
} SDL_ScaleMode;

#define SDL_BYTESPERPIXEL(format) (((format) == SDL_PIXELFORMAT_RGB565 || (format) == SDL_PIXELFORMAT_RGB555 || (format) == SDL_PIXELFORMAT_XRGB1555) ? 2 : 4)
#define SDL_BITSPERPIXEL(format) (SDL_BYTESPERPIXEL(format) * 8)

#define SDL_zero(x) memset(&(x), 0, sizeof(x))

#define SDL_mutexP(m) SDL_LockMutex(m)
#define SDL_mutexV(m) SDL_UnlockMutex(m)

#define SDL_CONTROLLER_BUTTON_INVALID SDL_GAMEPAD_BUTTON_INVALID
#define SDL_CONTROLLER_BUTTON_A SDL_GAMEPAD_BUTTON_SOUTH
#define SDL_CONTROLLER_BUTTON_B SDL_GAMEPAD_BUTTON_EAST
#define SDL_CONTROLLER_BUTTON_X SDL_GAMEPAD_BUTTON_WEST
#define SDL_CONTROLLER_BUTTON_Y SDL_GAMEPAD_BUTTON_NORTH
#define SDL_CONTROLLER_BUTTON_BACK SDL_GAMEPAD_BUTTON_BACK
#define SDL_CONTROLLER_BUTTON_GUIDE SDL_GAMEPAD_BUTTON_GUIDE
#define SDL_CONTROLLER_BUTTON_START SDL_GAMEPAD_BUTTON_START
#define SDL_CONTROLLER_BUTTON_LEFTSTICK SDL_GAMEPAD_BUTTON_LEFT_STICK
#define SDL_CONTROLLER_BUTTON_RIGHTSTICK SDL_GAMEPAD_BUTTON_RIGHT_STICK
#define SDL_CONTROLLER_BUTTON_LEFTSHOULDER SDL_GAMEPAD_BUTTON_LEFT_SHOULDER
#define SDL_CONTROLLER_BUTTON_RIGHTSHOULDER SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER
#define SDL_CONTROLLER_BUTTON_DPAD_UP SDL_GAMEPAD_BUTTON_DPAD_UP
#define SDL_CONTROLLER_BUTTON_DPAD_DOWN SDL_GAMEPAD_BUTTON_DPAD_DOWN
#define SDL_CONTROLLER_BUTTON_DPAD_LEFT SDL_GAMEPAD_BUTTON_DPAD_LEFT
#define SDL_CONTROLLER_BUTTON_DPAD_RIGHT SDL_GAMEPAD_BUTTON_DPAD_RIGHT
#define SDL_CONTROLLER_BUTTON_MAX SDL_GAMEPAD_BUTTON_COUNT

#define SDL_CONTROLLER_AXIS_INVALID SDL_GAMEPAD_AXIS_INVALID
#define SDL_CONTROLLER_AXIS_LEFTX SDL_GAMEPAD_AXIS_LEFTX
#define SDL_CONTROLLER_AXIS_LEFTY SDL_GAMEPAD_AXIS_LEFTY
#define SDL_CONTROLLER_AXIS_RIGHTX SDL_GAMEPAD_AXIS_RIGHTX
#define SDL_CONTROLLER_AXIS_RIGHTY SDL_GAMEPAD_AXIS_RIGHTY
#define SDL_CONTROLLER_AXIS_TRIGGERLEFT SDL_GAMEPAD_AXIS_LEFT_TRIGGER
#define SDL_CONTROLLER_AXIS_TRIGGERRIGHT SDL_GAMEPAD_AXIS_RIGHT_TRIGGER
#define SDL_CONTROLLER_AXIS_MAX SDL_GAMEPAD_AXIS_COUNT

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
size_t SDL_strlcpy(char* dst, const char* src, size_t dstsize);
size_t SDL_strlcat(char* dst, const char* src, size_t dstsize);
int SDL_snprintf(char* s, size_t n, const char* fmt, ...);
int SDL_vsnprintf(char* s, size_t n, const char* fmt, va_list ap);
int SDL_tolower(int c);
int SDL_toupper(int c);
int SDL_isdigit(int c);
int SDL_isspace(int c);
void* SDL_memset(void* dst, int c, size_t n);
char* SDL_getenv(const char* name);
void SDL_free(void* p);

Uint64 SDL_GetPerformanceCounter(void);
Uint64 SDL_GetPerformanceFrequency(void);
Uint64 SDL_GetTicks(void);
void SDL_Delay(Uint32 ms);

int SDL_GetVersion(void);
const char* SDL_GetPlatform(void);
const char* SDL_GetCurrentVideoDriver(void);
int SDL_GetNumVideoDisplays(void);
SDL_DisplayID* SDL_GetDisplays(int* count);
SDL_DisplayID SDL_GetPrimaryDisplay(void);
bool SDL_GetDisplayBounds(SDL_DisplayID displayID, SDL_Rect* rect);
const SDL_DisplayMode* SDL_GetDesktopDisplayMode(SDL_DisplayID displayID);
const SDL_DisplayMode* SDL_GetCurrentDisplayMode(SDL_DisplayID displayID);
const SDL_DisplayMode* SDL_GetDisplayMode(SDL_DisplayID displayID, int modeIndex);
const SDL_DisplayMode* const* SDL_GetFullscreenDisplayModes(SDL_DisplayID displayID, int* count);
bool SDL_GetDisplayDPI(SDL_DisplayID displayID, float* ddpi, float* hdpi, float* vdpi);
float SDL_GetDisplayContentScale(SDL_DisplayID displayID);
const char* SDL_GetDisplayName(SDL_DisplayID displayID);
int SDL_GetNumDisplayModes(SDL_DisplayID displayID);
bool SDL_GetWindowDisplayMode(SDL_Window* window, SDL_DisplayMode* mode);
SDL_DisplayID SDL_GetDisplayForWindow(SDL_Window* window);
bool SDL_GetClosestFullscreenDisplayMode(SDL_DisplayID displayID, int w, int h, float refresh_rate, bool include_high_density_modes, SDL_DisplayMode* closest);

SDL_Mutex* SDL_CreateMutex(void);
void SDL_DestroyMutex(SDL_Mutex* m);
void SDL_LockMutex(SDL_Mutex* m);
void SDL_UnlockMutex(SDL_Mutex* m);
bool SDL_TryLockMutex(SDL_Mutex* m);

SDL_Condition* SDL_CreateCondition(void);
void SDL_SignalCondition(SDL_Condition* c);
bool SDL_WaitConditionTimeout(SDL_Condition* c, SDL_Mutex* m, Sint32 ms);

SDL_Semaphore* SDL_CreateSemaphore(Uint32 initial_value);
void SDL_DestroySemaphore(SDL_Semaphore* s);
void SDL_SignalSemaphore(SDL_Semaphore* s);
void SDL_WaitSemaphore(SDL_Semaphore* s);
bool SDL_TryWaitSemaphore(SDL_Semaphore* s);
bool SDL_WaitSemaphoreTimeout(SDL_Semaphore* s, Sint32 ms);
Uint32 SDL_GetSemaphoreValue(SDL_Semaphore* s);

SDL_Thread* SDL_CreateThread(SDL_ThreadFunction fn, const char* name, void* data);
void SDL_DetachThread(SDL_Thread* t);
void SDL_WaitThread(SDL_Thread* t, int* status);
SDL_ThreadID SDL_GetThreadID(SDL_Thread* t);
int SDL_SetThreadPriority(SDL_ThreadPriority pri);
int SDL_SetCurrentThreadPriority(SDL_ThreadPriority pri);

SDL_Surface* SDL_CreateSurface(int w, int h, SDL_PixelFormat format);
SDL_Surface* SDL_CreateSurfaceFrom(int w, int h, SDL_PixelFormat format, void* pixels, int pitch);
void SDL_DestroySurface(SDL_Surface* s);
const SDL_PixelFormatDetails* SDL_GetPixelFormatDetails(SDL_PixelFormat format);
bool SDL_BlitSurface(SDL_Surface* src, const SDL_Rect* sr, SDL_Surface* dst, const SDL_Rect* dr);
bool SDL_FillSurfaceRect(SDL_Surface* dst, const SDL_Rect* rect, Uint32 color);

SDL_Texture* SDL_CreateTexture(SDL_Renderer* renderer, Uint32 format, int access, int w, int h);
SDL_Texture* SDL_CreateTextureFromSurface(SDL_Renderer* renderer, SDL_Surface* surface);
void SDL_DestroyTexture(SDL_Texture* texture);
bool SDL_QueryTexture(SDL_Texture* texture, Uint32* format, int* access, int* w, int* h);
bool SDL_GetTextureSize(SDL_Texture* texture, float* w, float* h);
bool SDL_UpdateTexture(SDL_Texture* texture, const SDL_Rect* rect, const void* pixels, int pitch);
bool SDL_LockTexture(SDL_Texture* texture, const SDL_Rect* rect, void** pixels, int* pitch);
void SDL_UnlockTexture(SDL_Texture* texture);
bool SDL_RenderTexture(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_FRect* sr, const SDL_FRect* dr);
bool SDL_RenderTextureRotated(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_FRect* sr, const SDL_FRect* dr, double angle, const SDL_FPoint* center, SDL_RendererFlip flip);
bool SDL_RenderFillRect(SDL_Renderer* renderer, const SDL_FRect* rect);
bool SDL_SetRenderLogicalPresentation(SDL_Renderer* renderer, int w, int h, SDL_LogicalPresentation mode);
bool SDL_GetRenderLogicalPresentation(SDL_Renderer* renderer, int* w, int* h, SDL_LogicalPresentation* mode);
bool SDL_RenderSetIntegerScale(SDL_Renderer* renderer, SDL_bool enable);
bool SDL_RenderSetScale(SDL_Renderer* renderer, float scaleX, float scaleY);
bool SDL_SetRenderScale(SDL_Renderer* renderer, float scaleX, float scaleY);
void SDL_RenderGetLogicalSize(SDL_Renderer* renderer, int* w, int* h);
bool SDL_GetCurrentRenderOutputSize(SDL_Renderer* renderer, int* w, int* h);
bool SDL_RenderPresent(SDL_Renderer* renderer);
bool SDL_RenderClear(SDL_Renderer* renderer);
void SDL_DestroyRenderer(SDL_Renderer* renderer);
bool SDL_SetRenderDrawColor(SDL_Renderer* renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
bool SDL_GetRenderDrawColor(SDL_Renderer* renderer, Uint8* r, Uint8* g, Uint8* b, Uint8* a);
bool SDL_SetRenderDrawBlendMode(SDL_Renderer* renderer, SDL_BlendMode blendMode);
bool SDL_SetSurfaceBlendMode(SDL_Surface* surface, SDL_BlendMode blendMode);
bool SDL_SetTextureBlendMode(SDL_Texture* texture, SDL_BlendMode blendMode);
bool SDL_SetTextureAlphaMod(SDL_Texture* texture, Uint8 alpha);
bool SDL_SetTextureColorMod(SDL_Texture* texture, Uint8 r, Uint8 g, Uint8 b);
bool SDL_SetTextureScaleMode(SDL_Texture* texture, SDL_ScaleMode scaleMode);

SDL_Window* SDL_CreateWindow(const char* title, int w, int h, SDL_WindowFlags flags);
SDL_Renderer* SDL_CreateRenderer(SDL_Window* window, const char* name);
SDL_Window* SDL_GetWindowFromID(SDL_WindowID id);
SDL_WindowID SDL_GetWindowID(SDL_Window* window);
SDL_WindowFlags SDL_GetWindowFlags(SDL_Window* window);
void SDL_GetWindowPosition(SDL_Window* window, int* x, int* y);
void SDL_GetWindowSize(SDL_Window* window, int* w, int* h);
void SDL_SetWindowGrab(SDL_Window* window, SDL_bool grabbed);
bool SDL_SetWindowMouseGrab(SDL_Window* window, SDL_bool grabbed);
bool SDL_SetWindowKeyboardGrab(SDL_Window* window, SDL_bool grabbed);
bool SDL_SetWindowFullscreenMode(SDL_Window* window, const SDL_DisplayMode* mode);
bool SDL_SetWindowFullscreen(SDL_Window* window, bool fullscreen);
void SDL_SyncWindow(SDL_Window* window);
void SDL_GetWindowSizeInPixels(SDL_Window* window, int* w, int* h);
void SDL_SetWindowSize(SDL_Window* window, int w, int h);
void SDL_SetWindowPosition(SDL_Window* window, int x, int y);
void SDL_RaiseWindow(SDL_Window* window);
void SDL_MinimizeWindow(SDL_Window* window);
void SDL_DestroyWindow(SDL_Window* window);
void SDL_SetWindowIcon(SDL_Window* window, SDL_Surface* icon);

SDL_Window* SDL_GetKeyboardFocus(void);
const bool* SDL_GetKeyboardState(int* numkeys);
SDL_Keycode SDL_GetKeyFromScancode(SDL_Scancode scancode, ...);
SDL_Scancode SDL_GetScancodeFromName(const char* name);
SDL_Keymod SDL_GetModState(void);

SDL_Cursor* SDL_GetCursor(void);
void SDL_ShowCursor(void);
void SDL_HideCursor(void);
SDL_Cursor* SDL_CreateColorCursor(SDL_Surface* surface, int hot_x, int hot_y);
SDL_Cursor* SDL_CreateSystemCursor(SDL_SystemCursor id);
void SDL_DestroyCursor(SDL_Cursor* cursor);
void SDL_SetCursor(SDL_Cursor* cursor);
bool SDL_WarpMouseGlobal(float x, float y);
void SDL_WarpMouseInWindow(SDL_Window* window, float x, float y);
bool SDL_SetWindowRelativeMouseMode(SDL_Window* window, bool enabled);
bool SDL_GetWindowRelativeMouseMode(SDL_Window* window);

bool SDL_HasClipboardText(void);
char* SDL_GetClipboardText(void);
bool SDL_SetClipboardText(const char* text);

bool SDL_SetHint(const char* name, const char* value);
bool SDL_GL_SetAttribute(SDL_GLattr attr, int value);
void SDL_GL_ResetAttributes(void);

bool SDL_PushEvent(SDL_Event* event);
bool SDL_PollEvent(SDL_Event* event);
void SDL_PumpEvents(void);
int SDL_PeepEvents(SDL_Event* events, int numevents, int action, Uint32 minType, Uint32 maxType);

SDL_Gamepad* SDL_OpenGamepad(int joystick_index);
void SDL_CloseGamepad(SDL_Gamepad* gamepad);
SDL_Joystick* SDL_GetGamepadJoystick(SDL_Gamepad* gamepad);
const char* SDL_GetGamepadName(SDL_Gamepad* gamepad);
const char* SDL_GetGamepadNameForID(SDL_JoystickID joystick_id);
const char* SDL_GetGamepadNameForIndex(int joystick_index);
char* SDL_GetGamepadMapping(SDL_Gamepad* gamepad);
const char* SDL_GetGamepadStringForAxis(SDL_GamepadAxis axis);
const char* SDL_GetGamepadStringForButton(SDL_GamepadButton button);
Uint8 SDL_GetGamepadButton(SDL_Gamepad* gamepad, SDL_GamepadButton button);
SDL_GamepadButton SDL_GetGamepadButtonFromString(const char* str);
Sint16 SDL_GetGamepadAxis(SDL_Gamepad* gamepad, SDL_GamepadAxis axis);
bool SDL_AddGamepadMappingsFromFile(const char* path);
bool SDL_IsGamepad(int joystick_index);
SDL_Gamepad* SDL_GetGamepadFromID(SDL_JoystickID joystick_id);
bool SDL_GamepadConnected(SDL_Gamepad* gamepad);
const SDL_GamepadBinding* SDL_GetGamepadBindings(SDL_Gamepad* gamepad, int* count);

SDL_Joystick* SDL_OpenJoystick(int device_index);
void SDL_CloseJoystick(SDL_Joystick* joystick);
const char* SDL_GetJoystickName(SDL_Joystick* joystick);
const char* SDL_GetJoystickNameForID(SDL_JoystickID joystick_id);
const char* SDL_GetJoystickNameForIndex(int device_index);
SDL_JoystickID SDL_GetJoystickID(SDL_Joystick* joystick);
SDL_JoystickGUID SDL_GetJoystickGUID(SDL_Joystick* joystick);
void SDL_GetJoystickGUIDString(SDL_JoystickGUID guid, char* pszGUID, int cbGUID);
void SDL_GUIDToString(SDL_JoystickGUID guid, char* pszGUID, int cbGUID);
int SDL_GetNumJoystickAxes(SDL_Joystick* joystick);
int SDL_GetNumJoystickBalls(SDL_Joystick* joystick);
int SDL_GetNumJoystickButtons(SDL_Joystick* joystick);
int SDL_GetNumJoystickHats(SDL_Joystick* joystick);
Sint16 SDL_GetJoystickAxis(SDL_Joystick* joystick, int axis);
Uint8 SDL_GetJoystickButton(SDL_Joystick* joystick, int button);
int SDL_GetJoysticks(int** joysticks);
int SDL_NumJoysticks(void);

const char* SDL_GetAudioDeviceName(SDL_AudioDeviceID devid, ...);
SDL_AudioDeviceID* SDL_GetAudioPlaybackDevices(int* count);
SDL_AudioDeviceID* SDL_GetAudioRecordingDevices(int* count);
SDL_AudioStream* SDL_OpenAudioDeviceStream(SDL_AudioDeviceID devid, const SDL_AudioSpec* spec, SDL_AudioStreamCallback callback, void* userdata);
bool SDL_PutAudioStreamData(SDL_AudioStream* stream, const void* buf, int len);
int SDL_GetAudioStreamData(SDL_AudioStream* stream, void* buf, int len);
bool SDL_ResumeAudioStreamDevice(SDL_AudioStream* stream);
bool SDL_PauseAudioStreamDevice(SDL_AudioStream* stream);
void SDL_DestroyAudioStream(SDL_AudioStream* stream);
int SDL_GetAudioStreamQueued(SDL_AudioStream* stream);
int SDL_GetAudioStreamAvailable(SDL_AudioStream* stream);
void SDL_LockAudioStream(SDL_AudioStream* stream);
void SDL_UnlockAudioStream(SDL_AudioStream* stream);
bool SDL_SetAudioStreamGain(SDL_AudioStream* stream, float gain);

bool SDL_Init(Uint32 flags);
void SDL_Quit(void);

SDL_TimerID SDL_AddTimer(Uint32 interval, Uint32 (*callback)(Uint32, void*), void* param);
SDL_bool SDL_RemoveTimer(SDL_TimerID id);

SDL_AudioSpec* SDL_LoadWAV(const char* file, SDL_AudioSpec* spec, Uint8** audio_buf, Uint32* audio_len);
void SDL_FreeWAV(Uint8* audio_buf);

#define SDL_UpperBlit SDL_BlitSurface

#define SDL_CreateCond SDL_CreateCondition
#define SDL_CondSignal SDL_SignalCondition
#define SDL_CondWaitTimeout(c, m, ms) SDL_WaitConditionTimeout((c), (m), (Sint32)(ms))

#define SDL_FreeSurface SDL_DestroySurface
#define SDL_FillRect SDL_FillSurfaceRect
#define SDL_GetRendererOutputSize SDL_GetCurrentRenderOutputSize

#define SDL_GameControllerOpen SDL_OpenGamepad
#define SDL_GameControllerClose SDL_CloseGamepad
#define SDL_GameControllerGetJoystick SDL_GetGamepadJoystick
#define SDL_GameControllerName SDL_GetGamepadName
#define SDL_GameControllerNameForIndex SDL_GetGamepadNameForIndex
#define SDL_GameControllerMapping SDL_GetGamepadMapping
#define SDL_GameControllerGetStringForAxis SDL_GetGamepadStringForAxis
#define SDL_GameControllerGetStringForButton SDL_GetGamepadStringForButton
#define SDL_GameControllerGetButton SDL_GetGamepadButton
#define SDL_GameControllerGetButtonFromString SDL_GetGamepadButtonFromString
#define SDL_GameControllerAddMappingsFromFile SDL_AddGamepadMappingsFromFile
#define SDL_IsGameController SDL_IsGamepad

#define SDL_JoystickOpen SDL_OpenJoystick
#define SDL_JoystickClose SDL_CloseJoystick
#define SDL_JoystickName SDL_GetJoystickName
#define SDL_JoystickNameForIndex SDL_GetJoystickNameForIndex
#define SDL_JoystickInstanceID SDL_GetJoystickID
#define SDL_JoystickGetGUID SDL_GetJoystickGUID
#define SDL_JoystickGetGUIDString SDL_GetJoystickGUIDString
#define SDL_JoystickNumAxes SDL_GetNumJoystickAxes
#define SDL_JoystickNumBalls SDL_GetNumJoystickBalls
#define SDL_JoystickNumButtons SDL_GetNumJoystickButtons
#define SDL_JoystickNumHats SDL_GetNumJoystickHats
#define SDL_JoystickGetAxis SDL_GetJoystickAxis
#define SDL_JoystickGetButton SDL_GetJoystickButton

#ifdef __cplusplus
}
#endif

#endif
