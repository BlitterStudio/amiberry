 /* 
  * UAE - The Un*x Amiga Emulator
  * 
  * Joystick emulation for Linux and BSD. They share too much code to
  * split this file.
  * 
  * Copyright 1997 Bernd Schmidt
  * Copyright 1998 Krister Walfridsson
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "config.h"
#include "uae.h"
#include "options.h"
#include "memory.h"
#include "custom.h"
#include "joystick.h"
#include "SDL.h"

#ifdef GP2X
#include "gp2x.h"
#include "xwin.h"
#if defined(PANDORA) || defined (ANDROIDSDL)
extern int dpadUp;
extern int dpadDown;
extern int dpadLeft;
extern int dpadRight;
extern int buttonA;
extern int buttonB;
extern int buttonX;
extern int buttonY;
extern int triggerL;
extern int triggerR;
#endif

static int delay=0;
#endif


static int nr_joysticks;

static SDL_Joystick *uae4all_joy0, *uae4all_joy1;

void read_joystick(int nr, unsigned int *dir, int *button)
{
  int left = 0, right = 0, top = 0, bot = 0, upRight=0, downRight=0, upLeft=0, downLeft=0, x=0, y=0, a=0, b=0;
  SDL_Joystick *joy = nr == 0 ? uae4all_joy0 : uae4all_joy1;

  *dir = 0;
  *button = 0;

  nr = (~nr)&0x1;

  SDL_JoystickUpdate ();

  #ifdef RASPBERRY
	// Always check joystick state on Raspberry pi.
  #else
	if (!triggerR /*R+dpad = arrow keys*/ && currprefs.pandora_custom_dpad==0)
  #endif
	{
		// get joystick direction via dPad or joystick
		int hat=SDL_JoystickGetHat(joy,0);
		if ((hat & SDL_HAT_RIGHT) || dpadRight || SDL_JoystickGetAxis(joy, 0) > 0) right=1;
		if ((hat & SDL_HAT_LEFT)  || dpadLeft  || SDL_JoystickGetAxis(joy, 0) < 0) left=1;
		if ((hat & SDL_HAT_UP)    || dpadUp    || SDL_JoystickGetAxis(joy, 1) < 0) top=1;
		if ((hat & SDL_HAT_DOWN)  || dpadDown  || SDL_JoystickGetAxis(joy, 1) > 0) bot=1;
		if (currprefs.pandora_joyConf)
		{
			if ((buttonX && currprefs.pandora_jump > -1) || SDL_JoystickGetButton(joy, currprefs.pandora_jump))
				top = 1;
		}
	}

	if(currprefs.pandora_customControls)
	{
	  // get joystick direction via custom keys
		if((currprefs.pandora_custom_A==-5 && buttonA) || (currprefs.pandora_custom_B==-5 && buttonB) || (currprefs.pandora_custom_X==-5 && buttonX) || (currprefs.pandora_custom_Y==-5 && buttonY) || (currprefs.pandora_custom_L==-5 && triggerL) || (currprefs.pandora_custom_R==-5 && triggerR))
			top = 1;
		else if(currprefs.pandora_custom_dpad == 2)
		{
			if((currprefs.pandora_custom_up==-5 && dpadUp) || (currprefs.pandora_custom_down==-5 && dpadDown) || (currprefs.pandora_custom_left==-5 && dpadLeft) || (currprefs.pandora_custom_right==-5 && dpadRight))
				top = 1;
		}

		if((currprefs.pandora_custom_A==-6 && buttonA) || (currprefs.pandora_custom_B==-6 && buttonB) || (currprefs.pandora_custom_X==-6 && buttonX) || (currprefs.pandora_custom_Y==-6 && buttonY) || (currprefs.pandora_custom_L==-6 && triggerL) || (currprefs.pandora_custom_R==-6 && triggerR))
			bot = 1;
		else if(currprefs.pandora_custom_dpad == 2)
		{
			if((currprefs.pandora_custom_up==-6 && dpadUp) || (currprefs.pandora_custom_down==-6 && dpadDown) || (currprefs.pandora_custom_left==-6 && dpadLeft) || (currprefs.pandora_custom_right==-6 && dpadRight))
				bot = 1;
		}

		if((currprefs.pandora_custom_A==-7 && buttonA) || (currprefs.pandora_custom_B==-7 && buttonB) || (currprefs.pandora_custom_X==-7 && buttonX) || (currprefs.pandora_custom_Y==-7 && buttonY) || (currprefs.pandora_custom_L==-7 && triggerL) || (currprefs.pandora_custom_R==-7 && triggerR))
			left = 1;
		else if(currprefs.pandora_custom_dpad == 2)
		{
			if((currprefs.pandora_custom_up==-7 && dpadUp) || (currprefs.pandora_custom_down==-7 && dpadDown) || (currprefs.pandora_custom_left==-7 && dpadLeft) || (currprefs.pandora_custom_right==-7 && dpadRight))
				left = 1;
		}

		if((currprefs.pandora_custom_A==-8 && buttonA) || (currprefs.pandora_custom_B==-8 && buttonB) || (currprefs.pandora_custom_X==-8 && buttonX) || (currprefs.pandora_custom_Y==-8 && buttonY) || (currprefs.pandora_custom_L==-8 && triggerL) || (currprefs.pandora_custom_R==-8 && triggerR))
			right = 1;
		else if(currprefs.pandora_custom_dpad == 2)
		{
			if((currprefs.pandora_custom_up==-8 && dpadUp) || (currprefs.pandora_custom_down==-8 && dpadDown) || (currprefs.pandora_custom_left==-8 && dpadLeft) || (currprefs.pandora_custom_right==-8 && dpadRight))
				right = 1;
		}
	}

  if(currprefs.pandora_custom_dpad == 0) // dPad as joystick
  {
    // Handle autofire (only available if no custom controls active)
  	if(!currprefs.pandora_customControls && 
  	  (((currprefs.pandora_autofireButton1==GP2X_BUTTON_B && buttonA) 
     || (currprefs.pandora_autofireButton1==GP2X_BUTTON_X && buttonX) 
     || (currprefs.pandora_autofireButton1==GP2X_BUTTON_Y && buttonY)) 
  	     && delay > currprefs.input_autofire_framecnt))
  	{
  		if(!buttonB)
  			*button=1;
  		delay=0;
  		*button |= (buttonB & 1) << 1;
  	}
  }

	if(currprefs.pandora_customControls)
	{
	  // get joystick button via custom controls
		if((currprefs.pandora_custom_A==-3 && buttonA) || (currprefs.pandora_custom_B==-3 && buttonB) || (currprefs.pandora_custom_X==-3 && buttonX) || (currprefs.pandora_custom_Y==-3 && buttonY) || (currprefs.pandora_custom_L==-3 && triggerL) || (currprefs.pandora_custom_R==-3 && triggerR))
			*button = 1;
		else if(currprefs.pandora_custom_dpad == 2)
		{
			if((currprefs.pandora_custom_up==-3 && dpadUp) || (currprefs.pandora_custom_down==-3 && dpadDown) || (currprefs.pandora_custom_left==-3 && dpadLeft) || (currprefs.pandora_custom_right==-3 && dpadRight))
				*button = 1;
		}

		if((currprefs.pandora_custom_A==-4 && buttonA) || (currprefs.pandora_custom_B==-4 && buttonB) || (currprefs.pandora_custom_X==-4 && buttonX) || (currprefs.pandora_custom_Y==-4 && buttonY) || (currprefs.pandora_custom_L==-4 && triggerL) || (currprefs.pandora_custom_R==-4 && triggerR))
			*button |= 1 << 1;
		else if(currprefs.pandora_custom_dpad == 2)
		{
			if((currprefs.pandora_custom_up==-4 && dpadUp) || (currprefs.pandora_custom_down==-4 && dpadDown) || (currprefs.pandora_custom_left==-4 && dpadLeft) || (currprefs.pandora_custom_right==-4 && dpadRight))
				*button |= 1 << 1;
		}
		delay++;
	}
	else
	{
	  // get joystick button via ABXY or joystick
 		*button = ((currprefs.pandora_button1==GP2X_BUTTON_B && buttonA) || (currprefs.pandora_button1==GP2X_BUTTON_X && buttonX) || (currprefs.pandora_button1==GP2X_BUTTON_Y && buttonY) || SDL_JoystickGetButton(joy, currprefs.pandora_button1)) & 1;
		delay++;

		*button |= ((buttonB || SDL_JoystickGetButton(joy, currprefs.pandora_button2)) & 1) << 1;
	}

  #ifdef RASPBERRY
  *button |= (SDL_JoystickGetButton(joy, 0)) & 1;
  *button |= ((SDL_JoystickGetButton(joy, 1)) & 1) << 1;
  #endif

  #ifdef SIX_AXIS_WORKAROUND
  *button |= (SDL_JoystickGetButton(joy, 13)) & 1;
  *button |= ((SDL_JoystickGetButton(joy, 14)) & 1) << 1;
  if (SDL_JoystickGetButton(joy, 4))   top=1;
  if (SDL_JoystickGetButton(joy, 5))   right=1;
  if (SDL_JoystickGetButton(joy, 6))   bot=1;
  if (SDL_JoystickGetButton(joy, 7))   left=1;
  #endif

  // normal joystick movement
  if (left) 
    top = !top;
	if (right) 
	  bot = !bot;
  *dir = bot | (right << 1) | (top << 8) | (left << 9);

  if(currprefs.pandora_joyPort != 0)
  {
    // Only one joystick active    
    if((nr == 0 && currprefs.pandora_joyPort == 2) || (nr == 1 && currprefs.pandora_joyPort == 1))
    {
      *dir = 0;
      *button = 0;
    }
  }
}

void handle_joymouse(void)
{
	if (currprefs.pandora_custom_dpad == 1)
	{
    int mouseScale = currprefs.input_joymouse_multiplier / 2;

		if (buttonY) // slow mouse active
			mouseScale = currprefs.input_joymouse_multiplier / 10;

		if (dpadLeft)
		{
			lastmx -= mouseScale;
			newmousecounters=1;
		}
		if (dpadRight)
		{
			lastmx += mouseScale;
			newmousecounters=1;
		}
		if (dpadUp)
		{    
			lastmy -= mouseScale;
			newmousecounters=1;
		}
		if (dpadDown)
		{
			lastmy += mouseScale;
			newmousecounters=1;
		}
	}
}

void init_joystick(void)
{
    nr_joysticks = SDL_NumJoysticks ();
    if (nr_joysticks > 0)
    {
		uae4all_joy0 = SDL_JoystickOpen (0);
                printf("Joystick0 : %s\n",SDL_JoystickName(0));
		printf("    Buttons: %i Axis: %i Hats: %i\n",SDL_JoystickNumButtons(uae4all_joy0),SDL_JoystickNumAxes(uae4all_joy0),SDL_JoystickNumHats(uae4all_joy0));
    }
    if (nr_joysticks > 1)
    {
		uae4all_joy1 = SDL_JoystickOpen (1);
                printf("Joystick1 : %s\n",SDL_JoystickName(1));
		printf("    Buttons: %i Axis: %i Hats: %i\n",SDL_JoystickNumButtons(uae4all_joy1),SDL_JoystickNumAxes(uae4all_joy1),SDL_JoystickNumHats(uae4all_joy1));
    }
    else
		uae4all_joy1 = NULL; 
}

void close_joystick(void)
{
    if (nr_joysticks > 0)
	SDL_JoystickClose (uae4all_joy0);
    if (nr_joysticks > 1)
	SDL_JoystickClose (uae4all_joy1);
}
