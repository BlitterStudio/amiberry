#include <stdbool.h>

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

#include "sysdeps.h"
#include "options.h"
#include "include/memory.h"
#include "newcpu.h"
#include "gui_handling.h"

static gcn::Window* grpCPU;
static gcn::UaeRadioButton* optCPU68000;
static gcn::UaeRadioButton* optCPU68010;
static gcn::UaeRadioButton* optCPU68020;
static gcn::UaeRadioButton* optCPU68030;
static gcn::UaeRadioButton* optCPU68040;
static gcn::UaeCheckBox* chk24Bit;
static gcn::UaeCheckBox* chkCPUCompatible;
static gcn::UaeCheckBox* chkJIT;
static gcn::Window* grpFPU;
static gcn::UaeRadioButton* optFPUnone;
static gcn::UaeRadioButton* optFPU68881;
static gcn::UaeRadioButton* optFPU68882;
static gcn::UaeRadioButton* optFPUinternal;
static gcn::UaeCheckBox* chkFPUstrict;
static gcn::UaeCheckBox* chkFPUJIT;
static gcn::Window* grpCPUSpeed;
static gcn::UaeRadioButton* opt7Mhz;
static gcn::UaeRadioButton* opt14Mhz;
static gcn::UaeRadioButton* opt28Mhz;
static gcn::UaeRadioButton* optFastest;
static gcn::UaeRadioButton* optTurbo;

class CPUButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == optCPU68000)
		{
			workprefs.cpu_model = 68000;
			workprefs.fpu_model = 0;
			workprefs.address_space_24 = true;
			workprefs.z3fastmem[0].size = 0;
			workprefs.rtgboards[0].rtgmem_size = 0;
			workprefs.cachesize = 0;
			workprefs.compfpu = false;
		}
		else if (actionEvent.getSource() == optCPU68010)
		{
			workprefs.cpu_model = 68010;
			workprefs.fpu_model = 0;
			workprefs.address_space_24 = true;
			workprefs.z3fastmem[0].size = 0;
			workprefs.rtgboards[0].rtgmem_size = 0;
			workprefs.cachesize = 0;
			workprefs.compfpu = false;
		}
		else if (actionEvent.getSource() == optCPU68020)
		{
			workprefs.cpu_model = 68020;
			if (workprefs.fpu_model == 68040)
				workprefs.fpu_model = 68881;
			workprefs.cpu_compatible = false;
		}
		else if (actionEvent.getSource() == optCPU68030)
		{
			workprefs.cpu_model = 68030;
			if (workprefs.fpu_model == 68040)
				workprefs.fpu_model = 68881;
			workprefs.address_space_24 = false;
			workprefs.cpu_compatible = false;
		}
		else if (actionEvent.getSource() == optCPU68040)
		{
			workprefs.cpu_model = 68040;
			workprefs.fpu_model = 68040;
			workprefs.address_space_24 = false;
			workprefs.cpu_compatible = false;
		}
		RefreshPanelCPU();
		RefreshPanelRAM();
	}
};

static CPUButtonActionListener* cpuButtonActionListener;

class FPUButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == optFPUnone)
		{
			workprefs.fpu_model = 0;
		}
		else if (actionEvent.getSource() == optFPU68881)
		{
			workprefs.fpu_model = 68881;
		}
		else if (actionEvent.getSource() == optFPU68882)
		{
			workprefs.fpu_model = 68882;
		}
		else if (actionEvent.getSource() == optFPUinternal)
		{
			workprefs.fpu_model = 68040;
		}
		RefreshPanelCPU();
		RefreshPanelRAM();
	}
};

static FPUButtonActionListener* fpuButtonActionListener;


class CPUSpeedButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == opt7Mhz)
			workprefs.m68k_speed = M68K_SPEED_7MHZ_CYCLES;
		else if (actionEvent.getSource() == opt14Mhz)
			workprefs.m68k_speed = M68K_SPEED_14MHZ_CYCLES;
		else if (actionEvent.getSource() == opt28Mhz)
			workprefs.m68k_speed = M68K_SPEED_25MHZ_CYCLES;
		else if (actionEvent.getSource() == optFastest)
			workprefs.m68k_speed = -1;
		else if (actionEvent.getSource() == optTurbo)
			workprefs.m68k_speed = -30;
	}
};

static CPUSpeedButtonActionListener* cpuSpeedButtonActionListener;


class CPU24BitActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		workprefs.address_space_24 = chk24Bit->isSelected();
		RefreshPanelCPU();
	}
};

static CPU24BitActionListener* cpu24BitActionListener;

class CPUCompActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (chkCPUCompatible->isSelected())
		{
			workprefs.cpu_compatible = true;
			workprefs.cachesize = 0;
		}
		else
		{
			workprefs.cpu_compatible = false;
		}
		RefreshPanelCPU();
	}
};

static CPUCompActionListener* cpuCompActionListener;


class JITActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == chkJIT)
		{
			if (chkJIT->isSelected())
			{
				workprefs.cpu_compatible = false;
				workprefs.cachesize = MAX_JIT_CACHE;
				workprefs.compfpu = true;
			}
			else
			{
				workprefs.cachesize = 0;
				workprefs.compfpu = false;
			}
		}
		else if (actionEvent.getSource() == chkFPUJIT)
		{
			workprefs.compfpu = chkFPUJIT->isSelected();
		}
		RefreshPanelCPU();
	}
};

static JITActionListener* jitActionListener;

class FPUActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == chkFPUstrict) {
			workprefs.fpu_strict = chkFPUstrict->isSelected();

		}
		RefreshPanelCPU();
	}
};
static FPUActionListener* fpuActionListener;

void InitPanelCPU(const struct _ConfigCategory& category)
{
	cpuButtonActionListener = new CPUButtonActionListener();
	cpu24BitActionListener = new CPU24BitActionListener();
	cpuCompActionListener = new CPUCompActionListener();
	jitActionListener = new JITActionListener();
	fpuActionListener = new FPUActionListener();

	optCPU68000 = new gcn::UaeRadioButton("68000", "radiocpugroup");
	optCPU68000->addActionListener(cpuButtonActionListener);
	optCPU68010 = new gcn::UaeRadioButton("68010", "radiocpugroup");
	optCPU68010->addActionListener(cpuButtonActionListener);
	optCPU68020 = new gcn::UaeRadioButton("68020", "radiocpugroup");
	optCPU68020->addActionListener(cpuButtonActionListener);
	optCPU68030 = new gcn::UaeRadioButton("68030", "radiocpugroup");
	optCPU68030->addActionListener(cpuButtonActionListener);
	optCPU68040 = new gcn::UaeRadioButton("68040", "radiocpugroup");
	optCPU68040->addActionListener(cpuButtonActionListener);

	chk24Bit = new gcn::UaeCheckBox("24-bit addressing", true);
	chk24Bit->setId("CPU24Bit");
	chk24Bit->addActionListener(cpu24BitActionListener);

	chkCPUCompatible = new gcn::UaeCheckBox("More compatible", true);
	chkCPUCompatible->setId("CPUComp");
	chkCPUCompatible->addActionListener(cpuCompActionListener);

	chkJIT = new gcn::UaeCheckBox("JIT", true);
	chkJIT->setId("JIT");
	chkJIT->addActionListener(jitActionListener);

	grpCPU = new gcn::Window("CPU");
	grpCPU->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	grpCPU->add(optCPU68000, 5, 10);
	grpCPU->add(optCPU68010, 5, 40);
	grpCPU->add(optCPU68020, 5, 70);
	grpCPU->add(optCPU68030, 5, 100);
	grpCPU->add(optCPU68040, 5, 130);
	grpCPU->add(chk24Bit, 5, 170);
	grpCPU->add(chkCPUCompatible, 5, 200);
	grpCPU->add(chkJIT, 5, 230);
	grpCPU->setMovable(false);
	grpCPU->setSize(175, 275);
	grpCPU->setBaseColor(gui_baseCol);

	category.panel->add(grpCPU);

	fpuButtonActionListener = new FPUButtonActionListener();

	optFPUnone = new gcn::UaeRadioButton("None", "radiofpugroup");
	optFPUnone->setId("FPUnone");
	optFPUnone->addActionListener(fpuButtonActionListener);

	optFPU68881 = new gcn::UaeRadioButton("68881", "radiofpugroup");
	optFPU68881->addActionListener(fpuButtonActionListener);

	optFPU68882 = new gcn::UaeRadioButton("68882", "radiofpugroup");
	optFPU68882->addActionListener(fpuButtonActionListener);

	optFPUinternal = new gcn::UaeRadioButton("CPU internal", "radiofpugroup");
	optFPUinternal->addActionListener(fpuButtonActionListener);

	chkFPUstrict = new gcn::UaeCheckBox("More compatible", true);
	chkFPUstrict->setId("FPUstrict");
	chkFPUstrict->addActionListener(fpuActionListener);

	chkFPUJIT = new gcn::UaeCheckBox("FPU JIT", true);
	chkFPUJIT->setId("FPUJIT");
	chkFPUJIT->addActionListener(jitActionListener);

	grpFPU = new gcn::Window("FPU");
	grpFPU->setPosition(DISTANCE_BORDER + grpCPU->getWidth() + DISTANCE_NEXT_X, DISTANCE_BORDER);
	grpFPU->add(optFPUnone, 5, 10);
	grpFPU->add(optFPU68881, 5, 40);
	grpFPU->add(optFPU68882, 5, 70);
	grpFPU->add(optFPUinternal, 5, 100);
	grpFPU->add(chkFPUstrict, 5, 140);
	grpFPU->add(chkFPUJIT, 5, 170);
	grpFPU->setMovable(false);
	grpFPU->setSize(185, 215);
	grpFPU->setBaseColor(gui_baseCol);

	category.panel->add(grpFPU);

	cpuSpeedButtonActionListener = new CPUSpeedButtonActionListener();

	opt7Mhz = new gcn::UaeRadioButton("7 Mhz", "radiocpuspeedgroup");
	opt7Mhz->addActionListener(cpuSpeedButtonActionListener);

	opt14Mhz = new gcn::UaeRadioButton("14 Mhz", "radiocpuspeedgroup");
	opt14Mhz->addActionListener(cpuSpeedButtonActionListener);

	opt28Mhz = new gcn::UaeRadioButton("25 Mhz", "radiocpuspeedgroup");
	opt28Mhz->addActionListener(cpuSpeedButtonActionListener);

	optFastest = new gcn::UaeRadioButton("Fastest", "radiocpuspeedgroup");
	optFastest->addActionListener(cpuSpeedButtonActionListener);

	optTurbo = new gcn::UaeRadioButton("Turbo", "radiocpuspeedgroup");
	optTurbo->addActionListener(cpuSpeedButtonActionListener);

	grpCPUSpeed = new gcn::Window("CPU Speed");
	grpCPUSpeed->setPosition(grpFPU->getX() + grpFPU->getWidth() + DISTANCE_NEXT_X, DISTANCE_BORDER);
	grpCPUSpeed->add(opt7Mhz, 5, 10);
	grpCPUSpeed->add(opt14Mhz, 5, 40);
	grpCPUSpeed->add(opt28Mhz, 5, 70);
	grpCPUSpeed->add(optFastest, 5, 100);
	grpCPUSpeed->add(optTurbo, 5, 130);
	grpCPUSpeed->setMovable(false);
	grpCPUSpeed->setSize(95, 200);
	grpCPUSpeed->setBaseColor(gui_baseCol);

	category.panel->add(grpCPUSpeed);

	RefreshPanelCPU();
}


void ExitPanelCPU()
{
	delete optCPU68000;
	delete optCPU68010;
	delete optCPU68020;
	delete optCPU68030;
	delete optCPU68040;
	delete chk24Bit;
	delete chkCPUCompatible;
	delete chkJIT;
	delete grpCPU;
	delete cpuButtonActionListener;
	delete cpu24BitActionListener;
	delete cpuCompActionListener;
	delete jitActionListener;

	delete optFPUnone;
	delete optFPU68881;
	delete optFPU68882;
	delete optFPUinternal;
	delete chkFPUstrict;
	delete chkFPUJIT;
	delete grpFPU;
	delete fpuButtonActionListener;
	delete fpuActionListener;

	delete opt7Mhz;
	delete opt14Mhz;
	delete opt28Mhz;
	delete optFastest;
	delete optTurbo;
	delete grpCPUSpeed;
	delete cpuSpeedButtonActionListener;
}


void RefreshPanelCPU()
{
	if (workprefs.cpu_model == 68000)
		optCPU68000->setSelected(true);
	else if (workprefs.cpu_model == 68010)
		optCPU68010->setSelected(true);
	else if (workprefs.cpu_model == 68020)
		optCPU68020->setSelected(true);
	else if (workprefs.cpu_model == 68030)
		optCPU68030->setSelected(true);
	else if (workprefs.cpu_model == 68040)
		optCPU68040->setSelected(true);

	chk24Bit->setSelected(workprefs.address_space_24);
	chk24Bit->setEnabled(workprefs.cpu_model == 68020);
	chkCPUCompatible->setSelected(workprefs.cpu_compatible > 0);
	chkCPUCompatible->setEnabled(workprefs.cpu_model <= 68010);
	chkJIT->setEnabled(workprefs.cpu_model > 68010);
	chkJIT->setSelected(workprefs.cachesize > 0);

	switch (workprefs.fpu_model)
	{
	case 68881:
		optFPU68881->setSelected(true);
		break;
	case 68882:
		optFPU68882->setSelected(true);
		break;
	case 68040:
		optFPUinternal->setSelected(true);
		break;
	default:
		optFPUnone->setSelected(true);
		break;
	}
	optFPU68881->setEnabled(workprefs.cpu_model >= 68020 && workprefs.cpu_model < 68040);
	optFPU68882->setEnabled(workprefs.cpu_model >= 68020 && workprefs.cpu_model < 68040);
	optFPUinternal->setEnabled(workprefs.cpu_model == 68040);

	chkFPUstrict->setSelected(workprefs.fpu_strict);

#ifdef USE_JIT_FPU
	chkFPUJIT->setEnabled(workprefs.cachesize > 0);
	chkFPUJIT->setSelected(workprefs.compfpu);
#else
  chkFPUJIT->setSelected(false);
  chkFPUJIT->setEnabled(false);
#endif

	if (workprefs.m68k_speed == M68K_SPEED_7MHZ_CYCLES)
		opt7Mhz->setSelected(true);
	else if (workprefs.m68k_speed == M68K_SPEED_14MHZ_CYCLES)
		opt14Mhz->setSelected(true);
	else if (workprefs.m68k_speed == M68K_SPEED_25MHZ_CYCLES)
		opt28Mhz->setSelected(true);
	else if (workprefs.m68k_speed == -1)
		optFastest->setSelected(true);
	else if (workprefs.m68k_speed == -30)
		optTurbo->setSelected(true);
}

bool HelpPanelCPU(std::vector<std::string> &helptext)
{
	helptext.clear();
	helptext.emplace_back("Select the required Amiga CPU (68000 - 68040).");
	helptext.emplace_back("If you select 68020, you can choose between 24-bit addressing (68EC020) or 32-bit");
	helptext.emplace_back("addressing (68020). The option \"More compatible\" is only available if 68000 or 68010");
	helptext.emplace_back("is selected and emulates simple prefetch of the 68000. This may improve compatibility");
	helptext.emplace_back("in few situations but is not required for most games and demos.");
	helptext.emplace_back("");
	helptext.emplace_back("JIT enables the Just-in-time compiler. This may break compatibility in some games.");
	helptext.emplace_back("");
	helptext.emplace_back("The available FPU models depending on the selected CPU.");
	helptext.emplace_back("The option \"More compatible\" activates more accurate rounding and compare of two floats.");
	helptext.emplace_back("");
	helptext.emplace_back("With \"CPU Speed\" you can choose the clock rate of the Amiga.");
	helptext.emplace_back("Use 7MHz for A500 games or 14MHz for A1200 ones. Fastest uses more emulation time");
	helptext.emplace_back("for the CPU, and Turbo will give only the minimum time to the chipset, using as");
	helptext.emplace_back("much as possible for the CPU, usually resulting in dropping frames also.");
	helptext.emplace_back("");
	helptext.emplace_back("In current version, you will not see a difference in the performance for 68020,");
	helptext.emplace_back("68030 and 68040 CPUs. The CPU cycles for the opcodes are based on 68020. The different");
	helptext.emplace_back("cycles for 68030 and 68040 may come in a later version.");
	return true;
}
