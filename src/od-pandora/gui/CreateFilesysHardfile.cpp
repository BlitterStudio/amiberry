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


#define DIALOG_WIDTH 620
#define DIALOG_HEIGHT 202

static const char *harddisk_filter[] = { ".hdf", "\0" };

static bool dialogResult = false;
static bool dialogFinished = false;
static bool fileSelected = false;

static gcn::Window *wndCreateFilesysHardfile;
static gcn::Button* cmdOK;
static gcn::Button* cmdCancel;
static gcn::Label *lblDevice;
static gcn::TextField *txtDevice;
static gcn::UaeCheckBox* chkAutoboot;
static gcn::Label *lblBootPri;
static gcn::TextField *txtBootPri;
static gcn::Label *lblPath;
static gcn::TextField *txtPath;
static gcn::Button* cmdPath;
static gcn::Label *lblSize;
static gcn::TextField *txtSize;


class CreateFilesysHardfileActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      if(actionEvent.getSource() == cmdPath)
      {
        char tmp[MAX_PATH];
        strncpy(tmp, txtPath->getText().c_str(), MAX_PATH);
        wndCreateFilesysHardfile->releaseModalFocus();
        if(SelectFile("Create harddisk file", tmp, harddisk_filter, true))
        {
          txtPath->setText(tmp);
          fileSelected = true;
        }
        wndCreateFilesysHardfile->requestModalFocus();
        cmdPath->requestFocus();
      }
      else
      {
        if (actionEvent.getSource() == cmdOK)
        {
          if(txtDevice->getText().length() <= 0)
          {
            wndCreateFilesysHardfile->setCaption("Please enter a device name.");
            return;
          }
          if(!fileSelected)
          {
            wndCreateFilesysHardfile->setCaption("Please select a new filename.");
            return;
          }
          dialogResult = true;
        }
        dialogFinished = true;
      }
    }
};
static CreateFilesysHardfileActionListener* createFilesysHardfileActionListener;


static void InitCreateFilesysHardfile(void)
{
	wndCreateFilesysHardfile = new gcn::Window("Create");
	wndCreateFilesysHardfile->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
  wndCreateFilesysHardfile->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
  wndCreateFilesysHardfile->setBaseColor(gui_baseCol + 0x202020);
  wndCreateFilesysHardfile->setCaption("Create hardfile");
  wndCreateFilesysHardfile->setTitleBarHeight(TITLEBAR_HEIGHT);
  
  createFilesysHardfileActionListener = new CreateFilesysHardfileActionListener();
  
	cmdOK = new gcn::Button("Ok");
	cmdOK->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH - DISTANCE_NEXT_X, DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
  cmdOK->setBaseColor(gui_baseCol + 0x202020);
  cmdOK->setId("createHdfOK");
  cmdOK->addActionListener(createFilesysHardfileActionListener);
  
	cmdCancel = new gcn::Button("Cancel");
	cmdCancel->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdCancel->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH, DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
  cmdCancel->setBaseColor(gui_baseCol + 0x202020);
  cmdCancel->setId("createHdfCancel");
  cmdCancel->addActionListener(createFilesysHardfileActionListener);

  lblDevice = new gcn::Label("Device Name:");
  lblDevice->setSize(100, LABEL_HEIGHT);
  lblDevice->setAlignment(gcn::Graphics::RIGHT);
  txtDevice = new gcn::TextField();
  txtDevice->setSize(80, TEXTFIELD_HEIGHT);
  txtDevice->setId("createHdfDev");

	chkAutoboot = new gcn::UaeCheckBox("Bootable", true);
  chkAutoboot->setId("createHdfAutoboot");

  lblBootPri = new gcn::Label("Boot priority:");
  lblBootPri->setSize(100, LABEL_HEIGHT);
  lblBootPri->setAlignment(gcn::Graphics::RIGHT);
  txtBootPri = new gcn::TextField();
  txtBootPri->setSize(40, TEXTFIELD_HEIGHT);
  txtBootPri->setId("createHdfBootPri");

  lblSize = new gcn::Label("Size (MB):");
  lblSize->setSize(100, LABEL_HEIGHT);
  lblSize->setAlignment(gcn::Graphics::RIGHT);
  txtSize = new gcn::TextField();
  txtSize->setSize(60, TEXTFIELD_HEIGHT);
  txtSize->setId("createHdfSize");

  lblPath = new gcn::Label("Path:");
  lblPath->setSize(100, LABEL_HEIGHT);
  lblPath->setAlignment(gcn::Graphics::RIGHT);
  txtPath = new gcn::TextField();
  txtPath->setSize(438, TEXTFIELD_HEIGHT);
  txtPath->setEnabled(false);
  cmdPath = new gcn::Button("...");
  cmdPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
  cmdPath->setBaseColor(gui_baseCol + 0x202020);
  cmdPath->setId("createHdfPath");
  cmdPath->addActionListener(createFilesysHardfileActionListener);

  int posY = DISTANCE_BORDER;
  wndCreateFilesysHardfile->add(lblDevice, DISTANCE_BORDER, posY);
  wndCreateFilesysHardfile->add(txtDevice, DISTANCE_BORDER + lblDevice->getWidth() + 8, posY);
  wndCreateFilesysHardfile->add(chkAutoboot, 235, posY + 1);
  wndCreateFilesysHardfile->add(lblBootPri, 335, posY);
  wndCreateFilesysHardfile->add(txtBootPri, 335 + lblBootPri->getWidth() + 8, posY);
  posY += txtDevice->getHeight() + DISTANCE_NEXT_Y;
  wndCreateFilesysHardfile->add(lblPath, DISTANCE_BORDER, posY);
  wndCreateFilesysHardfile->add(txtPath, DISTANCE_BORDER + lblPath->getWidth() + 8, posY);
  wndCreateFilesysHardfile->add(cmdPath, wndCreateFilesysHardfile->getWidth() - DISTANCE_BORDER - SMALL_BUTTON_WIDTH, posY);
  posY += txtPath->getHeight() + DISTANCE_NEXT_Y;
  wndCreateFilesysHardfile->add(lblSize, DISTANCE_BORDER, posY);
  wndCreateFilesysHardfile->add(txtSize, DISTANCE_BORDER + lblSize->getWidth() + 8, posY);

  wndCreateFilesysHardfile->add(cmdOK);
  wndCreateFilesysHardfile->add(cmdCancel);

  gui_top->add(wndCreateFilesysHardfile);
  
  txtDevice->requestFocus();
  wndCreateFilesysHardfile->requestModalFocus();
}


static void ExitCreateFilesysHardfile(void)
{
  wndCreateFilesysHardfile->releaseModalFocus();
  gui_top->remove(wndCreateFilesysHardfile);

  delete lblDevice;
  delete txtDevice;
  delete chkAutoboot;
  delete lblBootPri;
  delete txtBootPri;
  delete lblPath;
  delete txtPath;
  delete cmdPath;
  delete lblSize;
  delete txtSize;
    
  delete cmdOK;
  delete cmdCancel;
  delete createFilesysHardfileActionListener;
  
  delete wndCreateFilesysHardfile;
}


static void CreateFilesysHardfileLoop(void)
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


bool CreateFilesysHardfile(void)
{
  struct uaedev_config_info *uci;
  std::string strroot;
  char tmp[32];
  char zero = 0;
  
  dialogResult = false;
  dialogFinished = false;

  InitCreateFilesysHardfile();

  CreateDefaultDevicename(tmp);
  txtDevice->setText(tmp);
  strroot.assign(currentDir);
  txtPath->setText(strroot);
  fileSelected = false;
    
  txtBootPri->setText("0");
  txtSize->setText("100");

  CreateFilesysHardfileLoop();
  ExitCreateFilesysHardfile();
  
  if(dialogResult)
  {
    char buffer[512];
    int size = atoi(txtSize->getText().c_str());
    if(size < 1)
      size = 1;
    if(size > 2048)
      size = 2048;    
    int bp = tweakbootpri(atoi(txtBootPri->getText().c_str()), 1, 0);
    extractPath((char *) txtPath->getText().c_str(), currentDir);
    
    FILE *newFile = fopen(txtPath->getText().c_str(), "wb");
    if(!newFile)
    {
      ShowMessage("Create Hardfile", "Unable to create new file.", "", "Ok", "");
      return false;
    }
    fseek(newFile, size * 1024 * 1024 - 1, SEEK_SET);
    fwrite(&zero, 1, 1, newFile);
    fclose(newFile);
    
    uci = add_filesys_config(&changed_prefs, -1, (char *) txtDevice->getText().c_str(), 
      0, (char *) txtPath->getText().c_str(), 0, 
      32, (size / 1024) + 1, 2, 512, 
      bp, 0, 0, 0);
    if (uci)
    	hardfile_do_disk_change (uci, 1);
  }

  return dialogResult;
}
