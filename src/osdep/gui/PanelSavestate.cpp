#include <cstring>
#include <cstdio>

#include <guisan.hpp>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "gui.h"
#include "savestate.h"
#include "gui_handling.h"

int currentStateNum = 0;

static gcn::Window* grpNumber;
static gcn::RadioButton* optState0;
static gcn::RadioButton* optState1;
static gcn::RadioButton* optState2;
static gcn::RadioButton* optState3;
static gcn::RadioButton* optState4;
static gcn::RadioButton* optState5;
static gcn::RadioButton* optState6;
static gcn::RadioButton* optState7;
static gcn::RadioButton* optState8;
static gcn::RadioButton* optState9;
static gcn::RadioButton* optState10;
static gcn::RadioButton* optState11;
static gcn::RadioButton* optState12;
static gcn::RadioButton* optState13;
static gcn::RadioButton* optState14;
static gcn::Label* lblTimestamp;

static gcn::Window* grpScreenshot;
static gcn::Icon* icoSavestate = nullptr;
static gcn::Image* imgSavestate = nullptr;
static gcn::Button* cmdLoadState;
static gcn::Button* cmdSaveState;

char* getts(char *filename1, char *date_string, size_t date_string_size)
{
	struct stat st{};
	tm tm{};

	stat(filename1, &st);
	localtime_r(&st.st_mtime, &tm);
	strftime(date_string, date_string_size,"%c" , &tm);
	return date_string;
}

class SavestateActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == optState0)
			currentStateNum = 0;
		else if (actionEvent.getSource() == optState1)
			currentStateNum = 1;
		else if (actionEvent.getSource() == optState2)
			currentStateNum = 2;
		else if (actionEvent.getSource() == optState3)
			currentStateNum = 3;
		else if (actionEvent.getSource() == optState4)
			currentStateNum = 4;
		else if (actionEvent.getSource() == optState5)
			currentStateNum = 5;
		else if (actionEvent.getSource() == optState6)
			currentStateNum = 6;
		else if (actionEvent.getSource() == optState7)
			currentStateNum = 7;
		else if (actionEvent.getSource() == optState8)
			currentStateNum = 8;
		else if (actionEvent.getSource() == optState9)
			currentStateNum = 9;
		else if (actionEvent.getSource() == optState10)
			currentStateNum = 10;
		else if (actionEvent.getSource() == optState11)
			currentStateNum = 11;
		else if (actionEvent.getSource() == optState12)
			currentStateNum = 12;
		else if (actionEvent.getSource() == optState13)
			currentStateNum = 13;
		else if (actionEvent.getSource() == optState14)
			currentStateNum = 14;
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
					ShowMessage("Loading savestate", "Statefile doesn't exist.", "", "Ok", "");
			}
			else
				ShowMessage("Loading savestate", "Emulation hasn't started yet.", "", "Ok", "");

			cmdLoadState->requestFocus();
		}
		else if (actionEvent.getSource() == cmdSaveState)
		{
			bool unsafe = false;
			bool unsafe_confirmed = false;
			AmigaMonitor* mon = &AMonitors[0];
			// Check if we have RTG, then it might fail saving
			if (mon->screen_is_picasso)
			{
				unsafe = true;
				unsafe_confirmed = ShowMessage("Warning: P96 detected", "P96 is enabled. Savestates might be unsafe! Proceed anyway?", "", "Proceed", "Cancel");
			}
			// Check if we have JIT enabled
			if (changed_prefs.cachesize > 0)
			{
				unsafe = true;
				unsafe_confirmed = ShowMessage("Warning: JIT detected", "JIT is enabled. Savestates might be unsafe! Proceed anyway?", "", "Proceed", "Cancel");
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
				ShowMessage("Saving state", "Emulation hasn't started yet.", "", "Ok", "");

			cmdSaveState->requestFocus();
		}

		RefreshPanelSavestate();
	}
};

static SavestateActionListener* savestateActionListener;

void InitPanelSavestate(const config_category& category)
{
	savestateActionListener = new SavestateActionListener();

	optState0 = new gcn::RadioButton("0", "radiostategroup");
	optState0->setId("State0");
	optState0->addActionListener(savestateActionListener);

	optState1 = new gcn::RadioButton("1", "radiostategroup");
	optState1->setId("State1");
	optState1->addActionListener(savestateActionListener);

	optState2 = new gcn::RadioButton("2", "radiostategroup");
	optState2->setId("State2");
	optState2->addActionListener(savestateActionListener);

	optState3 = new gcn::RadioButton("3", "radiostategroup");
	optState3->setId("State3");
	optState3->addActionListener(savestateActionListener);

	optState4 = new gcn::RadioButton("4", "radiostategroup");
	optState4->setId("State4");
	optState4->addActionListener(savestateActionListener);

	optState5 = new gcn::RadioButton("5", "radiostategroup");
	optState5->setId("State5");
	optState5->addActionListener(savestateActionListener);

	optState6 = new gcn::RadioButton("6", "radiostategroup");
	optState6->setId("State6");
	optState6->addActionListener(savestateActionListener);

	optState7 = new gcn::RadioButton("7", "radiostategroup");
	optState7->setId("State7");
	optState7->addActionListener(savestateActionListener);

	optState8 = new gcn::RadioButton("8", "radiostategroup");
	optState8->setId("State8");
	optState8->addActionListener(savestateActionListener);

	optState9 = new gcn::RadioButton("9", "radiostategroup");
	optState9->setId("State9");
	optState9->addActionListener(savestateActionListener);

	optState10 = new gcn::RadioButton("10", "radiostategroup");
	optState10->setId("State10");
	optState10->addActionListener(savestateActionListener);

	optState11 = new gcn::RadioButton("11", "radiostategroup");
	optState11->setId("State11");
	optState11->addActionListener(savestateActionListener);

	optState12 = new gcn::RadioButton("12", "radiostategroup");
	optState12->setId("State12");
	optState12->addActionListener(savestateActionListener);

	optState13 = new gcn::RadioButton("13", "radiostategroup");
	optState13->setId("State13");
	optState13->addActionListener(savestateActionListener);

	optState14 = new gcn::RadioButton("14", "radiostategroup");
	optState14->setId("State14");
	optState14->addActionListener(savestateActionListener);

	lblTimestamp = new gcn::Label("Thu Aug 23 14:55:02 2001");

	grpNumber = new gcn::Window("Number");
	grpNumber->add(optState0, 10, 10);
	grpNumber->add(optState1, optState0->getX(), optState0->getY() + optState0->getHeight() + DISTANCE_NEXT_Y);
	grpNumber->add(optState2, optState0->getX(), optState1->getY() + optState1->getHeight() + DISTANCE_NEXT_Y);
	grpNumber->add(optState3, optState0->getX(), optState2->getY() + optState2->getHeight() + DISTANCE_NEXT_Y);
	grpNumber->add(optState4, optState0->getX(), optState3->getY() + optState3->getHeight() + DISTANCE_NEXT_Y);
	grpNumber->add(optState5, optState0->getX(), optState4->getY() + optState4->getHeight() + DISTANCE_NEXT_Y);
	grpNumber->add(optState6, optState0->getX(), optState5->getY() + optState5->getHeight() + DISTANCE_NEXT_Y);
	grpNumber->add(optState7, optState0->getX(), optState6->getY() + optState6->getHeight() + DISTANCE_NEXT_Y);
	grpNumber->add(optState8, optState0->getX(), optState7->getY() + optState7->getHeight() + DISTANCE_NEXT_Y);
	grpNumber->add(optState9, optState0->getX(), optState8->getY() + optState8->getHeight() + DISTANCE_NEXT_Y);
	grpNumber->add(optState10, optState0->getX(), optState9->getY() + optState9->getHeight() + DISTANCE_NEXT_Y);
	grpNumber->add(optState11, optState0->getX(), optState10->getY() + optState10->getHeight() + DISTANCE_NEXT_Y);
	grpNumber->add(optState12, optState0->getX(), optState11->getY() + optState11->getHeight() + DISTANCE_NEXT_Y);
	grpNumber->add(optState13, optState0->getX(), optState12->getY() + optState12->getHeight() + DISTANCE_NEXT_Y);
	grpNumber->add(optState14, optState0->getX(), optState13->getY() + optState13->getHeight() + DISTANCE_NEXT_Y);
	grpNumber->setMovable(false);
	grpNumber->setSize(BUTTON_WIDTH, optState14->getY() + TITLEBAR_HEIGHT * 2 + DISTANCE_NEXT_Y);
	grpNumber->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpNumber->setBaseColor(gui_baseCol);

	grpScreenshot = new gcn::Window("State screen");
	grpScreenshot->setMovable(false);
	grpScreenshot->setSize(320, 320);
	grpScreenshot->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpScreenshot->setBaseColor(gui_baseCol);

	cmdLoadState = new gcn::Button("Load State");
	cmdLoadState->setSize(BUTTON_WIDTH + 10, BUTTON_HEIGHT);
	cmdLoadState->setBaseColor(gui_baseCol);
	cmdLoadState->setId("LoadState");
	cmdLoadState->addActionListener(savestateActionListener);

	cmdSaveState = new gcn::Button("Save State");
	cmdSaveState->setSize(BUTTON_WIDTH + 10, BUTTON_HEIGHT);
	cmdSaveState->setBaseColor(gui_baseCol);
	cmdSaveState->setId("SaveState");
	cmdSaveState->addActionListener(savestateActionListener);

	category.panel->add(grpNumber, DISTANCE_BORDER, DISTANCE_BORDER);
	category.panel->add(grpScreenshot, grpNumber->getX() + grpNumber->getWidth() + DISTANCE_NEXT_X, DISTANCE_BORDER);
	category.panel->add(lblTimestamp, grpScreenshot->getX(), grpScreenshot->getY() + grpScreenshot->getHeight() + DISTANCE_NEXT_Y);
	const auto posY = category.panel->getHeight() - DISTANCE_BORDER - BUTTON_HEIGHT;
	category.panel->add(cmdLoadState, grpScreenshot->getX(), posY);
	category.panel->add(cmdSaveState, cmdLoadState->getX() + cmdLoadState->getWidth() + DISTANCE_NEXT_X, posY);

	RefreshPanelSavestate();
}

void ExitPanelSavestate()
{
	delete optState0;
	delete optState1;
	delete optState2;
	delete optState3;
	delete optState4;
	delete optState5;
	delete optState6;
	delete optState7;
	delete optState8;
	delete optState9;
	delete optState10;
	delete optState11;
	delete optState12;
	delete optState13;
	delete optState14;
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

	switch (currentStateNum)
	{
		case 0:
			optState0->setSelected(true);
			break;
		case 1:
			optState1->setSelected(true);
			break;
		case 2:
			optState2->setSelected(true);
			break;
		case 3:
			optState3->setSelected(true);
			break;
		case 4:
			optState4->setSelected(true);
			break;
		case 5:
			optState5->setSelected(true);
			break;
		case 6:
			optState6->setSelected(true);
			break;
		case 7:
			optState7->setSelected(true);
			break;
		case 8:
			optState8->setSelected(true);
			break;
		case 9:
			optState9->setSelected(true);
			break;
		case 10:
			optState10->setSelected(true);
			break;
		case 11:
			optState11->setSelected(true);
			break;
		case 12:
			optState12->setSelected(true);
			break;
		case 13:
			optState13->setSelected(true);
			break;
		case 14:
			optState14->setSelected(true);
			break;
		default:
			break;
	}

	gui_update();
	if (strlen(savestate_fname) > 0)
	{
		auto* const f = fopen(savestate_fname, "rbe");
		if (f) {
			fclose(f);
			char date_string[256];
			getts(savestate_fname, date_string, sizeof(date_string));
			lblTimestamp->setCaption(date_string);
		}
		else
		{
			lblTimestamp->setCaption("No savestate found");
		}
	}

	if (strlen(screenshot_filename) > 0)
	{
		auto* const f = fopen(screenshot_filename, "rbe");
		if (f)
		{
			fclose(f);
			const auto rect = grpScreenshot->getChildrenArea();
			auto* loadedImage = IMG_Load(screenshot_filename);
			if (loadedImage != nullptr)
			{
				SDL_Rect source = {0, 0, 0, 0};
				SDL_Rect target = {0, 0, 0, 0};
				auto* scaled = SDL_CreateRGBSurface(loadedImage->flags, rect.width, rect.height,
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

	const auto enabled = true;
	optState0->setEnabled(enabled);
	optState1->setEnabled(enabled);
	optState2->setEnabled(enabled);
	optState3->setEnabled(enabled);
	optState4->setEnabled(enabled);
	optState5->setEnabled(enabled);
	optState6->setEnabled(enabled);
	optState7->setEnabled(enabled);
	optState8->setEnabled(enabled);
	optState9->setEnabled(enabled);
	optState10->setEnabled(enabled);
	optState11->setEnabled(enabled);
	optState12->setEnabled(enabled);
	optState13->setEnabled(enabled);
	optState14->setEnabled(enabled);
	grpScreenshot->setVisible(enabled);
	cmdLoadState->setEnabled(enabled);
	cmdSaveState->setEnabled(enabled);
}

bool HelpPanelSavestate(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("Savestates are stored with the name of the disk in drive DF0 attached");
	helptext.emplace_back("with the selected number.");
	helptext.emplace_back(" ");
	helptext.emplace_back("When you hold left shoulder button and press 'l' during emulation, ");
	helptext.emplace_back("the state of the last active number will be loaded. Hold left shoulder ");
	helptext.emplace_back("button and press 's' to save the current state in the last active slot.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Note: Savestates might not work with HDDs, JIT or RTG.");
	return true;
}
