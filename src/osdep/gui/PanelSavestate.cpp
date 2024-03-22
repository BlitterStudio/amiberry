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

std::string get_file_timestamp(const std::string& filename)
{
	struct stat st {};
	tm tm{};

	if (stat(filename.c_str(), &st) == -1) {
		write_log("Failed to get file timestamp, stat failed: %s\n", strerror(errno));
		return "ERROR";
	}

	localtime_r(&st.st_mtime, &tm);
	char date_string[256];
	strftime(date_string, sizeof(date_string), "%c", &tm);
	return std::string(date_string);
}

class SavestateActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		const auto it = std::find(radioButtons.begin(), radioButtons.end(), actionEvent.getSource());
		if (it != radioButtons.end())
		{
			current_state_num = std::distance(radioButtons.begin(), it);
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
				}
				if (savestate_state != STATE_DORESTORE)
					ShowMessage("Loading savestate", "Statefile doesn't exist.", "", "", "Ok", "");
			}
			else
				ShowMessage("Loading savestate", "Emulation hasn't started yet.", "", "", "Ok", "");

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
			if (emulating && (!unsafe || unsafe && unsafe_confirmed))
			{
				savestate_initsave(savestate_fname, 1, true, true);
				save_state(savestate_fname, "...");
				savestate_state = STATE_DOSAVE; // Just to create the screenshot
				delay_savestate_frame = 2;
				gui_running = false;
			}
			else
				ShowMessage("Saving state", "Emulation hasn't started yet.", "", "", "Ok", "");

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
	grpNumber->setBaseColor(gui_baseCol);

	grpScreenshot = new gcn::Window("State screenshot");
	grpScreenshot->setMovable(false);
	grpScreenshot->setSize(category.panel->getWidth() - grpNumber->getWidth() - DISTANCE_BORDER * 2 - DISTANCE_NEXT_X, grpNumber->getHeight());
	grpScreenshot->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpScreenshot->setBaseColor(gui_baseCol);

	cmdLoadState = new gcn::Button("Load State");
	cmdLoadState->setSize(BUTTON_WIDTH + 10, BUTTON_HEIGHT);
	cmdLoadState->setBaseColor(gui_baseCol);
	cmdLoadState->setId("cmdLoadState");
	cmdLoadState->addActionListener(savestateActionListener);

	cmdSaveState = new gcn::Button("Save State");
	cmdSaveState->setSize(BUTTON_WIDTH + 10, BUTTON_HEIGHT);
	cmdSaveState->setBaseColor(gui_baseCol);
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

	if (current_state_num >= 0 && current_state_num < radioButtons.size()) {
		radioButtons[current_state_num]->setSelected(true);
	}

	gui_update();

	if (strlen(savestate_fname) > 0)
	{
		auto* const f = fopen(savestate_fname, "rbe");
		if (f) {
			fclose(f);
			lblTimestamp->setCaption(get_file_timestamp(savestate_fname));
		}
		else
		{
			lblTimestamp->setCaption("No savestate found");
		}
	}

	if (screenshot_filename.length() > 0)
	{
		auto* const f = fopen(screenshot_filename.c_str(), "rbe");
		if (f)
		{
			fclose(f);
			const auto rect = grpScreenshot->getChildrenArea();
			auto* loadedImage = IMG_Load(screenshot_filename.c_str());
			if (loadedImage != nullptr)
			{
				SDL_Rect source = {0, 0, 0, 0};
				SDL_Rect target = {0, 0, 0, 0};
				auto* scaled = SDL_CreateRGBSurface(0, rect.width, rect.height,
													loadedImage->format->BitsPerPixel,
													loadedImage->format->Rmask, loadedImage->format->Gmask,
													loadedImage->format->Bmask, loadedImage->format->Amask);
				source.w = loadedImage->w;
				source.h = loadedImage->h;
				target.w = rect.width;
				target.h = rect.height;
				SDL_SoftStretch(loadedImage, &source, scaled, &target);
				SDL_FreeSurface(loadedImage);
				loadedImage = nullptr;
				imgSavestate = new gcn::SDLImage(scaled, true);
				icoSavestate = new gcn::Icon(imgSavestate);
				grpScreenshot->add(icoSavestate);
			}
		}
	}

	for (const auto& radioButton : radioButtons) {
		radioButton->setEnabled(true);
	}

	grpScreenshot->setVisible(true);
	cmdLoadState->setEnabled(true);
	cmdSaveState->setEnabled(true);
}

bool HelpPanelSavestate(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("Savestates are stored with the name of the disk in drive DF0, or if no");
	helptext.emplace_back("disk is inserted, the name of the last loaded .uae config.");
	helptext.emplace_back(" ");
	helptext.emplace_back("When you hold left shoulder button and press 'l' during emulation, ");
	helptext.emplace_back("the state of the last active number will be loaded. Hold left shoulder ");
	helptext.emplace_back("button and press 's' to save the current state in the last active slot.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Note: Savestates may or may not work with HDDs, JIT or RTG. They were");
	helptext.emplace_back("designed to work with floppy disk images.");
	return true;
}
