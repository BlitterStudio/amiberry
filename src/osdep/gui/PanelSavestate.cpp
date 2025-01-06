#include <algorithm>
#include <cstring>
#include <cstdio>
#include <sys/stat.h>
#include <ctime>

#include <guisan.hpp>
#include <SDL_image.h>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "gui.h"
#include "savestate.h"
#include "gui_handling.h"

int current_state_num = 0;

static gcn::Window* grpNumber;
static std::vector<gcn::RadioButton*> radioButtons(15);
static gcn::Label* lblTimestamp;

static gcn::Window* grpScreenshot;
static gcn::Icon* icoSavestate = nullptr;
static gcn::Image* imgSavestate = nullptr;
static gcn::Button* cmdLoadState;
static gcn::Button* cmdSaveState;

static std::string get_file_timestamp(const TCHAR* filename)
{
	struct stat st {};
	tm tm{};

	if (stat(filename, &st) == -1) {
		write_log("Failed to get file timestamp, stat failed: %s\n", strerror(errno));
		return "ERROR";
	}

	localtime_r(&st.st_mtime, &tm);
	char date_string[256];
	strftime(date_string, sizeof(date_string), "%c", &tm);
	return {date_string};
}

class SavestateActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		const auto it = std::find(radioButtons.begin(), radioButtons.end(), actionEvent.getSource());
		if (it != radioButtons.end())
		{
			current_state_num = static_cast<int>(std::distance(radioButtons.begin(), it));
		}
		else if (actionEvent.getSource() == cmdLoadState)
		{
			//------------------------------------------
			// Load state
			//------------------------------------------
			if (emulating)
			{
				if (strlen(savestate_fname) > 0)
				{
					auto* const f = fopen(savestate_fname, "rbe");
					if (f)
					{
						fclose(f);
						savestate_initsave(savestate_fname, 1, true, true);
						savestate_state = STATE_DORESTORE;
						gui_running = false;
					}
					else
					{
						ShowMessage("Loading savestate", "Statefile doesn't exist.", "", "", "Ok", "");
					}
				}
			}
			else
			{
				ShowMessage("Loading savestate", "Emulation hasn't started yet.", "", "", "Ok", "");
			}

			cmdLoadState->requestFocus();
		}
		else if (actionEvent.getSource() == cmdSaveState)
		{
			bool unsafe = false;
			bool unsafe_confirmed = false;
			const AmigaMonitor* mon = &AMonitors[0];
			// Check if we have RTG, then it might fail saving
			if (mon->screen_is_picasso)
			{
				unsafe = true;
				unsafe_confirmed = ShowMessage("Warning: P96 detected", "P96 is enabled. Savestates might be unsafe! Proceed anyway?", "", "", "Proceed", "Cancel");
			}
			// Check if we have JIT enabled
			if (changed_prefs.cachesize > 0)
			{
				unsafe = true;
				unsafe_confirmed = ShowMessage("Warning: JIT detected", "JIT is enabled. Savestates might be unsafe! Proceed anyway?", "", "", "Proceed", "Cancel");
			}
			//------------------------------------------
			// Save current state
			//------------------------------------------
			if (emulating && (!unsafe || unsafe_confirmed))
			{
				savestate_initsave(savestate_fname, 1, true, true);
				save_state(savestate_fname, "...");
				if (create_screenshot())
					save_thumb(screenshot_filename);
			}
			else
			{
				ShowMessage("Saving state", "Emulation hasn't started yet.", "", "", "Ok", "");
			}

			cmdSaveState->requestFocus();
		}

		RefreshPanelSavestate();
	}
};

static SavestateActionListener* savestateActionListener;

void InitPanelSavestate(const config_category& category)
{
	savestateActionListener = new SavestateActionListener();

	for (int i = 0; i < 15; ++i) {
		radioButtons[i] = new gcn::RadioButton(std::to_string(i), "radiostategroup");
		radioButtons[i]->setId("State" + std::to_string(i));
		radioButtons[i]->setBaseColor(gui_base_color);
		radioButtons[i]->setBackgroundColor(gui_background_color);
		radioButtons[i]->setForegroundColor(gui_foreground_color);
		radioButtons[i]->addActionListener(savestateActionListener);
	}

	lblTimestamp = new gcn::Label("Thu Aug 23 14:55:02 2001");

	grpNumber = new gcn::Window("Slot");
	int pos_y = 10;
	for (const auto& radioButton : radioButtons) {
		grpNumber->add(radioButton, 10, pos_y);
		pos_y += radioButton->getHeight() + DISTANCE_NEXT_Y;
	}
	grpNumber->setMovable(false);
	grpNumber->setSize(BUTTON_WIDTH - 20, TITLEBAR_HEIGHT + pos_y);
	grpNumber->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpNumber->setBaseColor(gui_base_color);
	grpNumber->setForegroundColor(gui_foreground_color);

	grpScreenshot = new gcn::Window("State screenshot");
	grpScreenshot->setMovable(false);
	grpScreenshot->setSize(category.panel->getWidth() - grpNumber->getWidth() - DISTANCE_BORDER * 2 - DISTANCE_NEXT_X, grpNumber->getHeight());
	grpScreenshot->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpScreenshot->setBaseColor(gui_base_color);
	grpScreenshot->setForegroundColor(gui_foreground_color);

	cmdLoadState = new gcn::Button("Load State");
	cmdLoadState->setSize(BUTTON_WIDTH + 10, BUTTON_HEIGHT);
	cmdLoadState->setBaseColor(gui_base_color);
	cmdLoadState->setForegroundColor(gui_foreground_color);
	cmdLoadState->setId("cmdLoadState");
	cmdLoadState->addActionListener(savestateActionListener);

	cmdSaveState = new gcn::Button("Save State");
	cmdSaveState->setSize(BUTTON_WIDTH + 10, BUTTON_HEIGHT);
	cmdSaveState->setBaseColor(gui_base_color);
	cmdSaveState->setForegroundColor(gui_foreground_color);
	cmdSaveState->setId("cmdSaveState");
	cmdSaveState->addActionListener(savestateActionListener);

	category.panel->add(grpNumber, DISTANCE_BORDER, DISTANCE_BORDER);
	category.panel->add(grpScreenshot, grpNumber->getX() + grpNumber->getWidth() + DISTANCE_NEXT_X, DISTANCE_BORDER);
	category.panel->add(lblTimestamp, grpScreenshot->getX(), grpScreenshot->getY() + grpScreenshot->getHeight() + DISTANCE_NEXT_Y);
	pos_y = lblTimestamp->getY() + lblTimestamp->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(cmdLoadState, grpScreenshot->getX(), pos_y);
	category.panel->add(cmdSaveState, cmdLoadState->getX() + cmdLoadState->getWidth() + DISTANCE_NEXT_X, pos_y);

	RefreshPanelSavestate();
}

void ExitPanelSavestate()
{
	for (const auto& radioButton : radioButtons) {
		delete radioButton;
	}
	delete grpNumber;
	delete lblTimestamp;

	delete imgSavestate;
	imgSavestate = nullptr;
	delete icoSavestate;
	icoSavestate = nullptr;
	delete grpScreenshot;

	delete cmdLoadState;
	delete cmdSaveState;

	delete savestateActionListener;
}

void RefreshPanelSavestate()
{
	if (icoSavestate != nullptr)
	{
		grpScreenshot->remove(icoSavestate);
		delete icoSavestate;
		icoSavestate = nullptr;
	}
	if (imgSavestate != nullptr)
	{
		delete imgSavestate;
		imgSavestate = nullptr;
	}

	if (current_state_num >= 0 && current_state_num < static_cast<int>(radioButtons.size())) {
		radioButtons[current_state_num]->setSelected(true);
	}

	gui_update();

	if (strlen(savestate_fname) > 0)
	{
		auto* const f = fopen(savestate_fname, "rbe");
		if (f) {
			fclose(f);
			lblTimestamp->setCaption(get_file_timestamp(savestate_fname));

			if (!screenshot_filename.empty())
			{
				auto* const screenshot_file = fopen(screenshot_filename.c_str(), "rbe");
				if (screenshot_file)
				{
					fclose(screenshot_file);
					const auto rect = grpScreenshot->getChildrenArea();
					auto* loaded_image = IMG_Load(screenshot_filename.c_str());
					if (loaded_image != nullptr)
					{
						const SDL_Rect source = { 0, 0, loaded_image->w, loaded_image->h };
						const SDL_Rect target = { 0, 0, rect.width, rect.height };
						auto* scaled = SDL_CreateRGBSurface(0, rect.width, rect.height,
							loaded_image->format->BitsPerPixel,
							loaded_image->format->Rmask, loaded_image->format->Gmask,
							loaded_image->format->Bmask, loaded_image->format->Amask);
						SDL_SoftStretch(loaded_image, &source, scaled, &target);
						SDL_FreeSurface(loaded_image);
						imgSavestate = new gcn::SDLImage(scaled, true);
						icoSavestate = new gcn::Icon(imgSavestate);
						grpScreenshot->add(icoSavestate);
					}
				}
			}
		}
		else
		{
			lblTimestamp->setCaption("No savestate found: " + extract_filename(std::string(savestate_fname)));
		}
	}
	else
	{
		lblTimestamp->setCaption("No savestate loaded");
	}
	lblTimestamp->adjustSize();

	for (const auto& radioButton : radioButtons) {
		radioButton->setEnabled(true);
	}

	grpScreenshot->setVisible(true);
	cmdLoadState->setEnabled(strlen(savestate_fname) > 0);
	cmdSaveState->setEnabled(strlen(savestate_fname) > 0);
}

bool HelpPanelSavestate(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("Savestates can be used with floppy disk image files, whdload.lha files, and with HDD");
	helptext.emplace_back("emulation setups.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Note: Savestates will not work when emulating CD32/CDTV machine types, and will likely");
	helptext.emplace_back("fail when the JIT/PPC/RTG emulation options are enabled.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Savestates are stored in a .uss file, with the name being that of the currently loaded");
	helptext.emplace_back("floppy disk image or whdload.lha file, or the name of the loaded HDD .uae config.");
	helptext.emplace_back(" ");
	helptext.emplace_back("For more information about Savestates, please read the related Amiberry Wiki page.");
	helptext.emplace_back(" ");
	return true;
}
