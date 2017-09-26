#include <guisan.hpp>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include "SelectorEntry.hpp"
#include "UaeRadioButton.hpp"
#include "UaeCheckBox.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "include/memory.h"
#include "newcpu.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include "uae.h"
#include "gui.h"
#include "autoconf.h"
#include "savestate.h"
#include "gui_handling.h"


int currentStateNum = 0;

static gcn::Window* grpNumber;
static gcn::UaeRadioButton* optState0;
static gcn::UaeRadioButton* optState1;
static gcn::UaeRadioButton* optState2;
static gcn::UaeRadioButton* optState3;
static gcn::Window* wndScreenshot;
static gcn::Icon* icoSavestate = nullptr;
static gcn::Image* imgSavestate = nullptr;
static gcn::Button* cmdLoadState;
static gcn::Button* cmdSaveState;
static gcn::Label* lblWarningHDDon;


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
		else if (actionEvent.getSource() == cmdLoadState)
		{
			//------------------------------------------
			// Load state
			//------------------------------------------
			if (emulating)
			{
				if (strlen(savestate_fname) > 0)
				{
					FILE* f = fopen(savestate_fname, "rb");
					if (f)
					{
						fclose(f);
						savestate_initsave(savestate_fname, 2, 0, false);
						savestate_state = STATE_DORESTORE;
						gui_running = false;
					}
				}
				if (savestate_state != STATE_DORESTORE)
					ShowMessage("Loading savestate", "Statefile doesn't exist.", "", "Ok", "");
			}
			else
				ShowMessage("Loading savestate", "Emulation hasn't started yet.", "", "Ok", "");
		}
		else if (actionEvent.getSource() == cmdSaveState)
		{
			//------------------------------------------
			// Save current state
			//------------------------------------------
			if (emulating)
			{
				savestate_initsave(savestate_fname, 2, 0, false);
				save_state(savestate_fname, "...");
				savestate_state = STATE_DOSAVE; // Just to create the screenshot
				delay_savestate_frame = 2;
				gui_running = false;
			}
			else
				ShowMessage("Saving state", "Emulation hasn't started yet.", "", "Ok", "");
		}

		RefreshPanelSavestate();
	}
};

static SavestateActionListener* savestateActionListener;


void InitPanelSavestate(const struct _ConfigCategory& category)
{
	savestateActionListener = new SavestateActionListener();

	optState0 = new gcn::UaeRadioButton("0", "radiostategroup");
	optState0->setId("State0");
	optState0->addActionListener(savestateActionListener);

	optState1 = new gcn::UaeRadioButton("1", "radiostategroup");
	optState1->setId("State1");
	optState1->addActionListener(savestateActionListener);

	optState2 = new gcn::UaeRadioButton("2", "radiostategroup");
	optState2->setId("State2");
	optState2->addActionListener(savestateActionListener);

	optState3 = new gcn::UaeRadioButton("3", "radiostategroup");
	optState3->setId("State3");
	optState3->addActionListener(savestateActionListener);

	grpNumber = new gcn::Window("Number");
	grpNumber->add(optState0, 5, 10);
	grpNumber->add(optState1, 5, 40);
	grpNumber->add(optState2, 5, 70);
	grpNumber->add(optState3, 5, 100);
	grpNumber->setMovable(false);
	grpNumber->setSize(60, 145);
	grpNumber->setBaseColor(gui_baseCol);

	wndScreenshot = new gcn::Window("State screen");
	wndScreenshot->setMovable(false);
	wndScreenshot->setSize(300, 300);
	wndScreenshot->setBaseColor(gui_baseCol);

	cmdLoadState = new gcn::Button("Load State");
	cmdLoadState->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdLoadState->setBaseColor(gui_baseCol);
	cmdLoadState->setId("LoadState");
	cmdLoadState->addActionListener(savestateActionListener);

	cmdSaveState = new gcn::Button("Save State");
	cmdSaveState->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdSaveState->setBaseColor(gui_baseCol);
	cmdSaveState->setId("SaveState");
	cmdSaveState->addActionListener(savestateActionListener);

	lblWarningHDDon = new gcn::Label("State saves do not support harddrive emulation.");
	lblWarningHDDon->setSize(320, LABEL_HEIGHT);

	category.panel->add(grpNumber, DISTANCE_BORDER, DISTANCE_BORDER);
	category.panel->add(wndScreenshot, DISTANCE_BORDER + 100, DISTANCE_BORDER);
	int buttonY = category.panel->getHeight() - DISTANCE_BORDER - BUTTON_HEIGHT;
	category.panel->add(cmdLoadState, DISTANCE_BORDER, buttonY);
	category.panel->add(cmdSaveState, DISTANCE_BORDER + BUTTON_WIDTH + DISTANCE_NEXT_X, buttonY);
	category.panel->add(lblWarningHDDon, DISTANCE_BORDER + 100, DISTANCE_BORDER + 50);

	RefreshPanelSavestate();
}


void ExitPanelSavestate()
{
	delete optState0;
	delete optState1;
	delete optState2;
	delete optState3;
	delete grpNumber;

	if (imgSavestate != nullptr)
		delete imgSavestate;
	imgSavestate = nullptr;
	if (icoSavestate != nullptr)
		delete icoSavestate;
	icoSavestate = nullptr;
	delete wndScreenshot;

	delete cmdLoadState;
	delete cmdSaveState;
	delete lblWarningHDDon;

	delete savestateActionListener;
}


void RefreshPanelSavestate()
{
	if (icoSavestate != nullptr)
	{
		wndScreenshot->remove(icoSavestate);
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
	}

	gui_update();
	if (strlen(screenshot_filename) > 0)
	{
		FILE* f = fopen(screenshot_filename, "rb");
		if (f)
		{
			fclose(f);
			gcn::Rectangle rect = wndScreenshot->getChildrenArea();
			SDL_Surface* loadedImage = IMG_Load(screenshot_filename);
			if (loadedImage != nullptr)
			{
				SDL_Rect source = {0, 0, 0, 0};
				SDL_Rect target = {0, 0, 0, 0};
				SDL_Surface* scaled = SDL_CreateRGBSurface(loadedImage->flags, rect.width, rect.height, loadedImage->format->BitsPerPixel, loadedImage->format->Rmask, loadedImage->format->Gmask, loadedImage->format->Bmask, loadedImage->format->Amask);
				source.w = loadedImage->w;
				source.h = loadedImage->h;
				target.w = rect.width;
				target.h = rect.height;
				SDL_SoftStretch(loadedImage, &source, scaled, &target);
				SDL_FreeSurface(loadedImage);
				loadedImage = nullptr;
				imgSavestate = new gcn::SDLImage(scaled, true);
				icoSavestate = new gcn::Icon(imgSavestate);
				wndScreenshot->add(icoSavestate);
			}
		}
	}

	bool enabled = true; // nr_units () == 0;
	optState0->setEnabled(enabled);
	optState1->setEnabled(enabled);
	optState2->setEnabled(enabled);
	optState3->setEnabled(enabled);
	wndScreenshot->setVisible(enabled);
	cmdLoadState->setEnabled(enabled);
	cmdSaveState->setEnabled(enabled);
	lblWarningHDDon->setVisible(!enabled);
}

bool HelpPanelSavestate(std::vector<std::string> &helptext)
{
	helptext.clear();
	helptext.push_back("Savestates are stored with the name of the disk in drive DF0 attached with the selected number.");
	helptext.push_back("");
	helptext.push_back("Note: Savestates will not work with HDDs.");
	return true;
}
