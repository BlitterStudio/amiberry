#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "gui_handling.h"

static gcn::Window* grpCPU;
static gcn::RadioButton* optCPU68000;
static gcn::RadioButton* optCPU68010;
static gcn::RadioButton* optCPU68020;
static gcn::RadioButton* optCPU68030;
static gcn::RadioButton* optCPU68040;
static gcn::CheckBox* chk24Bit;
static gcn::CheckBox* chkCPUCompatible;
static gcn::CheckBox* chkJIT;
static gcn::Window* grpFPU;
static gcn::RadioButton* optFPUnone;
static gcn::RadioButton* optFPU68881;
static gcn::RadioButton* optFPU68882;
static gcn::RadioButton* optFPUinternal;
static gcn::CheckBox* chkFPUstrict;
static gcn::CheckBox* chkFPUJIT;
static gcn::Window* grpCPUSpeed;
static gcn::RadioButton* opt7Mhz;
static gcn::RadioButton* opt14Mhz;
static gcn::RadioButton* opt25Mhz;
static gcn::RadioButton* optFastest;
static gcn::Label* lblCpuIdle;
static gcn::Slider* sldCpuIdle;
static gcn::Label* lblCpuIdleInfo;
static gcn::CheckBox* chkCPUCycleExact;

class CPUButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == optCPU68000)
		{
			changed_prefs.cpu_model = 68000;
			changed_prefs.fpu_model = 0;
			changed_prefs.address_space_24 = true;
			changed_prefs.z3fastmem[0].size = 0;
			changed_prefs.rtgboards[0].rtgmem_size = 0;
			changed_prefs.cachesize = 0;
			changed_prefs.compfpu = false;
		}
		else if (actionEvent.getSource() == optCPU68010)
		{
			changed_prefs.cpu_model = 68010;
			changed_prefs.fpu_model = 0;
			changed_prefs.address_space_24 = true;
			changed_prefs.z3fastmem[0].size = 0;
			changed_prefs.rtgboards[0].rtgmem_size = 0;
			changed_prefs.cachesize = 0;
			changed_prefs.compfpu = false;
		}
		else if (actionEvent.getSource() == optCPU68020)
		{
			changed_prefs.cpu_model = 68020;
			if (changed_prefs.fpu_model == 68040)
				changed_prefs.fpu_model = 68881;

			changed_prefs.cpu_cycle_exact = false;
			changed_prefs.blitter_cycle_exact = false;
			changed_prefs.cpu_memory_cycle_exact = false;
		}
		else if (actionEvent.getSource() == optCPU68030)
		{
			changed_prefs.cpu_model = 68030;
			if (changed_prefs.fpu_model == 68040)
				changed_prefs.fpu_model = 68881;
			changed_prefs.address_space_24 = false;
			changed_prefs.cpu_cycle_exact = false;
			changed_prefs.blitter_cycle_exact = false;
			changed_prefs.cpu_memory_cycle_exact = false;
		}
		else if (actionEvent.getSource() == optCPU68040)
		{
			changed_prefs.cpu_model = 68040;
			changed_prefs.fpu_model = 68040;
			changed_prefs.address_space_24 = false;
			changed_prefs.cpu_cycle_exact = false;
			changed_prefs.blitter_cycle_exact = false;
			changed_prefs.cpu_memory_cycle_exact = false;
		}
		RefreshPanelCPU();
		RefreshPanelRAM();
		RefreshPanelChipset();
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
			changed_prefs.fpu_model = 0;
		}
		else if (actionEvent.getSource() == optFPU68881)
		{
			changed_prefs.fpu_model = 68881;
		}
		else if (actionEvent.getSource() == optFPU68882)
		{
			changed_prefs.fpu_model = 68882;
		}
		else if (actionEvent.getSource() == optFPUinternal)
		{
			changed_prefs.fpu_model = 68040;
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
			changed_prefs.m68k_speed = M68K_SPEED_7MHZ_CYCLES;
		else if (actionEvent.getSource() == opt14Mhz)
			changed_prefs.m68k_speed = M68K_SPEED_14MHZ_CYCLES;
		else if (actionEvent.getSource() == opt25Mhz)
			changed_prefs.m68k_speed = M68K_SPEED_25MHZ_CYCLES;
		else if (actionEvent.getSource() == optFastest)
			changed_prefs.m68k_speed = -1;
		else if (actionEvent.getSource() == chkCPUCycleExact) {
			if (chkCPUCycleExact->isSelected()) {
				changed_prefs.cpu_cycle_exact = true;
				changed_prefs.blitter_cycle_exact = true;
				changed_prefs.cpu_memory_cycle_exact = true;
				changed_prefs.cachesize = 0;
			}
			else {
				changed_prefs.cpu_cycle_exact = false;
				changed_prefs.blitter_cycle_exact = false;
				changed_prefs.cpu_memory_cycle_exact = false;
			}
			RefreshPanelChipset();
		}
	}
};

static CPUSpeedButtonActionListener* cpuSpeedButtonActionListener;


class CPU24BitActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		changed_prefs.address_space_24 = chk24Bit->isSelected();
		RefreshPanelCPU();
		RefreshPanelRAM();
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
			changed_prefs.cpu_compatible = true;
			changed_prefs.cachesize = 0;
		}
		else
		{
			changed_prefs.cpu_compatible = false;
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
				changed_prefs.cpu_compatible = false;
				changed_prefs.cachesize = MAX_JIT_CACHE;
				changed_prefs.compfpu = true;
				changed_prefs.cpu_cycle_exact = false;
				changed_prefs.cpu_memory_cycle_exact = false;
				changed_prefs.address_space_24 = false;
			}
			else
			{
				changed_prefs.cachesize = 0;
				changed_prefs.compfpu = false;
			}
		}
		else if (actionEvent.getSource() == chkFPUJIT)
		{
			changed_prefs.compfpu = chkFPUJIT->isSelected();
		}
		RefreshPanelCPU();
		RefreshPanelChipset();
		RefreshPanelRAM();
	}
};

static JITActionListener* jitActionListener;

class FPUActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == chkFPUstrict)
		{
			changed_prefs.fpu_strict = chkFPUstrict->isSelected();
		}
		RefreshPanelCPU();
	}
};

static FPUActionListener* fpuActionListener;

class CPUIdleSliderActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == sldCpuIdle)
		{
			changed_prefs.cpu_idle = static_cast<int>(sldCpuIdle->getValue());
			if (changed_prefs.cpu_idle > 0)
				changed_prefs.cpu_idle = (12 - changed_prefs.cpu_idle) * 15;
		}
		RefreshPanelCPU();
	}
};

static CPUIdleSliderActionListener* cpuIdleActionListener;

void InitPanelCPU(const struct _ConfigCategory& category)
{
	cpuButtonActionListener = new CPUButtonActionListener();
	cpu24BitActionListener = new CPU24BitActionListener();
	cpuCompActionListener = new CPUCompActionListener();
	jitActionListener = new JITActionListener();
	fpuActionListener = new FPUActionListener();
	cpuIdleActionListener = new CPUIdleSliderActionListener();

	optCPU68000 = new gcn::RadioButton("68000", "radiocpugroup");
	optCPU68000->setId("68000");
	optCPU68000->addActionListener(cpuButtonActionListener);
	optCPU68010 = new gcn::RadioButton("68010", "radiocpugroup");
	optCPU68010->setId("68010");
	optCPU68010->addActionListener(cpuButtonActionListener);
	optCPU68020 = new gcn::RadioButton("68020", "radiocpugroup");
	optCPU68020->setId("68020");
	optCPU68020->addActionListener(cpuButtonActionListener);
	optCPU68030 = new gcn::RadioButton("68030", "radiocpugroup");
	optCPU68030->setId("68030");
	optCPU68030->addActionListener(cpuButtonActionListener);
	optCPU68040 = new gcn::RadioButton("68040", "radiocpugroup");
	optCPU68040->setId("68040");
	optCPU68040->addActionListener(cpuButtonActionListener);

	chk24Bit = new gcn::CheckBox("24-bit addressing", true);
	chk24Bit->setId("CPU24Bit");
	chk24Bit->addActionListener(cpu24BitActionListener);

	chkCPUCompatible = new gcn::CheckBox("More compatible", true);
	chkCPUCompatible->setId("CPUComp");
	chkCPUCompatible->addActionListener(cpuCompActionListener);

	chkJIT = new gcn::CheckBox("JIT", true);
	chkJIT->setId("JIT");
	chkJIT->addActionListener(jitActionListener);

	grpCPU = new gcn::Window("CPU");
	grpCPU->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	grpCPU->add(optCPU68000, 10, 10);
	grpCPU->add(optCPU68010, 10, 40);
	grpCPU->add(optCPU68020, 10, 70);
	grpCPU->add(optCPU68030, 10, 100);
	grpCPU->add(optCPU68040, 10, 130);
	grpCPU->add(chk24Bit, 10, 170);
	grpCPU->add(chkCPUCompatible, 10, 200);
	grpCPU->add(chkJIT, 10, 230);
	grpCPU->setMovable(false);
	grpCPU->setSize(chk24Bit->getWidth() + 20, 285);
	grpCPU->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpCPU->setBaseColor(gui_baseCol);

	category.panel->add(grpCPU);

	fpuButtonActionListener = new FPUButtonActionListener();

	optFPUnone = new gcn::RadioButton("None", "radiofpugroup");
	optFPUnone->setId("FPUnone");
	optFPUnone->addActionListener(fpuButtonActionListener);

	optFPU68881 = new gcn::RadioButton("68881", "radiofpugroup");
	optFPU68881->setId("68881");
	optFPU68881->addActionListener(fpuButtonActionListener);

	optFPU68882 = new gcn::RadioButton("68882", "radiofpugroup");
	optFPU68882->setId("68882");
	optFPU68882->addActionListener(fpuButtonActionListener);

	optFPUinternal = new gcn::RadioButton("CPU internal", "radiofpugroup");
	optFPUinternal->setId("CPU internal");
	optFPUinternal->addActionListener(fpuButtonActionListener);

	chkFPUstrict = new gcn::CheckBox("More compatible", true);
	chkFPUstrict->setId("FPUstrict");
	chkFPUstrict->addActionListener(fpuActionListener);

	chkFPUJIT = new gcn::CheckBox("FPU JIT", true);
	chkFPUJIT->setId("FPUJIT");
	chkFPUJIT->addActionListener(jitActionListener);

	grpFPU = new gcn::Window("FPU");
	grpFPU->setPosition(DISTANCE_BORDER + grpCPU->getWidth() + DISTANCE_NEXT_X, DISTANCE_BORDER);
	grpFPU->add(optFPUnone, 10, 10);
	grpFPU->add(optFPU68881, 10, 40);
	grpFPU->add(optFPU68882, 10, 70);
	grpFPU->add(optFPUinternal, 10, 100);
	grpFPU->add(chkFPUstrict, 10, 140);
	grpFPU->add(chkFPUJIT, 10, 170);
	grpFPU->setMovable(false);
	grpFPU->setSize(175, 285);
	grpFPU->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpFPU->setBaseColor(gui_baseCol);

	category.panel->add(grpFPU);

	cpuSpeedButtonActionListener = new CPUSpeedButtonActionListener();

	opt7Mhz = new gcn::RadioButton("7 Mhz", "radiocpuspeedgroup");
	opt7Mhz->setId("7 Mhz");
	opt7Mhz->addActionListener(cpuSpeedButtonActionListener);

	opt14Mhz = new gcn::RadioButton("14 Mhz", "radiocpuspeedgroup");
	opt14Mhz->setId("14 Mhz");
	opt14Mhz->addActionListener(cpuSpeedButtonActionListener);

	opt25Mhz = new gcn::RadioButton("25 Mhz", "radiocpuspeedgroup");
	opt25Mhz->setId("25 Mhz");
	opt25Mhz->addActionListener(cpuSpeedButtonActionListener);

	optFastest = new gcn::RadioButton("Fastest", "radiocpuspeedgroup");
	optFastest->setId("Fastest");
	optFastest->addActionListener(cpuSpeedButtonActionListener);

	chkCPUCycleExact = new gcn::CheckBox("Cycle exact", true);
	chkCPUCycleExact->setId("chkCPUCycleExact");
	chkCPUCycleExact->addActionListener(cpuSpeedButtonActionListener);
	
	lblCpuIdle = new gcn::Label("CPU Idle");
	sldCpuIdle = new gcn::Slider(0, 10);
	sldCpuIdle->setSize(70, SLIDER_HEIGHT);
	sldCpuIdle->setBaseColor(gui_baseCol);
	sldCpuIdle->setMarkerLength(20);
	sldCpuIdle->setStepLength(1);
	sldCpuIdle->setId("sldCpuIdle");
	sldCpuIdle->addActionListener(cpuIdleActionListener);
	lblCpuIdleInfo = new gcn::Label("000");

	grpCPUSpeed = new gcn::Window("CPU Speed");
	grpCPUSpeed->setPosition(grpFPU->getX() + grpFPU->getWidth() + DISTANCE_NEXT_X, DISTANCE_BORDER);
	grpCPUSpeed->add(opt7Mhz, 10, 10);
	grpCPUSpeed->add(opt14Mhz, 10, 40);
	grpCPUSpeed->add(opt25Mhz, 10, 70);
	grpCPUSpeed->add(optFastest, 10, 100);
	grpCPUSpeed->add(chkCPUCycleExact, 10, 160);
	grpCPUSpeed->add(lblCpuIdle, 10, 190);
	grpCPUSpeed->add(sldCpuIdle, lblCpuIdle->getX() + lblCpuIdle->getWidth() + DISTANCE_NEXT_X/2, 190);
	grpCPUSpeed->add(lblCpuIdleInfo, sldCpuIdle->getX() + sldCpuIdle->getWidth() + DISTANCE_NEXT_X / 2, 190);
	grpCPUSpeed->setMovable(false);
	grpCPUSpeed->setSize(190, 285);
	grpCPUSpeed->setTitleBarHeight(TITLEBAR_HEIGHT);
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
	delete opt25Mhz;
	delete optFastest;
	delete lblCpuIdle;
	delete sldCpuIdle;
	delete lblCpuIdleInfo;
	delete chkCPUCycleExact;
	delete cpuIdleActionListener;
	
	delete grpCPUSpeed;
	delete cpuSpeedButtonActionListener;
}


void RefreshPanelCPU()
{
	if (changed_prefs.cpu_model == 68000)
		optCPU68000->setSelected(true);
	else if (changed_prefs.cpu_model == 68010)
		optCPU68010->setSelected(true);
	else if (changed_prefs.cpu_model == 68020)
		optCPU68020->setSelected(true);
	else if (changed_prefs.cpu_model == 68030)
		optCPU68030->setSelected(true);
	else if (changed_prefs.cpu_model == 68040)
		optCPU68040->setSelected(true);

	chk24Bit->setSelected(changed_prefs.address_space_24);
	chk24Bit->setEnabled(changed_prefs.cpu_model == 68020 && changed_prefs.cachesize == 0);
	
	chkCPUCompatible->setSelected(changed_prefs.cpu_compatible > 0);
	chkCPUCompatible->setEnabled(changed_prefs.cachesize == 0);
	
	chkCPUCycleExact->setSelected(changed_prefs.cpu_cycle_exact > 0);
	chkCPUCycleExact->setEnabled(changed_prefs.cpu_model <= 68010);
	
	chkJIT->setEnabled(changed_prefs.cpu_model >= 68020);
	chkJIT->setSelected(changed_prefs.cachesize > 0);

	switch (changed_prefs.fpu_model)
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
	optFPU68881->setEnabled(changed_prefs.cpu_model >= 68020 && changed_prefs.cpu_model < 68040);
	optFPU68882->setEnabled(changed_prefs.cpu_model >= 68020 && changed_prefs.cpu_model < 68040);
	optFPUinternal->setEnabled(changed_prefs.cpu_model == 68040);

	chkFPUstrict->setSelected(changed_prefs.fpu_strict);

#ifdef USE_JIT_FPU
	chkFPUJIT->setEnabled(changed_prefs.cachesize > 0);
	chkFPUJIT->setSelected(changed_prefs.compfpu);
#else
	chkFPUJIT->setSelected(false);
	chkFPUJIT->setEnabled(false);
#endif

	if (changed_prefs.m68k_speed == M68K_SPEED_7MHZ_CYCLES)
		opt7Mhz->setSelected(true);
	else if (changed_prefs.m68k_speed == M68K_SPEED_14MHZ_CYCLES)
		opt14Mhz->setSelected(true);
	else if (changed_prefs.m68k_speed == M68K_SPEED_25MHZ_CYCLES)
		opt25Mhz->setSelected(true);
	else if (changed_prefs.m68k_speed == -1)
		optFastest->setSelected(true);

	sldCpuIdle->setValue(changed_prefs.cpu_idle == 0 ? 0 : 12 - changed_prefs.cpu_idle / 15);
	const auto cpu_idle = to_string((changed_prefs.cpu_idle == 0 ? 0 : 12 - changed_prefs.cpu_idle / 15) * 10);
	lblCpuIdleInfo->setCaption(cpu_idle);
}

bool HelpPanelCPU(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("Select the required Amiga CPU (68000 - 68040).");
	helptext.emplace_back("If you select 68020, you can choose between 24-bit addressing (68EC020) or 32-bit");
	helptext.emplace_back("addressing (68020). The option \"More compatible\" is only available if 68000 or 68010");
	helptext.emplace_back("is selected and emulates simple prefetch of the 68000. This may improve compatibility");
	helptext.emplace_back("in few situations but is not required for most games and demos.");
	helptext.emplace_back(" ");
	helptext.emplace_back("JIT/JIT FPU enables the Just-in-time compiler. This may break compatibility in some games.");
	helptext.emplace_back(" ");
	helptext.emplace_back("The available FPU models depending on the selected CPU.");
	helptext.emplace_back("The option \"More compatible\" activates more accurate rounding and compare of two floats.");
	helptext.emplace_back(" ");
	helptext.emplace_back("With \"CPU Speed\" you can choose the clock rate of the emulated Amiga.");
	helptext.emplace_back("Use 7MHz for A500 or 14MHz for A1200 speed. Fastest Possible will give only the minimum time");
	helptext.emplace_back("to the Chipset, using as much as possible for the CPU, which might result in dropping");
	helptext.emplace_back("frames also.");
	helptext.emplace_back("\"Cycle exact\" emulates 68000 and chipset cycle accurate. This is very slow and only required in few situations.");
	helptext.emplace_back(" ");
	helptext.emplace_back("You can use the CPU Idle slider to set how much the CPU emulation should sleep when idle.");
	helptext.emplace_back("This is useful to keep the system temperature down.");
	helptext.emplace_back(" ");
	helptext.emplace_back("In current version, you will not see a difference in the performance for 68020,");
	helptext.emplace_back("68030 and 68040 CPUs. The CPU cycles for the opcodes are based on 68020. The different");
	helptext.emplace_back("cycles for 68030 and 68040 may come in a later version.");
	return true;
}
