//
// Created by rsn8887 on 05/18/18.

#if defined(__vita__)
#include <psp2/kernel/processmgr.h>
#define displayWidth 960.0
#define displayHeight 544.0
#endif

#if defined(__SWITCH__)
#define SCE_TOUCH_PORT_MAX_NUM 1
#define displayWidth 1280.0
#define displayHeight 720.0
#endif

#include "switch_touch.h"

#include "math.h"

static void initTouch();
static void preprocessEvents(SDL_Event *event);
static void preprocessFingerDown(SDL_Event *event);
static void preprocessFingerUp(SDL_Event *event);
static void preprocessFingerMotion(SDL_Event *event);

static int psp2RearTouch = 0; // always disable rear_touch for now
extern int lastmx;
extern int lastmy;
static int touch_initialized = 0;
unsigned int _simulatedClickStartTime[SCE_TOUCH_PORT_MAX_NUM][2]; // initiation time of last simulated left or right click (zero if no click)
static int hiresdx = 0;
static int hiresdy = 0;

enum {
	MAX_NUM_FINGERS = 3, // number of fingers to track per panel
	MAX_TAP_TIME = 250, // taps longer than this will not result in mouse click events
	MAX_TAP_MOTION_DISTANCE = 10, // max distance finger motion in Vita screen pixels to be considered a tap
	SIMULATED_CLICK_DURATION = 50, // time in ms how long simulated mouse clicks should be
}; // track three fingers per panel

typedef struct {
	int id; // -1: no touch
	Uint32 timeLastDown;
	int lastX; // last known screen coordinates
	int lastY; // last known screen coordinates
	float lastDownX; // SDL touch coordinates when last pressed down
	float lastDownY; // SDL touch coordinates when last pressed down
} Touch;

Touch _finger[SCE_TOUCH_PORT_MAX_NUM][MAX_NUM_FINGERS]; // keep track of finger status

typedef enum DraggingType {
	DRAG_NONE = 0,
	DRAG_TWO_FINGER,
	DRAG_THREE_FINGER,
} DraggingType;

DraggingType _multiFingerDragging[SCE_TOUCH_PORT_MAX_NUM]; // keep track whether we are currently drag-and-dropping

typedef int bool;
#define false 0;
#define true 1;

static void initTouch() {
	for (int port = 0; port < SCE_TOUCH_PORT_MAX_NUM; port++) {
		for (int i = 0; i < MAX_NUM_FINGERS; i++) {
			_finger[port][i].id = -1;
		}
		_multiFingerDragging[port] = DRAG_NONE;
	}

	for (int port = 0; port < SCE_TOUCH_PORT_MAX_NUM; port++) {
		for (int i = 0; i < 2; i++) {
			_simulatedClickStartTime[port][i] = 0;
		}
	}
}

void SWITCH_HandleTouch(SDL_Event *event) {
	if (!touch_initialized) {
		initTouch();
		touch_initialized = 1;
	}
	preprocessEvents(event);
}

static void preprocessEvents(SDL_Event *event) {

#if defined(__vita__)
	// prevent suspend (scummvm games contain a lot of cutscenes..)
	sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND);
	sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_OLED_OFF);
#endif
	// Supported touch gestures:
	// left mouse click: single finger short tap
	// right mouse click: second finger short tap while first finger is still down
	// pointer motion: single finger drag
	// left button drag and drop: dual finger drag
	// right button drag and drop: triple finger drag
	if (event->type == SDL_FINGERDOWN || event->type == SDL_FINGERUP || event->type == SDL_FINGERMOTION) {
		// front (0) or back (1) panel
		SDL_TouchID port = event->tfinger.touchId;
		if (port < SCE_TOUCH_PORT_MAX_NUM && port >= 0) {
			if (port == 0 || psp2RearTouch) {
				switch (event->type) {
				case SDL_FINGERDOWN:
					preprocessFingerDown(event);
					break;
				case SDL_FINGERUP:
					preprocessFingerUp(event);
					break;
				case SDL_FINGERMOTION:
					preprocessFingerMotion(event);
					break;
				}
			}
		}
	}
}

static void preprocessFingerDown(SDL_Event *event) {
	// front (0) or back (1) panel
	SDL_TouchID port = event->tfinger.touchId;
	// id (for multitouch)
	SDL_FingerID id = event->tfinger.fingerId;

	int x = lastmx;
	int y = lastmy;

	// make sure each finger is not reported down multiple times
	for (int i = 0; i < MAX_NUM_FINGERS; i++) {
		if (_finger[port][i].id == id) {
			_finger[port][i].id = -1;
		}
	}

	// we need the timestamps to decide later if the user performed a short tap (click)
	// or a long tap (drag)
	// we also need the last coordinates for each finger to keep track of dragging
	for (int i = 0; i < MAX_NUM_FINGERS; i++) {
		if (_finger[port][i].id == -1) {
			_finger[port][i].id = id;
			_finger[port][i].timeLastDown = event->tfinger.timestamp;
			_finger[port][i].lastDownX = event->tfinger.x;
			_finger[port][i].lastDownY = event->tfinger.y;
			_finger[port][i].lastX = x;
			_finger[port][i].lastY = y;
			break;
		}
	}
}

static void preprocessFingerUp(SDL_Event *event) {
	// front (0) or back (1) panel
	SDL_TouchID port = event->tfinger.touchId;
	// id (for multitouch)
	SDL_FingerID id = event->tfinger.fingerId;

	// find out how many fingers were down before this event
	int numFingersDown = 0;
	for (int i = 0; i < MAX_NUM_FINGERS; i++) {
		if (_finger[port][i].id >= 0) {
			numFingersDown++;
		}
	}

	int x = lastmx;
	int y = lastmy;

	for (int i = 0; i < MAX_NUM_FINGERS; i++) {
		if (_finger[port][i].id == id) {
			_finger[port][i].id = -1;
			if (!_multiFingerDragging[port]) {
				if ((event->tfinger.timestamp - _finger[port][i].timeLastDown) <= MAX_TAP_TIME) {
					// short (<MAX_TAP_TIME ms) tap is interpreted as right/left mouse click depending on # fingers already down
					// but only if the finger hasn't moved since it was pressed down by more than MAX_TAP_MOTION_DISTANCE pixels
					float xrel = ((event->tfinger.x * displayWidth) - (_finger[port][i].lastDownX * displayWidth));
					float yrel = ((event->tfinger.y * displayHeight) - (_finger[port][i].lastDownY * displayHeight));
					float maxRSquared = (float) (MAX_TAP_MOTION_DISTANCE * MAX_TAP_MOTION_DISTANCE);
					if ((xrel * xrel + yrel * yrel) < maxRSquared) {
						if (numFingersDown == 2 || numFingersDown == 1) {
							Uint8 simulatedButton = 0;
							if (numFingersDown == 2) {
								simulatedButton = SDL_BUTTON_RIGHT;
								// need to raise the button later
								_simulatedClickStartTime[port][1] = event->tfinger.timestamp;
							} else if (numFingersDown == 1) {
								simulatedButton = SDL_BUTTON_LEFT;
								// need to raise the button later
								_simulatedClickStartTime[port][0] = event->tfinger.timestamp;
							}

							event->type = SDL_MOUSEBUTTONDOWN;
							event->button.button = simulatedButton;
							event->button.state = SDL_PRESSED;
							event->button.x = x;
							event->button.y = y;
						}
					}
				}
			} else if (numFingersDown == 1) {
				// when dragging, and the last finger is lifted, the drag is over
				Uint8 simulatedButton = 0;
				if (_multiFingerDragging[port] == DRAG_THREE_FINGER)
					simulatedButton = SDL_BUTTON_RIGHT;
				else {
					simulatedButton = SDL_BUTTON_LEFT;
				}
				event->type = SDL_MOUSEBUTTONUP;
				event->button.button = simulatedButton;
				event->button.state = SDL_RELEASED;
				event->button.x = x;
				event->button.y = y;
				_multiFingerDragging[port] = DRAG_NONE;
			}
		}
	}
}

static void preprocessFingerMotion(SDL_Event *event) {
	// front (0) or back (1) panel
	SDL_TouchID port = event->tfinger.touchId;
	// id (for multitouch)
	SDL_FingerID id = event->tfinger.fingerId;

	// find out how many fingers were down before this event
	int numFingersDown = 0;
	for (int i = 0; i < MAX_NUM_FINGERS; i++) {
		if (_finger[port][i].id >= 0) {
			numFingersDown++;
		}
	}

	if (numFingersDown >= 1) {
		int x = lastmx;
		int y = lastmy;
		int xrel = (int)(event->tfinger.dx * displayWidth);
		int yrel = (int)(event->tfinger.dy * displayHeight);

		x = lastmx + (int)(event->tfinger.dx * displayWidth);
		y = lastmy + (int)(event->tfinger.dy * displayHeight);

		// update the current finger's coordinates so we can track it later
		for (int i = 0; i < MAX_NUM_FINGERS; i++) {
			if (_finger[port][i].id == id) {
				_finger[port][i].lastX = x;
				_finger[port][i].lastY = y;
			}
		}

		// If we are starting a multi-finger drag, start holding down the mouse button
		if (numFingersDown >= 2) {
			if (!_multiFingerDragging[port]) {
				// only start a multi-finger drag if at least two fingers have been down long enough
				int numFingersDownLong = 0;
				for (int i = 0; i < MAX_NUM_FINGERS; i++) {
					if (_finger[port][i].id >= 0) {
						if (event->tfinger.timestamp - _finger[port][i].timeLastDown > MAX_TAP_TIME) {
							numFingersDownLong++;
						}
					}
				}
				if (numFingersDownLong >= 2) {
					Uint8 simulatedButton = 0;
					if (numFingersDownLong == 2) {
						simulatedButton = SDL_BUTTON_LEFT;
						_multiFingerDragging[port] = DRAG_TWO_FINGER;
					} else {
						simulatedButton = SDL_BUTTON_RIGHT;
						_multiFingerDragging[port] = DRAG_THREE_FINGER;
					}
					int mouseDownX = lastmx;
					int mouseDownY = lastmy;
					SDL_Event ev;
					ev.type = SDL_MOUSEBUTTONDOWN;
					ev.button.button = simulatedButton;
					ev.button.state = SDL_PRESSED;
					ev.button.x = mouseDownX;
					ev.button.y = mouseDownY;
					SDL_PushEvent(&ev);
				}
			}
		}

		//check if this is the "oldest" finger down (or the only finger down), otherwise it will not affect mouse motion
		bool updatePointer = true;
		if (numFingersDown > 1) {
			for (int i = 0; i < MAX_NUM_FINGERS; i++) {
				if (_finger[port][i].id == id) {
					for (int j = 0; j < MAX_NUM_FINGERS; j++) {
						if (_finger[port][j].id >= 0 && (i != j) ) {
							if (_finger[port][j].timeLastDown < _finger[port][i].timeLastDown) {
								updatePointer = false;
							}
						}
					}
				}
			}
		}
		if (updatePointer) {
			const int slowdown = 16;
			hiresdx += (int)(event->tfinger.dx * displayWidth * slowdown);
			hiresdy += (int)(event->tfinger.dy * displayHeight * slowdown);
			int xrel = hiresdx / slowdown;
			int yrel = hiresdy / slowdown;

			if (xrel || yrel) {
				int x = lastmx + xrel;
				int y = lastmy + yrel;
				event->type = SDL_MOUSEMOTION;
				event->motion.x = x;
				event->motion.y = y;
				event->motion.xrel = xrel;
				event->motion.yrel = yrel;
				hiresdx %= slowdown;
				hiresdy %= slowdown;
			}
		}
	}
}

void SWITCH_FinishSimulatedMouseClicks() {
	for (int port = 0; port < SCE_TOUCH_PORT_MAX_NUM; port++) {
		for (int i = 0; i < 2; i++) {
			if (_simulatedClickStartTime[port][i] != 0) {
				Uint32 currentTime = SDL_GetTicks();
				if (currentTime - _simulatedClickStartTime[port][i] >= SIMULATED_CLICK_DURATION) {
					int simulatedButton;
					if (i == 0) {
						simulatedButton = SDL_BUTTON_LEFT;
					} else {
						simulatedButton = SDL_BUTTON_RIGHT;
					}
					SDL_Event ev;
					ev.type = SDL_MOUSEBUTTONUP;
					ev.button.button = simulatedButton;
					ev.button.state = SDL_RELEASED;
					ev.button.x = lastmx;
					ev.button.y = lastmy;
					SDL_PushEvent(&ev);

					_simulatedClickStartTime[port][i] = 0;
				}
			}
		}
	}
}
