#include <cstring>
#include <cstdio>

#include <guisan.hpp>
#include <SDL_image.h>
#include <guisan/sdl.hpp>

#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "options.h"
#include "gui_handling.h"
#include "StringListModel.h"

static gcn::Label* lblWhdload;
static gcn::DropDown* cboWhdload;
static gcn::Button* cmdWhdloadEject;
static gcn::Button* cmdWhdloadSelect;

// WHDLoad game options
static gcn::Label* lblGameName;
static gcn::TextField* txtGameName;

static gcn::Label* lblSubPath;
static gcn::TextField* txtSubPath;

static gcn::Label* lblVariantUuid;
static gcn::TextField* txtVariantUuid;

static gcn::Label* lblSlaveCount;
static gcn::TextField* txtSlaveCount;

static gcn::Label* lblSlaveDefault;
static gcn::TextField* txtSlaveDefault;

static gcn::CheckBox* chkSlaveLibraries;

static gcn::Label* lblSlaves;
static gcn::DropDown* cboSlaves;

// WHDLoad global options
static gcn::Window* grpWHDLoadGlobal;
static gcn::Label* lblCustomText;
static gcn::TextField* txtCustomText;

static gcn::CheckBox* chkButtonWait;
static gcn::CheckBox* chkShowSplash;

static gcn::Label* lblConfigDelay;
static gcn::TextField* txtConfigDelay;

static gcn::CheckBox* chkWriteCache;
static gcn::CheckBox* chkQuitOnExit;

// Selected Slave options
static gcn::Label* lblSlaveDataPath;
static gcn::TextField* txtSlaveDataPath;

// Selected Slave Custom options
static gcn::Label* lblCustom[5];
static gcn::CheckBox* chkCustom[5];
static gcn::DropDown* cboCustom[5];


static gcn::StringListModel whdloadFileList;
static gcn::StringListModel slaves_list;
static gcn::StringListModel custom_list[5];

static bool bIgnoreListChange = true;

static void RefreshWhdListModel()
{
	whdloadFileList.clear();
	for (const auto& i : lstMRUWhdloadList)
	{
		const std::string full_path = i;
		const std::string filename = full_path.substr(full_path.find_last_of("/\\") + 1);
		whdloadFileList.add(std::string(filename).append(" { ").append(full_path).append(" }"));
	}
}

static void AdjustDropDownControls()
{
	bIgnoreListChange = true;

	cboWhdload->clearSelected();
	if (!whdload_filename.empty())
	{
		for (auto i = 0; i < static_cast<int>(lstMRUWhdloadList.size()); ++i)
		{
			if (lstMRUWhdloadList[i] == whdload_filename)
			{
				cboWhdload->setSelected(i);
				break;
			}
		}
	}

	bIgnoreListChange = false;
}

class WHDLoadActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		//---------------------------------------
		// WHDLoad file from list selected
		//---------------------------------------
		if (!bIgnoreListChange)
		{
			if (actionEvent.getSource() == cboWhdload)
			{
				const auto idx = cboWhdload->getSelected();

				if (idx < 0)
				{
					whdload_filename = "";
				}
				else
				{
					const auto element = get_full_path_from_disk_list(whdloadFileList.getElementAt(idx));
					if (element != whdload_filename)
					{
						whdload_filename.assign(element);
						lstMRUWhdloadList.erase(lstMRUWhdloadList.begin() + idx);
						lstMRUWhdloadList.insert(lstMRUWhdloadList.begin(), whdload_filename);
						bIgnoreListChange = true;
						cboWhdload->setSelected(0);
						bIgnoreListChange = false;
					}
					whdload_auto_prefs(&changed_prefs, whdload_filename.c_str());
				}
				refresh_all_panels();
			}
		}
	}
};

static WHDLoadActionListener* whdloadActionListener;

class WHDLoadButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdWhdloadEject)
		{
			//---------------------------------------
			// Eject WHDLoad file
			//---------------------------------------
			whdload_filename = "";
		}
		else if (actionEvent.getSource() == cmdWhdloadSelect)
		{
			std::string tmp;
			if (!whdload_filename.empty())
				tmp = whdload_filename;
			else
				tmp = get_whdload_arch_path();

			tmp = SelectFile("Select WHDLoad LHA file", tmp, whdload_filter);
			{
				whdload_filename.assign(tmp);
				add_file_to_mru_list(lstMRUWhdloadList, whdload_filename);
				RefreshWhdListModel();
				whdload_auto_prefs(&changed_prefs, whdload_filename.c_str());

				AdjustDropDownControls();
			}
			cmdWhdloadSelect->requestFocus();
		}
		refresh_all_panels();
	}
};

static WHDLoadButtonActionListener* whdloadButtonActionListener;

void InitPanelWHDLoad(const struct config_category& category)
{
	slaves_list.clear();

	constexpr int textfield_width = 350;

	whdloadActionListener = new WHDLoadActionListener();
	whdloadButtonActionListener = new WHDLoadButtonActionListener();

	lblWhdload = new gcn::Label("WHDLoad auto-config:");
	cboWhdload = new gcn::DropDown(&whdloadFileList);
	cboWhdload->setSize(category.panel->getWidth() - 2 * DISTANCE_BORDER, cboWhdload->getHeight());
	cboWhdload->setBaseColor(gui_baseCol);
	cboWhdload->setBackgroundColor(colTextboxBackground);
	cboWhdload->setId("cboWhdload");
	cboWhdload->addActionListener(whdloadActionListener);

	cmdWhdloadEject = new gcn::Button("Eject");
	cmdWhdloadEject->setSize(SMALL_BUTTON_WIDTH * 2, SMALL_BUTTON_HEIGHT);
	cmdWhdloadEject->setBaseColor(gui_baseCol);
	cmdWhdloadEject->setId("cmdWhdloadEject");
	cmdWhdloadEject->addActionListener(whdloadButtonActionListener);

	cmdWhdloadSelect = new gcn::Button("Select file");
	cmdWhdloadSelect->setSize(BUTTON_WIDTH + 10, SMALL_BUTTON_HEIGHT);
	cmdWhdloadSelect->setBaseColor(gui_baseCol);
	cmdWhdloadSelect->setId("cmdWhdloadSelect");
	cmdWhdloadSelect->addActionListener(whdloadButtonActionListener);

	// WHDLoad options
	lblGameName = new gcn::Label("Game Name:");
	txtGameName = new gcn::TextField();
	txtGameName->setSize(textfield_width, TEXTFIELD_HEIGHT);
	txtGameName->setBackgroundColor(colTextboxBackground);

	lblSubPath = new gcn::Label("Sub Path:");
	txtSubPath = new gcn::TextField();
	txtSubPath->setSize(textfield_width, TEXTFIELD_HEIGHT);
	txtSubPath->setBackgroundColor(colTextboxBackground);

	lblVariantUuid = new gcn::Label("UUID:");
	txtVariantUuid = new gcn::TextField();
	txtVariantUuid->setSize(textfield_width, TEXTFIELD_HEIGHT);
	txtVariantUuid->setBackgroundColor(colTextboxBackground);

	lblSlaveCount = new gcn::Label("Slave count:");
	txtSlaveCount = new gcn::TextField();
	txtSlaveCount->setSize(textfield_width, TEXTFIELD_HEIGHT);
	txtSlaveCount->setBackgroundColor(colTextboxBackground);

	lblSlaveDefault = new gcn::Label("Slave Default:");
	txtSlaveDefault = new gcn::TextField();
	txtSlaveDefault->setSize(textfield_width, TEXTFIELD_HEIGHT);
	txtSlaveDefault->setBackgroundColor(colTextboxBackground);

	chkSlaveLibraries = new gcn::CheckBox("Slave Libraries");

	lblSlaves = new gcn::Label("Slaves:");
	cboSlaves = new gcn::DropDown(&slaves_list);
	cboSlaves->setSize(textfield_width, cboSlaves->getHeight());
	cboSlaves->setBaseColor(gui_baseCol);
	cboSlaves->setBackgroundColor(colTextboxBackground);

	lblSlaveDataPath = new gcn::Label("Slave Data path:");
	txtSlaveDataPath = new gcn::TextField();
	txtSlaveDataPath->setSize(textfield_width, TEXTFIELD_HEIGHT);
	txtSlaveDataPath->setBackgroundColor(colTextboxBackground);

	// CUSTOM 1-5
	for (int i = 0; i < 5; i++)
	{
		custom_list[i].clear();
		std::string caption = "Custom" + std::to_string(i + 1);
		std::string id;

		lblCustom[i] = new gcn::Label(caption);

		id = "chkCustom" + std::to_string(i);
		chkCustom[i] = new gcn::CheckBox(caption);
		chkCustom[i]->setId(id);

		id = "cboCustom" + std::to_string(i);
		cboCustom[i] = new gcn::DropDown(&custom_list[i]);
		cboCustom[i]->setId(id);
		cboCustom[i]->setSize(textfield_width, cboCustom[i]->getHeight());
		cboCustom[i]->setBaseColor(gui_baseCol);
		cboCustom[i]->setBackgroundColor(colTextboxBackground);
	}

	lblCustomText = new gcn::Label("Custom:");
	txtCustomText = new gcn::TextField();
	txtCustomText->setSize(textfield_width, TEXTFIELD_HEIGHT);
	txtCustomText->setBackgroundColor(colTextboxBackground);

	chkButtonWait = new gcn::CheckBox("Button Wait");
	chkShowSplash = new gcn::CheckBox("Show Splash");

	lblConfigDelay = new gcn::Label("Config Delay:");
	txtConfigDelay = new gcn::TextField();
	txtConfigDelay->setSize(textfield_width, TEXTFIELD_HEIGHT);
	txtConfigDelay->setBackgroundColor(colTextboxBackground);

	chkWriteCache = new gcn::CheckBox("Write Cache");
	chkQuitOnExit = new gcn::CheckBox("Quit on Exit");

	constexpr int pos_x1 = DISTANCE_BORDER;
	const int pos_x2 = chkSlaveLibraries->getWidth() + 8;
	int pos_y = DISTANCE_BORDER;

	category.panel->add(lblWhdload, DISTANCE_BORDER, pos_y);
	category.panel->add(cmdWhdloadEject, lblWhdload->getX() + lblWhdload->getWidth() + DISTANCE_NEXT_X * 16, pos_y);
	category.panel->add(cmdWhdloadSelect, cmdWhdloadEject->getX() + cmdWhdloadEject->getWidth() + DISTANCE_NEXT_X, pos_y);
	pos_y += cmdWhdloadSelect->getHeight() + 8;

	category.panel->add(cboWhdload, DISTANCE_BORDER, pos_y);
	pos_y += cboWhdload->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblGameName, pos_x1, pos_y);
	category.panel->add(txtGameName, pos_x2, pos_y);
	pos_y += lblGameName->getHeight() + 8;

	category.panel->add(lblSubPath, pos_x1, pos_y);
	category.panel->add(txtSubPath, pos_x2, pos_y);
	pos_y += lblSubPath->getHeight() + 8;

	category.panel->add(lblVariantUuid, pos_x1, pos_y);
	category.panel->add(txtVariantUuid, pos_x2, pos_y);
	pos_y += lblVariantUuid->getHeight() + 8;

	category.panel->add(lblSlaveCount, pos_x1, pos_y);
	category.panel->add(txtSlaveCount, pos_x2, pos_y);
	pos_y += lblSlaveCount->getHeight() + 8;

	category.panel->add(lblSlaveDefault, pos_x1, pos_y);
	category.panel->add(txtSlaveDefault, pos_x2, pos_y);
	pos_y += lblSlaveDefault->getHeight() + 8;

	category.panel->add(chkSlaveLibraries, pos_x1, pos_y);
	pos_y += chkSlaveLibraries->getHeight() + 8;

	category.panel->add(lblSlaves, pos_x1, pos_y);
	category.panel->add(cboSlaves, pos_x2, pos_y);
	pos_y += lblSlaves->getHeight() + 8;

	category.panel->add(lblSlaveDataPath, pos_x1, pos_y);
	category.panel->add(txtSlaveDataPath, pos_x2, pos_y);
	pos_y += lblSlaveDataPath->getHeight() + 8;

	category.panel->add(lblCustomText, pos_x1, pos_y);
	category.panel->add(txtCustomText, pos_x2, pos_y);
	pos_y += lblCustomText->getHeight() + DISTANCE_NEXT_Y;

	for (int i = 0; i < 5; i++)
	{
		category.panel->add(lblCustom[i], pos_x1, pos_y);
		category.panel->add(chkCustom[i], pos_x2, pos_y);
		category.panel->add(cboCustom[i], pos_x2, pos_y);
		pos_y += cboCustom[i]->getHeight() + 8;
	}

	pos_y += DISTANCE_NEXT_Y;

	grpWHDLoadGlobal = new gcn::Window("Global options");
	grpWHDLoadGlobal->setPosition(pos_x1, pos_y);
	grpWHDLoadGlobal->setMovable(false);
	grpWHDLoadGlobal->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpWHDLoadGlobal->setBaseColor(gui_baseCol);

	pos_y = 10;
		grpWHDLoadGlobal->add(chkButtonWait, pos_x1, pos_y);
	pos_y += chkButtonWait->getHeight() + 8;

	grpWHDLoadGlobal->add(chkShowSplash, pos_x1, pos_y);
	pos_y += chkShowSplash->getHeight() + 8;

	grpWHDLoadGlobal->add(lblConfigDelay, pos_x1, pos_y);
	grpWHDLoadGlobal->add(txtConfigDelay, pos_x2, pos_y);
	pos_y += txtConfigDelay->getHeight() + 8;

	grpWHDLoadGlobal->add(chkWriteCache, pos_x1, pos_y);
	pos_y += chkWriteCache->getHeight() + 8;

	grpWHDLoadGlobal->add(chkQuitOnExit, pos_x1, pos_y);
	grpWHDLoadGlobal->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2, 
		chkQuitOnExit->getY() + chkQuitOnExit->getHeight() + DISTANCE_NEXT_Y + TITLEBAR_HEIGHT);
	category.panel->add(grpWHDLoadGlobal);

	bIgnoreListChange = false;

	RefreshPanelWHDLoad();
}

void ExitPanelWHDLoad()
{
	delete lblWhdload;
	delete cboWhdload;
	delete cmdWhdloadEject;
	delete cmdWhdloadSelect;

	// WHDLoad options
	delete lblGameName;
	delete txtGameName;

	delete lblSubPath;
	delete txtSubPath;

	delete lblVariantUuid;
	delete txtVariantUuid;

	delete lblSlaveCount;
	delete txtSlaveCount;

	delete lblSlaveDefault;
	delete txtSlaveDefault;

	delete chkSlaveLibraries;

	delete lblSlaves;
	delete cboSlaves;

	delete lblCustomText;
	delete txtCustomText;

	delete chkButtonWait;
	delete chkShowSplash;

	delete lblConfigDelay;
	delete txtConfigDelay;

	delete chkWriteCache;
	delete chkQuitOnExit;

	// Selected Slave options
	delete lblSlaveDataPath;
	delete txtSlaveDataPath;

	// Selected Slave Custom options
	for (int i = 0; i < 5; i++)
	{
		delete lblCustom[i];
		delete chkCustom[i];
		delete cboCustom[i];
	}
	delete grpWHDLoadGlobal;

	delete whdloadActionListener;
	delete whdloadButtonActionListener;
}

void update_custom_fields(const whdload_custom& custom, const int custom_x)
{
	if (custom.type == checkbox)
	{
		chkCustom[custom_x]->setCaption(custom.caption);
		chkCustom[custom_x]->adjustSize();
		chkCustom[custom_x]->setVisible(true);
		chkCustom[custom_x]->setSelected(custom.value > 0);

		// Hide the other widgets
		cboCustom[custom_x]->setVisible(false);
	}
	else if (custom.type == list)
	{
		lblCustom[custom_x]->setCaption(custom.caption);
		lblCustom[custom_x]->adjustSize();
		custom_list[custom_x].clear();
		for (const auto& label : custom.labels)
		{
			custom_list[custom_x].add(label);
		}
		cboCustom[custom_x]->setVisible(true);
		cboCustom[custom_x]->setSelected(custom.value);

		// hide the other widgets
		chkCustom[custom_x]->setVisible(false);
	}
	else
	{
		// No Custom field for this item, hide all widgets
		cboCustom[custom_x]->setVisible(false);
		chkCustom[custom_x]->setVisible(false);
	}
}

void update_slaves_list(const std::vector<whdload_slave>& slaves)
{
	slaves_list.clear();
	for (auto& slave : slaves)
	{
		slaves_list.add(slave.filename);
	}
}

void update_selected_slave(const whdload_slave& selected_slave)
{
	int selected = 0;
	for (int i = 0; i < slaves_list.getNumberOfElements(); i++)
	{
		if (slaves_list.getElementAt(i) == selected_slave.filename)
		{
			selected = i;
			break;
		}
	}
	cboSlaves->setSelected(selected);
}

void RefreshPanelWHDLoad()
{
	RefreshWhdListModel();
	AdjustDropDownControls();

	if (!whdload_filename.empty())
	{
		txtGameName->setText(whdload_prefs.game_name);
		txtSubPath->setText(whdload_prefs.sub_path);
		txtVariantUuid->setText(whdload_prefs.variant_uuid);
		txtSlaveCount->setText(std::to_string(whdload_prefs.slave_count));
		txtSlaveDefault->setText(whdload_prefs.slave_default);
		chkSlaveLibraries->setSelected(whdload_prefs.slave_libraries);
		txtCustomText->setText(whdload_prefs.custom);

		update_slaves_list(whdload_prefs.slaves);
		update_selected_slave(whdload_prefs.selected_slave);

		if (!whdload_prefs.selected_slave.data_path.empty())
			txtSlaveDataPath->setText(whdload_prefs.selected_slave.data_path);
		else
			txtSlaveDataPath->setText("");

		update_custom_fields(whdload_prefs.selected_slave.custom1, 0);
		update_custom_fields(whdload_prefs.selected_slave.custom2, 1);
		update_custom_fields(whdload_prefs.selected_slave.custom3, 2);
		update_custom_fields(whdload_prefs.selected_slave.custom4, 3);
		update_custom_fields(whdload_prefs.selected_slave.custom5, 4);
	}

	// These are global
	chkButtonWait->setSelected(whdload_prefs.button_wait);
	chkShowSplash->setSelected(whdload_prefs.show_splash);
	txtConfigDelay->setText(std::to_string(whdload_prefs.config_delay));
	chkWriteCache->setSelected(whdload_prefs.write_cache);
	chkQuitOnExit->setSelected(whdload_prefs.quit_on_exit);
}

bool HelpPanelWHDLoad(std::vector<std::string>& helptext)
{
	helptext.clear();
	//TODO
	return true;
}