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

#ifdef RASPBERRY
	r.x=(prSDLScreen->w-160)/2;
	r.y=(prSDLScreen->h-160)/2;
#else
	r.x=80;
	r.y=prSDLScreen->h-200;
	r.w=160;
	r.h=160;
#endif

	if (inputMode[0] && inputMode[1] && inputMode[2])
	{
		surface = inputMode[currprefs.pandora_custom_dpad];

		SDL_BlitSurface(surface,NULL,prSDLScreen,&r);
	}
}


void set_joyConf(struct uae_prefs *p)
{
	if(p->pandora_joyConf == 0)
	{
		p->pandora_button1 = GP2X_BUTTON_X;
		p->pandora_button2 = GP2X_BUTTON_A;
		p->pandora_jump = -1;
		p->pandora_autofireButton1 = GP2X_BUTTON_B;
	}
	else if(p->pandora_joyConf == 1)
	{
		p->pandora_button1 = GP2X_BUTTON_B;
		p->pandora_button2 = GP2X_BUTTON_A;
		p->pandora_jump = -1;
		p->pandora_autofireButton1 = GP2X_BUTTON_X;
	}	
	else if(p->pandora_joyConf == 2)
	{
		p->pandora_button1 = GP2X_BUTTON_Y;
		p->pandora_button2 = GP2X_BUTTON_A;
		p->pandora_jump = GP2X_BUTTON_X;
		p->pandora_autofireButton1 = GP2X_BUTTON_B;
	}
	else if(changed_prefs.pandora_joyConf == 3)
	{
		p->pandora_button1 = GP2X_BUTTON_B;
		p->pandora_button2 = GP2X_BUTTON_A;
		p->pandora_jump = GP2X_BUTTON_X;
		p->pandora_autofireButton1 = GP2X_BUTTON_Y;
	}
}
