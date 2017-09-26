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


#define DIALOG_WIDTH 760
#define DIALOG_HEIGHT 420

static bool dialogFinished = false;

static gcn::Window *wndShowHelp;
static gcn::Button* cmdOK;
static gcn::ListBox* lstHelp;
static gcn::ScrollArea* scrAreaHelp;


class HelpListModel : public gcn::ListModel
{
  std::vector<std::string> lines;

  public:
    HelpListModel(std::vector<std::string> helptext)
    {
      lines = helptext;
    }
      
    int getNumberOfElements()
    {
      return lines.size();
    }
      
    std::string getElementAt(int i)
    {
      if(i >= 0 && i < lines.size())
        return lines[i];
      return "";
    }
};
static HelpListModel *helpList;


class ShowHelpActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      dialogFinished = true;
    }
};
static ShowHelpActionListener* showHelpActionListener;


static void InitShowHelp(const std::vector<std::string>& helptext)
{
	wndShowHelp = new gcn::Window("Help");
	wndShowHelp->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
  wndShowHelp->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
  wndShowHelp->setBaseColor(gui_baseCol + 0x202020);
  wndShowHelp->setTitleBarHeight(TITLEBAR_HEIGHT);

  showHelpActionListener = new ShowHelpActionListener();

  helpList = new HelpListModel(helptext);

  lstHelp = new gcn::ListBox(helpList);
  lstHelp->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER - 4, DIALOG_HEIGHT - 3 * DISTANCE_BORDER - BUTTON_HEIGHT - DISTANCE_NEXT_Y - 10);
  lstHelp->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
  lstHelp->setBaseColor(gui_baseCol + 0x202020);
  lstHelp->setBackgroundColor(gui_baseCol + 0x202020);
  lstHelp->setWrappingEnabled(true);

  scrAreaHelp = new gcn::ScrollArea(lstHelp);
  scrAreaHelp->setFrameSize(1);
  scrAreaHelp->setPosition(DISTANCE_BORDER, 10 + TEXTFIELD_HEIGHT + 10);
  scrAreaHelp->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER - 4, DIALOG_HEIGHT - 3 * DISTANCE_BORDER - BUTTON_HEIGHT - DISTANCE_NEXT_Y - 10);
  scrAreaHelp->setScrollbarWidth(20);
  scrAreaHelp->setBaseColor(gui_baseCol + 0x202020);
  scrAreaHelp->setBackgroundColor(gui_baseCol + 0x202020);

	cmdOK = new gcn::Button("Ok");
	cmdOK->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH, DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
  cmdOK->setBaseColor(gui_baseCol + 0x202020);
  cmdOK->addActionListener(showHelpActionListener);
  
  wndShowHelp->add(scrAreaHelp, DISTANCE_BORDER, DISTANCE_BORDER);
  wndShowHelp->add(cmdOK);

  gui_top->add(wndShowHelp);
  
  cmdOK->requestFocus();
  wndShowHelp->requestModalFocus();
}


static void ExitShowHelp(void)
{
  wndShowHelp->releaseModalFocus();
  gui_top->remove(wndShowHelp);

  delete lstHelp;
  delete scrAreaHelp;
  delete cmdOK;
  
  delete helpList;
  delete showHelpActionListener;

  delete wndShowHelp;
}


static void ShowHelpLoop(void)
{
  FocusBugWorkaround(wndShowHelp);  

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
    wait_for_vsync();
    SDL_Flip(gui_screen);
  }  
}


void ShowHelp(const char *title, const std::vector<std::string>& text)
{
  dialogFinished = false;

  InitShowHelp(text);
  
  wndShowHelp->setCaption(title);
  cmdOK->setCaption("Ok");
  ShowHelpLoop();
  ExitShowHelp();
}
