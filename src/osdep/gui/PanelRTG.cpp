#include <cstdio>
#include <cstdlib>

#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"
#include "StringListModel.h"

#include "sysdeps.h"
#include "options.h"
#include "gfxboard.h"
#include "gui_handling.h"

static std::vector<std::string> rtg_boards;
static gcn::StringListModel rtg_boards_list;

static const std::vector<std::string> rtg_refreshrates = { "Chipset", "Default", "50", "60", "70", "75" };
static gcn::StringListModel rtg_refreshrates_list(rtg_refreshrates);

static const std::vector<std::string> rtg_buffermodes = { "Double buffering", "Triple buffering" };
static gcn::StringListModel rtg_buffermodes_list(rtg_buffermodes);

static const std::vector<std::string> rtg_aspectratios = { "Disabled", "Automatic" };
static gcn::StringListModel rtg_aspectratios_list(rtg_aspectratios);

static const std::vector<std::string> rtg_16bit_modes = { "(15/16bit)", "All", "R5G6B5PC (*)", "R5G5B5PC", "R5G6B5", "R5G5B5", "B5G6R5PC", "B5G5R5PC" };
static gcn::StringListModel rtg_16bit_modes_list(rtg_16bit_modes);

static const std::vector<std::string> rtg_32bit_modes = { "(32bit)", "All", "A8R8G8B8", "A8B8G8R8", "R8G8B8A8", "B8G8R8A8 (*)" };
static gcn::StringListModel rtg_32bit_modes_list(rtg_32bit_modes);


static gcn::Label* lblBoard;
static gcn::DropDown* cboBoard;
static gcn::Label* lblGfxmem;
static gcn::Label* lblGfxsize;
static gcn::Slider* sldGfxmem;
static gcn::CheckBox* chkRtgMatchDepth;
static gcn::CheckBox* chkRtgAutoscale;
static gcn::CheckBox* chkRtgAllowScaling;
static gcn::CheckBox* chkRtgAlwaysCenter;
static gcn::CheckBox* chkRtgHardwareInterrupt;
static gcn::CheckBox* chkRtgHardwareSprite;
static gcn::CheckBox* chkRtgMultithreaded;
static gcn::Label* lblRtgRefreshRate;
static gcn::DropDown* cboRtgRefreshRate;
static gcn::Label* lblRtgBufferMode;
static gcn::DropDown* cboRtgBufferMode;
static gcn::Label* lblRtgAspectRatio;
static gcn::DropDown* cboRtgAspectRatio;
static gcn::Label* lblRtgColorModes;
static gcn::DropDown* cboRtg16bitModes;
static gcn::DropDown* cboRtg32bitModes;

class RTGActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& action_event) override
	{
		uae_u32 mask = changed_prefs.picasso96_modeflags;
		
		if (action_event.getSource() == cboBoard)
		{
			const auto selected_board = cboBoard->getSelected();
			if (selected_board == 0)
			{
				changed_prefs.rtgboards[0].rtgmem_type = GFXBOARD_UAE_Z3;
				changed_prefs.rtgboards[0].rtgmem_size = 0;
			}
			else
			{
				changed_prefs.rtgboards[0].rtgmem_type = gfxboard_get_id_from_index(selected_board - 1);
				if (changed_prefs.rtgboards[0].rtgmem_size == 0)
					changed_prefs.rtgboards[0].rtgmem_size = 4096 * 1024;
			}
			cfgfile_compatibility_rtg(&changed_prefs);
		}

		else if (action_event.getSource() == sldGfxmem)
		{
			changed_prefs.rtgboards[0].rtgmem_size = memsizes[msi_gfx[static_cast<int>(sldGfxmem->getValue())]];
		}

		else if (action_event.getSource() == chkRtgMatchDepth)
			changed_prefs.rtgmatchdepth = chkRtgMatchDepth->isSelected();

		else if (action_event.getSource() == chkRtgAutoscale)
		{
			if (chkRtgAutoscale->isSelected())
			{
				changed_prefs.gf[1].gfx_filter_autoscale = RTG_MODE_SCALE;
			}
		}

		else if (action_event.getSource() == chkRtgAllowScaling)
			changed_prefs.rtgallowscaling = chkRtgAllowScaling->isSelected();

		else if (action_event.getSource() == chkRtgAlwaysCenter)
		{
			if (chkRtgAlwaysCenter->isSelected())
			{
				changed_prefs.gf[1].gfx_filter_autoscale = RTG_MODE_CENTER;
			}
		}

		else if (action_event.getSource() == chkRtgHardwareInterrupt)
			changed_prefs.rtg_hardwareinterrupt = chkRtgHardwareInterrupt->isSelected();

		else if (action_event.getSource() == chkRtgHardwareSprite)
			changed_prefs.rtg_hardwaresprite = chkRtgHardwareSprite->isSelected();

		else if (action_event.getSource() == chkRtgMultithreaded)
			changed_prefs.rtg_multithread = chkRtgMultithreaded->isSelected();

		else if (action_event.getSource() == cboRtgRefreshRate)
			changed_prefs.rtgvblankrate = cboRtgRefreshRate->getSelected() == 0 ? 0 :
			cboRtgRefreshRate->getSelected() == 1 ? -1 : std::stoi(rtg_refreshrates[cboRtgRefreshRate->getSelected()]);
		
		else if (action_event.getSource() == cboRtgBufferMode)
			changed_prefs.gfx_apmode[1].gfx_backbuffers = cboRtgBufferMode->getSelected() + 1;

		else if (action_event.getSource() == cboRtgAspectRatio)
			changed_prefs.rtgscaleaspectratio = cboRtgAspectRatio->getSelected() == 0 ? 0 : -1;

		mask &= ~RGBFF_CLUT;
		mask |= RGBFF_CLUT;
		int v = cboRtg16bitModes->getSelected();
		mask &= ~(RGBFF_R5G6B5PC | RGBFF_R5G5B5PC | RGBFF_R5G6B5 | RGBFF_R5G5B5 | RGBFF_B5G6R5PC | RGBFF_B5G5R5PC);
		if (v == 1)
			mask |= RGBFF_R5G6B5PC | RGBFF_R5G5B5PC | RGBFF_R5G6B5 | RGBFF_R5G5B5 | RGBFF_B5G6R5PC | RGBFF_B5G5R5PC;
		if (v == 2)
			mask |= RGBFF_R5G6B5PC;
		if (v == 3)
			mask |= RGBFF_R5G5B5PC;
		if (v == 4)
			mask |= RGBFF_R5G6B5;
		if (v == 5)
			mask |= RGBFF_R5G5B5;
		if (v == 6)
			mask |= RGBFF_B5G6R5PC;
		if (v == 7)
			mask |= RGBFF_B5G5R5PC;

		v = cboRtg32bitModes->getSelected();
		mask &= ~(RGBFF_A8R8G8B8 | RGBFF_A8B8G8R8 | RGBFF_R8G8B8A8 | RGBFF_B8G8R8A8);
		if (v == 1)
			mask |= RGBFF_A8R8G8B8 | RGBFF_A8B8G8R8 | RGBFF_R8G8B8A8 | RGBFF_B8G8R8A8;
		if (v == 2)
			mask |= RGBFF_A8R8G8B8;
		if (v == 3)
			mask |= RGBFF_A8B8G8R8;
		if (v == 4)
			mask |= RGBFF_R8G8B8A8;
		if (v == 5)
			mask |= RGBFF_B8G8R8A8;

		changed_prefs.picasso96_modeflags = static_cast<int>(mask);
		
		RefreshPanelRTG();
	}
};

RTGActionListener* rtg_action_listener;

void InitPanelRTG(const config_category& category)
{
	rtg_boards.clear();
	rtg_boards.emplace_back("-");
	int v = 0;
	TCHAR tmp[256];
	for (;;) {
		int index = gfxboard_get_id_from_index(v);
		if (index < 0)
			break;
		const TCHAR* n1 = gfxboard_get_name(index);
		const TCHAR* n2 = gfxboard_get_manufacturername(index);
		v++;
		_tcscpy(tmp, n1);
		if (n2) {
			_tcscat(tmp, _T(" ("));
			_tcscat(tmp, n2);
			_tcscat(tmp, _T(")"));
		}
		rtg_boards.emplace_back(tmp);
	}
	rtg_boards_list = gcn::StringListModel(rtg_boards);

	constexpr int marker_length = 20;

	rtg_action_listener = new RTGActionListener();

	lblBoard = new gcn::Label("RTG Graphics Board:");
	lblBoard->setAlignment(gcn::Graphics::Left);

	cboBoard = new gcn::DropDown(&rtg_boards_list);
	cboBoard->setSize(300, cboBoard->getHeight());
	cboBoard->setBaseColor(gui_base_color);
	cboBoard->setBackgroundColor(gui_background_color);
	cboBoard->setForegroundColor(gui_foreground_color);
	cboBoard->setSelectionColor(gui_selection_color);
	cboBoard->setId("cboBoard");
	cboBoard->addActionListener(rtg_action_listener);

	lblGfxmem = new gcn::Label("VRAM size:");
	lblGfxsize = new gcn::Label("None   ");
	sldGfxmem = new gcn::Slider(0, 8);
	sldGfxmem->setSize(cboBoard->getWidth() - lblGfxmem->getWidth() - lblGfxsize->getWidth(), SLIDER_HEIGHT);
	sldGfxmem->setBaseColor(gui_base_color);
	sldGfxmem->setBackgroundColor(gui_background_color);
	sldGfxmem->setForegroundColor(gui_foreground_color);
	sldGfxmem->setMarkerLength(marker_length);
	sldGfxmem->setStepLength(1);
	sldGfxmem->setId("sldGfxmem");
	sldGfxmem->addActionListener(rtg_action_listener);

	chkRtgMatchDepth = new gcn::CheckBox("Match host and RTG color depth if possible");
	chkRtgMatchDepth->setId("chkRtgMatchDepth");
	chkRtgMatchDepth->setBaseColor(gui_base_color);
	chkRtgMatchDepth->setBackgroundColor(gui_background_color);
	chkRtgMatchDepth->setForegroundColor(gui_foreground_color);
	chkRtgMatchDepth->addActionListener(rtg_action_listener);

	chkRtgAutoscale = new gcn::CheckBox("Scale if smaller than display size setting");
	chkRtgAutoscale->setId("chkRtgAutoscale");
	chkRtgAutoscale->setBaseColor(gui_base_color);
	chkRtgAutoscale->setBackgroundColor(gui_background_color);
	chkRtgAutoscale->setForegroundColor(gui_foreground_color);
	chkRtgAutoscale->addActionListener(rtg_action_listener);

	chkRtgAllowScaling = new gcn::CheckBox("Always scale in windowed mode");
	chkRtgAllowScaling->setId("chkRtgAllowScaling");
	chkRtgAllowScaling->setBaseColor(gui_base_color);
	chkRtgAllowScaling->setBackgroundColor(gui_background_color);
	chkRtgAllowScaling->setForegroundColor(gui_foreground_color);
	chkRtgAllowScaling->addActionListener(rtg_action_listener);

	chkRtgAlwaysCenter = new gcn::CheckBox("Always center");
	chkRtgAlwaysCenter->setId("chkRtgAlwaysCenter");
	chkRtgAlwaysCenter->setBaseColor(gui_base_color);
	chkRtgAlwaysCenter->setBackgroundColor(gui_background_color);
	chkRtgAlwaysCenter->setForegroundColor(gui_foreground_color);
	chkRtgAlwaysCenter->addActionListener(rtg_action_listener);

	chkRtgHardwareInterrupt = new gcn::CheckBox("Hardware vertical blank interrupt");
	chkRtgHardwareInterrupt->setId("chkRtgHardwareInterrupt");
	chkRtgHardwareInterrupt->setBaseColor(gui_base_color);
	chkRtgHardwareInterrupt->setBackgroundColor(gui_background_color);
	chkRtgHardwareInterrupt->setForegroundColor(gui_foreground_color);
	chkRtgHardwareInterrupt->addActionListener(rtg_action_listener);

	chkRtgHardwareSprite = new gcn::CheckBox("Hardware sprite emulation");
	chkRtgHardwareSprite->setId("chkRtgHardwareSprite");
	chkRtgHardwareSprite->setBaseColor(gui_base_color);
	chkRtgHardwareSprite->setBackgroundColor(gui_background_color);
	chkRtgHardwareSprite->setForegroundColor(gui_foreground_color);
	chkRtgHardwareSprite->addActionListener(rtg_action_listener);

	chkRtgMultithreaded = new gcn::CheckBox("Multithreaded");
	chkRtgMultithreaded->setId("chkRtgMultithreaded");
	chkRtgMultithreaded->setBaseColor(gui_base_color);
	chkRtgMultithreaded->setBackgroundColor(gui_background_color);
	chkRtgMultithreaded->setForegroundColor(gui_foreground_color);
	chkRtgMultithreaded->addActionListener(rtg_action_listener);

	lblRtgRefreshRate = new gcn::Label("Refresh rate:");
	lblRtgRefreshRate->setAlignment(gcn::Graphics::Left);
	cboRtgRefreshRate = new gcn::DropDown(&rtg_refreshrates_list);
	cboRtgRefreshRate->setSize(150, cboRtgRefreshRate->getHeight());
	cboRtgRefreshRate->setBaseColor(gui_base_color);
	cboRtgRefreshRate->setBackgroundColor(gui_background_color);
	cboRtgRefreshRate->setForegroundColor(gui_foreground_color);
	cboRtgRefreshRate->setSelectionColor(gui_selection_color);
	cboRtgRefreshRate->setId("cboRtgRefreshRate");
	cboRtgRefreshRate->addActionListener(rtg_action_listener);

	lblRtgBufferMode = new gcn::Label("Buffer mode:");
	lblRtgBufferMode->setAlignment(gcn::Graphics::Left);
	cboRtgBufferMode = new gcn::DropDown(&rtg_buffermodes_list);
	cboRtgBufferMode->setSize(150, cboRtgBufferMode->getHeight());
	cboRtgBufferMode->setBaseColor(gui_base_color);
	cboRtgBufferMode->setBackgroundColor(gui_background_color);
	cboRtgBufferMode->setForegroundColor(gui_foreground_color);
	cboRtgBufferMode->setSelectionColor(gui_selection_color);
	cboRtgBufferMode->setId("cboRtgBufferMode");
	cboRtgBufferMode->addActionListener(rtg_action_listener);

	lblRtgAspectRatio = new gcn::Label("Aspect Ratio:");
	lblRtgAspectRatio->setAlignment(gcn::Graphics::Left);
	cboRtgAspectRatio = new gcn::DropDown(&rtg_aspectratios_list);
	cboRtgAspectRatio->setSize(150, cboRtgAspectRatio->getHeight());
	cboRtgAspectRatio->setBaseColor(gui_base_color);
	cboRtgAspectRatio->setBackgroundColor(gui_background_color);
	cboRtgAspectRatio->setForegroundColor(gui_foreground_color);
	cboRtgAspectRatio->setSelectionColor(gui_selection_color);
	cboRtgAspectRatio->setId("cboRtgAspectRatio");
	cboRtgAspectRatio->addActionListener(rtg_action_listener);

	lblRtgColorModes = new gcn::Label("Color modes:");
	lblRtgColorModes->setAlignment(gcn::Graphics::Left);

	cboRtg16bitModes = new gcn::DropDown(&rtg_16bit_modes_list);
	cboRtg16bitModes->setSize(150, cboRtg16bitModes->getHeight());
	cboRtg16bitModes->setBaseColor(gui_base_color);
	cboRtg16bitModes->setBackgroundColor(gui_background_color);
	cboRtg16bitModes->setForegroundColor(gui_foreground_color);
	cboRtg16bitModes->setSelectionColor(gui_selection_color);
	cboRtg16bitModes->setId("cboRtg16bitModes");
	cboRtg16bitModes->addActionListener(rtg_action_listener);

	cboRtg32bitModes = new gcn::DropDown(&rtg_32bit_modes_list);
	cboRtg32bitModes->setSize(150, cboRtg32bitModes->getHeight());
	cboRtg32bitModes->setBaseColor(gui_base_color);
	cboRtg32bitModes->setBackgroundColor(gui_background_color);
	cboRtg32bitModes->setForegroundColor(gui_foreground_color);
	cboRtg32bitModes->setSelectionColor(gui_selection_color);
	cboRtg32bitModes->setId("cboRtg32bitModes");
	cboRtg32bitModes->addActionListener(rtg_action_listener);
	
	int posY = DISTANCE_BORDER;
	
	category.panel->add(lblBoard, DISTANCE_BORDER, posY);
	category.panel->add(lblRtgColorModes, lblBoard->getX() + lblBoard->getWidth() + DISTANCE_NEXT_X * 18, posY);
	posY += lblBoard->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(cboBoard, DISTANCE_BORDER, posY);
	category.panel->add(cboRtg16bitModes, lblRtgColorModes->getX() - DISTANCE_NEXT_X, posY);
	posY += cboBoard->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblGfxmem, DISTANCE_BORDER, posY);
	category.panel->add(sldGfxmem, lblGfxmem->getWidth() + DISTANCE_NEXT_X + 8, posY);
	category.panel->add(lblGfxsize, sldGfxmem->getX() + sldGfxmem->getWidth() + 8, posY);
	category.panel->add(cboRtg32bitModes, cboRtg16bitModes->getX(), posY);
	posY += sldGfxmem->getHeight() + DISTANCE_NEXT_Y * 2;

	category.panel->add(chkRtgMatchDepth, DISTANCE_BORDER, posY);
	posY += chkRtgMatchDepth->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(chkRtgAutoscale, DISTANCE_BORDER, posY);
	posY += chkRtgAutoscale->getHeight() + DISTANCE_NEXT_Y;
	
	category.panel->add(chkRtgAllowScaling, DISTANCE_BORDER, posY);
	posY += chkRtgAllowScaling->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(chkRtgAlwaysCenter, DISTANCE_BORDER, posY);
	posY += chkRtgAlwaysCenter->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(chkRtgHardwareInterrupt, DISTANCE_BORDER, posY);
	posY += chkRtgHardwareInterrupt->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(chkRtgHardwareSprite, DISTANCE_BORDER, posY);
	posY += chkRtgHardwareSprite->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(chkRtgMultithreaded, DISTANCE_BORDER, posY);
	posY += chkRtgMultithreaded->getHeight() + DISTANCE_NEXT_Y * 2;

	category.panel->add(lblRtgRefreshRate, DISTANCE_BORDER, posY);
	category.panel->add(lblRtgBufferMode, lblRtgRefreshRate->getX() + lblRtgRefreshRate->getWidth() + DISTANCE_NEXT_X * 5, posY);
	category.panel->add(lblRtgAspectRatio, lblRtgBufferMode->getX() + lblRtgBufferMode->getWidth() + DISTANCE_NEXT_X * 6, posY);
	posY += lblRtgRefreshRate->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(cboRtgRefreshRate, DISTANCE_BORDER, posY);
	category.panel->add(cboRtgBufferMode, cboRtgRefreshRate->getX() + cboRtgRefreshRate->getWidth() + DISTANCE_NEXT_X * 2, posY);
	category.panel->add(cboRtgAspectRatio, cboRtgBufferMode->getX() + cboRtgBufferMode->getWidth() + DISTANCE_NEXT_X * 2, posY);

	RefreshPanelRTG();
}

void ExitPanelRTG()
{
	delete lblBoard;
	delete cboBoard;
	delete lblGfxmem;
	delete sldGfxmem;
	delete lblGfxsize;
	delete chkRtgMatchDepth;
	delete chkRtgAutoscale;
	delete chkRtgAllowScaling;
	delete chkRtgAlwaysCenter;
	delete chkRtgHardwareInterrupt;
	delete chkRtgHardwareSprite;
	delete chkRtgMultithreaded;
	delete lblRtgRefreshRate;
	delete cboRtgRefreshRate;
	delete lblRtgBufferMode;
	delete cboRtgBufferMode;
	delete lblRtgAspectRatio;
	delete cboRtgAspectRatio;
	delete lblRtgColorModes;
	delete cboRtg16bitModes;
	delete cboRtg32bitModes;

	delete rtg_action_listener;
}

void RefreshPanelRTG()
{
	rtgboardconfig* rbc = &changed_prefs.rtgboards[0];

	if (rbc->rtgmem_size == 0)
		cboBoard->setSelected(0);
	else
		cboBoard->setSelected(gfxboard_get_index_from_id(rbc->rtgmem_type) + 1);

	if (changed_prefs.address_space_24)
	{
		sldGfxmem->setEnabled(false);
		lblGfxmem->setEnabled(false);
		lblGfxsize->setEnabled(false);
		lblGfxsize->setCaption("N/A");
	}
	else
	{
		sldGfxmem->setEnabled(true);
		lblGfxmem->setEnabled(true);
		lblGfxsize->setEnabled(true);
		lblGfxsize->setCaption("None");

		int mem_size = 0;
		switch (rbc->rtgmem_size) {
			case 0x00000000: mem_size = 0; break;
			case 0x00100000: mem_size = 1; break;
			case 0x00200000: mem_size = 2; break;
			case 0x00400000: mem_size = 3; break;
			case 0x00800000: mem_size = 4; break;
			case 0x01000000: mem_size = 5; break;
			case 0x02000000: mem_size = 6; break;
			case 0x04000000: mem_size = 7; break;
			case 0x08000000: mem_size = 8; break;
			case 0x10000000: mem_size = 9; break;
			case 0x20000000: mem_size = 10; break;
			case 0x40000000: mem_size = 11; break;
			default: break;
		}
		sldGfxmem->setValue(mem_size);
		lblGfxsize->setCaption(memsize_names[msi_gfx[mem_size]]);
	}

	// We always match depth, so this option is disabled
	chkRtgMatchDepth->setEnabled(false);
	chkRtgMatchDepth->setSelected(changed_prefs.rtgmatchdepth);

	// We always scale, so this option is disabled
	chkRtgAutoscale->setEnabled(false);
	chkRtgAutoscale->setSelected(changed_prefs.gf[1].gfx_filter_autoscale == RTG_MODE_SCALE);

	chkRtgAlwaysCenter->setEnabled(false); // Not implemented yet
	chkRtgAlwaysCenter->setSelected(changed_prefs.gf[1].gfx_filter_autoscale == RTG_MODE_CENTER);

	chkRtgAllowScaling->setEnabled(false); // Not implemented yet
	chkRtgAllowScaling->setSelected(changed_prefs.rtgallowscaling);

	chkRtgHardwareInterrupt->setEnabled(!emulating);
	chkRtgHardwareInterrupt->setSelected(changed_prefs.rtg_hardwareinterrupt);

	// Only enable this if Virtual Mouse option is enabled,
	// otherwise we'll get no cursor at all (due to SDL2 and Relative Mouse mode)
	chkRtgHardwareSprite->setEnabled(changed_prefs.input_tablet > 0 && !emulating);
	chkRtgHardwareSprite->setSelected(changed_prefs.rtg_hardwaresprite);

	chkRtgMultithreaded->setEnabled(!emulating);
	chkRtgMultithreaded->setSelected(changed_prefs.rtg_multithread);

	if (changed_prefs.rtgvblankrate <= 0 ||
		changed_prefs.rtgvblankrate == 50 ||
		changed_prefs.rtgvblankrate == 60 ||
		changed_prefs.rtgvblankrate == 70 ||
		changed_prefs.rtgvblankrate == 75)
	{
		cboRtgRefreshRate->setSelected(
			changed_prefs.rtgvblankrate == 0 ? 0 :
			changed_prefs.rtgvblankrate == -1 ? 1 :
			changed_prefs.rtgvblankrate == -2 ? 0 :
			changed_prefs.rtgvblankrate == 50 ? 2 :
			changed_prefs.rtgvblankrate == 60 ? 3 :
			changed_prefs.rtgvblankrate == 70 ? 4 :
			changed_prefs.rtgvblankrate == 75 ? 5 : 0);
	}
	else
	{
		cboRtgRefreshRate->setSelected(0);
	}

	cboRtg16bitModes->setSelected(
		changed_prefs.picasso96_modeflags & RGBFF_R5G6B5PC ? 2 :
		changed_prefs.picasso96_modeflags & RGBFF_R5G5B5PC ? 3 :
		changed_prefs.picasso96_modeflags & RGBFF_R5G6B5 ? 4 :
		changed_prefs.picasso96_modeflags & RGBFF_R5G5B5 ? 5 :
		changed_prefs.picasso96_modeflags & RGBFF_B5G6R5PC ? 6 :
		changed_prefs.picasso96_modeflags & RGBFF_B5G5R5PC ? 7 : 0);

	cboRtg32bitModes->setSelected(
		changed_prefs.picasso96_modeflags & RGBFF_A8R8G8B8 ? 2 :
		changed_prefs.picasso96_modeflags & RGBFF_A8B8G8R8 ? 3 :
		changed_prefs.picasso96_modeflags & RGBFF_R8G8B8A8 ? 4 :
		changed_prefs.picasso96_modeflags & RGBFF_B8G8R8A8 ? 5 : 0);

	// Only double-buffering is supported in SDL2, which is the default here anyway
	cboRtgBufferMode->setEnabled(false); 
	cboRtgBufferMode->setSelected(changed_prefs.gfx_apmode[1].gfx_backbuffers == 0 ? 0 :
		changed_prefs.gfx_apmode[1].gfx_backbuffers - 1);

	cboRtgAspectRatio->setEnabled(false); // Not implemented yet
	cboRtgAspectRatio->setSelected(changed_prefs.rtgscaleaspectratio == 0 ? 0 : 1);
}

bool HelpPanelRTG(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("This panel allows you add a graphics card to your emulated Amiga");
	helptext.emplace_back("(otherwise known as an RTG card). Currently on the UAE Zorro II");
	helptext.emplace_back("and Zorro III options are available, as Board types.");
	helptext.emplace_back("These correspond to the \"uaegfx\" driver, of the P96 package.");
	helptext.emplace_back("You can either use the free Aminet version of P96 or the newer iComp");
	helptext.emplace_back("versions, as both as supported.");
	helptext.emplace_back(" ");
	helptext.emplace_back("The color modes dropdowns allow you to choose which pixel format");
	helptext.emplace_back("will be used. For optimized performance, Amiberry uses the same");
	helptext.emplace_back("pixel format always (RGB565 for 16-bit and BGRA32 for 32-bit modes).");
	helptext.emplace_back(" ");
	helptext.emplace_back("Some options are not implemented, so they appear as disabled.");
	helptext.emplace_back(" ");
	helptext.emplace_back("     Hardware vertical blank interrupt: If this option is enabled,");
	helptext.emplace_back("     an interrupt will be triggered in the emulated system on each");
	helptext.emplace_back("     vsync event under RTG modes.");
	helptext.emplace_back(" ");
	helptext.emplace_back("     Hardware sprite emulation: This option will use the system");
	helptext.emplace_back("     cursor as the Amiga cursor in RTG modes, if supported. Please");
	helptext.emplace_back("     note that some systems do not support this, and you will only");
	helptext.emplace_back("     get a blank cursor instead, if this option is enabled.");
	helptext.emplace_back("     Due to SDL2 limitations, this currently only works well when");
	helptext.emplace_back("     the \"Virtual Mouse driver\" option from the Input panel, is");
	helptext.emplace_back("     enabled as well.");
	helptext.emplace_back(" ");
	helptext.emplace_back("     Multithreaded: This option will set the RTG rendering to be");
	helptext.emplace_back("     done in a separate thread. This might improve the performance");
	helptext.emplace_back("     in some cases, especially with higher resolutions.");
	helptext.emplace_back(" ");
	helptext.emplace_back("The Refresh rate option allows you to select how the refresh rate");
	helptext.emplace_back("for the emulated graphics card should behave:");
	helptext.emplace_back(" ");
	helptext.emplace_back("     Chipset: (the default option) this will use the native chipset");
	helptext.emplace_back("              selected, for the refresh rate - PAL will use 50 Hz");
	helptext.emplace_back("              NTSC will use 60 Hz.");
	helptext.emplace_back(" ");
	helptext.emplace_back("     Default: this will use the default refresh rate for the selected");
	helptext.emplace_back("              RTG card. Usually that's 60 Hz always.");
	helptext.emplace_back(" ");
	helptext.emplace_back("     50/60/70/75: Use the selected refresh rate.");
	helptext.emplace_back(" ");
	helptext.emplace_back(" In most cases, leaving this to the default value (which is Chipset) should work fine.");
	helptext.emplace_back(" ");
	return true;
}