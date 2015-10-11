#include <guichan.hpp>
#include <SDL/SDL_ttf.h>
#include <guichan/sdl.hpp>
#include "sdltruetypefont.hpp"
#include "SelectorEntry.hpp"
#include "UaeRadioButton.hpp"
#include "UaeDropDown.hpp"
#include "UaeCheckBox.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "memory.h"
#include "uae.h"
#include "autoconf.h"
#include "filesys.h"
#include "gui.h"
#include "target.h"
#include "gui_handling.h"


#define DIALOG_WIDTH 520
#define DIALOG_HEIGHT 202

static bool dialogResult = false;
static bool dialogFinished = false;

static gcn::Window *wndEditFilesysVirtual;
static gcn::Button* cmdOK;
static gcn::Button* cmdCancel;
static gcn::Label *lblDevice;
static gcn::TextField *txtDevice;
static gcn::Label *lblVolume;
static gcn::TextField *txtVolume;
static gcn::Label *lblPath;
static gcn::TextField *txtPath;
static gcn::Button* cmdPath;
static gcn::UaeCheckBox* chkReadWrite;
static gcn::UaeCheckBox* chkAutoboot;
static gcn::Label *lblBootPri;
static gcn::TextField *txtBootPri;


class FilesysVirtualActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      if(actionEvent.getSource() == cmdPath)
      {
        char tmp[MAX_PATH];
        strncpy(tmp, txtPath->getText().c_str(), MAX_PATH);
        wndEditFilesysVirtual->releaseModalFocus();
        if(SelectFolder("Select folder", tmp))
          txtPath->setText(tmp);
        wndEditFilesysVirtual->requestModalFocus();
        cmdPath->requestFocus();
      }
      else
      {
        if (actionEvent.getSource() == cmdOK)
        {
          if(txtDevice->getText().length() <= 0)
          {
            wndEditFilesysVirtual->setCaption("Please enter a device name.");
            return;
          }
          if(txtVolume->getText().length() <= 0)
          {
            wndEditFilesysVirtual->setCaption("Please enter a volume name.");
            return;
          }
          dialogResult = true;
        }
        dialogFinished = true;
      }
    }
};
static FilesysVirtualActionListener* filesysVirtualActionListener;


static void InitEditFilesysVirtual(void)
{
	wndEditFilesysVirtual = new gcn::Window("Edit");
	wndEditFilesysVirtual->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
  wndEditFilesysVirtual->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
  wndEditFilesysVirtual->setBaseColor(gui_baseCol + 0x202020);
  wndEditFilesysVirtual->setCaption("Volume settings");
  wndEditFilesysVirtual->setTitleBarHeight(TITLEBAR_HEIGHT);
  
  filesysVirtualActionListener = new FilesysVirtualActionListener();
  
	cmdOK = new gcn::Button("Ok");
	cmdOK->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH - DISTANCE_NEXT_X, DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
  cmdOK->setBaseColor(gui_baseCol + 0x202020);
  cmdOK->setId("virtOK");
  cmdOK->addActionListener(filesysVirtualActionListener);
  
	cmdCancel = new gcn::Button("Cancel");
	cmdCancel->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdCancel->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH, DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
  cmdCancel->setBaseColor(gui_baseCol + 0x202020);
  cmdCancel->setId("virtCancel");
  cmdCancel->addActionListener(filesysVirtualActionListener);

  lblDevice = new gcn::Label("Device Name:");
  lblDevice->setSize(100, LABEL_HEIGHT);
  lblDevice->setAlignment(gcn::Graphics::RIGHT);
  txtDevice = new gcn::TextField();
  txtDevice->setSize(80, TEXTFIELD_HEIGHT);
  txtDevice->setId("virtDev");

  lblVolume = new gcn::Label("Volume Label:");
  lblVolume->setSize(100, LABEL_HEIGHT);
  lblVolume->setAlignment(gcn::Graphics::RIGHT);
  txtVolume = new gcn::TextField();
  txtVolume->setSize(80, TEXTFIELD_HEIGHT);
  txtVolume->setId("virtVol");

  lblPath = new gcn::Label("Path:");
  lblPath->setSize(100, LABEL_HEIGHT);
  lblPath->setAlignment(gcn::Graphics::RIGHT);
  txtPath = new gcn::TextField();
  txtPath->setSize(338, TEXTFIELD_HEIGHT);
  txtPath->setEnabled(false);
  cmdPath = new gcn::Button("...");
  cmdPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
  cmdPath->setBaseColor(gui_baseCol + 0x202020);
  cmdPath->setId("virtPath");
  cmdPath->addActionListener(filesysVirtualActionListener);

	chkReadWrite = new gcn::UaeCheckBox("Read/Write", true);
  chkReadWrite->setId("virtRW");

	chkAutoboot = new gcn::UaeCheckBox("Bootable", true);
  chkAutoboot->setId("virtAutoboot");

  lblBootPri = new gcn::Label("Boot priority:");
  lblBootPri->setSize(84, LABEL_HEIGHT);
  lblBootPri->setAlignment(gcn::Graphics::RIGHT);
  txtBootPri = new gcn::TextField();
  txtBootPri->setSize(40, TEXTFIELD_HEIGHT);
  txtBootPri->setId("virtBootpri");
  
  int posY = DISTANCE_BORDER;
  wndEditFilesysVirtual->add(lblDevice, DISTANCE_BORDER, posY);
  wndEditFilesysVirtual->add(txtDevice, DISTANCE_BORDER + lblDevice->getWidth() + 8, posY);
  wndEditFilesysVirtual->add(chkReadWrite, 250, posY + 1);
  posY += txtDevice->getHeight() + DISTANCE_NEXT_Y;
  wndEditFilesysVirtual->add(lblVolume, DISTANCE_BORDER, posY);
  wndEditFilesysVirtual->add(txtVolume, DISTANCE_BORDER + lblDevice->getWidth() + 8, posY);
  wndEditFilesysVirtual->add(chkAutoboot, 250, posY + 1);
  wndEditFilesysVirtual->add(lblBootPri, 374, posY);
  wndEditFilesysVirtual->add(txtBootPri, 374 + lblBootPri->getWidth() + 8, posY);
  posY += txtDevice->getHeight() + DISTANCE_NEXT_Y;
  wndEditFilesysVirtual->add(lblPath, DISTANCE_BORDER, posY);
  wndEditFilesysVirtual->add(txtPath, DISTANCE_BORDER + lblDevice->getWidth() + 8, posY);
  wndEditFilesysVirtual->add(cmdPath, wndEditFilesysVirtual->getWidth() - DISTANCE_BORDER - SMALL_BUTTON_WIDTH, posY);
  posY += txtDevice->getHeight() + DISTANCE_NEXT_Y;

  wndEditFilesysVirtual->add(cmdOK);
  wndEditFilesysVirtual->add(cmdCancel);

  gui_top->add(wndEditFilesysVirtual);
  
  txtDevice->requestFocus();
  wndEditFilesysVirtual->requestModalFocus();
}


static void ExitEditFilesysVirtual(void)
{
  wndEditFilesysVirtual->releaseModalFocus();
  gui_top->remove(wndEditFilesysVirtual);

  delete lblDevice;
  delete txtDevice;
  delete lblVolume;
  delete txtVolume;
  delete lblPath;
  delete txtPath;
  delete cmdPath;
  delete chkReadWrite;
  delete chkAutoboot;
  delete lblBootPri;
  delete txtBootPri;
  
  delete cmdOK;
  delete cmdCancel;
  delete filesysVirtualActionListener;
  
  delete wndEditFilesysVirtual;
}


static void EditFilesysVirtualLoop(void)
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
            
          case SDLK_UP:
            if(HandleNavigation(DIRECTION_UP))
              continue; // Don't change value when enter ComboBox -> don't send event to control
            break;
            
          case SDLK_DOWN:
            if(HandleNavigation(DIRECTION_DOWN))
              continue; // Don't change value when enter ComboBox -> don't send event to control
            break;

          case SDLK_LEFT:
            if(HandleNavigation(DIRECTION_LEFT))
              continue; // Don't change value when enter Slider -> don't send event to control
            break;
            
          case SDLK_RIGHT:
            if(HandleNavigation(DIRECTION_RIGHT))
              continue; // Don't change value when enter Slider -> don't send event to control
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


bool EditFilesysVirtual(int unit_no)
{
  struct mountedinfo mi;
  struct uaedev_config_info *uci;
  std::string strdevname, strvolname, strroot;
  char tmp[32];
  
  dialogResult = false;
  dialogFinished = false;

  InitEditFilesysVirtual();

  if(unit_no >= 0)
  {
    uci = &changed_prefs.mountconfig[unit_no];
    get_filesys_unitconfig(&changed_prefs, unit_no, &mi);

    strdevname.assign(uci->devname);
    txtDevice->setText(strdevname);
    strvolname.assign(uci->volname);
    txtVolume->setText(strvolname);
    strroot.assign(uci->rootdir);
    txtPath->setText(strroot);
    chkReadWrite->setSelected(!uci->readonly);
    chkAutoboot->setSelected(uci->bootpri != -128);
    snprintf(tmp, 32, "%d", uci->bootpri >= -127 ? uci->bootpri : -127);
    txtBootPri->setText(tmp);
  }
  else
  {
    CreateDefaultDevicename(tmp);
    txtDevice->setText(tmp);
    txtVolume->setText(tmp);
    strroot.assign(currentDir);
    txtPath->setText(strroot);
    chkReadWrite->setSelected(true);
    txtBootPri->setText("0");
  }

  EditFilesysVirtualLoop();
  ExitEditFilesysVirtual();
  
  if(dialogResult)
  {
    int bp = tweakbootpri(atoi(txtBootPri->getText().c_str()), chkAutoboot->isSelected() ? 1 : 0, 0);
    extractPath((char *) txtPath->getText().c_str(), currentDir);
    
    uci = add_filesys_config(&changed_prefs, unit_no, (char *) txtDevice->getText().c_str(), 
      (char *) txtVolume->getText().c_str(), (char *) txtPath->getText().c_str(), 
      !chkReadWrite->isSelected(), 0, 0, 0, 0, bp, 0, 0, 0);
    if (uci)
    	filesys_media_change (uci->rootdir, 1, uci);
  }

  return dialogResult;
}
