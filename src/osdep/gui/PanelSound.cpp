#include <stdio.h>

#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "options.h"
#include "gui_handling.h"
#include "sounddep/sound.h"


static gcn::Window* grpSound;
static gcn::RadioButton* optSoundDisabled;
static gcn::RadioButton* optSoundDisabledEmu;
static gcn::RadioButton* optSoundEmulated;
static gcn::RadioButton* optSoundEmulatedBest;
static gcn::Window* grpMode;
static gcn::RadioButton* optMono;
static gcn::RadioButton* optStereo;
static gcn::Label* lblFrequency;
static gcn::DropDown* cboFrequency;
static gcn::Label* lblInterpolation;
static gcn::DropDown* cboInterpolation;
static gcn::Label* lblFilter;
static gcn::DropDown* cboFilter;
static gcn::Label* lblSeparation;
static gcn::Label* lblSeparationInfo;
static gcn::Slider* sldSeparation;
static gcn::Label* lblStereoDelay;
static gcn::Label* lblStereoDelayInfo;
static gcn::Slider* sldStereoDelay;
static gcn::Label* lblPaulaVol;
static gcn::Label* lblPaulaVolInfo;
static gcn::Slider* sldPaulaVol;

static int curr_separation_idx;
static int curr_stereodelay_idx;


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
		if (i < 0 || i >= freq.size())
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
		if (i < 0 || i >= entry.size())
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
		if (i < 0 || i >= entry.size())
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

		else if (actionEvent.getSource() == optMono)
			changed_prefs.sound_stereo = 0;
		else if (actionEvent.getSource() == optStereo)
			changed_prefs.sound_stereo = 1;

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

		else if (actionEvent.getSource() == sldSeparation)
		{
			if (curr_separation_idx != static_cast<int>(sldSeparation->getValue())
				&& changed_prefs.sound_stereo > 0)
			{
				curr_separation_idx = static_cast<int>(sldSeparation->getValue());
				changed_prefs.sound_stereo_separation = 10 - curr_separation_idx;
			}
		}

		else if (actionEvent.getSource() == sldStereoDelay)
		{
			if (curr_stereodelay_idx != static_cast<int>(sldStereoDelay->getValue())
				&& changed_prefs.sound_stereo > 0)
			{
				curr_stereodelay_idx = static_cast<int>(sldStereoDelay->getValue());
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

	grpSound = new gcn::Window("Sound Emulation");
	grpSound->add(optSoundDisabled, 5, 10);
	grpSound->add(optSoundDisabledEmu, 5, 40);
	grpSound->add(optSoundEmulated, 5, 70);
	grpSound->add(optSoundEmulatedBest, 5, 100);
	grpSound->setMovable(false);
	grpSound->setSize(optSoundEmulatedBest->getWidth() + DISTANCE_BORDER, 150);
	grpSound->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpSound->setBaseColor(gui_baseCol);

	lblFrequency = new gcn::Label("Frequency:");
	lblFrequency->setAlignment(gcn::Graphics::RIGHT);
	cboFrequency = new gcn::DropDown(&frequencyTypeList);
	cboFrequency->setSize(160, cboFrequency->getHeight());
	cboFrequency->setBaseColor(gui_baseCol);
	cboFrequency->setBackgroundColor(colTextboxBackground);
	cboFrequency->setId("cboFrequency");
	cboFrequency->addActionListener(soundActionListener);

	optMono = new gcn::RadioButton("Mono", "radiosoundmodegroup");
	optMono->setId("Mono");
	optMono->addActionListener(soundActionListener);

	optStereo = new gcn::RadioButton("Stereo", "radiosoundmodegroup");
	optStereo->setId("Stereo");
	optStereo->addActionListener(soundActionListener);

	grpMode = new gcn::Window("Mode");
	grpMode->add(optMono, 5, 10);
	grpMode->add(optStereo, 5, 40);
	grpMode->setMovable(false);
	grpMode->setSize(90, 90);
	grpMode->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpMode->setBaseColor(gui_baseCol);

	lblInterpolation = new gcn::Label("Interpolation:");
	lblInterpolation->setAlignment(gcn::Graphics::RIGHT);
	cboInterpolation = new gcn::DropDown(&interpolationTypeList);
	cboInterpolation->setSize(160, cboInterpolation->getHeight());
	cboInterpolation->setBaseColor(gui_baseCol);
	cboInterpolation->setBackgroundColor(colTextboxBackground);
	cboInterpolation->setId("cboInterpol");
	cboInterpolation->addActionListener(soundActionListener);

	lblFilter = new gcn::Label("Filter:");
	lblFilter->setAlignment(gcn::Graphics::RIGHT);
	cboFilter = new gcn::DropDown(&filterTypeList);
	cboFilter->setSize(160, cboFilter->getHeight());
	cboFilter->setBaseColor(gui_baseCol);
	cboFilter->setBackgroundColor(colTextboxBackground);
	cboFilter->setId("cboFilter");
	cboFilter->addActionListener(soundActionListener);

	lblSeparation = new gcn::Label("Stereo separation:");
	lblSeparation->setAlignment(gcn::Graphics::RIGHT);
	sldSeparation = new gcn::Slider(0, 10);
	sldSeparation->setSize(160, SLIDER_HEIGHT);
	sldSeparation->setBaseColor(gui_baseCol);
	sldSeparation->setMarkerLength(20);
	sldSeparation->setStepLength(1);
	sldSeparation->setId("sldSeparation");
	sldSeparation->addActionListener(soundActionListener);
	lblSeparationInfo = new gcn::Label("100%");

	lblStereoDelay = new gcn::Label("Stereo delay:");
	lblStereoDelay->setAlignment(gcn::Graphics::RIGHT);
	sldStereoDelay = new gcn::Slider(0, 10);
	sldStereoDelay->setSize(160, SLIDER_HEIGHT);
	sldStereoDelay->setBaseColor(gui_baseCol);
	sldStereoDelay->setMarkerLength(20);
	sldStereoDelay->setStepLength(1);
	sldStereoDelay->setId("sldStereoDelay");
	sldStereoDelay->addActionListener(soundActionListener);
	lblStereoDelayInfo = new gcn::Label("10");

	lblPaulaVol = new gcn::Label("Paula Volume:");
	lblPaulaVol->setAlignment(gcn::Graphics::RIGHT);
	sldPaulaVol = new gcn::Slider(0, 100);
	sldPaulaVol->setSize(160, SLIDER_HEIGHT);
	sldPaulaVol->setBaseColor(gui_baseCol);
	sldPaulaVol->setMarkerLength(20);
	sldPaulaVol->setStepLength(10);
	sldPaulaVol->setId("sldPaulaVol");
	sldPaulaVol->addActionListener(soundActionListener);
	lblPaulaVolInfo = new gcn::Label("80 %");

	auto posY = DISTANCE_BORDER;
	category.panel->add(grpSound, DISTANCE_BORDER, posY);
	category.panel->add(grpMode, grpSound->getX() + grpSound->getWidth() + DISTANCE_NEXT_X, posY);
	posY += grpSound->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblFrequency, DISTANCE_BORDER, posY);
	category.panel->add(cboFrequency, lblFrequency->getX() + lblFrequency->getWidth() + DISTANCE_NEXT_X, posY);
	posY += cboFrequency->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblInterpolation, DISTANCE_BORDER, posY);
	category.panel->add(cboInterpolation, lblInterpolation->getX() + lblInterpolation->getWidth() + DISTANCE_NEXT_X,
	                    posY);
	posY += cboInterpolation->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblFilter, DISTANCE_BORDER, posY);
	category.panel->add(cboFilter, lblFilter->getX() + lblFilter->getWidth() + DISTANCE_NEXT_X, posY);
	posY += cboFilter->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblSeparation, DISTANCE_BORDER, posY);
	category.panel->add(sldSeparation, lblSeparation->getX() + lblSeparation->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(lblSeparationInfo, sldSeparation->getX() + sldSeparation->getWidth() + DISTANCE_NEXT_X, posY);
	posY += SLIDER_HEIGHT + DISTANCE_NEXT_Y;
	category.panel->add(lblStereoDelay, DISTANCE_BORDER, posY);
	category.panel->add(sldStereoDelay, lblStereoDelay->getX() + lblStereoDelay->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(lblStereoDelayInfo, sldStereoDelay->getX() + sldStereoDelay->getWidth() + DISTANCE_NEXT_X,
	                    posY);
	posY += SLIDER_HEIGHT + DISTANCE_NEXT_Y;
	category.panel->add(lblPaulaVol, DISTANCE_BORDER, posY);
	category.panel->add(sldPaulaVol, lblPaulaVol->getX() + lblPaulaVol->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(lblPaulaVolInfo, sldPaulaVol->getX() + sldPaulaVol->getWidth() + DISTANCE_NEXT_X, posY);
	posY += SLIDER_HEIGHT + DISTANCE_NEXT_Y;

	RefreshPanelSound();
}


void ExitPanelSound()
{
	delete optSoundDisabled;
	delete optSoundDisabledEmu;
	delete optSoundEmulated;
	delete optSoundEmulatedBest;
	delete grpSound;
	delete optMono;
	delete optStereo;
	delete grpMode;
	delete lblFrequency;
	delete cboFrequency;
	delete lblInterpolation;
	delete cboInterpolation;
	delete lblFilter;
	delete cboFilter;
	delete lblSeparation;
	delete sldSeparation;
	delete lblSeparationInfo;
	delete lblStereoDelay;
	delete sldStereoDelay;
	delete lblStereoDelayInfo;
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

	if (changed_prefs.sound_stereo == 0)
		optMono->setSelected(true);
	else if (changed_prefs.sound_stereo == 1)
		optStereo->setSelected(true);

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

	sldSeparation->setValue(curr_separation_idx);
	sldSeparation->setEnabled(changed_prefs.sound_stereo >= 1);
	snprintf(tmp, 10, "%d%%", 100 - 10 * curr_separation_idx);
	lblSeparationInfo->setCaption(tmp);

	sldStereoDelay->setValue(curr_stereodelay_idx);
	sldStereoDelay->setEnabled(changed_prefs.sound_stereo >= 1);
	if (curr_stereodelay_idx <= 0)
		lblStereoDelayInfo->setCaption("-");
	else
	{
		snprintf(tmp, 10, "%d", curr_stereodelay_idx);
		lblStereoDelayInfo->setCaption(tmp);
	}
	sldPaulaVol->setValue(100 - changed_prefs.sound_volume_paula);
	snprintf(tmp, sizeof(tmp) - 1, "%d %%", 100 - changed_prefs.sound_volume_paula);
	lblPaulaVolInfo->setCaption(tmp);
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
