#include <guisan.hpp>
#include <guisan/sdl.hpp>

#include "SelectorEntry.hpp"
#include "StringListModel.h"

#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "cpuboard.h"
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
static gcn::CheckBox* chkCpuDataCache;
static gcn::CheckBox* chkJIT;

static gcn::Window* grpMMU;
static gcn::RadioButton* optMMUNone;
static gcn::RadioButton* optMMUEnabled;
static gcn::RadioButton* optMMUEC;

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

static gcn::CheckBox* chkCPUMultiThread;

static gcn::Window* grpPPCOptions;
static gcn::CheckBox* chkPPCEnabled;
static gcn::Label* lblPPCIdle;
static gcn::Slider* sldPPCIdle;
static gcn::Label* lblPPCIdleInfo;

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

static const std::vector<std::string> cpu_freq_values = { "1x", "2x (A500)", "4x (A1200)", "8x", "16x" };
static gcn::StringListModel cpu_freq_list(cpu_freq_values);

static float getcpufreq(int m)
{
	const float f = changed_prefs.ntscmode ? 28636360.0f : 28375160.0f;
	return f * static_cast<float>(m >> 8) / 8.0f;
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

		// 0 = Host (64-bit), -1 = Host (80-bit), 1 = Softfloat (80-bit)
		changed_prefs.fpu_mode = 0;

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
					changed_prefs.address_space_24 = true;
				break;
			case 68020:
				changed_prefs.fpu_model = newfpu == 0 ? 0 : (newfpu == 2 ? 68882 : 68881);
				break;
			case 68030:
				if (newcpu != oldcpu)
					changed_prefs.address_space_24 = false;
				changed_prefs.fpu_model = newfpu == 0 ? 0 : (newfpu == 2 ? 68882 : 68881);
				changed_prefs.mmu_ec = optMMUEC->isSelected();
				changed_prefs.mmu_model = changed_prefs.mmu_ec || optMMUEnabled->isSelected() ? 68030 : 0;
				if (changed_prefs.cpu_compatible)
					changed_prefs.cpu_data_cache = chkCpuDataCache->isSelected();
				break;
			case 68040:
				changed_prefs.fpu_model = newfpu ? 68040 : 0;
				changed_prefs.address_space_24 = false;
				if (changed_prefs.fpu_model)
					changed_prefs.fpu_model = 68040;
				changed_prefs.mmu_ec = optMMUEC->isSelected();
				changed_prefs.mmu_model = changed_prefs.mmu_ec || optMMUEnabled->isSelected() ? 68040 : 0;
				if (changed_prefs.cpu_compatible)
					changed_prefs.cpu_data_cache = chkCpuDataCache->isSelected();
				break;
			case 68060:
				changed_prefs.fpu_model = newfpu ? 68060 : 0;
				changed_prefs.address_space_24 = false;
				changed_prefs.mmu_ec = optMMUEC->isSelected();
				changed_prefs.mmu_model = changed_prefs.mmu_ec || optMMUEnabled->isSelected() ? 68060 : 0;
				if (changed_prefs.cpu_compatible)
					changed_prefs.cpu_data_cache = chkCpuDataCache->isSelected();
				break;
			default:
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
            changed_prefs.blitter_cycle_exact = false;
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
			if (changed_prefs.fpu_mode > 0 || changed_prefs.fpu_model == 0) {
				changed_prefs.compfpu = false;
				chkFPUJIT->setSelected(false);
			}
			else if (changed_prefs.fpu_model > 0) {
				changed_prefs.compfpu = true;
				chkFPUJIT->setSelected(true);
			}
		}
		if (!changed_prefs.cachesize) {
			chkFPUJIT->setSelected(false);
		}
		if (changed_prefs.cachesize && changed_prefs.compfpu && changed_prefs.fpu_mode > 0) {
			changed_prefs.fpu_mode = 0;
		}
		if (oldcache == 0 && changed_prefs.cachesize > 0) {
			canbang = true;
		}
		if (changed_prefs.cachesize && changed_prefs.cpu_model >= 68040) {
			changed_prefs.cpu_compatible = false;
		}
#endif

		if (chkPPCEnabled->isSelected())
		{
			if (changed_prefs.ppc_mode == 0)
				changed_prefs.ppc_mode = 1;
			if (changed_prefs.ppc_mode == 1 && changed_prefs.cpu_model < 68040)
				changed_prefs.ppc_mode = 0;
		}
		else if (!chkPPCEnabled->isSelected() && changed_prefs.ppc_mode == 1)
		{
			changed_prefs.ppc_mode = 0;
		}

		changed_prefs.cpu_idle = static_cast<int>(sldCpuIdle->getValue());
		if (changed_prefs.cpu_idle > 0)
			changed_prefs.cpu_idle = (12 - changed_prefs.cpu_idle) * 15;
		changed_prefs.ppc_cpu_idle = static_cast<int>(sldPPCIdle->getValue());
		std::string ppc_idle_info = changed_prefs.ppc_cpu_idle == 10
			? "max"
			: changed_prefs.ppc_cpu_idle == 0 ? "disabled"
			: std::to_string(changed_prefs.ppc_cpu_idle);
		lblPPCIdleInfo->setCaption(ppc_idle_info);

		const int freq_idx = cboCPUFrequency->getSelected();
		const int m = changed_prefs.cpu_clock_multiplier;
		changed_prefs.cpu_frequency = 0;
		changed_prefs.cpu_clock_multiplier = 0;
		if (freq_idx < 5) {
			changed_prefs.cpu_clock_multiplier = (1 << 8) << freq_idx;
			if (changed_prefs.cpu_cycle_exact || changed_prefs.cpu_compatible) {
				TCHAR txt[20];
				const auto f = getcpufreq(changed_prefs.cpu_clock_multiplier);
				_sntprintf(txt, sizeof txt, _T("%.6f"), f / 1000000.0);
				lblCPUFrequencyMHz->setCaption(txt);
			}
			else {
				lblCPUFrequencyMHz->setCaption("");
			}
		}
		else if (changed_prefs.cpu_cycle_exact) {
			const auto& txt = lblCPUFrequencyMHz->getCaption();
			changed_prefs.cpu_clock_multiplier = 0;
			changed_prefs.cpu_frequency = static_cast<int>((_tstof(txt.c_str()) * 1000000.0));
			if (changed_prefs.cpu_frequency < 1 * 1000000)
				changed_prefs.cpu_frequency = 0;
			if (changed_prefs.cpu_frequency >= 99 * 1000000)
				changed_prefs.cpu_frequency = 0;
			if (!changed_prefs.cpu_frequency) {
				changed_prefs.cpu_frequency = static_cast<int>(getcpufreq(m) * 1000000.0);
			}
		}

		changed_prefs.cpu_thread = chkCPUMultiThread->isSelected();

		RefreshPanelCPU();
		RefreshPanelRAM();
		RefreshPanelChipset();
		RefreshPanelRTG();
	}
};

static CPUActionListener* cpuActionListener;

void InitPanelCPU(const struct config_category& category)
{
	cpuActionListener = new CPUActionListener();

	optCPU68000 = new gcn::RadioButton("68000", "radiocpugroup");
	optCPU68000->setId("optCPU68000");
	optCPU68000->setBaseColor(gui_base_color);
	optCPU68000->setBackgroundColor(gui_background_color);
	optCPU68000->setForegroundColor(gui_foreground_color);
	optCPU68000->addActionListener(cpuActionListener);
	optCPU68010 = new gcn::RadioButton("68010", "radiocpugroup");
	optCPU68010->setId("optCPU68010");
	optCPU68010->setBaseColor(gui_base_color);
	optCPU68010->setBackgroundColor(gui_background_color);
	optCPU68010->setForegroundColor(gui_foreground_color);
	optCPU68010->addActionListener(cpuActionListener);
	optCPU68020 = new gcn::RadioButton("68020", "radiocpugroup");
	optCPU68020->setId("optCPU68020");
	optCPU68020->setBaseColor(gui_base_color);
	optCPU68020->setBackgroundColor(gui_background_color);
	optCPU68020->setForegroundColor(gui_foreground_color);
	optCPU68020->addActionListener(cpuActionListener);
	optCPU68030 = new gcn::RadioButton("68030", "radiocpugroup");
	optCPU68030->setId("optCPU68030");
	optCPU68030->setBaseColor(gui_base_color);
	optCPU68030->setBackgroundColor(gui_background_color);
	optCPU68030->setForegroundColor(gui_foreground_color);
	optCPU68030->addActionListener(cpuActionListener);
	optCPU68040 = new gcn::RadioButton("68040", "radiocpugroup");
	optCPU68040->setId("optCPU68040");
	optCPU68040->setBaseColor(gui_base_color);
	optCPU68040->setBackgroundColor(gui_background_color);
	optCPU68040->setForegroundColor(gui_foreground_color);
	optCPU68040->addActionListener(cpuActionListener);
	optCPU68060 = new gcn::RadioButton("68060", "radiocpugroup");
	optCPU68060->setId("optCPU68060");
	optCPU68060->setBaseColor(gui_base_color);
	optCPU68060->setBackgroundColor(gui_background_color);
	optCPU68060->setForegroundColor(gui_foreground_color);
	optCPU68060->addActionListener(cpuActionListener);

	chk24Bit = new gcn::CheckBox("24-bit addressing", true);
	chk24Bit->setId("chk24Bit");
	chk24Bit->setBaseColor(gui_base_color);
	chk24Bit->setBackgroundColor(gui_background_color);
	chk24Bit->setForegroundColor(gui_foreground_color);
	chk24Bit->addActionListener(cpuActionListener);

	chkCPUCompatible = new gcn::CheckBox("More compatible", true);
	chkCPUCompatible->setId("chkCPUCompatible");
	chkCPUCompatible->setBaseColor(gui_base_color);
	chkCPUCompatible->setBackgroundColor(gui_background_color);
	chkCPUCompatible->setForegroundColor(gui_foreground_color);
	chkCPUCompatible->addActionListener(cpuActionListener);

	chkCpuDataCache = new gcn::CheckBox("Data cache");
	chkCpuDataCache->setId("chkCpuDataCache");
	chkCpuDataCache->setBaseColor(gui_base_color);
	chkCpuDataCache->setBackgroundColor(gui_background_color);
	chkCpuDataCache->setForegroundColor(gui_foreground_color);
	chkCpuDataCache->addActionListener(cpuActionListener);

	chkJIT = new gcn::CheckBox("JIT", true);
	chkJIT->setId("chkJIT");
	chkJIT->setBaseColor(gui_base_color);
	chkJIT->setBackgroundColor(gui_background_color);
	chkJIT->setForegroundColor(gui_foreground_color);
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
	grpCPU->add(chkCpuDataCache, 10, 260);
	grpCPU->add(chkJIT, 10, 290);
	grpCPU->setMovable(false);
	grpCPU->setSize(chk24Bit->getWidth() + 20, TITLEBAR_HEIGHT + 290 + chkJIT->getHeight() + DISTANCE_NEXT_Y);
	grpCPU->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpCPU->setBaseColor(gui_base_color);
	grpCPU->setForegroundColor(gui_foreground_color);
	category.panel->add(grpCPU);

	optMMUNone = new gcn::RadioButton("None", "radiommugroup");
	optMMUNone->setId("optMMUNone");
	optMMUNone->setBaseColor(gui_base_color);
	optMMUNone->setBackgroundColor(gui_background_color);
	optMMUNone->setForegroundColor(gui_foreground_color);
	optMMUNone->addActionListener(cpuActionListener);
	optMMUEnabled = new gcn::RadioButton("MMU", "radiommugroup");
	optMMUEnabled->setId("optMMUEnabled");
	optMMUEnabled->setBaseColor(gui_base_color);
	optMMUEnabled->setBackgroundColor(gui_background_color);
	optMMUEnabled->setForegroundColor(gui_foreground_color);
	optMMUEnabled->addActionListener(cpuActionListener);
	optMMUEC = new gcn::RadioButton("EC", "radiommugroup");
	optMMUEC->setId("optMMUEC");
	optMMUEC->setBaseColor(gui_base_color);
	optMMUEC->setBackgroundColor(gui_background_color);
	optMMUEC->setForegroundColor(gui_foreground_color);
	optMMUEC->addActionListener(cpuActionListener);

	grpMMU = new gcn::Window("MMU");
	grpMMU->setPosition(grpCPU->getX(), grpCPU->getY() + grpCPU->getHeight() + DISTANCE_NEXT_Y / 2);
	grpMMU->add(optMMUNone, 10, 10);
	grpMMU->add(optMMUEnabled, 10, 40);
	grpMMU->add(optMMUEC, optMMUEnabled->getX() + optMMUEnabled->getWidth() + DISTANCE_NEXT_X, 40);
	grpMMU->setMovable(false);
	grpMMU->setSize(grpCPU->getWidth(), TITLEBAR_HEIGHT + 40 + optMMUEnabled->getHeight() + DISTANCE_NEXT_Y);
	grpMMU->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpMMU->setBaseColor(gui_base_color);
	grpMMU->setForegroundColor(gui_foreground_color);
	category.panel->add(grpMMU);

	optFPUnone = new gcn::RadioButton("None", "radiofpugroup");
	optFPUnone->setId("optFPUnone");
	optFPUnone->setBaseColor(gui_base_color);
	optFPUnone->setBackgroundColor(gui_background_color);
	optFPUnone->setForegroundColor(gui_foreground_color);
	optFPUnone->addActionListener(cpuActionListener);

	optFPU68881 = new gcn::RadioButton("68881", "radiofpugroup");
	optFPU68881->setId("optFPU68881");
	optFPU68881->setBaseColor(gui_base_color);
	optFPU68881->setBackgroundColor(gui_background_color);
	optFPU68881->setForegroundColor(gui_foreground_color);
	optFPU68881->addActionListener(cpuActionListener);

	optFPU68882 = new gcn::RadioButton("68882", "radiofpugroup");
	optFPU68882->setId("optFPU68882");
	optFPU68882->setBaseColor(gui_base_color);
	optFPU68882->setBackgroundColor(gui_background_color);
	optFPU68882->setForegroundColor(gui_foreground_color);
	optFPU68882->addActionListener(cpuActionListener);

	optFPUinternal = new gcn::RadioButton("CPU internal", "radiofpugroup");
	optFPUinternal->setId("optFPUinternal");
	optFPUinternal->setBaseColor(gui_base_color);
	optFPUinternal->setBackgroundColor(gui_background_color);
	optFPUinternal->setForegroundColor(gui_foreground_color);
	optFPUinternal->addActionListener(cpuActionListener);

	chkFPUStrict = new gcn::CheckBox("More compatible", true);
	chkFPUStrict->setId("chkFPUStrict");
	chkFPUStrict->setBaseColor(gui_base_color);
	chkFPUStrict->setBackgroundColor(gui_background_color);
	chkFPUStrict->setForegroundColor(gui_foreground_color);
	chkFPUStrict->addActionListener(cpuActionListener);

	grpFPU = new gcn::Window("FPU");
	grpFPU->setPosition(grpMMU->getX(), grpMMU->getY() + grpMMU->getHeight() + DISTANCE_NEXT_Y / 2);
	grpFPU->add(optFPUnone, 10, 10);
	grpFPU->add(optFPU68881, 10, 40);
	grpFPU->add(optFPU68882, 10, 70);
	grpFPU->add(optFPUinternal, 10, 100);
	grpFPU->add(chkFPUStrict, 10, 130);
	grpFPU->setMovable(false);
	grpFPU->setSize(grpCPU->getWidth(), TITLEBAR_HEIGHT + 130 + chkFPUStrict->getHeight() + DISTANCE_NEXT_Y);
	grpFPU->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpFPU->setBaseColor(gui_base_color);
	grpFPU->setForegroundColor(gui_foreground_color);
	category.panel->add(grpFPU);

	optCPUSpeedFastest = new gcn::RadioButton("Fastest Possible", "radiocpuspeedgroup");
	optCPUSpeedFastest->setId("optCPUSpeedFastest");
	optCPUSpeedFastest->setBaseColor(gui_base_color);
	optCPUSpeedFastest->setBackgroundColor(gui_background_color);
	optCPUSpeedFastest->setForegroundColor(gui_foreground_color);
	optCPUSpeedFastest->addActionListener(cpuActionListener);

	optCPUSpeedReal = new gcn::RadioButton("A500/A1200 or cycle exact", "radiocpuspeedgroup");
	optCPUSpeedReal->setId("optCPUSpeedReal");
	optCPUSpeedReal->setBaseColor(gui_base_color);
	optCPUSpeedReal->setBackgroundColor(gui_background_color);
	optCPUSpeedReal->setForegroundColor(gui_foreground_color);
	optCPUSpeedReal->addActionListener(cpuActionListener);

	lblCpuSpeed = new gcn::Label("CPU Speed:");
	sldCpuSpeed = new gcn::Slider(-9, 50);
	sldCpuSpeed->setSize(320, SLIDER_HEIGHT);
	sldCpuSpeed->setBaseColor(gui_base_color);
	sldCpuSpeed->setBackgroundColor(gui_background_color);
	sldCpuSpeed->setForegroundColor(gui_foreground_color);
	sldCpuSpeed->setMarkerLength(20);
	sldCpuSpeed->setStepLength(1);
	sldCpuSpeed->setId("sldCpuSpeed");
	sldCpuSpeed->addActionListener(cpuActionListener);
	lblCpuSpeedInfo = new gcn::Label("+000%");
	
	lblCpuIdle = new gcn::Label("CPU Idle:");
	sldCpuIdle = new gcn::Slider(0, 10);
	sldCpuIdle->setSize(90, SLIDER_HEIGHT);
	sldCpuIdle->setBaseColor(gui_base_color);
	sldCpuIdle->setBackgroundColor(gui_background_color);
	sldCpuIdle->setForegroundColor(gui_foreground_color);
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
	grpCPUSpeed->setBaseColor(gui_base_color);
	grpCPUSpeed->setForegroundColor(gui_foreground_color);

	category.panel->add(grpCPUSpeed);

	lblCPUFrequency = new gcn::Label("CPU Frequency:");
	cboCPUFrequency = new gcn::DropDown(&cpu_freq_list);
	cboCPUFrequency->setSize(100, cboCPUFrequency->getHeight());
	cboCPUFrequency->setBaseColor(gui_base_color);
	cboCPUFrequency->setBackgroundColor(gui_background_color);
	cboCPUFrequency->setForegroundColor(gui_foreground_color);
	cboCPUFrequency->setSelectionColor(gui_selection_color);
	cboCPUFrequency->addActionListener(cpuActionListener);
	cboCPUFrequency->setId("cboCPUFrequency");
	lblCPUFrequencyMHz = new gcn::Label("50 MHz");

	chkCPUMultiThread = new gcn::CheckBox("Multi-threaded CPU");
	chkCPUMultiThread->setId("chkCPUMultiThread");
	chkCPUMultiThread->setBaseColor(gui_base_color);
	chkCPUMultiThread->setBackgroundColor(gui_background_color);
	chkCPUMultiThread->setForegroundColor(gui_foreground_color);
	chkCPUMultiThread->addActionListener(cpuActionListener);

	grpCPUCycleExact = new gcn::Window("Cycle-Exact CPU Emulation Speed");
	grpCPUCycleExact->setPosition(grpCPUSpeed->getX(), grpCPUSpeed->getY() + grpCPUSpeed->getHeight() + DISTANCE_NEXT_Y / 2);
	grpCPUCycleExact->add(lblCPUFrequency, 10, 10);
	grpCPUCycleExact->add(cboCPUFrequency, lblCPUFrequency->getX() + lblCPUFrequency->getWidth() + DISTANCE_NEXT_X / 2, 10);
	grpCPUCycleExact->add(lblCPUFrequencyMHz, cboCPUFrequency->getX() + cboCPUFrequency->getWidth() + DISTANCE_NEXT_X, 10);
	grpCPUCycleExact->add(chkCPUMultiThread, lblCPUFrequency->getX(), lblCPUFrequency->getY() + lblCPUFrequency->getHeight() + DISTANCE_NEXT_Y * 4);
	grpCPUCycleExact->setMovable(false);
	grpCPUCycleExact->setSize(grpCPUSpeed->getWidth(), 140);
	grpCPUCycleExact->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpCPUCycleExact->setBaseColor(gui_base_color);
	grpCPUCycleExact->setForegroundColor(gui_foreground_color);

	category.panel->add(grpCPUCycleExact);

	chkPPCEnabled = new gcn::CheckBox("PPC emulation (Blizzard PPC/CyberStorm PPC)", false);
	chkPPCEnabled->setId("chkPPCEnabled");
	chkPPCEnabled->setBaseColor(gui_base_color);
	chkPPCEnabled->setBackgroundColor(gui_background_color);
	chkPPCEnabled->setForegroundColor(gui_foreground_color);
	chkPPCEnabled->addActionListener(cpuActionListener);

	lblPPCIdle = new gcn::Label("Stopped M68K CPU Idle:");
	sldPPCIdle = new gcn::Slider(0, 10);
	sldPPCIdle->setSize(90, SLIDER_HEIGHT);
	sldPPCIdle->setBaseColor(gui_base_color);
	sldPPCIdle->setBackgroundColor(gui_background_color);
	sldPPCIdle->setForegroundColor(gui_foreground_color);
	sldPPCIdle->setMarkerLength(20);
	sldPPCIdle->setStepLength(1);
	sldPPCIdle->setId("sldPPCIdle");
	sldPPCIdle->addActionListener(cpuActionListener);
	lblPPCIdleInfo = new gcn::Label("disabled");

	grpPPCOptions = new gcn::Window("PowerPC CPU Options");
	grpPPCOptions->setPosition(grpCPUCycleExact->getX(), grpCPUCycleExact->getY() + grpCPUCycleExact->getHeight() + DISTANCE_NEXT_Y / 2);
	grpPPCOptions->add(chkPPCEnabled, 10, 10);
	grpPPCOptions->add(lblPPCIdle, chkPPCEnabled->getX(), chkPPCEnabled->getY() + chkPPCEnabled->getHeight() + DISTANCE_NEXT_Y);
	grpPPCOptions->add(sldPPCIdle, lblPPCIdle->getX() + lblPPCIdle->getWidth() + DISTANCE_NEXT_X / 2, lblPPCIdle->getY());
	grpPPCOptions->add(lblPPCIdleInfo, sldPPCIdle->getX() + sldPPCIdle->getWidth() + 8, lblPPCIdle->getY());
	grpPPCOptions->setMovable(false);
	grpPPCOptions->setSize(grpCPUCycleExact->getWidth(), 120);
	grpPPCOptions->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpPPCOptions->setBaseColor(gui_base_color);
	grpPPCOptions->setForegroundColor(gui_foreground_color);

	category.panel->add(grpPPCOptions);

	lblJitCacheSize = new gcn::Label("Cache size:");
	sldJitCacheSize = new gcn::Slider(0, 5);
	sldJitCacheSize->setSize(150, SLIDER_HEIGHT);
	sldJitCacheSize->setBaseColor(gui_base_color);
	sldJitCacheSize->setBackgroundColor(gui_background_color);
	sldJitCacheSize->setForegroundColor(gui_foreground_color);
	sldJitCacheSize->setMarkerLength(20);
	sldJitCacheSize->setStepLength(1);
	sldJitCacheSize->setId("sldJitCacheSize");
	sldJitCacheSize->addActionListener(cpuActionListener);
	lblJitCacheSizeInfo = new gcn::Label("0 MB");

	chkFPUJIT = new gcn::CheckBox("FPU Support", true);
	chkFPUJIT->setId("chkFPUJIT");
	chkFPUJIT->setBaseColor(gui_base_color);
	chkFPUJIT->setBackgroundColor(gui_background_color);
	chkFPUJIT->setForegroundColor(gui_foreground_color);
	chkFPUJIT->addActionListener(cpuActionListener);
	chkConstantJump = new gcn::CheckBox("Constant jump");
	chkConstantJump->setId("chkConstantJump");
	chkConstantJump->setBaseColor(gui_base_color);
	chkConstantJump->setBackgroundColor(gui_background_color);
	chkConstantJump->setForegroundColor(gui_foreground_color);
	chkConstantJump->addActionListener(cpuActionListener);
	chkHardFlush = new gcn::CheckBox("Hard flush");
	chkHardFlush->setId("chkHardFlush");
	chkHardFlush->setBaseColor(gui_base_color);
	chkHardFlush->setBackgroundColor(gui_background_color);
	chkHardFlush->setForegroundColor(gui_foreground_color);
	chkHardFlush->addActionListener(cpuActionListener);
	optDirect = new gcn::RadioButton("Direct", "radiomemaccessgroup");
	optDirect->setId("optDirect");
	optDirect->setBaseColor(gui_base_color);
	optDirect->setBackgroundColor(gui_background_color);
	optDirect->setForegroundColor(gui_foreground_color);
	optDirect->addActionListener(cpuActionListener);
	optIndirect = new gcn::RadioButton("Indirect", "radiomemaccessgroup");
	optIndirect->setId("optIndirect");
	optIndirect->setBaseColor(gui_base_color);
	optIndirect->setBackgroundColor(gui_background_color);
	optIndirect->setForegroundColor(gui_foreground_color);
	optIndirect->addActionListener(cpuActionListener);
	chkNoFlags = new gcn::CheckBox("No flags");
	chkNoFlags->setId("chkNoFlags");
	chkNoFlags->setBaseColor(gui_base_color);
	chkNoFlags->setBackgroundColor(gui_background_color);
	chkNoFlags->setForegroundColor(gui_foreground_color);
	chkNoFlags->addActionListener(cpuActionListener);
	chkCatchExceptions = new gcn::CheckBox("Catch unexpected exceptions");
	chkCatchExceptions->setId("chkCatchExceptions");
	chkCatchExceptions->setBaseColor(gui_base_color);
	chkCatchExceptions->setBackgroundColor(gui_background_color);
	chkCatchExceptions->setForegroundColor(gui_foreground_color);
	chkCatchExceptions->addActionListener(cpuActionListener);

	grpAdvJitSettings = new gcn::Window("Advanced JIT Settings");
	grpAdvJitSettings->setPosition(grpPPCOptions->getX(), grpPPCOptions->getY() + grpPPCOptions->getHeight() + DISTANCE_NEXT_Y / 2);
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
	grpAdvJitSettings->setBaseColor(gui_base_color);
	grpAdvJitSettings->setForegroundColor(gui_foreground_color);

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
	delete chkCpuDataCache;
	delete chkJIT;
	delete grpCPU;
	delete cpuActionListener;

	delete optMMUNone;
	delete optMMUEnabled;
	delete optMMUEC;
	delete grpMMU;

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
	delete chkCPUMultiThread;

	delete grpPPCOptions;
	delete chkPPCEnabled;
	delete lblPPCIdle;
	delete sldPPCIdle;
	delete lblPPCIdleInfo;

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
		_sntprintf(txt, sizeof txt, _T("%.6f"), changed_prefs.cpu_frequency / 1000000.0);
		lblCPUFrequencyMHz->setCaption(txt);
	}
	else {
		TCHAR txt[20];
		double f = getcpufreq(changed_prefs.cpu_clock_multiplier);
		_sntprintf(txt, sizeof txt, _T("%.6f"), f / 1000000.0);
		lblCPUFrequencyMHz->setCaption(txt);
	}

	// Set Enable status
	sldCpuSpeed->setScaleEnd(changed_prefs.m68k_speed < 0 || (changed_prefs.cpu_memory_cycle_exact && !changed_prefs.cpu_cycle_exact) ? 0 : 50);
	sldCpuSpeed->setEnabled(!changed_prefs.cpu_cycle_exact);
	chk24Bit->setEnabled(changed_prefs.cpu_model <= 68030 && changed_prefs.cachesize == 0);
	sldCpuIdle->setEnabled(changed_prefs.m68k_speed != 0);
	sldPPCIdle->setEnabled(changed_prefs.ppc_mode != 0);
	lblCpuIdleInfo->setEnabled(sldCpuIdle->isEnabled());

	bool cpu_based_enable = changed_prefs.cpu_model >= 68020 && changed_prefs.address_space_24 == 0;
	bool jit_enable = cpu_based_enable && !changed_prefs.mmu_model;
#ifndef JIT
	jit_enable = false;
#endif
	bool enable = jit_enable && changed_prefs.cachesize;

	optDirect->setEnabled(enable);
	optIndirect->setEnabled(enable);
	chkHardFlush->setEnabled(enable);
	chkConstantJump->setEnabled(enable);
	chkFPUJIT->setEnabled(enable && changed_prefs.fpu_model > 0);
	chkCatchExceptions->setEnabled(enable);
	chkNoFlags->setEnabled(enable);
	lblJitCacheSizeInfo->setEnabled(enable);
	sldJitCacheSize->setEnabled(enable);
	chkJIT->setEnabled(jit_enable);
	chkCPUCompatible->setEnabled(!changed_prefs.cpu_memory_cycle_exact && !(changed_prefs.cachesize && changed_prefs.cpu_model >= 68040));
	chkFPUStrict->setEnabled(changed_prefs.fpu_model > 0);

	cboCPUFrequency->setEnabled((changed_prefs.cpu_cycle_exact || changed_prefs.cpu_compatible) && changed_prefs.m68k_speed >= 0);
	lblCPUFrequencyMHz->setEnabled(changed_prefs.cpu_cycle_exact && !changed_prefs.cpu_clock_multiplier && changed_prefs.m68k_speed >= 0);

	optFPU68881->setEnabled(changed_prefs.cpu_model < 68040 && (changed_prefs.cpu_model >= 68020 || !changed_prefs.cpu_compatible));
	optFPU68882->setEnabled(changed_prefs.cpu_model < 68040 && (changed_prefs.cpu_model >= 68020 || !changed_prefs.cpu_compatible));
	optFPUinternal->setEnabled(changed_prefs.cpu_model >= 68040);

	optMMUEC->setEnabled(changed_prefs.cpu_model >= 68030 && changed_prefs.cachesize == 0);
	optMMUEnabled->setEnabled(changed_prefs.cpu_model >= 68030 && changed_prefs.cachesize == 0);
	chkCpuDataCache->setEnabled(changed_prefs.cpu_model >= 68030 && changed_prefs.cachesize == 0 && changed_prefs.cpu_compatible);
	chkPPCEnabled->setEnabled(changed_prefs.cpu_model >= 68040 && (changed_prefs.ppc_mode == 1 || (changed_prefs.ppc_mode == 0 && !is_ppc_cpu(&changed_prefs))));
	lblPPCIdle->setEnabled(chkPPCEnabled->isEnabled());
	sldPPCIdle->setEnabled(chkPPCEnabled->isEnabled());
	lblPPCIdleInfo->setEnabled(sldPPCIdle->isEnabled());

	// Set Values
	sldCpuSpeed->setValue(static_cast<int>(changed_prefs.m68k_speed_throttle / 100));
	auto cpu_speed_info = to_string(static_cast<int>(changed_prefs.m68k_speed_throttle / 10));
	cpu_speed_info.append("%");
	lblCpuSpeedInfo->setCaption(cpu_speed_info);

	chkCPUCompatible->setSelected(changed_prefs.cpu_compatible);
	chk24Bit->setSelected(changed_prefs.address_space_24);
	chkCpuDataCache->setSelected(changed_prefs.cpu_data_cache);
	chkCpuDataCache->setEnabled(changed_prefs.cpu_model >= 68030 && changed_prefs.cachesize == 0 && changed_prefs.cpu_compatible);
	chkFPUStrict->setSelected(changed_prefs.fpu_strict);

	sldCpuIdle->setValue(changed_prefs.cpu_idle == 0 ? 0 : 12 - changed_prefs.cpu_idle / 15);
	auto cpu_idle_string = to_string((changed_prefs.cpu_idle == 0 ? 0 : 12 - changed_prefs.cpu_idle / 15) * 10);
	cpu_idle_string.append("%");
	lblCpuIdleInfo->setCaption(cpu_idle_string);

	sldPPCIdle->setValue(changed_prefs.ppc_cpu_idle);
	auto ppc_idle_string = changed_prefs.ppc_cpu_idle == 0
		? "disabled"
		: changed_prefs.ppc_cpu_idle == 10 ? "max" :
		std::to_string(changed_prefs.ppc_cpu_idle);
	lblPPCIdleInfo->setCaption(ppc_idle_string);

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
	chkFPUJIT->setSelected(changed_prefs.compfpu && changed_prefs.fpu_model > 0);
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

	bool mmu = ((changed_prefs.cpu_model == 68060 && changed_prefs.mmu_model == 68060) ||
			(changed_prefs.cpu_model == 68040 && changed_prefs.mmu_model == 68040) ||
			(changed_prefs.cpu_model == 68030 && changed_prefs.mmu_model == 68030)) &&
					changed_prefs.cachesize == 0;
	if (!mmu)
	{
		optMMUNone->setSelected(true);
	}
	else
	{
		if (changed_prefs.mmu_ec)
			optMMUEC->setSelected(true);
		else
			optMMUEnabled->setSelected(true);
	}

	bool no_thread = (changed_prefs.cpu_compatible || changed_prefs.ppc_mode || changed_prefs.cpu_memory_cycle_exact || changed_prefs.cpu_model < 68020);
	// Disabled until this is fixed upstream (WinUAE) - it doesn't work as expected yet (black screen)
	chkCPUMultiThread->setEnabled(false);
	//chkCPUMultiThread->setEnabled(!no_thread && !emulating);
	chkCPUMultiThread->setSelected(changed_prefs.cpu_thread);

	chkPPCEnabled->setSelected(changed_prefs.ppc_mode || is_ppc_cpu(&changed_prefs));
}

bool HelpPanelCPU(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("Select the required Amiga CPU (68000 - 68060). If you select 68020, you can choose");
	helptext.emplace_back("between 24-bit (68EC020) or 32-bit (68020) addressing.");
	helptext.emplace_back(" ");
	helptext.emplace_back("The option \"More compatible\" will emulate prefetch (68000) or prefetch and");
	helptext.emplace_back("instruction cache. It's more compatible but slower, but not required");
	helptext.emplace_back("for most games and demos.");
	helptext.emplace_back(" ");
	helptext.emplace_back("JIT/JIT FPU enables the Just-in-time compiler. This may break compatibility in some games.");
	helptext.emplace_back("Note: Not available in all platforms currently");
	helptext.emplace_back(" ");
	helptext.emplace_back("The available FPU models depend on the selected CPU type. The option \"More compatible\"");
	helptext.emplace_back("activates a more accurate rounding method and compare of two floats.");
	helptext.emplace_back(" ");
	helptext.emplace_back("The CPU Speed slider allows you to set the CPU speed. The fastest possible setting");
	helptext.emplace_back("will run the CPU as fast as possible. The A500/A1200 setting will run the CPU at");
	helptext.emplace_back("the speed of an A500 or A1200. The slider allows you to set the CPU speed in");
	helptext.emplace_back("percentages of the maximum speed.");
	helptext.emplace_back(" ");
	helptext.emplace_back("The CPU Idle slider allows you to set how much the CPU should sleep when idle.");
	helptext.emplace_back("This is useful to keep the system temperature down.");
	helptext.emplace_back(" ");
	helptext.emplace_back("The MMU option allows you to enable the Memory Management Unit. This is only available");
	helptext.emplace_back("for the 68030, 68040 and 68060 CPUs.");
	helptext.emplace_back(" ");
	helptext.emplace_back("The FPU option allows you to enable the FPU. This is only available for the 68020, 68030,");
	helptext.emplace_back("68040 and 68060 CPUs.");
	helptext.emplace_back(" ");
	helptext.emplace_back("The PPC emulation option allows you to enable the PowerPC emulation. This is only available");
	helptext.emplace_back("for the 68040 and 68060 CPUs and requires an extra plugin (qemu-uae) to be available.");
	helptext.emplace_back(" ");
	return true;
}
