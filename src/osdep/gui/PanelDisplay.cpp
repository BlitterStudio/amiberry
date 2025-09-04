#include <guisan.hpp>
#include <iomanip>
#include <stdexcept>
#include <guisan/sdl.hpp>
#include <sstream>
#include <cmath> 
#include "SelectorEntry.hpp"
#include "StringListModel.h"

#include "sysdeps.h"
#include "options.h"
#include "uae.h"
#include "custom.h"
#include "xwin.h"
#include "gui_handling.h"
#include "drawing.h"

const int amigawidth_values[] = { 640, 704, 720, 754 };
const int amigaheight_values[] = { 400, 480, 512, 568, 576 };

enum
{
	AMIGAWIDTH_COUNT = 4,
	AMIGAHEIGHT_COUNT = 5
};

static std::vector<std::string> fullscreen_resolutions = {};
static gcn::StringListModel fullscreen_resolutions_list(fullscreen_resolutions);

static std::vector<std::string> refresh_rates = {};
static gcn::StringListModel refresh_rates_list(refresh_rates);

static const std::vector<std::string> fullscreen_modes = { "Windowed", "Fullscreen", "Full-window" };
static gcn::StringListModel fullscreen_modes_list(fullscreen_modes);

static const std::vector<std::string> resolution = { "LowRes", "HighRes (normal)", "SuperHighRes" };
static gcn::StringListModel resolution_list(resolution);

static const std::vector<std::string> scaling_method = { "Auto", "Pixelated", "Smooth", "Integer" };
static gcn::StringListModel scaling_method_list(scaling_method);

static const std::vector<std::string> res_autoswitch = { "Disabled", "Always On", "10%", "33%", "66%" };
static gcn::StringListModel res_autoswitch_list(res_autoswitch);

static const std::vector<std::string> vsync_options = { "-", "Lagless", "Lagless 50/60Hz", "Standard", "Standard 50/60Hz" };
static gcn::StringListModel vsync_options_list(vsync_options);

static std::vector<std::string> fps_options = { "PAL", "NTSC"};
static gcn::StringListModel fps_options_list(fps_options);

static gcn::Window* grpAmigaScreen;
static gcn::CheckBox* chkManualCrop;
static gcn::Label* lblAmigaWidth;
static gcn::TextField* txtAmigaWidth;
static gcn::Slider* sldAmigaWidth;

static gcn::Label* lblAmigaHeight;
static gcn::TextField* txtAmigaHeight;
static gcn::Slider* sldAmigaHeight;

static gcn::CheckBox* chkAutoCrop;
static gcn::CheckBox* chkBorderless;

static gcn::Label* lblHOffset;
static gcn::Slider* sldHOffset;
static gcn::Label* lblHOffsetValue;
static gcn::Label* lblVOffset;
static gcn::Slider* sldVOffset;
static gcn::Label* lblVOffsetValue;

static gcn::Label* lblScreenmode;
static gcn::DropDown* cboScreenmode;
static gcn::Label* lblFullscreen;
static gcn::DropDown* cboFullscreen;
static gcn::DropDown* cboRefreshRate;

static gcn::Label* lblVSyncNative;
static gcn::DropDown* cboVSyncNative;
static gcn::Label* lblVSyncRtg;
static gcn::DropDown* cboVSyncRtg;

static gcn::Window* grpCentering;
static gcn::CheckBox* chkHorizontal;
static gcn::CheckBox* chkVertical;
static gcn::CheckBox* chkFlickerFixer;

static gcn::Window* grpLineMode;
static gcn::RadioButton* optSingle;
static gcn::RadioButton* optDouble;
static gcn::RadioButton* optScanlines;
static gcn::RadioButton* optDouble2;
static gcn::RadioButton* optDouble3;

static gcn::Window* grpILineMode;
static gcn::RadioButton* optISingle;
static gcn::RadioButton* optIDouble;
static gcn::RadioButton* optIDouble2;
static gcn::RadioButton* optIDouble3;

static gcn::Label* lblScalingMethod;
static gcn::DropDown* cboScalingMethod;

static gcn::Label* lblResolution;
static gcn::DropDown* cboResolution;
static gcn::CheckBox* chkFilterLowRes;

static gcn::CheckBox* chkFrameskip;
static gcn::Slider* sldRefresh;
static gcn::Label* lblFrameRate;

static gcn::DropDown* cboFpsRate;
static gcn::CheckBox* chkFpsAdj;
static gcn::Slider* sldFpsAdj;
static gcn::TextField* txtFpsAdj;

static gcn::CheckBox* chkAspect;
static gcn::CheckBox* chkBlackerThanBlack;

static gcn::Label* lblBrightness;
static gcn::Slider* sldBrightness;
static gcn::Label* lblBrightnessValue;

static gcn::Label* lblResSwitch;
static gcn::DropDown* cboResSwitch;

static const int fakerefreshrates[] = { 50, 60, 100, 120, 0 };
struct storedrefreshrate
{
	int rate, type;
};
static struct storedrefreshrate storedrefreshrates[MAX_REFRESH_RATES + 4 + 1];

static void init_frequency_combo(int dmode)
{
	int i, j, freq;
	TCHAR hz[20], hz2[20];
	int index;
	MultiDisplay *md = getdisplay(&changed_prefs, 0);

	i = 0; index = 0;
	while (dmode >= 0 && (freq = md->DisplayModes[dmode].refresh[i]) > 0 && index < MAX_REFRESH_RATES) {
		storedrefreshrates[index].rate = freq;
		storedrefreshrates[index++].type = md->DisplayModes[dmode].refreshtype[i];
		i++;
	}
	if (changed_prefs.gfx_apmode[0].gfx_vsyncmode == 0 && changed_prefs.gfx_apmode[0].gfx_vsync) {
		i = 0;
		while ((freq = fakerefreshrates[i]) > 0 && index < MAX_REFRESH_RATES) {
			for (j = 0; j < index; j++) {
				if (storedrefreshrates[j].rate == freq)
					break;
			}
			if (j == index) {
				storedrefreshrates[index].rate = -freq;
				storedrefreshrates[index++].type = 0;
			}
			i++;
		}
	}
	storedrefreshrates[index].rate = 0;
	for (i = 0; i < index; i++) {
		for (j = i + 1; j < index; j++) {
			if (abs (storedrefreshrates[i].rate) >= abs (storedrefreshrates[j].rate)) {
				storedrefreshrate srr{};
				memcpy (&srr, &storedrefreshrates[i], sizeof (struct storedrefreshrate));
				memcpy (&storedrefreshrates[i], &storedrefreshrates[j], sizeof (struct storedrefreshrate));
				memcpy (&storedrefreshrates[j], &srr, sizeof (struct storedrefreshrate));
			}
		}
	}

	hz[0] = hz2[0] = 0;
	refresh_rates_list.clear();
	refresh_rates_list.add("Default");
	for (i = 0; i < index; i++) {
		bool lace = (storedrefreshrates[i].type & REFRESH_RATE_LACE) != 0;
		freq = storedrefreshrates[i].rate;
		if (freq < 0) {
			freq = -freq;
			_sntprintf (hz, sizeof hz, _T("(%dHz)"), freq);
		} else {
			_sntprintf (hz, sizeof hz, _T("%dHz"), freq);
		}
		if (freq == 50 || freq == 100 || (freq * 2 == 50 && lace))
			_tcscat (hz, _T(" PAL"));
		if (freq == 60 || freq == 120 || (freq * 2 == 60 && lace))
			_tcscat (hz, _T(" NTSC"));
		if (lace) {
			TCHAR tmp[10];
			_sntprintf (tmp, sizeof tmp, _T(" (%di)"), freq * 2);
			_tcscat (hz, tmp);
		}
		if (storedrefreshrates[i].type & REFRESH_RATE_RAW)
			_tcscat (hz, _T(" (*)"));
		if (abs (changed_prefs.gfx_apmode[0].gfx_refreshrate) == freq)
			_tcscpy (hz2, hz);
		refresh_rates_list.add(hz);
	}
	index = -1;
	if (hz2[0] >= 0)
	{
		for (int item = 0; item < refresh_rates_list.getNumberOfElements(); item++)
		{
			if (refresh_rates_list.getElementAt(item) == hz2)
			{
				index = item;
				cboRefreshRate->setSelected(item);
				break;
			}
		}
	}
	if (index == -1) {
		std::string default_refresh_rate = "Default";
		for (int item = 0; item < refresh_rates_list.getNumberOfElements(); item++)
		{
			if (refresh_rates_list.getElementAt(item) == default_refresh_rate)
			{
				cboRefreshRate->setSelected(item);
				break;
			}
		}
		changed_prefs.gfx_apmode[0].gfx_refreshrate = 0;
	}
}

static int display_mode_index (uae_u32 x, uae_u32 y, uae_u32 d)
{
	int i, j;
	MultiDisplay *md = getdisplay(&changed_prefs, 0);

	j = 0;
	for (i = 0; md->DisplayModes[i].inuse; i++) {
		if (md->DisplayModes[i].res.width == x &&
			md->DisplayModes[i].res.height == y)
			break;
		j++;
	}
	if (x == 0 && y == 0) {
		j = 0;
		for (i = 0; md->DisplayModes[i].inuse; i++) {
			if (md->DisplayModes[i].res.width == md->rect.w &&
				md->DisplayModes[i].res.height == md->rect.h)
				break;
			j++;
		}
	}
	if(!md->DisplayModes[i].inuse)
		j = -1;
	return j;
}

static int gui_display_depths[3];
static void init_display_mode ()
{
	int index;
	struct MultiDisplay *md = getdisplay(&changed_prefs, 0);
	struct monconfig *gm = &changed_prefs.gfx_monitor[0];

	if (gm->gfx_size_fs.special == WH_NATIVE) {
		int cnt = fullscreen_modes_list.getNumberOfElements();
		cboFullscreen->setSelected(cnt - 1);
		index = display_mode_index(gm->gfx_size_fs.width, gm->gfx_size_fs.height, 4);
	} else {
		index = display_mode_index(gm->gfx_size_fs.width, gm->gfx_size_fs.height, 4);
		if (index >= 0)
			cboFullscreen->setSelected(md->DisplayModes[index].residx);
		gm->gfx_size_fs.special = 0;
	}
	init_frequency_combo (index);
}


class AmigaScreenKeyListener : public gcn::KeyListener
{
public:
	void keyReleased(gcn::KeyEvent& keyEvent) override
	{
		if (keyEvent.getSource() == txtFpsAdj && chkFpsAdj->isSelected())
		{
			if (keyEvent.getKey().getValue() == gcn::Key::Enter)
			{
				const std::string tmp = txtFpsAdj->getText();
				if (!tmp.empty()) {
					TCHAR label[16];
					label[0] = 0;
					const auto& label_string = fps_options[cboFpsRate->getSelected()];
					strncpy(label, label_string.c_str(), sizeof(label) - 1);
					label[sizeof(label) - 1] = '\0';

					struct chipset_refresh* cr = nullptr;
					for (int i = 0; i < MAX_CHIPSET_REFRESH_TOTAL; i++) {
						cr = &changed_prefs.cr[i];
						if (!_tcscmp(label, cr->label) || (cr->label[0] == 0 && label[0] == ':' && _tstol(label + 1) == i)) {
							if (changed_prefs.cr_selected != i) {
								changed_prefs.cr_selected = i;
								chkFpsAdj->setSelected(cr->locked);
								sldFpsAdj->setEnabled(cr->locked);
								txtFpsAdj->setEnabled(cr->locked);
							}
							else {
								cr->locked = chkFpsAdj->isSelected();
								if (cr->locked) {
									cr->inuse = true;
								}
								else {
									// deactivate if plain non-customized PAL or NTSC
									if (!cr->commands[0] && !cr->filterprofile[0] && cr->resolution == 7 &&
										cr->horiz < 0 && cr->vert < 0 && cr->lace < 0 && cr->vsync < 0 && cr->framelength < 0 &&
										(cr == &changed_prefs.cr[CHIPSET_REFRESH_PAL] || cr == &changed_prefs.cr[CHIPSET_REFRESH_NTSC])) {
										cr->inuse = false;
									}
								}
							}
							break;
						}
					}
					try
					{
						double rate;
						std::stringstream ss(tmp);
						ss >> rate;
						if (ss.fail()) {
							throw std::invalid_argument("Invalid FPS argument in text box");
						}

						// Round the rate to 6 decimal places
						rate = std::round(rate * 1e6) / 1e6;

						if (cr != nullptr) {
							cr->rate = static_cast<float>(rate);
						}
					}
					catch (const std::invalid_argument&) 
					{
						write_log("Invalid FPS argument in text box, ignoring value\n");
					}
					catch (const std::out_of_range&) {
						write_log("Out of range FPS argument in text box, ignoring value\n");
					}

					if (cr != nullptr)
					{
						TCHAR buffer[20];
						_sntprintf(buffer, sizeof buffer, _T("%.6f"), cr->rate);
						txtFpsAdj->setText(std::string(buffer));
						sldFpsAdj->setValue(cr->rate);
					}
				}
			}
		}
	}
};

AmigaScreenKeyListener* amigaScreenKeyListener;

class AmigaScreenActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		auto source = actionEvent.getSource();

		changed_prefs.gfx_correct_aspect = chkAspect->isSelected();
		changed_prefs.gfx_lores_mode = chkFilterLowRes->isSelected() ? 1 : 0;
		changed_prefs.gfx_scandoubler = chkFlickerFixer->isSelected();
		changed_prefs.gfx_blackerthanblack = chkBlackerThanBlack->isSelected();
		//workprefs.gfx_autoresolution_vga = ischecked(hDlg, IDC_AUTORESOLUTIONVGA);
		//workprefs.gfx_grayscale = ischecked(hDlg, IDC_GRAYSCALE);
		//workprefs.gfx_monitorblankdelay = ischecked(hDlg, IDC_RESYNCBLANK) ? 1000 : 0;

		//int vres = changed_prefs.gfx_vresolution;
		//int viscan = changed_prefs.gfx_iscanlines;
		//int vpscan = changed_prefs.gfx_pscanlines;

		//changed_prefs.gfx_vresolution = (optDouble->isSelected() || optScanlines->isSelected() || optDouble2->isSelected() || optDouble3->isSelected()) ? VRES_DOUBLE : VRES_NONDOUBLE;
		//changed_prefs.gfx_iscanlines = 0;
		//changed_prefs.gfx_pscanlines = 0;
		//if (changed_prefs.gfx_vresolution >= VRES_DOUBLE) {
		//	if (optIDouble2->isSelected())
		//		changed_prefs.gfx_iscanlines = 1;
		//	if (optIDouble3->isSelected())
		//		changed_prefs.gfx_iscanlines = 2;
		//	if (optScanlines->isSelected())
		//		changed_prefs.gfx_pscanlines = 1;
		//	if (optDouble2->isSelected())
		//		changed_prefs.gfx_pscanlines = 2;
		//	if (optDouble3->isSelected())
		//		changed_prefs.gfx_pscanlines = 3;
		//}
		//if (vres != changed_prefs.gfx_vresolution || viscan != changed_prefs.gfx_iscanlines || vpscan != changed_prefs.gfx_pscanlines) {
		//	CheckRadioButton(hDlg, IDC_LM_NORMAL, IDC_LM_PDOUBLED3, IDC_LM_NORMAL + (changed_prefs.gfx_vresolution ? 1 : 0) + changed_prefs.gfx_pscanlines);
		//	CheckRadioButton(hDlg, IDC_LM_INORMAL, IDC_LM_IDOUBLED3, IDC_LM_INORMAL + (changed_prefs.gfx_iscanlines ? changed_prefs.gfx_iscanlines + 1 : (changed_prefs.gfx_vresolution ? 1 : 0)));
		//}

		//changed_prefs.gfx_apmode[0].gfx_backbuffers = xSendDlgItemMessage(hDlg, IDC_DISPLAY_BUFFERCNT, CB_GETCURSEL, 0, 0) + 1;

		if (source == chkFrameskip)
		{
			changed_prefs.gfx_framerate = chkFrameskip->isSelected() ? 2 : 1;
			sldRefresh->setEnabled(chkFrameskip->isSelected());
			sldRefresh->setValue(changed_prefs.gfx_framerate);
			lblFrameRate->setCaption(std::to_string(changed_prefs.gfx_framerate));
			lblFrameRate->adjustSize();
		}
		else if (source == sldRefresh)
			changed_prefs.gfx_framerate = static_cast<int>(sldRefresh->getValue());

		int i = cboVSyncNative->getSelected();
		int oldvsmode = changed_prefs.gfx_apmode[0].gfx_vsyncmode;
		int oldvs = changed_prefs.gfx_apmode[0].gfx_vsync;
		changed_prefs.gfx_apmode[0].gfx_vsync = 0;
		changed_prefs.gfx_apmode[0].gfx_vsyncmode = 0;
		if (i == 1) {
			changed_prefs.gfx_apmode[0].gfx_vsync = 1;
			changed_prefs.gfx_apmode[0].gfx_vsyncmode = 1;
		}
		else if (i == 2) {
			changed_prefs.gfx_apmode[0].gfx_vsync = 2;
			changed_prefs.gfx_apmode[0].gfx_vsyncmode = 1;
		}
		else if (i == 3) {
			changed_prefs.gfx_apmode[0].gfx_vsync = 1;
			changed_prefs.gfx_apmode[0].gfx_vsyncmode = 0;
		}
		else if (i == 4) {
			changed_prefs.gfx_apmode[0].gfx_vsync = 2;
			changed_prefs.gfx_apmode[0].gfx_vsyncmode = 0;
		}
		else if (i == 5) {
			changed_prefs.gfx_apmode[0].gfx_vsync = -1;
			changed_prefs.gfx_apmode[0].gfx_vsyncmode = 0;
		}

		i = cboVSyncRtg->getSelected();
		changed_prefs.gfx_apmode[1].gfx_vsync = 0;
		changed_prefs.gfx_apmode[1].gfx_vsyncmode = 0;
		if (i == 1) {
			changed_prefs.gfx_apmode[1].gfx_vsync = 1;
			changed_prefs.gfx_apmode[1].gfx_vsyncmode = 1;
		}
		else if (i == 2) {
			changed_prefs.gfx_apmode[1].gfx_vsync = -1;
			changed_prefs.gfx_apmode[1].gfx_vsyncmode = 0;
		}

		sldFpsAdj->setEnabled(chkFpsAdj->isSelected());
		txtFpsAdj->setEnabled(chkFpsAdj->isSelected());

		bool updaterate = false, updateslider = false;
		TCHAR label[16];
		label[0] = 0;
		const auto& label_string = fps_options[cboFpsRate->getSelected()];
		strncpy(label, label_string.c_str(), sizeof(label) - 1);
		label[sizeof(label) - 1] = '\0';

		struct chipset_refresh* cr = nullptr;
		for (i = 0; i < MAX_CHIPSET_REFRESH_TOTAL; i++) {
			cr = &changed_prefs.cr[i];
			if (!_tcscmp(label, cr->label) || (cr->label[0] == 0 && label[0] == ':' && _tstol(label + 1) == i)) {
				if (changed_prefs.cr_selected != i) {
					changed_prefs.cr_selected = i;
					updaterate = true;
					updateslider = true;
					chkFpsAdj->setSelected(cr->locked);
					sldFpsAdj->setEnabled(cr->locked);
					txtFpsAdj->setEnabled(cr->locked);
				} else {
					cr->locked = chkFpsAdj->isSelected();
					if (cr->locked) {
						cr->inuse = true;
					} else {
						// deactivate if plain non-customized PAL or NTSC
						if (!cr->commands[0] && !cr->filterprofile[0] && cr->resolution == 7 &&
							cr->horiz < 0 && cr->vert < 0 && cr->lace < 0 && cr->vsync < 0 && cr->framelength < 0 &&
							(cr == &changed_prefs.cr[CHIPSET_REFRESH_PAL] || cr == &changed_prefs.cr[CHIPSET_REFRESH_NTSC])) {
							cr->inuse = false;
						}
					}
				}
				break;
			}
		}
		if (cr->locked) {
			if (actionEvent.getSource() == sldFpsAdj) {
				i = static_cast<int>(sldFpsAdj->getValue());
				if (i != static_cast<int>(cr->rate))
					cr->rate = static_cast<float>(i);
				updaterate = true;
			}
		}
		else if (i == CHIPSET_REFRESH_PAL) {
			cr->rate = 50.0f;
			changed_prefs.ntscmode = false;
		}
		else if (i == CHIPSET_REFRESH_NTSC) {
			cr->rate = 60.0f;
			changed_prefs.ntscmode = true;
		}
		if (cr->rate > 0 && cr->rate < 1) {
			cr->rate = currprefs.ntscmode ? 60.0f : 50.0f;
			updaterate = true;
		}
		if (cr->rate > 300) {
			cr->rate = currprefs.ntscmode ? 60.0f : 50.0f;
			updaterate = true;
		}
		if (updaterate) {
			TCHAR buffer[20];
			_sntprintf(buffer, sizeof buffer, _T("%.6f"), cr->rate);
			txtFpsAdj->setText(std::string(buffer));
		}
		if (updateslider) {
			sldFpsAdj->setValue(cr->rate);
		}

		changed_prefs.gfx_xcenter = chkHorizontal->isSelected() ? 2 : 0;	// Smart centering
		changed_prefs.gfx_ycenter = chkVertical->isSelected() ? 2 : 0;		// Smart centering
		//workprefs.gfx_variable_sync = ischecked(hDlg, IDC_DISPLAY_VARSYNC) ? 1 : 0;
		//workprefs.gfx_windowed_resize = ischecked(hDlg, IDC_DISPLAY_RESIZE);

		int posn1 = cboResSwitch->getSelected();
		if (posn1 == 0)
			changed_prefs.gfx_autoresolution = 0;
		else if (posn1 == 1)
			changed_prefs.gfx_autoresolution = 1;
		else if (posn1 == 2)
			changed_prefs.gfx_autoresolution = 10;
		else if (posn1 == 3)
			changed_prefs.gfx_autoresolution = 33;
		else if (posn1 == 4)
			changed_prefs.gfx_autoresolution = 66;
		else
			changed_prefs.gfx_autoresolution = 100;

		int dmode = -1;
		bool native = false;
		struct MultiDisplay *md = getdisplay(&changed_prefs, 0);
		posn1 = cboFullscreen->getSelected();
		changed_prefs.gfx_monitor[0].gfx_size_fs.special = 0;
		for (dmode = 0; md->DisplayModes[dmode].inuse; dmode++) {
			if (md->DisplayModes[dmode].residx == posn1)
				break;
		}
		if (!md->DisplayModes[dmode].inuse) {
			for (dmode = 0; md->DisplayModes[dmode].inuse; dmode++) {
				if (md->DisplayModes[dmode].res.width == md->rect.w &&
					md->DisplayModes[dmode].res.height == md->rect.h)
				{
					changed_prefs.gfx_monitor[0].gfx_size_fs.special = WH_NATIVE;
					break;
				}
			}
			if (!md->DisplayModes[dmode].inuse)
				dmode = -1;
		} else {
			int i = dmode;
			if (md->DisplayModes[dmode].residx != posn1)
				dmode = i;
		}


		if (oldvsmode != changed_prefs.gfx_apmode[0].gfx_vsyncmode || oldvs != changed_prefs.gfx_apmode[0].gfx_vsync)
			init_frequency_combo(dmode);

		if (source == cboResolution)
			changed_prefs.gfx_resolution = cboResolution->getSelected();

		//TODO
		//else if (source == cboOverscan)
		//	changed_prefs.gfx_overscanmode = cboOverscan->getSelected();

		else if (source == cboFullscreen)
		{
			changed_prefs.gfx_monitor[0].gfx_size_fs.width = md->DisplayModes[dmode].res.width;
			changed_prefs.gfx_monitor[0].gfx_size_fs.height = md->DisplayModes[dmode].res.height;
			/* Set the Int boxes */
			//SetDlgItemInt(hDlg, IDC_XSIZE, changed_prefs.gfx_monitor[0].gfx_size_win.width, FALSE);
			//SetDlgItemInt(hDlg, IDC_YSIZE, changed_prefs.gfx_monitor[0].gfx_size_win.height, FALSE);
			init_display_mode();
		}
		else if (source == cboRefreshRate)
		{
			posn1 = cboRefreshRate->getSelected();
			if (posn1 == 0) {
				changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate = 0;
				changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_interlaced = dmode >= 0 && md->DisplayModes[dmode].lace;
			}
			else {
				posn1--;
				changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate = storedrefreshrates[posn1].rate;
				changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_interlaced = (storedrefreshrates[posn1].type & REFRESH_RATE_LACE) != 0;
			}
		}

		if (source == chkManualCrop)
		{
			changed_prefs.gfx_manual_crop = chkManualCrop->isSelected();
			if (changed_prefs.gfx_auto_crop)
				changed_prefs.gfx_auto_crop = false;
		}
		else if (source == sldAmigaWidth)
		{
			const int new_width = amigawidth_values[static_cast<int>(sldAmigaWidth->getValue())];
			const int new_x = ((AMIGA_WIDTH_MAX << changed_prefs.gfx_resolution) - new_width) / 2;

			changed_prefs.gfx_manual_crop_width = new_width;
			changed_prefs.gfx_horizontal_offset = new_x;
		}
		else if (source == sldAmigaHeight)
		{
			const int new_height = amigaheight_values[static_cast<int>(sldAmigaHeight->getValue())];
			const int new_y = ((AMIGA_HEIGHT_MAX << changed_prefs.gfx_vresolution) - new_height) / 2;

			changed_prefs.gfx_manual_crop_height = new_height;
			changed_prefs.gfx_vertical_offset = new_y;
		}

		else if (source == chkAutoCrop)
		{
			changed_prefs.gfx_auto_crop = chkAutoCrop->isSelected();
			if (changed_prefs.gfx_manual_crop)
				changed_prefs.gfx_manual_crop = false;
		}

		else if (source == chkBorderless)
			changed_prefs.borderless = chkBorderless->isSelected();

		else if (source == sldHOffset)
		{
			changed_prefs.gfx_horizontal_offset = static_cast<int>(sldHOffset->getValue());
			lblHOffsetValue->setCaption(std::to_string(changed_prefs.gfx_horizontal_offset));
			lblHOffsetValue->adjustSize();
		}
		else if (source == sldVOffset)
		{
			changed_prefs.gfx_vertical_offset = static_cast<int>(sldVOffset->getValue());
			lblVOffsetValue->setCaption(std::to_string(changed_prefs.gfx_vertical_offset));
			lblVOffsetValue->adjustSize();
		}

		else if (source == sldBrightness)
		{
			changed_prefs.gfx_luminance = static_cast<int>(sldBrightness->getValue());
			lblBrightnessValue->setCaption(std::to_string(changed_prefs.gfx_luminance));
			lblBrightnessValue->adjustSize();
		}

		else if (source == cboScreenmode)
		{
			if (cboScreenmode->getSelected() == 0)
			{
				changed_prefs.gfx_apmode[0].gfx_fullscreen = GFX_WINDOW;
				changed_prefs.gfx_apmode[1].gfx_fullscreen = GFX_WINDOW;
			}
			else if (cboScreenmode->getSelected() == 1)
			{
				changed_prefs.gfx_apmode[0].gfx_fullscreen = GFX_FULLSCREEN;
				changed_prefs.gfx_apmode[1].gfx_fullscreen = GFX_FULLSCREEN;
			}
			else if (cboScreenmode->getSelected() == 2)
			{
				changed_prefs.gfx_apmode[0].gfx_fullscreen = GFX_FULLWINDOW;
				changed_prefs.gfx_apmode[1].gfx_fullscreen = GFX_FULLWINDOW;
			}
		}

		RefreshPanelDisplay();
		RefreshPanelQuickstart();
	}
};

AmigaScreenActionListener* amigaScreenActionListener;

class ScalingMethodActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cboScalingMethod)
		{
			changed_prefs.scaling_method = cboScalingMethod->getSelected() - 1;
		}
	}
};

static ScalingMethodActionListener* scalingMethodActionListener;

static void disable_idouble_modes()
{
	optISingle->setEnabled(true);
	optISingle->setSelected(true);
	changed_prefs.gfx_iscanlines = 0;
	
	optIDouble->setEnabled(false);
	optIDouble2->setEnabled(false);
	optIDouble3->setEnabled(false);
}

static void enable_idouble_modes()
{
	if (optISingle->isSelected())
	{
		optISingle->setSelected(false);
		optIDouble->setSelected(true);
	}
	optISingle->setEnabled(false);
	
	optIDouble->setEnabled(true);
	optIDouble2->setEnabled(true);
	optIDouble3->setEnabled(true);
}

class LineModeActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& action_event) override
	{
		if (action_event.getSource() == optSingle)
		{
			changed_prefs.gfx_vresolution = VRES_NONDOUBLE;
			changed_prefs.gfx_pscanlines = 0;

			disable_idouble_modes();
		}
		else if (action_event.getSource() == optDouble)
		{
			changed_prefs.gfx_vresolution = VRES_DOUBLE;
			changed_prefs.gfx_pscanlines = 0;
			
			enable_idouble_modes();
		}
		else if (action_event.getSource() == optScanlines)
		{
			changed_prefs.gfx_vresolution = VRES_DOUBLE;
			changed_prefs.gfx_pscanlines = 1;

			enable_idouble_modes();
		}
		else if (action_event.getSource() == optDouble2)
		{
			changed_prefs.gfx_vresolution = VRES_DOUBLE;
			changed_prefs.gfx_pscanlines = 2;

			enable_idouble_modes();
		}
		else if (action_event.getSource() == optDouble3)
		{
			changed_prefs.gfx_vresolution = VRES_DOUBLE;
			changed_prefs.gfx_pscanlines = 3;

			enable_idouble_modes();
		}
		else if (action_event.getSource() == optISingle || action_event.getSource() == optIDouble)
		{
			changed_prefs.gfx_iscanlines = 0;
		}
		else if (action_event.getSource() == optIDouble2)
		{
			changed_prefs.gfx_iscanlines = 1;
		}
		else if (action_event.getSource() == optIDouble3)
		{
			changed_prefs.gfx_iscanlines = 2;
		}

		RefreshPanelDisplay();
	}
};

static LineModeActionListener* lineModeActionListener;

void InitPanelDisplay(const config_category& category)
{
	amigaScreenActionListener = new AmigaScreenActionListener();
	amigaScreenKeyListener = new AmigaScreenKeyListener();
	scalingMethodActionListener = new ScalingMethodActionListener();
	lineModeActionListener = new LineModeActionListener();
	
	int posY = DISTANCE_BORDER;

	int i, idx;
	TCHAR tmp[MAX_DPATH];
	struct MultiDisplay *md = getdisplay(&changed_prefs, 0);

	idx = -1;
	fullscreen_resolutions_list.clear();
	for (i = 0; md->DisplayModes[i].inuse; i++)
	{
		if (md->DisplayModes[i].residx != idx) {
			_sntprintf (tmp, sizeof tmp, _T("%dx%d%s"), md->DisplayModes[i].res.width, md->DisplayModes[i].res.height, md->DisplayModes[i].lace ? _T("i") : _T(""));
			if (md->DisplayModes[i].rawmode)
				_tcscat (tmp, _T(" (*)"));
			fullscreen_resolutions_list.add(tmp);
			idx = md->DisplayModes[i].residx;
		}
	}
	fullscreen_resolutions_list.add("Native");

	lblFullscreen = new gcn::Label("Fullscreen:");
	lblFullscreen->setAlignment(gcn::Graphics::Left);
	cboFullscreen = new gcn::DropDown(&fullscreen_resolutions_list);
	cboFullscreen->setSize(150, cboFullscreen->getHeight());
	cboFullscreen->setBaseColor(gui_base_color);
	cboFullscreen->setBackgroundColor(gui_background_color);
	cboFullscreen->setForegroundColor(gui_foreground_color);
	cboFullscreen->setSelectionColor(gui_selection_color);
	cboFullscreen->setId("cboFullscreen");
	cboFullscreen->addActionListener(amigaScreenActionListener);

	cboRefreshRate = new gcn::DropDown(&refresh_rates_list);
	cboRefreshRate->setSize(80, cboRefreshRate->getHeight());
	cboRefreshRate->setBaseColor(gui_base_color);
	cboRefreshRate->setBackgroundColor(gui_background_color);
	cboRefreshRate->setForegroundColor(gui_foreground_color);
	cboRefreshRate->setSelectionColor(gui_selection_color);
	cboRefreshRate->setId("cboRefreshRate");
	cboRefreshRate->addActionListener(amigaScreenActionListener);

	chkManualCrop = new gcn::CheckBox("Manual Crop");
	chkManualCrop->setId("chkManualCrop");
	chkManualCrop->setBaseColor(gui_base_color);
	chkManualCrop->setBackgroundColor(gui_background_color);
	chkManualCrop->setForegroundColor(gui_foreground_color);
	chkManualCrop->addActionListener(amigaScreenActionListener);

	lblAmigaWidth = new gcn::Label("Width:");
	lblAmigaWidth->setAlignment(gcn::Graphics::Left);
	sldAmigaWidth = new gcn::Slider(0, AMIGAWIDTH_COUNT - 1);
	sldAmigaWidth->setSize(180, SLIDER_HEIGHT);
	sldAmigaWidth->setBaseColor(gui_base_color);
	sldAmigaWidth->setBackgroundColor(gui_background_color);
	sldAmigaWidth->setForegroundColor(gui_foreground_color);
	sldAmigaWidth->setMarkerLength(20);
	sldAmigaWidth->setStepLength(1);
	sldAmigaWidth->setId("sldWidth");
	sldAmigaWidth->addActionListener(amigaScreenActionListener);

	txtAmigaWidth = new gcn::TextField();
	txtAmigaWidth->setSize(35, TEXTFIELD_HEIGHT);
	txtAmigaWidth->setBaseColor(gui_base_color);
	txtAmigaWidth->setBackgroundColor(gui_background_color);
	txtAmigaWidth->setForegroundColor(gui_foreground_color);
	txtAmigaWidth->addActionListener(amigaScreenActionListener);

	lblAmigaHeight = new gcn::Label("Height:");
	lblAmigaHeight->setAlignment(gcn::Graphics::Left);
	sldAmigaHeight = new gcn::Slider(0, AMIGAHEIGHT_COUNT - 1);
	sldAmigaHeight->setSize(180, SLIDER_HEIGHT);
	sldAmigaHeight->setBaseColor(gui_base_color);
	sldAmigaHeight->setBackgroundColor(gui_background_color);
	sldAmigaHeight->setForegroundColor(gui_foreground_color);
	sldAmigaHeight->setMarkerLength(20);
	sldAmigaHeight->setStepLength(1);
	sldAmigaHeight->setId("sldHeight");
	sldAmigaHeight->addActionListener(amigaScreenActionListener);

	txtAmigaHeight = new gcn::TextField();
	txtAmigaHeight->setSize(35, TEXTFIELD_HEIGHT);
	txtAmigaHeight->setBaseColor(gui_base_color);
	txtAmigaHeight->setBackgroundColor(gui_background_color);
	txtAmigaHeight->setForegroundColor(gui_foreground_color);
	txtAmigaHeight->addActionListener(amigaScreenActionListener);

	chkAutoCrop = new gcn::CheckBox("Auto Crop");
	chkAutoCrop->setId("chkAutoCrop");
	chkAutoCrop->setBaseColor(gui_base_color);
	chkAutoCrop->setBackgroundColor(gui_background_color);
	chkAutoCrop->setForegroundColor(gui_foreground_color);
	chkAutoCrop->addActionListener(amigaScreenActionListener);

	chkBorderless = new gcn::CheckBox("Borderless");
	chkBorderless->setId("chkBorderless");
	chkBorderless->setBaseColor(gui_base_color);
	chkBorderless->setBackgroundColor(gui_background_color);
	chkBorderless->setForegroundColor(gui_foreground_color);
	chkBorderless->addActionListener(amigaScreenActionListener);

	lblVSyncNative = new gcn::Label("VSync Native:");
	lblVSyncNative->setAlignment(gcn::Graphics::Left);
	cboVSyncNative = new gcn::DropDown(&vsync_options_list);
	cboVSyncNative->setSize(150, cboVSyncNative->getHeight());
	cboVSyncNative->setBaseColor(gui_base_color);
	cboVSyncNative->setBackgroundColor(gui_background_color);
	cboVSyncNative->setForegroundColor(gui_foreground_color);
	cboVSyncNative->setSelectionColor(gui_selection_color);
	cboVSyncNative->setId("cboVSyncNative");
	cboVSyncNative->addActionListener(amigaScreenActionListener);

	lblVSyncRtg = new gcn::Label("VSync RTG:");
	lblVSyncRtg->setAlignment(gcn::Graphics::Left);
	cboVSyncRtg = new gcn::DropDown(&vsync_options_list);
	cboVSyncRtg->setSize(150, cboVSyncRtg->getHeight());
	cboVSyncRtg->setBaseColor(gui_base_color);
	cboVSyncRtg->setBackgroundColor(gui_background_color);
	cboVSyncRtg->setForegroundColor(gui_foreground_color);
	cboVSyncRtg->setSelectionColor(gui_selection_color);
	cboVSyncRtg->setId("cboVSyncRtg");
	cboVSyncRtg->addActionListener(amigaScreenActionListener);

	lblHOffset = new gcn::Label("H. Offset:");
	lblHOffset->setAlignment(gcn::Graphics::Left);
	sldHOffset = new gcn::Slider(-80, 80);
	sldHOffset->setSize(200, SLIDER_HEIGHT);
	sldHOffset->setBaseColor(gui_base_color);
	sldHOffset->setBackgroundColor(gui_background_color);
	sldHOffset->setForegroundColor(gui_foreground_color);
	sldHOffset->setMarkerLength(20);
	sldHOffset->setStepLength(1);
	sldHOffset->setId("sldHOffset");
	sldHOffset->addActionListener(amigaScreenActionListener);
	lblHOffsetValue = new gcn::Label("-60");
	lblHOffsetValue->setAlignment(gcn::Graphics::Left);

	lblVOffset = new gcn::Label("V. Offset:");
	lblVOffset->setAlignment(gcn::Graphics::Left);
	sldVOffset = new gcn::Slider(-80, 80);
	sldVOffset->setSize(200, SLIDER_HEIGHT);
	sldVOffset->setBaseColor(gui_base_color);
	sldVOffset->setBackgroundColor(gui_background_color);
	sldVOffset->setForegroundColor(gui_foreground_color);
	sldVOffset->setMarkerLength(20);
	sldVOffset->setStepLength(1);
	sldVOffset->setId("sldVOffset");
	sldVOffset->addActionListener(amigaScreenActionListener);
	lblVOffsetValue = new gcn::Label("-20");
	lblVOffsetValue->setAlignment(gcn::Graphics::Left);

	chkHorizontal = new gcn::CheckBox("Horizontal");
	chkHorizontal->setId("chkHorizontal");
	chkHorizontal->setBaseColor(gui_base_color);
	chkHorizontal->setBackgroundColor(gui_background_color);
	chkHorizontal->setForegroundColor(gui_foreground_color);
	chkHorizontal->addActionListener(amigaScreenActionListener);
	chkVertical = new gcn::CheckBox("Vertical");
	chkVertical->setId("chkVertical");
	chkVertical->setBaseColor(gui_base_color);
	chkVertical->setBackgroundColor(gui_background_color);
	chkVertical->setForegroundColor(gui_foreground_color);
	chkVertical->addActionListener(amigaScreenActionListener);

	chkFlickerFixer = new gcn::CheckBox("Remove interlace artifacts");
	chkFlickerFixer->setId("chkFlickerFixer");
	chkFlickerFixer->setBaseColor(gui_base_color);
	chkFlickerFixer->setBackgroundColor(gui_background_color);
	chkFlickerFixer->setForegroundColor(gui_foreground_color);
	chkFlickerFixer->addActionListener(amigaScreenActionListener);

	chkFilterLowRes = new gcn::CheckBox("Filtered Low Res");
	chkFilterLowRes->setId("chkFilterLowRes");
	chkFilterLowRes->setBaseColor(gui_base_color);
	chkFilterLowRes->setBackgroundColor(gui_background_color);
	chkFilterLowRes->setForegroundColor(gui_foreground_color);
	chkFilterLowRes->addActionListener(amigaScreenActionListener);

	chkBlackerThanBlack = new gcn::CheckBox("Blacker than black");
	chkBlackerThanBlack->setId("chkBlackerThanBlack");
	chkBlackerThanBlack->setBaseColor(gui_base_color);
	chkBlackerThanBlack->setBackgroundColor(gui_background_color);
	chkBlackerThanBlack->setForegroundColor(gui_foreground_color);
	chkBlackerThanBlack->addActionListener(amigaScreenActionListener);
	
	chkAspect = new gcn::CheckBox("Correct Aspect Ratio");
	chkAspect->setId("chkAspect");
	chkAspect->setBaseColor(gui_base_color);
	chkAspect->setBackgroundColor(gui_background_color);
	chkAspect->setForegroundColor(gui_foreground_color);
	chkAspect->addActionListener(amigaScreenActionListener);

	chkFrameskip = new gcn::CheckBox("Refresh:");
	chkFrameskip->setId("chkFrameskip");
	chkFrameskip->setBaseColor(gui_base_color);
	chkFrameskip->setBackgroundColor(gui_background_color);
	chkFrameskip->setForegroundColor(gui_foreground_color);
	chkFrameskip->addActionListener(amigaScreenActionListener);

	sldRefresh = new gcn::Slider(1, 10);
	sldRefresh->setSize(100, SLIDER_HEIGHT);
	sldRefresh->setBaseColor(gui_base_color);
	sldRefresh->setBackgroundColor(gui_background_color);
	sldRefresh->setForegroundColor(gui_foreground_color);
	sldRefresh->setMarkerLength(20);
	sldRefresh->setStepLength(1);
	sldRefresh->setId("sldRefresh");
	sldRefresh->addActionListener(amigaScreenActionListener);
	lblFrameRate = new gcn::Label("50");
	lblFrameRate->setAlignment(gcn::Graphics::Left);

	chkFpsAdj = new gcn::CheckBox("FPS Adj:");
	chkFpsAdj->setId("chkFpsAdj");
	chkFpsAdj->setBaseColor(gui_base_color);
	chkFpsAdj->setBackgroundColor(gui_background_color);
	chkFpsAdj->setForegroundColor(gui_foreground_color);
	chkFpsAdj->addActionListener(amigaScreenActionListener);

	cboFpsRate = new gcn::DropDown(&fps_options_list);
	cboFpsRate->setSize(80, cboFpsRate->getHeight());
	cboFpsRate->setBaseColor(gui_base_color);
	cboFpsRate->setBackgroundColor(gui_background_color);
	cboFpsRate->setForegroundColor(gui_foreground_color);
	cboFpsRate->setSelectionColor(gui_selection_color);
	cboFpsRate->setId("cboFpsRate");
	cboFpsRate->addActionListener(amigaScreenActionListener);

	sldFpsAdj = new gcn::Slider(1, 99);
	sldFpsAdj->setSize(100, SLIDER_HEIGHT);
	sldFpsAdj->setBaseColor(gui_base_color);
	sldFpsAdj->setBackgroundColor(gui_background_color);
	sldFpsAdj->setForegroundColor(gui_foreground_color);
	sldFpsAdj->setMarkerLength(20);
	sldFpsAdj->setStepLength(1);
	sldFpsAdj->setId("sldFpsAdj");
	sldFpsAdj->addActionListener(amigaScreenActionListener);
	txtFpsAdj = new gcn::TextField();
	txtFpsAdj->setSize(80, TEXTFIELD_HEIGHT);
	txtFpsAdj->setBaseColor(gui_base_color);
	txtFpsAdj->setBackgroundColor(gui_background_color);
	txtFpsAdj->setForegroundColor(gui_foreground_color);
	txtFpsAdj->addKeyListener(amigaScreenKeyListener);

	lblBrightness = new gcn::Label("Brightness:");
	lblBrightness->setAlignment(gcn::Graphics::Left);
	sldBrightness = new gcn::Slider(-200, 200);
	sldBrightness->setSize(100, SLIDER_HEIGHT);
	sldBrightness->setBaseColor(gui_base_color);
	sldBrightness->setBackgroundColor(gui_background_color);
	sldBrightness->setForegroundColor(gui_foreground_color);
	sldBrightness->setMarkerLength(20);
	sldBrightness->setStepLength(1);
	sldBrightness->setId("sldBrightness");
	sldBrightness->addActionListener(amigaScreenActionListener);
	lblBrightnessValue = new gcn::Label("0.0");
	lblBrightnessValue->setAlignment(gcn::Graphics::Left);

	lblScreenmode = new gcn::Label("Screen mode:");
	lblScreenmode->setAlignment(gcn::Graphics::Right);
	cboScreenmode = new gcn::DropDown(&fullscreen_modes_list);
	cboScreenmode->setSize(150, cboScreenmode->getHeight());
	cboScreenmode->setBaseColor(gui_base_color);
	cboScreenmode->setBackgroundColor(gui_background_color);
	cboScreenmode->setForegroundColor(gui_foreground_color);
	cboScreenmode->setSelectionColor(gui_selection_color);
	cboScreenmode->setId("cboScreenmode");
	cboScreenmode->addActionListener(amigaScreenActionListener);

	lblResolution = new gcn::Label("Resolution:");
	lblResolution->setAlignment(gcn::Graphics::Right);
	cboResolution = new gcn::DropDown(&resolution_list);
	cboResolution->setSize(150, cboResolution->getHeight());
	cboResolution->setBaseColor(gui_base_color);
	cboResolution->setBackgroundColor(gui_background_color);
	cboResolution->setForegroundColor(gui_foreground_color);
	cboResolution->setSelectionColor(gui_selection_color);
	cboResolution->setId("cboResolution");
	cboResolution->addActionListener(amigaScreenActionListener);

	lblResSwitch = new gcn::Label("Res. autoswitch:");
	lblResSwitch->setAlignment(gcn::Graphics::Right);
	cboResSwitch = new gcn::DropDown(&res_autoswitch_list);
	cboResSwitch->setSize(150, cboResSwitch->getHeight());
	cboResSwitch->setBaseColor(gui_base_color);
	cboResSwitch->setBackgroundColor(gui_background_color);
	cboResSwitch->setForegroundColor(gui_foreground_color);
	cboResSwitch->setSelectionColor(gui_selection_color);
	cboResSwitch->setId("cboResSwitch");
	cboResSwitch->addActionListener(amigaScreenActionListener);
	
	grpAmigaScreen = new gcn::Window("Amiga Screen");
	grpAmigaScreen->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);

	grpAmigaScreen->add(lblFullscreen, DISTANCE_BORDER, posY);
	grpAmigaScreen->add(cboFullscreen, lblFullscreen->getX() + lblFullscreen->getWidth() + DISTANCE_NEXT_X, posY);
	grpAmigaScreen->add(cboRefreshRate, cboFullscreen->getX() + cboFullscreen->getWidth() + DISTANCE_NEXT_X, posY);
	posY += cboFullscreen->getHeight() + DISTANCE_NEXT_Y;

	grpAmigaScreen->add(lblScreenmode, DISTANCE_BORDER, posY);
	grpAmigaScreen->add(cboScreenmode, lblScreenmode->getX() + lblScreenmode->getWidth() + 8, posY);
	posY += cboScreenmode->getHeight() + DISTANCE_NEXT_Y;

	grpAmigaScreen->add(chkManualCrop, DISTANCE_BORDER, posY);
	posY += chkManualCrop->getHeight() + DISTANCE_NEXT_Y;

	grpAmigaScreen->add(lblAmigaWidth, DISTANCE_BORDER, posY);
	grpAmigaScreen->add(sldAmigaWidth, lblAmigaWidth->getX() + lblAmigaHeight->getWidth() + DISTANCE_NEXT_X, posY);
	grpAmigaScreen->add(txtAmigaWidth, sldAmigaWidth->getX() + sldAmigaWidth->getWidth() + DISTANCE_NEXT_X, posY);
	posY += sldAmigaWidth->getHeight() + DISTANCE_NEXT_Y;

	grpAmigaScreen->add(lblAmigaHeight, DISTANCE_BORDER, posY);
	grpAmigaScreen->add(sldAmigaHeight, lblAmigaHeight->getX() + lblAmigaHeight->getWidth() + DISTANCE_NEXT_X, posY);
	grpAmigaScreen->add(txtAmigaHeight, sldAmigaHeight->getX() + sldAmigaHeight->getWidth() + DISTANCE_NEXT_X,
		posY);
	
	posY += sldAmigaHeight->getHeight() + DISTANCE_NEXT_Y;
	grpAmigaScreen->add(chkAutoCrop, DISTANCE_BORDER, posY);
	grpAmigaScreen->add(chkBorderless, chkAutoCrop->getX() + chkAutoCrop->getWidth() + DISTANCE_NEXT_X, posY);
	posY += chkAutoCrop->getHeight() + DISTANCE_NEXT_Y;
	grpAmigaScreen->add(lblVSyncNative, DISTANCE_BORDER, posY);
	grpAmigaScreen->add(cboVSyncNative, lblVSyncNative->getX() + lblVSyncNative->getWidth() + 8, posY);
	posY += cboVSyncNative->getHeight() + DISTANCE_NEXT_Y;
	grpAmigaScreen->add(lblVSyncRtg, DISTANCE_BORDER, posY);
	grpAmigaScreen->add(cboVSyncRtg, cboVSyncNative->getX(), posY);
	posY += cboVSyncRtg->getHeight() + DISTANCE_NEXT_Y;
	grpAmigaScreen->add(lblHOffset, DISTANCE_BORDER, posY);
	grpAmigaScreen->add(sldHOffset, lblHOffset->getX() + lblHOffset->getWidth() + DISTANCE_NEXT_X, posY);
	grpAmigaScreen->add(lblHOffsetValue, sldHOffset->getX() + sldHOffset->getWidth() + 8, posY + 2);
	posY += sldVOffset->getHeight() + DISTANCE_NEXT_Y;
	grpAmigaScreen->add(lblVOffset, DISTANCE_BORDER, posY);
	grpAmigaScreen->add(sldVOffset, lblVOffset->getX() + lblVOffset->getWidth() + DISTANCE_NEXT_X, posY);
	grpAmigaScreen->add(lblVOffsetValue, sldVOffset->getX() + sldVOffset->getWidth() + 8, posY + 2);

	grpAmigaScreen->setMovable(false);
	grpAmigaScreen->setSize(cboVSyncNative->getX() + cboVSyncNative->getWidth() + DISTANCE_BORDER + DISTANCE_NEXT_X * 6, TITLEBAR_HEIGHT + lblVOffset->getY() + lblVOffset->getHeight() + DISTANCE_NEXT_Y);
	grpAmigaScreen->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpAmigaScreen->setBaseColor(gui_base_color);
	grpAmigaScreen->setForegroundColor(gui_foreground_color);
	category.panel->add(grpAmigaScreen);

	grpCentering = new gcn::Window("Centering");
	grpCentering->add(chkHorizontal, DISTANCE_BORDER, DISTANCE_BORDER);
	grpCentering->add(chkVertical, DISTANCE_BORDER, chkHorizontal->getY() + chkHorizontal->getHeight() + DISTANCE_NEXT_Y);
	grpCentering->setMovable(false);
	grpCentering->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpCentering->setBaseColor(gui_base_color);
	grpCentering->setForegroundColor(gui_foreground_color);
	grpCentering->setSize(chkHorizontal->getX() + chkHorizontal->getWidth() + DISTANCE_BORDER * 8, TITLEBAR_HEIGHT + chkVertical->getY() + chkVertical->getHeight() + DISTANCE_NEXT_Y);
	grpCentering->setPosition(category.panel->getWidth() - DISTANCE_BORDER - grpCentering->getWidth(), DISTANCE_BORDER);
	category.panel->add(grpCentering);	
	posY = DISTANCE_BORDER + grpAmigaScreen->getHeight() + DISTANCE_NEXT_Y;

	lblScalingMethod = new gcn::Label("Scaling method:");
	lblScalingMethod->setAlignment(gcn::Graphics::Right);
	cboScalingMethod = new gcn::DropDown(&scaling_method_list);
	cboScalingMethod->setSize(150, cboScalingMethod->getHeight());
	cboScalingMethod->setBaseColor(gui_base_color);
	cboScalingMethod->setBackgroundColor(gui_background_color);
	cboScalingMethod->setForegroundColor(gui_foreground_color);
	cboScalingMethod->setSelectionColor(gui_selection_color);
	cboScalingMethod->setId("cboScalingMethod");
	cboScalingMethod->addActionListener(scalingMethodActionListener);
	category.panel->add(lblScalingMethod, DISTANCE_BORDER, posY);
	category.panel->add(cboScalingMethod, lblScalingMethod->getX() + lblScalingMethod->getWidth() + 15, posY);
	posY += cboScalingMethod->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblResolution, DISTANCE_BORDER, posY);
	category.panel->add(cboResolution, cboScalingMethod->getX(), posY);
	posY += cboResolution->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblResSwitch, DISTANCE_BORDER, posY);
	category.panel->add(cboResSwitch, cboScalingMethod->getX(), posY);
	posY += cboResSwitch->getHeight() + DISTANCE_NEXT_Y;
	
	optSingle = new gcn::RadioButton("Single", "linemodegroup");
	optSingle->setId("optSingle");
	optSingle->setBaseColor(gui_base_color);
	optSingle->setBackgroundColor(gui_background_color);
	optSingle->setForegroundColor(gui_foreground_color);
	optSingle->addActionListener(lineModeActionListener);

	optDouble = new gcn::RadioButton("Double", "linemodegroup");
	optDouble->setId("optDouble");
	optDouble->setBaseColor(gui_base_color);
	optDouble->setBackgroundColor(gui_background_color);
	optDouble->setForegroundColor(gui_foreground_color);
	optDouble->addActionListener(lineModeActionListener);

	optScanlines = new gcn::RadioButton("Scanlines", "linemodegroup");
	optScanlines->setId("optScanlines");
	optScanlines->setBaseColor(gui_base_color);
	optScanlines->setBackgroundColor(gui_background_color);
	optScanlines->setForegroundColor(gui_foreground_color);
	optScanlines->addActionListener(lineModeActionListener);

	optDouble2 = new gcn::RadioButton("Double, fields", "linemodegroup");
	optDouble2->setId("optDouble2");
	optDouble2->setBaseColor(gui_base_color);
	optDouble2->setBackgroundColor(gui_background_color);
	optDouble2->setForegroundColor(gui_foreground_color);
	optDouble2->addActionListener(lineModeActionListener);

	optDouble3 = new gcn::RadioButton("Double, fields+", "linemodegroup");
	optDouble3->setId("optDouble3");
	optDouble3->setBaseColor(gui_base_color);
	optDouble3->setBackgroundColor(gui_background_color);
	optDouble3->setForegroundColor(gui_foreground_color);
	optDouble3->addActionListener(lineModeActionListener);
	
	grpLineMode = new gcn::Window("Line mode");
	grpLineMode->setPosition(grpCentering->getX(), grpCentering->getY() + grpCentering->getHeight() + DISTANCE_NEXT_Y);
	grpLineMode->add(optSingle, 10, 10);
	grpLineMode->add(optDouble, optSingle->getX(), optSingle->getY() + optSingle->getHeight() + DISTANCE_NEXT_Y);
	grpLineMode->add(optScanlines, optSingle->getX(), optDouble->getY() + optDouble->getHeight() + DISTANCE_NEXT_Y);
	grpLineMode->add(optDouble2, optSingle->getX(), optScanlines->getY() + optScanlines->getHeight() + DISTANCE_NEXT_Y);
	grpLineMode->add(optDouble3, optSingle->getX(), optDouble2->getY() + optDouble2->getHeight() + DISTANCE_NEXT_Y);
	grpLineMode->setMovable(false);
	grpLineMode->setSize(grpCentering->getWidth(), TITLEBAR_HEIGHT + optDouble3->getY() + optDouble3->getHeight() + DISTANCE_NEXT_Y);
	grpLineMode->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpLineMode->setBaseColor(gui_base_color);
	grpLineMode->setForegroundColor(gui_foreground_color);
	category.panel->add(grpLineMode);

	optISingle = new gcn::RadioButton("Single", "ilinemodegroup");
	optISingle->setId("optISingle");
	optISingle->setBaseColor(gui_base_color);
	optISingle->setBackgroundColor(gui_background_color);
	optISingle->setForegroundColor(gui_foreground_color);
	optISingle->addActionListener(lineModeActionListener);

	optIDouble = new gcn::RadioButton("Double, frames", "ilinemodegroup");
	optIDouble->setId("optIDouble");
	optIDouble->setBaseColor(gui_base_color);
	optIDouble->setBackgroundColor(gui_background_color);
	optIDouble->setForegroundColor(gui_foreground_color);
	optIDouble->addActionListener(lineModeActionListener);

	optIDouble2 = new gcn::RadioButton("Double, fields", "ilinemodegroup");
	optIDouble2->setId("optIDouble2");
	optIDouble2->setBaseColor(gui_base_color);
	optIDouble2->setBackgroundColor(gui_background_color);
	optIDouble2->setForegroundColor(gui_foreground_color);
	optIDouble2->addActionListener(lineModeActionListener);
	
	optIDouble3 = new gcn::RadioButton("Double, fields+", "ilinemodegroup");
	optIDouble3->setId("optIDouble3");
	optIDouble3->setBaseColor(gui_base_color);
	optIDouble3->setBackgroundColor(gui_background_color);
	optIDouble3->setForegroundColor(gui_foreground_color);
	optIDouble3->addActionListener(lineModeActionListener);

	grpILineMode = new gcn::Window("Interlaced line mode");
	grpILineMode->setPosition(grpLineMode->getX(), grpLineMode->getY() + grpLineMode->getHeight() + DISTANCE_NEXT_Y);
	grpILineMode->add(optISingle, 10, 10);
	grpILineMode->add(optIDouble, optISingle->getX(), optISingle->getY() + optISingle->getHeight() + DISTANCE_NEXT_Y);
	grpILineMode->add(optIDouble2, optISingle->getX(), optIDouble->getY() + optIDouble->getHeight() + DISTANCE_NEXT_Y);
	grpILineMode->add(optIDouble3, optISingle->getX(), optIDouble2->getY() + optIDouble2->getHeight() + DISTANCE_NEXT_Y);
	grpILineMode->setMovable(false);
	grpILineMode->setSize(grpCentering->getWidth(), TITLEBAR_HEIGHT + optIDouble3->getY() + optIDouble3->getHeight() + DISTANCE_NEXT_Y);
	grpILineMode->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpILineMode->setBaseColor(gui_base_color);
	grpILineMode->setForegroundColor(gui_foreground_color);
	category.panel->add(grpILineMode);

	category.panel->add(chkBlackerThanBlack, DISTANCE_BORDER, posY);
	category.panel->add(chkFilterLowRes, chkBlackerThanBlack->getX() + chkBlackerThanBlack->getWidth() + DISTANCE_NEXT_X, posY);
	posY += chkFilterLowRes->getHeight() + DISTANCE_NEXT_Y;
	
	category.panel->add(chkAspect, DISTANCE_BORDER, posY);
	posY += chkAspect->getHeight() + DISTANCE_NEXT_Y;
	
	category.panel->add(chkFlickerFixer, DISTANCE_BORDER, posY);
	posY += chkFlickerFixer->getHeight() + DISTANCE_NEXT_Y;
	
	category.panel->add(chkFrameskip, DISTANCE_BORDER, posY);
	category.panel->add(sldRefresh, chkFrameskip->getX() + chkFrameskip->getWidth() + DISTANCE_NEXT_X + 1, posY);
	category.panel->add(lblFrameRate, sldRefresh->getX() + sldRefresh->getWidth() + 8, posY + 2);

	category.panel->add(chkFpsAdj, lblFrameRate->getX() + lblFrameRate->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(sldFpsAdj, chkFpsAdj->getX() + chkFpsAdj->getWidth() + DISTANCE_NEXT_X + 1, posY);
	category.panel->add(txtFpsAdj, sldFpsAdj->getX() + sldFpsAdj->getWidth() + 8, posY);
	category.panel->add(cboFpsRate, txtFpsAdj->getX(), chkFlickerFixer->getY());
	posY += chkFrameskip->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblBrightness, DISTANCE_BORDER, posY);
	category.panel->add(sldBrightness, lblBrightness->getX() + lblBrightness->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(lblBrightnessValue, sldBrightness->getX() + sldBrightness->getWidth() + 8, posY);

	RefreshPanelDisplay();
}

void ExitPanelDisplay()
{
	delete chkFrameskip;
	delete sldRefresh;
	delete lblFrameRate;
	delete cboFpsRate;
	delete chkFpsAdj;
	delete sldFpsAdj;
	delete txtFpsAdj;
	delete lblBrightness;
	delete sldBrightness;
	delete lblBrightnessValue;
	delete amigaScreenActionListener;
	delete amigaScreenKeyListener;
	delete chkManualCrop;
	delete lblAmigaWidth;
	delete sldAmigaWidth;
	delete txtAmigaWidth;
	delete lblAmigaHeight;
	delete sldAmigaHeight;
	delete txtAmigaHeight;
	delete chkAutoCrop;
	delete chkBorderless;
	delete lblHOffset;
	delete sldHOffset;
	delete lblHOffsetValue;
	delete lblVOffset;
	delete sldVOffset;
	delete lblVOffsetValue;
	delete grpAmigaScreen;

	delete chkHorizontal;
	delete chkVertical;
	delete chkFlickerFixer;
	delete grpCentering;

	delete chkBlackerThanBlack;
	delete chkAspect;
	delete lblScreenmode;
	delete cboScreenmode;
	delete lblFullscreen;
	delete cboFullscreen;
	delete cboRefreshRate;

	delete lblVSyncNative;
	delete lblVSyncRtg;
	delete cboVSyncNative;
	delete cboVSyncRtg;

	delete optSingle;
	delete optDouble;
	delete optScanlines;
	delete optDouble2;
	delete optDouble3;
	delete grpLineMode;
	delete optISingle;
	delete optIDouble;
	delete optIDouble2;
	delete optIDouble3;
	delete grpILineMode;
	delete lineModeActionListener;

	delete lblScalingMethod;
	delete cboScalingMethod;
	delete scalingMethodActionListener;

	delete lblResolution;
	delete cboResolution;
	delete chkFilterLowRes;

	delete lblResSwitch;
	delete cboResSwitch;
}

static void refresh_fps_options()
{
	TCHAR buffer[MAX_DPATH];
	int rates[MAX_CHIPSET_REFRESH_TOTAL];

	fps_options.clear();
	int v = 0;
	const chipset_refresh* selectcr = changed_prefs.ntscmode ? &changed_prefs.cr[CHIPSET_REFRESH_NTSC] : &changed_prefs.cr[CHIPSET_REFRESH_PAL];
	for (int i = 0; i < MAX_CHIPSET_REFRESH_TOTAL; i++) {
		struct chipset_refresh* cr = &changed_prefs.cr[i];
		if (cr->rate > 0) {
			_tcscpy(buffer, cr->label);
			if (!buffer[0])
				_sntprintf(buffer, sizeof buffer, _T(":%d"), i);
			fps_options.emplace_back(buffer);
			double d = changed_prefs.chipset_refreshrate;
			if (abs(d) < 1)
				d = currprefs.ntscmode ? 60.0 : 50.0;
			if (selectcr->index == cr->index)
				changed_prefs.cr_selected = i;
			rates[i] = v;
			v++;
		}
	}

	if (changed_prefs.cr_selected < 0 || changed_prefs.cr[changed_prefs.cr_selected].rate <= 0)
		changed_prefs.cr_selected = CHIPSET_REFRESH_PAL;
	selectcr = &changed_prefs.cr[changed_prefs.cr_selected];
	cboFpsRate->setSelected(rates[changed_prefs.cr_selected]);
	sldFpsAdj->setValue(selectcr->rate + 0.5);
	_sntprintf(buffer, sizeof buffer, _T("%.6f"), selectcr->rate);
	txtFpsAdj->setText(std::string(buffer));
	chkFpsAdj->setSelected(selectcr->locked);

	txtFpsAdj->setEnabled(selectcr->locked != 0);
	sldFpsAdj->setEnabled(selectcr->locked != 0);

	v = changed_prefs.cpu_memory_cycle_exact ? 1 : changed_prefs.gfx_framerate;
	sldRefresh->setValue(v);
}

void RefreshPanelDisplay()
{
	init_display_mode();
	refresh_fps_options();

	chkFrameskip->setSelected(changed_prefs.gfx_framerate > 1);
	sldRefresh->setEnabled(chkFrameskip->isSelected());

	lblFrameRate->setCaption(std::to_string(changed_prefs.gfx_framerate));
	lblFrameRate->adjustSize();

	sldBrightness->setValue(changed_prefs.gfx_luminance);
	lblBrightnessValue->setCaption(std::to_string(changed_prefs.gfx_luminance));
	lblBrightnessValue->adjustSize();

	int i;
	for (i = 0; i < AMIGAWIDTH_COUNT; ++i)
	{
		if (changed_prefs.gfx_manual_crop_width == amigawidth_values[i])
		{
			sldAmigaWidth->setValue(i);
			txtAmigaWidth->setText(std::to_string(changed_prefs.gfx_manual_crop_width));
			break;
		}
		if (i == AMIGAWIDTH_COUNT - 1)
		{
			txtAmigaWidth->setText(std::to_string(changed_prefs.gfx_manual_crop_width));
			break;
		}
	}

	for (i = 0; i < AMIGAHEIGHT_COUNT; ++i)
	{
		if (changed_prefs.gfx_manual_crop_height == amigaheight_values[i])
		{
			sldAmigaHeight->setValue(i);
			txtAmigaHeight->setText(std::to_string(changed_prefs.gfx_manual_crop_height));
			break;
		}
		if (i == AMIGAHEIGHT_COUNT - 1)
		{
			txtAmigaHeight->setText(std::to_string(changed_prefs.gfx_manual_crop_height));
			break;
		}
	}

	if (changed_prefs.gfx_auto_crop)
	{
		changed_prefs.gfx_xcenter = 0;
		changed_prefs.gfx_ycenter = 0;
	}

	chkManualCrop->setSelected(changed_prefs.gfx_manual_crop);
	chkAutoCrop->setSelected(changed_prefs.gfx_auto_crop);

	sldAmigaWidth->setEnabled(changed_prefs.gfx_manual_crop);
	sldAmigaHeight->setEnabled(changed_prefs.gfx_manual_crop);
	txtAmigaWidth->setEnabled(changed_prefs.gfx_manual_crop);
	txtAmigaHeight->setEnabled(changed_prefs.gfx_manual_crop);
	sldHOffset->setEnabled(changed_prefs.gfx_manual_crop);
	sldVOffset->setEnabled(changed_prefs.gfx_manual_crop);

	chkBorderless->setSelected(changed_prefs.borderless);

	if (kmsdrm_detected)
	{
		changed_prefs.gfx_apmode[0].gfx_fullscreen = GFX_FULLWINDOW;
		changed_prefs.gfx_apmode[1].gfx_fullscreen = GFX_FULLWINDOW;
		cboScreenmode->setEnabled(false);
	}

	if (changed_prefs.gfx_apmode[0].gfx_fullscreen == GFX_WINDOW)
	{
		cboScreenmode->setSelected(0);
		cboFullscreen->setEnabled(false);
		cboRefreshRate->setEnabled(false);
	}
	else if (changed_prefs.gfx_apmode[0].gfx_fullscreen == GFX_FULLSCREEN)
	{
		cboScreenmode->setSelected(1);
		cboFullscreen->setEnabled(true);
		cboRefreshRate->setEnabled(true);
	}
	else if (changed_prefs.gfx_apmode[0].gfx_fullscreen == GFX_FULLWINDOW)
	{
		cboScreenmode->setSelected(2);
		cboFullscreen->setEnabled(false);
		cboRefreshRate->setEnabled(false);
	}
	int v = changed_prefs.gfx_apmode[0].gfx_vsync;
	if (v < 0)
		v = 5;
	else if (v > 0) {
		v = v + (changed_prefs.gfx_apmode[0].gfx_vsyncmode || !v ? 0 : 2);
	}
	cboVSyncNative->setSelected(v);

	v = changed_prefs.gfx_apmode[1].gfx_vsync;
	if (v < 0)
		v = 2;
	else if (v > 0)
		v = 1;
	cboVSyncRtg->setSelected(v);

	sldHOffset->setValue(changed_prefs.gfx_horizontal_offset);
	lblHOffsetValue->setCaption(std::to_string(changed_prefs.gfx_horizontal_offset));
	lblHOffsetValue->adjustSize();

	sldVOffset->setValue(changed_prefs.gfx_vertical_offset);
	lblVOffsetValue->setCaption(std::to_string(changed_prefs.gfx_vertical_offset));
	lblVOffsetValue->adjustSize();

	chkHorizontal->setEnabled(!changed_prefs.gfx_auto_crop);
	chkVertical->setEnabled(!changed_prefs.gfx_auto_crop);
	chkHorizontal->setSelected(changed_prefs.gfx_xcenter == 2);
	chkVertical->setSelected(changed_prefs.gfx_ycenter == 2);

	chkFlickerFixer->setSelected(changed_prefs.gfx_scandoubler);
	chkBlackerThanBlack->setSelected(changed_prefs.gfx_blackerthanblack);
	
	chkAspect->setSelected(changed_prefs.gfx_correct_aspect);
	chkFilterLowRes->setSelected(changed_prefs.gfx_lores_mode);

	//Disable Borderless checkbox in non-Windowed modes
	chkBorderless->setEnabled(cboScreenmode->getSelected() == 0);

	cboScalingMethod->setSelected(changed_prefs.scaling_method + 1);

	if (changed_prefs.gfx_autoresolution == 0 || changed_prefs.gfx_autoresolution > 99)
		cboResSwitch->setSelected(0);
	else if (changed_prefs.gfx_autoresolution == 1)
		cboResSwitch->setSelected(1);
	else if (changed_prefs.gfx_autoresolution <= 10)
		cboResSwitch->setSelected(2);
	else if (changed_prefs.gfx_autoresolution <= 33)
		cboResSwitch->setSelected(3);
	else if (changed_prefs.gfx_autoresolution <= 99)
		cboResSwitch->setSelected(4);

	if (changed_prefs.gfx_vresolution == VRES_NONDOUBLE && changed_prefs.gfx_pscanlines == 0)
	{
		optSingle->setSelected(true);
		disable_idouble_modes();
	}
	else if (changed_prefs.gfx_vresolution >= VRES_DOUBLE && changed_prefs.gfx_pscanlines == 0)
	{
		optDouble->setSelected(true);
		enable_idouble_modes();
	}
	else if (changed_prefs.gfx_vresolution >= VRES_DOUBLE && changed_prefs.gfx_pscanlines == 1)
	{
		optScanlines->setSelected(true);
		enable_idouble_modes();
	}
	else if (changed_prefs.gfx_vresolution >= VRES_DOUBLE && changed_prefs.gfx_pscanlines == 2)
	{
		optDouble2->setSelected(true);
		enable_idouble_modes();
	}
	else if (changed_prefs.gfx_vresolution >= VRES_DOUBLE && changed_prefs.gfx_pscanlines == 3)
	{
		optDouble3->setSelected(true);
		enable_idouble_modes();
	}

	if (changed_prefs.gfx_iscanlines == 0)
	{
		if (changed_prefs.gfx_vresolution >= VRES_DOUBLE)
			optIDouble->setSelected(true);
		else
			optISingle->setSelected(true);
	}
	else if (changed_prefs.gfx_iscanlines == 1)
		optIDouble2->setSelected(true);
	else if (changed_prefs.gfx_iscanlines == 2)
		optIDouble3->setSelected(true);
	
	cboResolution->setSelected(changed_prefs.gfx_resolution);

	bool isdouble = changed_prefs.gfx_vresolution > 0;
	optSingle->setEnabled(!changed_prefs.gfx_autoresolution);
	optDouble->setEnabled(!changed_prefs.gfx_autoresolution);
	optScanlines->setEnabled(!changed_prefs.gfx_autoresolution);
	optDouble2->setEnabled(!changed_prefs.gfx_autoresolution);
	optDouble3->setEnabled(!changed_prefs.gfx_autoresolution);
	optISingle->setEnabled(!changed_prefs.gfx_autoresolution && !isdouble);
	optIDouble->setEnabled(!changed_prefs.gfx_autoresolution && isdouble);
	optIDouble2->setEnabled(!changed_prefs.gfx_autoresolution && isdouble);
	optIDouble3->setEnabled(!changed_prefs.gfx_autoresolution && isdouble);
	cboResolution->setEnabled(!changed_prefs.gfx_autoresolution);

	//TODO Overscan settings
	//xSendDlgItemMessage(hDlg, IDC_OVERSCANMODE, CB_RESETCONTENT, 0, 0);
	//xSendDlgItemMessage(hDlg, IDC_OVERSCANMODE, CB_ADDSTRING, 0, (LPARAM)_T("TV (narrow)"));
	//xSendDlgItemMessage(hDlg, IDC_OVERSCANMODE, CB_ADDSTRING, 0, (LPARAM)_T("TV (standard"));
	//xSendDlgItemMessage(hDlg, IDC_OVERSCANMODE, CB_ADDSTRING, 0, (LPARAM)_T("TV (wide)"));
	//xSendDlgItemMessage(hDlg, IDC_OVERSCANMODE, CB_ADDSTRING, 0, (LPARAM)_T("Overscan"));
	//xSendDlgItemMessage(hDlg, IDC_OVERSCANMODE, CB_ADDSTRING, 0, (LPARAM)_T("Overscan+"));
	//xSendDlgItemMessage(hDlg, IDC_OVERSCANMODE, CB_ADDSTRING, 0, (LPARAM)_T("Extreme"));
	//xSendDlgItemMessage(hDlg, IDC_OVERSCANMODE, CB_ADDSTRING, 0, (LPARAM)_T("Ultra extreme debug"));
	//xSendDlgItemMessage(hDlg, IDC_OVERSCANMODE, CB_ADDSTRING, 0, (LPARAM)_T("Ultra extreme debug (HV)"));
	//xSendDlgItemMessage(hDlg, IDC_OVERSCANMODE, CB_ADDSTRING, 0, (LPARAM)_T("Ultra extreme debug (C)"));
	//xSendDlgItemMessage(hDlg, IDC_OVERSCANMODE, CB_SETCURSEL, workprefs.gfx_overscanmode, 0);

}

bool HelpPanelDisplay(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("Select the required width, height and other options of displayed the Amiga screen.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Here you can select the screen mode to be used as well:");
	helptext.emplace_back(" ");
	helptext.emplace_back("- Windowed: the Amiga screen will be shown in a Window on your desktop");
	helptext.emplace_back("- Fullscreen: the monitor resolution will change to the selected one.");
	helptext.emplace_back("- Full-window: the Amiga screen will be scaled to the current resolution.");
	helptext.emplace_back(" ");
	helptext.emplace_back("The \"Auto-Crop\" option will try to detect the width/height automatically,");
	helptext.emplace_back("and scale it up to the full area of the screen, eliminating any black borders.");
	helptext.emplace_back("When this option is disabled, the \"Centering\" options become active, and allow");
	helptext.emplace_back("you to enable automatic centering, for Horizontal and Vertical axes individually.");
	helptext.emplace_back("This can be useful but keep in mind that especially the vertical centering might");
	helptext.emplace_back("cause issues in some games/demos. If that happens, try disabling it and adjust");
	helptext.emplace_back("the Width/Height (in combination with H/V offset if you want), instead.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Borderless\" is only useful in a graphical environment (ie; not when running in");
	helptext.emplace_back("console). This will make the Amiberry window appear borderless, with no window");
	helptext.emplace_back("controls/decorations around it.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"VSync\" - enabling VSync will use the refresh rate of your monitor to do screen");
	helptext.emplace_back("updates. This might reduce tearing, but requires that your monitor supports the same");
	helptext.emplace_back("refresh rate that's being emulated (e.g. 50Hz for PAL).");
	helptext.emplace_back(" ");
	helptext.emplace_back("Also, you can use the \"Offset\" options, to manually shift the image horizontally");
	helptext.emplace_back("or vertically, to fine tune the image position.");
	helptext.emplace_back(" ");
	helptext.emplace_back("You can select the \"Scaling method\" used for the displayed Amiga screen.");
	helptext.emplace_back(" ");
	helptext.emplace_back(" - Auto: will try to find the best looking scaling method depending on your");
	helptext.emplace_back("monitor resolution. This is the default option.");
	helptext.emplace_back(" ");
	helptext.emplace_back(" - Pixelated: will give you a more pixelated and crisp image, but it may cause");
	helptext.emplace_back("some distortion if your resolution is not an exact multiple of what is displayed.");
	helptext.emplace_back(" ");
	helptext.emplace_back(" - Linear: will give you a smoother scaling but some people might find it a bit");
	helptext.emplace_back("blurry depending on the Amiga software being used.");
	helptext.emplace_back(" ");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Blacker than black\" - when enabled, boosts the gamma for the black areas, which");
	helptext.emplace_back("makes the border around them look darker.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Filtered Low Res\" - This option enables filtering when LowRes is selected in the");
	helptext.emplace_back("Resolution option above. It will make the output look smoother instead of pixelated");
	helptext.emplace_back("when using the LowRes option.");
	helptext.emplace_back(" ");
	helptext.emplace_back("The \"Correct Aspect Ratio\" - This allows you to choose if you want the correct");
	helptext.emplace_back("aspect ratio to be the default display method, or have the image stretched to fill");
	helptext.emplace_back("the screen instead.");
	helptext.emplace_back(" ");
	helptext.emplace_back("The \"Remove interlace artifacts\" option enables a special feature internally, to");
	helptext.emplace_back("eliminate the artifacts produced by Interlace modes. It might cause issues in some");
	helptext.emplace_back("applications, so keep that in mind.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Refresh\" - If this option is enabled, it allows you to modify the refresh rate of");
	helptext.emplace_back("the emulation, from the full frame rate that it's normally running at. When this");
	helptext.emplace_back("option is disabled, the value will be set to 1, which represents that every 1 frame");
	helptext.emplace_back("will be drawn/shown. If you enable the option, the value will be changed to 2, which");
	helptext.emplace_back("means that 1 out of every 2 frames will be drawn (essentially what is known as being");
	helptext.emplace_back("'frame-skip'). Increasing this value further, will increase the frame-skipping to");
	helptext.emplace_back("draw one frame out of X (where X = the value you set it to). This can be useful in");
	helptext.emplace_back("situations where you are doing high computational work (like 3D-rendering) and you");
	helptext.emplace_back("don't care about the redraw on the screen. This option can also be useful to get");
	helptext.emplace_back("to get some more games playable.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"FPS Adj.\" - This option allows you to specify a custom frame rate, instead of");
	helptext.emplace_back("the default ones (PAL or NTSC). This can be useful if your monitor does not handle");
	helptext.emplace_back("the exact refresh rate required, and you want to have perfect VSync");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Brightness\" - Allows adjustment of the output image brightness, from -200 to 200.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Line Mode\" - These options define how the Amiga screen is drawn.");
	helptext.emplace_back(" ");
	helptext.emplace_back(" - Single: This will draw single lines only, which will be stretched vertically to");
	helptext.emplace_back("fill the screen when necessary (ie; showing a 640x256 resolution in a 640x512 window)");
	helptext.emplace_back(" ");
	helptext.emplace_back(" - Double: This option will draw every line twice, which will take up a little more");
	helptext.emplace_back("resources, but looks better since they don't have to be stretched vertically. It also");
	helptext.emplace_back("enables the options for the Interlace line mode, below.");
	helptext.emplace_back(" ");
	helptext.emplace_back(" - Scanlines: This option will also draw double the lines, but try to replicate the");
	helptext.emplace_back("scanlines effect shown in CRT monitors. Please note that this is not a shader, so the");
	helptext.emplace_back("the screen output might not be perfect in all resolutions.");
	helptext.emplace_back(" ");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Interlaced line mode\" - These options further define how the Amiga screen is drawn");
	helptext.emplace_back("when using interlaced screenmodes.");
	helptext.emplace_back(" ");
	helptext.emplace_back(" - Single: Draw single lines, interlace modes will flicker like the real hardware");
	helptext.emplace_back(" ");
	helptext.emplace_back(" - Double, frames/fields/fields+: These options are only useful in Interlace modes.");
	helptext.emplace_back(" ");
	return true;
}
