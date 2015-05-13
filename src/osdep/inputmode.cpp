#include<stdio.h>
#include<stdlib.h>
#include<SDL.h>
#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gp2x.h"
#include "inputmode.h"
#include "uae.h"
#include "target.h"

extern SDL_Surface *prSDLScreen;

static SDL_Surface *inputMode[3];


void inputmode_init(void)
{
	int i;
	char tmpchar[256];
	SDL_Surface* tmp;

  if(inputMode[0] == NULL)
  {
	  snprintf(tmpchar, 256, "%s/data/joystick.bmp", start_path_data);
	  tmp = SDL_LoadBMP(tmpchar);

	  if (tmp)
	  {
		  inputMode[0] = SDL_DisplayFormat(tmp);
		  SDL_FreeSurface(tmp);
	  }
  }
	
  if(inputMode[1] == NULL)
  {
	  snprintf(tmpchar, 256, "%s/data/mouse.bmp", start_path_data);
	  tmp = SDL_LoadBMP(tmpchar);

	  if (tmp)
	  {
		  inputMode[1] = SDL_DisplayFormat(tmp);
		  SDL_FreeSurface(tmp);
	  }
  }

  if(inputMode[2] == NULL)
  {
	  snprintf(tmpchar, 256, "%s/data/remapping.bmp", start_path_data);
	  tmp = SDL_LoadBMP(tmpchar);

	  if (tmp)
	  {
		  inputMode[2] = SDL_DisplayFormat(tmp);
		  SDL_FreeSurface(tmp);
	  }
  }

	show_inputmode = 0;
}


void inputmode_redraw(void)
{
	SDL_Rect r;
	SDL_Surface* surface;

	r.x=80;
	r.y=prSDLScreen->h-200;
	r.w=160;
	r.h=160;

	if (inputMode[0] && inputMode[1] && inputMode[2])
	{
		surface = inputMode[currprefs.pandora_custom_dpad];

		SDL_BlitSurface(surface,NULL,prSDLScreen,&r);
	}
}


void set_joyConf(void)
{
	if(changed_prefs.pandora_joyConf == 0)
	{
		changed_prefs.pandora_button1 = GP2X_BUTTON_X;
		changed_prefs.pandora_button2 = GP2X_BUTTON_A;
		changed_prefs.pandora_jump = -1;
		changed_prefs.pandora_autofireButton1 = GP2X_BUTTON_B;
	}
	else if(changed_prefs.pandora_joyConf == 1)
	{
		changed_prefs.pandora_button1 = GP2X_BUTTON_B;
		changed_prefs.pandora_button2 = GP2X_BUTTON_A;
		changed_prefs.pandora_jump = -1;
		changed_prefs.pandora_autofireButton1 = GP2X_BUTTON_X;
	}	
	else if(changed_prefs.pandora_joyConf == 2)
	{
		changed_prefs.pandora_button1 = GP2X_BUTTON_Y;
		changed_prefs.pandora_button2 = GP2X_BUTTON_A;
		changed_prefs.pandora_jump = GP2X_BUTTON_X;
		changed_prefs.pandora_autofireButton1 = GP2X_BUTTON_B;
	}
	else if(changed_prefs.pandora_joyConf == 3)
	{
		changed_prefs.pandora_button1 = GP2X_BUTTON_B;
		changed_prefs.pandora_button2 = GP2X_BUTTON_A;
		changed_prefs.pandora_jump = GP2X_BUTTON_X;
		changed_prefs.pandora_autofireButton1 = GP2X_BUTTON_Y;
	}
}
