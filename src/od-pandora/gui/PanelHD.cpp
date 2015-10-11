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

static gcn::Label* lblList[COL_COUNT];
static gcn::Container* listEntry[MAX_HD_DEVICES];
static gcn::TextField* listCells[MAX_HD_DEVICES][COL_COUNT];
static gcn::Button* listCmdProps[MAX_HD_DEVICES];
static gcn::ImageButton* listCmdDelete[MAX_HD_DEVICES];
static gcn::Button* cmdAddDirectory;
static gcn::Button* cmdAddHardfile;
static gcn::Button* cmdCreateHardfile;
  

static int GetHDType(int index)
{
  int type;
  struct uaedev_config_info *uci;
  struct mountedinfo mi;

  type = get_filesys_unitconfig(&changed_prefs, index, &mi);
  if (type < 0) {
    uci = &changed_prefs.mountconfig[index];
		type = uci->ishdf ? FILESYS_HARDFILE : FILESYS_VIRTUAL;
  }
  return type;
}


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
            EditFilesysVirtual(i);
          else
            EditFilesysHardfile(i);
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
      EditFilesysVirtual(-1);
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
      EditFilesysHardfile(-1);
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
      CreateFilesysHardfile();
      cmdCreateHardfile->requestFocus();
      RefreshPanelHD();
    }
};
CreateHardfileActionListener* createHardfileActionListener;


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
  
  posY = category.panel->getHeight() - DISTANCE_BORDER - BUTTON_HEIGHT;
  category.panel->add(cmdAddDirectory, DISTANCE_BORDER, posY);
  category.panel->add(cmdAddHardfile, DISTANCE_BORDER + cmdAddDirectory->getWidth() + DISTANCE_NEXT_X, posY);
  category.panel->add(cmdCreateHardfile, cmdAddHardfile->getX() + cmdAddHardfile->getWidth() + DISTANCE_NEXT_X, posY);
  
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
  
  delete hdRemoveActionListener;
  delete hdEditActionListener;
  delete addVirtualHDActionListener;
  delete addHardfileActionListener;
  delete createHardfileActionListener;
}


void RefreshPanelHD(void)
{
  int row, col;
  char tmp[32];
  struct mountedinfo mi;
  struct uaedev_config_info *uci;
  int nosize = 0, type;
  
  for(row=0; row<MAX_HD_DEVICES; ++row)
  {
    uci = &changed_prefs.mountconfig[row];
    if(uci->devname && uci->devname[0])
    {
      type = get_filesys_unitconfig(&changed_prefs, row, &mi);
	    if (type < 0) {
    		type = uci->ishdf ? FILESYS_HARDFILE : FILESYS_VIRTUAL;
    		nosize = 1;
	    }
      
      if(type == FILESYS_VIRTUAL)
      {
        listCells[row][COL_DEVICE]->setText(uci->devname);
        listCells[row][COL_VOLUME]->setText(uci->volname);
        listCells[row][COL_PATH]->setText(uci->rootdir);
        if(uci->readonly)
          listCells[row][COL_READWRITE]->setText("no");
        else
          listCells[row][COL_READWRITE]->setText("yes");
        listCells[row][COL_SIZE]->setText("n/a");
        snprintf(tmp, 32, "%d", uci->bootpri);
        listCells[row][COL_BOOTPRI]->setText(tmp);
      }
      else
      {
        listCells[row][COL_DEVICE]->setText(uci->devname);
        listCells[row][COL_VOLUME]->setText("n/a");
        listCells[row][COL_PATH]->setText(uci->rootdir);
        if(uci->readonly)
          listCells[row][COL_READWRITE]->setText("no");
        else
          listCells[row][COL_READWRITE]->setText("yes");
  	    if (mi.size >= 1024 * 1024 * 1024)
	        snprintf (tmp, 32, "%.1fG", ((double)(uae_u32)(mi.size / (1024 * 1024))) / 1024.0);
  	    else
	        snprintf (tmp, 32, "%.1fM", ((double)(uae_u32)(mi.size / (1024))) / 1024.0);
        listCells[row][COL_SIZE]->setText(tmp);
        snprintf(tmp, 32, "%d", uci->bootpri);
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
}


int count_HDs(struct uae_prefs *p)
{
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
}
