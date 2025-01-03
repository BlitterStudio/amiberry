#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"
#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "gui_handling.h"
#include "target.h"

static gcn::Window* grpRAM;
static gcn::Label* lblChipmem;
static gcn::Label* lblChipsize;
static gcn::Slider* sldChipmem;
static gcn::Label* lblSlowmem;
static gcn::Label* lblSlowsize;
static gcn::Slider* sldSlowmem;
static gcn::Label* lblFastmem;
static gcn::Label* lblFastsize;
static gcn::Slider* sldFastmem;
static gcn::Label* lblZ3mem;
static gcn::Label* lblZ3size;
static gcn::Slider* sldZ3mem;
static gcn::Label* lblZ3chip;
static gcn::Label* lblZ3chip_size;
static gcn::Slider* sldZ3chip;
static gcn::Label* lblMbResLowmem;
static gcn::Label* lblMbResLowsize;
static gcn::Slider* sldMbResLowmem;
static gcn::Label* lblMbResHighmem;
static gcn::Label* lblMbResHighsize;
static gcn::Slider* sldMbResHighmem;

class MemorySliderActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == sldChipmem)
		{
			changed_prefs.chipmem.size = memsizes[msi_chip[static_cast<int>(sldChipmem->getValue())]];
			if (changed_prefs.chipmem.size > 0x200000 && changed_prefs.fastmem[0].size > 0)
				changed_prefs.fastmem[0].size = 0;
		}

		else if (actionEvent.getSource() == sldSlowmem)
		{
			changed_prefs.bogomem.size = memsizes[msi_bogo[static_cast<int>(sldSlowmem->getValue())]];
		}

		else if (actionEvent.getSource() == sldFastmem)
		{
			changed_prefs.fastmem[0].size = memsizes[msi_fast[static_cast<int>(sldFastmem->getValue())]];
			if (changed_prefs.fastmem[0].size > 0 && changed_prefs.chipmem.size > 0x200000)
				changed_prefs.chipmem.size = 0x200000;
		}

		else if (actionEvent.getSource() == sldZ3mem)
		{
			changed_prefs.z3fastmem[0].size = memsizes[msi_z3fast[static_cast<int>(sldZ3mem->getValue())]];
			if (changed_prefs.z3fastmem[0].size > max_z3fastmem)
				changed_prefs.z3fastmem[0].size = max_z3fastmem;
		}

		else if (actionEvent.getSource() == sldZ3chip)
		{
			changed_prefs.z3chipmem.size = memsizes[msi_z3chip[static_cast<int>(sldZ3chip->getValue())]];
			if (changed_prefs.z3chipmem.size > max_z3fastmem)
				changed_prefs.z3chipmem.size = max_z3fastmem;
		}

		else if (actionEvent.getSource() == sldMbResLowmem)
		{
			changed_prefs.mbresmem_low.size = memsizes[msi_mb[static_cast<int>(sldMbResLowmem->getValue())]];
			if (currprefs.mbresmem_low.size != changed_prefs.mbresmem_low.size)
				disable_resume();
		}

		else if (actionEvent.getSource() == sldMbResHighmem)
		{
			changed_prefs.mbresmem_high.size = memsizes[msi_mb[static_cast<int>(sldMbResHighmem->getValue())]];
			if (currprefs.mbresmem_high.size != changed_prefs.mbresmem_high.size)
				disable_resume();
		}

		RefreshPanelRAM();
	}
};

static MemorySliderActionListener* memorySliderActionListener;


void InitPanelRAM(const config_category& category)
{
	memorySliderActionListener = new MemorySliderActionListener();
	constexpr int sld_width = 150;
	int marker_length = 20;

	lblChipmem = new gcn::Label("Chip:");
	sldChipmem = new gcn::Slider(0, 6);
	sldChipmem->setSize(sld_width, SLIDER_HEIGHT);
	sldChipmem->setBaseColor(gui_base_color);
	sldChipmem->setBackgroundColor(gui_background_color);
	sldChipmem->setForegroundColor(gui_foreground_color);
	sldChipmem->setMarkerLength(marker_length);
	sldChipmem->setStepLength(1);
	sldChipmem->setId("sldChipmem");
	sldChipmem->addActionListener(memorySliderActionListener);
	lblChipsize = new gcn::Label("None");

	lblSlowmem = new gcn::Label("Slow:");
	sldSlowmem = new gcn::Slider(0, 4);
	sldSlowmem->setSize(sld_width, SLIDER_HEIGHT);
	sldSlowmem->setBaseColor(gui_base_color);
	sldSlowmem->setBackgroundColor(gui_background_color);
	sldSlowmem->setForegroundColor(gui_foreground_color);
	sldSlowmem->setMarkerLength(marker_length);
	sldSlowmem->setStepLength(1);
	sldSlowmem->setId("sldSlowmem");
	sldSlowmem->addActionListener(memorySliderActionListener);
	lblSlowsize = new gcn::Label("None");

	lblFastmem = new gcn::Label("Z2 Fast:");
	sldFastmem = new gcn::Slider(0, 8);
	sldFastmem->setSize(sld_width, SLIDER_HEIGHT);
	sldFastmem->setBaseColor(gui_base_color);
	sldFastmem->setBackgroundColor(gui_background_color);
	sldFastmem->setForegroundColor(gui_foreground_color);
	sldFastmem->setMarkerLength(marker_length);
	sldFastmem->setStepLength(1);
	sldFastmem->setId("sldFastmem");
	sldFastmem->addActionListener(memorySliderActionListener);
	lblFastsize = new gcn::Label("None");

	lblZ3mem = new gcn::Label("Z3 Fast:");
	if (can_have_1gb())
		sldZ3mem = new gcn::Slider(0, 11);
	else
		sldZ3mem = new gcn::Slider(0, 10);
	sldZ3mem->setSize(sld_width, SLIDER_HEIGHT);
	sldZ3mem->setBaseColor(gui_base_color);
	sldZ3mem->setBackgroundColor(gui_background_color);
	sldZ3mem->setForegroundColor(gui_foreground_color);
	sldZ3mem->setMarkerLength(marker_length);
	sldZ3mem->setStepLength(1);
	sldZ3mem->setId("sldZ3mem");
	sldZ3mem->addActionListener(memorySliderActionListener);
	lblZ3size = new gcn::Label("None");

	lblZ3chip = new gcn::Label("32-bit Chip RAM:");
	if (can_have_1gb())
		sldZ3chip = new gcn::Slider(0, 9);
	else
		sldZ3chip = new gcn::Slider(0, 8);
	sldZ3chip->setSize(sld_width, SLIDER_HEIGHT);
	sldZ3chip->setBaseColor(gui_base_color);
	sldZ3chip->setBackgroundColor(gui_background_color);
	sldZ3chip->setForegroundColor(gui_foreground_color);
	sldZ3chip->setMarkerLength(marker_length);
	sldZ3chip->setStepLength(1);
	sldZ3chip->setId("sldZ3chip");
	sldZ3chip->addActionListener(memorySliderActionListener);
	lblZ3chip_size = new gcn::Label("None");

	lblMbResLowmem = new gcn::Label("Motherboard Fast RAM:");
	sldMbResLowmem = new gcn::Slider(0, 7);
	sldMbResLowmem->setSize(sld_width, SLIDER_HEIGHT);
	sldMbResLowmem->setBaseColor(gui_base_color);
	sldMbResLowmem->setBackgroundColor(gui_background_color);
	sldMbResLowmem->setForegroundColor(gui_foreground_color);
	sldMbResLowmem->setMarkerLength(marker_length);
	sldMbResLowmem->setStepLength(1);
	sldMbResLowmem->setId("sldMbResLowmem");
	sldMbResLowmem->addActionListener(memorySliderActionListener);
	lblMbResLowsize = new gcn::Label("None");

	lblMbResHighmem = new gcn::Label("Processor slot Fast RAM:");
	sldMbResHighmem = new gcn::Slider(0, 8);
	sldMbResHighmem->setSize(sld_width, SLIDER_HEIGHT);
	sldMbResHighmem->setBaseColor(gui_base_color);
	sldMbResHighmem->setBackgroundColor(gui_background_color);
	sldMbResHighmem->setForegroundColor(gui_foreground_color);
	sldMbResHighmem->setMarkerLength(marker_length);
	sldMbResHighmem->setStepLength(1);
	sldMbResHighmem->setId("sldMbResHighmem");
	sldMbResHighmem->addActionListener(memorySliderActionListener);
	lblMbResHighsize = new gcn::Label("None");

	grpRAM = new gcn::Window("Memory Settings");
	grpRAM->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);

	int posY = 10;
	grpRAM->add(lblChipmem, 10, posY);
	grpRAM->add(sldChipmem, lblMbResHighmem->getWidth() + DISTANCE_NEXT_X, posY);
	grpRAM->add(lblChipsize, lblMbResHighmem->getWidth() + DISTANCE_NEXT_X + sldChipmem->getWidth() + 12, posY);
	posY += sldChipmem->getHeight() + DISTANCE_NEXT_Y;

	grpRAM->add(lblSlowmem, 10, posY);
	grpRAM->add(sldSlowmem, lblMbResHighmem->getWidth() + DISTANCE_NEXT_X, posY);
	grpRAM->add(lblSlowsize, lblMbResHighmem->getWidth() + DISTANCE_NEXT_X + sldSlowmem->getWidth() + 12, posY);
	posY += sldSlowmem->getHeight() + DISTANCE_NEXT_Y;

	grpRAM->add(lblFastmem, 10, posY);
	grpRAM->add(sldFastmem, lblMbResHighmem->getWidth() + DISTANCE_NEXT_X, posY);
	grpRAM->add(lblFastsize, lblMbResHighmem->getWidth() + DISTANCE_NEXT_X + sldFastmem->getWidth() + 12, posY);
	posY += sldFastmem->getHeight() + DISTANCE_NEXT_Y;

	grpRAM->add(lblZ3mem, 10, posY);
	grpRAM->add(sldZ3mem, lblMbResHighmem->getWidth() + DISTANCE_NEXT_X, posY);
	grpRAM->add(lblZ3size, lblMbResHighmem->getWidth() + DISTANCE_NEXT_X + sldZ3mem->getWidth() + 12, posY);
	posY += sldZ3mem->getHeight() + DISTANCE_NEXT_Y;

	grpRAM->add(lblZ3chip, 10, posY);
	grpRAM->add(sldZ3chip, lblMbResHighmem->getWidth() + DISTANCE_NEXT_X, posY);
	grpRAM->add(lblZ3chip_size, lblMbResHighmem->getWidth() + DISTANCE_NEXT_X + sldZ3chip->getWidth() + 12, posY);
	posY += sldZ3chip->getHeight() + DISTANCE_NEXT_Y;

	grpRAM->add(lblMbResLowmem, 10, posY);
	grpRAM->add(sldMbResLowmem, lblMbResHighmem->getWidth() + DISTANCE_NEXT_X, posY);
	grpRAM->add(lblMbResLowsize, lblMbResHighmem->getWidth() + DISTANCE_NEXT_X + sldMbResLowmem->getWidth() + 12, posY);
	posY += sldMbResLowmem->getHeight() + DISTANCE_NEXT_Y;

	grpRAM->add(lblMbResHighmem, 10, posY);
	grpRAM->add(sldMbResHighmem, lblMbResHighmem->getWidth() + DISTANCE_NEXT_X, posY);
	grpRAM->add(lblMbResHighsize, lblMbResHighmem->getWidth() + DISTANCE_NEXT_X + sldMbResHighmem->getWidth() + 12,
	            posY);
	posY += sldMbResHighmem->getHeight() + DISTANCE_NEXT_Y;

	grpRAM->setMovable(false);
	grpRAM->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2, TITLEBAR_HEIGHT + posY + DISTANCE_BORDER * 2);
	grpRAM->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpRAM->setBaseColor(gui_base_color);
	grpRAM->setForegroundColor(gui_foreground_color);

	category.panel->add(grpRAM);

	RefreshPanelRAM();
}


void ExitPanelRAM()
{
	delete lblChipmem;
	delete sldChipmem;
	delete lblChipsize;
	delete lblSlowmem;
	delete sldSlowmem;
	delete lblSlowsize;
	delete lblFastmem;
	delete sldFastmem;
	delete lblFastsize;
	delete lblZ3mem;
	delete sldZ3mem;
	delete lblZ3size;
	delete lblZ3chip;
	delete sldZ3chip;
	delete lblZ3chip_size;
	delete lblMbResLowmem;
	delete sldMbResLowmem;
	delete lblMbResLowsize;
	delete lblMbResHighmem;
	delete sldMbResHighmem;
	delete lblMbResHighsize;
	delete grpRAM;
	delete memorySliderActionListener;
}

void RefreshPanelRAM()
{
	int i;

	for (i = 0; i < 7; ++i)
	{
		if (changed_prefs.chipmem.size == memsizes[msi_chip[i]])
		{
			sldChipmem->setValue(i);
			lblChipsize->setCaption(memsize_names[msi_chip[i]]);
			lblChipsize->adjustSize();
			break;
		}
	}

	for (i = 0; i < 5; ++i)
	{
		if (changed_prefs.bogomem.size == memsizes[msi_bogo[i]])
		{
			sldSlowmem->setValue(i);
			lblSlowsize->setCaption(memsize_names[msi_bogo[i]]);
			lblSlowsize->adjustSize();
			break;
		}
	}

	for (i = 0; i < 9; ++i)
	{
		if (changed_prefs.fastmem[0].size == memsizes[msi_fast[i]])
		{
			sldFastmem->setValue(i);
			lblFastsize->setCaption(memsize_names[msi_fast[i]]);
			lblFastsize->adjustSize();
			break;
		}
	}

	if (changed_prefs.address_space_24)
	{
		// Disable Z3 and RTG memory
		sldZ3mem->setEnabled(false);
		lblZ3mem->setEnabled(false);
		lblZ3size->setEnabled(false);
		lblZ3size->setCaption("N/A");
		lblZ3size->adjustSize();
		sldZ3chip->setEnabled(false);
		lblZ3chip->setEnabled(false);
		lblZ3chip_size->setEnabled(false);
		lblZ3chip_size->setCaption("N/A");
		lblZ3chip_size->adjustSize();
	}
	else
	{
		sldZ3mem->setEnabled(true);
		lblZ3mem->setEnabled(true);
		lblZ3size->setEnabled(true);
		lblZ3size->setCaption("None");
		lblZ3size->adjustSize();
		
		auto counter = 11;
		if (can_have_1gb())
			counter = 12;
		for (i = 0; i < counter; ++i)
		{
			if (changed_prefs.z3fastmem[0].size == memsizes[msi_z3fast[i]])
			{
				sldZ3mem->setValue(i);
				lblZ3size->setCaption(memsize_names[msi_z3fast[i]]);
				lblZ3size->adjustSize();
				break;
			}
		}

		sldZ3chip->setEnabled(true);
		lblZ3chip->setEnabled(true);
		lblZ3chip_size->setEnabled(true);
		if (can_have_1gb())
			counter = 10;
		else
			counter = 9;
		for (i = 0; i < counter; ++i)
		{
			if (changed_prefs.z3chipmem.size == memsizes[msi_z3chip[i]])
			{
				sldZ3chip->setValue(i);
				lblZ3chip_size->setCaption(memsize_names[msi_z3chip[i]]);
				lblZ3chip_size->adjustSize();
				break;;
			}
		}
	}

	for (i = 0; i < 8; ++i)
	{
		if (changed_prefs.mbresmem_low.size == memsizes[msi_mb[i]])
		{
			sldMbResLowmem->setValue(i);
			lblMbResLowsize->setCaption(memsize_names[msi_mb[i]]);
			lblMbResLowsize->adjustSize();
			break;
		}
	}

	for (i = 0; i < 9; ++i)
	{
		if (changed_prefs.mbresmem_high.size == memsizes[msi_mb[i]])
		{
			sldMbResHighmem->setValue(i);
			lblMbResHighsize->setCaption(memsize_names[msi_mb[i]]);
			lblMbResHighsize->adjustSize();
			break;
		}
	}
}


bool HelpPanelRAM(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("Select the amount of RAM for each type you want to emulate in your Amiga.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Slow\" is the simple memory extension of an Amiga 500.");
	helptext.emplace_back("\"Z2 Fast\" is real fast memory in 24 bit address space.");
	helptext.emplace_back("\"Z3 Fast\" is real fast memory in 32 bit address space and only available if");
	helptext.emplace_back("a 32 bit CPU is selected.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Motherboard and processor board memory is only detected by the Amiga if");
	helptext.emplace_back("you choose the correct Kickstart ROM (A3000/A4000).");
	return true;
}
