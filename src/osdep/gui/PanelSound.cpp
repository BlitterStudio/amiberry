#include <cstdio>

#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"
#include "StringListModel.h"

#include "sysdeps.h"
#include "options.h"
#include "gui_handling.h"
#include "audio.h"
#include "sounddep/sound.h"

static gcn::Window* grpSound;
static gcn::RadioButton* optSoundDisabled;
static gcn::RadioButton* optSoundDisabledEmu;
static gcn::RadioButton* optSoundEmulated;
static gcn::CheckBox* chkAutoSwitching;
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
static gcn::Label* lblAHIVol;
static gcn::Label* lblAHIVolInfo;
static gcn::Slider* sldAHIVol;
static gcn::Label* lblMIDIVol;
static gcn::Label* lblMIDIVolInfo;
static gcn::Slider* sldMIDIVol;
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
static gcn::DropDown* cboSoundcard;
static gcn::Label* lblSwapChannels;
static gcn::DropDown* cboSwapChannels;

static int curr_separation_idx;
static int curr_stereodelay_idx;
static int numdevs;
static const int sndbufsizes[] = { 1024, 2048, 3072, 4096, 6144, 8192, 12288, 16384, 32768, 65536, -1 };

static gcn::StringListModel soundcard_list;

static const std::vector<std::string> swap_channels = { "-", "Paula only", "AHI only", "Both" };
static gcn::StringListModel swap_channels_list(swap_channels);

static const std::vector<std::string> channel_modes = { "Mono", "Stereo", "Cloned stereo (4 channels)", "4 Channels", "Cloned stereo (5.1)", "5.1 Channels", "Cloned stereo (7.1)", "7.1 channels" };
static gcn::StringListModel channel_mode_list(channel_modes);

static const std::vector<std::string> separation = { "100%", "90%", "80%", "70%", "60%", "50%", "40%", "30%", "20%", "10%", "0%" };
static gcn::StringListModel separation_list(separation);

static const std::vector<std::string> stereo_delay = { "-", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10" };
static gcn::StringListModel stereo_delay_list(stereo_delay);

static const std::vector<std::string> frequency = { "11025", "22050", "32000", "44100", "48000" };
static gcn::StringListModel frequency_type_list(frequency);

static const std::vector<std::string> interpolation = { "Disabled", "Anti", "Sinc", "RH", "Crux" };
static gcn::StringListModel interpolation_type_list(interpolation);

static const std::vector<std::string> filter = { "Always off", "Emulated (A500)", "Emulated (A1200)", "Always on (A500)", "Always on (A1200)" };
static gcn::StringListModel filter_type_list(filter);

class SoundActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cboSoundcard)
		{
			const int soundcard = cboSoundcard->getSelected();
			if (soundcard != changed_prefs.soundcard)
				changed_prefs.soundcard = soundcard;
		}
		else if (actionEvent.getSource() == optSoundDisabled)
			changed_prefs.produce_sound = 0;
		else if (actionEvent.getSource() == optSoundDisabledEmu)
			changed_prefs.produce_sound = 1;
		else if (actionEvent.getSource() == optSoundEmulated)
			changed_prefs.produce_sound = 2;

		else if (actionEvent.getSource() == chkAutoSwitching)
			changed_prefs.sound_auto = chkAutoSwitching->isSelected();

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

		else if (actionEvent.getSource() == cboSwapChannels)
		{
			switch (cboSwapChannels->getSelected())
			{
			case 0:
				changed_prefs.sound_stereo_swap_paula = false;
				changed_prefs.sound_stereo_swap_ahi = false;
				break;
			case 1:
				changed_prefs.sound_stereo_swap_paula = true;
				changed_prefs.sound_stereo_swap_ahi = false;
				break;
			case 2:
				changed_prefs.sound_stereo_swap_paula = false;
				changed_prefs.sound_stereo_swap_ahi = true;
				break;
			case 3:
				changed_prefs.sound_stereo_swap_paula = true;
				changed_prefs.sound_stereo_swap_ahi = true;
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
			if (curr_separation_idx != cboSeparation->getSelected()
				&& changed_prefs.sound_stereo > 0)
			{
				curr_separation_idx = cboSeparation->getSelected();
				changed_prefs.sound_stereo_separation = 10 - curr_separation_idx;
			}
		}

		else if (actionEvent.getSource() == cboStereoDelay)
		{
			if (curr_stereodelay_idx != cboStereoDelay->getSelected()
				&& changed_prefs.sound_stereo > 0)
			{
				curr_stereodelay_idx = cboStereoDelay->getSelected();
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
		else if (actionEvent.getSource() == sldAHIVol)
		{
			const auto newvol = 100 - static_cast<int>(sldAHIVol->getValue());
			if (changed_prefs.sound_volume_board != newvol)
				changed_prefs.sound_volume_board = newvol;
		}
		else if (actionEvent.getSource() == sldMIDIVol)
		{
			const auto newvol = 100 - static_cast<int>(sldMIDIVol->getValue());
			if (changed_prefs.sound_volume_midi != newvol)
				changed_prefs.sound_volume_midi = newvol;
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

void InitPanelSound(const config_category& category)
{
	numdevs = enumerate_sound_devices();
	soundcard_list.clear();
	for (int card = 0; card < numdevs; card++) {
		TCHAR tmp[MAX_DPATH];
		int type = sound_devices[card]->type;
		_stprintf(tmp, _T("%s: %s"),
			type == SOUND_DEVICE_SDL2 ? _T("SDL2") : (type == SOUND_DEVICE_DS ? _T("DSOUND") : (type == SOUND_DEVICE_AL ? _T("OpenAL") : (type == SOUND_DEVICE_PA ? _T("PortAudio") : (type == SOUND_DEVICE_WASAPI ? _T("WASAPI") : _T("WASAPI EX"))))),
			sound_devices[card]->name);
		soundcard_list.add(tmp);
	}
	if (numdevs == 0)
		changed_prefs.produce_sound = 0; // No sound card in system

	sound_action_listener = new SoundActionListener();

	cboSoundcard = new gcn::DropDown(&soundcard_list);
	cboSoundcard->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2, cboSoundcard->getHeight());
	cboSoundcard->setBaseColor(gui_baseCol);
	cboSoundcard->setBackgroundColor(colTextboxBackground);
	cboSoundcard->setId("cboSoundcard");
	cboSoundcard->addActionListener(sound_action_listener);

	optSoundDisabled = new gcn::RadioButton("Disabled", "radiosoundgroup");
	optSoundDisabled->setId("sndDisable");
	optSoundDisabled->addActionListener(sound_action_listener);

	optSoundDisabledEmu = new gcn::RadioButton("Disabled, but emulated", "radiosoundgroup");
	optSoundDisabledEmu->setId("sndDisEmu");
	optSoundDisabledEmu->addActionListener(sound_action_listener);

	optSoundEmulated = new gcn::RadioButton("Enabled", "radiosoundgroup");
	optSoundEmulated->setId("sndEmulate");
	optSoundEmulated->addActionListener(sound_action_listener);

	chkAutoSwitching = new gcn::CheckBox("Automatic switching");
	chkAutoSwitching->setId("chkAutoSwitching");
	chkAutoSwitching->addActionListener(sound_action_listener);

	lblFrequency = new gcn::Label("Frequency:");
	lblFrequency->setAlignment(gcn::Graphics::RIGHT);
	cboFrequency = new gcn::DropDown(&frequency_type_list);
	cboFrequency->setSize(90, cboFrequency->getHeight());
	cboFrequency->setBaseColor(gui_baseCol);
	cboFrequency->setBackgroundColor(colTextboxBackground);
	cboFrequency->setId("cboFrequency");
	cboFrequency->addActionListener(sound_action_listener);

	lblSwapChannels = new gcn::Label("Swap channels:");
	lblSwapChannels->setAlignment(gcn::Graphics::RIGHT);
	cboSwapChannels = new gcn::DropDown(&swap_channels_list);
	cboSwapChannels->setSize(95, cboSwapChannels->getHeight());
	cboSwapChannels->setBaseColor(gui_baseCol);
	cboSwapChannels->setBackgroundColor(colTextboxBackground);
	cboSwapChannels->setId("cboSwapChannels");
	cboSwapChannels->addActionListener(sound_action_listener);

	lblChannelMode = new gcn::Label("Channel mode:");
	lblChannelMode->setAlignment(gcn::Graphics::RIGHT);
	cboChannelMode = new gcn::DropDown(&channel_mode_list);
	cboChannelMode->setSize(200, cboChannelMode->getHeight());
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
	lblCDVolInfo = new gcn::Label("100 %");

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

	lblMIDIVol = new gcn::Label("MIDI Volume:");
	lblMIDIVol->setAlignment(gcn::Graphics::RIGHT);
	sldMIDIVol = new gcn::Slider(0, 100);
	sldMIDIVol->setSize(150, SLIDER_HEIGHT);
	sldMIDIVol->setBaseColor(gui_baseCol);
	sldMIDIVol->setMarkerLength(20);
	sldMIDIVol->setStepLength(10);
	sldMIDIVol->setId("sldMIDIVol");
	sldMIDIVol->addActionListener(sound_action_listener);
	lblMIDIVolInfo = new gcn::Label("100 %");

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
	sldSoundBufferSize->setStepLength(1);
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
	grpSound->add(chkAutoSwitching, optSoundDisabled->getY(), 100);
	grpSound->setMovable(false);
	grpSound->setSize(optSoundDisabledEmu->getWidth() + DISTANCE_BORDER + 10, chkAutoSwitching->getY() + chkAutoSwitching->getHeight() + DISTANCE_NEXT_Y * 3);
	grpSound->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpSound->setBaseColor(gui_baseCol);

	grpVolume = new gcn::Window("Volume");
	grpVolume->add(lblPaulaVol, 10, 10);
	grpVolume->add(sldPaulaVol, lblPaulaVol->getX() + lblPaulaVol->getWidth() + 8, lblPaulaVol->getY());
	grpVolume->add(lblPaulaVolInfo, sldPaulaVol->getX() + sldPaulaVol->getWidth() + 10, sldPaulaVol->getY());
	grpVolume->add(lblCDVol, lblPaulaVol->getX(), lblPaulaVol->getY() + lblPaulaVol->getHeight() + DISTANCE_NEXT_Y);
	grpVolume->add(sldCDVol, sldPaulaVol->getX(), lblCDVol->getY());
	grpVolume->add(lblCDVolInfo, sldCDVol->getX() + sldCDVol->getWidth() + 8, sldCDVol->getY());
	grpVolume->add(lblAHIVol, lblPaulaVol->getX(), lblCDVol->getY() + lblCDVol->getHeight() + DISTANCE_NEXT_Y);
	grpVolume->add(sldAHIVol, sldPaulaVol->getX(), lblAHIVol->getY());
	grpVolume->add(lblAHIVolInfo, sldAHIVol->getX() + sldAHIVol->getWidth() + 8, sldAHIVol->getY());
	grpVolume->add(lblMIDIVol, lblPaulaVol->getX(), lblAHIVol->getY() + lblAHIVol->getHeight() + DISTANCE_NEXT_Y);
	grpVolume->add(sldMIDIVol, sldPaulaVol->getX(), lblMIDIVol->getY());
	grpVolume->add(lblMIDIVolInfo, sldMIDIVol->getX() + sldMIDIVol->getWidth() + 8, sldMIDIVol->getY());
	grpVolume->setMovable(false);
	grpVolume->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2 - grpSound->getWidth() - DISTANCE_NEXT_X, grpSound->getHeight());
	grpVolume->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpVolume->setBaseColor(gui_baseCol);

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
	category.panel->add(cboSoundcard, DISTANCE_BORDER, posY);
	posY += cboSoundcard->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(grpSound, DISTANCE_BORDER, posY);
	category.panel->add(grpVolume, grpSound->getX() + grpSound->getWidth() + DISTANCE_NEXT_X, posY);
	posY += grpSound->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblChannelMode, DISTANCE_BORDER, posY);
	category.panel->add(cboChannelMode, lblChannelMode->getX(), lblChannelMode->getY() + lblChannelMode->getHeight() + 10);
	posY = cboChannelMode->getY() + cboChannelMode->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblFrequency, lblChannelMode->getX(), posY);
	category.panel->add(cboFrequency, lblChannelMode->getX(), lblFrequency->getY() + lblFrequency->getHeight() + 10);
	category.panel->add(lblSwapChannels, cboFrequency->getX() + cboFrequency->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(cboSwapChannels, cboFrequency->getX() + cboFrequency->getWidth() + DISTANCE_NEXT_X, cboFrequency->getY());
	category.panel->add(lblSeparation, cboChannelMode->getX() + cboChannelMode->getWidth() + DISTANCE_NEXT_X * 2, lblChannelMode->getY());
	category.panel->add(cboSeparation, lblSeparation->getX(), lblSeparation->getY() + lblSeparation->getHeight() + 10);
	category.panel->add(lblStereoDelay, cboSeparation->getX(), lblFrequency->getY());
	category.panel->add(cboStereoDelay, lblStereoDelay->getX(), cboFrequency->getY());
	category.panel->add(lblInterpolation, cboSeparation->getX() + cboSeparation->getWidth() + DISTANCE_NEXT_X * 2, lblSeparation->getY());
	category.panel->add(cboInterpolation, lblInterpolation->getX(), lblInterpolation->getY() + lblInterpolation->getHeight() + 10);
	category.panel->add(lblFilter, lblInterpolation->getX(), lblFrequency->getY());
	category.panel->add(cboFilter, cboInterpolation->getX(), cboFrequency->getY());

	posY = cboFrequency->getY() + cboFrequency->getHeight() + DISTANCE_NEXT_Y * 2;
	category.panel->add(grpFloppySound, DISTANCE_BORDER, posY);
	category.panel->add(grpSoundBufferSize, grpFloppySound->getX() + grpFloppySound->getWidth() + DISTANCE_NEXT_X, posY);

	RefreshPanelSound();
}


void ExitPanelSound()
{
	delete lblSwapChannels;
	delete cboSwapChannels;
	delete cboSoundcard;
	delete optSoundDisabled;
	delete optSoundDisabledEmu;
	delete optSoundEmulated;
	delete chkAutoSwitching;
	delete grpSound;
	delete lblPaulaVol;
	delete lblPaulaVolInfo;
	delete sldPaulaVol;
	delete lblCDVol;
	delete lblCDVolInfo;
	delete sldCDVol;
	delete lblAHIVol;
	delete lblAHIVolInfo;
	delete sldAHIVol;
	delete lblMIDIVol;
	delete sldMIDIVol;
	delete lblMIDIVolInfo;
	delete grpVolume;
	delete lblChannelMode;
	delete cboChannelMode;
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

	if (numdevs == 0)
	{
		cboSoundcard->setEnabled(false);
	}
	else
	{
		cboSoundcard->setEnabled(changed_prefs.produce_sound);
		cboSoundcard->setSelected(changed_prefs.soundcard);
	}

	switch (changed_prefs.produce_sound)
	{
	case 0:
		optSoundDisabled->setSelected(true);
		break;
	case 1:
		optSoundDisabledEmu->setSelected(true);
		break;
	case 2:
	case 3:
		optSoundEmulated->setSelected(true);
		break;
	default:
		break;
	}

	chkAutoSwitching->setSelected(changed_prefs.sound_auto);
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

	if (changed_prefs.sound_stereo_swap_paula || changed_prefs.sound_stereo_swap_ahi)
	{
		if (changed_prefs.sound_stereo_swap_paula && changed_prefs.sound_stereo_swap_ahi)
		{
			cboSwapChannels->setSelected(3);
		}
		else if (changed_prefs.sound_stereo_swap_ahi)
		{
			cboSwapChannels->setSelected(2);
		}
		else if (changed_prefs.sound_stereo_swap_paula)
		{
			cboSwapChannels->setSelected(1);
		}
	}
	else
	{
		cboSwapChannels->setSelected(0);
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
	snprintf(tmp, sizeof tmp - 1, "%d %%", 100 - changed_prefs.sound_volume_cd);
	lblCDVolInfo->setCaption(tmp);

	sldAHIVol->setValue(100 - changed_prefs.sound_volume_board);
	snprintf(tmp, sizeof tmp - 1, "%d %%", 100 - changed_prefs.sound_volume_board);
	lblAHIVolInfo->setCaption(tmp);

	sldMIDIVol->setValue(100 - changed_prefs.sound_volume_midi);
	snprintf(tmp, sizeof tmp - 1, "%d %%", 100 - changed_prefs.sound_volume_midi);
	lblMIDIVolInfo->setCaption(tmp);

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

	lblCDVol->setEnabled(enabled);
	lblCDVolInfo->setEnabled(enabled);
	sldCDVol->setEnabled(enabled);

	lblAHIVol->setEnabled(enabled);
	lblAHIVolInfo->setEnabled(enabled);
	sldAHIVol->setEnabled(enabled);

	lblMIDIVol->setEnabled(enabled);
	sldMIDIVol->setEnabled(enabled);
	lblMIDIVolInfo->setEnabled(enabled);

	sldFloppySoundDisk->setEnabled(chkFloppySound->isSelected());
	lblFloppySoundDisk->setEnabled(chkFloppySound->isSelected());
	lblFloppySoundDiskInfo->setEnabled(chkFloppySound->isSelected());
	sldFloppySoundEmpty->setEnabled(chkFloppySound->isSelected());
	lblFloppySoundEmpty->setEnabled(chkFloppySound->isSelected());
	lblFloppySoundEmptyInfo->setEnabled(chkFloppySound->isSelected());
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
