#include <sys/types.h>
#include <dirent.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "config.h"
#include "uae.h"
#include "gui_handling.h"

#include "options.h"
#include "inputdevice.h"
#include "amiberry_gfx.h"
#include "amiberry_input.h"

#ifdef ANDROID
#include "androidsdl_event.h"
#endif

#define DIALOG_WIDTH 520
#define DIALOG_HEIGHT 400

std::string volName;
static bool dialogResult = false;
static bool dialogFinished = false;
static char workingDir[MAX_DPATH];

static gcn::Window* wndSelectFolder;
static gcn::Button* cmdOK;
static gcn::Button* cmdCancel;
static gcn::ListBox* lstFolders;
static gcn::ScrollArea* scrAreaFolders;
static gcn::TextField* txtCurrent;


class FolderRequesterButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdOK)
		{
			dialogResult = true;
		}
		dialogFinished = true;
	}
};

static FolderRequesterButtonActionListener* folderButtonActionListener;


class SelectDirListModel : public gcn::ListModel
{
	std::vector<std::string> dirs;

public:
	SelectDirListModel(const char* path)
	{
		changeDir(path);
	}

	int getNumberOfElements() override
	{
		return dirs.size();
	}

	std::string getElementAt(const int i) override
	{
		if (i >= int(dirs.size()) || i < 0)
			return "---";
		return dirs[i];
	}

	void changeDir(const char* path)
	{
		read_directory(path, &dirs, nullptr);
		if (dirs.empty())
			dirs.emplace_back("..");
	}
};

static SelectDirListModel dirList(".");


static void checkfoldername(char* current)
{
	char actualpath [MAX_DPATH];
	DIR* dir;

	if ((dir = opendir(current)))
	{
		dirList = current;
		auto* const ptr = realpath(current, actualpath);
		strncpy(workingDir, ptr, MAX_DPATH);
		closedir(dir);
	}
	else
		strncpy(workingDir, start_path_data, MAX_DPATH);
	txtCurrent->setText(workingDir);
}


class ListBoxActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		char foldername[MAX_DPATH] = "";

		const auto selected_item = lstFolders->getSelected();
		strncpy(foldername, workingDir, MAX_DPATH - 1);
		strncat(foldername, "/", MAX_DPATH - 1);
		strncat(foldername, dirList.getElementAt(selected_item).c_str(), MAX_DPATH - 2);
		volName = dirList.getElementAt(selected_item);
		checkfoldername(foldername);
	}
};

static ListBoxActionListener* listBoxActionListener;

#ifdef ANDROID
class EditDirPathActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
       char tmp[MAX_DPATH];
       strncpy(tmp, txtCurrent->getText().c_str(), MAX_DPATH - 1);
       checkfoldername(tmp);
    }
};
static EditDirPathActionListener* editDirPathActionListener;
#endif

static void InitSelectFolder(const char* title)
{
	wndSelectFolder = new gcn::Window("Load");
	wndSelectFolder->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
	wndSelectFolder->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
	wndSelectFolder->setBaseColor(gui_baseCol);
	wndSelectFolder->setCaption(title);
	wndSelectFolder->setTitleBarHeight(TITLEBAR_HEIGHT);

	folderButtonActionListener = new FolderRequesterButtonActionListener();

	cmdOK = new gcn::Button("Ok");
	cmdOK->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH - DISTANCE_NEXT_X,
	                   DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdOK->setBaseColor(gui_baseCol);
	cmdOK->addActionListener(folderButtonActionListener);

	cmdCancel = new gcn::Button("Cancel");
	cmdCancel->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdCancel->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH,
	                       DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdCancel->setBaseColor(gui_baseCol);
	cmdCancel->addActionListener(folderButtonActionListener);

	txtCurrent = new gcn::TextField();
	txtCurrent->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER - 4, TEXTFIELD_HEIGHT);
	txtCurrent->setPosition(DISTANCE_BORDER, 10);
#ifdef ANDROID
  txtCurrent->setEnabled(true);
  editDirPathActionListener = new EditDirPathActionListener();
  txtCurrent->addActionListener(editDirPathActionListener);
#else
	txtCurrent->setEnabled(false);
#endif

	listBoxActionListener = new ListBoxActionListener();

	lstFolders = new gcn::ListBox(&dirList);
	lstFolders->setSize(800, 252);
	lstFolders->setBaseColor(gui_baseCol);
	lstFolders->setWrappingEnabled(true);
	lstFolders->addActionListener(listBoxActionListener);

	scrAreaFolders = new gcn::ScrollArea(lstFolders);
	scrAreaFolders->setBorderSize(1);
	scrAreaFolders->setPosition(DISTANCE_BORDER, 10 + TEXTFIELD_HEIGHT + 10);
	scrAreaFolders->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER - 4, 272);
	scrAreaFolders->setScrollbarWidth(30);
	scrAreaFolders->setBaseColor(gui_baseCol);

	wndSelectFolder->add(cmdOK);
	wndSelectFolder->add(cmdCancel);
	wndSelectFolder->add(txtCurrent);
	wndSelectFolder->add(scrAreaFolders);

	gui_top->add(wndSelectFolder);

	lstFolders->requestFocus();
	lstFolders->setSelected(0);
	wndSelectFolder->requestModalFocus();
}


static void ExitSelectFolder()
{
	wndSelectFolder->releaseModalFocus();
	gui_top->remove(wndSelectFolder);

	delete cmdOK;
	delete cmdCancel;
	delete folderButtonActionListener;

	delete txtCurrent;
	delete lstFolders;
	delete scrAreaFolders;
	delete listBoxActionListener;

	delete wndSelectFolder;
}

static void navigate_right()
{
	auto* const focusHdl = gui_top->_getFocusHandler();
	auto* const activeWidget = focusHdl->getFocused();
	if (activeWidget == lstFolders)
		cmdOK->requestFocus();
	else if (activeWidget == cmdCancel)
		lstFolders->requestFocus();
	else if (activeWidget == cmdOK)
		cmdCancel->requestFocus();
}

static void navigate_left()
{
	auto* const focusHdl = gui_top->_getFocusHandler();
	auto* const activeWidget = focusHdl->getFocused();
	if (activeWidget == lstFolders)
		cmdCancel->requestFocus();
	else if (activeWidget == cmdCancel)
		cmdOK->requestFocus();
	else if (activeWidget == cmdOK)
		lstFolders->requestFocus();
}

static void SelectFolderLoop()
{
	auto got_event = 0;
	SDL_Event event;
	SDL_Event touch_event;
	struct didata* did = &di_joystick[0];
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_KEYDOWN:
			got_event = 1;
			switch (event.key.keysym.sym)
			{
			case VK_ESCAPE:
				dialogFinished = true;
				break;

			case VK_LEFT:
				navigate_left();
				break;

			case VK_RIGHT:
				navigate_right();
				break;

			case VK_Red:
			case VK_Green:
				event.key.keysym.sym = SDLK_RETURN;
				gui_input->pushInput(event); // Fire key down
				event.type = SDL_KEYUP; // and the key up
				break;
			default:
				break;
			}
			break;

		case SDL_JOYBUTTONDOWN:
		case SDL_JOYHATMOTION:
			if (gui_joystick)
			{
				got_event = 1;
				const int hat = SDL_JoystickGetHat(gui_joystick, 0);
				
				if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_A]) ||
					SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_B]))
				{
					PushFakeKey(SDLK_RETURN);
					break;
				}

				if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_X]) ||
					SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_Y]) ||
					SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_START]))
				{
					dialogFinished = true;
					break;
				}

				if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_LEFT]) || hat & SDL_HAT_LEFT)
				{
					navigate_left();
					break;
				}

				if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_RIGHT]) || hat & SDL_HAT_RIGHT)
				{
					navigate_right();
					break;
				}

				if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_UP]) || hat & SDL_HAT_UP)
				{
					PushFakeKey(SDLK_UP);
					break;
				}

				if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_DOWN]) || hat & SDL_HAT_DOWN)
				{
					PushFakeKey(SDLK_DOWN);
					break;
				}
			}
			break;

		case SDL_JOYAXISMOTION:
			if (gui_joystick)
			{
				got_event = 1;
				if (event.jaxis.axis == SDL_CONTROLLER_AXIS_LEFTX)
				{
					if (event.jaxis.value > joystick_dead_zone && last_x != 1)
					{
						last_x = 1;
						navigate_right();
						break;
					}
					if (event.jaxis.value < -joystick_dead_zone && last_x != -1)
					{
						last_x = -1;
						navigate_left();
						break;
					}
					if (event.jaxis.value > -joystick_dead_zone && event.jaxis.value < joystick_dead_zone)
						last_x = 0;
				}
				else if (event.jaxis.axis == SDL_CONTROLLER_AXIS_LEFTY)
				{
					if (event.jaxis.value < -joystick_dead_zone && last_y != -1)
					{
						last_y = -1;
						PushFakeKey(SDLK_UP);
						break;
					}
					if (event.jaxis.value > joystick_dead_zone && last_y != 1)
					{
						last_y = 1;
						PushFakeKey(SDLK_DOWN);
						break;
					}
					if (event.jaxis.value > -joystick_dead_zone && event.jaxis.value < joystick_dead_zone)
						last_y = 0;
				}
			}
			break;

		case SDL_FINGERDOWN:
			got_event = 1;
			memcpy(&touch_event, &event, sizeof event);
			touch_event.type = SDL_MOUSEBUTTONDOWN;
			touch_event.button.which = 0;
			touch_event.button.button = SDL_BUTTON_LEFT;
			touch_event.button.state = SDL_PRESSED;
			touch_event.button.x = float(gui_graphics->getTarget()->w) * event.tfinger.x;
			touch_event.button.y = float(gui_graphics->getTarget()->h) * event.tfinger.y;
			gui_input->pushInput(touch_event);
			break;

		case SDL_FINGERUP:
			got_event = 1;
			memcpy(&touch_event, &event, sizeof event);
			touch_event.type = SDL_MOUSEBUTTONUP;
			touch_event.button.which = 0;
			touch_event.button.button = SDL_BUTTON_LEFT;
			touch_event.button.state = SDL_RELEASED;
			touch_event.button.x = float(gui_graphics->getTarget()->w) * event.tfinger.x;
			touch_event.button.y = float(gui_graphics->getTarget()->h) * event.tfinger.y;
			gui_input->pushInput(touch_event);
			break;

		case SDL_FINGERMOTION:
			got_event = 1;
			memcpy(&touch_event, &event, sizeof event);
			touch_event.type = SDL_MOUSEMOTION;
			touch_event.motion.which = 0;
			touch_event.motion.state = 0;
			touch_event.motion.x = float(gui_graphics->getTarget()->w) * event.tfinger.x;
			touch_event.motion.y = float(gui_graphics->getTarget()->h) * event.tfinger.y;
			gui_input->pushInput(touch_event);
			break;

		case SDL_KEYUP:
		case SDL_JOYBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEMOTION:
		case SDL_MOUSEWHEEL:
			got_event = 1;
			break;

		default:
			break;
		}

		//-------------------------------------------------
		// Send event to guisan-controls
		//-------------------------------------------------
#ifdef ANDROID
		androidsdl_event(event, gui_input);
#else
		gui_input->pushInput(event);
#endif
	}

	if (got_event)
	{
		// Now we let the Gui object perform its logic.
		uae_gui->logic();
		SDL_RenderClear(sdl_renderer);
		// Now we let the Gui object draw itself.
		uae_gui->draw();
		// Finally we update the screen.
		update_gui_screen();
	}
}


bool SelectFolder(const char* title, char* value)
{
	dialogResult = false;
	dialogFinished = false;

	InitSelectFolder(title);
	checkfoldername(value);

	// Prepare the screen once
	uae_gui->logic();
	SDL_RenderClear(sdl_renderer);
	uae_gui->draw();
	update_gui_screen();

	while (!dialogFinished)
	{
		const auto start = SDL_GetPerformanceCounter();
		SelectFolderLoop();
		cap_fps(start);
	}

	ExitSelectFolder();
	if (dialogResult)
	{
		strncpy(value, workingDir, MAX_DPATH);
	}
	return dialogResult;
}
