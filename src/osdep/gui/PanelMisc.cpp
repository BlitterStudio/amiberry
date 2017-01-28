#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include "guisan/sdl/sdltruetypefont.hpp"
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


static gcn::UaeCheckBox* chkStatusLine;
static gcn::UaeCheckBox* chkHideIdleLed;
static gcn::UaeCheckBox* chkShowGUI;
static gcn::UaeCheckBox* chkBSDSocket;
static gcn::Label *lblKeyForMenu;
static gcn::UaeDropDown* KeyForMenu;
static gcn::Label *lblButtonForMenu;
static gcn::UaeDropDown* ButtonForMenu;
static gcn::Label *lblKeyForQuit;
static gcn::UaeDropDown* KeyForQuit;
static gcn::Label *lblButtonForQuit;
static gcn::UaeDropDown* ButtonForQuit;

#ifdef RASPBERRY
class StringListModel : public gcn::ListModel
{
private:
    std::vector<std::string> values;
public:
    StringListModel(const char *entries[], int count)
    {
        for(int i=0; i<count; ++i)
            values.push_back(entries[i]);
    }

    int getNumberOfElements()
    {
        return values.size();
    }

    std::string getElementAt(int i)
    {
        if(i < 0 || i >= values.size())
            return "---";
        return values[i];
    }
};


static gcn::Label *lblNumLock;
static gcn::UaeDropDown* cboKBDLed_num;
static gcn::Label *lblScrLock;
static gcn::UaeDropDown* cboKBDLed_scr;

const char *listValues[] = { "none", "POWER", "DF0", "DF1", "DF2", "DF3", "DF*", "HD", "CD" };
StringListModel KBDLedList(listValues, 9);
#endif

static const int ControlKey_SDLKeyValues[] = { 0, SDLK_F11, SDLK_F12 };

const char *ControlKeyValues[] = { "------------------", "F11", "F12" };
StringListModel ControlKeyList(ControlKeyValues, 3);

static int GetControlKeyIndex(int key)
{
	int ControlKey_SDLKeyValues_Length = sizeof(ControlKey_SDLKeyValues) / sizeof(int);
	for (int i = 0; i < (ControlKey_SDLKeyValues_Length + 1); ++i)
	{
		if (ControlKey_SDLKeyValues[i] == key)
			return i;
	}
	return 0; // Default: no key
}

static const int ControlButton_SDLButtonValues[] = { -1, 0, 1, 2, 3 };

const char *ControlButtonValues[] = { "------------------", "JoyButton0", "JoyButton1", "JoyButton2", "JoyButton3" };
StringListModel ControlButtonList(ControlButtonValues, 5);

static int GetControlButtonIndex(int button)
{
	int ControlButton_SDLButtonValues_Length = sizeof(ControlButton_SDLButtonValues) / sizeof(int);
	for (int i = 0; i < (ControlButton_SDLButtonValues_Length + 1); ++i)
	{
		if (ControlButton_SDLButtonValues[i] == button)
			return i;
	}
	return 0; // Default: no key
}

class MiscActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent)
    {
        if (actionEvent.getSource() == chkStatusLine)
            changed_prefs.leds_on_screen = chkStatusLine->isSelected();

        else if (actionEvent.getSource() == chkHideIdleLed)
            changed_prefs.pandora_hide_idle_led = chkHideIdleLed->isSelected();

        else if (actionEvent.getSource() == chkShowGUI)
            changed_prefs.start_gui = chkShowGUI->isSelected();

        else if (actionEvent.getSource() == chkBSDSocket)
            changed_prefs.socket_emu = chkBSDSocket->isSelected();

	    else if (actionEvent.getSource() == KeyForMenu)
		    changed_prefs.key_for_menu = ControlKey_SDLKeyValues[KeyForMenu->getSelected()];
        
	    else if (actionEvent.getSource() == KeyForQuit)
		    changed_prefs.key_for_quit = ControlKey_SDLKeyValues[KeyForQuit->getSelected()];

	    else if (actionEvent.getSource() == ButtonForMenu)
		    changed_prefs.button_for_menu = ControlButton_SDLButtonValues[ButtonForMenu->getSelected()];
        
	    else if (actionEvent.getSource() == ButtonForQuit)
		    changed_prefs.button_for_quit = ControlButton_SDLButtonValues[ButtonForQuit->getSelected()];

#ifdef RASPBERRY
        else if (actionEvent.getSource() == cboKBDLed_num)
	        changed_prefs.kbd_led_num = cboKBDLed_num->getSelected();

        else if (actionEvent.getSource() == cboKBDLed_scr)
			changed_prefs.kbd_led_scr = cboKBDLed_scr->getSelected();
#endif
    }
};
MiscActionListener* miscActionListener;


void InitPanelMisc(const struct _ConfigCategory& category)
{
	miscActionListener = new MiscActionListener();

	chkStatusLine = new gcn::UaeCheckBox("Status Line");
	chkStatusLine->setId("StatusLine");
	chkStatusLine->addActionListener(miscActionListener);

	chkHideIdleLed = new gcn::UaeCheckBox("Hide idle led");
	chkHideIdleLed->setId("HideIdle");
	chkHideIdleLed->addActionListener(miscActionListener);

	chkShowGUI = new gcn::UaeCheckBox("Show GUI on startup");
	chkShowGUI->setId("ShowGUI");
	chkShowGUI->addActionListener(miscActionListener);

	chkBSDSocket = new gcn::UaeCheckBox("bsdsocket.library");
	chkBSDSocket->setId("BSDSocket");
	chkBSDSocket->addActionListener(miscActionListener);

	lblNumLock = new gcn::Label("NumLock:");
	lblNumLock->setSize(85, LABEL_HEIGHT);
	lblNumLock->setAlignment(gcn::Graphics::RIGHT);
	cboKBDLed_num = new gcn::UaeDropDown(&KBDLedList);
	cboKBDLed_num->setSize(100, DROPDOWN_HEIGHT);
	cboKBDLed_num->setBaseColor(gui_baseCol);
	cboKBDLed_num->setId("numlock");
	cboKBDLed_num->addActionListener(miscActionListener);

	lblScrLock = new gcn::Label("ScrollLock:");
	lblScrLock->setSize(85, LABEL_HEIGHT);
	lblScrLock->setAlignment(gcn::Graphics::RIGHT);
	cboKBDLed_scr = new gcn::UaeDropDown(&KBDLedList);
	cboKBDLed_scr->setSize(100, DROPDOWN_HEIGHT);
	cboKBDLed_scr->setBaseColor(gui_baseCol);
	cboKBDLed_scr->setId("scrolllock");
	cboKBDLed_scr->addActionListener(miscActionListener);

	lblKeyForMenu = new gcn::Label("Menu Key:");
	lblKeyForMenu->setSize(85, LABEL_HEIGHT);
	lblKeyForMenu->setAlignment(gcn::Graphics::RIGHT);
	KeyForMenu = new gcn::UaeDropDown(&ControlKeyList);
	KeyForMenu->setSize(100, DROPDOWN_HEIGHT);
	KeyForMenu->setBaseColor(gui_baseCol);
	KeyForMenu->setId("KeyForMenu");
	KeyForMenu->addActionListener(miscActionListener);

	lblKeyForQuit = new gcn::Label("Quit Key:");
	lblKeyForQuit->setSize(85, LABEL_HEIGHT);
	lblKeyForQuit->setAlignment(gcn::Graphics::RIGHT);
	KeyForQuit = new gcn::UaeDropDown(&ControlKeyList);
	KeyForQuit->setSize(100, DROPDOWN_HEIGHT);
	KeyForQuit->setBaseColor(gui_baseCol);
	KeyForQuit->setId("KeyForQuit");
	KeyForQuit->addActionListener(miscActionListener);

	lblButtonForMenu = new gcn::Label("Menu Button:");
	lblButtonForMenu->setSize(85, LABEL_HEIGHT);
	lblButtonForMenu->setAlignment(gcn::Graphics::RIGHT);
	ButtonForMenu = new gcn::UaeDropDown(&ControlButtonList);
	ButtonForMenu->setSize(100, DROPDOWN_HEIGHT);
	ButtonForMenu->setBaseColor(gui_baseCol);
	ButtonForMenu->setId("ButtonForMenu");
	ButtonForMenu->addActionListener(miscActionListener);

	lblButtonForQuit = new gcn::Label("Quit Button:");
	lblButtonForQuit->setSize(85, LABEL_HEIGHT);
	lblButtonForQuit->setAlignment(gcn::Graphics::RIGHT);
	ButtonForQuit = new gcn::UaeDropDown(&ControlButtonList);
	ButtonForQuit->setSize(100, DROPDOWN_HEIGHT);
	ButtonForQuit->setBaseColor(gui_baseCol);
	ButtonForQuit->setId("ButtonForQuit");
	ButtonForQuit->addActionListener(miscActionListener);

	int posY = DISTANCE_BORDER;
	category.panel->add(chkStatusLine, DISTANCE_BORDER, posY);
	posY += chkStatusLine->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkHideIdleLed, DISTANCE_BORDER, posY);
	posY += chkHideIdleLed->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkShowGUI, DISTANCE_BORDER, posY);
	posY += chkShowGUI->getHeight() + DISTANCE_NEXT_Y;
	
	category.panel->add(chkBSDSocket, DISTANCE_BORDER, posY);
	posY += chkBSDSocket->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblNumLock, DISTANCE_BORDER, posY);
	category.panel->add(cboKBDLed_num, DISTANCE_BORDER + lblNumLock->getWidth() + 8, posY);
	
	category.panel->add(lblScrLock, cboKBDLed_num->getX() + cboKBDLed_num->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(cboKBDLed_scr, lblScrLock->getX() + lblScrLock->getWidth() + 8, posY);
		
	posY += cboKBDLed_scr->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblKeyForMenu, DISTANCE_BORDER, posY);
	category.panel->add(KeyForMenu, DISTANCE_BORDER + lblKeyForMenu->getWidth() + 8, posY);
	
	category.panel->add(lblKeyForQuit, KeyForMenu->getX() + KeyForMenu->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(KeyForQuit, lblKeyForQuit->getX() + lblKeyForQuit->getWidth() + 8, posY);
	
	posY += KeyForMenu->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblButtonForMenu, DISTANCE_BORDER, posY);
	category.panel->add(ButtonForMenu, DISTANCE_BORDER + lblButtonForMenu->getWidth() + 8, posY);
	
	category.panel->add(lblButtonForQuit, ButtonForMenu->getX() + ButtonForMenu->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(ButtonForQuit, lblButtonForQuit->getX() + lblButtonForQuit->getWidth() + 8, posY);
	
	RefreshPanelMisc();
}

void ExitPanelMisc(void)
{
	delete chkStatusLine;
	delete chkHideIdleLed;
	delete chkShowGUI;
	delete chkBSDSocket;

#ifdef RASPBERRY
	delete lblScrLock;
	delete lblNumLock;
	delete cboKBDLed_num;
	delete cboKBDLed_scr;
#endif
	delete miscActionListener;
	delete lblKeyForMenu;
	delete KeyForMenu;
	delete lblKeyForQuit;
	delete KeyForQuit;
	delete lblButtonForMenu;
	delete ButtonForMenu;
	delete lblButtonForQuit;
	delete ButtonForQuit;
}

void RefreshPanelMisc(void)
{
    char tmp[20];

	chkStatusLine->setSelected(changed_prefs.leds_on_screen);
	chkHideIdleLed->setSelected(changed_prefs.pandora_hide_idle_led);
	chkShowGUI->setSelected(changed_prefs.start_gui);

	chkBSDSocket->setSelected(changed_prefs.socket_emu);
#ifdef RASPBERRY
	cboKBDLed_num->setSelected(changed_prefs.kbd_led_num);
	cboKBDLed_scr->setSelected(changed_prefs.kbd_led_scr);
#endif
	KeyForMenu->setSelected(GetControlKeyIndex(changed_prefs.key_for_menu));
	KeyForQuit->setSelected(GetControlKeyIndex(changed_prefs.key_for_quit));
	ButtonForMenu->setSelected(GetControlButtonIndex(changed_prefs.button_for_menu));
	ButtonForQuit->setSelected(GetControlButtonIndex(changed_prefs.button_for_quit));
}
