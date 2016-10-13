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
#include "blkdev.h"
#include "gui.h"
#include "gui_handling.h"


enum { COL_DEVICE, COL_VOLUME, COL_PATH, COL_READWRITE, COL_SIZE, COL_BOOTPRI, COL_COUNT };

static const char *column_caption[] = {
  "Device", "Volume", "Path", "R/W", "Size", "Bootpri" };
static const int COLUMN_SIZE[] = {
   50,  // Device
   70,  // Volume
  260,  // Path
   40,  // R/W
   50,  // Size
   50   // Bootpri
};

static const char *cdfile_filter[] = { ".cue", ".ccd", ".iso", "\0" };
static void AdjustDropDownControls(void);

static gcn::Label* lblList[COL_COUNT];
static gcn::Container* listEntry[MAX_HD_DEVICES];
static gcn::TextField* listCells[MAX_HD_DEVICES][COL_COUNT];
static gcn::Button* listCmdProps[MAX_HD_DEVICES];
static gcn::ImageButton* listCmdDelete[MAX_HD_DEVICES];
static gcn::Button* cmdAddDirectory;
static gcn::Button* cmdAddHardfile;
static gcn::Button* cmdCreateHardfile;
static gcn::UaeCheckBox* chkCD;
static gcn::UaeDropDown* cboCDFile;
static gcn::Button* cmdCDEject;
static gcn::Button* cmdCDSelect;
static gcn::Label* lblCDVol;
static gcn::Label* lblCDVolInfo;
static gcn::Slider* sldCDVol;


static int GetHDType(int index)
{
  int type;
  struct uaedev_config_data *uci;
  struct mountedinfo mi;

  type = get_filesys_unitconfig(&changed_prefs, index, &mi);
  if (type < 0) {
    uci = &changed_prefs.mountconfig[index];
		type = (uci->ci.type == UAEDEV_DIR) ? FILESYS_VIRTUAL : FILESYS_HARDFILE;
  }
  return type;
}


class CDfileListModel : public gcn::ListModel
{
  public:
    CDfileListModel()
    {
    }
    
    int getNumberOfElements()
    {
      return lstMRUCDList.size();
    }

    std::string getElementAt(int i)
    {
      if(i < 0 || i >= lstMRUCDList.size())
        return "---";
      return lstMRUCDList[i];
    }
};
static CDfileListModel cdfileList;


class HDRemoveActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      for(int i=0; i<MAX_HD_DEVICES; ++i)
      {
        if (actionEvent.getSource() == listCmdDelete[i])
        {
          kill_filesys_unitconfig(&changed_prefs, i);
          gui_force_rtarea_hdchange();
          break;
        }
      }
      cmdAddDirectory->requestFocus();
      RefreshPanelHD();
    }
};
static HDRemoveActionListener* hdRemoveActionListener;


class HDEditActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      for(int i=0; i<MAX_HD_DEVICES; ++i)
      {
        if (actionEvent.getSource() == listCmdProps[i])
        {
          if (GetHDType(i) == FILESYS_VIRTUAL)
          {
            if(EditFilesysVirtual(i))
              gui_force_rtarea_hdchange();
          }
          else
          {
            if(EditFilesysHardfile(i))
              gui_force_rtarea_hdchange();
          }
          listCmdProps[i]->requestFocus();
          break;
        }
      }
      RefreshPanelHD();
    }
};
static HDEditActionListener* hdEditActionListener;


class AddVirtualHDActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      if(EditFilesysVirtual(-1))
        gui_force_rtarea_hdchange();
      cmdAddDirectory->requestFocus();
      RefreshPanelHD();
    }
};
AddVirtualHDActionListener* addVirtualHDActionListener;


class AddHardfileActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      if(EditFilesysHardfile(-1))
        gui_force_rtarea_hdchange();
      cmdAddHardfile->requestFocus();
      RefreshPanelHD();
    }
};
AddHardfileActionListener* addHardfileActionListener;


class CreateHardfileActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      if(CreateFilesysHardfile())
        gui_force_rtarea_hdchange();
      cmdCreateHardfile->requestFocus();
      RefreshPanelHD();
    }
};
CreateHardfileActionListener* createHardfileActionListener;


class CDCheckActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      if(changed_prefs.cdslots[0].inuse) {
        changed_prefs.cdslots[0].inuse = false;
        changed_prefs.cdslots[0].type = SCSI_UNIT_DISABLED;
      } else {
        changed_prefs.cdslots[0].inuse = true;
        changed_prefs.cdslots[0].type = SCSI_UNIT_IMAGE;
      }
      RefreshPanelHD();
    }
};
CDCheckActionListener* cdCheckActionListener;


class CDButtonActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      if (actionEvent.getSource() == cmdCDEject)
      {
  	    //---------------------------------------
        // Eject CD from drive
  	    //---------------------------------------
        strcpy(changed_prefs.cdslots[0].name, "");
        AdjustDropDownControls();
      } 
      else if(actionEvent.getSource() == cmdCDSelect)
      {
  	    char tmp[MAX_DPATH];

  	    if(strlen(changed_prefs.cdslots[0].name) > 0)
  	      strncpy(tmp, changed_prefs.cdslots[0].name, MAX_DPATH);
  	    else
  	      strncpy(tmp, currentDir, MAX_DPATH);

  	    if(SelectFile("Select CD image file", tmp, cdfile_filter))
	      {
    	    if(strncmp(changed_prefs.cdslots[0].name, tmp, MAX_DPATH))
    	    {
      	    strncpy(changed_prefs.cdslots[0].name, tmp, sizeof(changed_prefs.cdslots[0].name));
      	    changed_prefs.cdslots[0].inuse = true;
      	    changed_prefs.cdslots[0].type = SCSI_UNIT_IMAGE;
      	    AddFileToCDList(tmp, 1);
      	    extractPath(tmp, currentDir);

            AdjustDropDownControls();
    	    }
	      }
	      cmdCDSelect->requestFocus();
      }
      RefreshPanelHD();
    }
};
CDButtonActionListener* cdButtonActionListener;


class GenericActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      if (actionEvent.getSource() == sldCDVol)
      {
        int newvol = 100 - (int) sldCDVol->getValue();
        if(changed_prefs.sound_volume_cd != newvol)
        {
          changed_prefs.sound_volume_cd = newvol;
          RefreshPanelHD();
        }
      }    
    }
};
GenericActionListener* genericActionListener;


static bool bIgnoreListChange = false;
class CDFileActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
	    //---------------------------------------
      // CD image from list selected
	    //---------------------------------------
	    if(!bIgnoreListChange)
      {
  	    int idx = cboCDFile->getSelected();

  	    if(idx < 0)
	      {
          strcpy(changed_prefs.cdslots[0].name, "");
          AdjustDropDownControls();
	      }
	      else
  	    {
    	    if(cdfileList.getElementAt(idx).compare(changed_prefs.cdslots[0].name))
	        {
      	    strncpy(changed_prefs.cdslots[0].name, cdfileList.getElementAt(idx).c_str(), sizeof(changed_prefs.cdslots[0].name));
      	    changed_prefs.cdslots[0].inuse = true;
      	    changed_prefs.cdslots[0].type = SCSI_UNIT_IMAGE;
      	    lstMRUCDList.erase(lstMRUCDList.begin() + idx);
      	    lstMRUCDList.insert(lstMRUCDList.begin(), changed_prefs.cdslots[0].name);
            bIgnoreListChange = true;
            cboCDFile->setSelected(0);
            bIgnoreListChange = false;
          }
  	    }
      }
      RefreshPanelHD();
    }
};
static CDFileActionListener* cdFileActionListener;


void InitPanelHD(const struct _ConfigCategory& category)
{
  int row, col;
  int posX;
  int posY = DISTANCE_BORDER;
  char tmp[20];

  hdRemoveActionListener = new HDRemoveActionListener();
  hdEditActionListener = new HDEditActionListener();
  addVirtualHDActionListener = new AddVirtualHDActionListener();
  addHardfileActionListener = new AddHardfileActionListener();
  createHardfileActionListener = new CreateHardfileActionListener();

  for(col=0; col<COL_COUNT; ++col)
    lblList[col] = new gcn::Label(column_caption[col]);

  for(row=0; row<MAX_HD_DEVICES; ++row)
  {
    listEntry[row] = new gcn::Container();
    listEntry[row]->setSize(category.panel->getWidth() - 2 * DISTANCE_BORDER, TEXTFIELD_HEIGHT + 4);
    listEntry[row]->setBaseColor(gui_baseCol);
    listEntry[row]->setFrameSize(0);

    listCmdProps[row] = new gcn::Button("...");
    listCmdProps[row]->setBaseColor(gui_baseCol);
    listCmdProps[row]->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
    snprintf(tmp, 20, "cmdProp%d", row);
    listCmdProps[row]->setId(tmp);
    listCmdProps[row]->addActionListener(hdEditActionListener);

    listCmdDelete[row] = new gcn::ImageButton("data/delete.png");
    listCmdDelete[row]->setBaseColor(gui_baseCol);
    listCmdDelete[row]->setSize(SMALL_BUTTON_HEIGHT, SMALL_BUTTON_HEIGHT);
    snprintf(tmp, 20, "cmdDel%d", row);
    listCmdDelete[row]->setId(tmp);
    listCmdDelete[row]->addActionListener(hdRemoveActionListener);

    for(col=0; col<COL_COUNT; ++col)
    {
      listCells[row][col] = new gcn::TextField();
      listCells[row][col]->setSize(COLUMN_SIZE[col] - 8, TEXTFIELD_HEIGHT);
      listCells[row][col]->setEnabled(false);
      listCells[row][col]->setBackgroundColor(gui_baseCol);
    }
  }

  cmdAddDirectory = new gcn::Button("Add Directory");
  cmdAddDirectory->setBaseColor(gui_baseCol);
  cmdAddDirectory->setSize(BUTTON_WIDTH + 20, BUTTON_HEIGHT);
  cmdAddDirectory->setId("cmdAddDir");
  cmdAddDirectory->addActionListener(addVirtualHDActionListener);

  cmdAddHardfile = new gcn::Button("Add Hardfile");
  cmdAddHardfile->setBaseColor(gui_baseCol);
  cmdAddHardfile->setSize(BUTTON_WIDTH + 20, BUTTON_HEIGHT);
  cmdAddHardfile->setId("cmdAddHDF");
  cmdAddHardfile->addActionListener(addHardfileActionListener);

  cmdCreateHardfile = new gcn::Button("Create Hardfile");
  cmdCreateHardfile->setBaseColor(gui_baseCol);
  cmdCreateHardfile->setSize(BUTTON_WIDTH + 20, BUTTON_HEIGHT);
  cmdCreateHardfile->setId("cmdCreateHDF");
  cmdCreateHardfile->addActionListener(createHardfileActionListener);

  cdCheckActionListener = new CDCheckActionListener();
  cdButtonActionListener = new CDButtonActionListener();
  cdFileActionListener = new CDFileActionListener();
  genericActionListener = new GenericActionListener();

  chkCD = new gcn::UaeCheckBox("CD drive");
  chkCD->addActionListener(cdCheckActionListener);

  cmdCDEject = new gcn::Button("Eject");
  cmdCDEject->setSize(SMALL_BUTTON_WIDTH * 2, SMALL_BUTTON_HEIGHT);
  cmdCDEject->setBaseColor(gui_baseCol);
  cmdCDEject->setId("cdEject");
  cmdCDEject->addActionListener(cdButtonActionListener);

  cmdCDSelect = new gcn::Button("...");
  cmdCDSelect->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
  cmdCDSelect->setBaseColor(gui_baseCol);
  cmdCDSelect->setId("CDSelect");
  cmdCDSelect->addActionListener(cdButtonActionListener);

  cboCDFile = new gcn::UaeDropDown(&cdfileList);
  cboCDFile->setSize(category.panel->getWidth() - 2 * DISTANCE_BORDER, DROPDOWN_HEIGHT);
  cboCDFile->setBaseColor(gui_baseCol);
  cboCDFile->setId("cboCD");
  cboCDFile->addActionListener(cdFileActionListener);

  lblCDVol = new gcn::Label("CD Volume:");
  lblCDVol->setSize(80, LABEL_HEIGHT);
  lblCDVol->setAlignment(gcn::Graphics::RIGHT);
  sldCDVol = new gcn::Slider(0, 100);
  sldCDVol->setSize(200, SLIDER_HEIGHT);
  sldCDVol->setBaseColor(gui_baseCol);
  sldCDVol->setMarkerLength(20);
  sldCDVol->setStepLength(10);
  sldCDVol->setId("CDVol");
  sldCDVol->addActionListener(genericActionListener);
  lblCDVolInfo = new gcn::Label("80 %");

  posX = DISTANCE_BORDER + 2 + SMALL_BUTTON_WIDTH + 34;
  for(col=0; col<COL_COUNT; ++col)
  {
    category.panel->add(lblList[col], posX, posY);
    posX += COLUMN_SIZE[col];
  }
  posY += lblList[0]->getHeight() + 2;

  for(row=0; row<MAX_HD_DEVICES; ++row)
  {
    posX = 0;
    listEntry[row]->add(listCmdProps[row], posX, 2);
    posX += listCmdProps[row]->getWidth() + 4;
    listEntry[row]->add(listCmdDelete[row], posX, 2);
    posX += listCmdDelete[row]->getWidth() + 8;
    for(col=0; col<COL_COUNT; ++col)
    {
      listEntry[row]->add(listCells[row][col], posX, 2);
      posX += COLUMN_SIZE[col];
    }
    category.panel->add(listEntry[row], DISTANCE_BORDER, posY);
    posY += listEntry[row]->getHeight() + 4;
  }

  posY += DISTANCE_NEXT_Y;
  category.panel->add(cmdAddDirectory, DISTANCE_BORDER, posY);
  category.panel->add(cmdAddHardfile, DISTANCE_BORDER + cmdAddDirectory->getWidth() + DISTANCE_NEXT_X, posY);
  category.panel->add(cmdCreateHardfile, cmdAddHardfile->getX() + cmdAddHardfile->getWidth() + DISTANCE_NEXT_X, posY);

  posY += cmdAddDirectory->getHeight() + 2 * DISTANCE_NEXT_Y;
  category.panel->add(chkCD, DISTANCE_BORDER, posY + 2);
  category.panel->add(cmdCDEject, category.panel->getWidth() - cmdCDEject->getWidth() - DISTANCE_NEXT_X - cmdCDSelect->getWidth() - DISTANCE_BORDER, posY);
  category.panel->add(cmdCDSelect, category.panel->getWidth() - cmdCDSelect->getWidth() - DISTANCE_BORDER, posY);
  posY += cmdCDSelect->getHeight() + DISTANCE_NEXT_Y;
  category.panel->add(cboCDFile, DISTANCE_BORDER, posY);
  posY += cboCDFile->getHeight() + DISTANCE_NEXT_Y;

  category.panel->add(lblCDVol, DISTANCE_BORDER, posY);
  category.panel->add(sldCDVol, DISTANCE_BORDER + lblCDVol->getWidth() + 8, posY);
  category.panel->add(lblCDVolInfo, sldCDVol->getX() + sldCDVol->getWidth() + 12, posY);
  posY += sldCDVol->getHeight() + DISTANCE_NEXT_Y;

  RefreshPanelHD();
}


void ExitPanelHD(void)
{
  int row, col;

  for(col=0; col<COL_COUNT; ++col)
    delete lblList[col];

  for(row=0; row<MAX_HD_DEVICES; ++row)
  {
    delete listCmdProps[row];
    delete listCmdDelete[row];
    for(col=0; col<COL_COUNT; ++col)
      delete listCells[row][col];
    delete listEntry[row];
  }
  
  delete cmdAddDirectory;
  delete cmdAddHardfile;
  delete cmdCreateHardfile;
  
  delete chkCD;
  delete cmdCDEject;
  delete cmdCDSelect;
  delete cboCDFile;
  delete lblCDVol;
  delete lblCDVolInfo;
  delete sldCDVol;
  
  delete cdCheckActionListener;
  delete cdButtonActionListener;
  delete cdFileActionListener;
  delete genericActionListener;
  
  delete hdRemoveActionListener;
  delete hdEditActionListener;
  delete addVirtualHDActionListener;
  delete addHardfileActionListener;
  delete createHardfileActionListener;
}


static void AdjustDropDownControls(void)
{
  int i;

  cboCDFile->clearSelected();
  if((changed_prefs.cdslots[0].inuse) && strlen(changed_prefs.cdslots[0].name) > 0)
  {
    for(i = 0; i < lstMRUCDList.size(); ++i)
    {
      if(!lstMRUCDList[i].compare(changed_prefs.cdslots[0].name))
      {
        cboCDFile->setSelected(i);
        break;
      }
    }
  }
}

void RefreshPanelHD(void)
{
  int row, col;
  char tmp[32];
  struct mountedinfo mi;
  struct uaedev_config_data *uci;
  struct uaedev_config_info *ci;
  int nosize = 0, type;
  
  AdjustDropDownControls();

  for(row=0; row<MAX_HD_DEVICES; ++row)
  {
    if(row < changed_prefs.mountitems)
    {
      uci = &changed_prefs.mountconfig[row];
      ci = &uci->ci;
      type = get_filesys_unitconfig(&changed_prefs, row, &mi);
	    if (type < 0) {
    		type = (uci->ci.type == UAEDEV_DIR) ? FILESYS_VIRTUAL : FILESYS_HARDFILE;
    		nosize = 1;
	    }
			if (mi.size < 0)
				nosize = 1;
      
      if(type == FILESYS_VIRTUAL)
      {
        listCells[row][COL_DEVICE]->setText(ci->devname);
        listCells[row][COL_VOLUME]->setText(ci->volname);
        listCells[row][COL_PATH]->setText(ci->rootdir);
        if(ci->readonly)
          listCells[row][COL_READWRITE]->setText("no");
        else
          listCells[row][COL_READWRITE]->setText("yes");
        listCells[row][COL_SIZE]->setText("n/a");
        snprintf(tmp, 32, "%d", ci->bootpri);
        listCells[row][COL_BOOTPRI]->setText(tmp);
      }
      else
      {
        listCells[row][COL_DEVICE]->setText(ci->devname);
        listCells[row][COL_VOLUME]->setText("n/a");
        listCells[row][COL_PATH]->setText(ci->rootdir);
        if(ci->readonly)
          listCells[row][COL_READWRITE]->setText("no");
        else
          listCells[row][COL_READWRITE]->setText("yes");
  	    if (nosize)
  	      snprintf (tmp, 32, "n/a");
  	    else if (mi.size >= 1024 * 1024 * 1024)
	        snprintf (tmp, 32, "%.1fG", ((double)(uae_u32)(mi.size / (1024 * 1024))) / 1024.0);
  	    else
	        snprintf (tmp, 32, "%.1fM", ((double)(uae_u32)(mi.size / (1024))) / 1024.0);
        listCells[row][COL_SIZE]->setText(tmp);
        snprintf(tmp, 32, "%d", ci->bootpri);
        listCells[row][COL_BOOTPRI]->setText(tmp);
      }
      listCmdProps[row]->setEnabled(true);
      listCmdDelete[row]->setEnabled(true);
    }
    else
    {
      // Empty slot
      for(col=0; col<COL_COUNT; ++col)
        listCells[row][col]->setText("");
      listCmdProps[row]->setEnabled(false);
      listCmdDelete[row]->setEnabled(false);
    }
  }
  
  chkCD->setSelected(changed_prefs.cdslots[0].inuse);
  cmdCDEject->setEnabled(changed_prefs.cdslots[0].inuse);
  cmdCDSelect->setEnabled(changed_prefs.cdslots[0].inuse);
  cboCDFile->setEnabled(changed_prefs.cdslots[0].inuse);
  sldCDVol->setEnabled(changed_prefs.cdslots[0].inuse);
  
  sldCDVol->setValue(100 - changed_prefs.sound_volume_cd);
  snprintf(tmp, 32, "%d %%", 100 - changed_prefs.sound_volume_cd);
  lblCDVolInfo->setCaption(tmp);
}


int count_HDs(struct uae_prefs *p)
{
/*
  int row;
  struct uaedev_config_info *uci;
  int cnt = 0;
  
  for(row=0; row<MAX_HD_DEVICES; ++row)
  {
    uci = &p->mountconfig[row];
    if(uci->devname && uci->devname[0])
    {
      ++cnt;
    }
  }

  return cnt;
*/
  return p->mountitems;
}
