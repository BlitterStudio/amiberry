#ifdef USE_SDL1
#include <guichan.hpp>
#include <SDL/SDL_ttf.h>
#include <guichan/sdl.hpp>
#include "sdltruetypefont.hpp"
#elif USE_SDL2
#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#endif
#include "SelectorEntry.hpp"
#include "UaeRadioButton.hpp"
#include "UaeCheckBox.hpp"
#include "UaeDropDown.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "uae.h"
#include "gui.h"
#include "include/memory.h"
#include "newcpu.h"
#include "custom.h"
#include "gui_handling.h"
#include "sounddep/sound.h"


static gcn::Window* grpSound;
static gcn::UaeRadioButton* optSoundDisabled;
static gcn::UaeRadioButton* optSoundDisabledEmu;
static gcn::UaeRadioButton* optSoundEmulated;
static gcn::UaeRadioButton* optSoundEmulatedBest;
static gcn::Window* grpMode;
static gcn::UaeRadioButton* optMono;
static gcn::UaeRadioButton* optStereo;
static gcn::Label* lblFrequency;
static gcn::UaeDropDown* cboFrequency;
static gcn::Label* lblInterpolation;
static gcn::UaeDropDown* cboInterpolation;
static gcn::Label* lblFilter;
static gcn::UaeDropDown* cboFilter;
static gcn::Label* lblSeparation;
static gcn::Label* lblSeparationInfo;
static gcn::Slider* sldSeparation;
static gcn::Label* lblStereoDelay;
static gcn::Label* lblStereoDelayInfo;
static gcn::Slider* sldStereoDelay;

static int curr_separation_idx;
static int curr_stereodelay_idx;


class FrequencyListModel : public gcn::ListModel
{
	vector<string> freq;

public:
	FrequencyListModel()
	{
		freq.push_back("11025");
		freq.push_back("22050");
		freq.push_back("32000");
		freq.push_back("44100");
	}

	int getNumberOfElements() override
	{
		return freq.size();
	}

	string getElementAt(const int i) override
	{
		if (i < 0 || i >= freq.size())
			return "---";
		return freq[i];
	}
};

static FrequencyListModel frequencyTypeList;


class InterpolationListModel : public gcn::ListModel
{
	vector<string> entry;

public:
	InterpolationListModel()
	{
		entry.push_back("Disabled");
		entry.push_back("Anti");
		entry.push_back("Sinc");
		entry.push_back("RH");
		entry.push_back("Crux");
	}

	int getNumberOfElements() override
	{
		return entry.size();
	}

	string getElementAt(const int i) override
	{
		if (i < 0 || i >= entry.size())
			return "---";
		return entry[i];
	}
};

static InterpolationListModel interpolationTypeList;


class FilterListModel : public gcn::ListModel
{
	vector<string> entry;

public:
	FilterListModel()
	{
		entry.push_back("Always off");
		entry.push_back("Emulated (A500)");
		entry.push_back("Emulated (A1200)");
		entry.push_back("Always on (A500)");
		entry.push_back("Always on (A1200)");
	}

	int getNumberOfElements() override
	{
		return entry.size();
	}

	string getElementAt(const int i) override
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
			if (curr_separation_idx != int(sldSeparation->getValue())
				&& changed_prefs.sound_stereo > 0)
			{
				curr_separation_idx = int(sldSeparation->getValue());
				changed_prefs.sound_stereo_separation = 10 - curr_separation_idx;
			}
		}

		else if (actionEvent.getSource() == sldStereoDelay)
		{
			if (curr_stereodelay_idx != int(sldStereoDelay->getValue())
				&& changed_prefs.sound_stereo > 0)
			{
				curr_stereodelay_idx = int(sldStereoDelay->getValue());
				if (curr_stereodelay_idx > 0)
					changed_prefs.sound_mixed_stereo_delay = curr_stereodelay_idx;
				else
					changed_prefs.sound_mixed_stereo_delay = -1;
			}
		}

		RefreshPanelSound();
	}
};

static SoundActionListener* soundActionListener;


void InitPanelSound(const struct _ConfigCategory& category)
{
	soundActionListener = new SoundActionListener();

	optSoundDisabled = new gcn::UaeRadioButton("Disabled", "radiosoundgroup");
	optSoundDisabled->setId("sndDisable");
	optSoundDisabled->addActionListener(soundActionListener);

	optSoundDisabledEmu = new gcn::UaeRadioButton("Disabled, but emulated", "radiosoundgroup");
	optSoundDisabledEmu->setId("sndDisEmu");
	optSoundDisabledEmu->addActionListener(soundActionListener);

	optSoundEmulated = new gcn::UaeRadioButton("Enabled", "radiosoundgroup");
	optSoundEmulated->setId("sndEmulate");
	optSoundEmulated->addActionListener(soundActionListener);

	optSoundEmulatedBest = new gcn::UaeRadioButton("Enabled, most accurate", "radiosoundgroup");
	optSoundEmulatedBest->setId("sndEmuBest");
	optSoundEmulatedBest->addActionListener(soundActionListener);

	grpSound = new gcn::Window("Sound Emulation");
	grpSound->add(optSoundDisabled, 5, 10);
	grpSound->add(optSoundDisabledEmu, 5, 40);
	grpSound->add(optSoundEmulated, 5, 70);
	grpSound->add(optSoundEmulatedBest, 5, 100);
	grpSound->setMovable(false);
	grpSound->setSize(optSoundEmulatedBest->getWidth() + DISTANCE_BORDER, 150);
	grpSound->setBaseColor(gui_baseCol);

	lblFrequency = new gcn::Label("Frequency:");
	lblFrequency->setAlignment(gcn::Graphics::RIGHT);
	cboFrequency = new gcn::UaeDropDown(&frequencyTypeList);
	cboFrequency->setSize(160, DROPDOWN_HEIGHT);
	cboFrequency->setBaseColor(gui_baseCol);
	cboFrequency->setBackgroundColor(colTextboxBackground);
	cboFrequency->setId("cboFrequency");
	cboFrequency->addActionListener(soundActionListener);

	optMono = new gcn::UaeRadioButton("Mono", "radiosoundmodegroup");
	optMono->addActionListener(soundActionListener);

	optStereo = new gcn::UaeRadioButton("Stereo", "radiosoundmodegroup");
	optStereo->addActionListener(soundActionListener);

	grpMode = new gcn::Window("Mode");
	grpMode->add(optMono, 5, 10);
	grpMode->add(optStereo, 5, 40);
	grpMode->setMovable(false);
	grpMode->setSize(90, 90);
	grpMode->setBaseColor(gui_baseCol);

	lblInterpolation = new gcn::Label("Interpolation:");
	lblInterpolation->setAlignment(gcn::Graphics::RIGHT);
	cboInterpolation = new gcn::UaeDropDown(&interpolationTypeList);
	cboInterpolation->setSize(160, DROPDOWN_HEIGHT);
	cboInterpolation->setBaseColor(gui_baseCol);
	cboInterpolation->setBackgroundColor(colTextboxBackground);
	cboInterpolation->setId("cboInterpol");
	cboInterpolation->addActionListener(soundActionListener);

	lblFilter = new gcn::Label("Filter:");
	lblFilter->setAlignment(gcn::Graphics::RIGHT);
	cboFilter = new gcn::UaeDropDown(&filterTypeList);
	cboFilter->setSize(160, DROPDOWN_HEIGHT);
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

	int posY = DISTANCE_BORDER;
	category.panel->add(grpSound, DISTANCE_BORDER, posY);
	category.panel->add(grpMode, grpSound->getX() + grpSound->getWidth() + DISTANCE_NEXT_X, posY);
	posY += grpSound->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblFrequency, DISTANCE_BORDER, posY);
	category.panel->add(cboFrequency, lblFrequency->getX() + lblFrequency->getWidth() + DISTANCE_NEXT_X, posY);
	posY += cboFrequency->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblInterpolation, DISTANCE_BORDER, posY);
	category.panel->add(cboInterpolation, lblInterpolation->getX() + lblInterpolation->getWidth() + DISTANCE_NEXT_X, posY);
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
	category.panel->add(lblStereoDelayInfo, sldStereoDelay->getX() + sldStereoDelay->getWidth() + DISTANCE_NEXT_X, posY);
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

	int i = 0;
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
}

bool HelpPanelSound(std::vector<std::string> &helptext)
{
	helptext.clear();
	helptext.push_back("You can turn on sound emulation with different levels of accuracy and choose between mono and stereo.");
	helptext.push_back("");
	helptext.push_back("The different types of interpolation have different impact on the performance. Play with the settings to find the");
	helptext.push_back("type you like most. You may need headphones to really hear the differences between the interpolations.");
	helptext.push_back("");
	helptext.push_back("With \"Filter\", you can select the type of the Amiga audio filter.");
	helptext.push_back("");
	helptext.push_back("With \"Stereo separation\" and \"Stereo delay\", you can adjust how the left and right audio channels of the Amiga");
	helptext.push_back("are mixed to the left and right channels of your device. A value of 70% for separation and no delay is a good");
	helptext.push_back("start.");
	return true;
}
