#include <stdio.h>

#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "options.h"
#include "gui_handling.h"
#include "audio.h"
#include "sounddep/sound.h"

static gcn::Window* grpSound;
static gcn::RadioButton* optSoundDisabled;
static gcn::RadioButton* optSoundDisabledEmu;
static gcn::RadioButton* optSoundEmulated;
static gcn::RadioButton* optSoundEmulatedBest;
static gcn::Container* grpSettings;
static gcn::Label* lblChannelMode;
static gcn::DropDown* cboChannelMode;
static gcn::Label* lblFrequency;
static gcn::DropDown* cboFrequency;
static gcn::Label* lblInterpolation;
static gcn::DropDown* cboInterpolation;
static gcn::Label* lblFilter;
static gcn::DropDown* cboFilter;
static gcn::Label* lblSeparation;
static gcn::DropDown* cboSeparation;
static gcn::Label* lblStereoDelay;
static gcn::DropDown* cboStereoDelay;
static gcn::Window* grpVolume;
static gcn::Label* lblMasterVol;
static gcn::Label* lblMasterVolInfo;
static gcn::Slider* sldMasterVol;
static gcn::Label* lblPaulaVol;
static gcn::Label* lblPaulaVolInfo;
static gcn::Slider* sldPaulaVol;
static gcn::Label* lblCDVol;
static gcn::Label* lblCDVolInfo;
static gcn::Slider* sldCDVol;
static gcn::Label* lblAHIVol;
static gcn::Label* lblAHIVolInfo;
static gcn::Slider* sldAHIVol;
static gcn::Window* grpFloppySound;
static gcn::CheckBox* chkFloppySound;
static gcn::Label* lblFloppySoundEmpty;
static gcn::Label* lblFloppySoundEmptyInfo;
static gcn::Slider* sldFloppySoundEmpty;
static gcn::Label* lblFloppySoundDisk;
static gcn::Label* lblFloppySoundDiskInfo;
static gcn::Slider* sldFloppySoundDisk;
static gcn::Window* grpSoundBufferSize;
static gcn::Slider* sldSoundBufferSize;
static gcn::Label* lblSoundBufferSize;
static gcn::RadioButton* optSoundPull;
static gcn::RadioButton* optSoundPush;

static int curr_separation_idx;
static int curr_stereodelay_idx;

static const int sndbufsizes[] = { 1024, 2048, 3072, 4096, 6144, 8192, 12288, 16384, 32768, 65536, -1 };

class ChannelModeListModel : public gcn::ListModel
{
	std::vector<std::string> mode;

public:
	ChannelModeListModel()
	{
		mode.emplace_back("Mono");
		mode.emplace_back("Stereo");
	}

	int getNumberOfElements() override
	{
		return mode.size();
	}

	std::string getElementAt(int i) override
	{
		if (i < 0 || i >= static_cast<int>(mode.size()))
			return "---";
		return mode[i];
	}
};

static ChannelModeListModel channel_mode_list;

class SeparationListModel : public gcn::ListModel
{
	std::vector<std::string> mode;

public:
	SeparationListModel()
	{
		mode.emplace_back("100%");
		mode.emplace_back("90%");
		mode.emplace_back("80%");
		mode.emplace_back("70%");
		mode.emplace_back("60%");
		mode.emplace_back("50%");
		mode.emplace_back("40%");
		mode.emplace_back("30%");
		mode.emplace_back("20%");
		mode.emplace_back("10%");
		mode.emplace_back("0%");
	}

	int getNumberOfElements() override
	{
		return mode.size();
	}

	std::string getElementAt(int i) override
	{
		if (i < 0 || i >= static_cast<int>(mode.size()))
			return "---";
		return mode[i];
	}
};

static SeparationListModel separation_list;

class StereoDelayListModel : public gcn::ListModel
{
	std::vector<std::string> mode;

public:
	StereoDelayListModel()
	{
		mode.emplace_back("-");
		mode.emplace_back("1");
		mode.emplace_back("2");
		mode.emplace_back("3");
		mode.emplace_back("4");
		mode.emplace_back("5");
		mode.emplace_back("6");
		mode.emplace_back("7");
		mode.emplace_back("8");
		mode.emplace_back("9");
		mode.emplace_back("10");
	}

	int getNumberOfElements() override
	{
		return mode.size();
	}

	std::string getElementAt(int i) override
	{
		if (i < 0 || i >= static_cast<int>(mode.size()))
			return "---";
		return mode[i];
	}
};

static StereoDelayListModel stereo_delay_list;

class FrequencyListModel : public gcn::ListModel
{
	std::vector<std::string> freq;

public:
	FrequencyListModel()
	{
		freq.emplace_back("11025");
		freq.emplace_back("22050");
		freq.emplace_back("32000");
		freq.emplace_back("44100");
		freq.emplace_back("48000");
	}

	int getNumberOfElements() override
	{
		return freq.size();
	}

	std::string getElementAt(const int i) override
	{
		if (i < 0 || i >= static_cast<int>(freq.size()))
			return "---";
		return freq[i];
	}
};

static FrequencyListModel frequency_type_list;

class InterpolationListModel : public gcn::ListModel
{
	std::vector<std::string> entry;

public:
	InterpolationListModel()
	{
		entry.emplace_back("Disabled");
		entry.emplace_back("Anti");
		entry.emplace_back("Sinc");
		entry.emplace_back("RH");
		entry.emplace_back("Crux");
	}

	int getNumberOfElements() override
	{
		return entry.size();
	}

	std::string getElementAt(const int i) override
	{
		if (i < 0 || i >= static_cast<int>(entry.size()))
			return "---";
		return entry[i];
	}
};

static InterpolationListModel interpolation_type_list;

class FilterListModel : public gcn::ListModel
{
	std::vector<std::string> entry;

public:
	FilterListModel()
	{
		entry.emplace_back("Always off");
		entry.emplace_back("Emulated (A500)");
		entry.emplace_back("Emulated (A1200)");
		entry.emplace_back("Always on (A500)");
		entry.emplace_back("Always on (A1200)");
	}

	int getNumberOfElements() override
	{
		return entry.size();
	}

	std::string getElementAt(const int i) override
	{
		if (i < 0 || i >= static_cast<int>(entry.size()))
			return "---";
		return entry[i];
	}
};

static FilterListModel filter_type_list;

class SoundActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == optSoundDisabled)
			changed_prefs.produce_sound = 0;
		else if (actionEvent.getSource() == optSoundDisabledEmu)
			changed_prefs.produce_sound = 1;
		else if (actionEvent.getSource() == optSoundEmulated)
			changed_prefs.produce_sound = 2;
		else if (actionEvent.getSource() == optSoundEmulatedBest)
			changed_prefs.produce_sound = 3;

		else if (actionEvent.getSource() == cboChannelMode)
			changed_prefs.sound_stereo = cboChannelMode->getSelected();

		else if (actionEvent.getSource() == cboFrequency)
		{
			switch (cboFrequency->getSelected())
			{
			case 0:
				changed_prefs.sound_freq = 11025;
				break;
			case 1:
				changed_prefs.sound_freq = 22050;
				break;
			case 2:
				changed_prefs.sound_freq = 32000;
				break;
			case 3:
				changed_prefs.sound_freq = 44100;
				break;
			case 4:
				changed_prefs.sound_freq = 48000;
				break;
			default:
				break;
			}
		}

		else if (actionEvent.getSource() == cboInterpolation)
			changed_prefs.sound_interpol = cboInterpolation->getSelected();

		else if (actionEvent.getSource() == cboFilter)
		{
			switch (cboFilter->getSelected())
			{
			case 0:
				changed_prefs.sound_filter = FILTER_SOUND_OFF;
				break;
			case 1:
				changed_prefs.sound_filter = FILTER_SOUND_EMUL;
				changed_prefs.sound_filter_type = 0;
				break;
			case 2:
				changed_prefs.sound_filter = FILTER_SOUND_EMUL;
				changed_prefs.sound_filter_type = 1;
				break;
			case 3:
				changed_prefs.sound_filter = FILTER_SOUND_ON;
				changed_prefs.sound_filter_type = 0;
				break;
			case 4:
				changed_prefs.sound_filter = FILTER_SOUND_ON;
				changed_prefs.sound_filter_type = 1;
				break;
			default:
				break;
			}
		}

		else if (actionEvent.getSource() == cboSeparation)
		{
			if (curr_separation_idx != static_cast<int>(cboSeparation->getSelected())
				&& changed_prefs.sound_stereo > 0)
			{
				curr_separation_idx = static_cast<int>(cboSeparation->getSelected());
				changed_prefs.sound_stereo_separation = 10 - curr_separation_idx;
			}
		}

		else if (actionEvent.getSource() == cboStereoDelay)
		{
			if (curr_stereodelay_idx != static_cast<int>(cboStereoDelay->getSelected())
				&& changed_prefs.sound_stereo > 0)
			{
				curr_stereodelay_idx = static_cast<int>(cboStereoDelay->getSelected());
				if (curr_stereodelay_idx > 0)
					changed_prefs.sound_mixed_stereo_delay = curr_stereodelay_idx;
				else
					changed_prefs.sound_mixed_stereo_delay = -1;
			}
		}
		else if (actionEvent.getSource() == sldMasterVol)
		{
			const auto newvol = 100 - static_cast<int>(sldMasterVol->getValue());
			if (changed_prefs.sound_volume_master != newvol)
				changed_prefs.sound_volume_master = newvol;
		}	
		else if (actionEvent.getSource() == sldPaulaVol)
		{
			const auto newvol = 100 - static_cast<int>(sldPaulaVol->getValue());
			if (changed_prefs.sound_volume_paula != newvol)
				changed_prefs.sound_volume_paula = newvol;
		}
		else if (actionEvent.getSource() == sldCDVol)
		{
			const auto newvol = 100 - static_cast<int>(sldCDVol->getValue());
			if (changed_prefs.sound_volume_cd != newvol)
				changed_prefs.sound_volume_cd = newvol;
		}
		else if (actionEvent.getSource() == sldAHIVol)
		{
		const auto newvol = 100 - static_cast<int>(sldAHIVol->getValue());
		if (changed_prefs.sound_volume_board != newvol)
			changed_prefs.sound_volume_board = newvol;
		}
		else if (actionEvent.getSource() == chkFloppySound)
		{
			for (auto& floppyslot : changed_prefs.floppyslots)
			{
				floppyslot.dfxclick = chkFloppySound->isSelected() ? 1 : 0;
			}
		}
		else if (actionEvent.getSource() == sldFloppySoundEmpty)
		{
			const auto newvol = 100 - static_cast<int>(sldFloppySoundEmpty->getValue());
			if (changed_prefs.dfxclickvolume_empty[0] != newvol)
			{
				for (auto& i : changed_prefs.dfxclickvolume_empty)
					i = newvol;
			}
		}
		else if (actionEvent.getSource() == sldFloppySoundDisk)
		{
			const auto newvol = 100 - static_cast<int>(sldFloppySoundDisk->getValue());
			if (changed_prefs.dfxclickvolume_disk[0] != newvol)
			{
				for (auto& i : changed_prefs.dfxclickvolume_disk)
					i = newvol;
			}
		}

		else if (actionEvent.getSource() == sldSoundBufferSize)
		{
			int v = static_cast<int>(sldSoundBufferSize->getValue());
			if (v >= 0)
			{
				if (v == 0)
					changed_prefs.sound_maxbsiz = 0;
				else
					changed_prefs.sound_maxbsiz = sndbufsizes[v - 1];
			}
		}

		else if (actionEvent.getSource() == optSoundPull)
			changed_prefs.sound_pullmode = 1;
		else if (actionEvent.getSource() == optSoundPush)
			changed_prefs.sound_pullmode = 0;
		
		RefreshPanelSound();
	}
};

static SoundActionListener* sound_action_listener;

void InitPanelSound(const struct _ConfigCategory& category)
{
	sound_action_listener = new SoundActionListener();

	optSoundDisabled = new gcn::RadioButton("Disabled", "radiosoundgroup");
	optSoundDisabled->setId("sndDisable");
	optSoundDisabled->addActionListener(sound_action_listener);

	optSoundDisabledEmu = new gcn::RadioButton("Disabled, but emulated", "radiosoundgroup");
	optSoundDisabledEmu->setId("sndDisEmu");
	optSoundDisabledEmu->addActionListener(sound_action_listener);

	optSoundEmulated = new gcn::RadioButton("Enabled", "radiosoundgroup");
	optSoundEmulated->setId("sndEmulate");
	optSoundEmulated->addActionListener(sound_action_listener);

	optSoundEmulatedBest = new gcn::RadioButton("Enabled, most accurate", "radiosoundgroup");
	optSoundEmulatedBest->setId("sndEmuBest");
	optSoundEmulatedBest->addActionListener(sound_action_listener);

	lblFrequency = new gcn::Label("Frequency:");
	lblFrequency->setAlignment(gcn::Graphics::RIGHT);
	cboFrequency = new gcn::DropDown(&frequency_type_list);
	cboFrequency->setSize(100, cboFrequency->getHeight());
	cboFrequency->setBaseColor(gui_baseCol);
	cboFrequency->setBackgroundColor(colTextboxBackground);
	cboFrequency->setId("cboFrequency");
	cboFrequency->addActionListener(sound_action_listener);

	lblChannelMode = new gcn::Label("Channel mode:");
	lblChannelMode->setAlignment(gcn::Graphics::RIGHT);
	cboChannelMode = new gcn::DropDown(&channel_mode_list);
	cboChannelMode->setSize(100, cboChannelMode->getHeight());
	cboChannelMode->setBaseColor(gui_baseCol);
	cboChannelMode->setBackgroundColor(colTextboxBackground);
	cboChannelMode->setId("cboChannelMode");
	cboChannelMode->addActionListener(sound_action_listener);
	
	lblInterpolation = new gcn::Label("Interpolation:");
	lblInterpolation->setAlignment(gcn::Graphics::RIGHT);
	cboInterpolation = new gcn::DropDown(&interpolation_type_list);
	cboInterpolation->setSize(150, cboInterpolation->getHeight());
	cboInterpolation->setBaseColor(gui_baseCol);
	cboInterpolation->setBackgroundColor(colTextboxBackground);
	cboInterpolation->setId("cboInterpol");
	cboInterpolation->addActionListener(sound_action_listener);

	lblFilter = new gcn::Label("Filter:");
	lblFilter->setAlignment(gcn::Graphics::RIGHT);
	cboFilter = new gcn::DropDown(&filter_type_list);
	cboFilter->setSize(150, cboFilter->getHeight());
	cboFilter->setBaseColor(gui_baseCol);
	cboFilter->setBackgroundColor(colTextboxBackground);
	cboFilter->setId("cboFilter");
	cboFilter->addActionListener(sound_action_listener);

	lblSeparation = new gcn::Label("Stereo separation:");
	lblSeparation->setAlignment(gcn::Graphics::RIGHT);
	cboSeparation = new gcn::DropDown(&separation_list);
	cboSeparation->setSize(150, cboSeparation->getHeight());
	cboSeparation->setBaseColor(gui_baseCol);
	cboSeparation->setBackgroundColor(colTextboxBackground);
	cboSeparation->setId("cboSeparation");
	cboSeparation->addActionListener(sound_action_listener);
	
	lblStereoDelay = new gcn::Label("Stereo delay:");
	lblStereoDelay->setAlignment(gcn::Graphics::RIGHT);
	cboStereoDelay = new gcn::DropDown(&stereo_delay_list);
	cboStereoDelay->setSize(150, cboStereoDelay->getHeight());
	cboStereoDelay->setBaseColor(gui_baseCol);
	cboStereoDelay->setBackgroundColor(colTextboxBackground);
	cboStereoDelay->setId("cboStereoDelay");
	cboStereoDelay->addActionListener(sound_action_listener);

	lblMasterVol = new gcn::Label("Master Volume:");
	lblMasterVol->setAlignment(gcn::Graphics::RIGHT);
	sldMasterVol = new gcn::Slider(0, 100);
	sldMasterVol->setSize(150, SLIDER_HEIGHT);
	sldMasterVol->setBaseColor(gui_baseCol);
	sldMasterVol->setMarkerLength(20);
	sldMasterVol->setStepLength(10);
	sldMasterVol->setId("sldMasterVol");
	sldMasterVol->addActionListener(sound_action_listener);
	lblMasterVolInfo = new gcn::Label("100 %");
	
	lblPaulaVol = new gcn::Label("Paula Volume:");
	lblPaulaVol->setAlignment(gcn::Graphics::RIGHT);
	sldPaulaVol = new gcn::Slider(0, 100);
	sldPaulaVol->setSize(150, SLIDER_HEIGHT);
	sldPaulaVol->setBaseColor(gui_baseCol);
	sldPaulaVol->setMarkerLength(20);
	sldPaulaVol->setStepLength(10);
	sldPaulaVol->setId("sldPaulaVol");
	sldPaulaVol->addActionListener(sound_action_listener);
	lblPaulaVolInfo = new gcn::Label("100 %");

	lblCDVol = new gcn::Label("CD Volume:");
	lblCDVol->setAlignment(gcn::Graphics::RIGHT);
	sldCDVol = new gcn::Slider(0, 100);
	sldCDVol->setSize(150, SLIDER_HEIGHT);
	sldCDVol->setBaseColor(gui_baseCol);
	sldCDVol->setMarkerLength(20);
	sldCDVol->setStepLength(10);
	sldCDVol->setId("sldCDVol");
	sldCDVol->addActionListener(sound_action_listener);
	lblCDVolInfo = new gcn::Label("80 %");

	lblAHIVol = new gcn::Label("AHI Volume:");
	lblAHIVol->setAlignment(gcn::Graphics::RIGHT);
	sldAHIVol = new gcn::Slider(0, 100);
	sldAHIVol->setSize(150, SLIDER_HEIGHT);
	sldAHIVol->setBaseColor(gui_baseCol);
	sldAHIVol->setMarkerLength(20);
	sldAHIVol->setStepLength(10);
	sldAHIVol->setId("sldAHIVol");
	sldAHIVol->addActionListener(sound_action_listener);
	lblAHIVolInfo = new gcn::Label("100 %");
	
	chkFloppySound = new gcn::CheckBox("Enable floppy drive sound");
	chkFloppySound->setId("chkFloppySound");
	chkFloppySound->addActionListener(sound_action_listener);
	
	sldFloppySoundEmpty = new gcn::Slider(0, 100);
	sldFloppySoundEmpty->setSize(100, SLIDER_HEIGHT);
	sldFloppySoundEmpty->setBaseColor(gui_baseCol);
	sldFloppySoundEmpty->setMarkerLength(20);
	sldFloppySoundEmpty->setStepLength(10);
	sldFloppySoundEmpty->setId("sldFloppySoundEmpty");
	sldFloppySoundEmpty->addActionListener(sound_action_listener);
	lblFloppySoundEmptyInfo = new gcn::Label("67 %");
	lblFloppySoundEmpty = new gcn::Label("Empty drive:");

	sldFloppySoundDisk = new gcn::Slider(0, 100);
	sldFloppySoundDisk->setSize(100, SLIDER_HEIGHT);
	sldFloppySoundDisk->setBaseColor(gui_baseCol);
	sldFloppySoundDisk->setMarkerLength(20);
	sldFloppySoundDisk->setStepLength(10);
	sldFloppySoundDisk->setId("sldFloppySoundDisk");
	sldFloppySoundDisk->addActionListener(sound_action_listener);
	lblFloppySoundDiskInfo = new gcn::Label("67 %");
	lblFloppySoundDisk = new gcn::Label("Disk in drive:");

	sldSoundBufferSize = new gcn::Slider(0, 10);
	sldSoundBufferSize->setSize(170, SLIDER_HEIGHT);
	sldSoundBufferSize->setBaseColor(gui_baseCol);
	sldSoundBufferSize->setMarkerLength(20);
	sldSoundBufferSize->setStepLength(10);
	sldSoundBufferSize->setId("sldSoundBufferSize");
	sldSoundBufferSize->addActionListener(sound_action_listener);
	lblSoundBufferSize = new gcn::Label("Min");

	optSoundPull = new gcn::RadioButton("Pull audio", "radioaudiomethod");
	optSoundPull->setId("optSoundPull");
	optSoundPull->addActionListener(sound_action_listener);

	optSoundPush = new gcn::RadioButton("Push audio", "radioaudiomethod");
	optSoundPush->setId("optSoundPush");
	optSoundPush->addActionListener(sound_action_listener);
	
	grpSound = new gcn::Window("Sound Emulation");
	grpSound->add(optSoundDisabled, 10, 10);
	grpSound->add(optSoundDisabledEmu, optSoundDisabled->getX(), 40);
	grpSound->add(optSoundEmulated, optSoundDisabled->getX(), 70);
	grpSound->add(optSoundEmulatedBest, optSoundDisabled->getX(), 100);
	grpSound->setMovable(false);
	grpSound->setSize(optSoundEmulatedBest->getWidth() + DISTANCE_BORDER + 10, optSoundEmulatedBest->getY() + optSoundEmulatedBest->getHeight() + 10 + DISTANCE_NEXT_Y * 2);
	grpSound->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpSound->setBaseColor(gui_baseCol);

	grpVolume = new gcn::Window("Volume");
	grpVolume->add(lblMasterVol, 10, 10);
	grpVolume->add(sldMasterVol, lblMasterVol->getX() + lblMasterVol->getWidth() + 8, lblMasterVol->getY());
	grpVolume->add(lblMasterVolInfo, sldMasterVol->getX() + sldMasterVol->getWidth() + 8, sldMasterVol->getY());
	grpVolume->add(lblPaulaVol, lblMasterVol->getX(), lblMasterVol->getY() + lblMasterVol->getHeight() + DISTANCE_NEXT_Y);
	grpVolume->add(sldPaulaVol, sldMasterVol->getX(), lblPaulaVol->getY());
	grpVolume->add(lblPaulaVolInfo, sldPaulaVol->getX() + sldPaulaVol->getWidth() + 10, sldPaulaVol->getY());
	grpVolume->add(lblCDVol, lblMasterVol->getX(), lblPaulaVol->getY() + lblPaulaVol->getHeight() + DISTANCE_NEXT_Y);
	grpVolume->add(sldCDVol, sldMasterVol->getX(), lblCDVol->getY());
	grpVolume->add(lblCDVolInfo, sldCDVol->getX() + sldCDVol->getWidth() + 8, sldCDVol->getY());
	grpVolume->add(lblAHIVol, lblMasterVol->getX(), lblCDVol->getY() + lblCDVol->getHeight() + DISTANCE_NEXT_Y);
	grpVolume->add(sldAHIVol, sldMasterVol->getX(), lblAHIVol->getY());
	grpVolume->add(lblAHIVolInfo, sldAHIVol->getX() + sldAHIVol->getWidth() + 8, sldAHIVol->getY());
	grpVolume->setMovable(false);
	grpVolume->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2 - grpSound->getWidth() - DISTANCE_NEXT_X, grpSound->getHeight());
	grpVolume->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpVolume->setBaseColor(gui_baseCol);
	
	grpSettings = new gcn::Container();
	grpSettings->add(lblChannelMode, 10, 10);
	grpSettings->add(cboChannelMode, lblChannelMode->getX(), lblChannelMode->getY() + lblChannelMode->getHeight() + 10);
	grpSettings->add(lblFrequency, lblChannelMode->getX(), 70);
	grpSettings->add(cboFrequency, lblChannelMode->getX(), lblFrequency->getY() + lblFrequency->getHeight() + 10);
	grpSettings->add(lblSeparation, cboChannelMode->getX() + cboChannelMode->getWidth() + DISTANCE_NEXT_X * 3, 10);
	grpSettings->add(cboSeparation, lblSeparation->getX(), lblSeparation->getY() + lblSeparation->getHeight() + 10);
	grpSettings->add(lblStereoDelay, cboSeparation->getX(), lblFrequency->getY());
	grpSettings->add(cboStereoDelay, lblStereoDelay->getX(), cboFrequency->getY());
	grpSettings->add(lblInterpolation, cboSeparation->getX() + cboSeparation->getWidth() + DISTANCE_NEXT_X * 3, 10);
	grpSettings->add(cboInterpolation, lblInterpolation->getX(), lblInterpolation->getY() + lblInterpolation->getHeight() + 10);
	grpSettings->add(lblFilter, lblInterpolation->getX(), lblFrequency->getY());
	grpSettings->add(cboFilter, cboInterpolation->getX(), cboFrequency->getY());
	grpSettings->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2, cboFrequency->getY() + cboFrequency->getHeight() + 15 + DISTANCE_NEXT_Y * 2);
	grpSettings->setBaseColor(gui_baseCol);

	grpFloppySound = new gcn::Window("Floppy Drive Sound Emulation");
	grpFloppySound->add(chkFloppySound, 10, 10);
	grpFloppySound->add(lblFloppySoundEmpty, chkFloppySound->getX(), chkFloppySound->getY() + chkFloppySound->getHeight() + DISTANCE_NEXT_Y);
	grpFloppySound->add(sldFloppySoundEmpty, lblFloppySoundEmpty->getX() + lblFloppySoundEmpty->getWidth() + DISTANCE_NEXT_X + 10, lblFloppySoundEmpty->getY());
	grpFloppySound->add(lblFloppySoundEmptyInfo, sldFloppySoundEmpty->getX() + sldFloppySoundEmpty->getWidth() + DISTANCE_NEXT_X, sldFloppySoundEmpty->getY());
	grpFloppySound->add(lblFloppySoundDisk, lblFloppySoundEmpty->getX(), lblFloppySoundEmpty->getY() + lblFloppySoundEmpty->getHeight() + DISTANCE_NEXT_Y);
	grpFloppySound->add(sldFloppySoundDisk, sldFloppySoundEmpty->getX(), lblFloppySoundDisk->getY());
	grpFloppySound->add(lblFloppySoundDiskInfo, sldFloppySoundDisk->getX() + sldFloppySoundDisk->getWidth() + DISTANCE_NEXT_X, sldFloppySoundDisk->getY());
	grpFloppySound->setMovable(false);
	grpFloppySound->setSize(lblFloppySoundDisk->getWidth() + sldFloppySoundDisk->getWidth() + lblFloppySoundDiskInfo->getWidth() + DISTANCE_NEXT_X + 10 + DISTANCE_BORDER * 2, sldFloppySoundDisk->getY() + sldFloppySoundDisk->getHeight() + TITLEBAR_HEIGHT + DISTANCE_NEXT_Y * 2);
	grpFloppySound->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpFloppySound->setBaseColor(gui_baseCol);

	grpSoundBufferSize = new gcn::Window("Sound Buffer Size");
	grpSoundBufferSize->add(sldSoundBufferSize, DISTANCE_BORDER * 2, 10);
	grpSoundBufferSize->add(lblSoundBufferSize, sldSoundBufferSize->getX() + sldSoundBufferSize->getWidth() + DISTANCE_NEXT_X, sldSoundBufferSize->getY());
	grpSoundBufferSize->add(optSoundPull, DISTANCE_BORDER * 2, sldSoundBufferSize->getY() + sldSoundBufferSize->getHeight() + DISTANCE_NEXT_Y);
	grpSoundBufferSize->add(optSoundPush, optSoundPull->getX(), optSoundPull->getY() + optSoundPull->getHeight() + DISTANCE_NEXT_Y);
	grpSoundBufferSize->setMovable(false);
	grpSoundBufferSize->setSize(category.panel->getWidth() - grpFloppySound->getWidth() - DISTANCE_NEXT_X - DISTANCE_BORDER * 2, grpFloppySound->getHeight());
	grpSoundBufferSize->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpSoundBufferSize->setBaseColor(gui_baseCol);
	
	auto posY = DISTANCE_BORDER;
	category.panel->add(grpSound, DISTANCE_BORDER, posY);
	category.panel->add(grpVolume, grpSound->getX() + grpSound->getWidth() + DISTANCE_NEXT_X, posY);
	posY += grpSound->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(grpSettings, DISTANCE_BORDER, posY);
	posY += grpSettings->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(grpFloppySound, DISTANCE_BORDER, posY);
	category.panel->add(grpSoundBufferSize, grpFloppySound->getX() + grpFloppySound->getWidth() + DISTANCE_NEXT_X, posY);

	RefreshPanelSound();
}


void ExitPanelSound()
{
	delete optSoundDisabled;
	delete optSoundDisabledEmu;
	delete optSoundEmulated;
	delete optSoundEmulatedBest;
	delete grpSound;
	delete lblMasterVol;
	delete lblMasterVolInfo;
	delete sldMasterVol;
	delete lblPaulaVol;
	delete lblPaulaVolInfo;
	delete sldPaulaVol;
	delete lblCDVol;
	delete lblCDVolInfo;
	delete sldCDVol;
	delete lblAHIVol;
	delete lblAHIVolInfo;
	delete sldAHIVol;
	delete grpVolume;
	delete lblChannelMode;
	delete cboChannelMode;
	delete grpSettings;
	delete lblFrequency;
	delete cboFrequency;
	delete lblInterpolation;
	delete cboInterpolation;
	delete lblFilter;
	delete cboFilter;
	delete lblSeparation;
	delete cboSeparation;
	delete lblStereoDelay;
	delete cboStereoDelay;
	delete chkFloppySound;
	delete lblFloppySoundEmpty;
	delete lblFloppySoundEmptyInfo;
	delete sldFloppySoundEmpty;
	delete lblFloppySoundDisk;
	delete lblFloppySoundDiskInfo;
	delete sldFloppySoundDisk;
	delete grpFloppySound;
	delete sldSoundBufferSize;
	delete lblSoundBufferSize;
	delete optSoundPull;
	delete optSoundPush;
	delete grpSoundBufferSize;
	delete sound_action_listener;
}

static int getsoundbufsizeindex(int size)
{
	int idx;
	if (size < sndbufsizes[0])
		return 0;
	for (idx = 0; sndbufsizes[idx] < size && sndbufsizes[idx + 1] >= 0; idx++);
	return idx + 1;
}

void RefreshPanelSound()
{
	char tmp[10];

	switch (changed_prefs.produce_sound)
	{
	case 0:
		optSoundDisabled->setSelected(true);
		break;
	case 1:
		optSoundDisabledEmu->setSelected(true);
		break;
	case 2:
		optSoundEmulated->setSelected(true);
		break;
	case 3:
		optSoundEmulatedBest->setSelected(true);
		break;
	default:
		break;
	}

	cboChannelMode->setSelected(changed_prefs.sound_stereo);
	
	switch (changed_prefs.sound_freq)
	{
	case 11025:
		cboFrequency->setSelected(0);
		break;
	case 22050:
		cboFrequency->setSelected(1);
		break;
	case 32000:
		cboFrequency->setSelected(2);
		break;
	case 44100:
		cboFrequency->setSelected(3);
		break;
	case 48000:
		cboFrequency->setSelected(4);
		break;
	default:
		cboFrequency->setSelected(3);
		break;
	}

	cboInterpolation->setSelected(changed_prefs.sound_interpol);

	auto i = 0;
	switch (changed_prefs.sound_filter)
	{
	case 0:
		i = 0;
		break;
	case 1:
		i = changed_prefs.sound_filter_type ? 2 : 1;
		break;
	case 2:
		i = changed_prefs.sound_filter_type ? 4 : 3;
		break;
	default:
		break;
	}
	cboFilter->setSelected(i);

	if (changed_prefs.sound_stereo == 0)
	{
		curr_separation_idx = 0;
		curr_stereodelay_idx = 0;
	}
	else
	{
		curr_separation_idx = 10 - changed_prefs.sound_stereo_separation;
		curr_stereodelay_idx = changed_prefs.sound_mixed_stereo_delay > 0 ? changed_prefs.sound_mixed_stereo_delay : 0;
	}

	cboSeparation->setSelected(curr_separation_idx);
	cboSeparation->setEnabled(changed_prefs.sound_stereo >= 1);

	cboStereoDelay->setSelected(curr_stereodelay_idx);
	cboStereoDelay->setEnabled(changed_prefs.sound_stereo >= 1);

	sldMasterVol->setValue(100 - changed_prefs.sound_volume_master);
	snprintf(tmp, sizeof tmp - 1, "%d %%", 100 - changed_prefs.sound_volume_master);
	lblMasterVolInfo->setCaption(tmp);
	
	sldPaulaVol->setValue(100 - changed_prefs.sound_volume_paula);
	snprintf(tmp, sizeof tmp - 1, "%d %%", 100 - changed_prefs.sound_volume_paula);
	lblPaulaVolInfo->setCaption(tmp);

	sldCDVol->setEnabled(changed_prefs.cdslots[0].inuse);
	lblCDVol->setEnabled(changed_prefs.cdslots[0].inuse);
	lblCDVolInfo->setEnabled(changed_prefs.cdslots[0].inuse);

	sldCDVol->setValue(100 - changed_prefs.sound_volume_cd);
	snprintf(tmp, sizeof tmp - 1, "%d %%", 100 - changed_prefs.sound_volume_cd);
	lblCDVolInfo->setCaption(tmp);

	sldAHIVol->setValue(100 - changed_prefs.sound_volume_board);
	snprintf(tmp, sizeof tmp - 1, "%d %%", 100 - changed_prefs.sound_volume_board);
	lblAHIVolInfo->setCaption(tmp);

	chkFloppySound->setSelected(changed_prefs.floppyslots[0].dfxclick == 1);
	
	sldFloppySoundEmpty->setValue(100 - changed_prefs.dfxclickvolume_empty[0]);
	snprintf(tmp, sizeof tmp - 1, "%d %%", 100 - changed_prefs.dfxclickvolume_empty[0]);
	lblFloppySoundEmptyInfo->setCaption(tmp);
	
	sldFloppySoundDisk->setValue(100 - changed_prefs.dfxclickvolume_disk[0]);
	snprintf(tmp, sizeof tmp - 1, "%d %%", 100 - changed_prefs.dfxclickvolume_disk[0]);
	lblFloppySoundDiskInfo->setCaption(tmp);

	int bufsize = getsoundbufsizeindex(changed_prefs.sound_maxbsiz);
	if (bufsize <= 0)
	{
		lblSoundBufferSize->setCaption("Min");
		sldSoundBufferSize->setValue(0);
	}
	else
	{
		snprintf(tmp, sizeof tmp - 1, "%d", bufsize);
		lblSoundBufferSize->setCaption(tmp);
		sldSoundBufferSize->setValue(bufsize);
	}

	if (changed_prefs.sound_pullmode)
		optSoundPull->setSelected(true);
	else
		optSoundPush->setSelected(true);
	
	const auto enabled = changed_prefs.produce_sound > 0;
	cboChannelMode->setEnabled(enabled);
	lblFrequency->setEnabled(enabled);
	cboFrequency->setEnabled(enabled);
	lblInterpolation->setEnabled(enabled);
	cboInterpolation->setEnabled(enabled);
	lblFilter->setEnabled(enabled);
	cboFilter->setEnabled(enabled);
	lblSeparation->setEnabled(enabled);
	cboSeparation->setEnabled(enabled);
	lblStereoDelay->setEnabled(enabled);
	cboStereoDelay->setEnabled(enabled);
	lblPaulaVol->setEnabled(enabled);
	lblPaulaVolInfo->setEnabled(enabled);
	sldPaulaVol->setEnabled(enabled);
	lblMasterVol->setEnabled(false);
	lblMasterVolInfo->setEnabled(false);
	sldMasterVol->setEnabled(false);
	lblAHIVol->setEnabled(false);
	lblAHIVolInfo->setEnabled(false);
	sldAHIVol->setEnabled(false);
}

bool HelpPanelSound(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("You can turn on sound emulation with different levels of accuracy and");
	helptext.emplace_back("choose between Mono and Stereo.");
	helptext.emplace_back(" ");
	helptext.emplace_back("The different types of interpolation have different impact on performance.");
	helptext.emplace_back("Play with the settings to find the type you like most. You may need headphones");
	helptext.emplace_back("to really hear the difference between the interpolation types.");
	helptext.emplace_back(" ");
	helptext.emplace_back("With \"Filter\", you can select the type of the Amiga audio filter.");
	helptext.emplace_back(" ");
	helptext.emplace_back(R"(With "Stereo separation" and "Stereo delay", you can adjust how the left )");
	helptext.emplace_back("and right audio channels of the Amiga are mixed to the left and right channels ");
	helptext.emplace_back("of your device. A value of 70% for separation and no delay is a good start.");
	helptext.emplace_back(" ");
	helptext.emplace_back("The audio volume of the Amiga (not CD) can be adjusted with \"Paula Volume\".");
	return true;
}
