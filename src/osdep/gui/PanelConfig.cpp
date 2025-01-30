#include <cstring>
#include <cstdio>

#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"
#include "StringListModel.h"

#include "sysdeps.h"
#include "options.h"
#include "uae.h"
#include "gui_handling.h"

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

static gcn::StringListModel configsList;

static void InitConfigsList()
{
	configsList.clear();
	for (const auto& i : ConfigFilesList)
	{
		char tmp[MAX_DPATH];
		strncpy(tmp, i->Name, MAX_DPATH);
		if (strlen(i->Description) > 0)
		{
			strncat(tmp, " (", MAX_DPATH - 1);
			strncat(tmp, i->Description, MAX_DPATH - 3);
			strncat(tmp, ")", MAX_DPATH - 1);
		}
		configsList.add(tmp);
	}
}

class ConfigActionListener : public gcn::ActionListener
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
			
			target_cfgfile_load(&changed_prefs, ConfigFilesList[i]->FullPath, CONFIG_TYPE_DEFAULT, 0);
			refresh_all_panels();
		}
		else if (actionEvent.getSource() == cmdSave)
		{
			//-----------------------------------------------
			// Save current configuration
			//-----------------------------------------------
			if (!txtName->getText().empty())
			{
				char filename[MAX_DPATH];
				get_configuration_path(filename, MAX_DPATH);
				strncat(filename, txtName->getText().c_str(), MAX_DPATH - 1);
				strncat(filename, ".uae", MAX_DPATH - 1);
				strncpy(changed_prefs.description, txtDesc->getText().c_str(), 256);
				if (cfgfile_save(&changed_prefs, filename, 0))
				{
					strncpy(last_active_config, txtName->getText().c_str(), MAX_DPATH);
					RefreshPanelConfig();
				}
			}
		}
		else if (actionEvent.getSource() == cmdDelete)
		{
			//-----------------------------------------------
			// Delete selected config
			//-----------------------------------------------
			i = lstConfigs->getSelected();
			if (i >= 0 && ConfigFilesList[i]->Name[0] != '\0')
			{
				char msg[256];
				(void)snprintf(msg, 256, "Do you want to delete '%s' ?", ConfigFilesList[i]->Name);
				if (ShowMessage("Delete Configuration", msg, "", "", "Yes", "No"))
				{
					if (remove(ConfigFilesList[i]->FullPath) != 0)
					{
						(void)snprintf(msg, 256, "Failed to delete '%s'", ConfigFilesList[i]->Name);
						ShowMessage("Delete Configuration", msg, "", "", "Ok", "");
					}
					txtName->setText("");
					txtDesc->setText("");
					last_active_config[0] = '\0';
					ConfigFilesList.erase(ConfigFilesList.begin() + i);
					RefreshPanelConfig();
				}
				cmdDelete->requestFocus();
			}
		}
	}
};

static ConfigActionListener* configActionListener;

static Uint32 last_click_time = 0;
class ConfigsListActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		const int selected_item = lstConfigs->getSelected();
		if (selected_item == -1) return;
		const Uint32 current_time = SDL_GetTicks();

		if (txtName->getText() != ConfigFilesList[selected_item]->Name || txtDesc->getText() != ConfigFilesList[
			selected_item]->Description)
		{
			//-----------------------------------------------
			// Selected a config -> Update Name and Description fields
			//-----------------------------------------------
			txtName->setText(ConfigFilesList[selected_item]->Name);
			txtDesc->setText(ConfigFilesList[selected_item]->Description);
		}

		//-----------------------------------------------
		// Double click on selected config -> Load it and start emulation
		// ----------------------------------------------
		if (current_time - last_click_time <= 500)
		{
			last_click_time = current_time;
			if (emulating)
			{
				disable_resume();
			}
			target_cfgfile_load(&changed_prefs, ConfigFilesList[selected_item]->FullPath, CONFIG_TYPE_DEFAULT, 0);
			strncpy(last_active_config, ConfigFilesList[selected_item]->Name, MAX_DPATH);
			refresh_all_panels();

			uae_reset(1, 1);
			gui_running = false;
		}
		last_click_time = current_time;
	}
};

static ConfigsListActionListener* configsListActionListener;

void InitPanelConfig(const struct config_category& category)
{
	configActionListener = new ConfigActionListener();
	configsListActionListener = new ConfigsListActionListener();

	lblName = new gcn::Label("Name:");
	lblName->setSize(lblName->getWidth(), lblName->getHeight());
	lblName->setAlignment(gcn::Graphics::Right);
	txtName = new gcn::TextField();
	txtName->setSize(300, TEXTFIELD_HEIGHT);
	txtName->setBaseColor(gui_base_color);
	txtName->setBackgroundColor(gui_background_color);
	txtName->setForegroundColor(gui_foreground_color);

	lblDesc = new gcn::Label("Description:");
	lblDesc->setSize(lblDesc->getWidth(), lblDesc->getHeight());
	lblDesc->setAlignment(gcn::Graphics::Right);
	txtDesc = new gcn::TextField();
	txtDesc->setSize(300, TEXTFIELD_HEIGHT);
	txtDesc->setBaseColor(gui_base_color);
	txtDesc->setBackgroundColor(gui_background_color);
	txtDesc->setForegroundColor(gui_foreground_color);

	cmdLoad = new gcn::Button("Load");
	cmdLoad->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdLoad->setBaseColor(gui_base_color);
	cmdLoad->setForegroundColor(gui_foreground_color);
	cmdLoad->setId("ConfigLoad");
	cmdLoad->addActionListener(configActionListener);

	cmdSave = new gcn::Button("Save");
	cmdSave->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdSave->setBaseColor(gui_base_color);
	cmdSave->setForegroundColor(gui_foreground_color);
	cmdSave->setId("ConfigSave");
	cmdSave->addActionListener(configActionListener);

	cmdDelete = new gcn::Button("Delete");
	cmdDelete->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdDelete->setBaseColor(gui_base_color);
	cmdDelete->setForegroundColor(gui_foreground_color);
	cmdDelete->setId("CfgDelete");
	cmdDelete->addActionListener(configActionListener);

	const int list_width = category.panel->getWidth() - 2 * DISTANCE_BORDER - SCROLLBAR_WIDTH - 2;
	const int list_height = category.panel->getHeight() - 2 * DISTANCE_BORDER - 2 * lblName->getHeight() - 3 * DISTANCE_NEXT_Y - 2 * BUTTON_HEIGHT;
	lstConfigs = new gcn::ListBox(&configsList);
	lstConfigs->setSize(list_width, list_height);
	lstConfigs->setBaseColor(gui_base_color);
	lstConfigs->setBackgroundColor(gui_background_color);
	lstConfigs->setForegroundColor(gui_foreground_color);
	lstConfigs->setSelectionColor(gui_selection_color);
	lstConfigs->setWrappingEnabled(true);
	lstConfigs->setId("ConfigList");
	lstConfigs->addActionListener(configsListActionListener);

	scrAreaConfigs = new gcn::ScrollArea(lstConfigs);
	scrAreaConfigs->setFrameSize(1);
	scrAreaConfigs->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	scrAreaConfigs->setSize(lstConfigs->getWidth() + SCROLLBAR_WIDTH, lstConfigs->getHeight() + DISTANCE_NEXT_Y);
	scrAreaConfigs->setScrollbarWidth(SCROLLBAR_WIDTH);
	scrAreaConfigs->setBackgroundColor(gui_background_color);
	scrAreaConfigs->setForegroundColor(gui_foreground_color);
	scrAreaConfigs->setBaseColor(gui_base_color);
	scrAreaConfigs->setSelectionColor(gui_selection_color);

	category.panel->add(scrAreaConfigs);
	category.panel->add(lblName, DISTANCE_BORDER, scrAreaConfigs->getY() + scrAreaConfigs->getHeight() + DISTANCE_NEXT_Y);
	category.panel->add(txtName, DISTANCE_BORDER + lblDesc->getWidth() + 8, lblName->getY());
	category.panel->add(lblDesc, DISTANCE_BORDER, txtName->getY() + txtName->getHeight() + DISTANCE_NEXT_Y);
	category.panel->add(txtDesc, DISTANCE_BORDER + lblDesc->getWidth() + 8, lblDesc->getY());

	int button_x = DISTANCE_BORDER;
	const auto buttonY = category.panel->getHeight() - DISTANCE_BORDER - BUTTON_HEIGHT;
	category.panel->add(cmdLoad, button_x, buttonY);
	button_x += BUTTON_WIDTH + DISTANCE_NEXT_X;
	category.panel->add(cmdSave, button_x, buttonY);
	button_x += BUTTON_WIDTH + 3 * DISTANCE_NEXT_X;
	button_x += BUTTON_WIDTH + DISTANCE_NEXT_X;
	button_x = category.panel->getWidth() - DISTANCE_BORDER - BUTTON_WIDTH;
	category.panel->add(cmdDelete, button_x, buttonY);

	ensureVisible = -1;
	RefreshPanelConfig();
}

void ExitPanelConfig()
{
	delete lstConfigs;
	delete scrAreaConfigs;
	delete configsListActionListener;

	delete cmdLoad;
	delete cmdSave;
	delete cmdDelete;

	delete configActionListener;

	delete lblName;
	delete txtName;
	delete lblDesc;
	delete txtDesc;
}

static void MakeCurrentVisible()
{
	if (ensureVisible >= 0)
	{
		scrAreaConfigs->setVerticalScrollAmount(ensureVisible * 14);
		ensureVisible = -1;
	}
}

void RefreshPanelConfig()
{
	ReadConfigFileList();
	InitConfigsList();

	if (last_active_config[0])
	{
		txtName->setText(last_active_config);
		txtDesc->setText(changed_prefs.description);
	}

	// Search current entry
	if (!txtName->getText().empty())
	{
		for (auto i = 0; i < ConfigFilesList.size(); ++i)
		{
			if (txtName->getText() == ConfigFilesList[i]->Name)
			{
				// Select current entry
				lstConfigs->setSelected(i);
				txtDesc->setText(ConfigFilesList[i]->Description);
				ensureVisible = i;
				MakeCurrentVisible();
				break;
			}
		}
	}
	else if (configsList.getNumberOfElements() > 0)
	{
		lstConfigs->setSelected(0);
	}
}

bool HelpPanelConfig(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("In this panel, you can see a list of all your previously saved configurations. The");
	helptext.emplace_back("Configuration file (.uae) contains all the emulator settings available in it. Loading");
	helptext.emplace_back("such a file, will apply those settings to Amiberry immediately. Accordingly, you can");
	helptext.emplace_back("Save your current settings in a file here, for future use.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Please note the \"default\" config name is special for Amiberry, since if it exists,");
	helptext.emplace_back("it will be loaded automatically on startup. This will override the emulator options");
	helptext.emplace_back("Amiberry sets internally at startup, and this may impact on compatibility when using");
	helptext.emplace_back("the Quickstart panel.");
	helptext.emplace_back(" ");
	helptext.emplace_back("To load a configuration, select the entry in the list, and then click on the \"Load\"");
	helptext.emplace_back("button. Note that if you double-click on an entry in the list, the emulation starts");
	helptext.emplace_back("immediately using that configuration.");
	helptext.emplace_back(" ");
	helptext.emplace_back("To create/save a new configuration, set all emulator options as required, then enter");
	helptext.emplace_back(R"(a new "Name", optionally provide a short description, and then click on the "Save")");
	helptext.emplace_back("button. When trying to Save a configuration, if the supplied filename already exists,");
	helptext.emplace_back("it will be automatically renamed to \"configuration.backup\", to keep as a backup.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Please note a special case exists when creating/saving a configuration file for use ");
	helptext.emplace_back("with floppy disk images and whdload archives. The auto-config logic in Amiberry will");
	helptext.emplace_back("scan for a configuration file of the same \"Name\" as the disk image or .lha archive");
	helptext.emplace_back("being loaded. After you load a floppy disk image or whdload archive, and Start the ");
	helptext.emplace_back(R"(emulation, you can use the "F12" key to show the GUI, and in this panel the "Name")");
	helptext.emplace_back("field for the configuration will be filled correctly. Do not change this, as it will");
	helptext.emplace_back("stop auto-config from working. You may change the description if you desire.");
	helptext.emplace_back(" ");
	helptext.emplace_back("To delete the currently selected configuration file from the disk (and the list),");
	helptext.emplace_back("click on the \"Delete\" button.");

	helptext.emplace_back(" ");
	return true;
}
