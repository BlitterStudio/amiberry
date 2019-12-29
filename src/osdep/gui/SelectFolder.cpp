#include <stdbool.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

#ifdef ANDROID
#include "androidsdl_event.h"
#endif

#define DIALOG_WIDTH 520
#define DIALOG_HEIGHT 400

string volName;
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
	vector<string> dirs;

public:
	SelectDirListModel(const char* path)
	{
		changeDir(path);
	}

	int getNumberOfElements() override
	{
		return dirs.size();
	}

	string getElementAt(const int i) override
	{
		if (i >= dirs.size() || i < 0)
			return "---";
		return dirs[i];
	}

	void changeDir(const char* path)
	{
		ReadDirectory(path, &dirs, nullptr);
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
		const auto ptr = realpath(current, actualpath);
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
#ifdef ANDROID
  scrAreaFolders->setScrollbarWidth(30);
#else
	scrAreaFolders->setScrollbarWidth(20);
#endif
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
	const auto focusHdl = gui_top->_getFocusHandler();
	const auto activeWidget = focusHdl->getFocused();
	if (activeWidget == lstFolders)
		cmdOK->requestFocus();
	else if (activeWidget == cmdCancel)
		lstFolders->requestFocus();
	else if (activeWidget == cmdOK)
		cmdCancel->requestFocus();
}

static void navigate_left()
{
	const auto focusHdl = gui_top->_getFocusHandler();
	const auto activeWidget = focusHdl->getFocused();
	if (activeWidget == lstFolders)
		cmdCancel->requestFocus();
	else if (activeWidget == cmdCancel)
		cmdOK->requestFocus();
	else if (activeWidget == cmdOK)
		lstFolders->requestFocus();
}

static void SelectFolderLoop()
{
	FocusBugWorkaround(wndSelectFolder);

	int gotEvent = 0;
	while (!dialogFinished)
	{	
		SDL_Event event;
		SDL_Event touch_event;
		while (SDL_PollEvent(&event))
		{
			gotEvent = 1;
			switch (event.type)
			{
			case SDL_KEYDOWN:
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
			case SDL_JOYAXISMOTION:
				if (gui_joystick)
				{
					const int hat = SDL_JoystickGetHat(gui_joystick, 0);

					if (SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].south_button))
					{
						PushFakeKey(SDLK_RETURN);
						break;
					}

					if (SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].east_button) ||
						SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].start_button))
					{
						dialogFinished = true;
						break;
					}

					if (SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].dpad_left) || (hat & SDL_HAT_LEFT) ||
						SDL_JoystickGetAxis(gui_joystick, host_input_buttons[0].lstick_axis_x) == -32768)
					{
						navigate_left();
						break;
					}

					if (SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].dpad_right) || (hat & SDL_HAT_RIGHT)
						|| SDL_JoystickGetAxis(gui_joystick, host_input_buttons[0].lstick_axis_x) == 32767)
					{
						navigate_right();
						break;
					}

					if (SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].dpad_up) || (hat & SDL_HAT_UP) ||
						SDL_JoystickGetAxis(gui_joystick, host_input_buttons[0].lstick_axis_y) == -32768)
					{
						PushFakeKey(SDLK_UP);
						break;
					}

					if (SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].dpad_down) || (hat & SDL_HAT_DOWN) ||
						SDL_JoystickGetAxis(gui_joystick, host_input_buttons[0].lstick_axis_y) == 32767)
					{
						PushFakeKey(SDLK_DOWN);
						break;
					}
				}
				break;

			case SDL_FINGERDOWN:
				memcpy(&touch_event, &event, sizeof event);
				touch_event.type = SDL_MOUSEBUTTONDOWN;
				touch_event.button.which = 0;
				touch_event.button.button = SDL_BUTTON_LEFT;
				touch_event.button.state = SDL_PRESSED;
				touch_event.button.x = gui_graphics->getTarget()->w * event.tfinger.x;
				touch_event.button.y = gui_graphics->getTarget()->h * event.tfinger.y;
				gui_input->pushInput(touch_event);
				break;

			case SDL_FINGERUP:
				memcpy(&touch_event, &event, sizeof event);
				touch_event.type = SDL_MOUSEBUTTONUP;
				touch_event.button.which = 0;
				touch_event.button.button = SDL_BUTTON_LEFT;
				touch_event.button.state = SDL_RELEASED;
				touch_event.button.x = gui_graphics->getTarget()->w * event.tfinger.x;
				touch_event.button.y = gui_graphics->getTarget()->h * event.tfinger.y;
				gui_input->pushInput(touch_event);
				break;

			case SDL_FINGERMOTION:
				memcpy(&touch_event, &event, sizeof event);
				touch_event.type = SDL_MOUSEMOTION;
				touch_event.motion.which = 0;
				touch_event.motion.state = 0;
				touch_event.motion.x = gui_graphics->getTarget()->w * event.tfinger.x;
				touch_event.motion.y = gui_graphics->getTarget()->h * event.tfinger.y;
				gui_input->pushInput(touch_event);
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
		if (gotEvent)
		{
			// Now we let the Gui object perform its logic.
			uae_gui->logic();
			// Now we let the Gui object draw itself.
			uae_gui->draw();
#ifdef USE_DISPMANX
			UpdateGuiScreen();
#else
			SDL_UpdateTexture(gui_texture, nullptr, gui_screen->pixels, gui_screen->pitch);
#endif
		}
		// Finally we update the screen.
		UpdateGuiScreen();
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
	uae_gui->draw();
#ifdef USE_DISPMANX
#else
	SDL_UpdateTexture(gui_texture, nullptr, gui_screen->pixels, gui_screen->pitch);
#endif
	UpdateGuiScreen();

	SelectFolderLoop();

	ExitSelectFolder();
	if (dialogResult)
	{
		strncpy(value, workingDir, MAX_DPATH);
		if (value[strlen(value) - 1] != '/')
			strncat(value, "/", MAX_DPATH - 1);
	}
	return dialogResult;
}
