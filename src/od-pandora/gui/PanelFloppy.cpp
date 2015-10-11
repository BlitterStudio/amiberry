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
#include "disk.h"
#include "uae.h"
#include "gui.h"
#include "target.h"
#include "gui_handling.h"


static gcn::UaeCheckBox* chkDFx[4];
static gcn::UaeDropDown* cboDFxType[4];
static gcn::UaeCheckBox* chkDFxWriteProtect[4];
static gcn::Button* cmdDFxInfo[4];
static gcn::Button* cmdDFxEject[4];
static gcn::Button* cmdDFxSelect[4];
static gcn::UaeDropDown* cboDFxFile[4];
static gcn::Label* lblDriveSpeed;
static gcn::Label* lblDriveSpeedInfo;
static gcn::Slider* sldDriveSpeed;
static gcn::UaeCheckBox* chkLoadConfig;
static gcn::Button *cmdSaveForDisk;
static gcn::Button *cmdCreateDDDisk;
static gcn::Button *cmdCreateHDDisk;

static const char *diskfile_filter[] = { ".adf", ".adz", ".zip", ".gz", ".dms", "\0" };
static const char *drivespeedlist[] = { "100% (compatible)", "200%", "400%", "800%" };
static const int drivespeedvalues[] = { 100, 200, 400, 800 };

static void AdjustDropDownControls(void);
static bool bLoadConfigForDisk = true;


class DriveTypeListModel : public gcn::ListModel
{
  private:
    std::vector<std::string> types;
      
  public:
    DriveTypeListModel()
    {
      types.push_back("Disabled");
      types.push_back("3.5'' DD");
      types.push_back("3.5'' HD");
      types.push_back("5.25'' SD");
      types.push_back("3.5'' ESCOM");
    }

    int getNumberOfElements()
    {
      return types.size();
    }

    std::string getElementAt(int i)
    {
      if(i < 0 || i >= types.size())
        return "---";
      return types[i];
    }
};
static DriveTypeListModel driveTypeList;


class DiskfileListModel : public gcn::ListModel
{
  public:
    DiskfileListModel()
    {
    }
    
    int getNumberOfElements()
    {
      return lstMRUDiskList.size();
    }

    std::string getElementAt(int i)
    {
      if(i < 0 || i >= lstMRUDiskList.size())
        return "---";
      return lstMRUDiskList[i];
    }
};
static DiskfileListModel diskfileList;


class DriveTypeActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
	    //---------------------------------------
	    // New drive type selected
	    //---------------------------------------
	    for(int i=0; i<4; ++i)
	    {
	      if (actionEvent.getSource() == cboDFxType[i])
          changed_prefs.dfxtype[i] = cboDFxType[i]->getSelected() - 1;
      }
      RefreshPanelFloppy();
    }
};
static DriveTypeActionListener* driveTypeActionListener;


class DFxCheckActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
	    if(actionEvent.getSource() == chkLoadConfig)
	      bLoadConfigForDisk = chkLoadConfig->isSelected();
	    else
      {
  	    for(int i=0; i<4; ++i)
  	    {
  	      if (actionEvent.getSource() == chkDFx[i])
          {
      	    //---------------------------------------
            // Drive enabled/disabled
      	    //---------------------------------------
            if(chkDFx[i]->isSelected())
              changed_prefs.dfxtype[i] = DRV_35_DD;
            else
              changed_prefs.dfxtype[i] = DRV_NONE;
          }
          else if(actionEvent.getSource() == chkDFxWriteProtect[i])
          {
      	    //---------------------------------------
            // Write-protect changed
      	    //---------------------------------------
            // ToDo: set write protect for floppy
          }
        }
      }
      RefreshPanelFloppy();
    }
};
static DFxCheckActionListener* dfxCheckActionListener;


class DFxButtonActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
	    for(int i=0; i<4; ++i)
	    {
	      if (actionEvent.getSource() == cmdDFxInfo[i])
        {
    	    //---------------------------------------
          // Show info about current disk-image
    	    //---------------------------------------
          if(changed_prefs.dfxtype[i] != DRV_NONE && strlen(changed_prefs.df[i]) > 0)
            ; // ToDo: Show info dialog
        }
        else if (actionEvent.getSource() == cmdDFxEject[i])
        {
    	    //---------------------------------------
          // Eject disk from drive
    	    //---------------------------------------
          disk_eject(i);
          strcpy(changed_prefs.df[i], "");
          AdjustDropDownControls();
        }
        else if (actionEvent.getSource() == cmdDFxSelect[i])
        {
    	    //---------------------------------------
          // Select disk for drive
    	    //---------------------------------------
    	    char tmp[MAX_PATH];

    	    if(strlen(changed_prefs.df[i]) > 0)
    	      strncpy(tmp, changed_prefs.df[i], MAX_PATH);
    	    else
    	      strncpy(tmp, currentDir, MAX_PATH);
    	    if(SelectFile("Select disk image file", tmp, diskfile_filter))
  	      {
      	    if(strncmp(changed_prefs.df[i], tmp, MAX_PATH))
      	    {
        	    strncpy(changed_prefs.df[i], tmp, sizeof(changed_prefs.df[i]));
        	    disk_insert(i, tmp);
        	    AddFileToDiskList(tmp, 1);
        	    extractPath(tmp, currentDir);

        	    if(i == 0 && chkLoadConfig->isSelected())
      	      {
      	        // Search for config of disk
      	        extractFileName(changed_prefs.df[i], tmp);
      	        removeFileExtension(tmp);
      	        LoadConfigByName(tmp);
      	      }
              AdjustDropDownControls();
      	    }
  	      }
  	      cmdDFxSelect[i]->requestFocus();
        }
      }
      RefreshPanelFloppy();
    }
};
static DFxButtonActionListener* dfxButtonActionListener;


static bool bIgnoreListChange = false;
class DiskFileActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
	    for(int i=0; i<4; ++i)
	    {
	      if (actionEvent.getSource() == cboDFxFile[i])
        {
    	    //---------------------------------------
          // Diskimage from list selected
    	    //---------------------------------------
    	    if(!bIgnoreListChange)
  	      {
      	    int idx = cboDFxFile[i]->getSelected();

      	    if(idx < 0)
    	      {
              disk_eject(i);
              strcpy(changed_prefs.df[i], "");
              AdjustDropDownControls();
    	      }
    	      else
      	    {
        	    if(diskfileList.getElementAt(idx).compare(changed_prefs.df[i]))
    	        {
          	    strncpy(changed_prefs.df[i], diskfileList.getElementAt(idx).c_str(), sizeof(changed_prefs.df[i]));
          	    disk_insert(i, changed_prefs.df[i]);
          	    lstMRUDiskList.erase(lstMRUDiskList.begin() + idx);
          	    lstMRUDiskList.insert(lstMRUDiskList.begin(), changed_prefs.df[i]);
                bIgnoreListChange = true;
                cboDFxFile[i]->setSelected(0);
                bIgnoreListChange = false;

          	    if(i == 0 && chkLoadConfig->isSelected())
        	      {
        	        // Search for config of disk
        	        char tmp[MAX_PATH];
        	        
        	        extractFileName(changed_prefs.df[i], tmp);
        	        removeFileExtension(tmp);
        	        LoadConfigByName(tmp);
                }
              }
      	    }
      	  }
        }
      }
      RefreshPanelFloppy();
    }
};
static DiskFileActionListener* diskFileActionListener;


class DriveSpeedSliderActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
  		changed_prefs.floppy_speed = drivespeedvalues[(int)sldDriveSpeed->getValue()];
  		RefreshPanelFloppy();
    }
};
static DriveSpeedSliderActionListener* driveSpeedSliderActionListener;


class SaveForDiskActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      //---------------------------------------
      // Save configuration for current disk
      //---------------------------------------
      if(strlen(changed_prefs.df[0]) > 0)
      {
        char filename[MAX_DPATH];
        char diskname[MAX_PATH];
        
        extractFileName(changed_prefs.df[0], diskname);
        removeFileExtension(diskname);
        
        fetch_configurationpath(filename, MAX_DPATH);
        strncat(filename, diskname, MAX_DPATH);
        strncat(filename, ".uae", MAX_DPATH);
        
        snprintf(changed_prefs.description, 256, "Configuration for disk '%s'", diskname);
        if(cfgfile_save(&changed_prefs, filename, 0))
          RefreshPanelConfig();
      }
    }
};
static SaveForDiskActionListener* saveForDiskActionListener;


class CreateDiskActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      if(actionEvent.getSource() == cmdCreateDDDisk)
      {
        // Create 3.5'' DD Disk
        char tmp[MAX_PATH];
        char diskname[MAX_PATH];
        strncpy(tmp, currentDir, MAX_PATH);
        if(SelectFile("Create 3.5'' DD disk file", tmp, diskfile_filter, true))
        {
          extractFileName(tmp, diskname);
          removeFileExtension(diskname);
          diskname[31] = '\0';
          disk_creatediskfile(tmp, 0, DRV_35_DD, diskname);
    	    AddFileToDiskList(tmp, 1);
    	    extractPath(tmp, currentDir);
        }
        cmdCreateDDDisk->requestFocus();
      }
      else if(actionEvent.getSource() == cmdCreateHDDisk)
      {
        // Create 3.5'' HD Disk
        char tmp[MAX_PATH];
        char diskname[MAX_PATH];
        strncpy(tmp, currentDir, MAX_PATH);
        if(SelectFile("Create 3.5'' HD disk file", tmp, diskfile_filter, true))
        {
          extractFileName(tmp, diskname);
          removeFileExtension(diskname);
          diskname[31] = '\0';
          disk_creatediskfile(tmp, 0, DRV_35_HD, diskname);
    	    AddFileToDiskList(tmp, 1);
    	    extractPath(tmp, currentDir);
        }
        cmdCreateHDDisk->requestFocus();
      }
    }
};
static CreateDiskActionListener* createDiskActionListener;


void InitPanelFloppy(const struct _ConfigCategory& category)
{
	int posX;
	int posY = DISTANCE_BORDER;
	int i;
	
	dfxCheckActionListener = new DFxCheckActionListener();
	driveTypeActionListener = new DriveTypeActionListener();
	dfxButtonActionListener = new DFxButtonActionListener();
	diskFileActionListener = new DiskFileActionListener();
	driveSpeedSliderActionListener = new DriveSpeedSliderActionListener();
	saveForDiskActionListener = new SaveForDiskActionListener();
	createDiskActionListener = new CreateDiskActionListener();
	
	for(i=0; i<4; ++i)
	{
	  char tmp[20];
	  snprintf(tmp, 20, "DF%d:", i); 
	  chkDFx[i] = new gcn::UaeCheckBox(tmp);
	  chkDFx[i]->addActionListener(dfxCheckActionListener);
	  
	  cboDFxType[i] = new gcn::UaeDropDown(&driveTypeList);
    cboDFxType[i]->setSize(106, DROPDOWN_HEIGHT);
    cboDFxType[i]->setBaseColor(gui_baseCol);
	  snprintf(tmp, 20, "cboType%d", i);
	  cboDFxType[i]->setId(tmp);
    cboDFxType[i]->addActionListener(driveTypeActionListener);
	  
	  chkDFxWriteProtect[i] = new gcn::UaeCheckBox("Write-protected");
	  chkDFxWriteProtect[i]->addActionListener(dfxCheckActionListener);
	  chkDFxWriteProtect[i]->setEnabled(false);
	  
    cmdDFxInfo[i] = new gcn::Button("?");
    cmdDFxInfo[i]->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
    cmdDFxInfo[i]->setBaseColor(gui_baseCol);
    cmdDFxInfo[i]->addActionListener(dfxButtonActionListener);

    cmdDFxEject[i] = new gcn::Button("Eject");
    cmdDFxEject[i]->setSize(SMALL_BUTTON_WIDTH * 2, SMALL_BUTTON_HEIGHT);
    cmdDFxEject[i]->setBaseColor(gui_baseCol);
	  snprintf(tmp, 20, "cmdEject%d", i);
	  cmdDFxEject[i]->setId(tmp);
    cmdDFxEject[i]->addActionListener(dfxButtonActionListener);

    cmdDFxSelect[i] = new gcn::Button("...");
    cmdDFxSelect[i]->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
    cmdDFxSelect[i]->setBaseColor(gui_baseCol);
	  snprintf(tmp, 20, "cmdSel%d", i);
	  cmdDFxSelect[i]->setId(tmp);
    cmdDFxSelect[i]->addActionListener(dfxButtonActionListener);

	  cboDFxFile[i] = new gcn::UaeDropDown(&diskfileList);
    cboDFxFile[i]->setSize(category.panel->getWidth() - 2 * DISTANCE_BORDER, DROPDOWN_HEIGHT);
    cboDFxFile[i]->setBaseColor(gui_baseCol);
	  snprintf(tmp, 20, "cboDisk%d", i);
	  cboDFxFile[i]->setId(tmp);
    cboDFxFile[i]->addActionListener(diskFileActionListener);

    if(i == 0)
    {
      chkLoadConfig = new gcn::UaeCheckBox("Load config with same name as disk");
      chkLoadConfig->setId("LoadDiskCfg");
      chkLoadConfig->addActionListener(dfxCheckActionListener);
    }
	}

	lblDriveSpeed = new gcn::Label("Floppy Drive Emulation Speed:");
  sldDriveSpeed = new gcn::Slider(0, 3);
  sldDriveSpeed->setSize(110, SLIDER_HEIGHT);
  sldDriveSpeed->setBaseColor(gui_baseCol);
	sldDriveSpeed->setMarkerLength(20);
	sldDriveSpeed->setStepLength(1);
	sldDriveSpeed->setId("DriveSpeed");
  sldDriveSpeed->addActionListener(driveSpeedSliderActionListener);
  lblDriveSpeedInfo = new gcn::Label(drivespeedlist[0]);

  cmdSaveForDisk = new gcn::Button("Save config for disk");
  cmdSaveForDisk->setSize(160, BUTTON_HEIGHT);
  cmdSaveForDisk->setBaseColor(gui_baseCol);
  cmdSaveForDisk->setId("SaveForDisk");
  cmdSaveForDisk->addActionListener(saveForDiskActionListener);

  cmdCreateDDDisk = new gcn::Button("Create 3.5'' DD disk");
  cmdCreateDDDisk->setSize(160, BUTTON_HEIGHT);
  cmdCreateDDDisk->setBaseColor(gui_baseCol);
  cmdCreateDDDisk->setId("CreateDD");
  cmdCreateDDDisk->addActionListener(createDiskActionListener);

  cmdCreateHDDisk = new gcn::Button("Create 3.5'' HD disk");
  cmdCreateHDDisk->setSize(160, BUTTON_HEIGHT);
  cmdCreateHDDisk->setBaseColor(gui_baseCol);
  cmdCreateHDDisk->setId("CreateHD");
  cmdCreateHDDisk->addActionListener(createDiskActionListener);
	
	for(i=0; i<4; ++i)
	{
	  posX = DISTANCE_BORDER;
	  category.panel->add(chkDFx[i], posX, posY);
	  posX += 100;
	  category.panel->add(cboDFxType[i], posX, posY);
	  posX += cboDFxType[i]->getWidth() + 2 * DISTANCE_NEXT_X;
	  category.panel->add(chkDFxWriteProtect[i], posX, posY);
	  posX += chkDFxWriteProtect[i]->getWidth() + 4 * DISTANCE_NEXT_X;
//	  category.panel->add(cmdDFxInfo[i], posX, posY);
	  posX += cmdDFxInfo[i]->getWidth() + DISTANCE_NEXT_X;
	  category.panel->add(cmdDFxEject[i], posX, posY);
	  posX += cmdDFxEject[i]->getWidth() + DISTANCE_NEXT_X;
	  category.panel->add(cmdDFxSelect[i], posX, posY);
	  posY += chkDFx[i]->getHeight() + 8;

	  category.panel->add(cboDFxFile[i], DISTANCE_BORDER, posY);
	  if(i == 0)
    {
  	  posY += cboDFxFile[i]->getHeight() + 8;
      category.panel->add(chkLoadConfig, DISTANCE_BORDER, posY);
    }
	  posY += cboDFxFile[i]->getHeight() + DISTANCE_NEXT_Y + 4;
  }
  
  posX = DISTANCE_BORDER;
  category.panel->add(lblDriveSpeed, posX, posY);
  posX += lblDriveSpeed->getWidth() + 8;
  category.panel->add(sldDriveSpeed, posX, posY);
  posX += sldDriveSpeed->getWidth() + DISTANCE_NEXT_X;
  category.panel->add(lblDriveSpeedInfo, posX, posY);
  posY += sldDriveSpeed->getHeight() + DISTANCE_NEXT_Y;

  posY = category.panel->getHeight() - DISTANCE_BORDER - BUTTON_HEIGHT;
  category.panel->add(cmdSaveForDisk, DISTANCE_BORDER, posY);
  category.panel->add(cmdCreateDDDisk, cmdSaveForDisk->getX() + cmdSaveForDisk->getWidth() + DISTANCE_NEXT_X, posY);
  category.panel->add(cmdCreateHDDisk, cmdCreateDDDisk->getX() + cmdCreateDDDisk->getWidth() + DISTANCE_NEXT_X, posY);
  
  RefreshPanelFloppy();
}


void ExitPanelFloppy(void)
{
	for(int i=0; i<4; ++i)
	{
	  delete chkDFx[i];
	  delete cboDFxType[i];
	  delete chkDFxWriteProtect[i];
	  delete cmdDFxInfo[i];
	  delete cmdDFxEject[i];
	  delete cmdDFxSelect[i];
	  delete cboDFxFile[i];
	}
  delete chkLoadConfig;
  delete lblDriveSpeed;
  delete sldDriveSpeed;
  delete lblDriveSpeedInfo;
  delete cmdSaveForDisk;
  delete cmdCreateDDDisk;
  delete cmdCreateHDDisk;
  
  delete dfxCheckActionListener;
  delete driveTypeActionListener;
  delete dfxButtonActionListener;
  delete diskFileActionListener;
  delete driveSpeedSliderActionListener;
  delete saveForDiskActionListener;
  delete createDiskActionListener;
}


static void AdjustDropDownControls(void)
{
  int i, j;
  
  bIgnoreListChange = true;
  
  for(i=0; i<4; ++i)
  {
    cboDFxFile[i]->clearSelected();

    if((changed_prefs.dfxtype[i] != DRV_NONE) && strlen(changed_prefs.df[i]) > 0)
    {
      for(j=0; j<lstMRUDiskList.size(); ++j)
      {
        if(!lstMRUDiskList[j].compare(changed_prefs.df[i]))
        {
          cboDFxFile[i]->setSelected(j);
          break;
        }
      }
    }
  }
       
  bIgnoreListChange = false;
}


void RefreshPanelFloppy(void)
{
  int i;
  bool prevAvailable = true;
  
  AdjustDropDownControls();

  changed_prefs.nr_floppies = 0;
  for(i=0; i<4; ++i)
  {
    bool driveEnabled = changed_prefs.dfxtype[i] != DRV_NONE;
    chkDFx[i]->setSelected(driveEnabled);
    cboDFxType[i]->setSelected(changed_prefs.dfxtype[i] + 1);
    chkDFxWriteProtect[i]->setSelected(disk_getwriteprotect(changed_prefs.df[i]));
    chkDFx[i]->setEnabled(prevAvailable);
    cboDFxType[i]->setEnabled(prevAvailable);
    
    cmdDFxInfo[i]->setEnabled(driveEnabled);
    cmdDFxEject[i]->setEnabled(driveEnabled);
    cmdDFxSelect[i]->setEnabled(driveEnabled);
    cboDFxFile[i]->setEnabled(driveEnabled);
    
    prevAvailable = driveEnabled;
    if(driveEnabled)
      changed_prefs.nr_floppies = i + 1;
  }

  chkLoadConfig->setSelected(bLoadConfigForDisk);
  
  for(i=0; i<4; ++i)
  {
    if(changed_prefs.floppy_speed == drivespeedvalues[i])
    {
      sldDriveSpeed->setValue(i);
      lblDriveSpeedInfo->setCaption(drivespeedlist[i]);
      break;
    }
  }
  
}
