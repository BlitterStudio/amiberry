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
static gcn::RadioButton* optCPU68060;
static gcn::CheckBox* chk24Bit;
static gcn::CheckBox* chkCPUCompatible;
static gcn::CheckBox* chkJIT;

static gcn::Window* grpFPU;
static gcn::RadioButton* optFPUnone;
static gcn::RadioButton* optFPU68881;
static gcn::RadioButton* optFPU68882;
static gcn::RadioButton* optFPUinternal;
static gcn::CheckBox* chkFPUStrict;

static gcn::Window* grpCPUSpeed;
static gcn::RadioButton* optCPUSpeedFastest;
static gcn::RadioButton* optCPUSpeedReal;
static gcn::Label* lblCpuSpeed;
static gcn::Slider* sldCpuSpeed;
static gcn::Label* lblCpuSpeedInfo;
static gcn::Label* lblCpuIdle;
static gcn::Slider* sldCpuIdle;
static gcn::Label* lblCpuIdleInfo;

static gcn::Window* grpCPUCycleExact;
static gcn::Label* lblCPUFrequency;
static gcn::DropDown* cboCPUFrequency;
static gcn::Label* lblCPUFrequencyMHz;

static gcn::Window* grpAdvJitSettings;
static gcn::Label* lblJitCacheSize;
static gcn::Slider* sldJitCacheSize;
static gcn::Label* lblJitCacheSizeInfo;
static gcn::CheckBox* chkFPUJIT;
static gcn::CheckBox* chkConstantJump;
static gcn::CheckBox* chkHardFlush;
static gcn::RadioButton* optDirect;
static gcn::RadioButton* optIndirect;
static gcn::CheckBox* chkNoFlags;
static gcn::CheckBox* chkCatchExceptions;

class string_list_model : public gcn::ListModel
{
	std::vector<std::string> values{};
public:
	string_list_model(const char* entries[], const int count)
	{
		for (auto i = 0; i < count; ++i)
			values.emplace_back(entries[i]);
	}

	int getNumberOfElements() override
	{
		return values.size();
	}

	int add_element(const char* Elem) override
	{
		values.emplace_back(Elem);
		return 0;
	}

	void clear_elements() override
	{
		values.clear();
	}

	std::string getElementAt(const int i) override
	{
		if (i < 0 || i >= static_cast<int>(values.size()))
			return "---";
		return values[i];
	}
};

const char* cpu_freq_values[] = { "1x", "2x (A500)", "4x (A1200)", "8x", "16x" };
string_list_model cpu_freq_list(cpu_freq_values, 5);

static float getcpufreq(int m)
{
	const float f = changed_prefs.ntscmode ? 28636360.0f : 28375160.0f;
	return f * (m >> 8) / 8.0f;
}

class CPUActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		changed_prefs.cpu_compatible = changed_prefs.cpu_memory_cycle_exact || chkCPUCompatible->isSelected();
		changed_prefs.fpu_strict = chkFPUStrict->isSelected();
		changed_prefs.address_space_24 = chk24Bit->isSelected();
		changed_prefs.m68k_speed = optCPUSpeedFastest->isSelected() ? -1 : 0;
		changed_prefs.m68k_speed_throttle = static_cast<float>(sldCpuSpeed->getValue() * 100);
		if (changed_prefs.m68k_speed_throttle > 0 && changed_prefs.m68k_speed < 0)
			changed_prefs.m68k_speed_throttle = 0;

		int newcpu = optCPU68000->isSelected()
			             ? 68000
			             : optCPU68010->isSelected()
			             ? 68010
			             : optCPU68020->isSelected()
			             ? 68020
			             : optCPU68030->isSelected()
			             ? 68030
			             : optCPU68040->isSelected()
			             ? 68040
			             : optCPU68060->isSelected()
			             ? 68060
			             : 0;
		int newfpu = optFPUnone->isSelected()
			             ? 0
			             : optFPU68881->isSelected()
			             ? 1
			             : optFPU68882->isSelected()
			             ? 2
			             : optFPUinternal->isSelected()
			             ? 3
			             : 0;

		// When switching away from 68000, disable 24 bit addressing.
		int oldcpu = changed_prefs.cpu_model;
		if (changed_prefs.cpu_model != newcpu && newcpu <= 68010)
			newfpu = 0;
		changed_prefs.cpu_model = newcpu;
		changed_prefs.mmu_model = 0;
		changed_prefs.mmu_ec = false;
		changed_prefs.cpu_data_cache = false;
		switch (newcpu)
		{
		case 68000:
		case 68010:
			changed_prefs.fpu_model = newfpu == 0 ? 0 : (newfpu == 2 ? 68882 : 68881);
			if (changed_prefs.cpu_compatible || changed_prefs.cpu_memory_cycle_exact)
				changed_prefs.fpu_model = 0;
			if (newcpu != oldcpu)
				changed_prefs.address_space_24 = 1;
			break;
		case 68020:
			changed_prefs.fpu_model = newfpu == 0 ? 0 : (newfpu == 2 ? 68882 : 68881);
			break;
		case 68030:
			if (newcpu != oldcpu)
				changed_prefs.address_space_24 = 0;
			changed_prefs.fpu_model = newfpu == 0 ? 0 : (newfpu == 2 ? 68882 : 68881);
			changed_prefs.mmu_ec = false; //ischecked(hDlg, IDC_MMUENABLEEC);
			changed_prefs.mmu_model = 0; //changed_prefs.mmu_ec || ischecked(hDlg, IDC_MMUENABLE) ? 68030 : 0;
			if (changed_prefs.cpu_compatible)
				changed_prefs.cpu_data_cache = false; //ischecked(hDlg, IDC_CPUDATACACHE);
			break;
		case 68040:
			changed_prefs.fpu_model = newfpu ? 68040 : 0;
			changed_prefs.address_space_24 = 0;
			if (changed_prefs.fpu_model)
				changed_prefs.fpu_model = 68040;
			changed_prefs.mmu_ec = false; //ischecked(hDlg, IDC_MMUENABLEEC);
			changed_prefs.mmu_model = 0; //changed_prefs.mmu_ec || ischecked(hDlg, IDC_MMUENABLE) ? 68040 : 0;
			if (changed_prefs.cpu_compatible)
				changed_prefs.cpu_data_cache = false; //ischecked(hDlg, IDC_CPUDATACACHE);
			break;
		case 68060:
			changed_prefs.fpu_model = newfpu ? 68060 : 0;
			changed_prefs.address_space_24 = 0;
			changed_prefs.mmu_ec = false; //ischecked(hDlg, IDC_MMUENABLEEC);
			changed_prefs.mmu_model = 0; //changed_prefs.mmu_ec || ischecked(hDlg, IDC_MMUENABLE) ? 68060 : 0;
			if (changed_prefs.cpu_compatible)
				changed_prefs.cpu_data_cache = false; //ischecked(hDlg, IDC_CPUDATACACHE);
			break;
		}

		if (newcpu != oldcpu && changed_prefs.cpu_compatible) {
			int idx = 0;
			if (newcpu <= 68010) {
				changed_prefs.cpu_clock_multiplier = 2 * 256;
				idx = 1;
			}
			else if (newcpu == 68020) {
				changed_prefs.cpu_clock_multiplier = 4 * 256;
				idx = 2;
			}
			else {
				changed_prefs.cpu_clock_multiplier = 8 * 256;
				idx = 3;
			}
			cboCPUFrequency->setSelected(idx);
		}

		auto newtrust = optDirect->isSelected() ? 0 : 1;
		changed_prefs.comptrustbyte = newtrust;
		changed_prefs.comptrustword = newtrust;
		changed_prefs.comptrustlong = newtrust;
		changed_prefs.comptrustnaddr = newtrust;

		changed_prefs.comp_catchfault = chkCatchExceptions->isSelected();
		changed_prefs.compnf = chkNoFlags->isSelected();
		changed_prefs.compfpu = chkFPUJIT->isSelected();
		changed_prefs.comp_hardflush = chkHardFlush->isSelected();
		changed_prefs.comp_constjump = chkConstantJump->isSelected();

#ifdef JIT
		static int cachesize_prev, trust_prev;
		int oldcache = changed_prefs.cachesize;
		int jitena = (chkJIT->isSelected() ? 1 : 0) && !changed_prefs.address_space_24 && changed_prefs.cpu_model >= 68020;
		int idx = (int)sldJitCacheSize->getValue();
		changed_prefs.cachesize = 1024 << idx;
		if (changed_prefs.cachesize <= 1024)
			changed_prefs.cachesize = 0;
		else
			changed_prefs.cachesize /= 2;
		if (!jitena) {
			cachesize_prev = changed_prefs.cachesize;
			trust_prev = changed_prefs.comptrustbyte;
			changed_prefs.cachesize = 0;
		}
		else if (jitena && !oldcache) {
			changed_prefs.cachesize = MAX_JIT_CACHE;
			changed_prefs.cpu_cycle_exact = false;
			changed_prefs.cpu_memory_cycle_exact = false;
			if (!cachesize_prev)
				trust_prev = 0;
			if (cachesize_prev) {
				changed_prefs.cachesize = cachesize_prev;
			}
			changed_prefs.comptrustbyte = trust_prev;
			changed_prefs.comptrustword = trust_prev;
			changed_prefs.comptrustlong = trust_prev;
			changed_prefs.comptrustnaddr = trust_prev;
			if (changed_prefs.fpu_mode > 0) {
				changed_prefs.compfpu = false;
				chkFPUJIT->setSelected(false);
			}
		}
		if (!changed_prefs.cachesize) {
			chkFPUJIT->setSelected(false);
		}
		if (changed_prefs.cachesize && changed_prefs.compfpu && changed_prefs.fpu_mode > 0) {
			changed_prefs.fpu_mode = 0;
		}
		if (oldcache == 0 && changed_prefs.cachesize > 0) {
			canbang = 1;
		}
		if (changed_prefs.cachesize && changed_prefs.cpu_model >= 68040) {
			changed_prefs.cpu_compatible = false;
		}
#endif

		changed_prefs.cpu_idle = static_cast<int>(sldCpuIdle->getValue());
		if (changed_prefs.cpu_idle > 0)
			changed_prefs.cpu_idle = (12 - changed_prefs.cpu_idle) * 15;

		const int freq_idx = cboCPUFrequency->getSelected();
		const int m = changed_prefs.cpu_clock_multiplier;
		changed_prefs.cpu_frequency = 0;
		changed_prefs.cpu_clock_multiplier = 0;
		if (freq_idx < 5) {
			changed_prefs.cpu_clock_multiplier = (1 << 8) << freq_idx;
			if (changed_prefs.cpu_cycle_exact || changed_prefs.cpu_compatible) {
				TCHAR txt[20];
				const auto f = getcpufreq(changed_prefs.cpu_clock_multiplier);
				_stprintf(txt, _T("%.6f"), f / 1000000.0);
				lblCPUFrequencyMHz->setCaption(txt);
			}
			else {
				lblCPUFrequencyMHz->setCaption("");
			}
		}
		else if (changed_prefs.cpu_cycle_exact) {
			const auto& txt = lblCPUFrequencyMHz->getCaption();
			changed_prefs.cpu_clock_multiplier = 0;
			changed_prefs.cpu_frequency = (int)(_tstof(txt.c_str()) * 1000000.0);
			if (changed_prefs.cpu_frequency < 1 * 1000000)
				changed_prefs.cpu_frequency = 0;
			if (changed_prefs.cpu_frequency >= 99 * 1000000)
				changed_prefs.cpu_frequency = 0;
			if (!changed_prefs.cpu_frequency) {
				changed_prefs.cpu_frequency = (int)(getcpufreq(m) * 1000000.0);
			}
		}

		RefreshPanelCPU();
		RefreshPanelRAM();
		RefreshPanelChipset();
	}
};

static CPUActionListener* cpuActionListener;

void InitPanelCPU(const struct config_category& category)
{
	cpuActionListener = new CPUActionListener();

	optCPU68000 = new gcn::RadioButton("68000", "radiocpugroup");
	optCPU68000->setId("optCPU68000");
	optCPU68000->addActionListener(cpuActionListener);
	optCPU68010 = new gcn::RadioButton("68010", "radiocpugroup");
	optCPU68010->setId("optCPU68010");
	optCPU68010->addActionListener(cpuActionListener);
	optCPU68020 = new gcn::RadioButton("68020", "radiocpugroup");
	optCPU68020->setId("optCPU68020");
	optCPU68020->addActionListener(cpuActionListener);
	optCPU68030 = new gcn::RadioButton("68030", "radiocpugroup");
	optCPU68030->setId("optCPU68030");
	optCPU68030->addActionListener(cpuActionListener);
	optCPU68040 = new gcn::RadioButton("68040", "radiocpugroup");
	optCPU68040->setId("optCPU68040");
	optCPU68040->addActionListener(cpuActionListener);
	optCPU68060 = new gcn::RadioButton("68060", "radiocpugroup");
	optCPU68060->setId("optCPU68060");
	optCPU68060->addActionListener(cpuActionListener);

	chk24Bit = new gcn::CheckBox("24-bit addressing", true);
	chk24Bit->setId("chk24Bit");
	chk24Bit->addActionListener(cpuActionListener);

	chkCPUCompatible = new gcn::CheckBox("More compatible", true);
	chkCPUCompatible->setId("chkCPUCompatible");
	chkCPUCompatible->addActionListener(cpuActionListener);

	chkJIT = new gcn::CheckBox("JIT", true);
	chkJIT->setId("chkJIT");
	chkJIT->addActionListener(cpuActionListener);

	grpCPU = new gcn::Window("CPU");
	grpCPU->setPosition(DISTANCE_BORDER / 2, DISTANCE_BORDER / 2);
	grpCPU->add(optCPU68000, 10, 10);
	grpCPU->add(optCPU68010, 10, 40);
	grpCPU->add(optCPU68020, 10, 70);
	grpCPU->add(optCPU68030, 10, 100);
	grpCPU->add(optCPU68040, 10, 130);
	grpCPU->add(optCPU68060, 10, 160);
	grpCPU->add(chk24Bit, 10, 200);
	grpCPU->add(chkCPUCompatible, 10, 230);
	grpCPU->add(chkJIT, 10, 260);
	grpCPU->setMovable(false);
	grpCPU->setSize(chk24Bit->getWidth() + 20, 315);
	grpCPU->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpCPU->setBaseColor(gui_baseCol);

	category.panel->add(grpCPU);

	optFPUnone = new gcn::RadioButton("None", "radiofpugroup");
	optFPUnone->setId("optFPUnone");
	optFPUnone->addActionListener(cpuActionListener);

	optFPU68881 = new gcn::RadioButton("68881", "radiofpugroup");
	optFPU68881->setId("optFPU68881");
	optFPU68881->addActionListener(cpuActionListener);

	optFPU68882 = new gcn::RadioButton("68882", "radiofpugroup");
	optFPU68882->setId("optFPU68882");
	optFPU68882->addActionListener(cpuActionListener);

	optFPUinternal = new gcn::RadioButton("CPU internal", "radiofpugroup");
	optFPUinternal->setId("optFPUinternal");
	optFPUinternal->addActionListener(cpuActionListener);

	chkFPUStrict = new gcn::CheckBox("More compatible", true);
	chkFPUStrict->setId("chkFPUStrict");
	chkFPUStrict->addActionListener(cpuActionListener);

	grpFPU = new gcn::Window("FPU");
	grpFPU->setPosition(grpCPU->getX(), grpCPU->getY() + grpCPU->getHeight() + DISTANCE_NEXT_Y / 2);
	grpFPU->add(optFPUnone, 10, 10);
	grpFPU->add(optFPU68881, 10, 40);
	grpFPU->add(optFPU68882, 10, 70);
	grpFPU->add(optFPUinternal, 10, 100);
	grpFPU->add(chkFPUStrict, 10, 130);
	grpFPU->setMovable(false);
	grpFPU->setSize(grpCPU->getWidth(), 185);
	grpFPU->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpFPU->setBaseColor(gui_baseCol);
	category.panel->add(grpFPU);

	optCPUSpeedFastest = new gcn::RadioButton("Fastest Possible", "radiocpuspeedgroup");
	optCPUSpeedFastest->setId("optCPUSpeedFastest");
	optCPUSpeedFastest->addActionListener(cpuActionListener);

	optCPUSpeedReal = new gcn::RadioButton("A500/A1200 or cycle exact", "radiocpuspeedgroup");
	optCPUSpeedReal->setId("optCPUSpeedReal");
	optCPUSpeedReal->addActionListener(cpuActionListener);

	lblCpuSpeed = new gcn::Label("CPU Speed:");
	sldCpuSpeed = new gcn::Slider(-9, 50);
	sldCpuSpeed->setSize(320, SLIDER_HEIGHT);
	sldCpuSpeed->setBaseColor(gui_baseCol);
	sldCpuSpeed->setMarkerLength(20);
	sldCpuSpeed->setStepLength(1);
	sldCpuSpeed->setId("sldCpuSpeed");
	sldCpuSpeed->addActionListener(cpuActionListener);
	lblCpuSpeedInfo = new gcn::Label("+000%");
	
	lblCpuIdle = new gcn::Label("CPU Idle:");
	sldCpuIdle = new gcn::Slider(0, 10);
	sldCpuIdle->setSize(90, SLIDER_HEIGHT);
	sldCpuIdle->setBaseColor(gui_baseCol);
	sldCpuIdle->setMarkerLength(20);
	sldCpuIdle->setStepLength(1);
	sldCpuIdle->setId("sldCpuIdle");
	sldCpuIdle->addActionListener(cpuActionListener);
	lblCpuIdleInfo = new gcn::Label("000");

	grpCPUSpeed = new gcn::Window("CPU Speed");
	grpCPUSpeed->setPosition(grpCPU->getX() + grpCPU->getWidth() + DISTANCE_NEXT_X, grpCPU->getY());
	grpCPUSpeed->add(optCPUSpeedFastest, 10, 10);
	grpCPUSpeed->add(optCPUSpeedReal, 10, 40);
	grpCPUSpeed->add(sldCpuSpeed, 30, 70);
	grpCPUSpeed->add(lblCpuSpeed, 10, 100);
	grpCPUSpeed->add(lblCpuSpeedInfo, lblCpuSpeed->getX() + lblCpuSpeed->getWidth() + DISTANCE_NEXT_X / 2, 100);
	grpCPUSpeed->add(lblCpuIdle, lblCpuSpeed->getX(), 130);
	grpCPUSpeed->add(sldCpuIdle, lblCpuSpeedInfo->getX(), 130);
	grpCPUSpeed->add(lblCpuIdleInfo, sldCpuIdle->getX() + sldCpuIdle->getWidth() + DISTANCE_NEXT_X / 2, 130);
	grpCPUSpeed->setMovable(false);
	grpCPUSpeed->setSize(395, 190);
	grpCPUSpeed->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpCPUSpeed->setBaseColor(gui_baseCol);

	category.panel->add(grpCPUSpeed);

	lblCPUFrequency = new gcn::Label("CPU Frequency:");
	cboCPUFrequency = new gcn::DropDown(&cpu_freq_list);
	cboCPUFrequency->setSize(100, cboCPUFrequency->getHeight());
	cboCPUFrequency->setBaseColor(gui_baseCol);
	cboCPUFrequency->setBackgroundColor(colTextboxBackground);
	cboCPUFrequency->addActionListener(cpuActionListener);
	cboCPUFrequency->setId("cboCPUFrequency");
	lblCPUFrequencyMHz = new gcn::Label("50 MHz");

	grpCPUCycleExact = new gcn::Window("Cycle-Exact CPU Emulation Speed");
	grpCPUCycleExact->setPosition(grpCPUSpeed->getX(), grpCPUSpeed->getY() + grpCPUSpeed->getHeight() + DISTANCE_NEXT_Y / 2);
	grpCPUCycleExact->add(lblCPUFrequency, 10, 10);
	grpCPUCycleExact->add(cboCPUFrequency, lblCPUFrequency->getX() + lblCPUFrequency->getWidth() + DISTANCE_NEXT_X / 2, 10);
	grpCPUCycleExact->add(lblCPUFrequencyMHz, cboCPUFrequency->getX() + cboCPUFrequency->getWidth() + DISTANCE_NEXT_X, 10);
	grpCPUCycleExact->setMovable(false);
	grpCPUCycleExact->setSize(grpCPUSpeed->getWidth(), 140);
	grpCPUCycleExact->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpCPUCycleExact->setBaseColor(gui_baseCol);

	category.panel->add(grpCPUCycleExact);

	lblJitCacheSize = new gcn::Label("Cache size:");
	sldJitCacheSize = new gcn::Slider(0, 5);
	sldJitCacheSize->setSize(150, SLIDER_HEIGHT);
	sldJitCacheSize->setBaseColor(gui_baseCol);
	sldJitCacheSize->setMarkerLength(20);
	sldJitCacheSize->setStepLength(1);
	sldJitCacheSize->setId("sldJitCacheSize");
	sldJitCacheSize->addActionListener(cpuActionListener);
	lblJitCacheSizeInfo = new gcn::Label("0 MB");

	chkFPUJIT = new gcn::CheckBox("FPU Support", true);
	chkFPUJIT->setId("chkFPUJIT");
	chkFPUJIT->addActionListener(cpuActionListener);
	chkConstantJump = new gcn::CheckBox("Constant jump");
	chkConstantJump->setId("chkConstantJump");
	chkConstantJump->addActionListener(cpuActionListener);
	chkHardFlush = new gcn::CheckBox("Hard flush");
	chkHardFlush->setId("chkHardFlush");
	chkHardFlush->addActionListener(cpuActionListener);
	optDirect = new gcn::RadioButton("Direct", "radiomemaccessgroup");
	optDirect->setId("optDirect");
	optDirect->addActionListener(cpuActionListener);
	optIndirect = new gcn::RadioButton("Indirect", "radiomemaccessgroup");
	optIndirect->setId("optIndirect");
	optIndirect->addActionListener(cpuActionListener);
	chkNoFlags = new gcn::CheckBox("No flags");
	chkNoFlags->setId("chkNoFlags");
	chkNoFlags->addActionListener(cpuActionListener);
	chkCatchExceptions = new gcn::CheckBox("Catch unexpected exceptions");
	chkCatchExceptions->setId("chkCatchExceptions");
	chkCatchExceptions->addActionListener(cpuActionListener);

	grpAdvJitSettings = new gcn::Window("Advanced JIT Settings");
	grpAdvJitSettings->setPosition(grpCPUCycleExact->getX(), grpCPUCycleExact->getY() + grpCPUCycleExact->getHeight() + DISTANCE_NEXT_Y / 2);
	grpAdvJitSettings->add(lblJitCacheSize, 10, 10);
	grpAdvJitSettings->add(sldJitCacheSize, lblJitCacheSize->getX() + lblJitCacheSize->getWidth() + DISTANCE_NEXT_X / 2, 10);
	grpAdvJitSettings->add(lblJitCacheSizeInfo, sldJitCacheSize->getX() + sldJitCacheSize->getWidth() + DISTANCE_NEXT_X / 2, 10);
	grpAdvJitSettings->add(chkFPUJIT, lblJitCacheSize->getX(), lblJitCacheSize->getY() + lblJitCacheSize->getHeight() + DISTANCE_NEXT_Y);
	grpAdvJitSettings->add(chkConstantJump, chkFPUJIT->getX() + chkFPUJIT->getWidth() + DISTANCE_NEXT_X, chkFPUJIT->getY());
	grpAdvJitSettings->add(chkHardFlush, chkConstantJump->getX() + chkConstantJump->getWidth() + DISTANCE_NEXT_X, chkConstantJump->getY());
	grpAdvJitSettings->add(optDirect, chkFPUJIT->getX(), chkFPUJIT->getY() + chkFPUJIT->getHeight() + DISTANCE_NEXT_Y);
	grpAdvJitSettings->add(optIndirect, chkConstantJump->getX(), chkConstantJump->getY() + chkConstantJump->getHeight() + DISTANCE_NEXT_Y);
	grpAdvJitSettings->add(chkNoFlags, chkHardFlush->getX(), chkHardFlush->getY() + chkHardFlush->getHeight() + DISTANCE_NEXT_Y);
	grpAdvJitSettings->add(chkCatchExceptions, optDirect->getX(), optDirect->getY() + optDirect->getHeight() + DISTANCE_NEXT_Y);
	grpAdvJitSettings->setMovable(false);
	grpAdvJitSettings->setSize(grpCPUCycleExact->getWidth(), 160);
	grpAdvJitSettings->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpAdvJitSettings->setBaseColor(gui_baseCol);

	category.panel->add(grpAdvJitSettings);

	RefreshPanelCPU();
}

void ExitPanelCPU()
{
	delete optCPU68000;
	delete optCPU68010;
	delete optCPU68020;
	delete optCPU68030;
	delete optCPU68040;
	delete optCPU68060;
	delete chk24Bit;
	delete chkCPUCompatible;
	delete chkJIT;
	delete grpCPU;
	delete cpuActionListener;

	delete optFPUnone;
	delete optFPU68881;
	delete optFPU68882;
	delete optFPUinternal;
	delete chkFPUStrict;
	delete grpFPU;

	delete optCPUSpeedFastest;
	delete optCPUSpeedReal;
	delete lblCpuSpeed;
	delete sldCpuSpeed;
	delete lblCpuSpeedInfo;
	delete lblCpuIdle;
	delete sldCpuIdle;
	delete lblCpuIdleInfo;

	delete grpCPUSpeed;

	delete grpCPUCycleExact;
	delete lblCPUFrequency;
	delete cboCPUFrequency;
	delete lblCPUFrequencyMHz;

	delete lblJitCacheSize;
	delete sldJitCacheSize;
	delete lblJitCacheSizeInfo;
	delete chkFPUJIT;
	delete chkConstantJump;
	delete chkHardFlush;
	delete optDirect;
	delete optIndirect;
	delete chkNoFlags;
	delete chkCatchExceptions;
	delete grpAdvJitSettings;
}

void RefreshPanelCPU()
{
	int idx = 5;
	if (changed_prefs.cpu_clock_multiplier >= 1 << 8) {
		idx = 0;
		while (idx < 4) {
			if (changed_prefs.cpu_clock_multiplier <= (1 << 8) << idx)
				break;
			idx++;
		}
	}
	else if (changed_prefs.cpu_clock_multiplier == 0 && changed_prefs.cpu_frequency == 0 && changed_prefs.cpu_model <= 68010) {
		idx = 1; // A500
	}
	else if (changed_prefs.cpu_clock_multiplier == 0 && changed_prefs.cpu_frequency == 0 && changed_prefs.cpu_model >= 68020) {
		idx = 2; // A1200
	}
	if (!changed_prefs.cpu_cycle_exact) {
		changed_prefs.cpu_frequency = 0;
		if (!changed_prefs.cpu_clock_multiplier && (idx == 1 || idx == 2)) {
			changed_prefs.cpu_clock_multiplier = (1 << idx) << 8;
		}
	}
	else {
		if (!changed_prefs.cpu_frequency && (idx == 1 || idx == 2)) {
			changed_prefs.cpu_clock_multiplier = (1 << idx) << 8;
		}
	}
	cboCPUFrequency->setSelected(idx);
	if (!changed_prefs.cpu_clock_multiplier) {
		TCHAR txt[20];
		_stprintf(txt, _T("%.6f"), changed_prefs.cpu_frequency / 1000000.0);
		lblCPUFrequencyMHz->setCaption(txt);
	}
	else {
		TCHAR txt[20];
		double f = getcpufreq(changed_prefs.cpu_clock_multiplier);
		_stprintf(txt, _T("%.6f"), f / 1000000.0);
		lblCPUFrequencyMHz->setCaption(txt);
	}

	// Set Enable status
	sldCpuSpeed->setEnabled(!changed_prefs.cpu_cycle_exact);
	chk24Bit->setEnabled(changed_prefs.cpu_model <= 68030 && changed_prefs.cachesize == 0);
	sldCpuIdle->setEnabled(changed_prefs.m68k_speed != 0);
	lblCpuIdleInfo->setEnabled(sldCpuIdle->isEnabled());

	bool cpu_based_enable = changed_prefs.cpu_model >= 68020 && changed_prefs.address_space_24 == 0;
	bool jitenable = cpu_based_enable && !changed_prefs.mmu_model;
#ifndef JIT
	jitenable = false;
#endif
	bool enable = jitenable && changed_prefs.cachesize;

	optDirect->setEnabled(enable);
	optIndirect->setEnabled(enable);
	chkHardFlush->setEnabled(enable);
	chkConstantJump->setEnabled(enable);
	chkFPUJIT->setEnabled(enable);
	chkCatchExceptions->setEnabled(enable);
	chkNoFlags->setEnabled(enable);
	lblJitCacheSizeInfo->setEnabled(enable);
	sldJitCacheSize->setEnabled(enable);
	chkJIT->setEnabled(enable);
	chkCPUCompatible->setEnabled(!changed_prefs.cpu_memory_cycle_exact && !(changed_prefs.cachesize && changed_prefs.cpu_model >= 68040));
	chkFPUStrict->setEnabled(changed_prefs.fpu_model > 0);

	cboCPUFrequency->setEnabled((changed_prefs.cpu_cycle_exact || changed_prefs.cpu_compatible) && changed_prefs.m68k_speed >= 0);
	lblCPUFrequencyMHz->setEnabled(changed_prefs.cpu_cycle_exact && !changed_prefs.cpu_clock_multiplier && changed_prefs.m68k_speed >= 0);

	optFPU68881->setEnabled(changed_prefs.cpu_model < 68040 && (changed_prefs.cpu_model >= 68020 || !changed_prefs.cpu_compatible));
	optFPU68882->setEnabled(changed_prefs.cpu_model < 68040 && (changed_prefs.cpu_model >= 68020 || !changed_prefs.cpu_compatible));
	optFPUinternal->setEnabled(changed_prefs.cpu_model >= 68040);

	// Set Values
	sldCpuSpeed->setValue(static_cast<int>(changed_prefs.m68k_speed_throttle / 100));
	auto cpu_speed_info = to_string(static_cast<int>(changed_prefs.m68k_speed_throttle / 10));
	cpu_speed_info.append("%");
	lblCpuSpeedInfo->setCaption(cpu_speed_info);

	chkCPUCompatible->setSelected(changed_prefs.cpu_compatible);
	chk24Bit->setSelected(changed_prefs.address_space_24);
	chkFPUStrict->setSelected(changed_prefs.fpu_strict);

	sldCpuIdle->setValue(changed_prefs.cpu_idle == 0 ? 0 : 12 - changed_prefs.cpu_idle / 15);
	auto cpu_idle_string = to_string((changed_prefs.cpu_idle == 0 ? 0 : 12 - changed_prefs.cpu_idle / 15) * 10);
	cpu_idle_string.append("%");
	lblCpuIdleInfo->setCaption(cpu_idle_string);

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
	else if (changed_prefs.cpu_model == 68060)
		optCPU68060->setSelected(true);

	if (changed_prefs.fpu_model == 68881)
		optFPU68881->setSelected(true);
	else if (changed_prefs.fpu_model == 68882)
		optFPU68882->setSelected(true);
	else if (changed_prefs.fpu_model >= 68040)
		optFPUinternal->setSelected(true);
	else
		optFPUnone->setSelected(true);

	if (changed_prefs.m68k_speed < 0)
		optCPUSpeedFastest->setSelected(true);
	else if (changed_prefs.m68k_speed >= 0)
		optCPUSpeedReal->setSelected(true);
	
	if (changed_prefs.comptrustbyte == 0)
		optDirect->setSelected(true);
	else
		optIndirect->setSelected(true);

	idx = 0;
	for (int i = 0; i < MAX_CACHE_SIZE; i++) {
		if (changed_prefs.cachesize >= (1024 << i) && changed_prefs.cachesize < (1024 << i) * 2) {
			idx = i + 1;
			break;
		}
	}
	sldJitCacheSize->setValue(idx);
	auto cache_size_string = to_string(changed_prefs.cachesize / 1024);
	cache_size_string.append(" MB");
	lblJitCacheSizeInfo->setCaption(cache_size_string);

	chkCatchExceptions->setSelected(changed_prefs.comp_catchfault);
	chkNoFlags->setSelected(changed_prefs.compnf);
#ifdef USE_JIT_FPU
	chkFPUJIT->setSelected(changed_prefs.compfpu);
#else
	chkFPUJIT->setSelected(false);
#endif
	chkHardFlush->setSelected(changed_prefs.comp_hardflush);
	chkConstantJump->setSelected(changed_prefs.comp_constjump);
#ifdef JIT
	chkJIT->setSelected(changed_prefs.cachesize > 0);
#else
	chkJIT->setSelected(false);
#endif
}

bool HelpPanelCPU(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("Select the required Amiga CPU (68000 - 68040).");
	helptext.emplace_back("If you select 68020, you can choose between 24-bit addressing (68EC020) or 32-bit");
	helptext.emplace_back("addressing (68020). The option \"More compatible\" will emulate prefetch (68000) or");
	helptext.emplace_back("prefetch and instruction cache. It's more compatible but slower, and not required");
	helptext.emplace_back("for most games and demos.");
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

	return true;
}
