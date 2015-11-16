#include <guichan.hpp>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_image.h>
#include <guichan/sdl.hpp>
#include "sdltruetypefont.hpp"
#include "SelectorEntry.hpp"
#include "UaeRadioButton.hpp"
#include "UaeCheckBox.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "autoconf.h"
#include "xwin.h"
#include "custom.h"
#include "drawing.h"
#include "uae.h"
#include "gui.h"
#include "savestate.h"
#include "target.h"
#include "gui_handling.h"


int currentStateNum = 0;

static gcn::Window *grpNumber;
static gcn::UaeRadioButton* optState0;
static gcn::UaeRadioButton* optState1;
static gcn::UaeRadioButton* optState2;
static gcn::UaeRadioButton* optState3;
static gcn::Window *wndScreenshot;
static gcn::Icon* icoSavestate = 0;
static gcn::Image *imgSavestate = 0;
static gcn::Button* cmdLoadState;
static gcn::Button* cmdSaveState;
static gcn::Label *lblWarningHDDon;
  

class SavestateActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
	    if (actionEvent.getSource() == optState0)
	      currentStateNum = 0;
	    else if (actionEvent.getSource() == optState1)
	      currentStateNum = 1;
	    else if (actionEvent.getSource() == optState2)
	      currentStateNum = 2;
	    else if (actionEvent.getSource() == optState3)
	      currentStateNum = 3;
	    else if (actionEvent.getSource() == cmdLoadState)
	    {
       	//------------------------------------------
       	// Load state
       	//------------------------------------------
       	if(emulating)
    	  {
    			if(strlen(savestate_fname) > 0)
  			  {
      			FILE *f = fopen(savestate_fname,"rb");
      			if (f)
      			{
      				fclose(f);
              savestate_initsave(savestate_fname, 2, 0);
      				savestate_state = STATE_DORESTORE;
      				gui_running = false;
      			}
      		}
    			if(savestate_state != STATE_DORESTORE)
    				ShowMessage("Loading savestate", "Statefile doesn't exist.", "", "Ok", "");
    		}
    		else
  	      ShowMessage("Loading savestate", "Emulation hasn't started yet.", "", "Ok", "");
      }
	    else if (actionEvent.getSource() == cmdSaveState)
	    {
        //------------------------------------------
        // Save current state
        //------------------------------------------
      	if(emulating)
    	  {
          savestate_initsave(savestate_fname, 2, 0);
    			save_state (savestate_fname, "...");
          savestate_state = STATE_DOSAVE; // Just to create the screenshot
          delay_savestate_frame = 1;          
      		gui_running = false;
        }	      
    		else
  	      ShowMessage("Saving state", "Emulation hasn't started yet.", "", "Ok", "");
      }

	    RefreshPanelSavestate();
    }
};
static SavestateActionListener* savestateActionListener;


void InitPanelSavestate(const struct _ConfigCategory& category)
{
  savestateActionListener = new SavestateActionListener();

	optState0 = new gcn::UaeRadioButton("0", "radiostategroup");
	optState0->setId("State0");
	optState0->addActionListener(savestateActionListener);

	optState1 = new gcn::UaeRadioButton("1", "radiostategroup");
	optState1->setId("State1");
	optState1->addActionListener(savestateActionListener);

	optState2 = new gcn::UaeRadioButton("2", "radiostategroup");
	optState2->setId("State2");
	optState2->addActionListener(savestateActionListener);

	optState3 = new gcn::UaeRadioButton("3", "radiostategroup");
	optState3->setId("State3");
	optState3->addActionListener(savestateActionListener);

	grpNumber = new gcn::Window("Number");
	grpNumber->add(optState0, 5, 10);
	grpNumber->add(optState1, 5, 40);
	grpNumber->add(optState2, 5, 70);
	grpNumber->add(optState3, 5, 100);
	grpNumber->setMovable(false);
	grpNumber->setSize(60, 145);
  grpNumber->setBaseColor(gui_baseCol);
  
	wndScreenshot = new gcn::Window("State screen");
	wndScreenshot->setMovable(false);
	wndScreenshot->setSize(400, 300);
  wndScreenshot->setBaseColor(gui_baseCol);

  cmdLoadState = new gcn::Button("Load State");
  cmdLoadState->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
  cmdLoadState->setBaseColor(gui_baseCol);
  cmdLoadState->setId("LoadState");
  cmdLoadState->addActionListener(savestateActionListener);

  cmdSaveState = new gcn::Button("Save State");
  cmdSaveState->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
  cmdSaveState->setBaseColor(gui_baseCol);
  cmdSaveState->setId("SaveState");
  cmdSaveState->addActionListener(savestateActionListener);

  lblWarningHDDon = new gcn::Label("State saves do not support harddrive emulation.");
  lblWarningHDDon->setSize(360, LABEL_HEIGHT);
  
  category.panel->add(grpNumber, DISTANCE_BORDER, DISTANCE_BORDER);
  category.panel->add(wndScreenshot, DISTANCE_BORDER + 100, DISTANCE_BORDER);
  int buttonY = category.panel->getHeight() - DISTANCE_BORDER - BUTTON_HEIGHT;
  category.panel->add(cmdLoadState, DISTANCE_BORDER, buttonY);
  category.panel->add(cmdSaveState, DISTANCE_BORDER + BUTTON_WIDTH + DISTANCE_NEXT_X, buttonY);
  category.panel->add(lblWarningHDDon, DISTANCE_BORDER + 100, DISTANCE_BORDER + 50);
  
  RefreshPanelSavestate();
}


void ExitPanelSavestate(void)
{
  delete optState0;
  delete optState1;
  delete optState2;
  delete optState3;
  delete grpNumber;

	if(imgSavestate != 0)
	  delete imgSavestate;
  imgSavestate = 0;
	if(icoSavestate != 0)
	  delete icoSavestate;
	icoSavestate = 0;
  delete wndScreenshot;

  delete cmdLoadState;
  delete cmdSaveState;
  delete lblWarningHDDon;
  
  delete savestateActionListener;
}


void RefreshPanelSavestate(void)
{
	if(icoSavestate != 0)
  {
		wndScreenshot->remove(icoSavestate);
	  delete icoSavestate;
	  icoSavestate = 0;
	}
	if(imgSavestate != 0)
	{
	  delete imgSavestate;
	  imgSavestate = 0;
	}
	
	switch(currentStateNum)
	{
	  case 0:
	    optState0->setSelected(true);
	    break;
	  case 1:
	    optState1->setSelected(true);
	    break;
	  case 2:
	    optState2->setSelected(true);
	    break;
	  case 3:
	    optState3->setSelected(true);
	    break;
	}
	
  gui_update();
	if(strlen(screenshot_filename) > 0)
  {
		FILE *f=fopen(screenshot_filename,"rb");
		if (f)
		{
			fclose(f);
			gcn::Rectangle rect = wndScreenshot->getChildrenArea();
			SDL_Surface *loadedImage = IMG_Load(screenshot_filename);
			if(loadedImage != NULL)
		  {
		    SDL_Rect source = {0, 0, 0, 0 };
		    SDL_Rect target = {0, 0, 0, 0 };
        SDL_Surface *scaled = SDL_CreateRGBSurface(loadedImage->flags, rect.width, rect.height, loadedImage->format->BitsPerPixel, loadedImage->format->Rmask, loadedImage->format->Gmask, loadedImage->format->Bmask, loadedImage->format->Amask);
        source.w = loadedImage->w;
        source.h = loadedImage->h;
        target.w = rect.width;
        target.h = rect.height;
  			SDL_SoftStretch(loadedImage, &source, scaled, &target);
  			SDL_FreeSurface(loadedImage);
  			imgSavestate = new gcn::SDLImage(scaled, true);
  			icoSavestate = new gcn::Icon(imgSavestate);
  			wndScreenshot->add(icoSavestate);
  		}
		}
  }
  
  bool enabled = nr_units () == 0;
  optState0->setEnabled(enabled);
  optState1->setEnabled(enabled);
  optState2->setEnabled(enabled);
  optState3->setEnabled(enabled);
  wndScreenshot->setVisible(enabled);
  cmdLoadState->setEnabled(enabled);
  cmdSaveState->setEnabled(enabled);
  lblWarningHDDon->setVisible(!enabled);
}
