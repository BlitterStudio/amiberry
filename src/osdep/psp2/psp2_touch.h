//
// Created by rsn8887 on 02/05/18.

#ifndef UAE4ALL2_PSP2_TOUCH_H
#define UAE4ALL2_PSP2_TOUCH_H

#include <psp2/touch.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum TouchEventType {
	FINGERDOWN,
	FINGERUP,
	FINGERMOTION,
} TouchEventType;

typedef struct {
	TouchEventType type;
	SceUInt64 timestamp;
	int touchId;
	int fingerId;
	float x;
	float y;
	float dx;
	float dy;
} FingerType;

typedef struct {
	TouchEventType type;
	FingerType tfinger;
} TouchEvent;

/* Touch functions */
void psp2InitTouch(void);
void psp2QuitTouch(void);
void psp2PollTouch(void);
void psp2ProcessTouchEvent(TouchEvent *event);
void psp2ProcessFingerDown(TouchEvent *event);
void psp2ProcessFingerUp(TouchEvent *event);
void psp2ProcessFingerMotion(TouchEvent *event);
void psp2ConvertTouchXYToSDLXY(float *sdl_x, float *sdl_y, int vita_x, int vita_y, int port);
void psp2FinishSimulatedMouseClicks(int port, SceUInt64 currentTime);

#ifdef __cplusplus
}
#endif

#endif
