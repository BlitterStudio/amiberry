#include "sysconfig.h"
#include "sysdeps.h"

#include "imgui_osk.h"
#include "imgui_overlay.h"

#include <imgui.h>
#include <cmath>
#include <cstring>
#include <set>
#include <map>
#include <SDL3/SDL.h>

#include "keyboard.h"
#include "inputdevice.h"

// Font accessors declared in imgui_overlay.h

// ============================================================
// Key types for visual styling
// ============================================================
enum OskKeyType {
	KEY_NORMAL = 0,
	KEY_MODIFIER,   // Shift, Ctrl, Alt, Caps
	KEY_FUNCTION,   // F1-F10, Esc, Del, Help
	KEY_SPACE,
	KEY_AMIGA_L,
	KEY_AMIGA_R,
	KEY_RETURN,
	KEY_ARROW,
	KEY_NUMPAD,
};

// ============================================================
// Key definition structure
// ============================================================
struct OskKeyDef {
	float x, y, w, h;        // Position/size in "unit" coordinates within the keyboard area
	int ak_code;              // AK_* keycode from keyboard.h
	const char* label;        // Primary label
	const char* shift_label;  // Shifted label (nullptr = same as label)
	OskKeyType type;
};

// ============================================================
// A500 US keyboard layout
// ============================================================
// Layout uses a coordinate system where one standard key = 1.0 units wide.
// Row heights are 1.0 units. The full keyboard is ~21 units wide (main + numpad).

// Row Y positions
static constexpr float ROW_FUNC  = 0.0f;
static constexpr float ROW_NUM   = 1.2f;  // small gap after function row
static constexpr float ROW_QWERT = 2.2f;
static constexpr float ROW_HOME  = 3.2f;
static constexpr float ROW_SHIFT = 4.2f;
static constexpr float ROW_SPACE = 5.2f;

static constexpr float KEY_H = 0.9f;  // key height (slightly less than 1.0 for gap)
static constexpr float FUNC_H = 0.7f; // function key height (shorter)

// Numpad X offset (gap of 0.5 after main section ending at ~15.5)
static constexpr float NP_X = 16.0f;

// Arrow cluster X offset (right of main keys, left of numpad)
static constexpr float AR_X = 14.0f;

static const OskKeyDef s_keys_us[] = {
	// ---- Row 0: Function keys ----
	{0.0f,  ROW_FUNC, 1.0f, FUNC_H, AK_ESC,  "Esc", nullptr, KEY_FUNCTION},
	{1.2f,  ROW_FUNC, 1.0f, FUNC_H, AK_F1,   "F1",  nullptr, KEY_FUNCTION},
	{2.2f,  ROW_FUNC, 1.0f, FUNC_H, AK_F2,   "F2",  nullptr, KEY_FUNCTION},
	{3.2f,  ROW_FUNC, 1.0f, FUNC_H, AK_F3,   "F3",  nullptr, KEY_FUNCTION},
	{4.2f,  ROW_FUNC, 1.0f, FUNC_H, AK_F4,   "F4",  nullptr, KEY_FUNCTION},
	{5.2f,  ROW_FUNC, 1.0f, FUNC_H, AK_F5,   "F5",  nullptr, KEY_FUNCTION},
	{6.4f,  ROW_FUNC, 1.0f, FUNC_H, AK_F6,   "F6",  nullptr, KEY_FUNCTION},
	{7.4f,  ROW_FUNC, 1.0f, FUNC_H, AK_F7,   "F7",  nullptr, KEY_FUNCTION},
	{8.4f,  ROW_FUNC, 1.0f, FUNC_H, AK_F8,   "F8",  nullptr, KEY_FUNCTION},
	{9.4f,  ROW_FUNC, 1.0f, FUNC_H, AK_F9,   "F9",  nullptr, KEY_FUNCTION},
	{10.4f, ROW_FUNC, 1.0f, FUNC_H, AK_F10,  "F10", nullptr, KEY_FUNCTION},
	{11.6f, ROW_FUNC, 1.5f, FUNC_H, AK_DEL,  "Del", nullptr, KEY_FUNCTION},
	{13.1f, ROW_FUNC, 1.5f, FUNC_H, AK_HELP, "Help",nullptr, KEY_FUNCTION},

	// ---- Row 1: Number row ----
	{0.0f,  ROW_NUM, 1.0f, KEY_H, AK_BACKQUOTE,  "`",  "~",  KEY_NORMAL},
	{1.0f,  ROW_NUM, 1.0f, KEY_H, AK_1,          "1",  "!",  KEY_NORMAL},
	{2.0f,  ROW_NUM, 1.0f, KEY_H, AK_2,          "2",  "@",  KEY_NORMAL},
	{3.0f,  ROW_NUM, 1.0f, KEY_H, AK_3,          "3",  "#",  KEY_NORMAL},
	{4.0f,  ROW_NUM, 1.0f, KEY_H, AK_4,          "4",  "$",  KEY_NORMAL},
	{5.0f,  ROW_NUM, 1.0f, KEY_H, AK_5,          "5",  "%",  KEY_NORMAL},
	{6.0f,  ROW_NUM, 1.0f, KEY_H, AK_6,          "6",  "^",  KEY_NORMAL},
	{7.0f,  ROW_NUM, 1.0f, KEY_H, AK_7,          "7",  "&",  KEY_NORMAL},
	{8.0f,  ROW_NUM, 1.0f, KEY_H, AK_8,          "8",  "*",  KEY_NORMAL},
	{9.0f,  ROW_NUM, 1.0f, KEY_H, AK_9,          "9",  "(",  KEY_NORMAL},
	{10.0f, ROW_NUM, 1.0f, KEY_H, AK_0,          "0",  ")",  KEY_NORMAL},
	{11.0f, ROW_NUM, 1.0f, KEY_H, AK_MINUS,      "-",  "_",  KEY_NORMAL},
	{12.0f, ROW_NUM, 1.0f, KEY_H, AK_EQUAL,      "=",  "+",  KEY_NORMAL},
	{13.0f, ROW_NUM, 1.0f, KEY_H, AK_BACKSLASH,  "\\", "|",  KEY_NORMAL},
	{14.0f, ROW_NUM, 1.5f, KEY_H, AK_BS,         "Bksp",nullptr, KEY_MODIFIER},

	// ---- Row 2: QWERTY ----
	{0.0f,  ROW_QWERT, 1.5f, KEY_H, AK_TAB,      "Tab",  nullptr, KEY_MODIFIER},
	{1.5f,  ROW_QWERT, 1.0f, KEY_H, AK_Q,        "Q",    nullptr, KEY_NORMAL},
	{2.5f,  ROW_QWERT, 1.0f, KEY_H, AK_W,        "W",    nullptr, KEY_NORMAL},
	{3.5f,  ROW_QWERT, 1.0f, KEY_H, AK_E,        "E",    nullptr, KEY_NORMAL},
	{4.5f,  ROW_QWERT, 1.0f, KEY_H, AK_R,        "R",    nullptr, KEY_NORMAL},
	{5.5f,  ROW_QWERT, 1.0f, KEY_H, AK_T,        "T",    nullptr, KEY_NORMAL},
	{6.5f,  ROW_QWERT, 1.0f, KEY_H, AK_Y,        "Y",    nullptr, KEY_NORMAL},
	{7.5f,  ROW_QWERT, 1.0f, KEY_H, AK_U,        "U",    nullptr, KEY_NORMAL},
	{8.5f,  ROW_QWERT, 1.0f, KEY_H, AK_I,        "I",    nullptr, KEY_NORMAL},
	{9.5f,  ROW_QWERT, 1.0f, KEY_H, AK_O,        "O",    nullptr, KEY_NORMAL},
	{10.5f, ROW_QWERT, 1.0f, KEY_H, AK_P,        "P",    nullptr, KEY_NORMAL},
	{11.5f, ROW_QWERT, 1.0f, KEY_H, AK_LBRACKET, "[",    "{",  KEY_NORMAL},
	{12.5f, ROW_QWERT, 1.0f, KEY_H, AK_RBRACKET, "]",    "}",  KEY_NORMAL},
	// Return key spans rows 2-3
	{13.5f, ROW_QWERT, 2.0f, KEY_H * 2.0f + 0.1f, AK_RET, "Ret", nullptr, KEY_RETURN},

	// ---- Row 3: Home row (indented to fit under QWERTY, Return fills the gap) ----
	{0.0f,  ROW_HOME, 1.75f, KEY_H, AK_CTRL,      "Ctrl",  nullptr, KEY_MODIFIER},
	{1.75f, ROW_HOME, 1.0f,  KEY_H, AK_CAPSLOCK,  "Caps",  nullptr, KEY_MODIFIER},
	{2.75f, ROW_HOME, 1.0f,  KEY_H, AK_A,         "A",     nullptr, KEY_NORMAL},
	{3.75f, ROW_HOME, 1.0f,  KEY_H, AK_S,         "S",     nullptr, KEY_NORMAL},
	{4.75f, ROW_HOME, 1.0f,  KEY_H, AK_D,         "D",     nullptr, KEY_NORMAL},
	{5.75f, ROW_HOME, 1.0f,  KEY_H, AK_F,         "F",     nullptr, KEY_NORMAL},
	{6.75f, ROW_HOME, 1.0f,  KEY_H, AK_G,         "G",     nullptr, KEY_NORMAL},
	{7.75f, ROW_HOME, 1.0f,  KEY_H, AK_H,         "H",     nullptr, KEY_NORMAL},
	{8.75f, ROW_HOME, 1.0f,  KEY_H, AK_J,         "J",     nullptr, KEY_NORMAL},
	{9.75f, ROW_HOME, 1.0f,  KEY_H, AK_K,         "K",     nullptr, KEY_NORMAL},
	{10.75f,ROW_HOME, 1.0f,  KEY_H, AK_L,         "L",     nullptr, KEY_NORMAL},
	{11.75f,ROW_HOME, 0.75f, KEY_H, AK_SEMICOLON, ";",     ":",  KEY_NORMAL},
	{12.5f, ROW_HOME, 0.5f,  KEY_H, AK_QUOTE,     "'",     "\"", KEY_NORMAL},
	{13.0f, ROW_HOME, 0.5f,  KEY_H, AK_NUMBERSIGN,"#",     "~",  KEY_NORMAL},
	// Return spans from row 2 at x=13.5. No overlap.

	// ---- Row 4: Shift row ----
	{0.0f,  ROW_SHIFT, 1.25f, KEY_H, AK_LSH,      "Shift", nullptr, KEY_MODIFIER},
	{1.25f, ROW_SHIFT, 1.0f,  KEY_H, AK_LTGT,     "<>",    nullptr, KEY_NORMAL},
	{2.25f, ROW_SHIFT, 1.0f,  KEY_H, AK_Z,        "Z",     nullptr, KEY_NORMAL},
	{3.25f, ROW_SHIFT, 1.0f,  KEY_H, AK_X,        "X",     nullptr, KEY_NORMAL},
	{4.25f, ROW_SHIFT, 1.0f,  KEY_H, AK_C,        "C",     nullptr, KEY_NORMAL},
	{5.25f, ROW_SHIFT, 1.0f,  KEY_H, AK_V,        "V",     nullptr, KEY_NORMAL},
	{6.25f, ROW_SHIFT, 1.0f,  KEY_H, AK_B,        "B",     nullptr, KEY_NORMAL},
	{7.25f, ROW_SHIFT, 1.0f,  KEY_H, AK_N,        "N",     nullptr, KEY_NORMAL},
	{8.25f, ROW_SHIFT, 1.0f,  KEY_H, AK_M,        "M",     nullptr, KEY_NORMAL},
	{9.25f, ROW_SHIFT, 1.0f,  KEY_H, AK_COMMA,    ",",     "<",  KEY_NORMAL},
	{10.25f,ROW_SHIFT, 1.0f,  KEY_H, AK_PERIOD,   ".",     ">",  KEY_NORMAL},
	{11.25f,ROW_SHIFT, 1.0f,  KEY_H, AK_SLASH,    "/",     "?",  KEY_NORMAL},
	{12.25f,ROW_SHIFT, 1.5f,  KEY_H, AK_RSH,      "Shift", nullptr, KEY_MODIFIER},
	// Cursor Up
	{AR_X + 1.0f, ROW_SHIFT, 1.0f, KEY_H, AK_UP, "^", nullptr, KEY_ARROW},

	// ---- Row 5: Space row ----
	{0.0f,  ROW_SPACE, 1.5f, KEY_H, AK_LALT,  "Alt",  nullptr, KEY_MODIFIER},
	{1.5f,  ROW_SPACE, 1.5f, KEY_H, AK_LAMI,  "A",    nullptr, KEY_AMIGA_L},
	{3.0f,  ROW_SPACE, 8.5f, KEY_H, AK_SPC,   "",     nullptr, KEY_SPACE},
	{11.5f, ROW_SPACE, 1.5f, KEY_H, AK_RAMI,  "A",    nullptr, KEY_AMIGA_R},
	{13.0f, ROW_SPACE, 1.5f, KEY_H, AK_RALT,  "Alt",  nullptr, KEY_MODIFIER},
	// Cursor Left/Down/Right
	{AR_X,       ROW_SPACE, 1.0f, KEY_H, AK_LF, "<", nullptr, KEY_ARROW},
	{AR_X + 1.0f,ROW_SPACE, 1.0f, KEY_H, AK_DN, "v", nullptr, KEY_ARROW},
	{AR_X + 2.0f,ROW_SPACE, 1.0f, KEY_H, AK_RT, ">", nullptr, KEY_ARROW},

	// ---- Numpad ----
	// Row 0 (function row level): ( ) / *
	{NP_X,       ROW_FUNC, 1.0f, FUNC_H, AK_NPLPAREN, "(", nullptr, KEY_NUMPAD},
	{NP_X + 1.0f,ROW_FUNC, 1.0f, FUNC_H, AK_NPRPAREN, ")", nullptr, KEY_NUMPAD},
	{NP_X + 2.0f,ROW_FUNC, 1.0f, FUNC_H, AK_NPDIV,    "/", nullptr, KEY_NUMPAD},
	{NP_X + 3.0f,ROW_FUNC, 1.0f, FUNC_H, AK_NPMUL,    "*", nullptr, KEY_NUMPAD},

	// Row 1: 7 8 9 -
	{NP_X,       ROW_NUM, 1.0f, KEY_H, AK_NP7, "7", nullptr, KEY_NUMPAD},
	{NP_X + 1.0f,ROW_NUM, 1.0f, KEY_H, AK_NP8, "8", nullptr, KEY_NUMPAD},
	{NP_X + 2.0f,ROW_NUM, 1.0f, KEY_H, AK_NP9, "9", nullptr, KEY_NUMPAD},
	{NP_X + 3.0f,ROW_NUM, 1.0f, KEY_H, AK_NPSUB,"-", nullptr, KEY_NUMPAD},

	// Row 2: 4 5 6 +
	{NP_X,       ROW_QWERT, 1.0f, KEY_H, AK_NP4, "4", nullptr, KEY_NUMPAD},
	{NP_X + 1.0f,ROW_QWERT, 1.0f, KEY_H, AK_NP5, "5", nullptr, KEY_NUMPAD},
	{NP_X + 2.0f,ROW_QWERT, 1.0f, KEY_H, AK_NP6, "6", nullptr, KEY_NUMPAD},
	{NP_X + 3.0f,ROW_QWERT, 1.0f, KEY_H, AK_NPADD,"+", nullptr, KEY_NUMPAD},

	// Row 3: 1 2 3 Enter (spans 2 rows)
	{NP_X,       ROW_HOME, 1.0f, KEY_H, AK_NP1, "1", nullptr, KEY_NUMPAD},
	{NP_X + 1.0f,ROW_HOME, 1.0f, KEY_H, AK_NP2, "2", nullptr, KEY_NUMPAD},
	{NP_X + 2.0f,ROW_HOME, 1.0f, KEY_H, AK_NP3, "3", nullptr, KEY_NUMPAD},
	{NP_X + 3.0f,ROW_HOME, 1.0f, KEY_H * 2.0f + 0.1f, AK_ENT, "Ent", nullptr, KEY_NUMPAD},

	// Row 4: 0 (wide) .
	{NP_X,       ROW_SHIFT, 2.0f, KEY_H, AK_NP0,   "0",  nullptr, KEY_NUMPAD},
	{NP_X + 2.0f,ROW_SHIFT, 1.0f, KEY_H, AK_NPDEL, ".",  nullptr, KEY_NUMPAD},
};

static const int NUM_KEYS = sizeof(s_keys_us) / sizeof(s_keys_us[0]);

// ============================================================
// Language-specific label overrides
// ============================================================
// The Amiga sends scancodes regardless of language — only keycap labels change.
// Each entry maps an AK_* code to language-specific label/shift_label.
struct OskLabelOverride {
	int ak_code;
	const char* label;
	const char* shift_label;
};

// German (QWERTZ) label overrides
static const OskLabelOverride s_labels_de[] = {
	{AK_Y,          "Z",  nullptr},
	{AK_Z,          "Y",  nullptr},
	{AK_LBRACKET,   "U:", nullptr},  // Ü
	{AK_RBRACKET,   "+",  "*"},
	{AK_SEMICOLON,  "O:", nullptr},  // Ö
	{AK_QUOTE,      "A:", nullptr},  // Ä
	{AK_BACKQUOTE,  "^",  nullptr},
	{AK_MINUS,      "ss", "?"},
	{AK_EQUAL,      "'",  "`"},
	{AK_SLASH,      "-",  "_"},
	{AK_COMMA,      ",",  ";"},
	{AK_PERIOD,     ".",  ":"},
	{AK_BACKSLASH,  "#",  "'"},
	{AK_LTGT,       "<",  ">"},
	{AK_2,          "2",  "\""},
	{AK_3,          "3",  nullptr},
	{AK_6,          "6",  "&"},
	{AK_7,          "7",  "/"},
	{AK_8,          "8",  "("},
	{AK_9,          "9",  ")"},
	{AK_0,          "0",  "="},
	{-1, nullptr, nullptr}
};

// UK label overrides (mostly US with minor differences)
static const OskLabelOverride s_labels_uk[] = {
	{AK_BACKSLASH,  "#",  "~"},
	{AK_NUMBERSIGN, "\\", "|"},
	{AK_2,          "2",  "\""},
	{AK_3,          "3",  nullptr},
	{AK_QUOTE,      "'",  "@"},
	{AK_LTGT,       "\\", "|"},
	{-1, nullptr, nullptr}
};

// French (AZERTY) label overrides
static const OskLabelOverride s_labels_fr[] = {
	{AK_Q,          "A",  nullptr},
	{AK_W,          "Z",  nullptr},
	{AK_A,          "Q",  nullptr},
	{AK_Z,          "W",  nullptr},
	{AK_SEMICOLON,  "M",  nullptr},
	{AK_M,          ",",  "?"},
	{AK_COMMA,      ";",  "."},
	{AK_PERIOD,     ":",  "/"},
	{AK_SLASH,      "!",  nullptr},
	{AK_1,          "&",  "1"},
	{AK_2,          "e'", "2"},
	{AK_3,          "\"", "3"},
	{AK_4,          "'",  "4"},
	{AK_5,          "(",  "5"},
	{AK_6,          "-",  "6"},
	{AK_7,          "e`", "7"},
	{AK_8,          "_",  "8"},
	{AK_9,          "c,", "9"},
	{AK_0,          "a`", "0"},
	{AK_MINUS,      ")",  nullptr},
	{AK_EQUAL,      "=",  "+"},
	{AK_LBRACKET,   "^",  nullptr},
	{AK_RBRACKET,   "$",  nullptr},
	{AK_BACKQUOTE,  "@",  "#"},
	{AK_QUOTE,      "u`", nullptr},
	{AK_LTGT,       "<",  ">"},
	{-1, nullptr, nullptr}
};

// Current active label overrides (nullptr = US, no overrides)
static const OskLabelOverride* s_active_overrides = nullptr;

// Find a label override for a given AK code
static const OskLabelOverride* find_override(int ak_code)
{
	if (!s_active_overrides) return nullptr;
	for (const OskLabelOverride* o = s_active_overrides; o->ak_code >= 0; o++) {
		if (o->ak_code == ak_code)
			return o;
	}
	return nullptr;
}

// ============================================================
// A500 color palette
// ============================================================
static ImU32 col_kb_bg;
static ImU32 col_key_normal;
static ImU32 col_key_modifier;
static ImU32 col_key_function;
static ImU32 col_key_pressed;
static ImU32 col_key_sticky_active;
static ImU32 col_bevel_light;
static ImU32 col_bevel_dark;
static ImU32 col_key_label;
static ImU32 col_key_label_dark;
static ImU32 col_amiga_l;
static ImU32 col_amiga_r;
static ImU32 col_focus_border;
static ImU32 col_key_shadow;
static ImU32 col_key_numpad;
static ImU32 col_key_arrow;
static ImU32 col_key_space;
static ImU32 col_key_return;

static void init_colors()
{
	col_kb_bg          = IM_COL32(70, 62, 52, 255);      // deep warm brown plate
	col_key_normal     = IM_COL32(228, 214, 188, 255);   // creamy beige
	col_key_modifier   = IM_COL32(178, 162, 138, 255);   // tan
	col_key_function   = IM_COL32(118, 92, 70, 255);     // deep brown
	col_key_pressed    = IM_COL32(255, 188, 90, 255);    // glowing amber (pressed)
	col_key_sticky_active = IM_COL32(232, 128, 30, 255); // strong amber (sticky)
	col_bevel_light    = IM_COL32(255, 246, 228, 255);   // bright highlight
	col_bevel_dark     = IM_COL32(70, 56, 40, 255);      // deep shadow
	col_key_label      = IM_COL32(32, 26, 20, 255);      // dark text
	col_key_label_dark = IM_COL32(248, 240, 222, 255);   // light text (for dark keys)
	col_amiga_l        = IM_COL32(220, 30, 30, 255);     // red (left Amiga)
	col_amiga_r        = IM_COL32(40, 110, 220, 255);    // blue (right Amiga)
	col_focus_border   = IM_COL32(255, 210, 60, 255);    // yellow focus highlight
	col_key_shadow     = IM_COL32(0, 0, 0, 110);         // stronger drop shadow
	col_key_numpad     = IM_COL32(206, 192, 168, 255);   // slightly cooler beige
	col_key_arrow      = IM_COL32(188, 174, 150, 255);   // arrow keys
	col_key_space      = IM_COL32(236, 224, 200, 255);   // space bar (lighter)
	col_key_return     = IM_COL32(206, 192, 168, 255);   // return key
}

// ============================================================
// State
// ============================================================
static bool s_initialized = false;
static bool s_visible = false;
static bool s_animating = false;
static float s_anim_progress = 0.0f; // 0 = hidden, 1 = fully visible
static Uint64 s_anim_start_time = 0;
static constexpr float ANIM_DURATION_MS = 300.0f; // slide animation duration

static float s_transparency = 0.85f; // configurable alpha

static int s_focused_key = -1; // currently focused key (for D-pad navigation)
static std::set<int> s_sticky_keys; // set of AK_* codes for active sticky modifiers
static std::map<int, int> s_finger_keys; // finger_id -> key index mapping
static std::set<int> s_pressed_keys; // set of key indices currently pressed (touch)

// Previous joystick state for edge detection
static int s_prev_joy_state = 0;

// Key repeat for held D-pad directions
static Uint64 s_repeat_start_time = 0;
static Uint64 s_repeat_last_time = 0;
static int s_repeat_dir = 0;
static constexpr Uint64 REPEAT_DELAY_MS = 400;  // initial delay before repeat
static constexpr Uint64 REPEAT_RATE_MS = 100;   // repeat interval

// Viewport dimensions (updated each frame)
static int s_viewport_w = 0;
static int s_viewport_h = 0;

// Computed keyboard geometry (updated when viewport changes)
static float s_kb_x = 0, s_kb_y = 0, s_kb_w = 0, s_kb_h = 0;
static float s_unit_w = 0, s_unit_h = 0;

// Total layout dimensions in units
static constexpr float LAYOUT_W = 20.0f; // total width in key units
static constexpr float LAYOUT_H = 6.1f;  // total height in key units
#ifdef __ANDROID__
static constexpr float KB_HEIGHT_FRAC = 0.55f; // larger keys for finger taps on phones
#else
static constexpr float KB_HEIGHT_FRAC = 0.42f; // keyboard occupies 42% of screen height
#endif

static bool is_modifier_key(int ak_code)
{
	return ak_code == AK_LSH || ak_code == AK_RSH ||
		   ak_code == AK_CTRL || ak_code == AK_LALT ||
		   ak_code == AK_RALT || ak_code == AK_LAMI ||
		   ak_code == AK_RAMI || ak_code == AK_CAPSLOCK;
}

// ============================================================
// Geometry computation
// ============================================================
static void compute_geometry(int dw, int dh)
{
	if (dw == s_viewport_w && dh == s_viewport_h)
		return;

	s_viewport_w = dw;
	s_viewport_h = dh;

	s_kb_h = dh * KB_HEIGHT_FRAC;
	s_kb_w = static_cast<float>(dw);

	// Unit size: fit the layout into the keyboard area with some padding
	float pad = s_kb_w * 0.02f;
	float usable_w = s_kb_w - pad * 2.0f;
	float usable_h = s_kb_h - pad * 2.0f;

	s_unit_w = usable_w / LAYOUT_W;
	s_unit_h = usable_h / LAYOUT_H;

	// Position: bottom of screen
	s_kb_x = 0;
	s_kb_y = dh - s_kb_h;
}

// Convert key definition coordinates to screen pixels
static void key_to_screen(const OskKeyDef& key, float anim_offset,
	float* out_x, float* out_y, float* out_w, float* out_h)
{
	float pad = s_kb_w * 0.02f;
	*out_x = s_kb_x + pad + key.x * s_unit_w;
	*out_y = s_kb_y + pad + key.y * s_unit_h + anim_offset;
	*out_w = key.w * s_unit_w - 2.0f; // 2px gap between keys
	*out_h = key.h * s_unit_h - 2.0f;
}

// ============================================================
// Rendering helpers
// ============================================================

static ImU32 apply_alpha(ImU32 col, float alpha)
{
	int a = static_cast<int>(((col >> 24) & 0xFF) * alpha);
	return (col & 0x00FFFFFF) | (static_cast<ImU32>(a) << 24);
}

static ImU32 get_key_color(const OskKeyDef& key)
{
	switch (key.type) {
	case KEY_MODIFIER:  return col_key_modifier;
	case KEY_FUNCTION:  return col_key_function;
	case KEY_SPACE:     return col_key_space;
	case KEY_AMIGA_L:   return col_key_modifier;
	case KEY_AMIGA_R:   return col_key_modifier;
	case KEY_RETURN:    return col_key_return;
	case KEY_ARROW:     return col_key_arrow;
	case KEY_NUMPAD:    return col_key_numpad;
	default:            return col_key_normal;
	}
}

static void draw_key(ImDrawList* dl, const OskKeyDef& key, int key_index,
	float kx, float ky, float kw, float kh, float alpha, ImFont* font, ImFont* font_small)
{
	bool is_pressed = s_pressed_keys.count(key_index) > 0;
	bool is_sticky = s_sticky_keys.count(key.ak_code) > 0;
	bool is_focused = (key_index == s_focused_key);

	// Determine key body color
	ImU32 body_col;
	if (is_pressed)
		body_col = col_key_pressed;
	else if (is_sticky)
		body_col = col_key_sticky_active;
	else
		body_col = get_key_color(key);

	body_col = apply_alpha(body_col, alpha);

	float rounding = 6.0f;
	float bevel = 2.0f;

	// Drop shadow (larger, softer, offset down)
	ImU32 shadow = apply_alpha(col_key_shadow, alpha);
	dl->AddRectFilled(
		ImVec2(kx + 1, ky + 3),
		ImVec2(kx + kw + 1, ky + kh + 3),
		shadow, rounding);

	// Key body
	dl->AddRectFilled(
		ImVec2(kx, ky), ImVec2(kx + kw, ky + kh),
		body_col, rounding);

	if (is_pressed) {
		// Glow ring around pressed key for strong feedback
		ImU32 glow = apply_alpha(IM_COL32(255, 210, 100, 180), alpha);
		dl->AddRect(
			ImVec2(kx - 1, ky - 1), ImVec2(kx + kw + 1, ky + kh + 1),
			glow, rounding + 1.0f, 0, 2.5f);
	} else {
		// Top highlight strip (subtle gloss)
		ImU32 blight = apply_alpha(col_bevel_light, alpha * 0.55f);
		dl->AddLine(ImVec2(kx + rounding * 0.5f, ky + 1.5f),
			ImVec2(kx + kw - rounding * 0.5f, ky + 1.5f), blight, bevel);

		// Bottom shadow strip
		ImU32 bdark = apply_alpha(col_bevel_dark, alpha * 0.45f);
		dl->AddLine(ImVec2(kx + rounding * 0.5f, ky + kh - 1.5f),
			ImVec2(kx + kw - rounding * 0.5f, ky + kh - 1.5f), bdark, bevel);

		// Outline for crisper definition against the dark plate
		ImU32 border = apply_alpha(IM_COL32(40, 32, 24, 120), alpha);
		dl->AddRect(
			ImVec2(kx, ky), ImVec2(kx + kw, ky + kh),
			border, rounding, 0, 1.0f);
	}

	// Amiga key decoration: colored stripe
	if (key.type == KEY_AMIGA_L) {
		ImU32 amiga_col = apply_alpha(col_amiga_l, alpha);
		float stripe_h = kh * 0.15f;
		dl->AddRectFilled(
			ImVec2(kx + 4, ky + kh - stripe_h - 4),
			ImVec2(kx + kw - 4, ky + kh - 4),
			amiga_col, 2.0f);
	} else if (key.type == KEY_AMIGA_R) {
		ImU32 amiga_col = apply_alpha(col_amiga_r, alpha);
		float stripe_h = kh * 0.15f;
		dl->AddRectFilled(
			ImVec2(kx + 4, ky + kh - stripe_h - 4),
			ImVec2(kx + kw - 4, ky + kh - 4),
			amiga_col, 2.0f);
	}

	// Focus border (for D-pad navigation)
	if (is_focused) {
		ImU32 focus = apply_alpha(col_focus_border, alpha);
		dl->AddRect(
			ImVec2(kx - 1, ky - 1), ImVec2(kx + kw + 1, ky + kh + 1),
			focus, rounding, 0, 3.0f);
	}

	// Determine label color
	ImU32 label_col;
	if (is_pressed)
		// Pressed keys now glow bright amber — dark text reads better.
		label_col = apply_alpha(col_key_label, alpha);
	else if (is_sticky || key.type == KEY_FUNCTION)
		label_col = apply_alpha(col_key_label_dark, alpha);
	else
		label_col = apply_alpha(col_key_label, alpha);

	// Arrow keys: draw triangle shapes instead of text
	if (key.type == KEY_ARROW) {
		float cx = kx + kw * 0.5f;
		float cy = ky + kh * 0.5f;
		float sz = (kw < kh ? kw : kh) * 0.25f;
		if (key.ak_code == AK_UP)
			dl->AddTriangleFilled(ImVec2(cx, cy - sz), ImVec2(cx - sz, cy + sz), ImVec2(cx + sz, cy + sz), label_col);
		else if (key.ak_code == AK_DN)
			dl->AddTriangleFilled(ImVec2(cx - sz, cy - sz), ImVec2(cx + sz, cy - sz), ImVec2(cx, cy + sz), label_col);
		else if (key.ak_code == AK_LF)
			dl->AddTriangleFilled(ImVec2(cx - sz, cy), ImVec2(cx + sz, cy - sz), ImVec2(cx + sz, cy + sz), label_col);
		else if (key.ak_code == AK_RT)
			dl->AddTriangleFilled(ImVec2(cx + sz, cy), ImVec2(cx - sz, cy - sz), ImVec2(cx - sz, cy + sz), label_col);
	} else {
		// Text label — apply language overrides if active
		bool show_shifted = s_sticky_keys.count(AK_LSH) > 0 || s_sticky_keys.count(AK_RSH) > 0;
		const char* base_label = key.label;
		const char* base_shift = key.shift_label;
		const OskLabelOverride* ovr = find_override(key.ak_code);
		if (ovr) {
			base_label = ovr->label;
			base_shift = ovr->shift_label;
		}
		const char* label = (show_shifted && base_shift) ? base_shift : base_label;
		if (label && label[0]) {
			ImFont* use_font = font;

			// Base size from key height — the natural target for label size
			float desired_size = kh * 0.55f;
			if (desired_size > s_unit_h * 0.6f) desired_size = s_unit_h * 0.6f;

			// Shrink only if the label overflows the key width.
			float max_w = kw * 0.88f;
			ImVec2 ts = use_font->CalcTextSizeA(desired_size, FLT_MAX, 0.0f, label);
			if (ts.x > max_w) {
				desired_size *= max_w / ts.x;
			}
			if (desired_size < 10.0f) desired_size = 10.0f;

			// Use the small pre-rasterised font for very small sizes to keep the
			// pixel font crisp instead of forcing the large one to scale way down.
			if (font_small && desired_size <= 18.0f)
				use_font = font_small;

			ImVec2 text_size = use_font->CalcTextSizeA(desired_size, FLT_MAX, 0.0f, label);
			float tx = kx + (kw - text_size.x) * 0.5f;
			float ty = ky + (kh - text_size.y) * 0.5f;

			dl->AddText(use_font, desired_size, ImVec2(tx, ty), label_col, label);
		}
	}
}

// ============================================================
// Public API
// ============================================================

void imgui_osk_init()
{
	if (s_initialized)
		return;

	init_colors();
	s_visible = false;
	s_animating = false;
	s_anim_progress = 0.0f;
	s_focused_key = 0;
	s_sticky_keys.clear();
	s_finger_keys.clear();
	s_pressed_keys.clear();
	s_prev_joy_state = 0;
	s_viewport_w = 0;
	s_viewport_h = 0;
	s_initialized = true;
}

void imgui_osk_shutdown()
{
	s_initialized = false;
	s_visible = false;
	s_animating = false;
	s_sticky_keys.clear();
	s_finger_keys.clear();
	s_pressed_keys.clear();
}

void imgui_osk_toggle()
{
	if (!s_initialized)
		return;

	s_visible = !s_visible;
	s_animating = true;
	s_anim_start_time = SDL_GetTicks();

	// Reset joystick navigation state when opening
	if (s_visible) {
		s_prev_joy_state = 0;
		s_repeat_dir = 0;
		s_repeat_start_time = 0;
		s_repeat_last_time = 0;
	}
}

bool imgui_osk_is_active()
{
	return s_visible;
}

bool imgui_osk_should_render()
{
	return s_visible || s_animating;
}

void imgui_osk_set_transparency(float alpha)
{
	s_transparency = alpha;
}

void imgui_osk_set_language(const char* lang)
{
	if (!lang || !lang[0] || strcasecmp(lang, "US") == 0)
		s_active_overrides = nullptr; // US = base layout, no overrides
	else if (strcasecmp(lang, "DE") == 0 || strcasecmp(lang, "GER") == 0)
		s_active_overrides = s_labels_de;
	else if (strcasecmp(lang, "UK") == 0)
		s_active_overrides = s_labels_uk;
	else if (strcasecmp(lang, "FR") == 0)
		s_active_overrides = s_labels_fr;
	else
		s_active_overrides = nullptr; // unknown = US fallback
}

void imgui_osk_render(int dw, int dh)
{
	if (!s_initialized || !imgui_overlay_is_initialized())
		return;

	compute_geometry(dw, dh);

	// Update animation
	float anim_offset = 0.0f;
	if (s_animating) {
		float elapsed = static_cast<float>(SDL_GetTicks() - s_anim_start_time);
		float t = elapsed / ANIM_DURATION_MS;
		if (t >= 1.0f) {
			t = 1.0f;
			s_animating = false;
		}
		// Ease-out cubic
		float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);

		if (s_visible) {
			s_anim_progress = ease;
			anim_offset = s_kb_h * (1.0f - ease);
		} else {
			s_anim_progress = 1.0f - ease;
			anim_offset = s_kb_h * ease;
			if (!s_animating) {
				s_anim_progress = 0.0f;
				return;
			}
		}
	} else if (!s_visible) {
		return;
	}

	float alpha = s_transparency * s_anim_progress;
	if (alpha <= 0.0f)
		return;

	ImDrawList* dl = ImGui::GetForegroundDrawList();
	ImFont* font = imgui_overlay_get_font();
	ImFont* font_small = imgui_overlay_get_font_small();

	// Draw keyboard background plate
	float bg_y = s_kb_y + anim_offset;

	// Soft drop shadow above the keyboard to blend with the emulation view
	float shadow_h = 16.0f;
	ImU32 shadow_top = IM_COL32(0, 0, 0, 0);
	ImU32 shadow_bot = apply_alpha(IM_COL32(0, 0, 0, 120), alpha);
	dl->AddRectFilledMultiColor(
		ImVec2(s_kb_x, bg_y - shadow_h),
		ImVec2(s_kb_x + s_kb_w, bg_y),
		shadow_top, shadow_top, shadow_bot, shadow_bot);

	ImU32 bg_col = apply_alpha(col_kb_bg, alpha);
	dl->AddRectFilled(
		ImVec2(s_kb_x, bg_y),
		ImVec2(s_kb_x + s_kb_w, bg_y + s_kb_h),
		bg_col, 12.0f, ImDrawFlags_RoundCornersTop);

	// Thin highlight along top edge
	ImU32 border_light = apply_alpha(IM_COL32(200, 180, 150, 220), alpha);
	dl->AddLine(
		ImVec2(s_kb_x + 12.0f, bg_y + 1.0f),
		ImVec2(s_kb_x + s_kb_w - 12.0f, bg_y + 1.0f),
		border_light, 1.5f);

	// Draw all keys
	for (int i = 0; i < NUM_KEYS; i++) {
		float kx, ky, kw, kh;
		key_to_screen(s_keys_us[i], anim_offset, &kx, &ky, &kw, &kh);
		draw_key(dl, s_keys_us[i], i, kx, ky, kw, kh, alpha, font, font_small);
	}

}

// ============================================================
// Touch input handling
// ============================================================

static int find_key_at(float screen_x, float screen_y)
{
	// Account for animation offset
	float anim_offset = 0.0f;
	if (s_animating) {
		if (s_visible)
			anim_offset = s_kb_h * (1.0f - s_anim_progress);
		else
			anim_offset = s_kb_h * s_anim_progress;
	}

	for (int i = 0; i < NUM_KEYS; i++) {
		float kx, ky, kw, kh;
		key_to_screen(s_keys_us[i], anim_offset, &kx, &ky, &kw, &kh);
		if (screen_x >= kx && screen_x <= kx + kw &&
			screen_y >= ky && screen_y <= ky + kh) {
			return i;
		}
	}
	return -1;
}

bool imgui_osk_hit_test(float screen_x, float screen_y)
{
	if (!s_visible || s_anim_progress < 1.0f)
		return false;

	float anim_offset = 0.0f;
	float bg_y = s_kb_y + anim_offset;
	return screen_y >= bg_y && screen_y <= bg_y + s_kb_h;
}

static void press_key(int ak_code)
{
	if (is_modifier_key(ak_code)) {
		// Toggle sticky modifier
		if (s_sticky_keys.count(ak_code)) {
			s_sticky_keys.erase(ak_code);
			inputdevice_do_keyboard(ak_code, 0);
		} else {
			s_sticky_keys.insert(ak_code);
			inputdevice_do_keyboard(ak_code, 1);
		}
	} else {
		inputdevice_do_keyboard(ak_code, 1);
	}
}

static void release_key(int ak_code)
{
	if (!is_modifier_key(ak_code)) {
		inputdevice_do_keyboard(ak_code, 0);

		// Release all non-caps-lock sticky modifiers after a normal key press
		std::set<int> to_release;
		for (int mod : s_sticky_keys) {
			if (mod != AK_CAPSLOCK) {
				to_release.insert(mod);
			}
		}
		for (int mod : to_release) {
			s_sticky_keys.erase(mod);
			inputdevice_do_keyboard(mod, 0);
		}
	}
}

bool imgui_osk_handle_finger_down(float screen_x, float screen_y, int finger_id)
{
	if (!s_visible || s_anim_progress < 0.99f)
		return false;

	int key_idx = find_key_at(screen_x, screen_y);
	if (key_idx < 0)
		return false;

	s_finger_keys[finger_id] = key_idx;
	s_pressed_keys.insert(key_idx);
	press_key(s_keys_us[key_idx].ak_code);
	return true;
}

bool imgui_osk_handle_finger_up(float screen_x, float screen_y, int finger_id)
{
	auto it = s_finger_keys.find(finger_id);
	if (it == s_finger_keys.end())
		return false;

	int key_idx = it->second;
	s_finger_keys.erase(it);
	s_pressed_keys.erase(key_idx);

	if (key_idx >= 0 && key_idx < NUM_KEYS) {
		release_key(s_keys_us[key_idx].ak_code);
	}
	return true;
}

bool imgui_osk_handle_finger_motion(float screen_x, float screen_y, int finger_id)
{
	auto it = s_finger_keys.find(finger_id);
	if (it == s_finger_keys.end())
		return false;

	int old_idx = it->second;
	int new_idx = find_key_at(screen_x, screen_y);

	if (new_idx != old_idx) {
		// Finger moved to a different key
		if (old_idx >= 0 && old_idx < NUM_KEYS) {
			s_pressed_keys.erase(old_idx);
			release_key(s_keys_us[old_idx].ak_code);
		}
		if (new_idx >= 0 && new_idx < NUM_KEYS) {
			s_finger_keys[finger_id] = new_idx;
			s_pressed_keys.insert(new_idx);
			press_key(s_keys_us[new_idx].ak_code);
		} else {
			s_finger_keys.erase(finger_id);
		}
	}
	return true;
}

// ============================================================
// Joystick/D-pad navigation
// ============================================================

// Navigation algorithm:
// - LEFT/RIGHT: stay on the same row, find nearest key horizontally.
//   If nothing on the same row, fall back to nearest in any row.
// - UP/DOWN: find the nearest row in that direction, then pick the
//   horizontally closest key in that row.
static int find_nearest_key(int from_idx, int direction)
{
	if (from_idx < 0 || from_idx >= NUM_KEYS)
		return 0;

	const OskKeyDef& from = s_keys_us[from_idx];
	float from_cx = from.x + from.w * 0.5f;
	float from_cy = from.y + from.h * 0.5f;

	// How much vertical offset counts as "same row":
	// keys whose center Y is within this tolerance of ours.
	// 0.6 accommodates tall keys like Return that span two rows.
	constexpr float ROW_TOLERANCE = 0.6f;

	if (direction == OSK_LEFT || direction == OSK_RIGHT) {
		// Horizontal: prefer same-row keys, pick nearest in X
		int best = -1;
		float best_dx = 999999.0f;

		for (int i = 0; i < NUM_KEYS; i++) {
			if (i == from_idx) continue;
			const OskKeyDef& to = s_keys_us[i];
			float to_cx = to.x + to.w * 0.5f;
			float to_cy = to.y + to.h * 0.5f;

			float dx = to_cx - from_cx;
			// Must be in the correct direction
			if (direction == OSK_LEFT  && dx >= -0.3f) continue;
			if (direction == OSK_RIGHT && dx <=  0.3f) continue;

			float dy = fabsf(to_cy - from_cy);
			float abs_dx = fabsf(dx);

			// Same row: only consider small vertical deviation
			if (dy < ROW_TOLERANCE) {
				if (abs_dx < best_dx) {
					best_dx = abs_dx;
					best = i;
				}
			}
		}

		// If nothing found on same row, try any row (fallback)
		if (best < 0) {
			for (int i = 0; i < NUM_KEYS; i++) {
				if (i == from_idx) continue;
				const OskKeyDef& to = s_keys_us[i];
				float to_cx = to.x + to.w * 0.5f;
				float dx = to_cx - from_cx;
				if (direction == OSK_LEFT  && dx >= -0.3f) continue;
				if (direction == OSK_RIGHT && dx <=  0.3f) continue;
				float abs_dx = fabsf(dx);
				if (abs_dx < best_dx) {
					best_dx = abs_dx;
					best = i;
				}
			}
		}
		return (best >= 0) ? best : from_idx;
	}

	// Vertical (UP/DOWN): two-phase — find nearest row, then closest key in it
	// Phase 1: find the nearest row center Y in the pressed direction
	float nearest_row_dy = 999999.0f;
	for (int i = 0; i < NUM_KEYS; i++) {
		if (i == from_idx) continue;
		const OskKeyDef& to = s_keys_us[i];
		float to_cy = to.y + to.h * 0.5f;
		float dy = to_cy - from_cy;

		if (direction == OSK_UP   && dy >= -0.3f) continue;
		if (direction == OSK_DOWN && dy <=  0.3f) continue;

		float abs_dy = fabsf(dy);
		if (abs_dy < nearest_row_dy)
			nearest_row_dy = abs_dy;
	}

	if (nearest_row_dy >= 999999.0f)
		return from_idx;

	// Phase 2: among keys at that row distance (±tolerance), pick closest horizontally
	int best = -1;
	float best_dx = 999999.0f;

	for (int i = 0; i < NUM_KEYS; i++) {
		if (i == from_idx) continue;
		const OskKeyDef& to = s_keys_us[i];
		float to_cx = to.x + to.w * 0.5f;
		float to_cy = to.y + to.h * 0.5f;
		float dy = to_cy - from_cy;

		if (direction == OSK_UP   && dy >= -0.3f) continue;
		if (direction == OSK_DOWN && dy <=  0.3f) continue;

		// Must be in the nearest row (within tolerance)
		if (fabsf(fabsf(dy) - nearest_row_dy) > ROW_TOLERANCE)
			continue;

		float cross = fabsf(to_cx - from_cx);
		if (cross < best_dx) {
			best_dx = cross;
			best = i;
		}
	}

	return (best >= 0) ? best : from_idx;
}

bool imgui_osk_process(int state, int* keycode, int* pressed)
{
	if (!s_visible || !s_initialized)
		return false;

	*keycode = 0;
	*pressed = 0;

	int prev = s_prev_joy_state;
	s_prev_joy_state = state;

	// Rising edges (newly pressed)
	int rising = state & ~prev;
	// Falling edges (newly released)
	int falling = prev & ~state;

	int dir_state = state & (OSK_UP | OSK_DOWN | OSK_LEFT | OSK_RIGHT);
	Uint64 now = SDL_GetTicks();

	// Navigation on rising edge
	bool moved = false;
	if (rising & (OSK_UP | OSK_DOWN | OSK_LEFT | OSK_RIGHT)) {
		if (rising & OSK_UP)    s_focused_key = find_nearest_key(s_focused_key, OSK_UP);
		if (rising & OSK_DOWN)  s_focused_key = find_nearest_key(s_focused_key, OSK_DOWN);
		if (rising & OSK_LEFT)  s_focused_key = find_nearest_key(s_focused_key, OSK_LEFT);
		if (rising & OSK_RIGHT) s_focused_key = find_nearest_key(s_focused_key, OSK_RIGHT);
		s_repeat_dir = dir_state;
		s_repeat_start_time = now;
		s_repeat_last_time = now;
		moved = true;
	}

	// Key repeat while direction held
	if (!moved && dir_state && dir_state == s_repeat_dir) {
		Uint64 held = now - s_repeat_start_time;
		if (held >= REPEAT_DELAY_MS) {
			Uint64 since_last = now - s_repeat_last_time;
			if (since_last >= REPEAT_RATE_MS) {
				if (dir_state & OSK_UP)    s_focused_key = find_nearest_key(s_focused_key, OSK_UP);
				if (dir_state & OSK_DOWN)  s_focused_key = find_nearest_key(s_focused_key, OSK_DOWN);
				if (dir_state & OSK_LEFT)  s_focused_key = find_nearest_key(s_focused_key, OSK_LEFT);
				if (dir_state & OSK_RIGHT) s_focused_key = find_nearest_key(s_focused_key, OSK_RIGHT);
				s_repeat_last_time = now;
			}
		}
	}

	// Direction released: reset repeat
	if (!dir_state) {
		s_repeat_dir = 0;
	}

	// Button press/release
	if (s_focused_key >= 0 && s_focused_key < NUM_KEYS) {
		int ak = s_keys_us[s_focused_key].ak_code;

		if (rising & OSK_BUTTON) {
			s_pressed_keys.insert(s_focused_key);
			press_key(ak);
			*keycode = ak;
			*pressed = 1;
			return true;
		}

		if (falling & OSK_BUTTON) {
			s_pressed_keys.erase(s_focused_key);
			release_key(ak);
			*keycode = ak;
			*pressed = 0;
			return true;
		}
	}

	return false;
}
