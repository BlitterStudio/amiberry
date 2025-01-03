#include <guisan.hpp>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "config.h"
#include "gui_handling.h"

#include "options.h"
#include "amiberry_gfx.h"
#include "amiberry_input.h"
#include "fsdb_host.h"
#include <string>

enum
{
	SCREEN_WIDTH = 640,
	SCREEN_HEIGHT = 500
};

enum
{
	MARKER_BUTTON = 1,
	MARKER_AXIS = 2
};

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

#define BINDING_COUNT (SDL_CONTROLLER_BUTTON_MAX + SDL_CONTROLLER_BINDING_AXIS_MAX)

// We use the offset values here for positioning the marker, to avoid messing with the pixel coordinates below
static constexpr int x_offset = 145;
static constexpr int y_offset = 255;

static struct
{
	int x, y;
	double angle;
	int marker;
	std::string input;
} s_arrBindingDisplay[] = {
	{ 387, 167, 0.0, MARKER_BUTTON, "Button A (South)"}, /* SDL_CONTROLLER_BUTTON_A */
	{ 431, 132, 0.0, MARKER_BUTTON, "Button B (East)"}, /* SDL_CONTROLLER_BUTTON_B */
	{ 342, 132, 0.0, MARKER_BUTTON, "Button X (West)"}, /* SDL_CONTROLLER_BUTTON_X */
	{ 389, 101, 0.0, MARKER_BUTTON, "Button Y (North)"}, /* SDL_CONTROLLER_BUTTON_Y */
	{ 174, 132, 0.0, MARKER_BUTTON, "Button Back"}, /* SDL_CONTROLLER_BUTTON_BACK */
	{ 232, 128, 0.0, MARKER_BUTTON, "Button Guide"}, /* SDL_CONTROLLER_BUTTON_GUIDE */
	{ 289, 132, 0.0, MARKER_BUTTON, "Button Start"}, /* SDL_CONTROLLER_BUTTON_START */
	{  75, 154, 0.0, MARKER_BUTTON, "Left Stick button"}, /* SDL_CONTROLLER_BUTTON_LEFTSTICK */
	{ 305, 230, 0.0, MARKER_BUTTON, "Right Stick button"}, /* SDL_CONTROLLER_BUTTON_RIGHTSTICK */
	{  77,  40, 0.0, MARKER_BUTTON, "Left Shoulder"}, /* SDL_CONTROLLER_BUTTON_LEFTSHOULDER */
	{ 396,  36, 0.0, MARKER_BUTTON, "Right Shoulder"}, /* SDL_CONTROLLER_BUTTON_RIGHTSHOULDER */
	{ 154, 188, 0.0, MARKER_BUTTON, "D-PAD Up"}, /* SDL_CONTROLLER_BUTTON_DPAD_UP */
	{ 154, 249, 0.0, MARKER_BUTTON, "D-PAD Down"}, /* SDL_CONTROLLER_BUTTON_DPAD_DOWN */
	{ 116, 217, 0.0, MARKER_BUTTON, "D-PAD Left"}, /* SDL_CONTROLLER_BUTTON_DPAD_LEFT */
	{ 186, 217, 0.0, MARKER_BUTTON, "D-PAD Right"}, /* SDL_CONTROLLER_BUTTON_DPAD_RIGHT */
#if SDL_VERSION_ATLEAST(2,0,14)
	{ 232, 174, 0.0, MARKER_BUTTON, "Button Misc1"}, /* SDL_CONTROLLER_BUTTON_MISC1 */
	{ 132, 135, 0.0, MARKER_BUTTON, "Button Paddle 1"}, /* SDL_CONTROLLER_BUTTON_PADDLE1 */
	{ 330, 135, 0.0, MARKER_BUTTON, "Button Paddle 2"}, /* SDL_CONTROLLER_BUTTON_PADDLE2 */
	{ 132, 175, 0.0, MARKER_BUTTON, "Button Paddle 3"}, /* SDL_CONTROLLER_BUTTON_PADDLE3 */
	{ 330, 175, 0.0, MARKER_BUTTON, "Button Paddle 4"}, /* SDL_CONTROLLER_BUTTON_PADDLE4 */
	{   0,   0, 0.0, MARKER_BUTTON, "PS4/PS5 Touchpad button"}, /* SDL_CONTROLLER_BUTTON_TOUCHPAD */
#endif
	{  74, 153, 270.0, MARKER_AXIS, "Left Stick X Negative"}, /* SDL_CONTROLLER_BINDING_AXIS_LEFTX_NEGATIVE */
	{  74, 153, 90.0,  MARKER_AXIS, "Left Stick X Positive"}, /* SDL_CONTROLLER_BINDING_AXIS_LEFTX_POSITIVE */
	{  74, 153, 0.0,   MARKER_AXIS, "Left Stick Y Negative"}, /* SDL_CONTROLLER_BINDING_AXIS_LEFTY_NEGATIVE */
	{  74, 153, 180.0, MARKER_AXIS, "Left Stick Y Positive"}, /* SDL_CONTROLLER_BINDING_AXIS_LEFTY_POSITIVE */
	{ 306, 231, 270.0, MARKER_AXIS, "Right Stick X Negative"}, /* SDL_CONTROLLER_BINDING_AXIS_RIGHTX_NEGATIVE */
	{ 306, 231, 90.0,  MARKER_AXIS, "Right Stick X Positive"}, /* SDL_CONTROLLER_BINDING_AXIS_RIGHTX_POSITIVE */
	{ 306, 231, 0.0,   MARKER_AXIS, "Right Stick Y Negative"}, /* SDL_CONTROLLER_BINDING_AXIS_RIGHTY_NEGATIVE */
	{ 306, 231, 180.0, MARKER_AXIS, "Right Stick Y Positive"}, /* SDL_CONTROLLER_BINDING_AXIS_RIGHTY_POSITIVE */
	{  91, -20, 180.0, MARKER_AXIS, "Left Trigger"}, /* SDL_CONTROLLER_BINDING_AXIS_TRIGGERLEFT */
	{ 375, -20, 180.0, MARKER_AXIS, "Right Trigger"}, /* SDL_CONTROLLER_BINDING_AXIS_TRIGGERRIGHT */
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

static bool done = SDL_FALSE;
static bool bind_touchpad = SDL_FALSE;

#ifdef AMIBERRY
std::string result;
std::string info_text =
"Press the buttons on your controller when indicated\n\
If you want to correct a mistake, press backspace or the\n\
back button on your device. To skip a button, press SPACE\n\
or click/touch the screen. To exit, press ESC";

static gcn::Window* wndControllerMap;
static gcn::TextBox* txtInformation;
static gcn::Label* lblMessage;
static gcn::Label* lblPressButtonAxis;
static gcn::Icon* background_front_icon, *background_back_icon;
static gcn::Image* background_front_image, *background_back_image;

static void InitControllerMap()
{
	done = SDL_FALSE;
	s_bBindingComplete = SDL_FALSE;

	wndControllerMap = new gcn::Window("Controller Map");
	wndControllerMap->setSize(SCREEN_WIDTH, SCREEN_HEIGHT);
	wndControllerMap->setPosition((GUI_WIDTH - SCREEN_WIDTH) / 2, (GUI_HEIGHT - SCREEN_HEIGHT) / 2);
	wndControllerMap->setBaseColor(gui_base_color);
	wndControllerMap->setForegroundColor(gui_foreground_color);
	wndControllerMap->setTitleBarHeight(TITLEBAR_HEIGHT);
	wndControllerMap->setMovable(false);

	txtInformation = new gcn::TextBox(info_text);
	txtInformation->setBackgroundColor(gui_base_color);
	txtInformation->setForegroundColor(gui_foreground_color);
	txtInformation->setEditable(false);
	txtInformation->setFrameSize(0);

	lblMessage = new gcn::Label("Press: ");
	lblPressButtonAxis = new gcn::Label("");

	wndControllerMap->add(txtInformation, DISTANCE_BORDER, DISTANCE_BORDER);
	const auto posY = txtInformation->getY() + txtInformation->getHeight() + DISTANCE_NEXT_Y;
	wndControllerMap->add(lblMessage, SCREEN_WIDTH / 3, posY);
	wndControllerMap->add(lblPressButtonAxis, lblMessage->getX() + lblMessage->getWidth() + 8, posY);

	gui_top->add(wndControllerMap);

	wndControllerMap->requestModalFocus();
}

static void ExitControllerMap()
{
	wndControllerMap->releaseModalFocus();
	gui_top->remove(wndControllerMap);

	delete txtInformation;
	delete lblMessage;
	delete lblPressButtonAxis;
	delete wndControllerMap;
	delete background_front_icon;
	delete background_back_icon;
	delete background_front_image;
	delete background_back_image;
}
#endif

SDL_Texture*
LoadTexture(SDL_Renderer* renderer, const char* file, SDL_bool transparent)
{
	/* Load the sprite image */
	SDL_Surface* temp = IMG_Load(file);
	if (temp == nullptr)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't load %s: %s", file, SDL_GetError());
		return nullptr;
	}

	/* Set transparent pixel as the pixel at (0,0) */
	if (transparent)
	{
		if (temp->format->palette)
		{
			SDL_SetColorKey(temp, SDL_TRUE, *static_cast<Uint8*>(temp->pixels));
		}
	}

	/* Create textures from the image */
	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, temp);
	if (!texture)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create texture: %s\n", SDL_GetError());
		SDL_FreeSurface(temp);
		return nullptr;
	}
	SDL_FreeSurface(temp);

	/* We're ready to roll. :) */
	return texture;
}

static int
StandardizeAxisValue(int nValue)
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

static void
SetCurrentBinding(int iBinding)
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
		!bind_touchpad)
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

static bool
BBindingContainsBinding(const SDL_GameControllerExtendedBind* pBindingA,
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
		/* Not reached */
	default:
		return SDL_memcmp(pBindingA, pBindingB, sizeof(*pBindingA)) == 0;
	}
}

static void
ConfigureBinding(const SDL_GameControllerExtendedBind* pBinding)
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

#ifdef DEBUG_CONTROLLERMAP
	switch (pBinding->bindType)
	{
	case SDL_CONTROLLER_BINDTYPE_NONE:
		break;
	case SDL_CONTROLLER_BINDTYPE_BUTTON:
		SDL_Log("Configuring button binding for button %d\n", pBinding->value.button);
		break;
	case SDL_CONTROLLER_BINDTYPE_AXIS:
		SDL_Log("Configuring axis binding for axis %d %d/%d committed = %s\n", pBinding->value.axis.axis, pBinding->value.axis.axis_min, pBinding->value.axis.axis_max, pBinding->committed ? "true" : "false");
		break;
	case SDL_CONTROLLER_BINDTYPE_HAT:
		SDL_Log("Configuring hat binding for hat %d %d\n", pBinding->value.hat.hat, pBinding->value.hat.hat_mask);
		break;
	}
#endif /* DEBUG_CONTROLLERMAP */

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

static bool
BMergeAxisBindings(int iIndex)
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

static void
WatchJoystick(SDL_Joystick* joystick)
{
	AmigaMonitor* mon = &AMonitors[0];
	SDL_Texture* button, *axis, *marker;
	const char* name = nullptr;
	SDL_Event event;
	SDL_Rect dst;
	Uint8 alpha = 200, alpha_step = -1;
	Uint32 alpha_ticks = 0;
	SDL_JoystickID nJoystickID;

	background_front_image = gcn::Image::load(prefix_with_data_path("controllermap.png"));
	background_front_icon = new gcn::Icon(background_front_image);

	background_back_image = gcn::Image::load(prefix_with_data_path("controllermap_back.png"));
	background_back_icon = new gcn::Icon(background_back_image);

	button = LoadTexture(mon->gui_renderer, prefix_with_data_path("button.png").c_str(), SDL_TRUE);
	axis = LoadTexture(mon->gui_renderer, prefix_with_data_path("axis.png").c_str(), SDL_TRUE);

	/* Print info about the joystick we are watching */
	name = SDL_JoystickName(joystick);
	write_log("Watching joystick %d: (%s)\n", SDL_JoystickInstanceID(joystick),
			name ? name : "Unknown Joystick");
	write_log("Joystick has %d axes, %d hats, %d balls, and %d buttons\n",
			SDL_JoystickNumAxes(joystick), SDL_JoystickNumHats(joystick),
			SDL_JoystickNumBalls(joystick), SDL_JoystickNumButtons(joystick));

	nJoystickID = SDL_JoystickInstanceID(joystick);

	s_nNumAxes = SDL_JoystickNumAxes(joystick);
	s_arrAxisState = static_cast<AxisState*>(SDL_calloc(s_nNumAxes, sizeof(*s_arrAxisState)));

	/* Skip any spurious events at start */
	while (SDL_PollEvent(&event) > 0)
	{
	}

	wndControllerMap->add(background_back_icon, (SCREEN_WIDTH - background_back_icon->getWidth()) / 2, lblMessage->getY() + lblMessage->getHeight() + DISTANCE_NEXT_Y);
	wndControllerMap->add(background_front_icon, (SCREEN_WIDTH - background_front_icon->getWidth()) / 2, lblMessage->getY() + lblMessage->getHeight() + DISTANCE_NEXT_Y);
	background_back_icon->setVisible(false);
	background_front_icon->setVisible(false);

	/* Loop, getting joystick events! */
	while (!done && !s_bBindingComplete)
	{
		int iElement = s_arrBindingOrder[s_iCurrentBinding];

		switch (s_arrBindingDisplay[iElement].marker)
		{
		case MARKER_AXIS:
			marker = axis;
			break;
		case MARKER_BUTTON:
			marker = button;
			break;
		default:
			break;
		}

		lblPressButtonAxis->setCaption(s_arrBindingDisplay[iElement].input);
		lblPressButtonAxis->adjustSize();

		dst.x = s_arrBindingDisplay[iElement].x + x_offset;
		dst.y = s_arrBindingDisplay[iElement].y + y_offset;

		SDL_QueryTexture(marker, nullptr, nullptr, &dst.w, &dst.h);

		if (SDL_GetTicks() - alpha_ticks > 5)
		{
			alpha_ticks = SDL_GetTicks();
			alpha += alpha_step;
			if (alpha == 255)
			{
				alpha_step = -1;
			}
			if (alpha < 128)
			{
				alpha_step = 1;
			}
		}

#if SDL_VERSION_ATLEAST(2,0,14)
		if (s_arrBindingOrder[s_iCurrentBinding] >= SDL_CONTROLLER_BUTTON_PADDLE1 &&
			s_arrBindingOrder[s_iCurrentBinding] <= SDL_CONTROLLER_BUTTON_PADDLE4) {
			//SDL_RenderCopy(sdl_renderer, background_back, NULL, NULL);
			background_back_icon->setVisible(true);
			background_front_icon->setVisible(false);
		}
		else
#endif
		{
			//SDL_RenderCopy(sdl_renderer, background_front, NULL, NULL);
			background_front_icon->setVisible(true);
			background_back_icon->setVisible(false);
		}

		// Update guisan
		uae_gui->logic();

		SDL_RenderClear(mon->gui_renderer);

		uae_gui->draw();

		SDL_UpdateTexture(gui_texture, nullptr, gui_screen->pixels, gui_screen->pitch);
		if (amiberry_options.rotation_angle == 0 || amiberry_options.rotation_angle == 180)
			renderQuad = { 0, 0, gui_screen->w, gui_screen->h };
		else
			renderQuad = { -(GUI_WIDTH - GUI_HEIGHT) / 2, (GUI_WIDTH - GUI_HEIGHT) / 2, gui_screen->w, gui_screen->h };
		SDL_RenderCopyEx(mon->gui_renderer, gui_texture, nullptr, &renderQuad, amiberry_options.rotation_angle, nullptr, SDL_FLIP_NONE);

		// Blit marker on top
		SDL_SetTextureAlphaMod(marker, alpha);
		SDL_SetTextureColorMod(marker, 10, 255, 21);
		SDL_RenderCopyEx(mon->gui_renderer, marker, nullptr, &dst, s_arrBindingDisplay[iElement].angle, nullptr, SDL_FLIP_NONE);
		SDL_RenderPresent(mon->gui_renderer);

		while (SDL_PollEvent(&event) > 0)
		{
			switch (event.type)
			{
			case SDL_JOYDEVICEREMOVED:
				if (event.jaxis.which == nJoystickID)
				{
					done = SDL_TRUE;
				}
				break;
			case SDL_JOYAXISMOTION:
				if (event.jaxis.which == nJoystickID)
				{
					constexpr int MAX_ALLOWED_JITTER = SDL_JOYSTICK_AXIS_MAX / 80; /* ShanWan PS3 controller needed 96 */
					AxisState* pAxisState = &s_arrAxisState[event.jaxis.axis];
					int nValue = event.jaxis.value;
					int nCurrentDistance, nFarthestDistance;
					if (!pAxisState->m_bMoving)
					{
						Sint16 nInitialValue;
						pAxisState->m_bMoving = SDL_JoystickGetAxisInitialState(
							joystick, event.jaxis.axis, &nInitialValue);
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

#ifdef DEBUG_CONTROLLERMAP
					SDL_Log("AXIS %d nValue %d nCurrentDistance %d nFarthestDistance %d\n", event.jaxis.axis, nValue, nCurrentDistance, nFarthestDistance);
#endif
					if (nFarthestDistance >= 16000)
					{
						/* If we've gone out far enough and started to come back, let's bind this axis */
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
				}
				break;
			case SDL_JOYHATMOTION:
				if (event.jhat.which == nJoystickID)
				{
					if (event.jhat.value != SDL_HAT_CENTERED)
					{
						SDL_GameControllerExtendedBind binding;

#ifdef DEBUG_CONTROLLERMAP
						SDL_Log("HAT %d %d\n", event.jhat.hat, event.jhat.value);
#endif
						SDL_zero(binding);
						binding.bindType = SDL_CONTROLLER_BINDTYPE_HAT;
						binding.value.hat.hat = event.jhat.hat;
						binding.value.hat.hat_mask = event.jhat.value;
						binding.committed = SDL_TRUE;
						ConfigureBinding(&binding);
					}
				}
				break;
			case SDL_JOYBALLMOTION:
				break;
			case SDL_JOYBUTTONDOWN:
				if (event.jbutton.which == nJoystickID)
				{
					SDL_GameControllerExtendedBind binding;

#ifdef DEBUG_CONTROLLERMAP
					SDL_Log("BUTTON %d\n", event.jbutton.button);
#endif
					SDL_zero(binding);
					binding.bindType = SDL_CONTROLLER_BINDTYPE_BUTTON;
					binding.value.button = event.jbutton.button;
					binding.committed = SDL_TRUE;
					ConfigureBinding(&binding);
				}
				break;
			case SDL_FINGERDOWN:
			case SDL_MOUSEBUTTONDOWN:
				/* Skip this step */
				SetCurrentBinding(s_iCurrentBinding + 1);
				break;
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_BACKSPACE || event.key.keysym.sym == SDLK_AC_BACK)
				{
					SetCurrentBinding(s_iCurrentBinding - 1);
					break;
				}
				if (event.key.keysym.sym == SDLK_SPACE)
				{
					SetCurrentBinding(s_iCurrentBinding + 1);
					break;
				}

				if ((event.key.keysym.sym != SDLK_ESCAPE))
				{
					break;
				}
				/* Fall through to signal quit */
			case SDL_QUIT:
				done = SDL_TRUE;
				break;
			default:
				break;
			}
		}

		SDL_Delay(15);

		/* Wait 100 ms for joystick events to stop coming in,
		   in case a controller sends multiple events for a single control (e.g. axis and button for trigger)
		*/
		if (s_unPendingAdvanceTime && SDL_GetTicks() - s_unPendingAdvanceTime >= 100)
		{
			SetCurrentBinding(s_iCurrentBinding + 1);
		}
	}

	if (s_bBindingComplete)
	{
		char mapping[1024];
		char trimmed_name[128];
		char* spot;
		int iIndex;
		char pszElement[12];

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
		SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joystick), mapping, std::size(mapping));
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

		write_log("Mapping:\n\n%s\n\n", mapping);
		result = std::string(mapping);
	}

	SDL_free(s_arrAxisState);
	s_arrAxisState = nullptr;

	SDL_DestroyTexture(axis);
	SDL_DestroyTexture(button);
}

std::string
show_controller_map(int device, bool map_touchpad)
{
	const AmigaMonitor* mon = &AMonitors[0];

	const char* name;
	SDL_Joystick* joystick;
	bind_touchpad = map_touchpad;
	
	while (!done && SDL_NumJoysticks() == 0) {
		SDL_Event event;

		while (SDL_PollEvent(&event) > 0) {
			switch (event.type) {
			case SDL_KEYDOWN:
				if ((event.key.keysym.sym != SDLK_ESCAPE)) {
					break;
				}
#if SDL_VERSION_ATLEAST(2,0,18)
				SDL_FALLTHROUGH;
#endif
			case SDL_QUIT:
				done = SDL_TRUE;
				break;
			default:
				break;
			}
		}

		SDL_RenderPresent(mon->gui_renderer);
	}
#ifdef DEBUG
	/* Print information about the joysticks */
	write_log("There are %d joysticks attached\n", SDL_NumJoysticks());
	for (int i = 0; i < SDL_NumJoysticks(); ++i)
	{
		name = SDL_JoystickNameForIndex(i);
		write_log("Joystick %d: %s\n", i, name ? name : "Unknown Joystick");
		joystick = SDL_JoystickOpen(i);
		if (joystick == nullptr)
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_JoystickOpen(%d) failed: %s\n", i,
						 SDL_GetError());
		}
		else
		{
			char guid[64];
			SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joystick),
									  guid, sizeof(guid));
			write_log("       axes: %d\n", SDL_JoystickNumAxes(joystick));
			write_log("      balls: %d\n", SDL_JoystickNumBalls(joystick));
			write_log("       hats: %d\n", SDL_JoystickNumHats(joystick));
			write_log("    buttons: %d\n", SDL_JoystickNumButtons(joystick));
			write_log("instance id: %d\n", SDL_JoystickInstanceID(joystick));
			write_log("       guid: %s\n", guid);
			write_log("    VID/PID: 0x%.4x/0x%.4x\n", SDL_JoystickGetVendor(joystick), SDL_JoystickGetProduct(joystick));
			SDL_JoystickClose(joystick);
		}
	}
#endif
	joystick = SDL_JoystickOpen(device);
	if (joystick == nullptr)
	{
		write_log("Couldn't open joystick %d: %s\n", device, SDL_GetError());
	}
	else
	{
		InitControllerMap();
		WatchJoystick(joystick);
		SDL_JoystickClose(joystick);
		ExitControllerMap();
	}

	return result;
}
