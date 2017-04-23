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

static gcn::Label* lblOpenGUI;
static gcn::TextField* txtOpenGUI;
static gcn::Button* cmdOpenGUI;

static gcn::Label* lblKeyForQuit;
static gcn::TextField* txtKeyForQuit;
static gcn::Button* cmdKeyForQuit;

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

		else if (actionEvent.getSource() == cmdOpenGUI)
		{
			const char* key = ShowMessageForInput("Press a key", "Press a key to map to Open the GUI", "Cancel");
			if (key != nullptr)
			{
				txtOpenGUI->setText(key);
				strcpy(changed_prefs.open_gui, key);
			}
		}

		else if (actionEvent.getSource() == cmdKeyForQuit)
		{
			const char* key = ShowMessageForInput("Press a key", "Press a key to map to Quit the emulator", "Cancel");
			if (key != nullptr)
			{
				txtKeyForQuit->setText(key);
				strcpy(changed_prefs.quit_amiberry, key);
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

	lblOpenGUI = new gcn::Label("Open GUI:");
	lblOpenGUI->setSize(85, LABEL_HEIGHT);
	lblOpenGUI->setAlignment(gcn::Graphics::RIGHT);
	txtOpenGUI = new gcn::TextField();
	txtOpenGUI->setEnabled(false);
	txtOpenGUI->setSize(85, txtOpenGUI->getHeight());
	txtOpenGUI->setBackgroundColor(colTextboxBackground);
	cmdOpenGUI = new gcn::Button("...");
	cmdOpenGUI->setId("OpenGUI");
	cmdOpenGUI->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdOpenGUI->setBaseColor(gui_baseCol);
	cmdOpenGUI->addActionListener(miscActionListener);

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

	category.panel->add(lblOpenGUI, DISTANCE_BORDER, posY);
	category.panel->add(txtOpenGUI, DISTANCE_BORDER + lblOpenGUI->getWidth() + 8, posY);
	category.panel->add(cmdOpenGUI, txtOpenGUI->getX() + txtOpenGUI->getWidth() + DISTANCE_NEXT_X, posY);

	category.panel->add(lblKeyForQuit, cmdOpenGUI->getX() + cmdOpenGUI->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(txtKeyForQuit, lblKeyForQuit->getX() + lblKeyForQuit->getWidth() + 8, posY);
	category.panel->add(cmdKeyForQuit, txtKeyForQuit->getX() + txtKeyForQuit->getWidth() + DISTANCE_NEXT_X, posY);

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
	delete lblOpenGUI;
	delete txtOpenGUI;

	delete lblKeyForQuit;
	delete txtKeyForQuit;
	delete cmdKeyForQuit;
}

void RefreshPanelMisc()
{
	chkStatusLine->setSelected(changed_prefs.leds_on_screen);
	chkShowGUI->setSelected(changed_prefs.start_gui);
	chkBSDSocket->setSelected(changed_prefs.socket_emu);
	cboKBDLed_num->setSelected(changed_prefs.kbd_led_num);
	cboKBDLed_scr->setSelected(changed_prefs.kbd_led_scr);

	txtOpenGUI->setText(changed_prefs.open_gui != "" ? changed_prefs.open_gui : "Click to map");
	txtKeyForQuit->setText(changed_prefs.quit_amiberry != "" ? changed_prefs.quit_amiberry : "Click to map");
}
