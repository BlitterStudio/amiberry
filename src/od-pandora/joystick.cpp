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
#include "newcpu.h"
#include "custom.h"
#include "joystick.h"
#include "inputdevice.h"
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
  int left = 0, right = 0, top = 0, bot = 0;
  SDL_Joystick *joy = nr == 1 ? uae4all_joy0 : uae4all_joy1;

  if(nr == 0)
  {
    static int last_buttonstate0 = 0;
    static int delay_leftmouse = 0;

    if(last_buttonstate0 != buttonstate[0])
    {
      last_buttonstate0 = buttonstate[0];
      if(buttonstate[0])
      {
        // Left mousebutton click -> delay click
        delay_leftmouse = 2;
      }
    }
    if(delay_leftmouse)
    {
      --delay_leftmouse;  
      *button = (buttonstate[2] ? 2 : 0) | (buttonstate[1] ? 4 : 0);
    }
    else
      *button = (buttonstate[0] ? 1 : 0) | (buttonstate[2] ? 2 : 0) | (buttonstate[1] ? 4 : 0);
  }
  else
    *button = 0;
        
  if(currprefs.pandora_joyPort != 0)
  {
    // Only one joystick active    
    if((nr == 0 && currprefs.pandora_joyPort == 2) || (nr == 1 && currprefs.pandora_joyPort == 1))
      return;
  }

  *dir = 0;

  SDL_JoystickUpdate ();

  #ifdef RASPBERRY
	// Always check joystick state on Raspberry pi.
  #else
	if (!triggerR /*R+dpad = arrow keys*/ && currprefs.pandora_custom_dpad==0)
  #endif
	{
		// get joystick direction via dPad or joystick
		int hat=SDL_JoystickGetHat(joy,0);
		int val = SDL_JoystickGetAxis(joy, 0);
		if (((hat & SDL_HAT_RIGHT) && currprefs.pandora_custom_dpad < 2) || (dpadRight && currprefs.pandora_custom_dpad < 2) || val > 6000) right=1;
		if (((hat & SDL_HAT_LEFT) && currprefs.pandora_custom_dpad < 2)  || (dpadLeft && currprefs.pandora_custom_dpad < 2)  || val < -6000) left=1;
		val = SDL_JoystickGetAxis(joy, 1);
		if (((hat & SDL_HAT_UP) && currprefs.pandora_custom_dpad < 2)  || (dpadUp && currprefs.pandora_custom_dpad < 2)   || val < -6000) top=1;
		if (((hat & SDL_HAT_DOWN) && currprefs.pandora_custom_dpad < 2) || (dpadDown && currprefs.pandora_custom_dpad < 2) || val > 6000) bot=1;
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
  			*button |= 1;
  		delay=0;
  		*button |= (buttonB & 1) << 1;
  	}
  }

	if(currprefs.pandora_customControls)
	{
	  // get joystick button via custom controls
		if((currprefs.pandora_custom_A==-3 && buttonA) || (currprefs.pandora_custom_B==-3 && buttonB) || (currprefs.pandora_custom_X==-3 && buttonX) || (currprefs.pandora_custom_Y==-3 && buttonY) || (currprefs.pandora_custom_L==-3 && triggerL) || (currprefs.pandora_custom_R==-3 && triggerR))
			*button |= 1;
		else if(currprefs.pandora_custom_dpad == 2)
		{
			if((currprefs.pandora_custom_up==-3 && dpadUp) || (currprefs.pandora_custom_down==-3 && dpadDown) || (currprefs.pandora_custom_left==-3 && dpadLeft) || (currprefs.pandora_custom_right==-3 && dpadRight))
				*button |= 1;
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
		if (!currprefs.pandora_customControls)
		{
			if (!currprefs.pandora_button1 == 3 && currprefs.pandora_joyConf < 2)
			{
				*button |= ((currprefs.pandora_button1==GP2X_BUTTON_B && buttonA) || (currprefs.pandora_button1==GP2X_BUTTON_X && buttonX) || (currprefs.pandora_button1==GP2X_BUTTON_Y && buttonY) || SDL_JoystickGetButton(joy, currprefs.pandora_button1)) & 1;
			}
			else
			{
				*button |= ((currprefs.pandora_button1==GP2X_BUTTON_B && buttonA) || (currprefs.pandora_button1==GP2X_BUTTON_X && buttonX)) & 1;
			}
			delay++;
		}

		if (!SDL_JoystickGetButton(joy, 1))
			*button |= ((buttonB || SDL_JoystickGetButton(joy, currprefs.pandora_button2)) & 1) << 1; 
	
	}

  #ifdef RASPBERRY
  if(!currprefs.pandora_customControls || currprefs.pandora_custom_A == 0)
	  *button |= (SDL_JoystickGetButton(joy, 0)) & 1;
  if(!currprefs.pandora_customControls || currprefs.pandora_custom_B == 0)
	  *button |= ((SDL_JoystickGetButton(joy, 1)) & 1) << 1;
  #endif

  #ifdef SIX_AXIS_WORKAROUND
  *button |= (SDL_JoystickGetButton(joy, 13)) & 1;
  *button |= ((SDL_JoystickGetButton(joy, 14)) & 1) << 1;
  if (!currprefs.pandora_customControls && SDL_JoystickGetButton(joy, 4))   top=1;
  if (!currprefs.pandora_customControls && SDL_JoystickGetButton(joy, 5))   right=1;
  if (!currprefs.pandora_customControls && SDL_JoystickGetButton(joy, 6))   bot=1;
  if (!currprefs.pandora_customControls && SDL_JoystickGetButton(joy, 7))   left=1;
  #endif

  // normal joystick movement
  if (left) 
    top = !top;
	if (right) 
	  bot = !bot;
  *dir = bot | (right << 1) | (top << 8) | (left << 9);
}

void handle_joymouse(void)
{
  int y = 1024;
  int mouseScale = currprefs.input_joymouse_multiplier / 2;

	if (buttonY) // slow mouse active
		mouseScale = currprefs.input_joymouse_multiplier / 10;

	if (dpadLeft)	{
	  setmousestate(0, 0, -mouseScale, 0);
	  y = 0;
	}
	if (dpadRight) {
	  setmousestate(0, 0, mouseScale, 0);
	  y = 0;
	}
	if (dpadUp) {    
	  y = -mouseScale;
	}
	if (dpadDown) {
	  y = mouseScale;
	}
	if(y < 1024)
	  setmousestate(0, 1, y, 0);
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
