class AmigaScreenActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		AmigaMonitor* mon = &AMonitors[0];
		if (actionEvent.getSource() == chkManualCrop)
		{
			changed_prefs.gfx_manual_crop = chkManualCrop->isSelected();
			if (changed_prefs.gfx_auto_crop)
				changed_prefs.gfx_auto_crop = false;
		}
		else if (actionEvent.getSource() == sldAmigaWidth)
		{
			const int new_width = amigawidth_values[static_cast<int>(sldAmigaWidth->getValue())];
			const int new_x = ((AMIGA_WIDTH_MAX << changed_prefs.gfx_resolution) - new_width) / 2;

			changed_prefs.gfx_manual_crop_width = new_width;
			changed_prefs.gfx_horizontal_offset = new_x;
		}
		else if (actionEvent.getSource() == sldAmigaHeight)
		{
			const int new_height = amigaheight_values[static_cast<int>(sldAmigaHeight->getValue())];
			const int new_y = ((AMIGA_HEIGHT_MAX << changed_prefs.gfx_vresolution) - new_height) / 2;

			changed_prefs.gfx_manual_crop_height = new_height;
			changed_prefs.gfx_vertical_offset = new_y;
		}

		else if (actionEvent.getSource() == chkAutoCrop)
		{
			changed_prefs.gfx_auto_crop = chkAutoCrop->isSelected();
			if (changed_prefs.gfx_manual_crop)
				changed_prefs.gfx_manual_crop = false;
		}

		else if (actionEvent.getSource() == chkBorderless)
			changed_prefs.borderless = chkBorderless->isSelected();

		else if (actionEvent.getSource() == sldHOffset)
		{
			changed_prefs.gfx_horizontal_offset = static_cast<int>(sldHOffset->getValue());
			lblHOffsetValue->setCaption(std::to_string(changed_prefs.gfx_horizontal_offset));
			lblHOffsetValue->adjustSize();
		}
		else if (actionEvent.getSource() == sldVOffset)
		{
			changed_prefs.gfx_vertical_offset = static_cast<int>(sldVOffset->getValue());
			lblVOffsetValue->setCaption(std::to_string(changed_prefs.gfx_vertical_offset));
			lblVOffsetValue->adjustSize();
		}
		else if (actionEvent.getSource() == chkFrameskip)
		{
			changed_prefs.gfx_framerate = chkFrameskip->isSelected() ? 2 : 1;
			sldRefresh->setEnabled(chkFrameskip->isSelected());
			sldRefresh->setValue(changed_prefs.gfx_framerate);
			lblFrameRate->setCaption(std::to_string(changed_prefs.gfx_framerate));
			lblFrameRate->adjustSize();
		}
		else if (actionEvent.getSource() == cboFpsRate
			|| actionEvent.getSource() == chkFpsAdj
			|| actionEvent.getSource() == sldFpsAdj)
		{
			sldFpsAdj->setEnabled(chkFpsAdj->isSelected());
			txtFpsAdj->setEnabled(chkFpsAdj->isSelected());

			int i;
			bool updaterate = false, updateslider = false;
			TCHAR label[16];
			label[0] = 0;
			auto label_string = fps_options[cboFpsRate->getSelected()];
			strncpy(label, label_string.c_str(), sizeof(label) - 1);
			label[sizeof(label) - 1] = '\0';

			struct chipset_refresh* cr;
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
					}
					else {
						cr->locked = chkFpsAdj->isSelected();
						if (cr->locked) {
							cr->inuse = true;
						}
						else {
							// deactivate if plain uncustomized PAL or NTSC
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
					i = sldFpsAdj->getValue();//xSendDlgItemMessage(hDlg, IDC_FRAMERATE2, TBM_GETPOS, 0, 0);
					if (i != (int)cr->rate)
						cr->rate = (float)i;
					updaterate = true;
				}
			}
			else if (i == CHIPSET_REFRESH_PAL) {
				cr->rate = 50.0f;
			}
			else if (i == CHIPSET_REFRESH_NTSC) {
				cr->rate = 60.0f;
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
				_stprintf(buffer, _T("%.6f"), cr->rate);
				txtFpsAdj->setText(std::string(buffer));
			}
			if (updateslider) {
				sldFpsAdj->setValue(cr->rate);
			}
		}
		else if (actionEvent.getSource() == sldBrightness)
		{
			changed_prefs.gfx_luminance = static_cast<int>(sldBrightness->getValue());
			lblBrightnessValue->setCaption(std::to_string(changed_prefs.gfx_luminance));
			lblBrightnessValue->adjustSize();
		}

		else if (actionEvent.getSource() == sldRefresh)
			changed_prefs.gfx_framerate = static_cast<int>(sldRefresh->getValue());

		else if (actionEvent.getSource() == chkAspect)
			changed_prefs.gfx_correct_aspect = chkAspect->isSelected();

		else if (actionEvent.getSource() == chkBlackerThanBlack)
			changed_prefs.gfx_blackerthanblack = chkBlackerThanBlack->isSelected();

		else if (actionEvent.getSource() == cboScreenmode)
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

		else if (actionEvent.getSource() == cboFullscreen)
		{
			const auto idx = cboFullscreen->getSelected();
			if (idx >= 0 && idx <= fullscreen_resolutions_list.getNumberOfElements())
			{
				auto* mon = &changed_prefs.gfx_monitor[0];
				mon->gfx_size_fs.width = fullscreen_width_values[idx];
				mon->gfx_size_fs.height = fullscreen_height_values[idx];
			}
		}

		else if (actionEvent.getSource() == chkHorizontal)
			changed_prefs.gfx_xcenter = chkHorizontal->isSelected() ? 2 : 0;

		else if (actionEvent.getSource() == chkVertical)
			changed_prefs.gfx_ycenter = chkVertical->isSelected() ? 2 : 0;

		else if (actionEvent.getSource() == chkFlickerFixer)
			changed_prefs.gfx_scandoubler = chkFlickerFixer->isSelected();

		else if (actionEvent.getSource() == cboResolution)
			changed_prefs.gfx_resolution = cboResolution->getSelected();

		else if (actionEvent.getSource() == chkFilterLowRes)
			changed_prefs.gfx_lores_mode = chkFilterLowRes->isSelected() ? 1 : 0;

		else if (actionEvent.getSource() == cboResSwitch)
		{
			int pos = cboResSwitch->getSelected();
			if (pos == 0)
				changed_prefs.gfx_autoresolution = 0;
			else if (pos == 1)
				changed_prefs.gfx_autoresolution = 1;
			else if (pos == 2)
				changed_prefs.gfx_autoresolution = 10;
			else if (pos == 3)
				changed_prefs.gfx_autoresolution = 33;
			else if (pos == 4)
				changed_prefs.gfx_autoresolution = 66;
		}

		int i = cboVSyncNative->getSelected();
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

		RefreshPanelDisplay();
	}
};

AmigaScreenActionListener *amigaScreenActionListener;

// Theme ideas:
// "Workbench 1.3", "Workbench 3.1", "Windows 95", "Macintosh", "Solarized Dark", "Solarized Light", "Gruvbox", "Nord"
static const std::vector<std::string> theme_options = { "Workbench 1.3", "Workbench 3.1" };
static gcn::StringListModel theme_options_list(theme_options);

void InitPanelThemes(const config_category& category)
{
  amigaScreenActionListener = new AmigaScreenActionListener();

  int posY = DISTANCE_BORDER;

  cboTheme = new gcn::DropDown(&theme_options_list);

  lblTheme = new gcn::Label("Amiberry Theme:");
	lblTheme->setAlignment(gcn::Graphics::Right);
	cboTheme = new gcn::DropDown(&theme_options_list);
	cboTheme->setSize(150, cboTheme->getHeight());
	cboTheme->setBaseColor(gui_base_color);
	cboTheme->setBackgroundColor(gui_textbox_background_color);
	cboTheme->setForegroundColor(gui_foreground_color);
	cboTheme->setSelectionColor(gui_selection_color);
	cboTheme->setId("cboTheme");
	cboTheme->addActionListener(amigaScreenActionListener);

  grpThemes = new gcn::Window("Themes");
	grpThemes->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);

	grpThemes->add(lblTheme, DISTANCE_BORDER, posY);
	grpThemes->add(cboTheme, lblTheme->getX() + lblTheme->getWidth() + DISTANCE_NEXT_X, posY);
  posY += cboTheme->getHeight() + DISTANCE_NEXT_Y;
}

bool HelpPanelDisplay(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("Select the desired theme for the Amiberry GUI to suit your liking.");
	return true;
}
