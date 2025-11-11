#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"
#include "StringListModel.h"

#include "sysdeps.h"
#include "options.h"
#include "gui_handling.h"
#include "memory.h"
#include "autoconf.h"
#include "cpuboard.h"
#include "ethernet.h"
#include "rommgr.h"
#include "sana2.h"
#include "uae.h"
#include "registry.h"

static gcn::Window* grpExpansionBoard;
static gcn::Window* grpAcceleratorBoard;
static gcn::Window* grpMiscExpansions;

static int scsiromselect_table[300];

static gcn::StringListModel scsirom_select_cat_list;
static gcn::StringListModel scsirom_select_list;
static gcn::StringListModel scsirom_subselect_list;
static gcn::StringListModel expansionboard_itemselector_list;
static gcn::StringListModel expansionboard_selector_list;
static gcn::StringListModel scsi_romid_list;
static gcn::StringListModel scsirom_selectnum_list;
static gcn::StringListModel scsirom_file_list;

static gcn::StringListModel cpuboard_type_list;
static gcn::StringListModel cpuboard_subtype_list;
static gcn::StringListModel cpuboard_romfile_list;
static gcn::StringListModel cpuboard_rom_subselect_list;
static gcn::StringListModel acceleratorboard_itemselector_list;
static gcn::StringListModel acceleratorboard_selector_list;

static gcn::DropDown* cboScsiRomSelectCat;
static gcn::DropDown* cboScsiRomSelect;
static gcn::DropDown* cboScsiRomSubSelect;
static gcn::DropDown* cboScsiRomFile;
static gcn::DropDown* cboScsiRomId;
static gcn::DropDown* cboScsiRomSelectNum;
static gcn::Button* btnScsiRomChooser;
static gcn::CheckBox* chkScsiRomFileAutoboot;
static gcn::CheckBox* chkScsiRomSelected;
static gcn::CheckBox* chkScsiRomFilePcmcia;
static gcn::CheckBox* chkScsiRom24bitDma;

static gcn::DropDown* cboExpansionBoardItemSelector;
static gcn::DropDown* cboExpansionBoardSelector;
static gcn::CheckBox* chkExpansionBoardCheckbox;
static gcn::TextField* txtExpansionBoardStringBox;

static gcn::DropDown* cboCpuBoardType;
static gcn::DropDown* cboCpuBoardSubType;
static gcn::DropDown* cboCpuBoardRomFile;
static gcn::DropDown* cboCpuRomSubSelect;
static gcn::Button* btnCpuBoardRomChooser;

static gcn::Label* lblCpuBoardMem;
static gcn::Slider* sldCpuBoardMem;
static gcn::Label* lblCpuBoardRam;

static gcn::DropDown* cboAcceleratorBoardItemSelector;
static gcn::DropDown* cboAcceleratorBoardSelector;
static gcn::CheckBox* chkAcceleratorBoardCheckbox;

static gcn::CheckBox* chkBSDSocket;
static gcn::CheckBox* chkScsi;
static gcn::CheckBox* chkCD32Fmv;
static gcn::CheckBox* chkSana2;

int scsiromselected = 0;
static int scsiromselectednum = 0;
static int scsiromselectedcatnum = 0;

static void gui_add_string(int* table, gcn::StringListModel* item, int id, const TCHAR* str)
{
	while (*table >= 0)
		table++;
	*table++ = id;
	*table = -1;
	item->add(str);
}
static void gui_set_string_cursor(const int* table, const gcn::DropDown* item, const int id)
{
	int idx = 0;
	while (*table >= 0) {
		if (*table == id) {
			item->setSelected(idx);
			return;
		}
		idx++;
		table++;
	}
}
static int gui_get_string_cursor(const int* table, const gcn::DropDown* item)
{
	int posn = item->getSelected();
	if (posn < 0)
		return -1;
	return table[posn];
}

static void getromfile(gcn::DropDown* d, TCHAR* path, const int size)
{
	const auto val = d->getSelected();

	const auto tmp1 = d->getListModel()->getElementAt(val);
	path[0] = 0;
	const romdata* rd = getromdatabyname(tmp1.c_str());
	if (rd) {
		const romlist* rl = getromlistbyromdata(rd);
		if (rd->configname)
			_sntprintf(path, sizeof path, _T(":%s"), rd->configname);
		else if (rl)
			_tcsncpy(path, rl->path, size);
	}
}

static void enable_for_expansion2dlg()
{
	chkBSDSocket->setEnabled(!emulating);
	chkScsi->setEnabled(!emulating);
	chkCD32Fmv->setEnabled(!emulating);
	chkSana2->setEnabled(!emulating);

	cboCpuBoardRomFile->setEnabled(changed_prefs.cpuboard_type != 0);
	btnCpuBoardRomChooser->setEnabled(changed_prefs.cpuboard_type != 0);
	sldCpuBoardMem->setEnabled(changed_prefs.cpuboard_type > 0);
	lblCpuBoardRam->setEnabled(changed_prefs.cpuboard_type > 0);
	cboCpuBoardSubType->setEnabled(changed_prefs.cpuboard_type);
}

static void setcpuboardmemsize()
{
	changed_prefs.cpuboardmem1.size = std::min<uae_u32>(changed_prefs.cpuboardmem1.size,
	                                                    cpuboard_maxmemory(&changed_prefs));

	if (cpuboard_memorytype(&changed_prefs) == BOARD_MEMORY_Z2) {
		changed_prefs.fastmem[0].size = changed_prefs.cpuboardmem1.size;
	}
	if (cpuboard_memorytype(&changed_prefs) == BOARD_MEMORY_25BITMEM) {
		changed_prefs.mem25bit.size = changed_prefs.cpuboardmem1.size;
	}
	if (changed_prefs.cpuboard_type == 0) {
		changed_prefs.mem25bit.size = 0;
	}

	if (cpuboard_memorytype(&changed_prefs) == BOARD_MEMORY_HIGHMEM)
		changed_prefs.mbresmem_high.size = changed_prefs.cpuboardmem1.size;

	int maxmem = cpuboard_maxmemory(&changed_prefs);
	changed_prefs.cpuboardmem1.size = std::min<uae_u32>(changed_prefs.cpuboardmem1.size, maxmem);
	if (maxmem <= 8 * 1024 * 1024)
	{
		sldCpuBoardMem->setScaleStart(MIN_CB_MEM);
		sldCpuBoardMem->setScaleEnd(MAX_CB_MEM_Z2);
	}
	else if (maxmem <= 16 * 1024 * 1024)
	{
		sldCpuBoardMem->setScaleStart(MIN_CB_MEM);
		sldCpuBoardMem->setScaleEnd(MAX_CB_MEM_16M);
	}
	else if (maxmem <= 32 * 1024 * 1024)
	{
		sldCpuBoardMem->setScaleStart(MIN_CB_MEM);
		sldCpuBoardMem->setScaleEnd(MAX_CB_MEM_32M);
	}
	else if (maxmem <= 64 * 1024 * 1024)
	{
		sldCpuBoardMem->setScaleStart(MIN_CB_MEM);
		sldCpuBoardMem->setScaleEnd(MAX_CB_MEM_64M);
	}
	else if (maxmem <= 128 * 1024 * 1024)
	{
		sldCpuBoardMem->setScaleStart(MIN_CB_MEM);
		sldCpuBoardMem->setScaleEnd(MAX_CB_MEM_128M);
	}
	else
	{
		sldCpuBoardMem->setScaleStart(MIN_CB_MEM);
		sldCpuBoardMem->setScaleEnd(MAX_CB_MEM_256M);
	}

	int mem_size = 0;
	switch (changed_prefs.cpuboardmem1.size) {
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
	}
	sldCpuBoardMem->setValue(mem_size);
	lblCpuBoardRam->setCaption(memsize_names[msi_cpuboard[mem_size]]);
	lblCpuBoardRam->adjustSize();
	cboCpuBoardType->setSelected(changed_prefs.cpuboard_type);
	cboCpuBoardSubType->setSelected(changed_prefs.cpuboard_subtype);
}

struct expansionrom_gui
{
	const expansionboardsettings* expansionrom_gui_ebs;
	int expansionrom_gui_item;
	gcn::DropDown* expansionrom_gui_itemselector;
	gcn::DropDown* expansionrom_gui_selector;
	gcn::CheckBox* expansionrom_gui_checkbox;
	gcn::TextField* expansionrom_gui_stringbox;
	int expansionrom_gui_settingsbits;
	int expansionrom_gui_settingsshift;
	int expansionrom_gui_settings;
	TCHAR expansionrom_gui_string[ROMCONFIG_CONFIGTEXT_LEN];
};
static expansionrom_gui expansion_gui_item;
static expansionrom_gui accelerator_gui_item;

static void reset_expansionrom_gui(expansionrom_gui* eg, gcn::DropDown* itemselector, gcn::DropDown* selector, gcn::CheckBox* checkbox, gcn::TextField* stringbox)
{
	eg->expansionrom_gui_settings = 0;
	eg->expansionrom_gui_ebs = nullptr;
	eg->expansionrom_gui_string[0] = 0;
	itemselector->setVisible(false);
	selector->setVisible(false);
	checkbox->setVisible(false);
	if (stringbox != nullptr) stringbox->setVisible(false);
}

static void create_expansionrom_gui(expansionrom_gui* eg, const expansionboardsettings* ebs,
	int settings, const TCHAR* settingsstring,
	gcn::DropDown* itemselector, gcn::DropDown* selector, gcn::CheckBox* checkbox, gcn::TextField* stringbox)
{
	bool reset = false;
	static int recursive;
	const expansionboardsettings* eb;
	if (eg->expansionrom_gui_ebs != ebs) {
		if (eg->expansionrom_gui_ebs)
			eg->expansionrom_gui_item = 0;
		reset = true;
	}
	eg->expansionrom_gui_ebs = ebs;
	eg->expansionrom_gui_itemselector = itemselector;
	eg->expansionrom_gui_selector = selector;
	eg->expansionrom_gui_checkbox = checkbox;
	eg->expansionrom_gui_stringbox = stringbox;
	eg->expansionrom_gui_settings = settings;
	if (settingsstring != eg->expansionrom_gui_string) {
		eg->expansionrom_gui_string[0] = 0;
		if (settingsstring)
			_tcscpy(eg->expansionrom_gui_string, settingsstring);
	}

	if (!ebs) {
		reset_expansionrom_gui(eg, itemselector, selector, checkbox, stringbox);
		return;
	}
	if (recursive > 0)
		return;
	recursive++;

retry:
	int item = eg->expansionrom_gui_item;
	itemselector->setVisible(true);
	int bitcnt = 0;
	for (int i = 0; i < item; i++) {
		eb = &ebs[i];
		if (eb->name == nullptr) {
			eg->expansionrom_gui_item = 0;
			goto retry;
		}
		if (eb->type == EXPANSIONBOARD_STRING) {
			;
		}
		else if (eb->type == EXPANSIONBOARD_MULTI) {
			const TCHAR* p = eb->configname;
			int itemcnt = -1;
			while (p[0]) {
				itemcnt++;
				p += _tcslen(p) + 1;
			}
			int bits = 1;
			for (int i2 = 0; i < 8; i2++) {
				if ((1 << i2) >= itemcnt) {
					bits = i2;
					break;
				}
			}
			bitcnt += bits;
		}
		else {
			bitcnt++;
		}
		bitcnt += eb->bitshift;
	}
	if (reset) {
		auto list_model = itemselector->getListModel();
		list_model->clear();
		for (int i = 0; ebs[i].name; i++) {
			eb = &ebs[i];
			list_model->add(eb->name);
		}
		itemselector->setSelected(item);
	}
	eb = &ebs[item];
	bitcnt += eb->bitshift;
	if (eb->type == EXPANSIONBOARD_STRING) {
		if (stringbox != nullptr) stringbox->setVisible(true);
		selector->setVisible(false);
		checkbox->setVisible(false);
		eg->expansionrom_gui_settingsbits = 0;
		if (stringbox != nullptr) stringbox->setText(eg->expansionrom_gui_string);
	}
	else if (eb->type == EXPANSIONBOARD_MULTI) {
		auto list_model = selector->getListModel();
		list_model->clear();
		int itemcnt = -1;
		const TCHAR* p = eb->name;
		while (p[0]) {
			if (itemcnt >= 0) {
				list_model->add(p);
			}
			itemcnt++;
			p += _tcslen(p) + 1;
		}
		int bits = 1;
		for (int i = 0; i < 8; i++) {
			if ((1 << i) >= itemcnt) {
				bits = i;
				break;
			}
		}
		int value = settings;
		if (eb->invert)
			value ^= 0x7fffffff;
		value >>= bitcnt;
		value &= (1 << bits) - 1;
		selector->setSelected(value);
		if (stringbox != nullptr) stringbox->setVisible(false);
		selector->setVisible(true);
		checkbox->setVisible(false);
		eg->expansionrom_gui_settingsbits = bits;
	}
	else {
		if (stringbox != nullptr) stringbox->setVisible(false);
		selector->setVisible(false);
		checkbox->setVisible(true);
		checkbox->setSelected((settings >> bitcnt) ^ (eb->invert ? 1 : 0));
		eg->expansionrom_gui_settingsbits = 1;
	}
	eg->expansionrom_gui_settingsshift = bitcnt;
	recursive--;
}

static void get_expansionrom_gui(expansionrom_gui* eg)
{
	if (!eg->expansionrom_gui_ebs)
		return;

	int settings = eg->expansionrom_gui_settings;

	int val = eg->expansionrom_gui_itemselector->getSelected();
	if (val != -1 && val != eg->expansionrom_gui_item) {
		eg->expansionrom_gui_item = val;
		create_expansionrom_gui(eg, eg->expansionrom_gui_ebs, eg->expansionrom_gui_settings, eg->expansionrom_gui_string,
			eg->expansionrom_gui_itemselector, eg->expansionrom_gui_selector, eg->expansionrom_gui_checkbox, eg->expansionrom_gui_stringbox);
		return;
	}
	const expansionboardsettings* eb = &eg->expansionrom_gui_ebs[eg->expansionrom_gui_item];
	if (eb->type == EXPANSIONBOARD_STRING) {
		_tcscpy(eg->expansionrom_gui_string, eg->expansionrom_gui_stringbox->getText().c_str());
	}
	else if (eb->type == EXPANSIONBOARD_MULTI) {
		val = eg->expansionrom_gui_selector->getSelected();
		if (val != -1) {
			int mask = (1 << eg->expansionrom_gui_settingsbits) - 1;
			settings &= ~(mask << eg->expansionrom_gui_settingsshift);
			settings |= val << eg->expansionrom_gui_settingsshift;
			if (eb->invert)
				settings ^= mask << eg->expansionrom_gui_settingsshift;
		}
	}
	else {
		settings &= ~(1 << eg->expansionrom_gui_settingsshift);
		if (eg->expansionrom_gui_checkbox->isSelected()) {
			settings |= 1 << eg->expansionrom_gui_settingsshift;
		}
		if (eb->invert)
			settings ^= 1 << eg->expansionrom_gui_settingsshift;
	}
	eg->expansionrom_gui_settings = settings;
}

static struct netdriverdata* ndd[MAX_TOTAL_NET_DEVICES + 1];
static int net_enumerated;

struct netdriverdata** target_ethernet_enumerate()
{
	if (net_enumerated)
		return ndd;
	ethernet_enumerate(ndd, 0);
	net_enumerated = 1;
	return ndd;
}

static const int scsiromselectedmask[] = {
	EXPANSIONTYPE_INTERNAL, EXPANSIONTYPE_SCSI, EXPANSIONTYPE_IDE, EXPANSIONTYPE_SASI, EXPANSIONTYPE_CUSTOM,
	EXPANSIONTYPE_PCI_BRIDGE, EXPANSIONTYPE_X86_BRIDGE, EXPANSIONTYPE_RTG,
	EXPANSIONTYPE_SOUND, EXPANSIONTYPE_NET, EXPANSIONTYPE_FLOPPY, EXPANSIONTYPE_X86_EXPANSION
};

static void init_expansion_scsi_id()
{
	int index;
	boardromconfig* brc = get_device_rom(&changed_prefs, static_cast<int>(expansionroms[scsiromselected].romtype), scsiromselectednum, &index);
	const expansionromtype* ert = &expansionroms[scsiromselected];
	if (brc && ert && ert->id_jumper) {
		if (scsi_romid_list.getNumberOfElements() < 8) {
			scsi_romid_list.clear();
			for (int i = 0; i < 8; i++) {
				TCHAR tmp[10];
				_sntprintf(tmp, sizeof tmp, _T("%d"), i);
				scsi_romid_list.add(tmp);
			}
		}
		cboScsiRomId->setSelected(brc->roms[index].device_id);
		cboScsiRomId->setEnabled(true);
	}
	else {
		if (scsi_romid_list.getNumberOfElements() != 1) {
			scsi_romid_list.clear();
			scsi_romid_list.add(_T("-"));
		}
		cboScsiRomId->setSelected(0);
		cboScsiRomId->setEnabled(false);
	}
}

static void init_expansion2(bool init)
{
	static int first = -1;
	bool last = false;

	for (;;)
	{
		bool matched = false;
		int total = 0;		
		scsirom_select_list.clear();
		scsiromselect_table[0] = -1;
		for (int i = 0; expansionroms[i].name; i++)
		{
			total++;
		}
		int* idtab = xcalloc(int, total * 2);
		int idcnt = 0;
		for (int i = 0; expansionroms[i].name; i++) {
			if (expansionroms[i].romtype & ROMTYPE_CPUBOARD)
				continue;
			if (!(expansionroms[i].deviceflags & scsiromselectedmask[scsiromselectedcatnum]))
				continue;
			if (scsiromselectedcatnum == 0 && (expansionroms[i].deviceflags & (EXPANSIONTYPE_SASI | EXPANSIONTYPE_CUSTOM)))
				continue;
			if ((expansionroms[i].deviceflags & EXPANSIONTYPE_X86_EXPANSION) && scsiromselectedmask[scsiromselectedcatnum] != EXPANSIONTYPE_X86_EXPANSION)
				continue;
			int cnt = 0;
			for (int j = 0; j < MAX_DUPLICATE_EXPANSION_BOARDS; j++) {
				if (is_board_enabled(&changed_prefs, static_cast<int>(expansionroms[i].romtype), j)) {
					cnt++;
				}
			}
			if (i == scsiromselected)
				matched = true;
			if (cnt > 0) {
				if (first < 0)
					first = i;
			}
			idtab[idcnt++] = i;
			idtab[idcnt++] = cnt;
		}
		for (int j = 0; j < idcnt; j += 2) {
			TCHAR* nameval = nullptr;
			TCHAR* cnameval = nullptr;
			int ididx = -1;
			for (int i = 0; i < idcnt; i += 2) {
				TCHAR name[256], cname[256];
				int id = idtab[i];
				int cnt = idtab[i + 1];
				if (id < 0)
					continue;
				name[0] = 0;
				cname[0] = 0;
				if (cnt == 1)
					_tcscat(name, _T("* "));
				else if (cnt > 1)
					_sntprintf(name + _tcslen(name), sizeof name + _tcslen(name), _T("[%d] "), cnt);
				_tcscat(name, expansionroms[id].friendlyname);
				_tcscat(cname, expansionroms[id].friendlyname);
				if (expansionroms[id].friendlymanufacturer) {
					_tcscat(name, _T(" ("));
					_tcscat(name, expansionroms[id].friendlymanufacturer);
					_tcscat(name, _T(")"));
				}
				if (!cnameval || _tcsicmp(cnameval, cname) > 0) {
					xfree(nameval);
					xfree(cnameval);
					nameval = my_strdup(name);
					cnameval = my_strdup(cname);
					ididx = i;
				}
			}
			gui_add_string(scsiromselect_table, &scsirom_select_list, idtab[ididx], nameval);
			idtab[ididx] = -1;
			xfree(nameval);
			xfree(cnameval);
		}
		xfree(idtab);

		if (scsiromselected > 0 && matched)
			break;
		int found = -1;
		for (int i = 0; expansionroms[i].name; i++) {
			int romtype = static_cast<int>(expansionroms[i].romtype);
			if (romtype & ROMTYPE_CPUBOARD)
				continue;
			if (!(expansionroms[i].deviceflags & scsiromselectedmask[scsiromselectedcatnum]))
				continue;
			if (scsiromselectedcatnum == 0 && (expansionroms[i].deviceflags & (EXPANSIONTYPE_SASI | EXPANSIONTYPE_CUSTOM)))
				continue;
			if (is_board_enabled(&changed_prefs, romtype, 0)) {
				if (found == -1)
					found = i;
				else
					found = -2;
			}
		}
		if (scsiromselected < 0 && found < 0)
			found = first;
		if (found > 0) {
			scsiromselected = found;
			break;
		}
		if (last || !init)
			break;
		scsiromselectedcatnum++;
		if (scsiromselectedcatnum > 5) {
			last = true;
			scsiromselectedcatnum = 0;
			scsiromselected = 0;
		}
	}

	if (scsiromselected > 0)
		gui_set_string_cursor(scsiromselect_table, cboScsiRomSelect, scsiromselected);
	cboScsiRomSelectCat->setSelected(scsiromselectedcatnum);
	init_expansion_scsi_id();
}

static void values_to_expansion2dlg_sub()
{
	cpuboard_rom_subselect_list.clear();
	cboCpuRomSubSelect->setEnabled(false);

	scsirom_subselect_list.clear();
	const expansionromtype* er = &expansionroms[scsiromselected];
	const expansionsubromtype* srt = er->subtypes;
	int deviceflags = er->deviceflags;
	cboScsiRomSubSelect->setEnabled(srt != nullptr);
	while (srt && srt->name) {
		scsirom_subselect_list.add(srt->name);
		srt++;
	}
	int index;
	boardromconfig* brc = get_device_rom(&changed_prefs, static_cast<int>(expansionroms[scsiromselected].romtype), scsiromselectednum, &index);
	if (brc) {
		if (er->subtypes) {
			cboScsiRomSubSelect->setSelected(brc->roms[index].subtype);
			deviceflags |= er->subtypes[brc->roms[index].subtype].deviceflags;
		}
	}
	else if (srt) {
		cboScsiRomSubSelect->setSelected(0);
	}
	else
	{
		cboScsiRomId->setEnabled(false);
	}
	init_expansion_scsi_id();

	scsirom_selectnum_list.clear();
	if (deviceflags & EXPANSIONTYPE_CLOCKPORT) {
		scsirom_selectnum_list.add(_T("-"));
	}
	for (int i = 0; i < MAX_AVAILABLE_DUPLICATE_EXPANSION_BOARDS; i++) {
		TCHAR tmp[10];
		_sntprintf(tmp, sizeof tmp, _T("%d"), i + 1);
		scsirom_selectnum_list.add(tmp);
	}
	cboScsiRomSelectNum->setSelected(scsiromselectednum);
	if ((er->zorro < 2 || er->singleonly) && !(deviceflags & EXPANSIONTYPE_CLOCKPORT)) {
		scsiromselectednum = 0;
		cboScsiRomSelectNum->setSelected(0);
	}
	cboScsiRomSelectNum->setEnabled((er->zorro >= 2 && !er->singleonly) || (deviceflags & EXPANSIONTYPE_CLOCKPORT));
	chkScsiRom24bitDma->setVisible((deviceflags & EXPANSIONTYPE_DMA24) != 0);
	chkScsiRom24bitDma->setEnabled((deviceflags & EXPANSIONTYPE_DMA24) != 0);
}

static void values_from_expansion2dlg()
{
	int index;
	boardromconfig* brc;
	TCHAR tmp[MAX_DPATH];
	bool changed = false;
	bool isnew = false;

	int checked = chkScsiRomSelected->isSelected();
	getromfile(cboScsiRomFile, tmp, MAX_DPATH / sizeof(TCHAR));
	if (tmp[0] || checked) {
		const expansionromtype* ert = &expansionroms[scsiromselected];
		if (!get_device_rom(&changed_prefs, static_cast<int>(expansionroms[scsiromselected].romtype), scsiromselectednum, &index))
			isnew = true;
		brc = get_device_rom_new(&changed_prefs, static_cast<int>(expansionroms[scsiromselected].romtype), scsiromselectednum, &index);
		if (checked) {
			if (!brc->roms[index].romfile[0])
				changed = true;
			_tcscpy(brc->roms[index].romfile, _T(":ENABLED"));
		}
		else {
			changed = _tcscmp(tmp, brc->roms[index].romfile) != 0;
			getromfile(cboScsiRomFile, brc->roms[index].romfile, MAX_DPATH / sizeof(TCHAR));
		}
		brc->roms[index].autoboot_disabled = chkScsiRomFileAutoboot->isSelected();
		brc->roms[index].inserted = chkScsiRomFilePcmcia->isSelected();
		brc->roms[index].dma24bit = chkScsiRom24bitDma->isSelected();

		int v = cboScsiRomId->getSelected();
		if (!isnew)
			brc->roms[index].device_id = v;

		const expansionboardsettings* cbs = ert->settings;
		if (cbs) {
			brc->roms[index].device_settings = expansion_gui_item.expansionrom_gui_settings;
			_tcscpy(brc->roms[index].configtext, expansion_gui_item.expansionrom_gui_string);
		}

		v = cboScsiRomSubSelect->getSelected();
		brc->roms[index].subtype = v;
	}
	else {
		brc = get_device_rom(&changed_prefs, static_cast<int>(expansionroms[scsiromselected].romtype), scsiromselectednum, &index);
		if (brc && brc->roms[index].romfile[0])
			changed = true;
		clear_device_rom(&changed_prefs, static_cast<int>(expansionroms[scsiromselected].romtype), scsiromselectednum, true);
	}
	if (changed) {
		// singleonly check and removal
		if (expansionroms[scsiromselected].singleonly) {
			if (get_device_rom(&changed_prefs, static_cast<int>(expansionroms[scsiromselected].romtype), scsiromselectednum, &index)) {
				for (int i = 0; i < MAX_EXPANSION_BOARDS; i++) {
					if (i != scsiromselectednum) {
						clear_device_rom(&changed_prefs, static_cast<int>(expansionroms[scsiromselected].romtype), i, true);
					}
				}
			}
		}
		init_expansion2(false);
		values_to_expansion2dlg_sub();
	}

	changed_prefs.cpuboard_settings = accelerator_gui_item.expansionrom_gui_settings;
	getromfile(cboCpuBoardType, tmp, sizeof(brc->roms[index].romfile) / sizeof(TCHAR));
	if (tmp[0]) {
		brc = get_device_rom_new(&changed_prefs, ROMTYPE_CPUBOARD, 0, &index);
		getromfile(cboCpuBoardType, brc->roms[index].romfile, sizeof(brc->roms[index].romfile) / sizeof(TCHAR));
	}
	else {
		clear_device_rom(&changed_prefs, ROMTYPE_CPUBOARD, 0, true);
	}
}

static void values_to_expansion2_expansion_roms(UAEREG* fkey)
{
	int index;
	bool keyallocated = false;

	if (!fkey) {
		fkey = regcreatetree(nullptr, _T("DetectedROMs"));
		keyallocated = true;
	}
	if (scsiromselected) {
		const expansionromtype* ert = &expansionroms[scsiromselected];
		int romtype = static_cast<int>(ert->romtype);
		int romtype_extra = static_cast<int>(ert->romtype_extra);
		int deviceflags = ert->deviceflags;

		boardromconfig* brc = get_device_rom(&changed_prefs, romtype, scsiromselectednum, &index);
		if (brc && ert->subtypes) {
			const expansionsubromtype* esrt = &ert->subtypes[brc->roms[index].subtype];
			if (esrt->romtype) {
				romtype = static_cast<int>(esrt->romtype);
				romtype_extra = 0;
			}
			deviceflags |= esrt->deviceflags;
		}
		cboScsiRomFile->setEnabled(true);
		btnScsiRomChooser->setEnabled(true);
		chkScsiRomFileAutoboot->setVisible(!ert->autoboot_jumper);
		if (romtype & ROMTYPE_NOT) {
			btnScsiRomChooser->setVisible(false);
			cboScsiRomFile->setVisible(false);
			chkScsiRomSelected->setVisible(true);
			chkScsiRomSelected->setSelected(brc && brc->roms[index].romfile[0] != 0);
		}
		else {
			btnScsiRomChooser->setVisible(true);
			cboScsiRomFile->setVisible(true);
			chkScsiRomSelected->setVisible(false);
			chkScsiRomSelected->setSelected(false);
			addromfiles(fkey, cboScsiRomFile, brc ? brc->roms[index].romfile : nullptr, romtype, romtype_extra);
			chkScsiRomFileAutoboot->setSelected(brc && brc->roms[index].autoboot_disabled);
		}
		if (deviceflags & EXPANSIONTYPE_PCMCIA) {
			chkScsiRomFilePcmcia->setSelected(brc && brc->roms[index].inserted);
			chkScsiRomFilePcmcia->setVisible(true);
		}
		else {
			chkScsiRomFilePcmcia->setVisible(false);
			if (brc)
				brc->roms[index].inserted = false;
		}
		chkScsiRom24bitDma->setVisible((deviceflags & EXPANSIONTYPE_DMA24) != 0);
		chkScsiRom24bitDma->setEnabled((deviceflags & EXPANSIONTYPE_DMA24) != 0);
		chkScsiRom24bitDma->setSelected(brc && brc->roms[index].dma24bit);
	}
	else {
		btnScsiRomChooser->setVisible(true);
		cboScsiRomFile->setVisible(true);
		chkScsiRomSelected->setVisible(false);
		chkScsiRomFilePcmcia->setVisible(false);
		chkScsiRomFileAutoboot->setVisible(false);
		chkScsiRomSelected->setSelected(false);
		chkScsiRomFileAutoboot->setSelected(false);
		chkScsiRomFilePcmcia->setSelected(false);
		auto list_model = cboScsiRomFile->getListModel();
		list_model->clear();
		cboScsiRomFile->setEnabled(false);
		btnScsiRomChooser->setEnabled(false);
		chkScsiRom24bitDma->setEnabled(false);
		chkScsiRom24bitDma->setVisible(false);
	}
	if (keyallocated)
		regclosetree(fkey);
}

static void values_to_expansion2_expansion_settings()
{
	int index;
	if (scsiromselected) {
		const expansionromtype* ert = &expansionroms[scsiromselected];
		boardromconfig* brc = get_device_rom(&changed_prefs, static_cast<int>(expansionroms[scsiromselected].romtype),
		                                     scsiromselectednum, &index);
		if (brc) {
			if (brc->roms[index].romfile[0])
				chkScsiRomFileAutoboot->setEnabled(ert->autoboot_jumper);
		}
		else {
			chkScsiRomFileAutoboot->setEnabled(false);
			chkScsiRomFileAutoboot->setSelected(false);
		}
		cboScsiRomId->setEnabled(ert->id_jumper);
		const expansionboardsettings* cbs = ert->settings;
		create_expansionrom_gui(&expansion_gui_item, cbs,
			brc ? brc->roms[index].device_settings : 0,
			brc ? brc->roms[index].configtext : nullptr,
			cboExpansionBoardItemSelector, cboExpansionBoardSelector, chkExpansionBoardCheckbox, txtExpansionBoardStringBox);
	}
	else {
		reset_expansionrom_gui(&expansion_gui_item,
			cboExpansionBoardItemSelector, cboExpansionBoardSelector, chkExpansionBoardCheckbox, txtExpansionBoardStringBox);
	}
}

static void updatecpuboardsubtypes()
{
	cpuboard_subtype_list.clear();
	for (int i = 0; cpuboards[changed_prefs.cpuboard_type].subtypes[i].name; i++)
	{
		cpuboard_subtype_list.add(cpuboards[changed_prefs.cpuboard_type].subtypes[i].name);
	}

	const expansionboardsettings* cbs = cpuboards[changed_prefs.cpuboard_type].subtypes[changed_prefs.cpuboard_subtype].settings;
	create_expansionrom_gui(&accelerator_gui_item, cbs, changed_prefs.cpuboard_settings, nullptr,
		cboAcceleratorBoardItemSelector, cboAcceleratorBoardSelector, chkAcceleratorBoardCheckbox, nullptr);
}

static void expansion2dlgproc()
{
	// Populate Ethernet list
	if (!net_enumerated) {
		target_ethernet_enumerate();
		for (int i = 0; ndd[i]; i++) {
			struct netdriverdata* n = ndd[i];
			if (!n->active)
				continue;
			if (n->type == UAENET_SLIRP) {
				n->desc = my_strdup("SLIRP User Mode NAT");
			}
			else if (n->type == UAENET_SLIRP_INBOUND) {
				n->desc = my_strdup("SLIRP + Open ports (21-23,80)");
			}
		}
		ethernet_updateselection();
		net_enumerated = 1;
	}

	//Populate CPU boards list
	cpuboard_type_list.clear();
	for (int i = 0; cpuboards[i].name; i++)
	{
		cpuboard_type_list.add(cpuboards[i].name);
	}

	//Populate Expansion Categories
	scsirom_select_cat_list.clear();
	scsirom_select_cat_list.add("Built-in expansions");
	scsirom_select_cat_list.add("SCSI controllers");
	scsirom_select_cat_list.add("IDE controllers");
	scsirom_select_cat_list.add("SASI controllers");
	scsirom_select_cat_list.add("Custom controllers");
	scsirom_select_cat_list.add("PCI bridgeboards");
	scsirom_select_cat_list.add("x86 Bridgeboards");
	scsirom_select_cat_list.add("Graphics boards");
	scsirom_select_cat_list.add("Sound cards");
	scsirom_select_cat_list.add("Network adapters");
	scsirom_select_cat_list.add("Disk controllers");
	scsirom_select_cat_list.add("x86 bridgeboard expansions");

	reset_expansionrom_gui(&expansion_gui_item, cboExpansionBoardItemSelector, cboExpansionBoardSelector, chkExpansionBoardCheckbox, txtExpansionBoardStringBox);
	reset_expansionrom_gui(&accelerator_gui_item, cboAcceleratorBoardItemSelector, cboAcceleratorBoardSelector, chkAcceleratorBoardCheckbox, nullptr);

	chkScsiRomSelected->setVisible(false);
	init_expansion2(true);
	updatecpuboardsubtypes();
	setcpuboardmemsize();
}

static void values_to_expansion2dlg()
{
	int index;

	chkBSDSocket->setSelected(changed_prefs.socket_emu);
	chkScsi->setSelected(changed_prefs.scsi == 1);
	chkCD32Fmv->setSelected(changed_prefs.cs_cd32fmv);
	chkSana2->setSelected(changed_prefs.sana2);
	// We don't really need Catweasel support in Amiberry, let's disable it
	changed_prefs.catweasel = 0;

	UAEREG* fkey = regcreatetree(nullptr, _T("DetectedROMs"));
	load_keyring(&changed_prefs, nullptr);

	values_to_expansion2_expansion_roms(fkey);
	values_to_expansion2_expansion_settings();

	if (changed_prefs.cpuboard_type) {
		const cpuboardsubtype* cst = &cpuboards[changed_prefs.cpuboard_type].subtypes[changed_prefs.cpuboard_subtype];
		boardromconfig* brc = get_device_rom(&changed_prefs, ROMTYPE_CPUBOARD, 0, &index);
		addromfiles(fkey, cboCpuBoardRomFile, brc ? brc->roms[index].romfile : nullptr,
			static_cast<int>(cst->romtype), cst->romtype_extra);
	}
	else {
		auto list_model = cboCpuBoardRomFile->getListModel();
		list_model->clear();
	}

	regclosetree(fkey);

	gui_set_string_cursor(scsiromselect_table, cboScsiRomSelect, scsiromselected);
	values_to_expansion2dlg_sub();
}

class ExpansionsActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& action_event) override
	{
		const auto source = action_event.getSource();

		if (source == btnScsiRomChooser)
		{
			const char* filter[] = { ".rom", ".bin", "\0" };
			std::string full_path = SelectFile("Select ROM", get_rom_path(), filter);
			if (!full_path.empty())
			{
				int index;
				struct boardromconfig* brc = get_device_rom_new(&changed_prefs,
				                                                static_cast<int>(expansionroms[scsiromselected].romtype),
				                                                scsiromselectednum, &index);
				_tcscpy(brc->roms[index].romfile, full_path.c_str());
				fullpath(brc->roms[index].romfile, MAX_DPATH);
			}
			values_to_expansion2dlg();
		}
		else if (source == btnCpuBoardRomChooser)
		{
			const char* filter[] = { ".rom", ".bin", "\0" };
			std::string full_path = SelectFile("Select ROM", get_rom_path(), filter);
			if (!full_path.empty())
			{
				int index;
				struct boardromconfig* brc = get_device_rom_new(&changed_prefs, ROMTYPE_CPUBOARD, 0, &index);
				_tcscpy(brc->roms[index].romfile, full_path.c_str());
				fullpath(brc->roms[index].romfile, MAX_DPATH);
			}
			values_to_expansion2dlg();
		}
		else if (source == txtExpansionBoardStringBox
			|| source == chkExpansionBoardCheckbox
			|| source == cboExpansionBoardItemSelector
			|| source == cboExpansionBoardSelector)
		{
			get_expansionrom_gui(&expansion_gui_item);
			values_from_expansion2dlg();
		}
		else if (source == chkAcceleratorBoardCheckbox
			|| source == cboAcceleratorBoardItemSelector
			|| source == cboAcceleratorBoardSelector)
		{
			get_expansionrom_gui(&accelerator_gui_item);
			values_from_expansion2dlg();
		}
		else if (source == chkScsiRomFileAutoboot
			|| source == chkScsiRomFilePcmcia
			|| source == chkScsiRom24bitDma)
		{
			values_from_expansion2dlg();
		}
		else if (source == chkBSDSocket)
		{
			changed_prefs.socket_emu = chkBSDSocket->isSelected();
		}
		else if (source == chkScsi)
		{
			changed_prefs.scsi = chkScsi->isSelected();
			enable_for_expansion2dlg();
		}
		else if (source == chkSana2)
		{
			changed_prefs.sana2 = chkSana2->isSelected();
		}
		else if (source == chkCD32Fmv)
		{
			changed_prefs.cs_cd32fmv = chkCD32Fmv->isSelected();
			cfgfile_compatibility_romtype(&changed_prefs);
		}
		else if (source == chkScsiRomSelected
			|| source == cboScsiRomFile
			|| source == cboScsiRomId)
		{
			values_from_expansion2dlg();
			values_to_expansion2_expansion_settings();
		}
		else if (source == cboCpuBoardRomFile
			|| source == cboCpuRomSubSelect)
		{
			values_from_expansion2dlg();
			values_to_expansion2_expansion_settings();
		}
		else if (source == cboScsiRomSubSelect)
		{
			values_from_expansion2dlg();
			values_to_expansion2_expansion_roms(nullptr);
			values_to_expansion2_expansion_settings();
		}
		else if (source == cboScsiRomSelectCat)
		{
			auto val = cboScsiRomSelectCat->getSelected();
			if (val != -1 && val != scsiromselectedcatnum)
			{
				scsiromselectedcatnum = val;
				scsiromselected = 0;
				init_expansion2(false);
				values_to_expansion2_expansion_roms(nullptr);
				values_to_expansion2_expansion_settings();
				values_to_expansion2dlg_sub();
			}
		}
		else if (source == cboScsiRomSelectNum
			|| source == cboScsiRomSelect)
		{
			auto val = cboScsiRomSelectNum->getSelected();
			if (val != -1)
				scsiromselectednum = val;
			val = gui_get_string_cursor(scsiromselect_table, cboScsiRomSelect);
			if (val != -1)
			{
				scsiromselected = val;
				values_to_expansion2_expansion_roms(nullptr);
				values_to_expansion2_expansion_settings();
				values_to_expansion2dlg_sub();
			}
		}
		else if (source == cboCpuBoardType)
		{
			auto v = cboCpuBoardType->getSelected();
			if (v != -1 && v != changed_prefs.cpuboard_type)
			{
				changed_prefs.cpuboard_type = v;
				changed_prefs.cpuboard_subtype = 0;
				changed_prefs.cpuboard_settings = 0;
				updatecpuboardsubtypes();
				if (is_ppc_cpu(&changed_prefs)) {
					changed_prefs.ppc_mode = 2;
				}
				else if (changed_prefs.ppc_mode == 2) {
					changed_prefs.ppc_mode = 0;
				}
				cpuboard_set_cpu(&changed_prefs);
				setcpuboardmemsize();
				enable_for_expansion2dlg();
				values_to_expansion2dlg();
			}
		}
		else if (source == cboCpuBoardSubType)
		{
			auto v = cboCpuBoardSubType->getSelected();
			if (v != -1 && v != changed_prefs.cpuboard_subtype)
			{
				changed_prefs.cpuboard_subtype = v;
				changed_prefs.cpuboard_settings = 0;
				updatecpuboardsubtypes();
				if (is_ppc_cpu(&changed_prefs)) {
					changed_prefs.ppc_mode = 2;
				}
				else if (changed_prefs.ppc_mode == 2) {
					changed_prefs.ppc_mode = 0;
				}
				cpuboard_set_cpu(&changed_prefs);
				setcpuboardmemsize();
				enable_for_expansion2dlg();
				values_to_expansion2dlg();
			}
		}
		else if (source == sldCpuBoardMem)
		{
			changed_prefs.cpuboardmem1.size = memsizes[msi_cpuboard[static_cast<int>(sldCpuBoardMem->getValue())]];
			setcpuboardmemsize();
		}
	}
};

ExpansionsActionListener* expansions_action_listener;

void InitPanelExpansions(const config_category& category)
{
	expansions_action_listener = new ExpansionsActionListener();

	cboScsiRomSelectCat = new gcn::DropDown(&scsirom_select_cat_list);
	cboScsiRomSelectCat->setSize(250, cboScsiRomSelectCat->getHeight());
	cboScsiRomSelectCat->setBaseColor(gui_base_color);
	cboScsiRomSelectCat->setBackgroundColor(gui_background_color);
	cboScsiRomSelectCat->setForegroundColor(gui_foreground_color);
	cboScsiRomSelectCat->setSelectionColor(gui_selection_color);
	cboScsiRomSelectCat->setId("cboScsiRomSelectCat");
	cboScsiRomSelectCat->addActionListener(expansions_action_listener);
	
	cboScsiRomSelect = new gcn::DropDown(&scsirom_select_list);
	cboScsiRomSelect->setSize(250, cboScsiRomSelect->getHeight());
	cboScsiRomSelect->setBaseColor(gui_base_color);
	cboScsiRomSelect->setBackgroundColor(gui_background_color);
	cboScsiRomSelect->setForegroundColor(gui_foreground_color);
	cboScsiRomSelect->setSelectionColor(gui_selection_color);
	cboScsiRomSelect->setId("cboScsiRomSelect");
	cboScsiRomSelect->addActionListener(expansions_action_listener);

	cboScsiRomSubSelect = new gcn::DropDown(&scsirom_subselect_list);
	cboScsiRomSubSelect->setSize(250, cboScsiRomSubSelect->getHeight());
	cboScsiRomSubSelect->setBaseColor(gui_base_color);
	cboScsiRomSubSelect->setBackgroundColor(gui_background_color);
	cboScsiRomSubSelect->setForegroundColor(gui_foreground_color);
	cboScsiRomSubSelect->setSelectionColor(gui_selection_color);
	cboScsiRomSubSelect->setId("cboScsiRomSubSelect");
	cboScsiRomSubSelect->addActionListener(expansions_action_listener);

	chkScsiRomSelected = new gcn::CheckBox("Enabled");
	chkScsiRomSelected->setId("chkScsiRomSelected");
	chkScsiRomSelected->setBaseColor(gui_base_color);
	chkScsiRomSelected->setBackgroundColor(gui_background_color);
	chkScsiRomSelected->setForegroundColor(gui_foreground_color);
	chkScsiRomSelected->addActionListener(expansions_action_listener);

	cboScsiRomFile = new gcn::DropDown(&scsirom_file_list);
	cboScsiRomFile->setSize(200, cboScsiRomFile->getHeight());
	cboScsiRomFile->setBaseColor(gui_base_color);
	cboScsiRomFile->setBackgroundColor(gui_background_color);
	cboScsiRomFile->setForegroundColor(gui_foreground_color);
	cboScsiRomFile->setSelectionColor(gui_selection_color);
	cboScsiRomFile->setId("cboScsiRomFile");
	cboScsiRomFile->addActionListener(expansions_action_listener);

	cboScsiRomId = new gcn::DropDown(&scsi_romid_list);
	cboScsiRomId->setSize(30, cboScsiRomId->getHeight());
	cboScsiRomId->setBaseColor(gui_base_color);
	cboScsiRomId->setBackgroundColor(gui_background_color);
	cboScsiRomId->setForegroundColor(gui_foreground_color);
	cboScsiRomId->setSelectionColor(gui_selection_color);
	cboScsiRomId->setId("cboScsiRomId");
	cboScsiRomId->addActionListener(expansions_action_listener);

	cboScsiRomSelectNum = new gcn::DropDown(&scsirom_selectnum_list);
	cboScsiRomSelectNum->setSize(30, cboScsiRomSelectNum->getHeight());
	cboScsiRomSelectNum->setBaseColor(gui_base_color);
	cboScsiRomSelectNum->setBackgroundColor(gui_background_color);
	cboScsiRomSelectNum->setForegroundColor(gui_foreground_color);
	cboScsiRomSelectNum->setSelectionColor(gui_selection_color);
	cboScsiRomSelectNum->setId("cboScsiRomSelectNum");
	cboScsiRomSelectNum->addActionListener(expansions_action_listener);
	
	btnScsiRomChooser = new gcn::Button("...");
	btnScsiRomChooser->setBaseColor(gui_base_color);
	btnScsiRomChooser->setBackgroundColor(gui_background_color);
	btnScsiRomChooser->setForegroundColor(gui_foreground_color);
	btnScsiRomChooser->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	btnScsiRomChooser->setId("btnScsiRomChooser");
	btnScsiRomChooser->addActionListener(expansions_action_listener);

	chkScsiRomFileAutoboot = new gcn::CheckBox("Autoboot disabled");
	chkScsiRomFileAutoboot->setId("chkScsiRomFileAutoboot");
	chkScsiRomFileAutoboot->setBaseColor(gui_base_color);
	chkScsiRomFileAutoboot->setBackgroundColor(gui_background_color);
	chkScsiRomFileAutoboot->setForegroundColor(gui_foreground_color);
	chkScsiRomFileAutoboot->addActionListener(expansions_action_listener);

	chkScsiRomFilePcmcia = new gcn::CheckBox("PCMCIA inserted");
	chkScsiRomFilePcmcia->setId("chkScsiRomFilePcmcia");
	chkScsiRomFilePcmcia->setBaseColor(gui_base_color);
	chkScsiRomFilePcmcia->setBackgroundColor(gui_background_color);
	chkScsiRomFilePcmcia->setForegroundColor(gui_foreground_color);
	chkScsiRomFilePcmcia->addActionListener(expansions_action_listener);

	chkScsiRom24bitDma = new gcn::CheckBox("24-bit DMA");
	chkScsiRom24bitDma->setId("chkScsiRom24bitDma");
	chkScsiRom24bitDma->setBaseColor(gui_base_color);
	chkScsiRom24bitDma->setBackgroundColor(gui_background_color);
	chkScsiRom24bitDma->setForegroundColor(gui_foreground_color);
	chkScsiRom24bitDma->addActionListener(expansions_action_listener);

	cboExpansionBoardItemSelector = new gcn::DropDown(&expansionboard_itemselector_list);
	cboExpansionBoardItemSelector->setSize(250, cboExpansionBoardItemSelector->getHeight());
	cboExpansionBoardItemSelector->setBaseColor(gui_base_color);
	cboExpansionBoardItemSelector->setBackgroundColor(gui_background_color);
	cboExpansionBoardItemSelector->setForegroundColor(gui_foreground_color);
	cboExpansionBoardItemSelector->setSelectionColor(gui_selection_color);
	cboExpansionBoardItemSelector->setId("cboExpansionBoardItemSelector");
	cboExpansionBoardItemSelector->addActionListener(expansions_action_listener);

	cboExpansionBoardSelector = new gcn::DropDown(&expansionboard_selector_list);
	cboExpansionBoardSelector->setSize(250, cboExpansionBoardSelector->getHeight());
	cboExpansionBoardSelector->setBaseColor(gui_base_color);
	cboExpansionBoardSelector->setBackgroundColor(gui_background_color);
	cboExpansionBoardSelector->setForegroundColor(gui_foreground_color);
	cboExpansionBoardSelector->setSelectionColor(gui_selection_color);
	cboExpansionBoardSelector->setId("cboExpansionBoardSelector");
	cboExpansionBoardSelector->addActionListener(expansions_action_listener);

	chkExpansionBoardCheckbox = new gcn::CheckBox("");
	chkExpansionBoardCheckbox->setId("chkExpansionBoardCheckbox");
	chkExpansionBoardCheckbox->setBaseColor(gui_base_color);
	chkExpansionBoardCheckbox->setBackgroundColor(gui_background_color);
	chkExpansionBoardCheckbox->setForegroundColor(gui_foreground_color);
	chkExpansionBoardCheckbox->addActionListener(expansions_action_listener);

	txtExpansionBoardStringBox = new gcn::TextField();
	txtExpansionBoardStringBox->setSize(200, txtExpansionBoardStringBox->getHeight());
	txtExpansionBoardStringBox->setId("txtExpansionBoardStringBox");
	txtExpansionBoardStringBox->setBaseColor(gui_base_color);
	txtExpansionBoardStringBox->setBackgroundColor(gui_background_color);
	txtExpansionBoardStringBox->setForegroundColor(gui_foreground_color);
	
	cboCpuBoardType = new gcn::DropDown(&cpuboard_type_list);
	cboCpuBoardType->setSize(250, cboCpuBoardType->getHeight());
	cboCpuBoardType->setBaseColor(gui_base_color);
	cboCpuBoardType->setBackgroundColor(gui_background_color);
	cboCpuBoardType->setForegroundColor(gui_foreground_color);
	cboCpuBoardType->setSelectionColor(gui_selection_color);
	cboCpuBoardType->setId("cboCpuBoardType");
	cboCpuBoardType->addActionListener(expansions_action_listener);

	cboCpuBoardSubType = new gcn::DropDown(&cpuboard_subtype_list);
	cboCpuBoardSubType->setSize(250, cboCpuBoardSubType->getHeight());
	cboCpuBoardSubType->setBaseColor(gui_base_color);
	cboCpuBoardSubType->setBackgroundColor(gui_background_color);
	cboCpuBoardSubType->setForegroundColor(gui_foreground_color);
	cboCpuBoardSubType->setSelectionColor(gui_selection_color);
	cboCpuBoardSubType->setId("cboCpuBoardSubType");
	cboCpuBoardSubType->addActionListener(expansions_action_listener);

	cboCpuBoardRomFile = new gcn::DropDown(&cpuboard_romfile_list);
	cboCpuBoardRomFile->setSize(200, cboCpuBoardRomFile->getHeight());
	cboCpuBoardRomFile->setBaseColor(gui_base_color);
	cboCpuBoardRomFile->setBackgroundColor(gui_background_color);
	cboCpuBoardRomFile->setForegroundColor(gui_foreground_color);
	cboCpuBoardRomFile->setSelectionColor(gui_selection_color);
	cboCpuBoardRomFile->setId("cboCpuBoardRomFile");
	cboCpuBoardRomFile->addActionListener(expansions_action_listener);

	cboCpuRomSubSelect = new gcn::DropDown(&cpuboard_rom_subselect_list);
	cboCpuRomSubSelect->setSize(250, cboCpuRomSubSelect->getHeight());
	cboCpuRomSubSelect->setBaseColor(gui_base_color);
	cboCpuRomSubSelect->setBackgroundColor(gui_background_color);
	cboCpuRomSubSelect->setForegroundColor(gui_foreground_color);
	cboCpuRomSubSelect->setSelectionColor(gui_selection_color);
	cboCpuRomSubSelect->setId("cboCpuRomSubSelect");
	cboCpuRomSubSelect->addActionListener(expansions_action_listener);

	btnCpuBoardRomChooser = new gcn::Button("...");
	btnCpuBoardRomChooser->setBaseColor(gui_base_color);
	btnCpuBoardRomChooser->setBackgroundColor(gui_background_color);
	btnCpuBoardRomChooser->setForegroundColor(gui_foreground_color);
	btnCpuBoardRomChooser->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	btnCpuBoardRomChooser->setId("btnCpuBoardRomChooser");
	btnCpuBoardRomChooser->addActionListener(expansions_action_listener);

	lblCpuBoardMem = new gcn::Label("Memory:");
	lblCpuBoardMem->setBaseColor(gui_base_color);
	lblCpuBoardMem->setForegroundColor(gui_foreground_color);
	sldCpuBoardMem = new gcn::Slider(0, 256);
	sldCpuBoardMem->setSize(150, SLIDER_HEIGHT);
	sldCpuBoardMem->setBaseColor(gui_base_color);
	sldCpuBoardMem->setBackgroundColor(gui_background_color);
	sldCpuBoardMem->setForegroundColor(gui_foreground_color);
	sldCpuBoardMem->setMarkerLength(20);
	sldCpuBoardMem->setStepLength(1);
	sldCpuBoardMem->setId("sldCpuBoardMem");
	sldCpuBoardMem->addActionListener(expansions_action_listener);
	lblCpuBoardRam = new gcn::Label("none");
	lblCpuBoardRam->setBaseColor(gui_base_color);
	lblCpuBoardRam->setForegroundColor(gui_foreground_color);

	cboAcceleratorBoardItemSelector = new gcn::DropDown(&acceleratorboard_itemselector_list);
	cboAcceleratorBoardItemSelector->setSize(250, cboAcceleratorBoardItemSelector->getHeight());
	cboAcceleratorBoardItemSelector->setBaseColor(gui_base_color);
	cboAcceleratorBoardItemSelector->setBackgroundColor(gui_background_color);
	cboAcceleratorBoardItemSelector->setForegroundColor(gui_foreground_color);
	cboAcceleratorBoardItemSelector->setSelectionColor(gui_selection_color);
	cboAcceleratorBoardItemSelector->setId("cboAcceleratorBoardItemSelector");
	cboAcceleratorBoardItemSelector->addActionListener(expansions_action_listener);

	cboAcceleratorBoardSelector = new gcn::DropDown(&acceleratorboard_selector_list);
	cboAcceleratorBoardSelector->setSize(250, cboAcceleratorBoardItemSelector->getHeight());
	cboAcceleratorBoardSelector->setBaseColor(gui_base_color);
	cboAcceleratorBoardSelector->setBackgroundColor(gui_background_color);
	cboAcceleratorBoardSelector->setForegroundColor(gui_foreground_color);
	cboAcceleratorBoardSelector->setSelectionColor(gui_selection_color);
	cboAcceleratorBoardSelector->setId("cboAcceleratorBoardSelector");
	cboAcceleratorBoardSelector->addActionListener(expansions_action_listener);

	chkAcceleratorBoardCheckbox = new gcn::CheckBox("");
	chkAcceleratorBoardCheckbox->setId("chkAcceleratorBoardCheckbox");
	chkAcceleratorBoardCheckbox->setBaseColor(gui_base_color);
	chkAcceleratorBoardCheckbox->setBackgroundColor(gui_background_color);
	chkAcceleratorBoardCheckbox->setForegroundColor(gui_foreground_color);
	chkAcceleratorBoardCheckbox->addActionListener(expansions_action_listener);
	
	chkBSDSocket = new gcn::CheckBox("bsdsocket.library");
	chkBSDSocket->setId("chkBSDSocket");
	chkBSDSocket->setBaseColor(gui_base_color);
	chkBSDSocket->setBackgroundColor(gui_background_color);
	chkBSDSocket->setForegroundColor(gui_foreground_color);
	chkBSDSocket->addActionListener(expansions_action_listener);

	chkScsi = new gcn::CheckBox("uaescsi.device");
	chkScsi->setId("chkSCSI");
	chkScsi->setBaseColor(gui_base_color);
	chkScsi->setBackgroundColor(gui_background_color);
	chkScsi->setForegroundColor(gui_foreground_color);
	chkScsi->addActionListener(expansions_action_listener);

	chkCD32Fmv = new gcn::CheckBox("CD32 Full Motion Video cartridge");
	chkCD32Fmv->setId("chkCD32Fmv");
	chkCD32Fmv->setBaseColor(gui_base_color);
	chkCD32Fmv->setBackgroundColor(gui_background_color);
	chkCD32Fmv->setForegroundColor(gui_foreground_color);
	chkCD32Fmv->addActionListener(expansions_action_listener);

	chkSana2 = new gcn::CheckBox("uaenet.device");
	chkSana2->setId("chkSana2");
	chkSana2->setBaseColor(gui_base_color);
	chkSana2->setBackgroundColor(gui_background_color);
	chkSana2->setForegroundColor(gui_foreground_color);
	chkSana2->addActionListener(expansions_action_listener);
	chkSana2->setEnabled(true);
	
	int posY = DISTANCE_BORDER;
	int posX = DISTANCE_BORDER;
	
	grpExpansionBoard = new gcn::Window("Expansion Board Settings");
	grpExpansionBoard->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	grpExpansionBoard->add(cboScsiRomSelectCat, posX, posY);
	grpExpansionBoard->add(cboScsiRomId, cboScsiRomSelectCat->getX() + cboScsiRomSelectCat->getWidth() + DISTANCE_NEXT_X * 15, cboScsiRomSelectCat->getY());
	grpExpansionBoard->add(cboScsiRomSelect, posX, posY + cboScsiRomSelectCat->getHeight() + DISTANCE_NEXT_Y);
	grpExpansionBoard->add(cboScsiRomSelectNum, cboScsiRomSelect->getX() + cboScsiRomSelect->getWidth() + DISTANCE_NEXT_X, cboScsiRomSelect->getY());
	grpExpansionBoard->add(chkScsiRom24bitDma, cboScsiRomSelectNum->getX() + cboScsiRomSelectNum->getWidth() + DISTANCE_NEXT_X, cboScsiRomSelectCat->getY());
	grpExpansionBoard->add(chkScsiRomSelected, cboScsiRomSelectNum->getX() + cboScsiRomSelectNum->getWidth() + DISTANCE_NEXT_X, cboScsiRomSelectNum->getY());
	grpExpansionBoard->add(cboScsiRomFile, chkScsiRomSelected->getX(), chkScsiRomSelected->getY());
	grpExpansionBoard->add(btnScsiRomChooser, cboScsiRomFile->getX() + cboScsiRomFile->getWidth() + DISTANCE_NEXT_X, cboScsiRomFile->getY());
	grpExpansionBoard->add(cboScsiRomSubSelect, posX, cboScsiRomSelect->getY() + cboScsiRomSelect->getHeight() + DISTANCE_NEXT_Y);
	grpExpansionBoard->add(chkScsiRomFileAutoboot, cboScsiRomSelectNum->getX(), cboScsiRomSelectNum->getY() + cboScsiRomSelectNum->getHeight() + DISTANCE_NEXT_Y);
	grpExpansionBoard->add(chkScsiRomFilePcmcia, chkScsiRomFileAutoboot->getX() + chkScsiRomFileAutoboot->getWidth() + 2, chkScsiRomFileAutoboot->getY());
	grpExpansionBoard->add(txtExpansionBoardStringBox, chkScsiRomFileAutoboot->getX(), chkScsiRomFileAutoboot->getY() + chkScsiRomFileAutoboot->getHeight() + DISTANCE_NEXT_Y);
	grpExpansionBoard->add(chkExpansionBoardCheckbox, chkScsiRomFileAutoboot->getX(), chkScsiRomFileAutoboot->getY() + chkScsiRomFileAutoboot->getHeight() + DISTANCE_NEXT_Y);
	grpExpansionBoard->add(cboExpansionBoardItemSelector, posX, cboScsiRomSubSelect->getY() + cboScsiRomSubSelect->getHeight() + DISTANCE_NEXT_Y);
	grpExpansionBoard->add(cboExpansionBoardSelector, chkScsiRomSelected->getX(), cboExpansionBoardItemSelector->getY());
	grpExpansionBoard->setMovable(false);
	grpExpansionBoard->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2, 250);
	grpExpansionBoard->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpExpansionBoard->setBaseColor(gui_base_color);
	category.panel->add(grpExpansionBoard);

	grpAcceleratorBoard = new gcn::Window("Accelerator Board Settings");
	grpAcceleratorBoard->setPosition(DISTANCE_BORDER, grpExpansionBoard->getY() + grpExpansionBoard->getHeight() + DISTANCE_NEXT_Y);
	grpAcceleratorBoard->add(cboCpuBoardType, posX, posY);
	grpAcceleratorBoard->add(cboCpuBoardSubType, posX, posY + cboCpuBoardType->getHeight() + DISTANCE_NEXT_Y);
	grpAcceleratorBoard->add(cboCpuBoardRomFile, cboCpuBoardType->getX() + cboCpuBoardType->getWidth() + DISTANCE_NEXT_X * 3, cboCpuBoardType->getY());
	grpAcceleratorBoard->add(btnCpuBoardRomChooser, cboCpuBoardRomFile->getX() + cboCpuBoardRomFile->getWidth() + DISTANCE_NEXT_X, cboCpuBoardRomFile->getY());
	grpAcceleratorBoard->add(lblCpuBoardMem, cboCpuBoardType->getX() + cboCpuBoardType->getWidth() + DISTANCE_NEXT_X, cboCpuBoardSubType->getY());
	grpAcceleratorBoard->add(sldCpuBoardMem, lblCpuBoardMem->getX() + lblCpuBoardMem->getWidth() + DISTANCE_NEXT_X, lblCpuBoardMem->getY());
	grpAcceleratorBoard->add(lblCpuBoardRam, sldCpuBoardMem->getX() + sldCpuBoardMem->getWidth() + DISTANCE_NEXT_X, lblCpuBoardMem->getY());

	//grpAcceleratorBoard->add(cboCpuRomSubSelect, posX, cboCpuBoardSubType->getY() + cboCpuBoardSubType->getHeight() + DISTANCE_NEXT_Y);
	//grpAcceleratorBoard->add(cboAcceleratorBoardSelector, posX, cboAcceleratorBoardItemSelector->getY() + cboAcceleratorBoardItemSelector->getHeight() + DISTANCE_NEXT_Y);
	grpAcceleratorBoard->add(cboAcceleratorBoardItemSelector, posX, cboCpuBoardSubType->getY() + cboCpuBoardSubType->getHeight() + DISTANCE_NEXT_Y * 2);
	grpAcceleratorBoard->add(chkAcceleratorBoardCheckbox, cboAcceleratorBoardItemSelector->getX() + cboAcceleratorBoardItemSelector->getWidth() + DISTANCE_NEXT_X, cboAcceleratorBoardItemSelector->getY());
	
	grpAcceleratorBoard->setMovable(false);
	grpAcceleratorBoard->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2, 200);
	grpAcceleratorBoard->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpAcceleratorBoard->setBaseColor(gui_base_color);
	grpAcceleratorBoard->setForegroundColor(gui_foreground_color);
	category.panel->add(grpAcceleratorBoard);
	
	grpMiscExpansions = new gcn::Window("Miscellaneous Expansions");
	grpMiscExpansions->setPosition(DISTANCE_BORDER, grpAcceleratorBoard->getY() + grpAcceleratorBoard->getHeight() + DISTANCE_NEXT_Y);
	grpMiscExpansions->add(chkBSDSocket, DISTANCE_BORDER, posY);
	posX += chkScsi->getX() + chkScsi->getWidth() + DISTANCE_NEXT_X * 5;
	grpMiscExpansions->add(chkSana2, posX, posY);
	posY += chkBSDSocket->getHeight() + DISTANCE_NEXT_Y;
	grpMiscExpansions->add(chkScsi, DISTANCE_BORDER, posY);
	grpMiscExpansions->add(chkCD32Fmv, posX, posY);
	grpMiscExpansions->setMovable(false);
	grpMiscExpansions->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2, TITLEBAR_HEIGHT + posY + chkCD32Fmv->getHeight() + DISTANCE_NEXT_Y);
	grpMiscExpansions->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpMiscExpansions->setBaseColor(gui_base_color);
	grpMiscExpansions->setForegroundColor(gui_foreground_color);
	category.panel->add(grpMiscExpansions);

	expansion2dlgproc();

	RefreshPanelExpansions();
}

void ExitPanelExpansions()
{
	delete cboScsiRomSelectCat;
	delete cboScsiRomSelect;
	delete cboScsiRomSubSelect;
	delete cboScsiRomFile;
	delete btnScsiRomChooser;
	delete chkScsiRomFileAutoboot;
	delete chkScsiRomFilePcmcia;
	delete chkScsiRom24bitDma;
	delete cboScsiRomId;
	delete cboScsiRomSelectNum;
	delete chkScsiRomSelected;

	delete cboExpansionBoardItemSelector;
	delete cboExpansionBoardSelector;
	delete chkExpansionBoardCheckbox;
	delete txtExpansionBoardStringBox;
	
	delete cboCpuBoardType;
	delete cboCpuBoardSubType;
	delete cboCpuBoardRomFile;
	delete cboCpuRomSubSelect;
	delete btnCpuBoardRomChooser;

	delete lblCpuBoardMem;
	delete sldCpuBoardMem;
	delete lblCpuBoardRam;

	delete cboAcceleratorBoardItemSelector;
	delete cboAcceleratorBoardSelector;
	delete chkAcceleratorBoardCheckbox;
	
	delete chkBSDSocket;
	delete chkScsi;
	delete chkCD32Fmv;
	delete chkSana2;
	
	delete grpExpansionBoard;
	delete grpAcceleratorBoard;
	delete grpMiscExpansions;
	
	delete expansions_action_listener;
}

void RefreshPanelExpansions()
{
	scsiromselectedcatnum = 0;
	scsiromselected = 0;
	values_to_expansion2dlg();
	enable_for_expansion2dlg();
}

bool HelpPanelExpansions(std::vector<std::string>& helptext)
{
    helptext.clear();
    helptext.emplace_back("Some of these expansions will be set automatically for you, depending on the model of");
    helptext.emplace_back("Amiga you chose to emulate (ie; the internal IDE for A1200 models).");
    helptext.emplace_back(" ");
    helptext.emplace_back("Here you can enable a few expansion options that are commonly used:");
    helptext.emplace_back(" ");
    helptext.emplace_back("\"bsdsocket.library\" enables network functions (ie; for web browsers in OS3.9).");
    helptext.emplace_back("When this option is enabled, you do not need to use a native Amiga TCP stack (ie;");
    helptext.emplace_back("AmiTCP/Genesis/Roadshow), as the Amiga applications you run will see your emulated");
    helptext.emplace_back("Amiga as being connected to the network Amiberry is running on.");
    helptext.emplace_back(" ");
    helptext.emplace_back("\"uaescsi.device\" enables the uaescsi.device, which can be used to connect to hard");
    helptext.emplace_back("drives inside the emulation. This is required for CDFS automounting to work.");
    helptext.emplace_back(" ");
    helptext.emplace_back("\"CD32 Full Motion Video cartridge\" enables the FMV module for the CD32, which is");
    helptext.emplace_back("only useful if you're emulating a CD32 of course.");
    helptext.emplace_back(" ");
    helptext.emplace_back("\"uaenet.device\" is currently disabled. ");
    helptext.emplace_back(" ");
    return true;
}