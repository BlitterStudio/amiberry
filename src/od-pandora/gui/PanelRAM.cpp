#include <guichan.hpp>
#include <SDL/SDL_ttf.h>
#include <guichan/sdl.hpp>
#include "sdltruetypefont.hpp"
#include "SelectorEntry.hpp"
#include "UaeRadioButton.hpp"
#include "UaeDropDown.hpp"
#include "UaeCheckBox.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "memory.h"
#include "uae.h"
#include "gui.h"
#include "target.h"
#include "gui_handling.h"


static const char *ChipMem_list[] = { "512 K", "1 MB", "2 MB", "4 MB", "8 MB" };
static const int ChipMem_values[] = { 0x080000, 0x100000, 0x200000, 0x400000, 0x800000 };
static const char *SlowMem_list[] = { "None", "512 K", "1 MB", "1.5 MB", "1.8 MB" };
static const int SlowMem_values[] = { 0x000000, 0x080000, 0x100000, 0x180000, 0x1c0000 };
static const char *FastMem_list[] = { "None", "1 MB", "2 MB", "4 MB", "8 MB", "16 MB", "32 MB", "64 MB" };
static const int FastMem_values[] = { 0x000000, 0x100000, 0x200000, 0x400000, 0x800000, 0x1000000, 0x2000000, 0x4000000 };

static gcn::Window *grpRAM;
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
static gcn::Label* lblGfxmem;
static gcn::Label* lblGfxsize;
static gcn::Slider* sldGfxmem;  


class MemorySliderActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
 	    if (actionEvent.getSource() == sldChipmem) {
    		changed_prefs.chipmem_size = ChipMem_values[(int)(sldChipmem->getValue())];
      	if ((changed_prefs.chipmem_size > 0x200000) && (changed_prefs.fastmem_size > 0))
      		changed_prefs.fastmem_size = 0;
			}
			
 	    if (actionEvent.getSource() == sldSlowmem) {
      	changed_prefs.bogomem_size = SlowMem_values[(int)(sldSlowmem->getValue())];
      }
      
	    if (actionEvent.getSource() == sldFastmem) {
     		changed_prefs.fastmem_size = FastMem_values[(int)(sldFastmem->getValue())];
	      if (changed_prefs.fastmem_size > 0 && changed_prefs.chipmem_size > 0x200000)
	        changed_prefs.chipmem_size = 0x200000;
  		}	

	    if (actionEvent.getSource() == sldZ3mem) {
     		changed_prefs.z3fastmem_size = FastMem_values[(int)(sldZ3mem->getValue())];
	      if (changed_prefs.z3fastmem_size > max_z3fastmem)
	        changed_prefs.z3fastmem_size = max_z3fastmem;
  		}	

	    if (actionEvent.getSource() == sldGfxmem) {
     		changed_prefs.gfxmem_size = FastMem_values[(int)(sldGfxmem->getValue())];
  		}	

  		RefreshPanelRAM();
    }
};
static MemorySliderActionListener* memorySliderActionListener;


void InitPanelRAM(const struct _ConfigCategory& category)
{
	memorySliderActionListener = new MemorySliderActionListener();
  
	lblChipmem = new gcn::Label("Chip:");
	sldChipmem = new gcn::Slider(0, 4);
	sldChipmem->setSize(110, SLIDER_HEIGHT);
	sldChipmem->setBaseColor(gui_baseCol);
	sldChipmem->setMarkerLength(20);
	sldChipmem->setStepLength(1);
	sldChipmem->setId("Chipmem");
	sldChipmem->addActionListener(memorySliderActionListener);
	lblChipsize = new gcn::Label("None  ");

	lblSlowmem = new gcn::Label("Slow:");
	sldSlowmem = new gcn::Slider(0, 4);
	sldSlowmem->setSize(110, SLIDER_HEIGHT);
	sldSlowmem->setBaseColor(gui_baseCol);
	sldSlowmem->setMarkerLength(20);
	sldSlowmem->setStepLength(1);
	sldSlowmem->setId("Slowmem");
	sldSlowmem->addActionListener(memorySliderActionListener);
	lblSlowsize = new gcn::Label("None  ");

	lblFastmem = new gcn::Label("Fast:");
	sldFastmem = new gcn::Slider(0, 4);
	sldFastmem->setSize(110, SLIDER_HEIGHT);
	sldFastmem->setBaseColor(gui_baseCol);
	sldFastmem->setMarkerLength(20);
	sldFastmem->setStepLength(1);
	sldFastmem->setId("Fastmem");
	sldFastmem->addActionListener(memorySliderActionListener);
	lblFastsize = new gcn::Label("None  ");

	lblZ3mem = new gcn::Label("Z3 fast:");
	sldZ3mem = new gcn::Slider(0, 7);
	sldZ3mem->setSize(110, SLIDER_HEIGHT);
	sldZ3mem->setBaseColor(gui_baseCol);
	sldZ3mem->setMarkerLength(20);
	sldZ3mem->setStepLength(1);
	sldZ3mem->setId("Z3mem");
	sldZ3mem->addActionListener(memorySliderActionListener);
	lblZ3size = new gcn::Label("None  ");

	lblGfxmem = new gcn::Label("RTG:");
	sldGfxmem = new gcn::Slider(0, 5);
	sldGfxmem->setSize(110, SLIDER_HEIGHT);
	sldGfxmem->setBaseColor(gui_baseCol);
	sldGfxmem->setMarkerLength(20);
	sldGfxmem->setStepLength(1);
	sldGfxmem->setId("Gfxmem");
	sldGfxmem->addActionListener(memorySliderActionListener);
	lblGfxsize = new gcn::Label("None  ");

	grpRAM = new gcn::Window("Memory Settings");
	grpRAM->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);

	int posY = 10;
	grpRAM->add(lblChipmem, 8, posY);
	grpRAM->add(sldChipmem, 70, posY);
	grpRAM->add(lblChipsize, 70 + sldChipmem->getWidth() + 12, posY);
	posY += sldChipmem->getHeight() + DISTANCE_NEXT_Y;

	grpRAM->add(lblSlowmem, 8, posY);
	grpRAM->add(sldSlowmem, 70, posY);
	grpRAM->add(lblSlowsize, 70 + sldSlowmem->getWidth() + 12, posY);
	posY += sldSlowmem->getHeight() + DISTANCE_NEXT_Y;
	 
	grpRAM->add(lblFastmem, 8, posY);
	grpRAM->add(sldFastmem, 70, posY);
	grpRAM->add(lblFastsize, 70 + sldFastmem->getWidth() + 12, posY);
	posY += sldFastmem->getHeight() + DISTANCE_NEXT_Y;

	grpRAM->add(lblZ3mem, 8, posY);
	grpRAM->add(sldZ3mem, 70, posY);
	grpRAM->add(lblZ3size, 70 + sldZ3mem->getWidth() + 12, posY);
	posY += sldZ3mem->getHeight() + DISTANCE_NEXT_Y;
#ifdef PICASSO96
	grpRAM->add(lblGfxmem, 8, posY);
	grpRAM->add(sldGfxmem, 70, posY);
	grpRAM->add(lblGfxsize, 70 + sldGfxmem->getWidth() + 12, posY);
	posY += sldGfxmem->getHeight() + DISTANCE_NEXT_Y;
#endif
	grpRAM->setMovable(false);
	grpRAM->setSize(250, posY + DISTANCE_BORDER);
	grpRAM->setBaseColor(gui_baseCol);
  
	category.panel->add(grpRAM);
  
	RefreshPanelRAM();
}


void ExitPanelRAM(void)
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
  delete lblGfxmem;
  delete sldGfxmem;
  delete lblGfxsize;
  delete grpRAM;
  delete memorySliderActionListener;
}


void RefreshPanelRAM(void)
{
  int i;
  
  for(i=0; i<5; ++i)
  {
    if(changed_prefs.chipmem_size == ChipMem_values[i])
    {
      sldChipmem->setValue(i);
      lblChipsize->setCaption(ChipMem_list[i]);
      break;
    }
  }

  for(i=0; i<5; ++i)
  {
    if(changed_prefs.bogomem_size == SlowMem_values[i])
    {
      sldSlowmem->setValue(i);
      lblSlowsize->setCaption(SlowMem_list[i]);
      break;
    }
  }

  for(i=0; i<5; ++i)
  {
    if(changed_prefs.fastmem_size == FastMem_values[i])
    {
      sldFastmem->setValue(i);
      lblFastsize->setCaption(FastMem_list[i]);
      break;
    }
  }

  for(i=0; i<8; ++i)
  {
    if(changed_prefs.z3fastmem_size == FastMem_values[i])
    {
      sldZ3mem->setValue(i);
      lblZ3size->setCaption(FastMem_list[i]);
      break;
    }
  }
  sldZ3mem->setEnabled(!changed_prefs.address_space_24);

  for(i=0; i<6; ++i)
  {
    if(changed_prefs.gfxmem_size == FastMem_values[i])
    {
      sldGfxmem->setValue(i);
      lblGfxsize->setCaption(FastMem_list[i]);
      break;
    }
  }
  sldGfxmem->setEnabled(!changed_prefs.address_space_24);
}
