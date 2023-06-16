 #include <cstring>

#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "options.h"
#include "gui_handling.h"
#include "sounddep/sound.h"

#include <libserialport.h>

#ifdef SERIAL_PORT
static gcn::Label* lblSerialPort;
static gcn::DropDown* cboSerialPort;
static gcn::CheckBox* chkSerialDirect;
static gcn::CheckBox* chkRTSCTS;
static gcn::CheckBox* chkUaeSerial;
static gcn::CheckBox* chkSerialStatus;
static gcn::CheckBox* chkSerialStatusRi;
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
static string_list_model serial_ports(nullptr, 0);

class IOActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
#ifdef SERIAL_PORT
		if (actionEvent.getSource() == cboSerialPort)
		{
			const auto selected = cboSerialPort->getSelected();
			if (selected == 0)
			{
				changed_prefs.sername[0] = 0;
				changed_prefs.use_serial = false;
			}
			else
			{
				const auto port_name = serial_ports.getElementAt(selected);
				snprintf(changed_prefs.sername, 256, "%s", port_name.c_str());
				changed_prefs.use_serial = true;
			}
		}

		else if (actionEvent.getSource() == chkSerialDirect)
			changed_prefs.serial_direct = chkSerialDirect->isSelected();

		else if (actionEvent.getSource() == chkRTSCTS)
			changed_prefs.serial_hwctsrts = chkRTSCTS->isSelected();

		else if (actionEvent.getSource() == chkUaeSerial)
			changed_prefs.uaeserial = chkUaeSerial->isSelected();

		else if (actionEvent.getSource() == chkSerialStatus)
			changed_prefs.serial_rtsctsdtrdtecd = chkSerialStatus->isSelected();

		else if (actionEvent.getSource() == chkSerialStatusRi)
			changed_prefs.serial_ri = chkSerialStatusRi->isSelected();

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

	serial_ports.clear_elements();
	serial_ports.add_element("none");
	/* A pointer to a null-terminated array of pointers to
	* struct sp_port, which will contain the serial ports found.*/
	struct sp_port** port_list;
	/* Call sp_list_ports() to get the ports. The port_list
	* pointer will be updated to refer to the array created. */
	const enum sp_return result = sp_list_ports(&port_list);
	if (result != SP_OK) 
	{
		write_log("sp_list_ports() failed!\n");
	}
	else
	{
		for (int i = 0; port_list[i] != NULL; i++)
		{
			const struct sp_port* port = port_list[i];

			/* Get the name of the port. */
			const char* port_name = sp_get_port_name(port);
			serial_ports.add_element(port_name);
		}
		/* Free the array created by sp_list_ports(). */
		sp_free_port_list(port_list);
	}

	ioActionListener = new IOActionListener();

#ifdef SERIAL_PORT
	lblSerialPort = new gcn::Label("Serial port:");
	lblSerialPort->setAlignment(gcn::Graphics::RIGHT);
	cboSerialPort = new gcn::DropDown(&serial_ports);
	cboSerialPort->setSize(350, cboSerialPort->getHeight());
	cboSerialPort->setBaseColor(gui_baseCol);
	cboSerialPort->setBackgroundColor(colTextboxBackground);
	cboSerialPort->setId("cboSerialPort");
	cboSerialPort->addActionListener(ioActionListener);

	chkSerialDirect = new gcn::CheckBox("Direct");
	chkSerialDirect->setId("chkSerialDirect");
	chkSerialDirect->addActionListener(ioActionListener);

	chkRTSCTS = new gcn::CheckBox("Host RTS/CTS");
	chkRTSCTS->setId("chkRTSCTS");
	chkRTSCTS->addActionListener(ioActionListener);

	chkUaeSerial = new gcn::CheckBox("uaeserial.device");
	chkUaeSerial->setId("chkUaeSerial");
	chkUaeSerial->addActionListener(ioActionListener);

	chkSerialStatus = new gcn::CheckBox("Serial status (RTS/CTS/DTR/DTE/CD)");
	chkSerialStatus->setId("chkSerialStatus");
	chkSerialStatus->addActionListener(ioActionListener);

	chkSerialStatusRi = new gcn::CheckBox("Serial status: Ring Indicator");
	chkSerialStatusRi->setId("chkSerialStatusRi");
	chkSerialStatusRi->addActionListener(ioActionListener);
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

	category.panel->add(lblSerialPort, DISTANCE_BORDER, DISTANCE_BORDER);
	category.panel->add(cboSerialPort, DISTANCE_BORDER + lblProtectionDongle->getWidth() + 8, DISTANCE_BORDER);
	int posY = cboSerialPort->getY() + cboSerialPort->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkRTSCTS, DISTANCE_BORDER, posY);
	category.panel->add(chkSerialDirect, chkRTSCTS->getWidth() + chkRTSCTS->getX() + DISTANCE_NEXT_X, posY);
	category.panel->add(chkUaeSerial, chkSerialDirect->getWidth() + chkSerialDirect->getX() + DISTANCE_NEXT_X, posY);
	posY = chkRTSCTS->getY() + chkRTSCTS->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkSerialStatus, DISTANCE_BORDER, posY);
	category.panel->add(chkSerialStatusRi, chkSerialStatus->getWidth() + chkSerialStatus->getX() + DISTANCE_NEXT_X, posY);
	posY = chkSerialStatus->getY() + chkSerialStatus->getHeight() + DISTANCE_NEXT_Y * 2;

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
	delete lblSerialPort;
	delete cboSerialPort;
	delete chkRTSCTS;
	delete chkSerialDirect;
	delete chkUaeSerial;
	delete chkSerialStatus;
	delete chkSerialStatusRi;
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
	chkUaeSerial->setSelected(changed_prefs.uaeserial);
	chkSerialStatus->setSelected(changed_prefs.serial_rtsctsdtrdtecd);
	chkSerialStatusRi->setSelected(changed_prefs.serial_ri);
	cboSerialPort->setSelected(0);
	if (changed_prefs.sername[0])
	{
		const auto serial_name = string(changed_prefs.sername);
		for (int i = 0; i < serial_ports.getNumberOfElements(); i++)
		{
			if (serial_ports.getElementAt(i) == serial_name)
			{
				cboSerialPort->setSelected(i);
				break;
			}
		}
		chkRTSCTS->setEnabled(true);
		chkSerialDirect->setEnabled(true);
		chkUaeSerial->setEnabled(true);
		chkSerialStatus->setEnabled(true);
		chkSerialStatusRi->setEnabled(true);
	}
	else
	{
		chkRTSCTS->setEnabled(false);
		chkSerialDirect->setEnabled(false);
		chkUaeSerial->setEnabled(false);
		chkSerialStatus->setEnabled(false);
		chkSerialStatusRi->setEnabled(false);
	}
#endif

	cboProtectionDongle->setSelected(changed_prefs.dongle);

	if (changed_prefs.prtname[0])
	{
		changed_prefs.samplersoundcard = -1;
	}
	const int sampler_idx = changed_prefs.samplersoundcard;
	cboSampler->setSelected(sampler_idx + 1);
	chkSamplerStereo->setSelected(changed_prefs.sampler_stereo);
	chkSamplerStereo->setEnabled(sampler_idx >= 0);
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
