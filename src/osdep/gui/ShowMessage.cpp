#include <algorithm>
#include <guichan.hpp>
#include <iostream>
#include <sstream>
#include <SDL/SDL_ttf.h>
#include <guichan/sdl.hpp>
#include "sdltruetypefont.hpp"
#include "SelectorEntry.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "gui.h"
#include "gui_handling.h"


#define DIALOG_WIDTH 340
#define DIALOG_HEIGHT 140

static bool dialogResult = false;
static bool dialogFinished = false;

static gcn::Window *wndShowMessage;
static gcn::Button* cmdOK;
static gcn::Button* cmdCancel;
static gcn::Label* lblText1;
static gcn::Label* lblText2;


class ShowMessageActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      if (actionEvent.getSource() == cmdOK)
        dialogResult = true;
      dialogFinished = true;
    }
};
static ShowMessageActionListener* showMessageActionListener;


static void InitShowMessage(void)
{
	wndShowMessage = new gcn::Window("Message");
	wndShowMessage->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
  wndShowMessage->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
  wndShowMessage->setBaseColor(gui_baseCol + 0x202020);
  wndShowMessage->setTitleBarHeight(TITLEBAR_HEIGHT);

  showMessageActionListener = new ShowMessageActionListener();

	lblText1 = new gcn::Label("");
	lblText1->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER, LABEL_HEIGHT);
	lblText2 = new gcn::Label("");
	lblText2->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER, LABEL_HEIGHT);

	cmdOK = new gcn::Button("Ok");
	cmdOK->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH - DISTANCE_NEXT_X, DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
  cmdOK->setBaseColor(gui_baseCol + 0x202020);
  cmdOK->addActionListener(showMessageActionListener);
  
	cmdCancel = new gcn::Button("Cancel");
	cmdCancel->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdCancel->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH, DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
  cmdCancel->setBaseColor(gui_baseCol + 0x202020);
  cmdCancel->addActionListener(showMessageActionListener);

  wndShowMessage->add(lblText1, DISTANCE_BORDER, DISTANCE_BORDER);
  wndShowMessage->add(lblText2, DISTANCE_BORDER, DISTANCE_BORDER + lblText1->getHeight() + 4);
  wndShowMessage->add(cmdOK);
  wndShowMessage->add(cmdCancel);

  gui_top->add(wndShowMessage);
  
  cmdOK->requestFocus();
  wndShowMessage->requestModalFocus();
}


static void ExitShowMessage(void)
{
  wndShowMessage->releaseModalFocus();
  gui_top->remove(wndShowMessage);

  delete lblText1;
  delete lblText2;
  delete cmdOK;
  delete cmdCancel;
  
  delete showMessageActionListener;

  delete wndShowMessage;
}


static void ShowMessageLoop(void)
{
  while(!dialogFinished)
  {
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
      if (event.type == SDL_KEYDOWN)
      {
        switch(event.key.keysym.sym)
        {
          case SDLK_ESCAPE:
            dialogFinished = true;
            break;
            
          case SDLK_LEFT:
          case SDLK_RIGHT:
            {
              gcn::FocusHandler* focusHdl = gui_top->_getFocusHandler();
              gcn::Widget* activeWidget = focusHdl->getFocused();
              if(activeWidget == cmdCancel)
                cmdOK->requestFocus();
              else if(activeWidget == cmdOK)
                cmdCancel->requestFocus();
              continue;
            }
            break;

          case SDLK_PAGEDOWN:
          case SDLK_HOME:
            event.key.keysym.sym = SDLK_RETURN;
            gui_input->pushInput(event); // Fire key down
            event.type = SDL_KEYUP;  // and the key up
            break;
        }
      }

      //-------------------------------------------------
      // Send event to guichan-controls
      //-------------------------------------------------
      gui_input->pushInput(event);
    }

    // Now we let the Gui object perform its logic.
    uae_gui->logic();
    // Now we let the Gui object draw itself.
    uae_gui->draw();
    // Finally we update the screen.
    SDL_Flip(gui_screen);
  }  
}


bool ShowMessage(const char *title, const char *line1, const char *line2, const char *button1, const char *button2)
{
  dialogResult = false;
  dialogFinished = false;
  
  InitShowMessage();
  
  wndShowMessage->setCaption(title);
  lblText1->setCaption(line1);
  lblText2->setCaption(line2);
  cmdOK->setCaption(button1);
  cmdCancel->setCaption(button2);
  if(strlen(button2) == 0)
  {
    cmdCancel->setVisible(false);
    cmdOK->setPosition(cmdCancel->getX(), cmdCancel->getY());
  }
  ShowMessageLoop();
  ExitShowMessage();
  
  return dialogResult;
}
