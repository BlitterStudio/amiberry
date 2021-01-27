#include <cstdio>
#include <cstdlib>

#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "options.h"
#include "gfxboard.h"
#include "gui_handling.h"

class StringListModel : public gcn::ListModel
{
	std::vector<std::string> values;
public:
	StringListModel(const char* entries[], const int count)
	{
		for (auto i = 0; i < count; ++i)
			values.emplace_back(entries[i]);
	}

	int getNumberOfElements() override
	{
		return values.size();
	}

	int add_element(const char* Elem)
	{
		values.emplace_back(Elem);
		return 0;
	}

	std::string getElementAt(const int i) override
	{
		if (i < 0 || i >= static_cast<int>(values.size()))
			return "---";
		return values[i];
	}
};

const char* rtg_boards[] = { "-", "UAE Zorro III" };
StringListModel rtg_boards_list(rtg_boards, 2);

const char* rtg_refreshrates[] = { "Chipset", "Default", "50", "60", "70", "75" };
StringListModel rtg_refreshrates_list(rtg_refreshrates, 6);

const char* rtg_buffermodes[] = { "Double buffering", "Triple buffering" };
StringListModel rtg_buffermodes_list(rtg_buffermodes, 2);

const char* rtg_aspectratios[] = { "Disabled", "Automatic" };
StringListModel rtg_aspectratios_list(rtg_aspectratios, 2);

const char* rtg_16bit_modes[] = { "(15/16bit)", "All", "R5G6B5PC (*)", "R5G5B5PC", "R5G6B5", "R5G5B5", "B5G6R5PC", "B5G5R5PC" };
StringListModel rtg_16bit_modes_list(rtg_16bit_modes, 8);

const char* rtg_32bit_modes[] = { "(32bit)", "All", "A8R8G8B8", "A8B8G8R8", "R8G8B8A8", "B8G8R8A8 (*)" };
StringListModel rtg_32bit_modes_list(rtg_32bit_modes, 6);


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
		int v;
		uae_u32 mask = changed_prefs.picasso96_modeflags;
		
		if (action_event.getSource() == cboBoard)
		{
			if (cboBoard->getSelected() == 0)
			{
				changed_prefs.rtgboards[0].rtgmem_type = 1;
				changed_prefs.rtgboards[0].rtgmem_size = 0;
			}
			else if (cboBoard->getSelected() == 1)
			{
				changed_prefs.rtgboards[0].rtgmem_type = gfxboard_get_id_from_index(1);
				if (changed_prefs.rtgboards[0].rtgmem_size == 0)
					changed_prefs.rtgboards[0].rtgmem_size = 4096 * 1024;
			}
			cfgfile_compatibility_rtg(&changed_prefs);
		}

		else if (action_event.getSource() == sldGfxmem)
		{
			changed_prefs.rtgboards[0].rtgmem_size = memsizes[msi_gfx[static_cast<int>(sldGfxmem->getValue())]];
			changed_prefs.rtgboards[0].rtgmem_type = GFXBOARD_UAE_Z3;
		}

		else if (action_event.getSource() == chkRtgMatchDepth)
			changed_prefs.rtgmatchdepth = chkRtgMatchDepth->isSelected();

		else if (action_event.getSource() == chkRtgAutoscale)
		{
			if (chkRtgAutoscale->isSelected())
			{
				changed_prefs.gf[1].gfx_filter_autoscale == RTG_MODE_SCALE;
			}
		}

		else if (action_event.getSource() == chkRtgAllowScaling)
			changed_prefs.rtgallowscaling = chkRtgAllowScaling->isSelected();

		else if (action_event.getSource() == chkRtgAlwaysCenter)
		{
			if (chkRtgAlwaysCenter->isSelected())
			{
				changed_prefs.gf[1].gfx_filter_autoscale == RTG_MODE_CENTER;
			}
		}

		else if (action_event.getSource() == chkRtgHardwareInterrupt)
			changed_prefs.rtg_hardwareinterrupt = chkRtgHardwareInterrupt->isSelected();

		else if (action_event.getSource() == chkRtgHardwareSprite)
			changed_prefs.rtg_hardwaresprite = chkRtgHardwareSprite->isSelected();

		else if (action_event.getSource() == cboRtgRefreshRate)
			changed_prefs.rtgvblankrate = cboRtgRefreshRate->getSelected() == 0 ? 0 :
			cboRtgRefreshRate->getSelected() == 1 ? -1 : atoi(rtg_refreshrates[cboRtgRefreshRate->getSelected()]);
		
		else if (action_event.getSource() == cboRtgBufferMode)
			changed_prefs.gfx_apmode[1].gfx_backbuffers = cboRtgBufferMode->getSelected() + 1;

		else if (action_event.getSource() == cboRtgAspectRatio)
			changed_prefs.rtgscaleaspectratio = cboRtgAspectRatio->getSelected() == 0 ? 0 : -1;

		mask &= ~RGBFF_CLUT;
		mask |= RGBFF_CLUT;
		v = cboRtg16bitModes->getSelected();
		mask &= ~(RGBFF_R5G6B5PC | RGBFF_R5G5B5PC | RGBFF_R5G6B5 | RGBFF_R5G5B5 | RGBFF_B5G6R5PC | RGBFF_B5G5R5PC);
		if (v == 1)
			mask |= RGBFF_R5G6B5PC | RGBFF_R5G6B5PC | RGBFF_R5G5B5PC | RGBFF_R5G6B5 | RGBFF_R5G5B5 | RGBFF_B5G6R5PC | RGBFF_B5G5R5PC;
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

		changed_prefs.picasso96_modeflags = mask;
		
		RefreshPanelRTG();
	}
};

RTGActionListener* rtg_action_listener;

void InitPanelRTG(const struct _ConfigCategory& category)
{
	int sld_width;
	int marker_length;
#ifdef ANDROID
	sldWidth = 150;
	markerLength = 30;
#else
	sld_width = 110;
	marker_length = 20;
#endif
	rtg_action_listener = new RTGActionListener();

	lblBoard = new gcn::Label("Board:");
	lblBoard->setAlignment(gcn::Graphics::LEFT);

	cboBoard = new gcn::DropDown(&rtg_boards_list);
	cboBoard->setSize(250, cboBoard->getHeight());
	cboBoard->setBaseColor(gui_baseCol);
	cboBoard->setBackgroundColor(colTextboxBackground);
	cboBoard->setId("cboBoard");
	cboBoard->addActionListener(rtg_action_listener);

	lblGfxmem = new gcn::Label("VRAM size:");
	sldGfxmem = new gcn::Slider(0, 8);
	sldGfxmem->setSize(sld_width, SLIDER_HEIGHT);
	sldGfxmem->setBaseColor(gui_baseCol);
	sldGfxmem->setMarkerLength(marker_length);
	sldGfxmem->setStepLength(1);
	sldGfxmem->setId("Gfxmem");
	sldGfxmem->addActionListener(rtg_action_listener);
	lblGfxsize = new gcn::Label("None   ");

	chkRtgMatchDepth = new gcn::CheckBox("Match host and RTG color depth if possible");
	chkRtgMatchDepth->setId("chkRtgMatchDepth");
	chkRtgMatchDepth->addActionListener(rtg_action_listener);
	chkRtgMatchDepth->setEnabled(false); // Not implemented yet

	chkRtgAutoscale = new gcn::CheckBox("Scale if smaller than display size setting");
	chkRtgAutoscale->setId("chkRtgAutoscale");
	chkRtgAutoscale->addActionListener(rtg_action_listener);
	chkRtgAutoscale->setEnabled(false); // Not implemented yet
	
	chkRtgAllowScaling = new gcn::CheckBox("Always scale in windowed mode");
	chkRtgAllowScaling->setId("chkRtgAllowScaling");
	chkRtgAllowScaling->addActionListener(rtg_action_listener);
	chkRtgAllowScaling->setEnabled(false); // Not implemented yet
	
	chkRtgAlwaysCenter = new gcn::CheckBox("Always center");
	chkRtgAlwaysCenter->setId("chkRtgAlwaysCenter");
	chkRtgAlwaysCenter->addActionListener(rtg_action_listener);
	chkRtgAlwaysCenter->setEnabled(false); // Not implemented yet
	
	chkRtgHardwareInterrupt = new gcn::CheckBox("Hardware vertical blank interrupt");
	chkRtgHardwareInterrupt->setId("chkRtgHardwareInterrupt");
	chkRtgHardwareInterrupt->addActionListener(rtg_action_listener);

	chkRtgHardwareSprite = new gcn::CheckBox("Hardware sprite emulation");
	chkRtgHardwareSprite->setId("chkRtgHardwareSprite");
	chkRtgHardwareSprite->addActionListener(rtg_action_listener);
	chkRtgHardwareSprite->setEnabled(false); // Not implemented yet

	lblRtgRefreshRate = new gcn::Label("Refresh rate:");
	lblRtgRefreshRate->setAlignment(gcn::Graphics::LEFT);
	cboRtgRefreshRate = new gcn::DropDown(&rtg_refreshrates_list);
	cboRtgRefreshRate->setSize(150, cboRtgRefreshRate->getHeight());
	cboRtgRefreshRate->setBaseColor(gui_baseCol);
	cboRtgRefreshRate->setBackgroundColor(colTextboxBackground);
	cboRtgRefreshRate->setId("cboRtgRefreshRate");
	cboRtgRefreshRate->addActionListener(rtg_action_listener);

	lblRtgBufferMode = new gcn::Label("Buffer mode:");
	lblRtgBufferMode->setAlignment(gcn::Graphics::LEFT);
	cboRtgBufferMode = new gcn::DropDown(&rtg_buffermodes_list);
	cboRtgBufferMode->setSize(150, cboRtgBufferMode->getHeight());
	cboRtgBufferMode->setBaseColor(gui_baseCol);
	cboRtgBufferMode->setBackgroundColor(colTextboxBackground);
	cboRtgBufferMode->setId("cboRtgBufferMode");
	cboRtgBufferMode->addActionListener(rtg_action_listener);
	cboRtgBufferMode->setEnabled(false); // Not implemented yet

	lblRtgAspectRatio = new gcn::Label("Aspect Ratio:");
	lblRtgAspectRatio->setAlignment(gcn::Graphics::LEFT);
	cboRtgAspectRatio = new gcn::DropDown(&rtg_aspectratios_list);
	cboRtgAspectRatio->setSize(150, cboRtgAspectRatio->getHeight());
	cboRtgAspectRatio->setBaseColor(gui_baseCol);
	cboRtgAspectRatio->setBackgroundColor(colTextboxBackground);
	cboRtgAspectRatio->setId("cboRtgAspectRatio");
	cboRtgAspectRatio->addActionListener(rtg_action_listener);
	cboRtgAspectRatio->setEnabled(false); // Not implemented yet

	lblRtgColorModes = new gcn::Label("Color modes:");
	lblRtgColorModes->setAlignment(gcn::Graphics::LEFT);

	cboRtg16bitModes = new gcn::DropDown(&rtg_16bit_modes_list);
	cboRtg16bitModes->setSize(150, cboRtg16bitModes->getHeight());
	cboRtg16bitModes->setBaseColor(gui_baseCol);
	cboRtg16bitModes->setBackgroundColor(colTextboxBackground);
	cboRtg16bitModes->setId("cboRtg16bitModes");
	cboRtg16bitModes->addActionListener(rtg_action_listener);

	cboRtg32bitModes = new gcn::DropDown(&rtg_32bit_modes_list);
	cboRtg32bitModes->setSize(150, cboRtg32bitModes->getHeight());
	cboRtg32bitModes->setBaseColor(gui_baseCol);
	cboRtg32bitModes->setBackgroundColor(colTextboxBackground);
	cboRtg32bitModes->setId("cboRtg32bitModes");
	cboRtg32bitModes->addActionListener(rtg_action_listener);
	
	auto posY = DISTANCE_BORDER;
	
	category.panel->add(lblBoard, DISTANCE_BORDER, posY);
	category.panel->add(lblRtgColorModes, lblBoard->getX() + lblBoard->getWidth() + DISTANCE_NEXT_X * 20, posY);
	posY += lblBoard->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(cboBoard, DISTANCE_BORDER, posY);
	category.panel->add(cboRtg16bitModes, lblRtgColorModes->getX() - DISTANCE_NEXT_X, posY);
	posY += cboBoard->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblGfxmem, DISTANCE_BORDER, posY);
	category.panel->add(sldGfxmem, lblGfxmem->getWidth() + DISTANCE_NEXT_X + 8, posY);
	category.panel->add(lblGfxsize, lblGfxmem->getWidth() + DISTANCE_NEXT_X + sldGfxmem->getWidth() + 12, posY);
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
	posY += chkRtgHardwareSprite->getHeight() + DISTANCE_NEXT_Y * 2;

	category.panel->add(lblRtgRefreshRate, DISTANCE_BORDER, posY);
	category.panel->add(lblRtgBufferMode, lblRtgRefreshRate->getX() + lblRtgRefreshRate->getWidth() + DISTANCE_NEXT_X * 5, posY);
	category.panel->add(lblRtgAspectRatio, lblRtgBufferMode->getX() + lblRtgBufferMode->getWidth() + DISTANCE_NEXT_X * 6, posY);
	posY += lblRtgRefreshRate->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(cboRtgRefreshRate, DISTANCE_BORDER, posY);
	category.panel->add(cboRtgBufferMode, cboRtgRefreshRate->getX() + cboRtgRefreshRate->getWidth() + DISTANCE_NEXT_X * 2, posY);
	category.panel->add(cboRtgAspectRatio, cboRtgBufferMode->getX() + cboRtgBufferMode->getWidth() + DISTANCE_NEXT_X * 2, posY);
	posY += cboRtgRefreshRate->getHeight() + DISTANCE_NEXT_Y;
	
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
	struct rtgboardconfig* rbc = &changed_prefs.rtgboards[0];

	if (rbc->rtgmem_size == 0)
		cboBoard->setSelected(0);
	else
		cboBoard->setSelected(1);

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
		}
		sldGfxmem->setValue(mem_size);
		lblGfxsize->setCaption(memsize_names[msi_gfx[mem_size]]);
	}

	chkRtgMatchDepth->setSelected(changed_prefs.rtgmatchdepth);
	chkRtgAutoscale->setSelected(changed_prefs.gf[1].gfx_filter_autoscale == RTG_MODE_SCALE);
	chkRtgAlwaysCenter->setSelected(changed_prefs.gf[1].gfx_filter_autoscale == RTG_MODE_CENTER);
	chkRtgAllowScaling->setSelected(changed_prefs.rtgallowscaling);
	chkRtgHardwareInterrupt->setSelected(changed_prefs.rtg_hardwareinterrupt);
	chkRtgHardwareSprite->setSelected(changed_prefs.rtg_hardwaresprite);

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
	
	cboRtgBufferMode->setSelected(changed_prefs.gfx_apmode[1].gfx_backbuffers == 0 ? 0 :
		changed_prefs.gfx_apmode[1].gfx_backbuffers - 1);

	cboRtgAspectRatio->setSelected(changed_prefs.rtgscaleaspectratio == 0 ? 0 : 1);
}

bool HelpPanelRTG(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("\"RTG board\" is the graphics memory used by Picasso96 and only available if");
	helptext.emplace_back("a 32 bit CPU is selected. If you select some memory for this type,");
	helptext.emplace_back("the Z3 RTG board will be activated.");
	return true;
}