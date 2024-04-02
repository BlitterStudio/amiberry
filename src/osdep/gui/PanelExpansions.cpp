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
#include "rommgr.h"

static gcn::Window* grpExpansionBoard;
static gcn::Window* grpAcceleratorBoard;
static gcn::Window* grpMiscExpansions;

static int scsiromselect_table[300];

static gcn::StringListModel scsirom_select_cat_list;
static gcn::StringListModel scsirom_select_list;
static gcn::StringListModel scsirom_subselect_list;
static gcn::StringListModel expansionboard_itemselector_list;
static gcn::StringListModel scsi_romid_list;
static gcn::StringListModel scsirom_selectnum_list;
static gcn::StringListModel scsirom_file_list;

static gcn::StringListModel cpuboards_list;
static gcn::StringListModel cpuboards_subtype_list;
static gcn::StringListModel cpuboard_romfile_list;
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

static gcn::DropDown* cboExpansionBoardItemSelector;
static gcn::DropDown* cboExpansionBoardSelector;
static gcn::CheckBox* chkExpansionBoardCheckbox;
static gcn::TextBox* txtExpansionBoardStringBox;

static gcn::DropDown* cboCpuBoardType;
static gcn::DropDown* cboCpuBoardSubType;
static gcn::DropDown* cboCpuBoardRomFile;

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

static const int scsiromselectedmask[] = {
	EXPANSIONTYPE_INTERNAL, EXPANSIONTYPE_SCSI, EXPANSIONTYPE_IDE, EXPANSIONTYPE_SASI, EXPANSIONTYPE_CUSTOM,
	EXPANSIONTYPE_PCI_BRIDGE, EXPANSIONTYPE_X86_BRIDGE, EXPANSIONTYPE_RTG,
	EXPANSIONTYPE_SOUND, EXPANSIONTYPE_NET, EXPANSIONTYPE_FLOPPY, EXPANSIONTYPE_X86_EXPANSION
};

struct expansionrom_gui
{
	const expansionboardsettings* expansionrom_gui_ebs;
	int expansionrom_gui_item;
	gcn::DropDown* expansionrom_gui_itemselector;
	gcn::DropDown* expansionrom_gui_selector;
	gcn::CheckBox* expansionrom_gui_checkbox;
	gcn::TextBox* expansionrom_gui_stringbox;
	int expansionrom_gui_settingsbits;
	int expansionrom_gui_settingsshift;
	int expansionrom_gui_settings;
	TCHAR expansionrom_gui_string[ROMCONFIG_CONFIGTEXT_LEN];
};
static expansionrom_gui expansion_gui_item;
static expansionrom_gui accelerator_gui_item;

static void gui_add_string(int *table, gcn::StringListModel *item, int id, const TCHAR *str)
{
	while (*table >= 0)
		table++;
	*table++ = id;
	*table = -1;
	item->add(str); //SendDlgItemMessage(hDlg, item, CB_ADDSTRING, 0, (LPARAM)str);
}
static void gui_set_string_cursor(int* table, gcn::DropDown* item, int id)
{
	int idx = 0;
	while (*table >= 0) {
		if (*table == id) {
			item->setSelected(idx); //SendDlgItemMessage(item, CB_SETCURSEL, idx, 0);
			return;
		}
		idx++;
		table++;
	}
}
static int gui_get_string_cursor(int* table, gcn::DropDown* item)
{
	int posn = item->getSelected(); //SendDlgItemMessage(item, CB_GETCURSEL, 0, 0);
	if (posn < 0)
		return -1;
	return table[posn];
}

static void reset_expansionrom_gui(expansionrom_gui* eg, gcn::DropDown* itemselector, gcn::DropDown* selector, gcn::CheckBox* checkbox, gcn::TextBox* stringbox)
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
	gcn::DropDown* itemselector, gcn::DropDown* selector, gcn::CheckBox* checkbox, gcn::TextBox* stringbox)
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

	int val;
	int settings = eg->expansionrom_gui_settings;

	val = eg->expansionrom_gui_itemselector->getSelected();
	if (val != eg->expansionrom_gui_item) {
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

static void setcpuboardmemsize()
{
	//if (changed_prefs.cpuboardmem1.size > cpuboard_maxmemory(&changed_prefs))
	//	changed_prefs.cpuboardmem1.size = cpuboard_maxmemory(&changed_prefs);

	//if (cpuboard_memorytype(&changed_prefs) == BOARD_MEMORY_Z2) {
	//	changed_prefs.fastmem[0].size = changed_prefs.cpuboardmem1.size;
	//}
	//if (cpuboard_memorytype(&changed_prefs) == BOARD_MEMORY_25BITMEM) {
	//	changed_prefs.mem25bit.size = changed_prefs.cpuboardmem1.size;
	//}
	//if (changed_prefs.cpuboard_type == 0) {
	//	changed_prefs.mem25bit.size = 0;
	//}

	//if (cpuboard_memorytype(&changed_prefs) == BOARD_MEMORY_HIGHMEM)
	//	changed_prefs.mbresmem_high.size = changed_prefs.cpuboardmem1.size;

	//int maxmem = cpuboard_maxmemory(&changed_prefs);
	//if (changed_prefs.cpuboardmem1.size > maxmem) {
	//	changed_prefs.cpuboardmem1.size = maxmem;
	//}
	//if (maxmem <= 8 * 1024 * 1024)
	//	SendDlgItemMessage(IDC_CPUBOARDMEM, TBM_SETRANGE, TRUE, MAKELONG(MIN_CB_MEM, MAX_CB_MEM_Z2));
	//else if (maxmem <= 16 * 1024 * 1024)
	//	SendDlgItemMessage(IDC_CPUBOARDMEM, TBM_SETRANGE, TRUE, MAKELONG(MIN_CB_MEM, MAX_CB_MEM_16M));
	//else if (maxmem <= 32 * 1024 * 1024)
	//	SendDlgItemMessage(IDC_CPUBOARDMEM, TBM_SETRANGE, TRUE, MAKELONG(MIN_CB_MEM, MAX_CB_MEM_32M));
	//else if (maxmem <= 64 * 1024 * 1024)
	//	SendDlgItemMessage(IDC_CPUBOARDMEM, TBM_SETRANGE, TRUE, MAKELONG(MIN_CB_MEM, MAX_CB_MEM_64M));
	//else if (maxmem <= 128 * 1024 * 1024)
	//	SendDlgItemMessage(IDC_CPUBOARDMEM, TBM_SETRANGE, TRUE, MAKELONG(MIN_CB_MEM, MAX_CB_MEM_128M));
	//else
	//	SendDlgItemMessage(IDC_CPUBOARDMEM, TBM_SETRANGE, TRUE, MAKELONG(MIN_CB_MEM, MAX_CB_MEM_256M));

	//int mem_size = 0;
	//switch (changed_prefs.cpuboardmem1.size) {
	//case 0x00000000: mem_size = 0; break;
	//case 0x00100000: mem_size = 1; break;
	//case 0x00200000: mem_size = 2; break;
	//case 0x00400000: mem_size = 3; break;
	//case 0x00800000: mem_size = 4; break;
	//case 0x01000000: mem_size = 5; break;
	//case 0x02000000: mem_size = 6; break;
	//case 0x04000000: mem_size = 7; break;
	//case 0x08000000: mem_size = 8; break;
	//case 0x10000000: mem_size = 9; break;
	//}
	//SendDlgItemMessage(hDlg, IDC_CPUBOARDMEM, TBM_SETPOS, TRUE, mem_size);
	//SetDlgItemText(hDlg, IDC_CPUBOARDRAM, memsize_names[msi_cpuboard[mem_size]]);
	//SendDlgItemMessage(hDlg, IDC_CPUBOARD_TYPE, CB_SETCURSEL, changed_prefs.cpuboard_type, 0);
	//SendDlgItemMessage(hDlg, IDC_CPUBOARD_SUBTYPE, CB_SETCURSEL, changed_prefs.cpuboard_subtype, 0);
}

static void init_expansion2(bool init)
{
	static int first = -1;
	bool last = false;

	for (;;)
	{
		bool matched = false;
		int* idtab;
		int total = 0;		
		scsirom_select_list.clear(); //SendDlgItemMessage(hDlg, IDC_SCSIROMSELECT, CB_RESETCONTENT, 0, 0);
		scsiromselect_table[0] = -1;
		for (int i = 0; expansionroms[i].name; i++)
		{
			total++;
		}
		idtab = xcalloc(int, total * 2);
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
				if (is_board_enabled(&changed_prefs, int(expansionroms[i].romtype), j)) {
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
					_stprintf(name + _tcslen(name), _T("[%d] "), cnt);
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
			if (ididx > -1) {
				gui_add_string(scsiromselect_table, &scsirom_select_list, idtab[ididx], nameval);
				idtab[ididx] = -1;
			}
			xfree(nameval);
			xfree(cnameval);
		}
		xfree(idtab);

		if (scsiromselected > 0 && matched)
			break;
		int found = -1;
		for (int i = 0; expansionroms[i].name; i++) {
			int romtype = int(expansionroms[i].romtype);
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
	cboScsiRomSelectCat->setSelected(scsiromselectedcatnum); //SendDlgItemMessage(hDlg, IDC_SCSIROMSELECTCAT, CB_SETCURSEL, scsiromselectedcatnum, 0);

	scsi_romid_list.clear(); //SendDlgItemMessage(hDlg, IDC_SCSIROMID, CB_RESETCONTENT, 0, 0);
	int index;
	boardromconfig* brc = get_device_rom(&changed_prefs, int(expansionroms[scsiromselected].romtype), scsiromselectednum, &index);
	const expansionromtype* ert = &expansionroms[scsiromselected];
	if (brc && ert && ert->id_jumper) {
		for (int i = 0; i < 8; i++) {
			TCHAR tmp[10];
			_stprintf(tmp, _T("%d"), i);
			scsi_romid_list.add(tmp); //SendDlgItemMessage(hDlg, IDC_SCSIROMID, CB_ADDSTRING, 0, (LPARAM)tmp);
		}
	}
	else {
		scsi_romid_list.add(_T("-")); //SendDlgItemMessage(hDlg, IDC_SCSIROMID, CB_ADDSTRING, 0, (LPARAM)_T("-"));
		cboScsiRomId->setSelected(0); //SendDlgItemMessage(hDlg, IDC_SCSIROMID, CB_SETCURSEL, 0, 0);
		cboScsiRomId->setEnabled(false); //ew(hDlg, IDC_SCSIROMID, 0);
	}
}

static void updatecpuboardsubtypes()
{
	cpuboards_subtype_list.clear();
	for (int i = 0; cpuboards[changed_prefs.cpuboard_type].subtypes[i].name; i++)
	{
		cpuboards_subtype_list.add(cpuboards[changed_prefs.cpuboard_type].subtypes[i].name);
	}

	const expansionboardsettings* cbs = cpuboards[changed_prefs.cpuboard_type].subtypes[changed_prefs.cpuboard_subtype].settings;
	create_expansionrom_gui(&accelerator_gui_item, cbs, changed_prefs.cpuboard_settings, nullptr,
		cboAcceleratorBoardItemSelector, cboAcceleratorBoardSelector, chkAcceleratorBoardCheckbox, nullptr);
}

static void addromfiles(gcn::DropDown* d, const TCHAR* path, int type1, int type2)
{
	//TODO when we implement ROM files
}

static void values_to_expansion2_expansion_roms()
{
	int index;
	boardromconfig* brc;

	if (scsiromselected) {
		const expansionromtype* ert = &expansionroms[scsiromselected];
		int romtype = int(ert->romtype);
		int romtype_extra = int(ert->romtype_extra);
		int deviceflags = ert->deviceflags;

		brc = get_device_rom(&changed_prefs, romtype, scsiromselectednum, &index);
		if (brc && ert->subtypes) {
			const expansionsubromtype* esrt = &ert->subtypes[brc->roms[index].subtype];
			if (esrt->romtype) {
				romtype = int(esrt->romtype);
				romtype_extra = 0;
			}
			deviceflags |= esrt->deviceflags;
		}
		cboScsiRomFile->setEnabled(true); //ew(hDlg, IDC_SCSIROMFILE, true);
		btnScsiRomChooser->setEnabled(true); //ew(hDlg, IDC_SCSIROMCHOOSER, true);
		chkScsiRomFileAutoboot->setVisible(true); //hide(hDlg, IDC_SCSIROMFILEAUTOBOOT, 0);
		if (romtype & ROMTYPE_NOT) {
			btnScsiRomChooser->setVisible(false); //hide(hDlg, IDC_SCSIROMCHOOSER, 1);
			cboScsiRomFile->setVisible(false); //hide(hDlg, IDC_SCSIROMFILE, 1);
			chkScsiRomSelected->setVisible(true); //hide(hDlg, IDC_SCSIROMSELECTED, 0);
			chkScsiRomSelected->setSelected(brc && brc->roms[index].romfile[0] != 0); //setchecked(hDlg, IDC_SCSIROMSELECTED, brc && brc->roms[index].romfile[0] != 0);
		}
		else {
			btnScsiRomChooser->setVisible(true); //hide(hDlg, IDC_SCSIROMCHOOSER, 0);
			cboScsiRomFile->setVisible(false); //hide(hDlg, IDC_SCSIROMFILE, 0);
			chkScsiRomSelected->setVisible(false); //hide(hDlg, IDC_SCSIROMSELECTED, 1);
			chkScsiRomSelected->setSelected(false); //setchecked(hDlg, IDC_SCSIROMSELECTED, false);
			addromfiles(cboScsiRomFile, brc ? brc->roms[index].romfile : nullptr, romtype, romtype_extra);
			chkScsiRomFileAutoboot->setSelected(brc && brc->roms[index].autoboot_disabled); //setchecked(hDlg, IDC_SCSIROMFILEAUTOBOOT, brc && brc->roms[index].autoboot_disabled);
		}
		if (deviceflags & EXPANSIONTYPE_PCMCIA) {
			chkScsiRomFilePcmcia->setSelected(brc && brc->roms[index].inserted); //setchecked(hDlg, IDC_SCSIROMFILEPCMCIA, brc && brc->roms[index].inserted);
			chkScsiRomFilePcmcia->setVisible(true); //hide(hDlg, IDC_SCSIROMFILEPCMCIA, 0);
		}
		else {
			chkScsiRomFilePcmcia->setVisible(false); //hide(hDlg, IDC_SCSIROMFILEPCMCIA, 1);
			if (brc)
				brc->roms[index].inserted = false;
		}
		//hide(hDlg, IDC_SCSIROM24BITDMA, (deviceflags & EXPANSIONTYPE_DMA24) == 0);
		//ew(hDlg, IDC_SCSIROM24BITDMA, (deviceflags & EXPANSIONTYPE_DMA24) != 0);
		//setchecked(hDlg, IDC_SCSIROM24BITDMA, brc && brc->roms[index].dma24bit);
	}
	else {
		btnScsiRomChooser->setVisible(true); //hide(hDlg, IDC_SCSIROMCHOOSER, 0);
		cboScsiRomFile->setVisible(true); //hide(hDlg, IDC_SCSIROMFILE, 0);
		chkScsiRomSelected->setVisible(false); //hide(hDlg, IDC_SCSIROMSELECTED, 1);
		chkScsiRomFilePcmcia->setVisible(false); //hide(hDlg, IDC_SCSIROMFILEPCMCIA, 1);
		chkScsiRomFileAutoboot->setVisible(false); //hide(hDlg, IDC_SCSIROMFILEAUTOBOOT, 1);
		chkScsiRomSelected->setSelected(false); //setchecked(hDlg, IDC_SCSIROMSELECTED, false);
		chkScsiRomFileAutoboot->setSelected(false); //setchecked(hDlg, IDC_SCSIROMFILEAUTOBOOT, false);
		chkScsiRomFilePcmcia->setSelected(false); //setchecked(hDlg, IDC_SCSIROMFILEPCMCIA, false);
		auto list_model = cboScsiRomFile->getListModel();
		list_model->clear(); //SendDlgItemMessage(hDlg, IDC_SCSIROMFILE, CB_RESETCONTENT, 0, 0);
		cboScsiRomFile->setEnabled(false); //ew(hDlg, IDC_SCSIROMFILE, false);
		btnScsiRomChooser->setEnabled(false); //ew(hDlg, IDC_SCSIROMCHOOSER, false);
		//ew(hDlg, IDC_SCSIROM24BITDMA, 0);
		//hide(hDlg, IDC_SCSIROM24BITDMA, 1);
	}
}

static void values_to_expansion2_expansion_settings()
{
	int index;
	boardromconfig* brc;
	if (scsiromselected) {
		const expansionromtype* ert = &expansionroms[scsiromselected];
		brc = get_device_rom(&changed_prefs, int(expansionroms[scsiromselected].romtype), scsiromselectednum, &index);
		if (brc) {
			if (brc->roms[index].romfile[0])
				chkScsiRomFileAutoboot->setEnabled(ert->autoboot_jumper); //ew(hDlg, IDC_SCSIROMFILEAUTOBOOT, ert->autoboot_jumper);
		}
		else {
			chkScsiRomFileAutoboot->setEnabled(false); //ew(hDlg, IDC_SCSIROMFILEAUTOBOOT, FALSE);
			chkScsiRomFileAutoboot->setSelected(false); //setchecked(hDlg, IDC_SCSIROMFILEAUTOBOOT, false);
		}
		cboScsiRomId->setEnabled(ert->id_jumper); //ew(hDlg, IDC_SCSIROMID, ert->id_jumper);
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

static void values_to_expansion2dlg_sub()
{
	cpuboards_subtype_list.clear(); //SendDlgItemMessage(hDlg, IDC_CPUBOARDROMSUBSELECT, CB_RESETCONTENT, 0, 0);
	cboCpuBoardSubType->setEnabled(false); //ew(hDlg, IDC_CPUBOARDROMSUBSELECT, false);

	scsirom_subselect_list.clear(); //SendDlgItemMessage(hDlg, IDC_SCSIROMSUBSELECT, CB_RESETCONTENT, 0, 0);
	const expansionromtype* er = &expansionroms[scsiromselected];
	const expansionsubromtype* srt = er->subtypes;
	int deviceflags = er->deviceflags;
	cboScsiRomSubSelect->setEnabled(srt != nullptr); //ew(hDlg, IDC_SCSIROMSUBSELECT, srt != NULL);
	while (srt && srt->name) {
		scsirom_subselect_list.add(srt->name); //SendDlgItemMessage(hDlg, IDC_SCSIROMSUBSELECT, CB_ADDSTRING, 0, (LPARAM)srt->name);
		srt++;
	}
	int index;
	boardromconfig* brc = get_device_rom(&changed_prefs, int(expansionroms[scsiromselected].romtype), scsiromselectednum, &index);
	if (brc && er->subtypes) {
		cboScsiRomSubSelect->setSelected(brc->roms[index].subtype); //SendDlgItemMessage(hDlg, IDC_SCSIROMSUBSELECT, CB_SETCURSEL, brc->roms[index].subtype, 0);
		cboScsiRomId->setSelected(brc->roms[index].device_id); //SendDlgItemMessage(hDlg, IDC_SCSIROMID, CB_SETCURSEL, brc->roms[index].device_id, 0);
		deviceflags |= er->subtypes[brc->roms[index].subtype].deviceflags;
	}
	else if (srt) {
		cboScsiRomSubSelect->setSelected(0); //SendDlgItemMessage(hDlg, IDC_SCSIROMSUBSELECT, CB_SETCURSEL, 0, 0);
		cboScsiRomId->setSelected(0); //SendDlgItemMessage(hDlg, IDC_SCSIROMID, CB_SETCURSEL, 0, 0);
	}
	scsirom_selectnum_list.clear(); //SendDlgItemMessage(hDlg, IDC_SCSIROMSELECTNUM, CB_RESETCONTENT, 0, 0);
	if (deviceflags & EXPANSIONTYPE_CLOCKPORT) {
		scsirom_selectnum_list.add(_T("-")); //SendDlgItemMessage(hDlg, IDC_SCSIROMSELECTNUM, CB_ADDSTRING, 0, (LPARAM)_T("-"));
	}
	for (int i = 0; i < MAX_AVAILABLE_DUPLICATE_EXPANSION_BOARDS; i++) {
		TCHAR tmp[10];
		_stprintf(tmp, _T("%d"), i + 1);
		scsirom_selectnum_list.add(tmp); //SendDlgItemMessage(hDlg, IDC_SCSIROMSELECTNUM, CB_ADDSTRING, 0, (LPARAM)tmp);
	}
	cboScsiRomSelectNum->setSelected(scsiromselectednum); //SendDlgItemMessage(hDlg, IDC_SCSIROMSELECTNUM, CB_SETCURSEL, scsiromselectednum, 0);
	if ((er->zorro < 2 || er->singleonly) && !(deviceflags & EXPANSIONTYPE_CLOCKPORT)) {
		scsiromselectednum = 0;
		cboScsiRomSelectNum->setSelected(0); //SendDlgItemMessage(hDlg, IDC_SCSIROMSELECTNUM, CB_SETCURSEL, 0, 0);
	}
	cboScsiRomSelectNum->setEnabled((er->zorro >= 2 && !er->singleonly) || (deviceflags & EXPANSIONTYPE_CLOCKPORT)); //ew(hDlg, IDC_SCSIROMSELECTNUM, (er->zorro >= 2 && !er->singleonly) || (deviceflags & EXPANSIONTYPE_CLOCKPORT));
	//hide(hDlg, IDC_SCSIROM24BITDMA, (deviceflags & EXPANSIONTYPE_DMA24) == 0);
	//ew(hDlg, IDC_SCSIROM24BITDMA, (deviceflags & EXPANSIONTYPE_DMA24) != 0);
}

static void getromfile(gcn::DropDown* d, TCHAR* path, int size)
{
	auto val = d->getSelected();

	romdata* rd;
	auto tmp1 = d->getListModel()->getElementAt(val);	//SendDlgItemMessage(d, CB_GETLBTEXT, (WPARAM)val, (LPARAM)tmp1);
	path[0] = 0;
	rd = getromdatabyname(tmp1.c_str());
	if (rd) {
		romlist* rl = getromlistbyromdata(rd);
		if (rd->configname)
			_stprintf(path, _T(":%s"), rd->configname);
		else if (rl)
			_tcsncpy(path, rl->path, size);
	}
}

static void values_from_expansion2dlg()
{
	int index;
	boardromconfig* brc;
	TCHAR tmp[MAX_DPATH];
	bool changed = false;
	bool isnew = false;
	
	int checked = chkScsiRomSelected->isSelected();
	getromfile(cboScsiRomSelect, tmp, MAX_DPATH / sizeof(TCHAR));
	if (tmp[0] || checked) {
		const expansionromtype* ert = &expansionroms[scsiromselected];
		if (!get_device_rom(&changed_prefs, int(expansionroms[scsiromselected].romtype), scsiromselectednum, &index))
			isnew = true;
		brc = get_device_rom_new(&changed_prefs, int(expansionroms[scsiromselected].romtype), scsiromselectednum, &index);
		if (checked) {
			if (!brc->roms[index].romfile[0])
				changed = true;
			_tcscpy(brc->roms[index].romfile, _T(":ENABLED"));
		}
		else {
			changed = _tcscmp(tmp, brc->roms[index].romfile) != 0;
			getromfile(cboScsiRomSelect, brc->roms[index].romfile, MAX_DPATH / sizeof(TCHAR));
		}
		brc->roms[index].autoboot_disabled = false; //ischecked(hDlg, IDC_SCSIROMFILEAUTOBOOT);
		brc->roms[index].inserted = false; //ischecked(hDlg, IDC_SCSIROMFILEPCMCIA);
		brc->roms[index].dma24bit = false; //ischecked(hDlg, IDC_SCSIROM24BITDMA);

		int v = cboScsiRomId->getSelected(); //SendDlgItemMessage(IDC_SCSIROMID, CB_GETCURSEL, 0, 0L);
		if (!isnew)
			brc->roms[index].device_id = v;

		const expansionboardsettings* cbs = ert->settings;
		if (cbs) {
			brc->roms[index].device_settings = expansion_gui_item.expansionrom_gui_settings;
			_tcscpy(brc->roms[index].configtext, expansion_gui_item.expansionrom_gui_string);
		}

		v = cboScsiRomSubSelect->getSelected(); //SendDlgItemMessage(IDC_SCSIROMSUBSELECT, CB_GETCURSEL, 0, 0L);
		brc->roms[index].subtype = v;
	}
	else {
		brc = get_device_rom(&changed_prefs, int(expansionroms[scsiromselected].romtype), scsiromselectednum, &index);
		if (brc && brc->roms[index].romfile[0])
			changed = true;
		clear_device_rom(&changed_prefs, int(expansionroms[scsiromselected].romtype), scsiromselectednum, true);
	}
	if (changed) {
		// singleonly check and removal
		if (expansionroms[scsiromselected].singleonly) {
			if (get_device_rom(&changed_prefs, int(expansionroms[scsiromselected].romtype), scsiromselectednum, &index)) {
				for (int i = 0; i < MAX_EXPANSION_BOARDS; i++) {
					if (i != scsiromselectednum) {
						clear_device_rom(&changed_prefs, int(expansionroms[scsiromselected].romtype), i, true);
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

static void expansion2dlgproc()
{
	//Populate CPU boards list
	cpuboards_list.clear();
	for (int i = 0; cpuboards[i].name; i++)
	{
		cpuboards_list.add(cpuboards[i].name);
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

	chkScsiRomSelected->setVisible(false); //hide(IDC_SCSIROMSELECTED, 1);
	init_expansion2(true);
	updatecpuboardsubtypes();
	setcpuboardmemsize();
}

class ExpansionsActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& action_event) override
	{
		if (action_event.getSource() == txtExpansionBoardStringBox
			|| action_event.getSource() == chkExpansionBoardCheckbox
			|| action_event.getSource() == cboExpansionBoardItemSelector
			|| action_event.getSource() == cboExpansionBoardSelector)
		{
			get_expansionrom_gui(&expansion_gui_item);
			values_from_expansion2dlg();
		}
		else if (action_event.getSource() == chkAcceleratorBoardCheckbox
			|| action_event.getSource() == cboAcceleratorBoardItemSelector
			|| action_event.getSource() == cboAcceleratorBoardSelector)
		{
			get_expansionrom_gui(&accelerator_gui_item);
			values_from_expansion2dlg();
		}
		else if (action_event.getSource() == chkScsiRomFileAutoboot
			|| action_event.getSource() == chkScsiRomFilePcmcia)
		{
			values_from_expansion2dlg();
		}
		else if (action_event.getSource() == chkBSDSocket)
		{
			changed_prefs.socket_emu = chkBSDSocket->isSelected();
		}
		else if (action_event.getSource() == chkScsi)
		{
			changed_prefs.scsi = chkScsi->isSelected();
			RefreshPanelExpansions();
		}
		else if (action_event.getSource() == chkSana2)
		{
			changed_prefs.sana2 = chkSana2->isSelected();
		}
		else if (action_event.getSource() == chkCD32Fmv)
		{
			changed_prefs.cs_cd32fmv = chkCD32Fmv->isSelected();
			cfgfile_compatibility_romtype(&changed_prefs);
		}
		else if (action_event.getSource() == chkScsiRomSelected
			|| action_event.getSource() == cboScsiRomFile
			|| action_event.getSource() == cboScsiRomId
			|| action_event.getSource() == cboCpuBoardRomFile
			|| action_event.getSource() == cboCpuBoardSubType)
		{
			values_from_expansion2dlg();
			values_to_expansion2_expansion_settings();
		}
		else if (action_event.getSource() == cboScsiRomSubSelect)
		{
			values_from_expansion2dlg();
			values_to_expansion2_expansion_roms();
			values_to_expansion2_expansion_settings();
		}
		else if (action_event.getSource() == cboScsiRomSelectCat)
		{
			auto val = cboScsiRomSelectCat->getSelected();
			if (val != -1 && val != scsiromselectedcatnum)
			{
				scsiromselectedcatnum = val;
				scsiromselected = 0;
				init_expansion2(false);
				values_to_expansion2_expansion_roms();
				values_to_expansion2_expansion_settings();
				values_to_expansion2dlg_sub();
			}
		}
		else if (action_event.getSource() == cboScsiRomSelectNum
			|| action_event.getSource() == cboScsiRomSelect)
		{
			auto val = cboScsiRomSelectNum->getSelected();
			if (val != -1)
				scsiromselectednum = val;
			val = gui_get_string_cursor(scsiromselect_table, cboScsiRomSelect);
			if (val != -1)
			{
				scsiromselected = val;
				values_to_expansion2_expansion_roms();
				values_to_expansion2_expansion_settings();
				values_to_expansion2dlg_sub();
			}
		}
		
		else if (action_event.getSource() == cboCpuBoardType)
		{
			changed_prefs.cpuboard_type = cboCpuBoardType->getSelected();
			changed_prefs.cpuboard_subtype = 0;
			changed_prefs.cpuboard_settings = 0;
			updatecpuboardsubtypes();
			//cpuboard_set_cpu(&changed_prefs);
			setcpuboardmemsize();
			RefreshPanelExpansions();
		}
		else if (action_event.getSource() == cboCpuBoardSubType)
		{
			changed_prefs.cpuboard_subtype = cboCpuBoardSubType->getSelected();
			changed_prefs.cpuboard_settings = 0;
			updatecpuboardsubtypes();
			//cpuboard_set_cpu(&changed_prefs);
			setcpuboardmemsize();
			RefreshPanelExpansions();
		}	
	}
};

ExpansionsActionListener* expansions_action_listener;

void InitPanelExpansions(const config_category& category)
{
	expansions_action_listener = new ExpansionsActionListener();

	cboScsiRomSelectCat = new gcn::DropDown(&scsirom_select_cat_list);
	cboScsiRomSelectCat->setSize(250, cboScsiRomSelectCat->getHeight());
	cboScsiRomSelectCat->setBaseColor(gui_baseCol);
	cboScsiRomSelectCat->setBackgroundColor(colTextboxBackground);
	cboScsiRomSelectCat->setSelectionColor(gui_selection_color);
	cboScsiRomSelectCat->setId("cboScsiRomSelectCat");
	cboScsiRomSelectCat->addActionListener(expansions_action_listener);
	
	cboScsiRomSelect = new gcn::DropDown(&scsirom_select_list);
	cboScsiRomSelect->setSize(250, cboScsiRomSelect->getHeight());
	cboScsiRomSelect->setBaseColor(gui_baseCol);
	cboScsiRomSelect->setBackgroundColor(colTextboxBackground);
	cboScsiRomSelect->setSelectionColor(gui_selection_color);
	cboScsiRomSelect->setId("cboScsiRomSelect");
	cboScsiRomSelect->addActionListener(expansions_action_listener);

	cboScsiRomSubSelect = new gcn::DropDown(&scsirom_subselect_list);
	cboScsiRomSubSelect->setSize(250, cboScsiRomSubSelect->getHeight());
	cboScsiRomSubSelect->setBaseColor(gui_baseCol);
	cboScsiRomSubSelect->setBackgroundColor(colTextboxBackground);
	cboScsiRomSubSelect->setSelectionColor(gui_selection_color);
	cboScsiRomSubSelect->setId("cboScsiRomSubSelect");
	cboScsiRomSubSelect->addActionListener(expansions_action_listener);

	chkScsiRomSelected = new gcn::CheckBox("Enabled");
	chkScsiRomSelected->setId("chkScsiRomSelected");
	chkScsiRomSelected->addActionListener(expansions_action_listener);

	cboScsiRomFile = new gcn::DropDown(&scsirom_file_list);
	cboScsiRomFile->setSize(200, cboScsiRomFile->getHeight());
	cboScsiRomFile->setBaseColor(gui_baseCol);
	cboScsiRomFile->setBackgroundColor(colTextboxBackground);
	cboScsiRomFile->setSelectionColor(gui_selection_color);
	cboScsiRomFile->setId("cboScsiRomFile");
	cboScsiRomFile->addActionListener(expansions_action_listener);

	cboScsiRomId = new gcn::DropDown(&scsi_romid_list);
	cboScsiRomId->setSize(30, cboScsiRomId->getHeight());
	cboScsiRomId->setBaseColor(gui_baseCol);
	cboScsiRomId->setBackgroundColor(colTextboxBackground);
	cboScsiRomId->setSelectionColor(gui_selection_color);
	cboScsiRomId->setId("cboScsiRomId");
	cboScsiRomId->addActionListener(expansions_action_listener);

	cboScsiRomSelectNum = new gcn::DropDown(&scsirom_selectnum_list);
	cboScsiRomSelectNum->setSize(30, cboScsiRomSelectNum->getHeight());
	cboScsiRomSelectNum->setBaseColor(gui_baseCol);
	cboScsiRomSelectNum->setBackgroundColor(colTextboxBackground);
	cboScsiRomSelectNum->setSelectionColor(gui_selection_color);
	cboScsiRomSelectNum->setId("cboScsiRomSelectNum");
	cboScsiRomSelectNum->addActionListener(expansions_action_listener);
	
	btnScsiRomChooser = new gcn::Button("...");
	btnScsiRomChooser->setBaseColor(gui_baseCol);
	btnScsiRomChooser->setBackgroundColor(colTextboxBackground);
	btnScsiRomChooser->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	btnScsiRomChooser->setId("btnScsiRomChooser");
	btnScsiRomChooser->addActionListener(expansions_action_listener);

	chkScsiRomFileAutoboot = new gcn::CheckBox("Autoboot disabled");
	chkScsiRomFileAutoboot->setId("chkScsiRomFileAutoboot");
	chkScsiRomFileAutoboot->addActionListener(expansions_action_listener);

	chkScsiRomFilePcmcia = new gcn::CheckBox("PCMCIA inserted");
	chkScsiRomFilePcmcia->setId("chkScsiRomFilePcmcia");
	chkScsiRomFilePcmcia->addActionListener(expansions_action_listener);
	
	cboExpansionBoardItemSelector = new gcn::DropDown(&expansionboard_itemselector_list);
	cboExpansionBoardItemSelector->setSize(250, cboExpansionBoardItemSelector->getHeight());
	cboExpansionBoardItemSelector->setBaseColor(gui_baseCol);
	cboExpansionBoardItemSelector->setBackgroundColor(colTextboxBackground);
	cboExpansionBoardItemSelector->setSelectionColor(gui_selection_color);
	cboExpansionBoardItemSelector->setId("cboExpansionBoardItemSelector");
	cboExpansionBoardItemSelector->addActionListener(expansions_action_listener);

	//TODO this one is probably not correct
	cboExpansionBoardSelector = new gcn::DropDown(&expansionboard_itemselector_list);
	cboExpansionBoardSelector->setSize(250, cboExpansionBoardSelector->getHeight());
	cboExpansionBoardSelector->setBaseColor(gui_baseCol);
	cboExpansionBoardSelector->setBackgroundColor(colTextboxBackground);
	cboExpansionBoardSelector->setSelectionColor(gui_selection_color);
	cboExpansionBoardSelector->setId("cboExpansionBoardSelector");
	cboExpansionBoardSelector->addActionListener(expansions_action_listener);

	chkExpansionBoardCheckbox = new gcn::CheckBox("");
	chkExpansionBoardCheckbox->setId("chkExpansionBoardCheckbox");
	chkExpansionBoardCheckbox->addActionListener(expansions_action_listener);

	txtExpansionBoardStringBox = new gcn::TextBox();
	txtExpansionBoardStringBox->setSize(200, txtExpansionBoardStringBox->getHeight());
	txtExpansionBoardStringBox->setId("txtExpansionBoardStringBox");
	
	cboCpuBoardType = new gcn::DropDown(&cpuboards_list);
	cboCpuBoardType->setSize(250, cboCpuBoardType->getHeight());
	cboCpuBoardType->setBaseColor(gui_baseCol);
	cboCpuBoardType->setBackgroundColor(colTextboxBackground);
	cboCpuBoardType->setSelectionColor(gui_selection_color);
	cboCpuBoardType->setId("cboCpuBoardType");
	cboCpuBoardType->addActionListener(expansions_action_listener);
	cboCpuBoardType->setEnabled(false); //TODO enable this when implemented

	cboCpuBoardSubType = new gcn::DropDown(&cpuboards_subtype_list);
	cboCpuBoardSubType->setSize(250, cboCpuBoardSubType->getHeight());
	cboCpuBoardSubType->setBaseColor(gui_baseCol);
	cboCpuBoardSubType->setBackgroundColor(colTextboxBackground);
	cboCpuBoardSubType->setSelectionColor(gui_selection_color);
	cboCpuBoardSubType->setId("cboCpuBoardSubType");
	cboCpuBoardSubType->addActionListener(expansions_action_listener);
	cboCpuBoardSubType->setEnabled(false); //TODO enable this when implemented

	cboCpuBoardRomFile = new gcn::DropDown(&cpuboard_romfile_list);
	cboCpuBoardRomFile->setSize(250, cboCpuBoardRomFile->getHeight());
	cboCpuBoardRomFile->setBaseColor(gui_baseCol);
	cboCpuBoardRomFile->setBackgroundColor(colTextboxBackground);
	cboCpuBoardRomFile->setSelectionColor(gui_selection_color);
	cboCpuBoardRomFile->setId("cboCpuBoardRomFile");
	cboCpuBoardRomFile->addActionListener(expansions_action_listener);
	cboCpuBoardRomFile->setEnabled(false); //TODO Enable this when implemented

	cboAcceleratorBoardItemSelector = new gcn::DropDown(&acceleratorboard_itemselector_list);
	cboAcceleratorBoardItemSelector->setSize(250, cboAcceleratorBoardItemSelector->getHeight());
	cboAcceleratorBoardItemSelector->setBaseColor(gui_baseCol);
	cboAcceleratorBoardItemSelector->setBackgroundColor(colTextboxBackground);
	cboAcceleratorBoardItemSelector->setSelectionColor(gui_selection_color);
	cboAcceleratorBoardItemSelector->setId("cboAcceleratorBoardItemSelector");
	cboAcceleratorBoardItemSelector->addActionListener(expansions_action_listener);

	cboAcceleratorBoardSelector = new gcn::DropDown(&acceleratorboard_selector_list);
	cboAcceleratorBoardSelector->setSize(250, cboAcceleratorBoardItemSelector->getHeight());
	cboAcceleratorBoardSelector->setBaseColor(gui_baseCol);
	cboAcceleratorBoardSelector->setBackgroundColor(colTextboxBackground);
	cboAcceleratorBoardSelector->setSelectionColor(gui_selection_color);
	cboAcceleratorBoardSelector->setId("cboAcceleratorBoardSelector");
	cboAcceleratorBoardSelector->addActionListener(expansions_action_listener);

	chkAcceleratorBoardCheckbox = new gcn::CheckBox("");
	chkAcceleratorBoardCheckbox->setId("chkAcceleratorBoardCheckbox");
	chkAcceleratorBoardCheckbox->addActionListener(expansions_action_listener);
	
	chkBSDSocket = new gcn::CheckBox("bsdsocket.library");
	chkBSDSocket->setId("chkBSDSocket");
	chkBSDSocket->addActionListener(expansions_action_listener);

	chkScsi = new gcn::CheckBox("uaescsi.device");
	chkScsi->setId("chkSCSI");
	chkScsi->addActionListener(expansions_action_listener);

	chkCD32Fmv = new gcn::CheckBox("CD32 Full Motion Video cartridge");
	chkCD32Fmv->setId("chkCD32Fmv");
	chkCD32Fmv->addActionListener(expansions_action_listener);

	chkSana2 = new gcn::CheckBox("uaenet.device");
	chkSana2->setId("chkSana2");
	chkSana2->addActionListener(expansions_action_listener);
	chkSana2->setEnabled(false); //TODO enable this when SANA2 support is implemented
	
	int posY = DISTANCE_BORDER;
	int posX = DISTANCE_BORDER;
	
	grpExpansionBoard = new gcn::Window("Expansion Board Settings");
	grpExpansionBoard->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	grpExpansionBoard->add(cboScsiRomSelectCat, posX, posY);
	grpExpansionBoard->add(cboScsiRomId, cboScsiRomSelectCat->getX() + cboScsiRomSelectCat->getWidth() + DISTANCE_NEXT_X * 15, cboScsiRomSelectCat->getY());
	grpExpansionBoard->add(cboScsiRomSelect, posX, posY + cboScsiRomSelectCat->getHeight() + DISTANCE_NEXT_Y);
	grpExpansionBoard->add(cboScsiRomSelectNum, cboScsiRomSelect->getX() + cboScsiRomSelect->getWidth() + DISTANCE_NEXT_X, cboScsiRomSelect->getY());
	grpExpansionBoard->add(chkScsiRomSelected, cboScsiRomSelectNum->getX() + cboScsiRomSelectNum->getWidth() + DISTANCE_NEXT_X, cboScsiRomSelectNum->getY());
	grpExpansionBoard->add(cboScsiRomFile, chkScsiRomSelected->getX(), chkScsiRomSelected->getY());
	grpExpansionBoard->add(btnScsiRomChooser, cboScsiRomFile->getX() + cboScsiRomFile->getWidth() + DISTANCE_NEXT_X, cboScsiRomFile->getY());
	grpExpansionBoard->add(cboScsiRomSubSelect, posX, cboScsiRomSelect->getY() + cboScsiRomSelect->getHeight() + DISTANCE_NEXT_Y);
	grpExpansionBoard->add(chkScsiRomFileAutoboot, chkScsiRomSelected->getX(), cboScsiRomSubSelect->getY());
	grpExpansionBoard->add(chkScsiRomFilePcmcia, chkScsiRomFileAutoboot->getX() + chkScsiRomFileAutoboot->getWidth(), chkScsiRomFileAutoboot->getY());
	grpExpansionBoard->add(txtExpansionBoardStringBox, chkScsiRomFileAutoboot->getX(), chkScsiRomFileAutoboot->getY());
	grpExpansionBoard->add(chkExpansionBoardCheckbox, chkScsiRomFileAutoboot->getX(), chkScsiRomFileAutoboot->getY() + DISTANCE_NEXT_Y);
	grpExpansionBoard->add(cboExpansionBoardItemSelector, posX, cboScsiRomSubSelect->getY() + cboScsiRomSubSelect->getHeight() + DISTANCE_NEXT_Y);
	//grpExpansionBoard->add(cboExpansionBoardSelector, chkScsiRomSelected->getX(), cboExpansionBoardItemSelector->getY());
	//TODO add items here
	grpExpansionBoard->setMovable(false);
	grpExpansionBoard->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2, 250);
	grpExpansionBoard->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpExpansionBoard->setBaseColor(gui_baseCol);
	category.panel->add(grpExpansionBoard);

	grpAcceleratorBoard = new gcn::Window("Accelerator Board Settings");
	grpAcceleratorBoard->setPosition(DISTANCE_BORDER, grpExpansionBoard->getY() + grpExpansionBoard->getHeight() + DISTANCE_NEXT_Y);
	grpAcceleratorBoard->add(cboCpuBoardType, posX, posY);
	grpAcceleratorBoard->add(cboCpuBoardSubType, posX, posY + cboCpuBoardType->getHeight() + DISTANCE_NEXT_Y);
	grpAcceleratorBoard->add(cboCpuBoardRomFile, cboCpuBoardType->getX() + cboCpuBoardType->getWidth() + DISTANCE_NEXT_X * 3, cboCpuBoardType->getY());
	//TODO add items here
	grpAcceleratorBoard->setMovable(false);
	grpAcceleratorBoard->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2, 200);
	grpAcceleratorBoard->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpAcceleratorBoard->setBaseColor(gui_baseCol);
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
	grpMiscExpansions->setBaseColor(gui_baseCol);
	category.panel->add(grpMiscExpansions);

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
	expansion2dlgproc();
	
	// values_to_expansion2dlg is covered here
	chkBSDSocket->setEnabled(!emulating);
	chkBSDSocket->setSelected(changed_prefs.socket_emu);
	chkScsi->setSelected(changed_prefs.scsi);
	chkCD32Fmv->setSelected(changed_prefs.cs_cd32fmv);
	chkSana2->setSelected(changed_prefs.sana2);

	values_to_expansion2_expansion_roms();
	values_to_expansion2_expansion_settings();

	int index;
	boardromconfig* brc;
	if (changed_prefs.cpuboard_type) {
		const cpuboardsubtype* cst = &cpuboards[changed_prefs.cpuboard_type].subtypes[changed_prefs.cpuboard_subtype];
		brc = get_device_rom(&changed_prefs, ROMTYPE_CPUBOARD, 0, &index);
		addromfiles(cboCpuBoardRomFile, brc ? brc->roms[index].romfile : nullptr,
			int(cst->romtype), cst->romtype_extra);
	}
	else {
		auto list_model = cboCpuBoardRomFile->getListModel();
		list_model->clear(); //SendDlgItemMessage(hDlg, IDC_CPUBOARDROMFILE, CB_RESETCONTENT, 0, 0);
	}

	gui_set_string_cursor(scsiromselect_table, cboScsiRomSelect, scsiromselected);
	values_to_expansion2dlg_sub();

	// enable_for_expansion2dlg is covered here
	cboCpuBoardRomFile->setEnabled(changed_prefs.cpuboard_type != 0); //ew(hDlg, IDC_CPUBOARDROMFILE, workprefs.cpuboard_type != 0);
	//ew(hDlg, IDC_CPUBOARDROMCHOOSER, workprefs.cpuboard_type != 0);
	//ew(hDlg, IDC_CPUBOARDMEM, workprefs.cpuboard_type > 0);
	//ew(hDlg, IDC_CPUBOARDRAM, workprefs.cpuboard_type > 0);
	//ew(hDlg, IDC_CPUBOARD_SUBTYPE, workprefs.cpuboard_type);
}

bool HelpPanelExpansions(std::vector<std::string>& helptext)
{
    helptext.clear();
    helptext.emplace_back("Some of these expansions will be set automatically for you, depending on the model of");
    helptext.emplace_back("Amiga you chose to emulate (ie; the internal IDE for A1200 models).");
    helptext.emplace_back(" ");
    helptext.emplace_back("Please note that many of the options here are not yet fully functional, since they are");
    helptext.emplace_back("currently stripped down in Amiberry. They may be added in future versions, but for now ");
    helptext.emplace_back("only the most commonly used options are available.");
    helptext.emplace_back(" ");
    helptext.emplace_back("Here you can enable a few expansion options that are commonly used:");
    helptext.emplace_back(" ");
    helptext.emplace_back("\"bsdsocket.library\" enables network functions (ie; for web browsers in OS3.9).");
    helptext.emplace_back("When this option is enabled, you do not need to use a native Amiga TCP stack (ie;");
    helptext.emplace_back("AmiTCP/Genesis/Roadshow), as the Amiga applications you run will see your emulated");
    helptext.emplace_back("Amiga as being connected to the network Amiberry is running on.");
    helptext.emplace_back(" ");
    helptext.emplace_back("\"uaescsi.device\" enables the uaescsi.device, which can be used to connect to hard");
    helptext.emplace_back("drives inside the emulation.");
    helptext.emplace_back(" ");
    helptext.emplace_back("\"CD32 Full Motion Video cartridge\" enables the FMV module for the CD32, which is");
    helptext.emplace_back("only useful if you're emulating a CD32 of course.");
    helptext.emplace_back(" ");
    helptext.emplace_back("\"uaenet.device\" is currently disabled. ");
    helptext.emplace_back(" ");
    return true;
}
