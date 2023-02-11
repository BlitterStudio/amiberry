 #include <cstring>

#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "options.h"
#include "gui_handling.h"
#include "sounddep/sound.h"

#ifdef SERIAL_PORT
static gcn::Window* grpSerialDevice;
static gcn::TextField* txtSerialDevice;
static gcn::CheckBox* chkSerialDirect;
static gcn::CheckBox* chkRTSCTS;
static gcn::CheckBox* chkUaeSerial;
#endif

static gcn::Label* lblProtectionDongle;
static gcn::DropDown* cboProtectionDongle;

static gcn::Label* lblSampler;
static gcn::DropDown* cboSampler;
static gcn::CheckBox* chkSamplerStereo;

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

static string_list_model sampler_list(nullptr, 0);

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
		else if (actionEvent.getSource() == cboSampler)
		{
			auto item = cboSampler->getSelected();
			changed_prefs.samplersoundcard = item - 1;
			if (item > 0)
				changed_prefs.prtname[0] = 0;
		}
		else if (actionEvent.getSource() == chkSamplerStereo)
			changed_prefs.sampler_stereo = chkSamplerStereo->isSelected();

		RefreshPanelIO();
	}
};

IOActionListener* ioActionListener;
#ifdef SERIAL_PORT
IOKeyListener* ioKeyListener;
#endif

void InitPanelIO(const config_category& category)
{
	enumerate_sound_devices();
	sampler_list.clear_elements();
	sampler_list.add_element("none");
	for (int card = 0; card < MAX_SOUND_DEVICES && record_devices[card]; card++) {
		int type = record_devices[card]->type;
		TCHAR tmp[MAX_DPATH];
		_stprintf(tmp, _T("%s: %s"),
			type == SOUND_DEVICE_SDL2 ? _T("SDL2") : (type == SOUND_DEVICE_DS ? _T("DSOUND") : (type == SOUND_DEVICE_AL ? _T("OpenAL") : (type == SOUND_DEVICE_PA ? _T("PortAudio") : _T("WASAPI")))),
			record_devices[card]->name);
		if (type == SOUND_DEVICE_SDL2)
			sampler_list.add_element(tmp);
	}

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

	lblSampler = new gcn::Label("Sampler:");
	lblSampler->setAlignment(gcn::Graphics::RIGHT);
	cboSampler = new gcn::DropDown(&sampler_list);
	cboSampler->setSize(350, cboSampler->getHeight());
	cboSampler->setBaseColor(gui_baseCol);
	cboSampler->setBackgroundColor(colTextboxBackground);
	cboSampler->setId("cboSampler");
	cboSampler->addActionListener(ioActionListener);

	chkSamplerStereo = new gcn::CheckBox("Stereo sampler");
	chkSamplerStereo->setId("chkSamplerStereo");
	chkSamplerStereo->addActionListener(ioActionListener);

	lblProtectionDongle = new gcn::Label("Protection Dongle:");
	lblProtectionDongle->setAlignment(gcn::Graphics::RIGHT);
	cboProtectionDongle = new gcn::DropDown(&dongle_list);
	cboProtectionDongle->setSize(350, cboProtectionDongle->getHeight());
	cboProtectionDongle->setBaseColor(gui_baseCol);
	cboProtectionDongle->setBackgroundColor(colTextboxBackground);
	cboProtectionDongle->setId("cboProtectionDongle");
	cboProtectionDongle->addActionListener(ioActionListener);

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

	category.panel->add(lblSampler, DISTANCE_BORDER, posY);
	category.panel->add(cboSampler, DISTANCE_BORDER + lblProtectionDongle->getWidth() + 8, posY);
	posY += lblSampler->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkSamplerStereo, DISTANCE_BORDER, posY);
	posY += chkSamplerStereo->getHeight() + DISTANCE_NEXT_Y * 2;

	category.panel->add(lblProtectionDongle, DISTANCE_BORDER, posY);
	category.panel->add(cboProtectionDongle, DISTANCE_BORDER + lblProtectionDongle->getWidth() + 8, posY);

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

	delete lblProtectionDongle;
	delete cboProtectionDongle;
	delete lblSampler;
	delete cboSampler;
	delete chkSamplerStereo;
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

	if (changed_prefs.prtname[0])
	{
		changed_prefs.samplersoundcard = -1;
	}
	const int sampler_idx = changed_prefs.samplersoundcard;
	cboSampler->setSelected(sampler_idx + 1);
	chkSamplerStereo->setSelected(changed_prefs.sampler_stereo);
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