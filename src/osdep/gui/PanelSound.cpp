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
static gcn::Window* grpSettings;
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
static gcn::Label* lblPaulaVol;
static gcn::Label* lblPaulaVolInfo;
static gcn::Slider* sldPaulaVol;
static gcn::Label* lblCDVol;
static gcn::Label* lblCDVolInfo;
static gcn::Slider* sldCDVol;

static int curr_separation_idx;
static int curr_stereodelay_idx;


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

static ChannelModeListModel channelModeList;

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

static SeparationListModel separationList;

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

static StereoDelayListModel stereoDelayList;

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

static FrequencyListModel frequencyTypeList;


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

static InterpolationListModel interpolationTypeList;


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

static FilterListModel filterTypeList;


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

		RefreshPanelSound();
	}
};

static SoundActionListener* soundActionListener;


void InitPanelSound(const struct _ConfigCategory& category)
{
	soundActionListener = new SoundActionListener();

	optSoundDisabled = new gcn::RadioButton("Disabled", "radiosoundgroup");
	optSoundDisabled->setId("sndDisable");
	optSoundDisabled->addActionListener(soundActionListener);

	optSoundDisabledEmu = new gcn::RadioButton("Disabled, but emulated", "radiosoundgroup");
	optSoundDisabledEmu->setId("sndDisEmu");
	optSoundDisabledEmu->addActionListener(soundActionListener);

	optSoundEmulated = new gcn::RadioButton("Enabled", "radiosoundgroup");
	optSoundEmulated->setId("sndEmulate");
	optSoundEmulated->addActionListener(soundActionListener);

	optSoundEmulatedBest = new gcn::RadioButton("Enabled, most accurate", "radiosoundgroup");
	optSoundEmulatedBest->setId("sndEmuBest");
	optSoundEmulatedBest->addActionListener(soundActionListener);

	lblFrequency = new gcn::Label("Frequency:");
	lblFrequency->setAlignment(gcn::Graphics::RIGHT);
	cboFrequency = new gcn::DropDown(&frequencyTypeList);
	cboFrequency->setSize(100, cboFrequency->getHeight());
	cboFrequency->setBaseColor(gui_baseCol);
	cboFrequency->setBackgroundColor(colTextboxBackground);
	cboFrequency->setId("cboFrequency");
	cboFrequency->addActionListener(soundActionListener);

	lblChannelMode = new gcn::Label("Channel mode:");
	lblChannelMode->setAlignment(gcn::Graphics::RIGHT);
	cboChannelMode = new gcn::DropDown(&channelModeList);
	cboChannelMode->setSize(100, cboChannelMode->getHeight());
	cboChannelMode->setBaseColor(gui_baseCol);
	cboChannelMode->setBackgroundColor(colTextboxBackground);
	cboChannelMode->setId("cboChannelMode");
	cboChannelMode->addActionListener(soundActionListener);
	
	lblInterpolation = new gcn::Label("Interpolation:");
	lblInterpolation->setAlignment(gcn::Graphics::RIGHT);
	cboInterpolation = new gcn::DropDown(&interpolationTypeList);
	cboInterpolation->setSize(150, cboInterpolation->getHeight());
	cboInterpolation->setBaseColor(gui_baseCol);
	cboInterpolation->setBackgroundColor(colTextboxBackground);
	cboInterpolation->setId("cboInterpol");
	cboInterpolation->addActionListener(soundActionListener);

	lblFilter = new gcn::Label("Filter:");
	lblFilter->setAlignment(gcn::Graphics::RIGHT);
	cboFilter = new gcn::DropDown(&filterTypeList);
	cboFilter->setSize(150, cboFilter->getHeight());
	cboFilter->setBaseColor(gui_baseCol);
	cboFilter->setBackgroundColor(colTextboxBackground);
	cboFilter->setId("cboFilter");
	cboFilter->addActionListener(soundActionListener);

	lblSeparation = new gcn::Label("Stereo separation:");
	lblSeparation->setAlignment(gcn::Graphics::RIGHT);
	cboSeparation = new gcn::DropDown(&separationList);
	cboSeparation->setSize(150, cboSeparation->getHeight());
	cboSeparation->setBaseColor(gui_baseCol);
	cboSeparation->setBackgroundColor(colTextboxBackground);
	cboSeparation->setId("cboSeparation");
	cboSeparation->addActionListener(soundActionListener);
	
	lblStereoDelay = new gcn::Label("Stereo delay:");
	lblStereoDelay->setAlignment(gcn::Graphics::RIGHT);
	cboStereoDelay = new gcn::DropDown(&stereoDelayList);
	cboStereoDelay->setSize(150, cboStereoDelay->getHeight());
	cboStereoDelay->setBaseColor(gui_baseCol);
	cboStereoDelay->setBackgroundColor(colTextboxBackground);
	cboStereoDelay->setId("cboSeparation");
	cboStereoDelay->addActionListener(soundActionListener);

	lblPaulaVol = new gcn::Label("Paula Volume:");
	lblPaulaVol->setAlignment(gcn::Graphics::RIGHT);
	sldPaulaVol = new gcn::Slider(0, 100);
	sldPaulaVol->setSize(150, SLIDER_HEIGHT);
	sldPaulaVol->setBaseColor(gui_baseCol);
	sldPaulaVol->setMarkerLength(20);
	sldPaulaVol->setStepLength(10);
	sldPaulaVol->setId("sldPaulaVol");
	sldPaulaVol->addActionListener(soundActionListener);
	lblPaulaVolInfo = new gcn::Label("100 %");

	lblCDVol = new gcn::Label("CD Volume:");
	lblCDVol->setAlignment(gcn::Graphics::RIGHT);
	sldCDVol = new gcn::Slider(0, 100);
	sldCDVol->setSize(150, SLIDER_HEIGHT);
	sldCDVol->setBaseColor(gui_baseCol);
	sldCDVol->setMarkerLength(20);
	sldCDVol->setStepLength(10);
	sldCDVol->setId("sldCDVol");
	sldCDVol->addActionListener(soundActionListener);
	lblCDVolInfo = new gcn::Label("80 %");

	grpSound = new gcn::Window("Sound Emulation");
	grpSound->add(optSoundDisabled, 10, 10);
	grpSound->add(optSoundDisabledEmu, 10, 40);
	grpSound->add(optSoundEmulated, 10, 70);
	grpSound->add(optSoundEmulatedBest, 10, 100);
	grpSound->setMovable(false);
	grpSound->setSize(optSoundEmulatedBest->getWidth() + DISTANCE_BORDER + 10, 160);
	grpSound->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpSound->setBaseColor(gui_baseCol);

	grpVolume = new gcn::Window("Volume");
	grpVolume->add(lblPaulaVol, 10, 10);
	grpVolume->add(sldPaulaVol, lblPaulaVol->getX() + lblPaulaVol->getWidth() + 10, lblPaulaVol->getY());
	grpVolume->add(lblPaulaVolInfo, sldPaulaVol->getX() + sldPaulaVol->getWidth() + 10, sldPaulaVol->getY());
	grpVolume->add(lblCDVol, lblPaulaVol->getX(), lblPaulaVol->getY() + lblPaulaVol->getHeight() + DISTANCE_NEXT_Y);
	grpVolume->add(sldCDVol, sldPaulaVol->getX(), lblCDVol->getY());
	grpVolume->add(lblCDVolInfo, sldCDVol->getX() + sldCDVol->getWidth() + 10, sldCDVol->getY());
	grpVolume->setMovable(false);
	grpVolume->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2 - grpSound->getWidth() - DISTANCE_NEXT_X, grpSound->getHeight());
	grpVolume->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpVolume->setBaseColor(gui_baseCol);
	
	grpSettings = new gcn::Window("Settings");
	grpSettings->add(lblChannelMode, 10, 10);
	grpSettings->add(cboChannelMode, 10, lblChannelMode->getY() + lblChannelMode->getHeight() + 10);
	grpSettings->add(lblFrequency, 10, 70);
	grpSettings->add(cboFrequency, 10, lblFrequency->getY() + lblFrequency->getHeight() + 10);
	grpSettings->add(lblSeparation, cboChannelMode->getX() + cboChannelMode->getWidth() + DISTANCE_NEXT_X * 3, 10);
	grpSettings->add(cboSeparation, lblSeparation->getX(), lblSeparation->getY() + lblSeparation->getHeight() + 10);
	grpSettings->add(lblStereoDelay, cboSeparation->getX(), lblFrequency->getY());
	grpSettings->add(cboStereoDelay, lblStereoDelay->getX(), cboFrequency->getY());
	grpSettings->add(lblInterpolation, cboSeparation->getX() + cboSeparation->getWidth() + DISTANCE_NEXT_X * 3, 10);
	grpSettings->add(cboInterpolation, lblInterpolation->getX(), lblInterpolation->getY() + lblInterpolation->getHeight() + 10);
	grpSettings->add(lblFilter, lblInterpolation->getX(), lblFrequency->getY());
	grpSettings->add(cboFilter, cboInterpolation->getX(), cboFrequency->getY());
	grpSettings->setMovable(false);
	grpSettings->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2, category.panel->getHeight() - grpSound->getHeight() -DISTANCE_NEXT_Y * 3);
	grpSettings->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpSettings->setBaseColor(gui_baseCol);
	
	auto posY = DISTANCE_BORDER;
	category.panel->add(grpSound, DISTANCE_BORDER, posY);
	category.panel->add(grpVolume, grpSound->getX() + grpSound->getWidth() + DISTANCE_NEXT_X, posY);
	posY += grpSound->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(grpSettings, DISTANCE_BORDER, posY);

	RefreshPanelSound();
}


void ExitPanelSound()
{
	delete optSoundDisabled;
	delete optSoundDisabledEmu;
	delete optSoundEmulated;
	delete optSoundEmulatedBest;
	delete grpSound;
	delete lblPaulaVol;
	delete lblPaulaVolInfo;
	delete sldPaulaVol;
	delete lblCDVol;
	delete lblCDVolInfo;
	delete sldCDVol;
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
	delete soundActionListener;
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

	sldPaulaVol->setValue(100 - changed_prefs.sound_volume_paula);
	snprintf(tmp, sizeof tmp - 1, "%d %%", 100 - changed_prefs.sound_volume_paula);
	lblPaulaVolInfo->setCaption(tmp);

	sldCDVol->setEnabled(changed_prefs.cdslots[0].inuse);
	lblCDVol->setEnabled(changed_prefs.cdslots[0].inuse);
	lblCDVolInfo->setEnabled(changed_prefs.cdslots[0].inuse);

	sldCDVol->setValue(100 - changed_prefs.sound_volume_cd);
	snprintf(tmp, 10, "%d %%", 100 - changed_prefs.sound_volume_cd);
	lblCDVolInfo->setCaption(tmp);
	
	cboChannelMode->setEnabled(changed_prefs.produce_sound > 0);
	lblFrequency->setEnabled(changed_prefs.produce_sound > 0);
	cboFrequency->setEnabled(changed_prefs.produce_sound > 0);
	lblInterpolation->setEnabled(changed_prefs.produce_sound > 0);
	cboInterpolation->setEnabled(changed_prefs.produce_sound > 0);
	lblFilter->setEnabled(changed_prefs.produce_sound > 0);
	cboFilter->setEnabled(changed_prefs.produce_sound > 0);
	lblSeparation->setEnabled(changed_prefs.produce_sound > 0);
	cboSeparation->setEnabled(changed_prefs.produce_sound > 0);
	lblStereoDelay->setEnabled(changed_prefs.produce_sound > 0);
	cboStereoDelay->setEnabled(changed_prefs.produce_sound > 0);
	lblPaulaVol->setEnabled(changed_prefs.produce_sound > 0);
	lblPaulaVolInfo->setEnabled(changed_prefs.produce_sound > 0);
	sldPaulaVol->setEnabled(changed_prefs.produce_sound > 0);
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
