#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "options.h"
#include "gui_handling.h"
#include "sounddep/sound.h"
#include "parser.h"

static gcn::Window* grpParallelPort;
static gcn::Window* grpSerialPort;
static gcn::Window* grpMidi;
static gcn::Window* grpDongle;

static gcn::Label* lblSampler;
static gcn::DropDown* cboSampler;
static gcn::CheckBox* chkSamplerStereo;

static gcn::DropDown* cboSerialPort;
static gcn::CheckBox* chkSerialDirect;
static gcn::CheckBox* chkRTSCTS;
static gcn::CheckBox* chkUaeSerial;
static gcn::CheckBox* chkSerialStatus;
static gcn::CheckBox* chkSerialStatusRi;

static gcn::Label* lblMidiIn;
static gcn::DropDown* cboMidiIn;
static gcn::Label* lblMidiOut;
static gcn::DropDown* cboMidiOut;
static gcn::CheckBox* chkMidiRoute;

static gcn::DropDown* cboProtectionDongle;

class string_list_model : public gcn::ListModel
{
	std::vector<std::string> values{};
public:
	string_list_model(const char* entries[], const int count)
	{
		for (auto i = 0; i < count; ++i) {
			if (entries != nullptr && entries[i] != nullptr)
				values.emplace_back(entries[i]);
		}
	}

	int getNumberOfElements() override
	{
		return int(values.size());
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

static const char* listValues[] = {
	"none", "RoboCop 3", "Leader Board", "B.A.T. II", "Italy '90 Soccer",
	"Dames Grand-Maitre", "Rugby Coach", "Cricket Captain", "Leviathan",
	"Music Master", "Logistics/SuperBase", "Scala MM (Red)", "Scala MM (Green)",
	"Striker Manager", "Multi-player Soccer Manager", "Football Director 2"
};
static string_list_model dongle_list(listValues, 16);

static string_list_model sampler_list(nullptr, 0);
static string_list_model serial_ports_list(nullptr, 0);
static string_list_model midi_in_ports_list(nullptr, 0);
static string_list_model midi_out_ports_list(nullptr, 0);

class IOActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
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
				const auto port_name = serial_ports_list.getElementAt(selected);
				_sntprintf(changed_prefs.sername, 256, "%s", port_name.c_str());
				changed_prefs.use_serial = true;
			}
		}

		else if (actionEvent.getSource() == cboMidiOut)
		{
			const auto selected = cboMidiOut->getSelected();
			if (selected == 0)
			{
				changed_prefs.midioutdev[0] = 0;
				changed_prefs.midiindev[0] = 0;
			}
			else
			{
				const auto port_name = midi_out_ports_list.getElementAt(selected);
				_sntprintf(changed_prefs.midioutdev, 256, "%s", port_name.c_str());
			}
		}

		else if (actionEvent.getSource() == cboMidiIn)
		{
			const auto selected = cboMidiIn->getSelected();
			if (selected == 0)
			{
				changed_prefs.midiindev[0] = 0;
			}
			else
			{
				const auto port_name = midi_in_ports_list.getElementAt(selected);
				_sntprintf(changed_prefs.midiindev, 256, "%s", port_name.c_str());
			}
		}

		else if (actionEvent.getSource() == chkMidiRoute)
			changed_prefs.midirouter = chkMidiRoute->isSelected();

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

	serial_ports_list.clear_elements();
	serial_ports_list.add_element("none");
	for(const auto& i : serial_ports) {
		serial_ports_list.add_element(i.c_str());
	}
	// Add TCP ports also
	serial_ports_list.add_element("TCP://0.0.0.0:1234");
	serial_ports_list.add_element("TCP://0.0.0.0:1234/wait");

	midi_in_ports_list.clear_elements();
	midi_in_ports_list.add_element("none");
	for(const auto& i : midi_in_ports) {
		midi_in_ports_list.add_element(i.c_str());
	}

	midi_out_ports_list.clear_elements();
	midi_out_ports_list.add_element("none");
	for(const auto& i : midi_out_ports) {
		midi_out_ports_list.add_element(i.c_str());
	}

	ioActionListener = new IOActionListener();

	auto posY = DISTANCE_BORDER;

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

	grpParallelPort = new gcn::Window("Parallel Port");
	grpParallelPort->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	grpParallelPort->add(lblSampler, DISTANCE_BORDER, posY);
	grpParallelPort->add(cboSampler, DISTANCE_BORDER + lblSampler->getWidth() + 8, posY);
	posY += lblSampler->getHeight() + DISTANCE_NEXT_Y;
	grpParallelPort->add(chkSamplerStereo, DISTANCE_BORDER, posY);
	grpParallelPort->setMovable(false);
	grpParallelPort->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpParallelPort->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2, TITLEBAR_HEIGHT + chkSamplerStereo->getY() + chkSamplerStereo->getHeight() + DISTANCE_NEXT_Y * 3);
	grpParallelPort->setBaseColor(gui_baseCol);
	category.panel->add(grpParallelPort);

	cboSerialPort = new gcn::DropDown(&serial_ports_list);
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

	grpSerialPort = new gcn::Window("Serial Port");
	grpSerialPort->setPosition(DISTANCE_BORDER, grpParallelPort->getY() + grpParallelPort->getHeight() + DISTANCE_NEXT_Y);
	grpSerialPort->add(cboSerialPort, cboSampler->getX(), DISTANCE_BORDER);
	posY = cboSerialPort->getY() + cboSerialPort->getHeight() + DISTANCE_NEXT_Y;
	grpSerialPort->add(chkRTSCTS, DISTANCE_BORDER, posY);
	grpSerialPort->add(chkSerialDirect, chkRTSCTS->getWidth() + chkRTSCTS->getX() + DISTANCE_NEXT_X, posY);
	grpSerialPort->add(chkUaeSerial, chkSerialDirect->getWidth() + chkSerialDirect->getX() + DISTANCE_NEXT_X, posY);
	posY = chkRTSCTS->getY() + chkRTSCTS->getHeight() + DISTANCE_NEXT_Y;
	grpSerialPort->add(chkSerialStatus, DISTANCE_BORDER, posY);
	grpSerialPort->add(chkSerialStatusRi, chkSerialStatus->getWidth() + chkSerialStatus->getX() + DISTANCE_NEXT_X, posY);
	grpSerialPort->setMovable(false);
	grpSerialPort->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpSerialPort->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2, TITLEBAR_HEIGHT + chkSerialStatus->getY() + chkSerialStatus->getHeight() + DISTANCE_NEXT_Y * 4);
	grpSerialPort->setBaseColor(gui_baseCol);
	category.panel->add(grpSerialPort, DISTANCE_BORDER, grpParallelPort->getY() + grpParallelPort->getHeight() + DISTANCE_NEXT_Y);

	lblMidiOut = new gcn::Label("Out:");
	lblMidiOut->setAlignment(gcn::Graphics::RIGHT);
	cboMidiOut = new gcn::DropDown(&midi_out_ports_list);
	cboMidiOut->setSize(200, cboMidiOut->getHeight());
	cboMidiOut->setBaseColor(gui_baseCol);
	cboMidiOut->setBackgroundColor(colTextboxBackground);
	cboMidiOut->setId("cboMidiOut");
	cboMidiOut->addActionListener(ioActionListener);

	lblMidiIn = new gcn::Label("In:");
	lblMidiIn->setAlignment(gcn::Graphics::RIGHT);
	cboMidiIn = new gcn::DropDown(&midi_in_ports_list);
	cboMidiIn->setSize(200, cboMidiIn->getHeight());
	cboMidiIn->setBaseColor(gui_baseCol);
	cboMidiIn->setBackgroundColor(colTextboxBackground);
	cboMidiIn->setId("cboMidiIn");
	cboMidiIn->addActionListener(ioActionListener);

	chkMidiRoute = new gcn::CheckBox("Route MIDI In to MIDI Out");
	chkMidiRoute->setId("chkMidiRoute");
	chkMidiRoute->addActionListener(ioActionListener);

	grpMidi = new gcn::Window("MIDI");
	grpMidi->setPosition(DISTANCE_BORDER, grpSerialPort->getY() + grpSerialPort->getHeight() + DISTANCE_NEXT_Y);
	grpMidi->add(lblMidiOut, DISTANCE_BORDER, DISTANCE_BORDER);
	grpMidi->add(cboMidiOut, cboSampler->getX(), DISTANCE_BORDER);
	grpMidi->add(lblMidiIn, cboMidiOut->getX() + cboMidiOut->getWidth() + DISTANCE_NEXT_X * 3, DISTANCE_BORDER);
	grpMidi->add(cboMidiIn, lblMidiIn->getX() + lblMidiIn->getWidth() + 8, DISTANCE_BORDER);
	posY = cboSampler->getY() + cboSampler->getHeight() + DISTANCE_NEXT_Y;
	grpMidi->add(chkMidiRoute, DISTANCE_BORDER, posY);
	grpMidi->setMovable(false);
	grpMidi->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpMidi->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2, TITLEBAR_HEIGHT + chkMidiRoute->getY() + chkMidiRoute->getHeight() + DISTANCE_NEXT_Y * 4);
	grpMidi->setBaseColor(gui_baseCol);
	category.panel->add(grpMidi, DISTANCE_BORDER, grpSerialPort->getY() + grpSerialPort->getHeight() + DISTANCE_NEXT_Y);

	cboProtectionDongle = new gcn::DropDown(&dongle_list);
	cboProtectionDongle->setSize(350, cboProtectionDongle->getHeight());
	cboProtectionDongle->setBaseColor(gui_baseCol);
	cboProtectionDongle->setBackgroundColor(colTextboxBackground);
	cboProtectionDongle->setId("cboProtectionDongle");
	cboProtectionDongle->addActionListener(ioActionListener);

	grpDongle = new gcn::Window("Protection Dongle");
	grpDongle->setPosition(DISTANCE_BORDER, grpMidi->getY() + grpMidi->getHeight() + DISTANCE_NEXT_Y);
	grpDongle->add(cboProtectionDongle, cboSampler->getX(), DISTANCE_BORDER);
	grpDongle->setMovable(false);
	grpDongle->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpDongle->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2, category.panel->getHeight() - grpParallelPort->getHeight() - grpSerialPort->getHeight() - grpMidi->getHeight() - TITLEBAR_HEIGHT * 3);
	grpDongle->setBaseColor(gui_baseCol);
	category.panel->add(grpDongle, DISTANCE_BORDER, grpMidi->getY() + grpMidi->getHeight() + DISTANCE_NEXT_Y);

	RefreshPanelIO();
}

void ExitPanelIO()
{
	delete ioActionListener;

	delete lblSampler;
	delete cboSampler;
	delete chkSamplerStereo;

	delete cboSerialPort;
	delete chkRTSCTS;
	delete chkSerialDirect;
	delete chkUaeSerial;
	delete chkSerialStatus;
	delete chkSerialStatusRi;

	delete lblMidiIn;
	delete cboMidiIn;
	delete lblMidiOut;
	delete cboMidiOut;
	delete chkMidiRoute;

	delete cboProtectionDongle;

	delete grpParallelPort;
	delete grpSerialPort;
	delete grpMidi;
	delete grpDongle;
}

void RefreshPanelIO()
{
	chkRTSCTS->setSelected(changed_prefs.serial_hwctsrts);
	chkSerialDirect->setSelected(changed_prefs.serial_direct);
	chkUaeSerial->setSelected(changed_prefs.uaeserial);
	chkSerialStatus->setSelected(changed_prefs.serial_rtsctsdtrdtecd);
	chkSerialStatusRi->setSelected(changed_prefs.serial_ri);
	cboSerialPort->setSelected(0);
	if (changed_prefs.sername[0])
	{
		const auto serial_name = string(changed_prefs.sername);
		for (int i = 0; i < serial_ports_list.getNumberOfElements(); i++)
		{
			if (serial_ports_list.getElementAt(i) == serial_name)
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

	chkMidiRoute->setEnabled(false);
	if (changed_prefs.midioutdev[0])
	{
		const auto midi_out_name = string(changed_prefs.midioutdev);
		for (int i = 0; i < midi_out_ports_list.getNumberOfElements(); i++)
		{
			if (midi_out_ports_list.getElementAt(i) == midi_out_name)
			{
				cboMidiOut->setSelected(i);
				break;
			}
		}
		cboMidiIn->setEnabled(true);
		if (changed_prefs.midiindev[0])
		{
			const auto midi_in_name = string(changed_prefs.midiindev);
			for (int i = 0; i < midi_in_ports_list.getNumberOfElements(); i++)
			{
				if (midi_in_ports_list.getElementAt(i) == midi_in_name)
				{
					cboMidiIn->setSelected(i);
					chkMidiRoute->setEnabled(true);
					break;
				}
			}
		}
	}
	else
	{
		cboMidiIn->setEnabled(false);
	}

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
	helptext.emplace_back("In this panel, you can configure the various I/O Ports that Amiberry can emulate.");
	helptext.emplace_back("Some of these options will depend on what hardware you have available in your system");
	helptext.emplace_back("(e.g. serial ports).");
	helptext.emplace_back(" ");
	helptext.emplace_back("Sampler: This dropdown will be populated with any Recording devices detected in your");
	helptext.emplace_back("         system. You can select such a device, to use it as a Sampler in the emulated");
	helptext.emplace_back("         Amiga.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Serial Port: If you have any physical serial ports connected to your computer,");
	helptext.emplace_back("         they will be shown in this dropdown. You can select one, to use as the");
	helptext.emplace_back("         emulated Amiga serial port. You can also use USB to Serial adapters for ");
	helptext.emplace_back("         this purpose. Only standard 7/8-bit serial protocols are supported, as the");
	helptext.emplace_back("         9-bit serial protocol is not supported by PC hardware.");
	helptext.emplace_back(" ");
	helptext.emplace_back("RTS/CTS: If you require handshake signals, you can enable this checkbox. This is usually");
	helptext.emplace_back("         required by certain hardware, like modems or other physical devices.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Direct: If you are connecting directly to another instance of Amiberry, you can use");
	helptext.emplace_back("        this option, to emulate a null-modem cable connection. Some games supported");
	helptext.emplace_back("        this feature.");
	helptext.emplace_back(" ");
	helptext.emplace_back("uaeserial.device: You can use this option if you want to use multiple serial ports,");
	helptext.emplace_back("         by mapping Amiga side unit X = host serial port X");
	helptext.emplace_back(" ");
	helptext.emplace_back("MIDI Out/In: If you have any MIDI devices connected, you can select them here.");
	helptext.emplace_back("         Amiberry uses the PortMidi library to send and receive MIDI messages.");
	helptext.emplace_back("         Amiberry also offers the option to emulate a Roland MT-32 MIDI device,");
	helptext.emplace_back("         if you have the required ROMs installed. The dropdown will indicate if");
	helptext.emplace_back("         the required ROM files were detected or not. Please note that the path");
	helptext.emplace_back("         Amiberry will look for, is hardcoded to be \"<kickstarts dir>/mt32-roms\".");
	helptext.emplace_back("         If the ROMs were detected, you can set the MIDI Out to an MT-32 option and");
	helptext.emplace_back("         use any software that makes use of it (e.g. several Sierra games).");
	helptext.emplace_back("         The audio output is mixed with Paula's output.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Route MIDI In to MIDI Out: This option will reroute the MIDI In to the MIDI Out port,");
	helptext.emplace_back("         as the name implies.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Protection Dongle: This option allows you to emulate such a dongle, which some");
	helptext.emplace_back("         software required in order to work.");
	helptext.emplace_back(" ");
	return true;
}
