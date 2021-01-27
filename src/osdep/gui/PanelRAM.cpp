#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include "SelectorEntry.hpp"
#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "gui_handling.h"
#include "target.h"

static const char* ChipMem_list[] = {"512 K", "1 MB", "2 MB", "4 MB", "8 MB"};
static unsigned int ChipMem_values[] = {0x080000, 0x100000, 0x200000, 0x400000, 0x800000};
static const char* SlowMem_list[] = {"None", "512 K", "1 MB", "1.5 MB", "1.8 MB"};
static unsigned int SlowMem_values[] = {0x000000, 0x080000, 0x100000, 0x180000, 0x1c0000};
static const char* FastMem_list[] = {
	"None", "1 MB", "2 MB", "4 MB", "8 MB", "16 MB", "32 MB", "64 MB", "128 MB", "256 MB", "512 MB", "1 GB"
};
static unsigned int FastMem_values[] = {
	0x000000, 0x100000, 0x200000, 0x400000, 0x800000, 0x1000000, 0x2000000, 0x4000000, 0x8000000, 0x10000000, 0x20000000, 0x40000000
};
static const char* A3000LowMem_list[] = {"None", "8 MB", "16 MB"};
static unsigned int A3000LowMem_values[] = {0x080000, 0x800000, 0x1000000};
static const char* A3000HighMem_list[] = {"None", "8 MB", "16 MB", "32 MB", "64 MB", "128 MB"};
static unsigned int A3000HighMem_values[] = {0x080000, 0x800000, 0x1000000, 0x2000000, 0x4000000, 0x8000000};

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
static gcn::Label* lblA3000Lowmem;
static gcn::Label* lblA3000Lowsize;
static gcn::Slider* sldA3000Lowmem;
static gcn::Label* lblA3000Highmem;
static gcn::Label* lblA3000Highsize;
static gcn::Slider* sldA3000Highmem;


class MemorySliderActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == sldChipmem)
		{
			changed_prefs.chipmem.size = ChipMem_values[static_cast<int>(sldChipmem->getValue())];
			if ((changed_prefs.chipmem.size > 0x200000) && (changed_prefs.fastmem[0].size > 0))
				changed_prefs.fastmem[0].size = 0;
		}

		if (actionEvent.getSource() == sldSlowmem)
		{
			changed_prefs.bogomem.size = SlowMem_values[static_cast<int>(sldSlowmem->getValue())];
		}

		if (actionEvent.getSource() == sldFastmem)
		{
			changed_prefs.fastmem[0].size = FastMem_values[static_cast<int>(sldFastmem->getValue())];
			if (changed_prefs.fastmem[0].size > 0 && changed_prefs.chipmem.size > 0x200000)
				changed_prefs.chipmem.size = 0x200000;
		}

		if (actionEvent.getSource() == sldZ3mem)
		{
			changed_prefs.z3fastmem[0].size = FastMem_values[static_cast<int>(sldZ3mem->getValue())];
			if (changed_prefs.z3fastmem[0].size > max_z3fastmem)
				changed_prefs.z3fastmem[0].size = max_z3fastmem;
		}

		if (actionEvent.getSource() == sldA3000Lowmem)
		{
			changed_prefs.mbresmem_low.size = A3000LowMem_values[static_cast<int>(sldA3000Lowmem->getValue())];
			if (currprefs.mbresmem_low.size != changed_prefs.mbresmem_low.size)
				disable_resume();
		}

		if (actionEvent.getSource() == sldA3000Highmem)
		{
			changed_prefs.mbresmem_high.size = A3000HighMem_values[static_cast<int>(sldA3000Highmem->getValue())];
			if (currprefs.mbresmem_high.size != changed_prefs.mbresmem_high.size)
				disable_resume();
		}

		RefreshPanelRAM();
	}
};

static MemorySliderActionListener* memorySliderActionListener;


void InitPanelRAM(const struct _ConfigCategory& category)
{
	memorySliderActionListener = new MemorySliderActionListener();
	int sld_width;
	int marker_length;
#ifdef ANDROID
	sldWidth = 150;
	markerLength = 30;
#else
	sld_width = 110;
	marker_length = 20;
#endif

	lblChipmem = new gcn::Label("Chip:");
	sldChipmem = new gcn::Slider(0, 4);
	sldChipmem->setSize(sld_width, SLIDER_HEIGHT);
	sldChipmem->setBaseColor(gui_baseCol);
	sldChipmem->setMarkerLength(marker_length);
	sldChipmem->setStepLength(1);
	sldChipmem->setId("Chipmem");
	sldChipmem->addActionListener(memorySliderActionListener);
	lblChipsize = new gcn::Label("None   ");

	lblSlowmem = new gcn::Label("Slow:");
	sldSlowmem = new gcn::Slider(0, 4);
	sldSlowmem->setSize(sld_width, SLIDER_HEIGHT);
	sldSlowmem->setBaseColor(gui_baseCol);
	sldSlowmem->setMarkerLength(marker_length);
	sldSlowmem->setStepLength(1);
	sldSlowmem->setId("Slowmem");
	sldSlowmem->addActionListener(memorySliderActionListener);
	lblSlowsize = new gcn::Label("None   ");

	lblFastmem = new gcn::Label("Z2 Fast:");
	sldFastmem = new gcn::Slider(0, 4);
	sldFastmem->setSize(sld_width, SLIDER_HEIGHT);
	sldFastmem->setBaseColor(gui_baseCol);
	sldFastmem->setMarkerLength(marker_length);
	sldFastmem->setStepLength(1);
	sldFastmem->setId("Fastmem");
	sldFastmem->addActionListener(memorySliderActionListener);
	lblFastsize = new gcn::Label("None   ");

	lblZ3mem = new gcn::Label("Z3 fast:");
	if (can_have_1gb())
		sldZ3mem = new gcn::Slider(0, 11);
	else
		sldZ3mem = new gcn::Slider(0, 10);
	sldZ3mem->setSize(sld_width, SLIDER_HEIGHT);
	sldZ3mem->setBaseColor(gui_baseCol);
	sldZ3mem->setMarkerLength(marker_length);
	sldZ3mem->setStepLength(1);
	sldZ3mem->setId("Z3mem");
	sldZ3mem->addActionListener(memorySliderActionListener);
	lblZ3size = new gcn::Label("None    ");

	lblA3000Lowmem = new gcn::Label("A4000 Motherb. slot:");
	sldA3000Lowmem = new gcn::Slider(0, 2);
	sldA3000Lowmem->setSize(sld_width, SLIDER_HEIGHT);
	sldA3000Lowmem->setBaseColor(gui_baseCol);
	sldA3000Lowmem->setMarkerLength(marker_length);
	sldA3000Lowmem->setStepLength(1);
	sldA3000Lowmem->setId("A3000Low");
	sldA3000Lowmem->addActionListener(memorySliderActionListener);
	lblA3000Lowsize = new gcn::Label("None   ");

	lblA3000Highmem = new gcn::Label("A4000 Proc. board:");
	sldA3000Highmem = new gcn::Slider(0, 5);
	sldA3000Highmem->setSize(sld_width, SLIDER_HEIGHT);
	sldA3000Highmem->setBaseColor(gui_baseCol);
	sldA3000Highmem->setMarkerLength(marker_length);
	sldA3000Highmem->setStepLength(1);
	sldA3000Highmem->setId("A3000High");
	sldA3000Highmem->addActionListener(memorySliderActionListener);
	lblA3000Highsize = new gcn::Label("None   ");

	grpRAM = new gcn::Window("Memory Settings");
	grpRAM->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);

	int posY = 10;
	grpRAM->add(lblChipmem, 10, posY);
	grpRAM->add(sldChipmem, lblA3000Lowmem->getWidth() + DISTANCE_NEXT_Y, posY);
	grpRAM->add(lblChipsize, lblA3000Lowmem->getWidth() + DISTANCE_NEXT_Y + sldChipmem->getWidth() + 12, posY);
	posY += sldChipmem->getHeight() + DISTANCE_NEXT_Y;

	grpRAM->add(lblSlowmem, 10, posY);
	grpRAM->add(sldSlowmem, lblA3000Lowmem->getWidth() + DISTANCE_NEXT_Y, posY);
	grpRAM->add(lblSlowsize, lblA3000Lowmem->getWidth() + DISTANCE_NEXT_Y + sldSlowmem->getWidth() + 12, posY);
	posY += sldSlowmem->getHeight() + DISTANCE_NEXT_Y;

	grpRAM->add(lblFastmem, 10, posY);
	grpRAM->add(sldFastmem, lblA3000Lowmem->getWidth() + DISTANCE_NEXT_Y, posY);
	grpRAM->add(lblFastsize, lblA3000Lowmem->getWidth() + DISTANCE_NEXT_Y + sldFastmem->getWidth() + 12, posY);
	posY += sldFastmem->getHeight() + DISTANCE_NEXT_Y;

	grpRAM->add(lblZ3mem, 10, posY);
	grpRAM->add(sldZ3mem, lblA3000Lowmem->getWidth() + DISTANCE_NEXT_Y, posY);
	grpRAM->add(lblZ3size, lblA3000Lowmem->getWidth() + DISTANCE_NEXT_Y + sldZ3mem->getWidth() + 12, posY);
	posY += sldZ3mem->getHeight() + DISTANCE_NEXT_Y;

	grpRAM->add(lblA3000Lowmem, 10, posY);
	grpRAM->add(sldA3000Lowmem, lblA3000Lowmem->getWidth() + DISTANCE_NEXT_Y, posY);
	grpRAM->add(lblA3000Lowsize, lblA3000Lowmem->getWidth() + DISTANCE_NEXT_Y + sldA3000Lowmem->getWidth() + 12, posY);
	posY += sldA3000Lowmem->getHeight() + DISTANCE_NEXT_Y;

	grpRAM->add(lblA3000Highmem, 10, posY);
	grpRAM->add(sldA3000Highmem, lblA3000Lowmem->getWidth() + DISTANCE_NEXT_Y, posY);
	grpRAM->add(lblA3000Highsize, lblA3000Lowmem->getWidth() + DISTANCE_NEXT_Y + sldA3000Highmem->getWidth() + 12,
	            posY);
	posY += sldA3000Highmem->getHeight() + DISTANCE_NEXT_Y;

	grpRAM->setMovable(false);
	grpRAM->setSize(375, posY + DISTANCE_BORDER * 2);
	grpRAM->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpRAM->setBaseColor(gui_baseCol);

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
	delete lblA3000Lowmem;
	delete sldA3000Lowmem;
	delete lblA3000Lowsize;
	delete lblA3000Highmem;
	delete sldA3000Highmem;
	delete lblA3000Highsize;
	delete grpRAM;
	delete memorySliderActionListener;
}


void RefreshPanelRAM()
{
	int i;

	for (i = 0; i < 5; ++i)
	{
		if (changed_prefs.chipmem.size == ChipMem_values[i])
		{
			sldChipmem->setValue(i);
			lblChipsize->setCaption(ChipMem_list[i]);
			break;
		}
	}

	for (i = 0; i < 5; ++i)
	{
		if (changed_prefs.bogomem.size == SlowMem_values[i])
		{
			sldSlowmem->setValue(i);
			lblSlowsize->setCaption(SlowMem_list[i]);
			break;
		}
	}

	for (i = 0; i < 5; ++i)
	{
		if (changed_prefs.fastmem[0].size == FastMem_values[i])
		{
			sldFastmem->setValue(i);
			lblFastsize->setCaption(FastMem_list[i]);
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
	}
	else
	{
		sldZ3mem->setEnabled(true);
		lblZ3mem->setEnabled(true);
		lblZ3size->setEnabled(true);
		lblZ3size->setCaption("None");
		
		auto counter = 11;
		if (can_have_1gb())
			counter = 12;
		for (i = 0; i < counter; ++i)
		{
			if (changed_prefs.z3fastmem[0].size == FastMem_values[i])
			{
				sldZ3mem->setValue(i);
				lblZ3size->setCaption(FastMem_list[i]);
				break;
			}
		}
	}

	for (i = 0; i < 3; ++i)
	{
		if (changed_prefs.mbresmem_low.size == A3000LowMem_values[i])
		{
			sldA3000Lowmem->setValue(i);
			lblA3000Lowsize->setCaption(A3000LowMem_list[i]);
			break;
		}
	}

	for (i = 0; i < 6; ++i)
	{
		if (changed_prefs.mbresmem_high.size == A3000HighMem_values[i])
		{
			sldA3000Highmem->setValue(i);
			lblA3000Highsize->setCaption(A3000HighMem_list[i]);
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
	helptext.emplace_back("A4000 motherboard and processor board memory is only detected by the Amiga if");
	helptext.emplace_back("you choose the correct Kickstart ROM (A4000).");
	return true;
}
