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
#include "include/memory.h"
#include "uae.h"
#include "autoconf.h"
#include "filesys.h"
#include "gui.h"
#include "gui_handling.h"


#define DIALOG_WIDTH 620
#define DIALOG_HEIGHT 242

static const char *harddisk_filter[] = { ".hdf", "\0" };

static bool dialogResult = false;
static bool dialogFinished = false;
static bool fileSelected = false;


static gcn::Window *wndEditFilesysHardfile;
static gcn::Button* cmdOK;
static gcn::Button* cmdCancel;
static gcn::Label *lblDevice;
static gcn::TextField *txtDevice;
static gcn::UaeCheckBox* chkReadWrite;
static gcn::UaeCheckBox* chkAutoboot;
static gcn::Label *lblBootPri;
static gcn::TextField *txtBootPri;
static gcn::Label *lblPath;
static gcn::TextField *txtPath;
static gcn::Button* cmdPath;
static gcn::Label *lblSurfaces;
static gcn::TextField *txtSurfaces;
static gcn::Label *lblReserved;
static gcn::TextField *txtReserved;
static gcn::Label *lblSectors;
static gcn::TextField *txtSectors;
static gcn::Label *lblBlocksize;
static gcn::TextField *txtBlocksize;


static void check_rdb(const TCHAR *filename)
{
  bool isrdb = hardfile_testrdb(filename);
  if(isrdb)
  {
		txtSectors->setText("0");
		txtSurfaces->setText("0");
		txtReserved->setText("0");
		txtBootPri->setText("0");
  }
  txtSectors->setEnabled(!isrdb);
	txtSurfaces->setEnabled(!isrdb);
	txtReserved->setEnabled(!isrdb);
	txtBootPri->setEnabled(!isrdb);
}


class FilesysHardfileActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      if(actionEvent.getSource() == cmdPath)
      {
        char tmp[MAX_PATH];
        strncpy(tmp, txtPath->getText().c_str(), MAX_PATH);
        wndEditFilesysHardfile->releaseModalFocus();
        if(SelectFile("Select harddisk file", tmp, harddisk_filter))
        {
          txtPath->setText(tmp);
          fileSelected = true;
          check_rdb(tmp);
        }
        wndEditFilesysHardfile->requestModalFocus();
        cmdPath->requestFocus();
      }
      else
      {
        if (actionEvent.getSource() == cmdOK)
        {
          if(txtDevice->getText().length() <= 0)
          {
            wndEditFilesysHardfile->setCaption("Please enter a device name.");
            return;
          }
          if(!fileSelected)
          {
            wndEditFilesysHardfile->setCaption("Please select a filename.");
            return;
          }
          dialogResult = true;
        }
        dialogFinished = true;
      }
    }
};
static FilesysHardfileActionListener* filesysHardfileActionListener;


static void InitEditFilesysHardfile(void)
{
	wndEditFilesysHardfile = new gcn::Window("Edit");
	wndEditFilesysHardfile->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
  wndEditFilesysHardfile->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
  wndEditFilesysHardfile->setBaseColor(gui_baseCol + 0x202020);
  wndEditFilesysHardfile->setCaption("Volume settings");
  wndEditFilesysHardfile->setTitleBarHeight(TITLEBAR_HEIGHT);
  
  filesysHardfileActionListener = new FilesysHardfileActionListener();
  
	cmdOK = new gcn::Button("Ok");
	cmdOK->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH - DISTANCE_NEXT_X, DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
  cmdOK->setBaseColor(gui_baseCol + 0x202020);
  cmdOK->setId("hdfOK");
  cmdOK->addActionListener(filesysHardfileActionListener);
  
	cmdCancel = new gcn::Button("Cancel");
	cmdCancel->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdCancel->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH, DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
  cmdCancel->setBaseColor(gui_baseCol + 0x202020);
  cmdCancel->setId("hdfCancel");
  cmdCancel->addActionListener(filesysHardfileActionListener);

  lblDevice = new gcn::Label("Device Name:");
  lblDevice->setSize(100, LABEL_HEIGHT);
  lblDevice->setAlignment(gcn::Graphics::RIGHT);
  txtDevice = new gcn::TextField();
  txtDevice->setSize(80, TEXTFIELD_HEIGHT);
  txtDevice->setId("hdfDev");

	chkReadWrite = new gcn::UaeCheckBox("Read/Write", true);
  chkReadWrite->setId("hdfRW");

	chkAutoboot = new gcn::UaeCheckBox("Bootable", true);
  chkAutoboot->setId("hdfAutoboot");

  lblBootPri = new gcn::Label("Boot priority:");
  lblBootPri->setSize(100, LABEL_HEIGHT);
  lblBootPri->setAlignment(gcn::Graphics::RIGHT);
  txtBootPri = new gcn::TextField();
  txtBootPri->setSize(40, TEXTFIELD_HEIGHT);
  txtBootPri->setId("hdfBootPri");

  lblSurfaces = new gcn::Label("Surfaces:");
  lblSurfaces->setSize(100, LABEL_HEIGHT);
  lblSurfaces->setAlignment(gcn::Graphics::RIGHT);
  txtSurfaces = new gcn::TextField();
  txtSurfaces->setSize(40, TEXTFIELD_HEIGHT);
  txtSurfaces->setId("hdfSurface");

  lblReserved = new gcn::Label("Reserved:");
  lblReserved->setSize(100, LABEL_HEIGHT);
  lblReserved->setAlignment(gcn::Graphics::RIGHT);
  txtReserved = new gcn::TextField();
  txtReserved->setSize(40, TEXTFIELD_HEIGHT);
  txtReserved->setId("hdfReserved");

  lblSectors = new gcn::Label("Sectors:");
  lblSectors->setSize(100, LABEL_HEIGHT);
  lblSectors->setAlignment(gcn::Graphics::RIGHT);
  txtSectors = new gcn::TextField();
  txtSectors->setSize(40, TEXTFIELD_HEIGHT);
  txtSectors->setId("hdfSectors");

  lblBlocksize = new gcn::Label("Blocksize:");
  lblBlocksize->setSize(100, LABEL_HEIGHT);
  lblBlocksize->setAlignment(gcn::Graphics::RIGHT);
  txtBlocksize = new gcn::TextField();
  txtBlocksize->setSize(40, TEXTFIELD_HEIGHT);
  txtBlocksize->setId("hdfBlocksize");

  lblPath = new gcn::Label("Path:");
  lblPath->setSize(100, LABEL_HEIGHT);
  lblPath->setAlignment(gcn::Graphics::RIGHT);
  txtPath = new gcn::TextField();
  txtPath->setSize(438, TEXTFIELD_HEIGHT);
  txtPath->setEnabled(false);
  cmdPath = new gcn::Button("...");
  cmdPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
  cmdPath->setBaseColor(gui_baseCol + 0x202020);
  cmdPath->setId("hdfPath");
  cmdPath->addActionListener(filesysHardfileActionListener);

  int posY = DISTANCE_BORDER;
  wndEditFilesysHardfile->add(lblDevice, DISTANCE_BORDER, posY);
  wndEditFilesysHardfile->add(txtDevice, DISTANCE_BORDER + lblDevice->getWidth() + 8, posY);
  wndEditFilesysHardfile->add(chkReadWrite, 235, posY + 1);
  wndEditFilesysHardfile->add(chkAutoboot, 360, posY + 1);
  wndEditFilesysHardfile->add(lblBootPri, 460, posY);
  wndEditFilesysHardfile->add(txtBootPri, 460 + lblBootPri->getWidth() + 8, posY);
  posY += txtDevice->getHeight() + DISTANCE_NEXT_Y;
  wndEditFilesysHardfile->add(lblPath, DISTANCE_BORDER, posY);
  wndEditFilesysHardfile->add(txtPath, DISTANCE_BORDER + lblPath->getWidth() + 8, posY);
  wndEditFilesysHardfile->add(cmdPath, wndEditFilesysHardfile->getWidth() - DISTANCE_BORDER - SMALL_BUTTON_WIDTH, posY);
  posY += txtPath->getHeight() + DISTANCE_NEXT_Y;
  wndEditFilesysHardfile->add(lblSurfaces, DISTANCE_BORDER, posY);
  wndEditFilesysHardfile->add(txtSurfaces, DISTANCE_BORDER + lblSurfaces->getWidth() + 8, posY);
  wndEditFilesysHardfile->add(lblReserved, 240, posY);
  wndEditFilesysHardfile->add(txtReserved, 240 + lblReserved->getWidth() + 8, posY);
  posY += txtSurfaces->getHeight() + DISTANCE_NEXT_Y;
  wndEditFilesysHardfile->add(lblSectors, DISTANCE_BORDER, posY);
  wndEditFilesysHardfile->add(txtSectors, DISTANCE_BORDER + lblSectors->getWidth() + 8, posY);
  wndEditFilesysHardfile->add(lblBlocksize, 240, posY);
  wndEditFilesysHardfile->add(txtBlocksize, 240 + lblBlocksize->getWidth() + 8, posY);
  posY += txtSectors->getHeight() + DISTANCE_NEXT_Y;

  wndEditFilesysHardfile->add(cmdOK);
  wndEditFilesysHardfile->add(cmdCancel);

  gui_top->add(wndEditFilesysHardfile);
  
  txtDevice->requestFocus();
  wndEditFilesysHardfile->requestModalFocus();
}


static void ExitEditFilesysHardfile(void)
{
  wndEditFilesysHardfile->releaseModalFocus();
  gui_top->remove(wndEditFilesysHardfile);

  delete lblDevice;
  delete txtDevice;
  delete chkReadWrite;
  delete chkAutoboot;
  delete lblBootPri;
  delete txtBootPri;
  delete lblPath;
  delete txtPath;
  delete cmdPath;
  delete lblSurfaces;
  delete txtSurfaces;
  delete lblReserved;
  delete txtReserved;
  delete lblSectors;
  delete txtSectors;
  delete lblBlocksize;
  delete txtBlocksize;
    
  delete cmdOK;
  delete cmdCancel;
  delete filesysHardfileActionListener;
  
  delete wndEditFilesysHardfile;
}


static void EditFilesysHardfileLoop(void)
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
    wait_for_vsync();
    SDL_Flip(gui_screen);
  }  
}


bool EditFilesysHardfile(int unit_no)
{
  struct mountedinfo mi;
  struct uaedev_config_data *uci;
  std::string strdevname, strroot;
  char tmp[32];
    
  dialogResult = false;
  dialogFinished = false;

  InitEditFilesysHardfile();

  if(unit_no >= 0)
  {
    struct uaedev_config_info *ci;

    uci = &changed_prefs.mountconfig[unit_no];
    ci = &uci->ci;
    get_filesys_unitconfig(&changed_prefs, unit_no, &mi);

    strdevname.assign(ci->devname);
    txtDevice->setText(strdevname);
    strroot.assign(ci->rootdir);
    txtPath->setText(strroot);
    fileSelected = true;

    chkReadWrite->setSelected(!ci->readonly);
    chkAutoboot->setSelected(ci->bootpri != BOOTPRI_NOAUTOBOOT);
    snprintf(tmp, 32, "%d", ci->bootpri >= -127 ? ci->bootpri : -127);
    txtBootPri->setText(tmp);
    snprintf(tmp, 32, "%d", ci->surfaces);
    txtSurfaces->setText(tmp);
    snprintf(tmp, 32, "%d", ci->reserved);
    txtReserved->setText(tmp);
    snprintf(tmp, 32, "%d", ci->sectors);
    txtSectors->setText(tmp);
    snprintf(tmp, 32, "%d", ci->blocksize);
    txtBlocksize->setText(tmp);
    
    check_rdb(strroot.c_str());
  }
  else
  {
    CreateDefaultDevicename(tmp);
    txtDevice->setText(tmp);
    strroot.assign(currentDir);
    txtPath->setText(strroot);
    fileSelected = false;
    
    chkReadWrite->setSelected(true);
    txtBootPri->setText("0");
    txtSurfaces->setText("1");
    txtReserved->setText("2");
    txtSectors->setText("32");
    txtBlocksize->setText("512");
  }

  EditFilesysHardfileLoop();
  
  if(dialogResult)
  {
    struct uaedev_config_info ci;
    int bp = tweakbootpri(atoi(txtBootPri->getText().c_str()), chkAutoboot->isSelected() ? 1 : 0, 0);
    extractPath((char *) txtPath->getText().c_str(), currentDir);

    uci_set_defaults(&ci, false);
    strcpy(ci.devname, (char *) txtDevice->getText().c_str());
    strcpy(ci.rootdir, (char *) txtPath->getText().c_str());
    ci.type = UAEDEV_HDF;
    ci.readonly = !chkReadWrite->isSelected();
    ci.sectors = atoi(txtSectors->getText().c_str());
    ci.surfaces = atoi(txtSurfaces->getText().c_str());
    ci.reserved = atoi(txtReserved->getText().c_str());
    ci.blocksize = atoi(txtBlocksize->getText().c_str());
    ci.bootpri = bp;
    
    uci = add_filesys_config(&changed_prefs, unit_no, &ci);
    if (uci) {
  		struct hardfiledata *hfd = get_hardfile_data (uci->configoffset);
      hardfile_media_change (hfd, &ci, true, false);
    }
  }

  ExitEditFilesysHardfile();

  return dialogResult;
}
