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
#include "options.h"
#include "gui.h"
#include "gui_handling.h"


static gcn::UaeCheckBox* chkStatusLine;
static gcn::UaeCheckBox* chkHideIdleLed;
static gcn::UaeCheckBox* chkShowGUI;
static gcn::UaeCheckBox* chkBSDSocket;

static gcn::Label* lblKeyForMenu;
static gcn::TextField* txtKeyForMenu;
static gcn::Button* cmdKeyForMenu;

static gcn::Label* lblButtonForMenu;
static gcn::TextField* txtButtonForMenu;
static gcn::Button* cmdButtonForMenu;

static gcn::Label* lblKeyForQuit;
static gcn::TextField* txtKeyForQuit;
static gcn::Button* cmdKeyForQuit;

static gcn::Label* lblButtonForQuit;
static gcn::TextField* txtButtonForQuit;
static gcn::Button* cmdButtonForQuit;

static gcn::Label* lblNumLock;
static gcn::UaeDropDown* cboKBDLed_num;
static gcn::Label* lblScrLock;
static gcn::UaeDropDown* cboKBDLed_scr;

class StringListModel : public gcn::ListModel
{
	vector<string> values;
public:
	StringListModel(const char* entries[], int count)
	{
		for (int i = 0; i < count; ++i)
			values.push_back(entries[i]);
	}

	int getNumberOfElements() override
	{
		return values.size();
	}

	string getElementAt(int i) override
	{
		if (i < 0 || i >= values.size())
			return "---";
		return values[i];
	}
};

const char* listValues[] = {"none", "POWER", "DF0", "DF1", "DF2", "DF3", "DF*", "HD", "CD"};
StringListModel KBDLedList(listValues, 9);

class MiscActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == chkStatusLine)
			changed_prefs.leds_on_screen = chkStatusLine->isSelected();

		else if (actionEvent.getSource() == chkShowGUI)
			changed_prefs.start_gui = chkShowGUI->isSelected();

		else if (actionEvent.getSource() == chkBSDSocket)
			changed_prefs.socket_emu = chkBSDSocket->isSelected();

		else if (actionEvent.getSource() == cmdKeyForMenu)
		{
			SDL_Keycode key = ShowMessageForKey("Press a key", "Press a key to map to Open the GUI", "Cancel");
			if (key > 0)
			{
				txtKeyForMenu->setText(SDL_GetScancodeName(SDL_GetScancodeFromKey(key)));
				changed_prefs.key_for_menu = key;
			}
		}

		else if (actionEvent.getSource() == cmdKeyForQuit)
		{
			SDL_Keycode key = ShowMessageForKey("Press a key", "Press a key to map to Quit the emulator", "Cancel");
			if (key > 0)
			{
				txtKeyForQuit->setText(SDL_GetScancodeName(SDL_GetScancodeFromKey(key)));
				changed_prefs.key_for_quit = key;
			}
		}

		else if (actionEvent.getSource() == cmdButtonForMenu)
		{
			Uint8 button = ShowMessageForButton("Press a button", "Press a button to map to Open the GUI", "Cancel");
			if (button > 0)
			{
				txtKeyForQuit->setText(string(reinterpret_cast<char const*>(button)));
				changed_prefs.button_for_menu = button;
			}
		}

		else if (actionEvent.getSource() == cmdButtonForQuit)
		{
			Uint8 button = ShowMessageForButton("Press a button", "Press a button to map to Quit the emulator", "Cancel");
			if (button > 0)
			{
				txtKeyForQuit->setText(string(reinterpret_cast<char const*>(button)));
				changed_prefs.button_for_quit = button;
			}
		}

		else if (actionEvent.getSource() == cboKBDLed_num)
			changed_prefs.kbd_led_num = cboKBDLed_num->getSelected();

		else if (actionEvent.getSource() == cboKBDLed_scr)
			changed_prefs.kbd_led_scr = cboKBDLed_scr->getSelected();
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
	cboKBDLed_num->setBaseColor(gui_baseCol);
	cboKBDLed_num->setBackgroundColor(colTextboxBackground);
	cboKBDLed_num->setId("numlock");
	cboKBDLed_num->addActionListener(miscActionListener);

	lblScrLock = new gcn::Label("ScrollLock:");
	lblScrLock->setSize(85, LABEL_HEIGHT);
	lblScrLock->setAlignment(gcn::Graphics::RIGHT);
	cboKBDLed_scr = new gcn::UaeDropDown(&KBDLedList);
	cboKBDLed_scr->setBaseColor(gui_baseCol);
	cboKBDLed_scr->setBackgroundColor(colTextboxBackground);
	cboKBDLed_scr->setId("scrolllock");
	cboKBDLed_scr->addActionListener(miscActionListener);

	lblKeyForMenu = new gcn::Label("Menu Key:");
	lblKeyForMenu->setSize(85, LABEL_HEIGHT);
	lblKeyForMenu->setAlignment(gcn::Graphics::RIGHT);
	txtKeyForMenu = new gcn::TextField();
	txtKeyForMenu->setEnabled(false);
	txtKeyForMenu->setSize(85, txtKeyForMenu->getHeight());
	txtKeyForMenu->setBackgroundColor(colTextboxBackground);
	cmdKeyForMenu = new gcn::Button("...");
	cmdKeyForMenu->setId("KeyForMenu");
	cmdKeyForMenu->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyForMenu->setBaseColor(gui_baseCol);
	cmdKeyForMenu->addActionListener(miscActionListener);

	lblKeyForQuit = new gcn::Label("Quit Key:");
	lblKeyForQuit->setSize(85, LABEL_HEIGHT);
	lblKeyForQuit->setAlignment(gcn::Graphics::RIGHT);
	txtKeyForQuit = new gcn::TextField();
	txtKeyForQuit->setEnabled(false);
	txtKeyForQuit->setSize(85, txtKeyForQuit->getHeight());
	txtKeyForQuit->setBackgroundColor(colTextboxBackground);
	cmdKeyForQuit = new gcn::Button("...");
	cmdKeyForQuit->setId("KeyForQuit");
	cmdKeyForQuit->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyForQuit->setBaseColor(gui_baseCol);
	cmdKeyForQuit->addActionListener(miscActionListener);


	lblButtonForMenu = new gcn::Label("Menu Button:");
	lblButtonForMenu->setSize(85, LABEL_HEIGHT);
	lblButtonForMenu->setAlignment(gcn::Graphics::RIGHT);
	txtButtonForMenu = new gcn::TextField();
	txtButtonForMenu->setEnabled(false);
	txtButtonForMenu->setSize(85, txtButtonForMenu->getHeight());
	txtButtonForMenu->setBackgroundColor(colTextboxBackground);
	cmdButtonForMenu = new gcn::Button("...");
	cmdButtonForMenu->setId("ButtonForMenu");
	cmdButtonForMenu->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdButtonForMenu->setBaseColor(gui_baseCol);
	cmdButtonForMenu->addActionListener(miscActionListener);

	lblButtonForQuit = new gcn::Label("Quit Button:");
	lblButtonForQuit->setSize(85, LABEL_HEIGHT);
	lblButtonForQuit->setAlignment(gcn::Graphics::RIGHT);
	txtButtonForQuit = new gcn::TextField();
	txtButtonForQuit->setBaseColor(gui_baseCol);
	txtButtonForQuit->setEnabled(false);
	txtButtonForQuit->setSize(85, txtButtonForQuit->getHeight());
	txtButtonForQuit->setBackgroundColor(colTextboxBackground);
	cmdButtonForQuit = new gcn::Button("...");
	cmdButtonForQuit->setId("ButtonForQuit");
	cmdButtonForQuit->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdButtonForQuit->setBaseColor(gui_baseCol);
	cmdButtonForQuit->addActionListener(miscActionListener);

	int posY = DISTANCE_BORDER;
	category.panel->add(chkStatusLine, DISTANCE_BORDER, posY);
	posY += chkStatusLine->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkHideIdleLed, DISTANCE_BORDER, posY);
	posY += chkHideIdleLed->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkShowGUI, DISTANCE_BORDER, posY);
	posY += chkShowGUI->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(chkBSDSocket, DISTANCE_BORDER, posY);
	posY += chkBSDSocket->getHeight() + DISTANCE_NEXT_Y * 2;

	category.panel->add(lblNumLock, DISTANCE_BORDER, posY);
	category.panel->add(cboKBDLed_num, DISTANCE_BORDER + lblNumLock->getWidth() + 8, posY);

	category.panel->add(lblScrLock, cboKBDLed_num->getX() + cboKBDLed_num->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(cboKBDLed_scr, lblScrLock->getX() + lblScrLock->getWidth() + 8, posY);

	posY += cboKBDLed_scr->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblKeyForMenu, DISTANCE_BORDER, posY);
	category.panel->add(txtKeyForMenu, DISTANCE_BORDER + lblKeyForMenu->getWidth() + 8, posY);
	category.panel->add(cmdKeyForMenu, txtKeyForMenu->getX() + txtKeyForMenu->getWidth() + DISTANCE_NEXT_X, posY);

	category.panel->add(lblKeyForQuit, cmdKeyForMenu->getX() + cmdKeyForMenu->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(txtKeyForQuit, lblKeyForQuit->getX() + lblKeyForQuit->getWidth() + 8, posY);
	category.panel->add(cmdKeyForQuit, txtKeyForQuit->getX() + txtKeyForQuit->getWidth() + DISTANCE_NEXT_X, posY);

	posY += txtKeyForMenu->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblButtonForMenu, DISTANCE_BORDER, posY);
	category.panel->add(txtButtonForMenu, DISTANCE_BORDER + lblButtonForMenu->getWidth() + 8, posY);
	category.panel->add(cmdButtonForMenu, txtButtonForMenu->getX() + txtButtonForMenu->getWidth() + DISTANCE_NEXT_X, posY);

	category.panel->add(lblButtonForQuit, cmdButtonForMenu->getX() + cmdButtonForMenu->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(txtButtonForQuit, lblButtonForQuit->getX() + lblButtonForQuit->getWidth() + 8, posY);
	category.panel->add(cmdButtonForQuit, txtButtonForQuit->getX() + txtButtonForQuit->getWidth() + DISTANCE_NEXT_X, posY);

	RefreshPanelMisc();
}

void ExitPanelMisc()
{
	delete chkStatusLine;
	delete chkHideIdleLed;
	delete chkShowGUI;
	delete chkBSDSocket;

	delete lblScrLock;
	delete lblNumLock;
	delete cboKBDLed_num;
	delete cboKBDLed_scr;

	delete miscActionListener;
	delete lblKeyForMenu;
	delete txtKeyForMenu;
	
	delete lblKeyForQuit;
	delete txtKeyForQuit;
	delete cmdKeyForQuit;

	delete lblButtonForMenu;
	delete txtButtonForMenu;
	delete cmdButtonForMenu;

	delete lblButtonForQuit;
	delete txtButtonForQuit;
	delete cmdButtonForQuit;
}

void RefreshPanelMisc()
{
	chkStatusLine->setSelected(changed_prefs.leds_on_screen);
	chkShowGUI->setSelected(changed_prefs.start_gui);
	chkBSDSocket->setSelected(changed_prefs.socket_emu);
	cboKBDLed_num->setSelected(changed_prefs.kbd_led_num);
	cboKBDLed_scr->setSelected(changed_prefs.kbd_led_scr);

	txtKeyForMenu->setText(changed_prefs.key_for_menu > 0 ? SDL_GetScancodeName(SDL_GetScancodeFromKey(SDL_Keycode(changed_prefs.key_for_menu))) : "Click to map");
	txtKeyForQuit->setText(changed_prefs.key_for_quit > 0 ? SDL_GetScancodeName(SDL_GetScancodeFromKey(SDL_Keycode(changed_prefs.key_for_quit))) : "Click to map");

	txtButtonForMenu->setText(changed_prefs.button_for_menu > 0 ? SDL_GetScancodeName(SDL_GetScancodeFromKey(SDL_Keycode(changed_prefs.button_for_menu))) : "Click to map");
	txtButtonForQuit->setText(changed_prefs.button_for_quit > 0 ? SDL_GetScancodeName(SDL_GetScancodeFromKey(SDL_Keycode(changed_prefs.button_for_quit))) : "Click to map");
}
