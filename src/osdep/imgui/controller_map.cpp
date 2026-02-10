#include "imgui.h"
#include "imgui_panels.h"

#include "sysdeps.h"
#include "gui/gui_handling.h"
#include "amiberry_gfx.h"
#include "fsdb_host.h"
#include "dpi_handler.hpp"

#include <SDL_image.h>
#include <cmath>
#include <string>

enum
{
	SDL_CONTROLLER_BINDING_AXIS_LEFTX_NEGATIVE,
	SDL_CONTROLLER_BINDING_AXIS_LEFTX_POSITIVE,
	SDL_CONTROLLER_BINDING_AXIS_LEFTY_NEGATIVE,
	SDL_CONTROLLER_BINDING_AXIS_LEFTY_POSITIVE,
	SDL_CONTROLLER_BINDING_AXIS_RIGHTX_NEGATIVE,
	SDL_CONTROLLER_BINDING_AXIS_RIGHTX_POSITIVE,
	SDL_CONTROLLER_BINDING_AXIS_RIGHTY_NEGATIVE,
	SDL_CONTROLLER_BINDING_AXIS_RIGHTY_POSITIVE,
	SDL_CONTROLLER_BINDING_AXIS_TRIGGERLEFT,
	SDL_CONTROLLER_BINDING_AXIS_TRIGGERRIGHT,
	SDL_CONTROLLER_BINDING_AXIS_MAX,
};

enum
{
	SCREEN_WIDTH = 640,
	SCREEN_HEIGHT = 500
};

// Offsets from the original controller map layout (pixel coordinates).
static constexpr int x_offset = 145;
static constexpr int y_offset = 255;

#define BINDING_COUNT (SDL_CONTROLLER_BUTTON_MAX + SDL_CONTROLLER_BINDING_AXIS_MAX)

static struct
{
	int x, y;
	double angle;
	int marker;
	std::string input;
} s_arrBindingDisplay[] = {
	{ 387, 167, 0.0, 1, "Button A (South)"}, /* SDL_CONTROLLER_BUTTON_A */
	{ 431, 132, 0.0, 1, "Button B (East)"}, /* SDL_CONTROLLER_BUTTON_B */
	{ 342, 132, 0.0, 1, "Button X (West)"}, /* SDL_CONTROLLER_BUTTON_X */
	{ 389, 101, 0.0, 1, "Button Y (North)"}, /* SDL_CONTROLLER_BUTTON_Y */
	{ 174, 132, 0.0, 1, "Button Back"}, /* SDL_CONTROLLER_BUTTON_BACK */
	{ 232, 128, 0.0, 1, "Button Guide"}, /* SDL_CONTROLLER_BUTTON_GUIDE */
	{ 289, 132, 0.0, 1, "Button Start"}, /* SDL_CONTROLLER_BUTTON_START */
	{  75, 154, 0.0, 1, "Left Stick button"}, /* SDL_CONTROLLER_BUTTON_LEFTSTICK */
	{ 305, 230, 0.0, 1, "Right Stick button"}, /* SDL_CONTROLLER_BUTTON_RIGHTSTICK */
	{  77,  40, 0.0, 1, "Left Shoulder"}, /* SDL_CONTROLLER_BUTTON_LEFTSHOULDER */
	{ 396,  36, 0.0, 1, "Right Shoulder"}, /* SDL_CONTROLLER_BUTTON_RIGHTSHOULDER */
	{ 154, 188, 0.0, 1, "D-PAD Up"}, /* SDL_CONTROLLER_BUTTON_DPAD_UP */
	{ 154, 249, 0.0, 1, "D-PAD Down"}, /* SDL_CONTROLLER_BUTTON_DPAD_DOWN */
	{ 116, 217, 0.0, 1, "D-PAD Left"}, /* SDL_CONTROLLER_BUTTON_DPAD_LEFT */
	{ 186, 217, 0.0, 1, "D-PAD Right"}, /* SDL_CONTROLLER_BUTTON_DPAD_RIGHT */
#if SDL_VERSION_ATLEAST(2,0,14)
	{ 232, 174, 0.0, 1, "Button Misc1"}, /* SDL_CONTROLLER_BUTTON_MISC1 */
	{ 132, 135, 0.0, 1, "Button Paddle 1"}, /* SDL_CONTROLLER_BUTTON_PADDLE1 */
	{ 330, 135, 0.0, 1, "Button Paddle 2"}, /* SDL_CONTROLLER_BUTTON_PADDLE2 */
	{ 132, 175, 0.0, 1, "Button Paddle 3"}, /* SDL_CONTROLLER_BUTTON_PADDLE3 */
	{ 330, 175, 0.0, 1, "Button Paddle 4"}, /* SDL_CONTROLLER_BUTTON_PADDLE4 */
	{   0,   0, 0.0, 1, "PS4/PS5 Touchpad button"}, /* SDL_CONTROLLER_BUTTON_TOUCHPAD */
#endif
	{  74, 153, 270.0, 2, "Left Stick X Negative"}, /* SDL_CONTROLLER_BINDING_AXIS_LEFTX_NEGATIVE */
	{  74, 153, 90.0,  2, "Left Stick X Positive"}, /* SDL_CONTROLLER_BINDING_AXIS_LEFTX_POSITIVE */
	{  74, 153, 0.0,   2, "Left Stick Y Negative"}, /* SDL_CONTROLLER_BINDING_AXIS_LEFTY_NEGATIVE */
	{  74, 153, 180.0, 2, "Left Stick Y Positive"}, /* SDL_CONTROLLER_BINDING_AXIS_LEFTY_POSITIVE */
	{ 306, 231, 270.0, 2, "Right Stick X Negative"}, /* SDL_CONTROLLER_BINDING_AXIS_RIGHTX_NEGATIVE */
	{ 306, 231, 90.0,  2, "Right Stick X Positive"}, /* SDL_CONTROLLER_BINDING_AXIS_RIGHTX_POSITIVE */
	{ 306, 231, 0.0,   2, "Right Stick Y Negative"}, /* SDL_CONTROLLER_BINDING_AXIS_RIGHTY_NEGATIVE */
	{ 306, 231, 180.0, 2, "Right Stick Y Positive"}, /* SDL_CONTROLLER_BINDING_AXIS_RIGHTY_POSITIVE */
	{  91, -20, 180.0, 2, "Left Trigger"}, /* SDL_CONTROLLER_BINDING_AXIS_TRIGGERLEFT */
	{ 375, -20, 180.0, 2, "Right Trigger"}, /* SDL_CONTROLLER_BINDING_AXIS_TRIGGERRIGHT */
};
SDL_COMPILE_TIME_ASSERT(s_arrBindingDisplay, std::size(s_arrBindingDisplay) == BINDING_COUNT);

static int s_arrBindingOrder[BINDING_COUNT] = {
	SDL_CONTROLLER_BUTTON_A,
	SDL_CONTROLLER_BUTTON_B,
	SDL_CONTROLLER_BUTTON_Y,
	SDL_CONTROLLER_BUTTON_X,
	SDL_CONTROLLER_BUTTON_MAX + SDL_CONTROLLER_BINDING_AXIS_LEFTX_NEGATIVE,
	SDL_CONTROLLER_BUTTON_MAX + SDL_CONTROLLER_BINDING_AXIS_LEFTX_POSITIVE,
	SDL_CONTROLLER_BUTTON_MAX + SDL_CONTROLLER_BINDING_AXIS_LEFTY_NEGATIVE,
	SDL_CONTROLLER_BUTTON_MAX + SDL_CONTROLLER_BINDING_AXIS_LEFTY_POSITIVE,
	SDL_CONTROLLER_BUTTON_LEFTSTICK,
	SDL_CONTROLLER_BUTTON_MAX + SDL_CONTROLLER_BINDING_AXIS_RIGHTX_NEGATIVE,
	SDL_CONTROLLER_BUTTON_MAX + SDL_CONTROLLER_BINDING_AXIS_RIGHTX_POSITIVE,
	SDL_CONTROLLER_BUTTON_MAX + SDL_CONTROLLER_BINDING_AXIS_RIGHTY_NEGATIVE,
	SDL_CONTROLLER_BUTTON_MAX + SDL_CONTROLLER_BINDING_AXIS_RIGHTY_POSITIVE,
	SDL_CONTROLLER_BUTTON_RIGHTSTICK,
	SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
	SDL_CONTROLLER_BUTTON_MAX + SDL_CONTROLLER_BINDING_AXIS_TRIGGERLEFT,
	SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
	SDL_CONTROLLER_BUTTON_MAX + SDL_CONTROLLER_BINDING_AXIS_TRIGGERRIGHT,
	SDL_CONTROLLER_BUTTON_DPAD_UP,
	SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
	SDL_CONTROLLER_BUTTON_DPAD_DOWN,
	SDL_CONTROLLER_BUTTON_DPAD_LEFT,
	SDL_CONTROLLER_BUTTON_BACK,
	SDL_CONTROLLER_BUTTON_GUIDE,
	SDL_CONTROLLER_BUTTON_START,
#if SDL_VERSION_ATLEAST(2,0,14)
	SDL_CONTROLLER_BUTTON_MISC1,
	SDL_CONTROLLER_BUTTON_PADDLE1,
	SDL_CONTROLLER_BUTTON_PADDLE2,
	SDL_CONTROLLER_BUTTON_PADDLE3,
	SDL_CONTROLLER_BUTTON_PADDLE4,
	SDL_CONTROLLER_BUTTON_TOUCHPAD,
#endif
};
SDL_COMPILE_TIME_ASSERT(s_arrBindingOrder, std::size(s_arrBindingOrder) == BINDING_COUNT);

typedef struct
{
	SDL_GameControllerBindType bindType;

	union
	{
		int button;

		struct
		{
			int axis;
			int axis_min;
			int axis_max;
		} axis;

		struct
		{
			int hat;
			int hat_mask;
		} hat;
	} value;

	SDL_bool committed;
} SDL_GameControllerExtendedBind;

static SDL_GameControllerExtendedBind s_arrBindings[BINDING_COUNT];

typedef struct
{
	SDL_bool m_bMoving;
	int m_nLastValue;
	int m_nStartingValue;
	int m_nFarthestValue;
} AxisState;

static int s_nNumAxes;
static AxisState* s_arrAxisState;

static int s_iCurrentBinding;
static Uint32 s_unPendingAdvanceTime;
static SDL_bool s_bBindingComplete;

static struct
{
	bool active = false;
	bool popup_open_requested = false;
	bool bind_touchpad = false;
	SDL_Joystick* joystick = nullptr;
	SDL_JoystickID joystick_id = 0;
	std::string joystick_name;
	std::string mapping_result;
	std::string error_text;
	SDL_Texture* background_front = nullptr;
	SDL_Texture* background_back = nullptr;
	SDL_Texture* marker_button = nullptr;
	SDL_Texture* marker_axis = nullptr;
	int bg_w = 0;
	int bg_h = 0;
	int marker_button_w = 0;
	int marker_button_h = 0;
	int marker_axis_w = 0;
	int marker_axis_h = 0;
} g_controller_map;

static const char* kControllerMapTitle = "Controller Map";

static SDL_Texture* LoadTexture(SDL_Renderer* renderer, const char* file, SDL_bool transparent, int* out_w, int* out_h)
{
	SDL_Surface* temp = IMG_Load(file);
	if (temp == nullptr)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't load %s: %s", file, SDL_GetError());
		return nullptr;
	}

	if (transparent)
	{
		if (temp->format->palette)
		{
			SDL_SetColorKey(temp, SDL_TRUE, *static_cast<Uint8*>(temp->pixels));
		}
	}

	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, temp);
	if (!texture)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create texture: %s\n", SDL_GetError());
		SDL_FreeSurface(temp);
		return nullptr;
	}

	if (out_w && out_h)
	{
		*out_w = temp->w;
		*out_h = temp->h;
	}
	SDL_FreeSurface(temp);
	return texture;
}

static void ControllerMap_EnsureTextures()
{
	if (g_controller_map.background_front && g_controller_map.background_back &&
		g_controller_map.marker_button && g_controller_map.marker_axis)
	{
		return;
	}

	AmigaMonitor* mon = &AMonitors[0];
	const auto front_path = prefix_with_data_path("controllermap.png");
	const auto back_path = prefix_with_data_path("controllermap_back.png");
	const auto button_path = prefix_with_data_path("button.png");
	const auto axis_path = prefix_with_data_path("axis.png");

	g_controller_map.background_front = LoadTexture(mon->gui_renderer, front_path.c_str(), SDL_TRUE, &g_controller_map.bg_w, &g_controller_map.bg_h);
	g_controller_map.background_back = LoadTexture(mon->gui_renderer, back_path.c_str(), SDL_TRUE, nullptr, nullptr);
	g_controller_map.marker_button = LoadTexture(mon->gui_renderer, button_path.c_str(), SDL_TRUE, &g_controller_map.marker_button_w, &g_controller_map.marker_button_h);
	g_controller_map.marker_axis = LoadTexture(mon->gui_renderer, axis_path.c_str(), SDL_TRUE, &g_controller_map.marker_axis_w, &g_controller_map.marker_axis_h);
}

static void ControllerMap_DestroyTextures()
{
	if (g_controller_map.background_front)
	{
		SDL_DestroyTexture(g_controller_map.background_front);
		g_controller_map.background_front = nullptr;
	}
	if (g_controller_map.background_back)
	{
		SDL_DestroyTexture(g_controller_map.background_back);
		g_controller_map.background_back = nullptr;
	}
	if (g_controller_map.marker_button)
	{
		SDL_DestroyTexture(g_controller_map.marker_button);
		g_controller_map.marker_button = nullptr;
	}
	if (g_controller_map.marker_axis)
	{
		SDL_DestroyTexture(g_controller_map.marker_axis);
		g_controller_map.marker_axis = nullptr;
	}
	g_controller_map.bg_w = 0;
	g_controller_map.bg_h = 0;
	g_controller_map.marker_button_w = 0;
	g_controller_map.marker_button_h = 0;
	g_controller_map.marker_axis_w = 0;
	g_controller_map.marker_axis_h = 0;
}

static void AddImageRotated(ImDrawList* draw_list, ImTextureID tex, ImVec2 center, ImVec2 size, float angle_rad, ImU32 tint)
{
	const ImVec2 half = ImVec2(size.x * 0.5f, size.y * 0.5f);
	ImVec2 corners[4] = {
		ImVec2(-half.x, -half.y),
		ImVec2( half.x, -half.y),
		ImVec2( half.x,  half.y),
		ImVec2(-half.x,  half.y)
	};

	const float s = sinf(angle_rad);
	const float c = cosf(angle_rad);
	for (auto& p : corners)
	{
		const float x = p.x * c - p.y * s;
		const float y = p.x * s + p.y * c;
		p = ImVec2(x + center.x, y + center.y);
	}

	draw_list->AddImageQuad(
		tex,
		corners[0], corners[1], corners[2], corners[3],
		ImVec2(0, 0), ImVec2(1, 0), ImVec2(1, 1), ImVec2(0, 1),
		tint
	);
}

static int StandardizeAxisValue(int nValue)
{
	if (nValue > SDL_JOYSTICK_AXIS_MAX / 2)
	{
		return SDL_JOYSTICK_AXIS_MAX;
	}
	if (nValue < SDL_JOYSTICK_AXIS_MIN / 2)
	{
		return SDL_JOYSTICK_AXIS_MIN;
	}
	return 0;
}

static void SetCurrentBinding(int iBinding)
{
	SDL_GameControllerExtendedBind* pBinding;

	if (iBinding < 0)
	{
		return;
	}

	if (iBinding == BINDING_COUNT)
	{
		s_bBindingComplete = SDL_TRUE;
		return;
	}

	if (s_arrBindingOrder[iBinding] == -1)
	{
		SetCurrentBinding(iBinding + 1);
		return;
	}

#if SDL_VERSION_ATLEAST(2,0,14)
	if (s_arrBindingOrder[iBinding] == SDL_CONTROLLER_BUTTON_TOUCHPAD &&
		!g_controller_map.bind_touchpad)
	{
		SetCurrentBinding(iBinding + 1);
		return;
	}
#endif
	s_iCurrentBinding = iBinding;

	pBinding = &s_arrBindings[s_arrBindingOrder[s_iCurrentBinding]];
	SDL_zerop(pBinding);

	for (int iIndex = 0; iIndex < s_nNumAxes; ++iIndex)
	{
		s_arrAxisState[iIndex].m_nFarthestValue = s_arrAxisState[iIndex].m_nStartingValue;
	}

	s_unPendingAdvanceTime = 0;
}

static bool BBindingContainsBinding(const SDL_GameControllerExtendedBind* pBindingA,
									const SDL_GameControllerExtendedBind* pBindingB)
{
	if (pBindingA->bindType != pBindingB->bindType)
	{
		return SDL_FALSE;
	}
	switch (pBindingA->bindType)
	{
	case SDL_CONTROLLER_BINDTYPE_AXIS:
		if (pBindingA->value.axis.axis != pBindingB->value.axis.axis)
		{
			return SDL_FALSE;
		}
		if (!pBindingA->committed)
		{
			return SDL_FALSE;
		}
		{
			const int minA = SDL_min(pBindingA->value.axis.axis_min, pBindingA->value.axis.axis_max);
			const int maxA = SDL_max(pBindingA->value.axis.axis_min, pBindingA->value.axis.axis_max);
			const int minB = SDL_min(pBindingB->value.axis.axis_min, pBindingB->value.axis.axis_max);
			const int maxB = SDL_max(pBindingB->value.axis.axis_min, pBindingB->value.axis.axis_max);
			return (minA <= minB && maxA >= maxB);
		}
	default:
		return SDL_memcmp(pBindingA, pBindingB, sizeof(*pBindingA)) == 0;
	}
}

static void ConfigureBinding(const SDL_GameControllerExtendedBind* pBinding)
{
	SDL_GameControllerExtendedBind* pCurrent;
	const int iCurrentElement = s_arrBindingOrder[s_iCurrentBinding];

	/* Do we already have this binding? */
	for (int iIndex = 0; iIndex < std::size(s_arrBindings); ++iIndex)
	{
		pCurrent = &s_arrBindings[iIndex];
		if (BBindingContainsBinding(pCurrent, pBinding))
		{
			if (iIndex == SDL_CONTROLLER_BUTTON_A && iCurrentElement != SDL_CONTROLLER_BUTTON_B)
			{
				/* Skip to the next binding */
				SetCurrentBinding(s_iCurrentBinding + 1);
				return;
			}

			if (iIndex == SDL_CONTROLLER_BUTTON_B)
			{
				/* Go back to the previous binding */
				SetCurrentBinding(s_iCurrentBinding - 1);
				return;
			}

			/* Already have this binding, ignore it */
			return;
		}
	}

	/* Should the new binding override the existing one? */
	pCurrent = &s_arrBindings[iCurrentElement];
	if (pCurrent->bindType != SDL_CONTROLLER_BINDTYPE_NONE)
	{
		const bool bNativeDPad = (iCurrentElement == SDL_CONTROLLER_BUTTON_DPAD_UP ||
			iCurrentElement == SDL_CONTROLLER_BUTTON_DPAD_DOWN ||
			iCurrentElement == SDL_CONTROLLER_BUTTON_DPAD_LEFT ||
			iCurrentElement == SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
		const bool bCurrentDPad = (pCurrent->bindType == SDL_CONTROLLER_BINDTYPE_HAT);
		if (bNativeDPad && bCurrentDPad)
		{
			/* We already have a binding of the type we want, ignore the new one */
			return;
		}

		const bool bNativeAxis = (iCurrentElement >= SDL_CONTROLLER_BUTTON_MAX);
		const bool bCurrentAxis = (pCurrent->bindType == SDL_CONTROLLER_BINDTYPE_AXIS);
		if (bNativeAxis == bCurrentAxis &&
			(pBinding->bindType != SDL_CONTROLLER_BINDTYPE_AXIS ||
				pBinding->value.axis.axis != pCurrent->value.axis.axis))
		{
			/* We already have a binding of the type we want, ignore the new one */
			return;
		}
	}

	*pCurrent = *pBinding;

	if (pBinding->committed)
	{
		s_unPendingAdvanceTime = SDL_GetTicks();
	}
	else
	{
		s_unPendingAdvanceTime = 0;
	}
}

static bool BMergeAxisBindings(int iIndex)
{
	SDL_GameControllerExtendedBind* pBindingA = &s_arrBindings[iIndex];
	SDL_GameControllerExtendedBind* pBindingB = &s_arrBindings[iIndex + 1];
	if (pBindingA->bindType == SDL_CONTROLLER_BINDTYPE_AXIS &&
		pBindingB->bindType == SDL_CONTROLLER_BINDTYPE_AXIS &&
		pBindingA->value.axis.axis == pBindingB->value.axis.axis)
	{
		if (pBindingA->value.axis.axis_min == pBindingB->value.axis.axis_min)
		{
			pBindingA->value.axis.axis_min = pBindingA->value.axis.axis_max;
			pBindingA->value.axis.axis_max = pBindingB->value.axis.axis_max;
			pBindingB->bindType = SDL_CONTROLLER_BINDTYPE_NONE;
			return SDL_TRUE;
		}
	}
	return SDL_FALSE;
}

static void ControllerMap_ResetState()
{
	g_controller_map.mapping_result.clear();
	g_controller_map.error_text.clear();
	s_bBindingComplete = SDL_FALSE;
	s_unPendingAdvanceTime = 0;
	s_iCurrentBinding = 0;
	SDL_memset(s_arrBindings, 0, sizeof(s_arrBindings));
}

static void ControllerMap_Close()
{
	if (g_controller_map.joystick)
	{
		SDL_JoystickClose(g_controller_map.joystick);
		g_controller_map.joystick = nullptr;
	}
	SDL_free(s_arrAxisState);
	s_arrAxisState = nullptr;
	s_nNumAxes = 0;

	ControllerMap_DestroyTextures();

	g_controller_map.active = false;
	g_controller_map.popup_open_requested = false;
}

static void ControllerMap_BuildMapping()
{
	char mapping[1024];
	char trimmed_name[128];
	char* spot;
	int iIndex;
	char pszElement[12];

	const char* name = g_controller_map.joystick_name.c_str();

	SDL_strlcpy(trimmed_name, name, std::size(trimmed_name));
	while (SDL_isspace(trimmed_name[0]))
	{
		SDL_memmove(&trimmed_name[0], &trimmed_name[1], SDL_strlen(trimmed_name));
	}
	while (trimmed_name[0] && SDL_isspace(trimmed_name[SDL_strlen(trimmed_name) - 1]))
	{
		trimmed_name[SDL_strlen(trimmed_name) - 1] = '\0';
	}
	while ((spot = SDL_strchr(trimmed_name, ',')) != nullptr)
	{
		SDL_memmove(spot, spot + 1, SDL_strlen(spot));
	}

	/* Initialize mapping with GUID and name */
	SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(g_controller_map.joystick), mapping, std::size(mapping));
	SDL_strlcat(mapping, ",", std::size(mapping));
	SDL_strlcat(mapping, trimmed_name, std::size(mapping));
	SDL_strlcat(mapping, ",", std::size(mapping));
	SDL_strlcat(mapping, "platform:", std::size(mapping));
	SDL_strlcat(mapping, SDL_GetPlatform(), std::size(mapping));
	SDL_strlcat(mapping, ",", std::size(mapping));

	for (iIndex = 0; iIndex < std::size(s_arrBindings); ++iIndex)
	{
		SDL_GameControllerExtendedBind* pBinding = &s_arrBindings[iIndex];
		if (pBinding->bindType == SDL_CONTROLLER_BINDTYPE_NONE)
		{
			continue;
		}

		if (iIndex < SDL_CONTROLLER_BUTTON_MAX)
		{
			auto eButton = static_cast<SDL_GameControllerButton>(iIndex);
			SDL_strlcat(mapping, SDL_GameControllerGetStringForButton(eButton), std::size(mapping));
		}
		else
		{
			const char* pszAxisName;
			switch (iIndex - SDL_CONTROLLER_BUTTON_MAX)
			{
			case SDL_CONTROLLER_BINDING_AXIS_LEFTX_NEGATIVE:
				if (!BMergeAxisBindings(iIndex))
				{
					SDL_strlcat(mapping, "-", std::size(mapping));
				}
				pszAxisName = SDL_GameControllerGetStringForAxis(SDL_CONTROLLER_AXIS_LEFTX);
				break;
			case SDL_CONTROLLER_BINDING_AXIS_LEFTX_POSITIVE:
				SDL_strlcat(mapping, "+", std::size(mapping));
				pszAxisName = SDL_GameControllerGetStringForAxis(SDL_CONTROLLER_AXIS_LEFTX);
				break;
			case SDL_CONTROLLER_BINDING_AXIS_LEFTY_NEGATIVE:
				if (!BMergeAxisBindings(iIndex))
				{
					SDL_strlcat(mapping, "-", std::size(mapping));
				}
				pszAxisName = SDL_GameControllerGetStringForAxis(SDL_CONTROLLER_AXIS_LEFTY);
				break;
			case SDL_CONTROLLER_BINDING_AXIS_LEFTY_POSITIVE:
				SDL_strlcat(mapping, "+", std::size(mapping));
				pszAxisName = SDL_GameControllerGetStringForAxis(SDL_CONTROLLER_AXIS_LEFTY);
				break;
			case SDL_CONTROLLER_BINDING_AXIS_RIGHTX_NEGATIVE:
				if (!BMergeAxisBindings(iIndex))
				{
					SDL_strlcat(mapping, "-", std::size(mapping));
				}
				pszAxisName = SDL_GameControllerGetStringForAxis(SDL_CONTROLLER_AXIS_RIGHTX);
				break;
			case SDL_CONTROLLER_BINDING_AXIS_RIGHTX_POSITIVE:
				SDL_strlcat(mapping, "+", std::size(mapping));
				pszAxisName = SDL_GameControllerGetStringForAxis(SDL_CONTROLLER_AXIS_RIGHTX);
				break;
			case SDL_CONTROLLER_BINDING_AXIS_RIGHTY_NEGATIVE:
				if (!BMergeAxisBindings(iIndex))
				{
					SDL_strlcat(mapping, "-", std::size(mapping));
				}
				pszAxisName = SDL_GameControllerGetStringForAxis(SDL_CONTROLLER_AXIS_RIGHTY);
				break;
			case SDL_CONTROLLER_BINDING_AXIS_RIGHTY_POSITIVE:
				SDL_strlcat(mapping, "+", std::size(mapping));
				pszAxisName = SDL_GameControllerGetStringForAxis(SDL_CONTROLLER_AXIS_RIGHTY);
				break;
			case SDL_CONTROLLER_BINDING_AXIS_TRIGGERLEFT:
				pszAxisName = SDL_GameControllerGetStringForAxis(SDL_CONTROLLER_AXIS_TRIGGERLEFT);
				break;
			case SDL_CONTROLLER_BINDING_AXIS_TRIGGERRIGHT:
				pszAxisName = SDL_GameControllerGetStringForAxis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
				break;
			default: /* Shouldn't happen */
				pszAxisName = "Unknown";
				break;
			}
			SDL_strlcat(mapping, pszAxisName, std::size(mapping));
		}
		SDL_strlcat(mapping, ":", std::size(mapping));

		pszElement[0] = '\0';
		switch (pBinding->bindType)
		{
		case SDL_CONTROLLER_BINDTYPE_BUTTON:
			SDL_snprintf(pszElement, sizeof(pszElement), "b%d", pBinding->value.button);
			break;
		case SDL_CONTROLLER_BINDTYPE_AXIS:
			if (pBinding->value.axis.axis_min == 0 && pBinding->value.axis.axis_max == SDL_JOYSTICK_AXIS_MIN)
			{
				/* The negative half axis */
				SDL_snprintf(pszElement, sizeof(pszElement), "-a%d", pBinding->value.axis.axis);
			}
			else if (pBinding->value.axis.axis_min == 0 && pBinding->value.axis.axis_max == SDL_JOYSTICK_AXIS_MAX)
			{
				/* The positive half axis */
				SDL_snprintf(pszElement, sizeof(pszElement), "+a%d", pBinding->value.axis.axis);
			}
			else
			{
				SDL_snprintf(pszElement, sizeof(pszElement), "a%d", pBinding->value.axis.axis);
				if (pBinding->value.axis.axis_min > pBinding->value.axis.axis_max)
				{
					/* Invert the axis */
					SDL_strlcat(pszElement, "~", std::size(pszElement));
				}
			}
			break;
		case SDL_CONTROLLER_BINDTYPE_HAT:
			SDL_snprintf(pszElement, sizeof(pszElement), "h%d.%d", pBinding->value.hat.hat,
						 pBinding->value.hat.hat_mask);
			break;
		default:
			SDL_assert(!"Unknown bind type");
			break;
		}
		SDL_strlcat(mapping, pszElement, std::size(mapping));
		SDL_strlcat(mapping, ",", std::size(mapping));
	}

	g_controller_map.mapping_result = std::string(mapping);
}

void ControllerMap_Open(int device, bool map_touchpad)
{
	if (g_controller_map.active)
	{
		return;
	}

	ControllerMap_ResetState();

	if (SDL_NumJoysticks() <= 0 || device < 0 || device >= SDL_NumJoysticks())
	{
		g_controller_map.error_text = "No joystick found for mapping.";
		g_controller_map.active = true;
		g_controller_map.popup_open_requested = true;
		return;
	}

	g_controller_map.joystick = SDL_JoystickOpen(device);
	if (!g_controller_map.joystick)
	{
		g_controller_map.error_text = "Could not open joystick for mapping.";
		g_controller_map.active = true;
		g_controller_map.popup_open_requested = true;
		return;
	}

	const char* name = SDL_JoystickName(g_controller_map.joystick);
	g_controller_map.joystick_name = name ? name : "Unknown Joystick";
	g_controller_map.joystick_id = SDL_JoystickInstanceID(g_controller_map.joystick);
	g_controller_map.bind_touchpad = map_touchpad;

	ControllerMap_EnsureTextures();

	s_nNumAxes = SDL_JoystickNumAxes(g_controller_map.joystick);
	s_arrAxisState = static_cast<AxisState*>(SDL_calloc(s_nNumAxes, sizeof(*s_arrAxisState)));

	SetCurrentBinding(0);

	g_controller_map.active = true;
	g_controller_map.popup_open_requested = true;
}

bool ControllerMap_IsActive()
{
	return g_controller_map.active;
}

bool ControllerMap_ConsumeResult(std::string& out_mapping)
{
	if (g_controller_map.mapping_result.empty())
	{
		return false;
	}

	out_mapping = g_controller_map.mapping_result;
	g_controller_map.mapping_result.clear();
	return true;
}

bool ControllerMap_HandleEvent(const SDL_Event& event)
{
	if (!g_controller_map.active || !g_controller_map.error_text.empty())
	{
		return false;
	}

	switch (event.type)
	{
	case SDL_JOYDEVICEREMOVED:
		if (event.jaxis.which == g_controller_map.joystick_id)
		{
			g_controller_map.error_text = "Joystick removed.";
			return true;
		}
		break;
	case SDL_JOYAXISMOTION:
		if (event.jaxis.which == g_controller_map.joystick_id)
		{
			constexpr int MAX_ALLOWED_JITTER = SDL_JOYSTICK_AXIS_MAX / 80;
			AxisState* pAxisState = &s_arrAxisState[event.jaxis.axis];
			int nValue = event.jaxis.value;
			int nCurrentDistance, nFarthestDistance;
			if (!pAxisState->m_bMoving)
			{
				Sint16 nInitialValue;
				pAxisState->m_bMoving = SDL_JoystickGetAxisInitialState(
					g_controller_map.joystick, event.jaxis.axis, &nInitialValue);
				pAxisState->m_nLastValue = nInitialValue;
				pAxisState->m_nStartingValue = nInitialValue;
				pAxisState->m_nFarthestValue = nInitialValue;
			}
			else if (SDL_abs(nValue - pAxisState->m_nLastValue) <= MAX_ALLOWED_JITTER)
			{
				break;
			}
			else
			{
				pAxisState->m_nLastValue = nValue;
			}
			nCurrentDistance = SDL_abs(nValue - pAxisState->m_nStartingValue);
			nFarthestDistance = SDL_abs(pAxisState->m_nFarthestValue - pAxisState->m_nStartingValue);
			if (nCurrentDistance > nFarthestDistance)
			{
				pAxisState->m_nFarthestValue = nValue;
				nFarthestDistance = SDL_abs(pAxisState->m_nFarthestValue - pAxisState->m_nStartingValue);
			}

			if (nFarthestDistance >= 16000)
			{
				SDL_bool bCommitBinding = (nCurrentDistance <= 10000) ? SDL_TRUE : SDL_FALSE;
				SDL_GameControllerExtendedBind binding;
				SDL_zero(binding);
				binding.bindType = SDL_CONTROLLER_BINDTYPE_AXIS;
				binding.value.axis.axis = event.jaxis.axis;
				binding.value.axis.axis_min = StandardizeAxisValue(pAxisState->m_nStartingValue);
				binding.value.axis.axis_max = StandardizeAxisValue(pAxisState->m_nFarthestValue);
				binding.committed = bCommitBinding;
				ConfigureBinding(&binding);
			}
			return true;
		}
		break;
	case SDL_JOYHATMOTION:
		if (event.jhat.which == g_controller_map.joystick_id)
		{
			if (event.jhat.value != SDL_HAT_CENTERED)
			{
				SDL_GameControllerExtendedBind binding;
				SDL_zero(binding);
				binding.bindType = SDL_CONTROLLER_BINDTYPE_HAT;
				binding.value.hat.hat = event.jhat.hat;
				binding.value.hat.hat_mask = event.jhat.value;
				binding.committed = SDL_TRUE;
				ConfigureBinding(&binding);
			}
			return true;
		}
		break;
	case SDL_JOYBUTTONDOWN:
		if (event.jbutton.which == g_controller_map.joystick_id)
		{
			SDL_GameControllerExtendedBind binding;
			SDL_zero(binding);
			binding.bindType = SDL_CONTROLLER_BINDTYPE_BUTTON;
			binding.value.button = event.jbutton.button;
			binding.committed = SDL_TRUE;
			ConfigureBinding(&binding);
			return true;
		}
		break;
	case SDL_KEYDOWN:
		if (event.key.keysym.sym == SDLK_BACKSPACE || event.key.keysym.sym == SDLK_AC_BACK)
		{
			SetCurrentBinding(s_iCurrentBinding - 1);
			return true;
		}
		if (event.key.keysym.sym == SDLK_SPACE)
		{
			SetCurrentBinding(s_iCurrentBinding + 1);
			return true;
		}
		if (event.key.keysym.sym == SDLK_ESCAPE)
		{
			g_controller_map.error_text = "Mapping canceled.";
			return true;
		}
		break;
	case SDL_MOUSEBUTTONDOWN:
	case SDL_FINGERDOWN:
		SetCurrentBinding(s_iCurrentBinding + 1);
		return true;
	default:
		break;
	}

	return false;
}

void ControllerMap_RenderModal()
{
	if (!g_controller_map.active)
	{
		return;
	}

	if (g_controller_map.popup_open_requested)
	{
		ImGui::OpenPopup(kControllerMapTitle);
		g_controller_map.popup_open_requested = false;
	}

	// Compute a fixed window height to avoid scrollbars while fitting all content.
	const float ui_scale = DPIHandler::get_layout_scale();
	const float scaled_screen_w = SCREEN_WIDTH * ui_scale;
	const float text_line_h = ImGui::GetTextLineHeightWithSpacing();
	const float wrap_w = scaled_screen_w - 2.0f * DISTANCE_BORDER;
	const ImVec2 info_size = ImGui::CalcTextSize(
		"Press the buttons on your controller when indicated.\n"
		"Backspace/Back goes to previous, Space skips, ESC cancels.", nullptr, false, wrap_w);
	const float scaled_bg_h = static_cast<float>(g_controller_map.bg_h) * ui_scale;
	const float desired_height =
		DISTANCE_BORDER + info_size.y + DISTANCE_NEXT_Y +
		text_line_h + DISTANCE_NEXT_Y +
		scaled_bg_h + DISTANCE_NEXT_Y * 0.5f +
		text_line_h * 2.0f + DISTANCE_NEXT_Y +
		BUTTON_HEIGHT + DISTANCE_BORDER;

	ImGui::SetNextWindowSize(ImVec2(scaled_screen_w, desired_height), ImGuiCond_Always);
	if (ImGui::BeginPopupModal(kControllerMapTitle, nullptr,
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
	{
		const ImVec2 window_pos = ImGui::GetWindowPos();
		const ImVec2 content_min = ImGui::GetWindowContentRegionMin();
		const ImVec2 origin = ImVec2(window_pos.x + content_min.x, window_pos.y + content_min.y);

		const char* info_line1 = "Press the buttons on your controller when indicated.";
		const char* info_line2 = "Backspace/Back goes to previous, Space skips, ESC cancels.";

		ImGui::SetCursorPos(ImVec2(DISTANCE_BORDER, DISTANCE_BORDER));
		ImGui::PushTextWrapPos(DISTANCE_BORDER + wrap_w);
		ImGui::TextUnformatted(info_line1);
		ImGui::TextUnformatted(info_line2);
		ImGui::PopTextWrapPos();

		const float message_y = DISTANCE_BORDER + info_size.y + DISTANCE_NEXT_Y;
		ImGui::SetCursorPos(ImVec2(DISTANCE_BORDER, message_y));
		ImGui::Text("Device: %s", g_controller_map.joystick_name.c_str());

		if (!g_controller_map.error_text.empty())
		{
			ImGui::TextWrapped("%s", g_controller_map.error_text.c_str());
			ImGui::Spacing();
			if (AmigaButton("Close", ImVec2(BUTTON_WIDTH, 0)))
			{
				ImGui::CloseCurrentPopup();
				ControllerMap_Close();
			}
			ImGui::EndPopup();
			return;
		}

		const int iElement = s_arrBindingOrder[s_iCurrentBinding];
		ControllerMap_EnsureTextures();

		if (g_controller_map.background_front)
		{
#if SDL_VERSION_ATLEAST(2,0,14)
			const bool use_back = (s_arrBindingOrder[s_iCurrentBinding] >= SDL_CONTROLLER_BUTTON_PADDLE1 &&
				s_arrBindingOrder[s_iCurrentBinding] <= SDL_CONTROLLER_BUTTON_PADDLE4);
#else
			const bool use_back = false;
#endif

			SDL_Texture* bg_tex = use_back && g_controller_map.background_back ? g_controller_map.background_back : g_controller_map.background_front;

			const float bg_top_y = message_y + text_line_h + DISTANCE_NEXT_Y;
			const float scaled_bg_w = static_cast<float>(g_controller_map.bg_w) * ui_scale;
			const ImVec2 img_pos = ImVec2(origin.x + (scaled_screen_w - scaled_bg_w) * 0.5f,
										  origin.y + bg_top_y);
			const ImVec2 img_size = ImVec2(scaled_bg_w, scaled_bg_h);

			ImDrawList* dl = ImGui::GetWindowDrawList();
			ImDrawList* dl_overlay = ImGui::GetForegroundDrawList();
			dl->AddImage(reinterpret_cast<ImTextureID>(bg_tex),
				img_pos,
				ImVec2(img_pos.x + img_size.x, img_pos.y + img_size.y));

			const bool is_axis_marker = (s_arrBindingDisplay[iElement].marker == 2);
			SDL_Texture* marker_tex = is_axis_marker ? g_controller_map.marker_axis : g_controller_map.marker_button;
			int marker_src_w = is_axis_marker ? g_controller_map.marker_axis_w : g_controller_map.marker_button_w;
			int marker_src_h = is_axis_marker ? g_controller_map.marker_axis_h : g_controller_map.marker_button_h;
			if (marker_src_w <= 0 || marker_src_h <= 0)
			{
				marker_src_w = 32;
				marker_src_h = 32;
			}
			const ImVec2 marker_size = ImVec2(static_cast<float>(marker_src_w) * ui_scale,
											  static_cast<float>(marker_src_h) * ui_scale);

			const float dst_x = static_cast<float>(s_arrBindingDisplay[iElement].x) * ui_scale;
			const float dst_y = static_cast<float>(s_arrBindingDisplay[iElement].y) * ui_scale;
			const ImVec2 marker_center = ImVec2(img_pos.x + dst_x + marker_size.x * 0.5f,
												img_pos.y + dst_y + marker_size.y * 0.5f);

			const float t = static_cast<float>(ImGui::GetTime());
			const float alpha = 0.5f + 0.5f * std::sin(t * 2.0f);
			const ImU32 tint = IM_COL32(10, 255, 21, static_cast<int>(128 + 127 * alpha));
			const float angle = static_cast<float>(s_arrBindingDisplay[iElement].angle) * (3.14159265f / 180.0f);
			if (marker_tex)
			{
				AddImageRotated(dl_overlay,
					reinterpret_cast<ImTextureID>(marker_tex),
					marker_center,
					marker_size,
					angle,
					tint);
			}
			else
			{
				dl_overlay->AddCircleFilled(marker_center, marker_size.x * 0.5f, tint, 24);
			}

		}

		const float press_y = message_y + text_line_h + DISTANCE_NEXT_Y + scaled_bg_h + DISTANCE_NEXT_Y * 0.5f;
		ImGui::SetCursorPos(ImVec2(DISTANCE_BORDER, press_y));
		ImGui::Text("Press: %s", s_arrBindingDisplay[iElement].input.c_str());
		ImGui::SetCursorPos(ImVec2(DISTANCE_BORDER, press_y + text_line_h));
		ImGui::Text("Step %d / %d", s_iCurrentBinding + 1, BINDING_COUNT);

		const float buttons_y = SCREEN_HEIGHT - DISTANCE_BORDER - BUTTON_HEIGHT;
		ImGui::SetCursorPos(ImVec2(DISTANCE_BORDER, buttons_y));
		if (AmigaButton("Back", ImVec2(BUTTON_WIDTH, 0)))
		{
			SetCurrentBinding(s_iCurrentBinding - 1);
		}
		ImGui::SameLine();
		if (AmigaButton("Skip", ImVec2(BUTTON_WIDTH, 0)))
		{
			SetCurrentBinding(s_iCurrentBinding + 1);
		}
		ImGui::SameLine();
		if (AmigaButton("Cancel", ImVec2(BUTTON_WIDTH, 0)))
		{
			g_controller_map.error_text = "Mapping canceled.";
		}

		if (s_unPendingAdvanceTime && SDL_GetTicks() - s_unPendingAdvanceTime >= 100)
		{
			SetCurrentBinding(s_iCurrentBinding + 1);
		}

		if (s_bBindingComplete)
		{
			ControllerMap_BuildMapping();
			ImGui::CloseCurrentPopup();
			ControllerMap_Close();
		}

		ImGui::EndPopup();
	}
}
