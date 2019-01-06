//
// Created by rsn8887 on 02/05/18.

#include <SDL.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/touch.h>
#include "psp2_touch.h"
//#include "math.h"

typedef int bool;
#define false 0;
#define true 1;

SceTouchData touch_old[SCE_TOUCH_PORT_MAX_NUM];
SceTouchData touch[SCE_TOUCH_PORT_MAX_NUM];

SceTouchPanelInfo panelinfo[SCE_TOUCH_PORT_MAX_NUM];

float aAWidth[SCE_TOUCH_PORT_MAX_NUM];
float aAHeight[SCE_TOUCH_PORT_MAX_NUM];
float dispWidth[SCE_TOUCH_PORT_MAX_NUM];
float dispHeight[SCE_TOUCH_PORT_MAX_NUM];
float forcerange[SCE_TOUCH_PORT_MAX_NUM];

extern int mainMenu_touchControls;
extern int lastmx;
extern int lastmy;
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
	int timeLastDown;
	float lastDownX;
	float lastDownY;
} Touch;

Touch _finger[SCE_TOUCH_PORT_MAX_NUM][MAX_NUM_FINGERS]; // keep track of finger status

typedef enum DraggingType {
	DRAG_NONE = 0,
	DRAG_TWO_FINGER,
	DRAG_THREE_FINGER,
} DraggingType;

DraggingType _multiFingerDragging[SCE_TOUCH_PORT_MAX_NUM]; // keep track whether we are currently drag-and-dropping

int _simulatedClickStartTime[SCE_TOUCH_PORT_MAX_NUM][2]; // initiation time of last simulated left or right click (zero if no click)

void psp2InitTouch(void) {
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, SCE_TOUCH_SAMPLING_STATE_START);
	sceTouchEnableTouchForce(SCE_TOUCH_PORT_FRONT);
	sceTouchEnableTouchForce(SCE_TOUCH_PORT_BACK);

	SceTouchPanelInfo panelinfo[SCE_TOUCH_PORT_MAX_NUM];
	for(int port = 0; port < SCE_TOUCH_PORT_MAX_NUM; port++) {
		sceTouchGetPanelInfo(port, &panelinfo[port]);
		aAWidth[port] = (float)(panelinfo[port].maxAaX - panelinfo[port].minAaX);
		aAHeight[port] = (float)(panelinfo[port].maxAaY - panelinfo[port].minAaY);
		dispWidth[port] = (float)(panelinfo[port].maxDispX - panelinfo[port].minDispX);
		dispHeight[port] = (float)(panelinfo[port].maxDispY - panelinfo[port].minDispY);
		forcerange[port] = (float)(panelinfo[port].maxForce - panelinfo[port].minForce);
	}
	
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

void psp2QuitTouch(void) {
	sceTouchDisableTouchForce(SCE_TOUCH_PORT_FRONT);
	sceTouchDisableTouchForce(SCE_TOUCH_PORT_BACK);
}

void psp2PollTouch(void) {
	int finger_id = 0;
	
	memcpy(touch_old, touch, sizeof(touch_old));

	for(int port = 0; port < SCE_TOUCH_PORT_MAX_NUM; port++) {
		if ((port == SCE_TOUCH_PORT_FRONT) || (mainMenu_touchControls == 2 && port == SCE_TOUCH_PORT_BACK)) {
			sceTouchPeek(port, &touch[port], 1);
			psp2FinishSimulatedMouseClicks(port, touch[port].timeStamp / 1000);
			if (touch[port].reportNum > 0) {
				for (int i = 0; i < touch[port].reportNum; i++) {
					// adjust coordinates and forces to return normalized values
					// for the front, screen area is used as a reference (for direct touch)
					// e.g. touch_x = 1.0 corresponds to screen_x = 960
					// for the back panel, the active touch area is used as reference
					float x = 0;
					float y = 0;
					psp2ConvertTouchXYToSDLXY(&x, &y, touch[port].report[i].x, touch[port].report[i].y, port);
					finger_id = touch[port].report[i].id;

					// Send an initial touch if finger hasn't been down
					bool hasBeenDown = false;
					int j = 0;
					if (touch_old[port].reportNum > 0) {
						for (j = 0; j < touch_old[port].reportNum; j++) {
							if (touch_old[port].report[j].id == touch[port].report[i].id ) {
								hasBeenDown = true;
								break;
							}
						}
					}
					if (!hasBeenDown) {
						TouchEvent ev;
						ev.type = FINGERDOWN;
						ev.tfinger.timestamp = touch[port].timeStamp / 1000;
						ev.tfinger.touchId = port;
						ev.tfinger.fingerId = finger_id;
						ev.tfinger.x = x;
						ev.tfinger.y = y;
						psp2ProcessTouchEvent(&ev);
					} else {
						// If finger moved, send motion event instead
						if (touch_old[port].report[j].x != touch[port].report[i].x ||
							touch_old[port].report[j].y != touch[port].report[i].y) {
							float oldx = 0;
							float oldy = 0;
							psp2ConvertTouchXYToSDLXY(&oldx, &oldy, touch_old[port].report[j].x, touch_old[port].report[j].y, port);
							TouchEvent ev;
							ev.type = FINGERMOTION;
							ev.tfinger.timestamp = touch[port].timeStamp / 1000;
							ev.tfinger.touchId = port;
							ev.tfinger.fingerId = finger_id;
							ev.tfinger.x = x;
							ev.tfinger.y = y;
							ev.tfinger.dx = x - oldx;
							ev.tfinger.dy = y - oldy;
							psp2ProcessTouchEvent(&ev);
						}
					}
				}
			}
			// some fingers might have been let go
			if (touch_old[port].reportNum > 0) {
				for (int i = 0; i < touch_old[port].reportNum; i++) {
					int finger_up = 1;
					if (touch[port].reportNum > 0) {
						for (int j = 0; j < touch[port].reportNum; j++) {
							if (touch[port].report[j].id == touch_old[port].report[i].id ) {
								finger_up = 0;
							}
						}
					}
					if (finger_up == 1) {
						float x = 0;
						float y = 0;
						psp2ConvertTouchXYToSDLXY(&x, &y, touch_old[port].report[i].x, touch_old[port].report[i].y, port);
						finger_id = touch_old[port].report[i].id;
						// Finger released from screen
						TouchEvent ev;
						ev.type = FINGERUP;
						ev.tfinger.timestamp = touch[port].timeStamp / 1000;
						ev.tfinger.touchId = port;
						ev.tfinger.fingerId = finger_id;
						ev.tfinger.x = x;
						ev.tfinger.y = y;
						psp2ProcessTouchEvent(&ev);
					}
				}
			}
		}
	}
}

void psp2ConvertTouchXYToSDLXY(float *sdl_x, float *sdl_y, int vita_x, int vita_y, int port) {
	float x = 0;
	float y = 0;
	if (port == SCE_TOUCH_PORT_FRONT) {
		x = (vita_x - panelinfo[port].minDispX) / dispWidth[port];
		y = (vita_y - panelinfo[port].minDispY) / dispHeight[port];
	} else {
		x = (vita_x - panelinfo[port].minAaX) / aAWidth[port];
		y = (vita_y - panelinfo[port].minAaY) / aAHeight[port];				
	}
	if (x < 0.0) {
		x = 0.0;
	} else if (x > 1.0) {
		x = 1.0;
	}
	if (y < 0.0) {
		y = 0.0;
	} else if (y > 1.0) {
		y = 1.0;
	}
	*sdl_x = x;
	*sdl_y = y;
}

void psp2ProcessTouchEvent(TouchEvent *event) {
	// Supported touch gestures:
	// left mouse click: single finger short tap
	// right mouse click: second finger short tap while first finger is still down
	// pointer motion: single finger drag
	// drag and drop: dual finger drag
	if (event->type == FINGERDOWN || event->type == FINGERUP || event->type == FINGERMOTION) {
		// front (0) or back (1) panel
		int port = event->tfinger.touchId;
		if (port >= 0 && port < SCE_TOUCH_PORT_MAX_NUM) {
			switch (event->type) {
			case FINGERDOWN:
				psp2ProcessFingerDown(event);
				break;
			case FINGERUP:
				psp2ProcessFingerUp(event);
				break;
			case FINGERMOTION:
				psp2ProcessFingerMotion(event);
				break;
			}
		}
	}
}

void psp2ProcessFingerDown(TouchEvent *event) {
	// front (0) or back (1) panel
	int port = event->tfinger.touchId;
	// id (for multitouch)
	int id = event->tfinger.fingerId;

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
			break;
		}
	}
}

void psp2ProcessFingerUp(TouchEvent *event) {
	// front (0) or back (1) panel
	int port = event->tfinger.touchId;
	// id (for multitouch)
	int id = event->tfinger.fingerId;

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
					float xrel = ((event->tfinger.x * 960.0) - (_finger[port][i].lastDownX * 960.0));
					float yrel = ((event->tfinger.y * 544.0) - (_finger[port][i].lastDownY * 544.0));
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

							SDL_Event ev0;
							ev0.type = SDL_MOUSEBUTTONDOWN;
							ev0.button.button = simulatedButton;
							ev0.button.x = x;
							ev0.button.y = y;
							SDL_PushEvent(&ev0);
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
				SDL_Event ev0;
				ev0.type = SDL_MOUSEBUTTONUP;
				ev0.button.button = simulatedButton;
				ev0.button.x = x;
				ev0.button.y = y;
				_multiFingerDragging[port] = DRAG_NONE;
				SDL_PushEvent(&ev0);
			}
		}
	}
}

void psp2ProcessFingerMotion(TouchEvent *event) {
	// front (0) or back (1) panel
	int port = event->tfinger.touchId;
	// id (for multitouch)
	int id = event->tfinger.fingerId;

	// find out how many fingers were down before this event
	int numFingersDown = 0;
	for (int i = 0; i < MAX_NUM_FINGERS; i++) {
		if (_finger[port][i].id >= 0) {
			numFingersDown++;
		}
	}

	if (numFingersDown >= 1) {
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
			hiresdx += (int)(event->tfinger.dx * 960.0 * slowdown);
			hiresdy += (int)(event->tfinger.dy * 544.0 * slowdown);
			int xrel = hiresdx / slowdown;
			int yrel = hiresdy / slowdown;

			if (xrel || yrel) {
				int x = lastmx + xrel;
				int y = lastmy + yrel;
				SDL_Event ev0;
				ev0.type = SDL_MOUSEMOTION;
				ev0.motion.x = x;
				ev0.motion.y = y;
				ev0.motion.xrel = xrel;
				ev0.motion.yrel = yrel;
				SDL_PushEvent(&ev0);

				hiresdx %= slowdown;
				hiresdy %= slowdown;
			}
		}
	}
}

void psp2FinishSimulatedMouseClicks(int port, SceUInt64 currentTime) {
	for (int i = 0; i < 2; i++) {
		if (_simulatedClickStartTime[port][i] != 0) {
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
				ev.button.x = lastmx;
				ev.button.y = lastmy;
				SDL_PushEvent(&ev);

				_simulatedClickStartTime[port][i] = 0;
			}
		}
	}
}
