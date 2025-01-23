#include <cstring>
#include <cstdio>
#include <vector>

#include <guisan.hpp>
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

static gcn::Label* lblVariantUuid;
static gcn::TextField* txtVariantUuid;

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
static gcn::Button* cmdCustomFields;

static gcn::StringListModel whdloadFileList;
static gcn::StringListModel slaves_list;

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
	if (!whdload_prefs.whdload_filename.empty())
	{
		for (auto i = 0; i < static_cast<int>(lstMRUWhdloadList.size()); ++i)
		{
			if (lstMRUWhdloadList[i] == whdload_prefs.whdload_filename)
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
		const auto source = actionEvent.getSource();
		if (source == cboWhdload)
		{
			//---------------------------------------
			// WHDLoad file from list selected
			//---------------------------------------
			if (!bIgnoreListChange)
			{
				const auto idx = cboWhdload->getSelected();

				if (idx < 0)
				{
					whdload_prefs.whdload_filename = "";
				}
				else
				{
					const auto element = get_full_path_from_disk_list(whdloadFileList.getElementAt(idx));
					if (element != whdload_prefs.whdload_filename)
					{
						whdload_prefs.whdload_filename.assign(element);
						lstMRUWhdloadList.erase(lstMRUWhdloadList.begin() + idx);
						lstMRUWhdloadList.insert(lstMRUWhdloadList.begin(), whdload_prefs.whdload_filename);
						bIgnoreListChange = true;
						cboWhdload->setSelected(0);
						bIgnoreListChange = false;
					}
					whdload_auto_prefs(&changed_prefs, whdload_prefs.whdload_filename.c_str());
					if (!last_loaded_config[0])
						set_last_active_config(whdload_prefs.whdload_filename.c_str());
				}
				refresh_all_panels();
			}
		}
		else if (source == cmdWhdloadEject)
		{
			//---------------------------------------
			// Eject WHDLoad file
			//---------------------------------------
			whdload_prefs.whdload_filename = "";
			refresh_all_panels();
		}
		else if (source == cmdWhdloadSelect)
		{
			std::string tmp;
			if (!whdload_prefs.whdload_filename.empty())
				tmp = whdload_prefs.whdload_filename;
			else
				tmp = get_whdload_arch_path();

			tmp = SelectFile("Select WHDLoad LHA file", tmp, whdload_filter);
			if (!tmp.empty())
			{
				whdload_prefs.whdload_filename.assign(tmp);
				add_file_to_mru_list(lstMRUWhdloadList, whdload_prefs.whdload_filename);
				RefreshWhdListModel();
				whdload_auto_prefs(&changed_prefs, whdload_prefs.whdload_filename.c_str());

				AdjustDropDownControls();
			}
			cmdWhdloadSelect->requestFocus();
			refresh_all_panels();
		}
		else if (source == cboSlaves)
		{
			if (cboSlaves->getSelected() >= 0)
			{
				whdload_prefs.selected_slave = whdload_prefs.slaves[cboSlaves->getSelected()];
				txtSlaveDataPath->setText(whdload_prefs.selected_slave.data_path.empty() ? "" : whdload_prefs.selected_slave.data_path);
				create_startup_sequence();
			}
		}
		else if (source == cmdCustomFields)
		{
			ShowCustomFields();
			cmdCustomFields->requestFocus();
		}
		else if (source == chkButtonWait)
		{
			whdload_prefs.button_wait = chkButtonWait->isSelected();
			create_startup_sequence();
		}
		else if (source == chkShowSplash)
		{
			whdload_prefs.show_splash = chkShowSplash->isSelected();
			create_startup_sequence();
		}
		else if (source == chkWriteCache)
		{
			whdload_prefs.write_cache = chkWriteCache->isSelected();
			create_startup_sequence();
		}
		else if (source == chkQuitOnExit)
		{
			whdload_prefs.quit_on_exit = chkQuitOnExit->isSelected();
			create_startup_sequence();
		}
	}
};

static WHDLoadActionListener* whdloadActionListener;

void InitPanelWHDLoad(const struct config_category& category)
{
	slaves_list.clear();

	constexpr int textfield_width = 350;

	whdloadActionListener = new WHDLoadActionListener();

	lblWhdload = new gcn::Label("WHDLoad auto-config:");
	cboWhdload = new gcn::DropDown(&whdloadFileList);
	cboWhdload->setSize(category.panel->getWidth() - 2 * DISTANCE_BORDER, cboWhdload->getHeight());
	cboWhdload->setBaseColor(gui_base_color);
	cboWhdload->setBackgroundColor(gui_background_color);
	cboWhdload->setForegroundColor(gui_foreground_color);
	cboWhdload->setSelectionColor(gui_selection_color);
	cboWhdload->setId("cboWhdload");
	cboWhdload->addActionListener(whdloadActionListener);

	cmdWhdloadEject = new gcn::Button("Eject");
	cmdWhdloadEject->setSize(SMALL_BUTTON_WIDTH * 2, SMALL_BUTTON_HEIGHT);
	cmdWhdloadEject->setBaseColor(gui_base_color);
	cmdWhdloadEject->setForegroundColor(gui_foreground_color);
	cmdWhdloadEject->setId("cmdWhdloadEject");
	cmdWhdloadEject->addActionListener(whdloadActionListener);

	cmdWhdloadSelect = new gcn::Button("Select file");
	cmdWhdloadSelect->setSize(BUTTON_WIDTH + 10, SMALL_BUTTON_HEIGHT);
	cmdWhdloadSelect->setBaseColor(gui_base_color);
	cmdWhdloadSelect->setForegroundColor(gui_foreground_color);
	cmdWhdloadSelect->setId("cmdWhdloadSelect");
	cmdWhdloadSelect->addActionListener(whdloadActionListener);

	// WHDLoad options
	lblGameName = new gcn::Label("Game Name:");
	txtGameName = new gcn::TextField();
	txtGameName->setId("txtGameName");
	txtGameName->setSize(textfield_width, TEXTFIELD_HEIGHT);
	txtGameName->setBaseColor(gui_base_color);
	txtGameName->setBackgroundColor(gui_background_color);
	txtGameName->setForegroundColor(gui_foreground_color);
	txtGameName->setEnabled(false);

	lblVariantUuid = new gcn::Label("UUID:");
	txtVariantUuid = new gcn::TextField();
	txtVariantUuid->setId("txtVariantUuid");
	txtVariantUuid->setSize(textfield_width, TEXTFIELD_HEIGHT);
	txtVariantUuid->setBaseColor(gui_base_color);
	txtVariantUuid->setBackgroundColor(gui_background_color);
	txtVariantUuid->setForegroundColor(gui_foreground_color);
	txtVariantUuid->setEnabled(false);

	lblSlaveDefault = new gcn::Label("Slave Default:");
	txtSlaveDefault = new gcn::TextField();
	txtSlaveDefault->setId("txtSlaveDefault");
	txtSlaveDefault->setSize(textfield_width, TEXTFIELD_HEIGHT);
	txtSlaveDefault->setBaseColor(gui_base_color);
	txtSlaveDefault->setBackgroundColor(gui_background_color);
	txtSlaveDefault->setForegroundColor(gui_foreground_color);
	txtSlaveDefault->setEnabled(false);

	chkSlaveLibraries = new gcn::CheckBox("Slave Libraries");
	chkSlaveLibraries->setId("chkSlaveLibraries");
	chkSlaveLibraries->setBaseColor(gui_base_color);
	chkSlaveLibraries->setBackgroundColor(gui_background_color);
	chkSlaveLibraries->setForegroundColor(gui_foreground_color);

	lblSlaves = new gcn::Label("Slaves:");
	cboSlaves = new gcn::DropDown(&slaves_list);
	cboSlaves->setId("cboSlaves");
	cboSlaves->setSize(textfield_width, cboSlaves->getHeight());
	cboSlaves->setBaseColor(gui_base_color);
	cboSlaves->setBackgroundColor(gui_background_color);
	cboSlaves->setForegroundColor(gui_foreground_color);
	cboSlaves->setSelectionColor(gui_selection_color);
	cboSlaves->addActionListener(whdloadActionListener);

	lblSlaveDataPath = new gcn::Label("Slave Data path:");
	txtSlaveDataPath = new gcn::TextField();
	txtSlaveDataPath->setId("txtSlaveDataPath");
	txtSlaveDataPath->setSize(textfield_width, TEXTFIELD_HEIGHT);
	txtSlaveDataPath->setBaseColor(gui_base_color);
	txtSlaveDataPath->setBackgroundColor(gui_background_color);
	txtSlaveDataPath->setForegroundColor(gui_foreground_color);
	txtSlaveDataPath->setEnabled(false);

	cmdCustomFields = new gcn::Button("Custom Fields");
	cmdCustomFields->setSize(BUTTON_WIDTH * 2, BUTTON_HEIGHT);
	cmdCustomFields->setBaseColor(gui_base_color);
	cmdCustomFields->setForegroundColor(gui_foreground_color);
	cmdCustomFields->setId("cmdCustomFields");
	cmdCustomFields->addActionListener(whdloadActionListener);

	lblCustomText = new gcn::Label("Custom:");
	txtCustomText = new gcn::TextField();
	txtCustomText->setId("txtCustomText");
	txtCustomText->setSize(textfield_width, TEXTFIELD_HEIGHT);
	txtCustomText->setBaseColor(gui_base_color);
	txtCustomText->setBackgroundColor(gui_background_color);
	txtCustomText->setForegroundColor(gui_foreground_color);

	chkButtonWait = new gcn::CheckBox("Button Wait");
	chkButtonWait->setId("chkButtonWait");
	chkButtonWait->setBaseColor(gui_base_color);
	chkButtonWait->setBackgroundColor(gui_background_color);
	chkButtonWait->setForegroundColor(gui_foreground_color);
	chkButtonWait->addActionListener(whdloadActionListener);
	chkShowSplash = new gcn::CheckBox("Show Splash");
	chkShowSplash->setId("chkShowSplash");
	chkShowSplash->setBaseColor(gui_base_color);
	chkShowSplash->setBackgroundColor(gui_background_color);
	chkShowSplash->setForegroundColor(gui_foreground_color);
	chkShowSplash->addActionListener(whdloadActionListener);

	lblConfigDelay = new gcn::Label("Config Delay:");
	txtConfigDelay = new gcn::TextField();
	txtConfigDelay->setId("txtConfigDelay");
	txtConfigDelay->setSize(textfield_width, TEXTFIELD_HEIGHT);
	txtConfigDelay->setBaseColor(gui_base_color);
	txtConfigDelay->setBackgroundColor(gui_background_color);
	txtConfigDelay->setForegroundColor(gui_foreground_color);

	chkWriteCache = new gcn::CheckBox("Write Cache");
	chkWriteCache->setId("chkWriteCache");
	chkWriteCache->setBaseColor(gui_base_color);
	chkWriteCache->setBackgroundColor(gui_background_color);
	chkWriteCache->setForegroundColor(gui_foreground_color);
	chkWriteCache->addActionListener(whdloadActionListener);
	chkQuitOnExit = new gcn::CheckBox("Quit on Exit");
	chkQuitOnExit->setId("chkQuitOnExit");
	chkQuitOnExit->setBaseColor(gui_base_color);
	chkQuitOnExit->setBackgroundColor(gui_background_color);
	chkQuitOnExit->setForegroundColor(gui_foreground_color);
	chkQuitOnExit->addActionListener(whdloadActionListener);

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

	category.panel->add(lblVariantUuid, pos_x1, pos_y);
	category.panel->add(txtVariantUuid, pos_x2, pos_y);
	pos_y += lblVariantUuid->getHeight() + 8;

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

	category.panel->add(cmdCustomFields, pos_x2, pos_y);

	grpWHDLoadGlobal = new gcn::Window("Global options");
	grpWHDLoadGlobal->setMovable(false);
	grpWHDLoadGlobal->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpWHDLoadGlobal->setBaseColor(gui_base_color);
	grpWHDLoadGlobal->setForegroundColor(gui_foreground_color);

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
	grpWHDLoadGlobal->setPosition(pos_x1, category.panel->getHeight() - grpWHDLoadGlobal->getHeight() - DISTANCE_NEXT_Y);
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

	delete lblVariantUuid;
	delete txtVariantUuid;

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
	delete cmdCustomFields;
	delete grpWHDLoadGlobal;

	delete whdloadActionListener;
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

	cmdCustomFields->setEnabled(!whdload_prefs.whdload_filename.empty());

	if (whdload_prefs.whdload_filename.empty())
	{
		clear_whdload_prefs();
		slaves_list.clear();
	}
	else
	{
		update_slaves_list(whdload_prefs.slaves);
		update_selected_slave(whdload_prefs.selected_slave);
		txtSlaveDataPath->setText(whdload_prefs.selected_slave.data_path.empty() ? "" : whdload_prefs.selected_slave.data_path);
	}

	txtGameName->setText(whdload_prefs.game_name.empty() ? "" : whdload_prefs.game_name);
	txtVariantUuid->setText(whdload_prefs.variant_uuid.empty() ? "" : whdload_prefs.variant_uuid);
	txtSlaveDefault->setText(whdload_prefs.slave_default.empty() ? "" : whdload_prefs.slave_default);
	chkSlaveLibraries->setSelected(whdload_prefs.slave_libraries);
	txtCustomText->setText(whdload_prefs.custom.empty() ? "" : whdload_prefs.custom);

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
	helptext.emplace_back("WHDLoad auto-config:");
	helptext.emplace_back("Select a WHDLoad LHA file to auto-configure the emulator for game.");
	helptext.emplace_back("The game name, UUID, default slave, and other options will be parsed from the XML.");
	helptext.emplace_back("You can also select a slave from the list.");
	helptext.emplace_back("The custom fields can be used to add custom options per slave.");
	helptext.emplace_back("The global options are used for all games.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Button Wait: Wait for button press before starting the game.");
	helptext.emplace_back("Show Splash: Show the WHDLoad splash screen.");
	helptext.emplace_back("Config Delay: Delay in seconds before starting the game.");
	helptext.emplace_back("Write Cache: Enable Write cache before starting the game.");
	helptext.emplace_back("Quit on Exit: Quit Amiberry when the game exits.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Note: You must load the same WHDLoad .lha file, to be able to change");
	helptext.emplace_back("the custom/global settings here, when an associated .uae config file");
	helptext.emplace_back("is used. You cannot change these settings by loading the .uae config");
	helptext.emplace_back("file alone - you must first load the .lha file itself, change any");
	helptext.emplace_back("settings, and then save the .uae config file.");
	helptext.emplace_back(" ");
	return true;
}