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
static std::vector<gcn::RadioButton*> optStateSlot(15);
static gcn::Label* lblFilename;
static gcn::Label* lblTimestamp;

static gcn::Window* grpScreenshot;
static gcn::Icon* icoSavestate = nullptr;
static gcn::Image* imgSavestate = nullptr;
static gcn::Button* cmdLoadStateSlot;
static gcn::Button* cmdSaveStateSlot;
static gcn::Button* cmdDeleteStateSlot;
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
		const auto action = actionEvent.getSource();

		for (int i = 0; i < 15; ++i)
		{
			if (action == optStateSlot[i])
			{
				current_state_num = i;
				break;
			}
		}

		if (action == cmdLoadStateSlot)
		{
			//------------------------------------------
			// Load state from selected slot
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

			cmdLoadStateSlot->requestFocus();
		}
		else if (action == cmdSaveStateSlot)
		{
			//------------------------------------------
			// Save current state to selected slot
			//------------------------------------------
			if (emulating)
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

				if (!unsafe || unsafe_confirmed)
				{
					savestate_initsave(savestate_fname, 1, false, true);
					save_state(savestate_fname, "...");
					if (create_screenshot())
						save_thumb(screenshot_filename);
				}
			}
			else
			{
				ShowMessage("Saving state", "Emulation hasn't started yet.", "", "", "Ok", "");
			}

			cmdSaveStateSlot->requestFocus();
		}
		else if (action == cmdDeleteStateSlot)
		{
			//------------------------------------------
			// Delete state from selected slot
			//------------------------------------------
			if (strlen(savestate_fname) > 0)
			{
				if (ShowMessage("Delete savestate", "Do you really want to delete the statefile?", "This will also delete the corresponding screenshot.", "", "Yes", "No"))
				{
					if (remove(savestate_fname) == 0)
					{
						if (!screenshot_filename.empty())
						{
							remove(screenshot_filename.c_str());
						}
						RefreshPanelSavestate();
					}
					else
					{
						ShowMessage("Delete savestate", "Failed to delete statefile.", "", "", "Ok", "");
					}
				}
			}
			else
			{
				ShowMessage("Delete savestate", "No statefile selected.", "", "", "Ok", "");
			}
			cmdDeleteStateSlot->requestFocus();
		}
		else if (action == cmdLoadState)
		{
			// Load state from file
			disk_selection(4, &changed_prefs);
			current_state_num = 99; // Set it to something invalid, so no slot is selected
		}
		else if (action == cmdSaveState)
		{
			// Save current state to file
			if (emulating)
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
				if (!unsafe || unsafe_confirmed)
				{
					disk_selection(5, &changed_prefs);
				}
			}
			else
			{
				ShowMessage("Saving state", "Emulation hasn't started yet.", "", "", "Ok", "");
			}
		}

		RefreshPanelSavestate();
	}
};

static SavestateActionListener* savestateActionListener;

void InitPanelSavestate(const config_category& category)
{
	savestateActionListener = new SavestateActionListener();

	for (int i = 0; i < 15; ++i) {
		optStateSlot[i] = new gcn::RadioButton(std::to_string(i), "radiostategroup");
		optStateSlot[i]->setId("State" + std::to_string(i));
		optStateSlot[i]->setBaseColor(gui_base_color);
		optStateSlot[i]->setBackgroundColor(gui_background_color);
		optStateSlot[i]->setForegroundColor(gui_foreground_color);
		optStateSlot[i]->addActionListener(savestateActionListener);
	}

	lblFilename = new gcn::Label("Filename: ");
	lblTimestamp = new gcn::Label("Thu Aug 23 14:55:02 2001");

	grpNumber = new gcn::Window("Slot");
	int pos_y = 10;
	for (const auto& radioButton : optStateSlot) {
		grpNumber->add(radioButton, 10, pos_y);
		pos_y += radioButton->getHeight() + 12;
	}
	grpNumber->setMovable(false);
	grpNumber->setSize(BUTTON_WIDTH - 20, TITLEBAR_HEIGHT + pos_y);
	grpNumber->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpNumber->setBaseColor(gui_base_color);
	grpNumber->setForegroundColor(gui_foreground_color);

	grpScreenshot = new gcn::Window("State screenshot");
	grpScreenshot->setMovable(false);
	const auto screenshot_width = category.panel->getWidth() - grpNumber->getWidth() - DISTANCE_BORDER * 2 - DISTANCE_NEXT_X;
	const auto screenshot_height = grpNumber->getHeight();
	grpScreenshot->setSize(screenshot_width, screenshot_height);
	grpScreenshot->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpScreenshot->setBaseColor(gui_base_color);
	grpScreenshot->setForegroundColor(gui_foreground_color);

	cmdLoadStateSlot = new gcn::Button("Load from Slot");
	cmdLoadStateSlot->setSize(BUTTON_WIDTH + 35, BUTTON_HEIGHT);
	cmdLoadStateSlot->setBaseColor(gui_base_color);
	cmdLoadStateSlot->setForegroundColor(gui_foreground_color);
	cmdLoadStateSlot->setId("cmdLoadStateSlot");
	cmdLoadStateSlot->addActionListener(savestateActionListener);

	cmdSaveStateSlot = new gcn::Button("Save to Slot");
	cmdSaveStateSlot->setSize(BUTTON_WIDTH + 35, BUTTON_HEIGHT);
	cmdSaveStateSlot->setBaseColor(gui_base_color);
	cmdSaveStateSlot->setForegroundColor(gui_foreground_color);
	cmdSaveStateSlot->setId("cmdSaveStateSlot");
	cmdSaveStateSlot->addActionListener(savestateActionListener);

	cmdDeleteStateSlot = new gcn::Button("Delete Slot");
	cmdDeleteStateSlot->setSize(BUTTON_WIDTH + 35, BUTTON_HEIGHT);
	cmdDeleteStateSlot->setBaseColor(gui_base_color);
	cmdDeleteStateSlot->setForegroundColor(gui_foreground_color);
	cmdDeleteStateSlot->setId("cmdDeleteStateSlot");
	cmdDeleteStateSlot->addActionListener(savestateActionListener);

	cmdLoadState = new gcn::Button("Load state...");
	cmdLoadState->setSize(BUTTON_WIDTH + 35, BUTTON_HEIGHT);
	cmdLoadState->setBaseColor(gui_base_color);
	cmdLoadState->setForegroundColor(gui_foreground_color);
	cmdLoadState->setId("cmdLoadState");
	cmdLoadState->addActionListener(savestateActionListener);

	cmdSaveState = new gcn::Button("Save state...");
	cmdSaveState->setSize(BUTTON_WIDTH + 35, BUTTON_HEIGHT);
	cmdSaveState->setBaseColor(gui_base_color);
	cmdSaveState->setForegroundColor(gui_foreground_color);
	cmdSaveState->setId("cmdSaveState");
	cmdSaveState->addActionListener(savestateActionListener);

	category.panel->add(grpNumber, DISTANCE_BORDER, DISTANCE_BORDER);
	category.panel->add(grpScreenshot, grpNumber->getX() + grpNumber->getWidth() + DISTANCE_NEXT_X, DISTANCE_BORDER);
	category.panel->add(lblFilename, grpScreenshot->getX(), grpScreenshot->getY() + grpScreenshot->getHeight() + DISTANCE_NEXT_Y/2);
	category.panel->add(lblTimestamp, grpScreenshot->getX(), lblFilename->getY() + lblFilename->getHeight() + DISTANCE_NEXT_Y/2);

	pos_y = lblTimestamp->getY() + lblTimestamp->getHeight() + DISTANCE_NEXT_Y/2;
	category.panel->add(cmdLoadStateSlot, grpScreenshot->getX(), pos_y);
	category.panel->add(cmdSaveStateSlot, cmdLoadStateSlot->getX() + cmdLoadStateSlot->getWidth() + DISTANCE_NEXT_X, pos_y);
	category.panel->add(cmdDeleteStateSlot, cmdSaveStateSlot->getX() + cmdSaveStateSlot->getWidth() + DISTANCE_NEXT_X, pos_y);

	pos_y = cmdLoadStateSlot->getY() + cmdLoadStateSlot->getHeight() + DISTANCE_NEXT_Y/2;
	category.panel->add(cmdLoadState, grpScreenshot->getX(), pos_y);
	category.panel->add(cmdSaveState, cmdLoadState->getX() + cmdLoadState->getWidth() + DISTANCE_NEXT_X, pos_y);

	RefreshPanelSavestate();
}

void ExitPanelSavestate()
{
	for (const auto& radioButton : optStateSlot) {
		delete radioButton;
	}
	delete grpNumber;
	delete lblFilename;
	delete lblTimestamp;

	delete imgSavestate;
	imgSavestate = nullptr;
	delete icoSavestate;
	icoSavestate = nullptr;
	delete grpScreenshot;

	delete cmdLoadStateSlot;
	delete cmdSaveStateSlot;
	delete cmdDeleteStateSlot;
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

	if (current_state_num >= 0 && current_state_num < static_cast<int>(optStateSlot.size())) {
		optStateSlot[current_state_num]->setSelected(true);
	}
	else
	{
		for (const auto& radio_button : optStateSlot) {
			radio_button->setSelected(false);
		}
	}

	gui_update();

	if (strlen(savestate_fname) > 0)
	{
		auto* const f = fopen(savestate_fname, "rbe");
		if (f) {
			fclose(f);
			lblFilename->setCaption("Filename: " + extract_filename(std::string(savestate_fname)));
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
			lblFilename->setCaption("No savestate found:");
			lblTimestamp->setCaption(extract_filename(std::string(savestate_fname)));
		}
	}
	else
	{
		lblFilename->setCaption("No savestate loaded");
		lblTimestamp->setCaption("");
	}
	lblFilename->adjustSize();
	lblTimestamp->adjustSize();

	grpScreenshot->setVisible(true);
	cmdLoadStateSlot->setEnabled(strlen(savestate_fname) > 0);
	cmdSaveStateSlot->setEnabled(strlen(savestate_fname) > 0);
	cmdDeleteStateSlot->setEnabled(strlen(savestate_fname) > 0);
}

bool HelpPanelSavestate(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("Savestates");
	helptext.emplace_back(" ");
	helptext.emplace_back("Savestates allow you to save the current state of the emulation to a file, and to");
	helptext.emplace_back("restore it later. This can be useful to save your progress in a game, or to quickly");
	helptext.emplace_back("load a specific configuration. A screenshot of the current state is saved along with");
	helptext.emplace_back("the statefile.");
	helptext.emplace_back(" ");
	helptext.emplace_back("To save a state, click the 'Save to Slot' button. To load a state, click the 'Load from");
	helptext.emplace_back("Slot' button. To delete a state, click the 'Delete Slot' button.");
	helptext.emplace_back(" ");
	helptext.emplace_back("You can also save and load states to and from files. Click the 'Save state...' button");
	helptext.emplace_back("to save the current state to a file, and the 'Load state...' button to load a state");
	helptext.emplace_back("from a file.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Savestates are saved to the 'savestates' directory in the Amiberry data directory.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Note: Savestates will not work when emulating CD32/CDTV machine types, and will likely");
	helptext.emplace_back("fail when the JIT/PPC/RTG emulation options are enabled.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Savestates are stored in a .uss file, with the name being that of the currently loaded");
	helptext.emplace_back("floppy disk image or whdload .lha file, or the name of the loaded HDD .uae config.");
	helptext.emplace_back(" ");
	return true;
}
