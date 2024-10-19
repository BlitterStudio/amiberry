#include <algorithm>
#include <sys/types.h>
#include <dirent.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "config.h"
#include "uae.h"
#include "gui_handling.h"

#include "options.h"
#include "inputdevice.h"
#include "amiberry_gfx.h"
#include "amiberry_input.h"

enum
{
	DIALOG_WIDTH = 520,
	DIALOG_HEIGHT = 200
};

static bool create_folder_dialog_result = false;
static bool create_folder_dialog_finished = false;
static std::string create_folder_working_dir;

static gcn::Window* wndCreateFolder;
static gcn::Label* lblCreateFolder;
static gcn::TextField* txtCreateFolder;
static gcn::Button* cmdOK;
static gcn::Button* cmdCancel;

class CreateFolderRequesterButtonActionListener : public gcn::ActionListener
{
public:
    void action(const gcn::ActionEvent& actionEvent) override
    {
        if (actionEvent.getSource() == cmdOK)
        {
            // create a new directory in the current working directory
            const std::string& folder_name = txtCreateFolder->getText();
			if (!folder_name.empty()) {
				const std::string folder_path = create_folder_working_dir + "/" + folder_name;
				const int result = mkdir(folder_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
				if (result == 0)
				{
					create_folder_dialog_result = true;
				}
			}
        }
        create_folder_dialog_finished = true;
    }
};
static CreateFolderRequesterButtonActionListener* createFolderButtonActionListener;

static void InitCreateFolder(const std::string& path)
{
	create_folder_working_dir = path;

	wndCreateFolder = new gcn::Window("Create Folder");
	wndCreateFolder->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
	wndCreateFolder->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
	wndCreateFolder->setBaseColor(gui_base_color);
	wndCreateFolder->setForegroundColor(gui_foreground_color);
	wndCreateFolder->setTitleBarHeight(TITLEBAR_HEIGHT);
	wndCreateFolder->setMovable(false);
	gui_top->add(wndCreateFolder);

	createFolderButtonActionListener = new CreateFolderRequesterButtonActionListener();

	lblCreateFolder = new gcn::Label("Enter the name of the folder you want to create:");
	lblCreateFolder->setAlignment(gcn::Graphics::Right);
	lblCreateFolder->setBaseColor(gui_base_color);
	lblCreateFolder->setForegroundColor(gui_foreground_color);
	lblCreateFolder->setPosition(10, 30);
	wndCreateFolder->add(lblCreateFolder);

	txtCreateFolder = new gcn::TextField();
	txtCreateFolder->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER - 4, TEXTFIELD_HEIGHT);
	txtCreateFolder->setBaseColor(gui_base_color);
	txtCreateFolder->setForegroundColor(gui_foreground_color);
	txtCreateFolder->setBackgroundColor(gui_background_color);
	txtCreateFolder->setPosition(lblCreateFolder->getX(), lblCreateFolder->getY() + lblCreateFolder->getHeight() + DISTANCE_NEXT_Y);
	txtCreateFolder->setEnabled(true);
	txtCreateFolder->setText("");
	wndCreateFolder->add(txtCreateFolder);

	cmdOK = new gcn::Button("OK");
	cmdOK->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdOK->setBaseColor(gui_base_color);
	cmdOK->setForegroundColor(gui_foreground_color);
	cmdOK->addActionListener(createFolderButtonActionListener);
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH - DISTANCE_NEXT_X,
		DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	wndCreateFolder->add(cmdOK);

	cmdCancel = new gcn::Button("Cancel");
	cmdCancel->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdCancel->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH,
		DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdCancel->setBaseColor(gui_base_color);
	cmdCancel->setForegroundColor(gui_foreground_color);
	cmdCancel->addActionListener(createFolderButtonActionListener);
	wndCreateFolder->add(cmdCancel);

	wndCreateFolder->requestModalFocus();
	focus_bug_workaround(wndCreateFolder);
	txtCreateFolder->requestFocus();
}

static void ExitCreateFolder()
{
	wndCreateFolder->releaseModalFocus();
	gui_top->remove(wndCreateFolder);

	delete cmdOK;
	delete cmdCancel;
	delete txtCreateFolder;
	delete lblCreateFolder;
	delete createFolderButtonActionListener;
	delete wndCreateFolder;
}

static void navigate_right()
{
	const auto* const focusHdl = gui_top->_getFocusHandler();
	const auto* const activeWidget = focusHdl->getFocused();
	if (activeWidget == cmdCancel)
		cmdOK->requestFocus();
	else if (activeWidget == cmdOK)
		cmdCancel->requestFocus();
}

static void navigate_left()
{
	const auto* const focusHdl = gui_top->_getFocusHandler();
	const auto* const activeWidget = focusHdl->getFocused();
	if (activeWidget == cmdOK)
		cmdCancel->requestFocus();
	else if (activeWidget == cmdCancel)
		cmdOK->requestFocus();
}

static void CreateFolderLoop()
{
	const AmigaMonitor* mon = &AMonitors[0];

	bool got_event = false;
	SDL_Event event;
	SDL_Event touch_event;
	bool nav_left, nav_right;
	while (SDL_PollEvent(&event))
	{
		nav_left = nav_right = false;
		switch (event.type)
		{
		case SDL_KEYDOWN:
			got_event = handle_keydown(event, create_folder_dialog_finished, nav_left, nav_right);
			break;

		case SDL_JOYBUTTONDOWN:
		case SDL_JOYHATMOTION:
			if (gui_joystick)
			{
				got_event = handle_joybutton(&di_joystick[0], create_folder_dialog_finished, nav_left, nav_right);
			}
			break;

		case SDL_JOYAXISMOTION:
			if (gui_joystick)
			{
				got_event = handle_joyaxis(event, nav_left, nav_right);
			}
			break;

		case SDL_FINGERDOWN:
		case SDL_FINGERUP:
		case SDL_FINGERMOTION:
			got_event = handle_finger(event, touch_event);
			gui_input->pushInput(touch_event);
			break;

		case SDL_MOUSEWHEEL:
			got_event = handle_mousewheel(event);
			break;

		default:
			got_event = true;
			break;
		}

		if (nav_left)
			navigate_left();
		else if (nav_right)
			navigate_right();

		//-------------------------------------------------
		// Send event to guisan-controls
		//-------------------------------------------------
		gui_input->pushInput(event);
	}

	if (got_event)
	{
		// Now we let the Gui object perform its logic.
		uae_gui->logic();
		SDL_RenderClear(mon->gui_renderer);
		// Now we let the Gui object draw itself.
		uae_gui->draw();
		// Finally we update the screen.
		update_gui_screen();
	}
}

bool Create_Folder(const std::string& path)
{
	const AmigaMonitor* mon = &AMonitors[0];

	create_folder_dialog_result = false;
	create_folder_dialog_finished = false;

	InitCreateFolder(path);

	// Prepare the screen once
	uae_gui->logic();

	SDL_RenderClear(mon->gui_renderer);

	uae_gui->draw();
	update_gui_screen();

	while (!create_folder_dialog_finished)
	{
		const auto start = SDL_GetPerformanceCounter();
		CreateFolderLoop();
		cap_fps(start);
	}

	ExitCreateFolder();
	return create_folder_dialog_result;
}