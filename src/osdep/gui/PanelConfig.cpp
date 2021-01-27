#include <string.h>
#include <stdio.h>

#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "options.h"
#include "uae.h"
#include "gui_handling.h"

static char last_active_config[MAX_DPATH] = {'\0'};
static int ensureVisible = -1;

static gcn::Button* cmdLoad;
static gcn::Button* cmdSave;
static gcn::Button* cmdDelete;
static gcn::Label* lblName;
static gcn::TextField* txtName;
static gcn::Label* lblDesc;
static gcn::TextField* txtDesc;
static gcn::ListBox* lstConfigs;
static gcn::ScrollArea* scrAreaConfigs;

bool LoadConfigByName(const char* name)
{
	auto* config = SearchConfigInList(name);
	if (config != nullptr)
	{
		if (emulating)
		{
			uae_restart(-1, config->FullPath);
		}
		else
		{
			txtName->setText(config->Name);
			txtDesc->setText(config->Description);
			target_cfgfile_load(&changed_prefs, config->FullPath, 0, 0);
			strncpy(last_active_config, config->Name, MAX_DPATH);
			disable_resume();
			refresh_all_panels();
		}
	}

	return false;
}

void SetLastActiveConfig(const char* filename)
{
	extract_filename(filename, last_active_config);
	remove_file_extension(last_active_config);
}

class ConfigsListModel : public gcn::ListModel
{
	std::vector<std::string> configs;

public:
	ConfigsListModel()
	= default;

	int getNumberOfElements() override
	{
		return configs.size();
	}

	std::string getElementAt(int i) override
	{
		if (i >= static_cast<int>(configs.size()) || i < 0)
			return "---";
		return configs[i];
	}

	void InitConfigsList(void)
	{
		configs.clear();
		for (auto& i : ConfigFilesList)
		{
			char tmp[MAX_DPATH];
			strncpy(tmp, i->Name, MAX_DPATH);
			if (strlen(i->Description) > 0)
			{
				strncat(tmp, " (", MAX_DPATH - 1);
				strncat(tmp, i->Description, MAX_DPATH - 3);
				strncat(tmp, ")", MAX_DPATH - 1);
			}
			configs.emplace_back(tmp);
		}
	}
};

static ConfigsListModel* configsList;

class ConfigButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		int i;
		if (actionEvent.getSource() == cmdLoad)
		{
			//-----------------------------------------------
			// Load selected configuration
			//-----------------------------------------------
			i = lstConfigs->getSelected();
			if (i == -1) return;
			
			if (emulating)
			{
				disable_resume();
			}
			
			target_cfgfile_load(&changed_prefs, ConfigFilesList[i]->FullPath, 0, 0);
			strncpy(last_active_config, ConfigFilesList[i]->Name, MAX_DPATH);
			refresh_all_panels();
		}
		else if (actionEvent.getSource() == cmdSave)
		{
			//-----------------------------------------------
			// Save current configuration
			//-----------------------------------------------
			char filename[MAX_DPATH];
			if (!txtName->getText().empty())
			{
				get_configuration_path(filename, MAX_DPATH);
				strncat(filename, txtName->getText().c_str(), MAX_DPATH - 1);
				strncat(filename, ".uae", MAX_DPATH - 1);
				strncpy(changed_prefs.description, txtDesc->getText().c_str(), 256);
				if (cfgfile_save(&changed_prefs, filename, 0))
					RefreshPanelConfig();
			}
		}
		else if (actionEvent.getSource() == cmdDelete)
		{
			//-----------------------------------------------
			// Delete selected config
			//-----------------------------------------------
			char msg[256];
			i = lstConfigs->getSelected();
			if (i >= 0 && strcmp(ConfigFilesList[i]->Name, OPTIONSFILENAME) != 0)
			{
				snprintf(msg, 256, "Do you want to delete '%s' ?", ConfigFilesList[i]->Name);
				if (ShowMessage("Delete Configuration", msg, "", "Yes", "No"))
				{
					remove(ConfigFilesList[i]->FullPath);
					if (!strcmp(last_active_config, ConfigFilesList[i]->Name))
					{
						txtName->setText("");
						txtDesc->setText("");
						last_active_config[0] = '\0';
					}
					ConfigFilesList.erase(ConfigFilesList.begin() + i);
					RefreshPanelConfig();
				}
				cmdDelete->requestFocus();
			}
		}
	}
};

static ConfigButtonActionListener* configButtonActionListener;

class ConfigsListActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		const int selected_item = lstConfigs->getSelected();
		if (selected_item == -1) return;
		
		if (txtName->getText() != ConfigFilesList[selected_item]->Name || txtDesc->getText() != ConfigFilesList[
			selected_item]->Description)
		{
			//-----------------------------------------------
			// Selected a config -> Update Name and Description fields
			//-----------------------------------------------
			txtName->setText(ConfigFilesList[selected_item]->Name);
			txtDesc->setText(ConfigFilesList[selected_item]->Description);
		}
		else
		{
			//-----------------------------------------------
			// Second click on selected config -> Load it and start emulation
			// ----------------------------------------------
			if (emulating)
			{
				disable_resume();
			}
			target_cfgfile_load(&changed_prefs, ConfigFilesList[selected_item]->FullPath, 0, 0);
			strncpy(last_active_config, ConfigFilesList[selected_item]->Name, MAX_DPATH);
			refresh_all_panels();
			
			uae_reset(1, 1);
			gui_running = false;
		}
	}
};

static ConfigsListActionListener* configsListActionListener;

void InitPanelConfig(const struct _ConfigCategory& category)
{
	configButtonActionListener = new ConfigButtonActionListener();

	ReadConfigFileList();
	configsList = new ConfigsListModel();
	configsList->InitConfigsList();
	configsListActionListener = new ConfigsListActionListener();

	lstConfigs = new gcn::ListBox(configsList);
	lstConfigs->setSize(category.panel->getWidth() - 2 * DISTANCE_BORDER - 22, 232);
	lstConfigs->setBaseColor(colTextboxBackground);
	lstConfigs->setBackgroundColor(colTextboxBackground);
	lstConfigs->setSelectionColor(colSelectorActive);
	lstConfigs->setWrappingEnabled(true);
	lstConfigs->setId("ConfigList");
	lstConfigs->addActionListener(configsListActionListener);

	scrAreaConfigs = new gcn::ScrollArea(lstConfigs);
	scrAreaConfigs->setBorderSize(1);
	scrAreaConfigs->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	scrAreaConfigs->setSize(category.panel->getWidth() - 2 * DISTANCE_BORDER - 2, 252);
	scrAreaConfigs->setScrollbarWidth(20);
	scrAreaConfigs->setBackgroundColor(colTextboxBackground);

	lblName = new gcn::Label("Name:");
	lblName->setSize(lblName->getWidth(), lblName->getHeight());
	lblName->setAlignment(gcn::Graphics::RIGHT);
	txtName = new gcn::TextField();
	txtName->setSize(300, TEXTFIELD_HEIGHT);
	txtName->setBackgroundColor(colTextboxBackground);

	lblDesc = new gcn::Label("Description:");
	lblDesc->setSize(lblDesc->getWidth(), lblDesc->getHeight());
	lblDesc->setAlignment(gcn::Graphics::RIGHT);
	txtDesc = new gcn::TextField();
	txtDesc->setSize(300, TEXTFIELD_HEIGHT);
	txtDesc->setBackgroundColor(colTextboxBackground);
	
	category.panel->add(scrAreaConfigs);
	category.panel->add(lblName, DISTANCE_BORDER,
	                    scrAreaConfigs->getY() + scrAreaConfigs->getHeight() + DISTANCE_NEXT_Y);
	category.panel->add(txtName, DISTANCE_BORDER + lblDesc->getWidth() + 8,
	                    scrAreaConfigs->getY() + scrAreaConfigs->getHeight() + DISTANCE_NEXT_Y);
	category.panel->add(lblDesc, DISTANCE_BORDER, txtName->getY() + txtName->getHeight() + DISTANCE_NEXT_Y);
	category.panel->add(txtDesc, DISTANCE_BORDER + lblDesc->getWidth() + 8,
	                    txtName->getY() + txtName->getHeight() + DISTANCE_NEXT_Y);

	cmdLoad = new gcn::Button("Load");
	cmdLoad->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdLoad->setBaseColor(gui_baseCol);
	cmdLoad->setId("ConfigLoad");
	cmdLoad->addActionListener(configButtonActionListener);

	cmdSave = new gcn::Button("Save");
	cmdSave->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdSave->setBaseColor(gui_baseCol);
	cmdSave->setId("ConfigSave");
	cmdSave->addActionListener(configButtonActionListener);

	cmdDelete = new gcn::Button("Delete");
	cmdDelete->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdDelete->setBaseColor(gui_baseCol);
	cmdDelete->setId("CfgDelete");
	cmdDelete->addActionListener(configButtonActionListener);

	auto button_x = DISTANCE_BORDER;
	const auto buttonY = category.panel->getHeight() - DISTANCE_BORDER - BUTTON_HEIGHT;
	category.panel->add(cmdLoad, button_x, buttonY);
	button_x += BUTTON_WIDTH + DISTANCE_NEXT_X;
	category.panel->add(cmdSave, button_x, buttonY);
	button_x += BUTTON_WIDTH + 3 * DISTANCE_NEXT_X;
	button_x += BUTTON_WIDTH + DISTANCE_NEXT_X;
	button_x = category.panel->getWidth() - DISTANCE_BORDER - BUTTON_WIDTH;
	category.panel->add(cmdDelete, button_x, buttonY);
	
	if (strlen(last_active_config) == 0)
	{
		if (strlen(last_loaded_config) == 0)
			strncpy(last_active_config, OPTIONSFILENAME, MAX_DPATH);
		else
		{
			strcpy(last_active_config, last_loaded_config);
			remove_file_extension(last_active_config);
		}
	}
	txtName->setText(last_active_config);
	txtDesc->setText(changed_prefs.description);
	ensureVisible = -1;
	RefreshPanelConfig();
}

void ExitPanelConfig()
{
	delete lstConfigs;
	delete scrAreaConfigs;
	delete configsListActionListener;
	delete configsList;

	delete cmdLoad;
	delete cmdSave;
	delete cmdDelete;

	delete configButtonActionListener;

	delete lblName;
	delete txtName;
	delete lblDesc;
	delete txtDesc;
}

static void MakeCurrentVisible()
{
	if (ensureVisible >= 0)
	{
		scrAreaConfigs->setVerticalScrollAmount(ensureVisible * 19);
		ensureVisible = -1;
	}
}

void RefreshPanelConfig()
{
	ReadConfigFileList();
	configsList->InitConfigsList();

	// Search current entry
	if (!txtName->getText().empty())
	{
		for (auto i = 0; i < ConfigFilesList.size(); ++i)
		{
			if (txtName->getText() == ConfigFilesList[i]->Name)
			{
				// Select current entry
				lstConfigs->setSelected(i);
				ensureVisible = i;
				register_refresh_func(MakeCurrentVisible);
				break;
			}
		}
	}
}

bool HelpPanelConfig(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("To load a configuration, select the entry in the list and then click on \"Load\".");
	helptext.emplace_back("If you double-click on an entry in the list, the emulation starts with this configuration.");
	helptext.emplace_back(" ");
	helptext.emplace_back("If you want to create a new configuration, set all options, enter a new name in");
	helptext.emplace_back(R"("Name", optionally provide a short description and then click on "Save".)");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Delete\" will delete the selected configuration.");
	return true;
}
