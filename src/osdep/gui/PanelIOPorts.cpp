 #include <cstring>

#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "options.h"
#include "gui_handling.h"

#ifdef SERIAL_PORT
static gcn::Window* grpSerialDevice;
static gcn::TextField* txtSerialDevice;
static gcn::CheckBox* chkSerialDirect;
static gcn::CheckBox* chkRTSCTS;
static gcn::CheckBox* chkUaeSerial;
#endif

static gcn::Window* grpProtectionDongle;
static gcn::DropDown* cboProtectionDongle;

class string_list_model : public gcn::ListModel
{
	std::vector<std::string> values{};
public:
	string_list_model(const char* entries[], const int count)
	{
		for (auto i = 0; i < count; ++i)
			values.emplace_back(entries[i]);
	}

	int getNumberOfElements() override
	{
		return values.size();
	}

	int add_element(const char* elem) override
	{
		values.emplace_back(elem);
		return 0;
	}

	void clear_elements() override
	{
		values.clear();
	}

	std::string getElementAt(int i) override
	{
		if (i < 0 || i >= static_cast<int>(values.size()))
			return "---";
		return values[i];
	}
};

static const char* listValues[] = { "none", "RoboCop 3", "Leader Board", "B.A.T. II", "Italy '90 Soccer", "Dames Grand-Maître", "Rugby Coach", "Cricket Captain", "Leviathan", "Music Master", "Logistics/SuperBase", "Scala MM (Red)", "Scala MM (Green)"};
static string_list_model dongle_list(listValues, 13);

#ifdef SERIAL_PORT
class IOKeyListener : public gcn::KeyListener
{
public:
	void keyPressed(gcn::KeyEvent& keyEvent) override
	{
		if (keyEvent.getSource() == txtSerialDevice)
		{
			snprintf(changed_prefs.sername, 256, "%s", txtSerialDevice->getText().c_str());
			if (changed_prefs.sername[0])
				changed_prefs.use_serial = true;
			else
				changed_prefs.use_serial = false;
		}
	}
};
#endif

class IOActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
#ifdef SERIAL_PORT
		if (actionEvent.getSource() == txtSerialDevice)
			snprintf(changed_prefs.sername, 256, "%s", txtSerialDevice->getText().c_str());

		else if (actionEvent.getSource() == chkSerialDirect)
			changed_prefs.serial_direct = chkSerialDirect->isSelected();

		else if (actionEvent.getSource() == chkRTSCTS)
			changed_prefs.serial_hwctsrts = chkRTSCTS->isSelected();

		else if (actionEvent.getSource() == chkUaeSerial)
			changed_prefs.uaeserial = chkUaeSerial->isSelected();
#endif
		else if (actionEvent.getSource() == cboProtectionDongle)
			changed_prefs.dongle = cboProtectionDongle->getSelected();
	}
};

IOActionListener* ioActionListener;
#ifdef SERIAL_PORT
IOKeyListener* ioKeyListener;
#endif

void InitPanelIO(const config_category& category)
{
	ioActionListener = new IOActionListener();
#ifdef SERIAL_PORT
	ioKeyListener = new IOKeyListener();
#endif

#ifdef SERIAL_PORT
	grpSerialDevice = new gcn::Window("Serial Port");
	grpSerialDevice->setId("grpSerialDevice");

	txtSerialDevice = new gcn::TextField();
	txtSerialDevice->setId("txtSerialDevice");
	txtSerialDevice->addActionListener(ioActionListener);
	txtSerialDevice->addKeyListener(ioKeyListener);

	chkSerialDirect = new gcn::CheckBox("Direct");
	chkSerialDirect->setId("chkSerialDirect");
	chkSerialDirect->addActionListener(ioActionListener);

	chkRTSCTS = new gcn::CheckBox("RTS/CTS");
	chkRTSCTS->setId("chkRTSCTS");
	chkRTSCTS->addActionListener(ioActionListener);

	chkUaeSerial = new gcn::CheckBox("uaeserial.device");
	chkUaeSerial->setId("chkUaeSerial");
	chkUaeSerial->addActionListener(ioActionListener);
#endif

	cboProtectionDongle = new gcn::DropDown(&dongle_list);
	cboProtectionDongle->setSize(250, cboProtectionDongle->getHeight());
	cboProtectionDongle->setBaseColor(gui_baseCol);
	cboProtectionDongle->setBackgroundColor(colTextboxBackground);
	cboProtectionDongle->setId("cboProtectionDongle");
	cboProtectionDongle->addActionListener(ioActionListener);

	grpProtectionDongle = new gcn::Window("Protection Dongle");
	grpProtectionDongle->setId("grpProtectionDongle");

	auto posY = DISTANCE_BORDER;
#ifdef SERIAL_PORT
	grpSerialDevice->setPosition(DISTANCE_BORDER, posY);
	grpSerialDevice->setSize(450, TITLEBAR_HEIGHT + chkSerialDirect->getHeight() * 5);
	grpSerialDevice->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpSerialDevice->setBaseColor(gui_baseCol);
	txtSerialDevice->setSize(grpSerialDevice->getWidth() - DISTANCE_BORDER * 3, TEXTFIELD_HEIGHT);
	txtSerialDevice->setBackgroundColor(colTextboxBackground);
	grpSerialDevice->add(txtSerialDevice, DISTANCE_BORDER, DISTANCE_BORDER);
	grpSerialDevice->add(chkRTSCTS, DISTANCE_BORDER, DISTANCE_BORDER * 3);
	grpSerialDevice->add(chkSerialDirect, chkRTSCTS->getWidth() + chkRTSCTS->getX() + DISTANCE_NEXT_X, DISTANCE_BORDER * 3);
	grpSerialDevice->add(chkUaeSerial, chkSerialDirect->getWidth() + chkSerialDirect->getX() + DISTANCE_NEXT_X, DISTANCE_BORDER * 3);
	category.panel->add(grpSerialDevice);
	posY += grpSerialDevice->getHeight() + DISTANCE_NEXT_Y * 2;
#endif

	grpProtectionDongle->setPosition(DISTANCE_BORDER, posY);
	grpProtectionDongle->setSize(450, TITLEBAR_HEIGHT + 200);
	grpProtectionDongle->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpProtectionDongle->setBaseColor(gui_baseCol);
	grpProtectionDongle->add(cboProtectionDongle, DISTANCE_BORDER, DISTANCE_BORDER);
	category.panel->add(grpProtectionDongle);

	RefreshPanelIO();
}

void ExitPanelIO()
{
	delete ioActionListener;
#ifdef SERIAL_PORT
	delete ioKeyListener;

	delete grpSerialDevice;
	delete txtSerialDevice;
	delete chkRTSCTS;
	delete chkSerialDirect;
	delete chkUaeSerial;
#endif

	delete cboProtectionDongle;
	delete grpProtectionDongle;
}

void RefreshPanelIO()
{
#ifdef SERIAL_PORT
	chkRTSCTS->setSelected(changed_prefs.serial_hwctsrts);
	chkSerialDirect->setSelected(changed_prefs.serial_direct);
	txtSerialDevice->setText(changed_prefs.sername);
	chkUaeSerial->setSelected(changed_prefs.uaeserial);
#endif

	cboProtectionDongle->setSelected(changed_prefs.dongle);
}

bool HelpPanelIO(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("Serial Port emulates the Amiga UART through a hardware based UART device on the host.");
	helptext.emplace_back("For example /dev/ttyUSB0. For use on emulator to emulator use Direct ON and RTS/CTS OFF.");
	helptext.emplace_back("For emulator to physical device configurations the opposite should apply.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Protection dongle emulation allows you to emulate such a dongle, which some software");
	helptext.emplace_back("required in order to work.");
	return true;
}