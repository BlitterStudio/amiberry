#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include "guisan/sdl/sdltruetypefont.hpp"
#include "SelectorEntry.hpp"
#include "UaeListBox.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "gui.h"
#include "gui_handling.h"

static char last_active_config[MAX_DPATH] = {'\0'};
static int ensureVisible = -1;

static gcn::Button* cmdLoad;
static gcn::Button* cmdSave;
static gcn::Button* cmdLoadFrom;
static gcn::Button* cmdSaveAs;
static gcn::Button* cmdDelete;
static gcn::Label* lblName;
static gcn::TextField* txtName;
static gcn::Label* lblDesc;
static gcn::TextField* txtDesc;
static gcn::UaeListBox* lstConfigs;
static gcn::ScrollArea* scrAreaConfigs;


bool LoadConfigByName(const char* name)
{
	ConfigFileInfo* config = SearchConfigInList(name);
	if (config != nullptr)
	{
		txtName->setText(config->Name);
		txtDesc->setText(config->Description);
		target_cfgfile_load(&changed_prefs, config->FullPath, 0, 0);
		strcpy(last_active_config, config->Name);
		DisableResume();
		RefreshAllPanels();
	}

	return false;
}

void load_builtin_config(int id)
{
	if (changed_prefs.cdslots[0].inuse)
		gui_force_rtarea_hdchange();
	discard_prefs(&changed_prefs, 0);
	default_prefs(&changed_prefs, 0);
	switch (id)
	{
	case BUILTINID_A500:
		built_in_prefs(&changed_prefs, 0, 1, 0, 0);
		break;

	case BUILTINID_A1200:
		built_in_prefs(&changed_prefs, 4, 0, 0, 0);
		break;

	case BUILTINID_CD32:
		built_in_prefs(&changed_prefs, 8, 0, 0, 0);
		break;
	}
}

void SetLastActiveConfig(const char* filename)
{
	extractFileName(filename, last_active_config);
	removeFileExtension(last_active_config);
}


class ConfigsListModel : public gcn::ListModel
{
	vector<string> configs;

public:
	ConfigsListModel()
	{
	}

	int getNumberOfElements() override
	{
		return configs.size();
	}

	string getElementAt(int i) override
	{
		if (i >= configs.size() || i < 0)
			return "---";
		return configs[i];
	}

	void InitConfigsList()
	{
		configs.clear();
		for (int i = 0; i < ConfigFilesList.size(); ++i)
		{
			char tmp[MAX_DPATH];
			strcpy(tmp, ConfigFilesList[i]->Name);
			if (strlen(ConfigFilesList[i]->Description) > 0)
			{
				strncat(tmp, " (", sizeof tmp);
				strncat(tmp, ConfigFilesList[i]->Description, 255);
				strncat(tmp, ")", sizeof tmp);
			}
			configs.push_back(tmp);
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
			if (i != -1)
			{
				if (ConfigFilesList[i]->BuiltInID != BUILTINID_NONE)
				{
					load_builtin_config(ConfigFilesList[i]->BuiltInID);
					strcpy(changed_prefs.description, ConfigFilesList[i]->Description);
				}
				else
				{
					target_cfgfile_load(&changed_prefs, ConfigFilesList[i]->FullPath, 0, 0);
				}
				strcpy(last_active_config, ConfigFilesList[i]->Name);
				DisableResume();
				RefreshAllPanels();
			}
		}
		else if (actionEvent.getSource() == cmdSave)
		{
			//-----------------------------------------------
			// Save current configuration
			//-----------------------------------------------
			char filename[MAX_DPATH];
			if (!txtName->getText().empty())
			{
				fetch_configurationpath(filename, sizeof filename);
				strncat(filename, txtName->getText().c_str(), sizeof filename);
				strncat(filename, ".uae", sizeof filename);
				strncpy(changed_prefs.description, txtDesc->getText().c_str(), sizeof changed_prefs.description);
				if (cfgfile_save(&changed_prefs, filename, 0))
					RefreshPanelConfig();
			}
		}
		else if (actionEvent.getSource() == cmdLoadFrom)
		{
		}
		else if (actionEvent.getSource() == cmdSaveAs)
		{
		}
		else if (actionEvent.getSource() == cmdDelete)
		{
			//-----------------------------------------------
			// Delete selected config
			//-----------------------------------------------
			char msg[MAX_DPATH];
			i = lstConfigs->getSelected();
			if (i >= 0 && ConfigFilesList[i]->BuiltInID == BUILTINID_NONE && strcmp(ConfigFilesList[i]->Name, OPTIONSFILENAME))
			{
				snprintf(msg, sizeof msg, "Do you want to delete '%s' ?", ConfigFilesList[i]->Name);
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
		int selected_item;
		selected_item = lstConfigs->getSelected();
		if (!txtName->getText().compare(ConfigFilesList[selected_item]->Name))
		{
			//-----------------------------------------------
			// Selected same config again -> load and start it
			//-----------------------------------------------
			if (ConfigFilesList[selected_item]->BuiltInID != BUILTINID_NONE)
			{
				load_builtin_config(ConfigFilesList[selected_item]->BuiltInID);
				strcpy(changed_prefs.description, ConfigFilesList[selected_item]->Description);
			}
			else
			{
				target_cfgfile_load(&changed_prefs, ConfigFilesList[selected_item]->FullPath, 0, 0);
			}
			strcpy(last_active_config, ConfigFilesList[selected_item]->Name);
			DisableResume();
			RefreshAllPanels();
		}
		else
		{
			txtName->setText(ConfigFilesList[selected_item]->Name);
			txtDesc->setText(ConfigFilesList[selected_item]->Description);
		}
	}
};

static ConfigsListActionListener* configsListActionListener;

void InitPanelConfig(const struct _ConfigCategory& category)
{
	configButtonActionListener = new ConfigButtonActionListener();

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

	cmdLoadFrom = new gcn::Button("Load From...");
	cmdLoadFrom->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdLoadFrom->setBaseColor(gui_baseCol);
	cmdLoadFrom->setId("CfgLoadFrom");
	cmdLoadFrom->addActionListener(configButtonActionListener);
	cmdLoadFrom->setEnabled(false);

	cmdSaveAs = new gcn::Button("Save As...");
	cmdSaveAs->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdSaveAs->setBaseColor(gui_baseCol);
	cmdSaveAs->setId("CfgSaveAs");
	cmdSaveAs->addActionListener(configButtonActionListener);
	cmdSaveAs->setEnabled(false);

	cmdDelete = new gcn::Button("Delete");
	cmdDelete->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdDelete->setBaseColor(gui_baseCol);
	cmdDelete->setId("CfgDelete");
	cmdDelete->addActionListener(configButtonActionListener);

	int buttonX = DISTANCE_BORDER;
	int buttonY = category.panel->getHeight() - DISTANCE_BORDER - BUTTON_HEIGHT;
	category.panel->add(cmdLoad, buttonX, buttonY);
	buttonX += BUTTON_WIDTH + DISTANCE_NEXT_X;
	category.panel->add(cmdSave, buttonX, buttonY);
	buttonX += BUTTON_WIDTH + 3 * DISTANCE_NEXT_X;
	buttonX += BUTTON_WIDTH + DISTANCE_NEXT_X;
	buttonX = category.panel->getWidth() - DISTANCE_BORDER - BUTTON_WIDTH;
	category.panel->add(cmdDelete, buttonX, buttonY);

	lblName = new gcn::Label("Name:");
	lblName->setSize(lblName->getWidth(), lblName->getHeight());
	lblName->setAlignment(gcn::Graphics::RIGHT);
	txtName = new gcn::TextField();
	txtName->setSize(300, txtName->getHeight());
	txtName->setId("ConfigName");
	txtName->setBackgroundColor(colTextboxBackground);

	lblDesc = new gcn::Label("Description:");
	lblDesc->setSize(lblDesc->getWidth(), lblDesc->getHeight());
	lblDesc->setAlignment(gcn::Graphics::RIGHT);
	txtDesc = new gcn::TextField();
	txtDesc->setSize(300, txtDesc->getHeight());
	txtDesc->setId("ConfigDesc");
	txtDesc->setBackgroundColor(colTextboxBackground);

	ReadConfigFileList();
	configsList = new ConfigsListModel();
	configsList->InitConfigsList();
	configsListActionListener = new ConfigsListActionListener();

	lstConfigs = new gcn::UaeListBox(configsList);
	lstConfigs->setSize(category.panel->getWidth() - 2 * DISTANCE_BORDER - 22, 232);
	lstConfigs->setBackgroundColor(colTextboxBackground);
	lstConfigs->setWrappingEnabled(true);
	lstConfigs->setId("ConfigList");
	lstConfigs->addActionListener(configsListActionListener);

	scrAreaConfigs = new gcn::ScrollArea(lstConfigs);
	scrAreaConfigs->setBorderSize(1);
	scrAreaConfigs->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	scrAreaConfigs->setSize(category.panel->getWidth() - 2 * DISTANCE_BORDER - 2, 252);
	scrAreaConfigs->setScrollbarWidth(20);
	scrAreaConfigs->setBackgroundColor(colTextboxBackground);
	category.panel->add(scrAreaConfigs);

	category.panel->add(lblName, DISTANCE_BORDER, scrAreaConfigs->getY() + scrAreaConfigs->getHeight() + DISTANCE_NEXT_Y);
	category.panel->add(txtName, DISTANCE_BORDER + lblDesc->getWidth() + 8, scrAreaConfigs->getY() + scrAreaConfigs->getHeight() + DISTANCE_NEXT_Y);
	category.panel->add(lblDesc, DISTANCE_BORDER, lblName->getY() + lblName->getHeight() + DISTANCE_NEXT_Y);
	category.panel->add(txtDesc, DISTANCE_BORDER + lblDesc->getWidth() + 8, txtName->getY() + txtName->getHeight() + DISTANCE_NEXT_Y);

	if (strlen(last_active_config) == 0)
		strncpy(last_active_config, OPTIONSFILENAME, MAX_DPATH);
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
	delete cmdLoadFrom;
	delete cmdSaveAs;
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
		for (int i = 0; i < ConfigFilesList.size(); ++i)
		{
			if (!strcmp(ConfigFilesList[i]->Name, txtName->getText().c_str()))
			{
				// Select current entry
				lstConfigs->setSelected(i);
				ensureVisible = i;
				RegisterRefreshFunc(MakeCurrentVisible);
				break;
			}
		}
	}
}
