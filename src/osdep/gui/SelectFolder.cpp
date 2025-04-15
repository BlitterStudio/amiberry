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
	DIALOG_WIDTH = 600,
	DIALOG_HEIGHT = 600
};

std::string volName;
static bool dialogResult = false;
static bool dialogFinished = false;
static std::string workingDir;

static gcn::Window* wndSelectFolder;
static gcn::Button* cmdOK;
static gcn::Button* cmdCancel;
static gcn::ListBox* lstFolders;
static gcn::ScrollArea* scrAreaFolders;
static gcn::TextField* txtCurrent;
static gcn::Button* cmdCreateFolder;

class SelectDirListModel : public gcn::ListModel
{
	std::vector<std::string> dirs;

public:
	SelectDirListModel(const std::string& path)
	{
		changeDir(path);
	}

	int getNumberOfElements() override
	{
		return static_cast<int>(dirs.size());
	}

	void add(const std::string& elem) override
	{
		dirs.push_back(elem);
	}

	void clear() override
	{
		dirs.clear();
	}
	
	std::string getElementAt(int i) override
	{
		if (i < 0 || i >= getNumberOfElements())
			return "---";
		return dirs[i];
	}

	void changeDir(const std::string& path)
	{
		read_directory(path, &dirs, nullptr);
		if (std::find(dirs.begin(), dirs.end(), "..") == dirs.end())
			dirs.insert(dirs.begin(), "..");
	}
};

static SelectDirListModel dirList(".");

static void checkfoldername(const std::string& current)
{
	DIR* dir;

	if ((dir = opendir(current.c_str())))
	{
		char actualpath [MAX_DPATH];
		dirList = current;
		auto* const ptr = realpath(current.c_str(), actualpath);
		workingDir = std::string(ptr);
		closedir(dir);
		lstFolders->adjustSize();
	}
	else
	{
		workingDir = home_dir;
		dirList = workingDir;
		lstFolders->adjustSize();
	}
	txtCurrent->setText(workingDir);
}

class ListBoxActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		const auto selected_item = lstFolders->getSelected();
		const std::string folder_name = workingDir.append("/").append(dirList.getElementAt(selected_item));

		volName = dirList.getElementAt(selected_item);
		checkfoldername(folder_name);
	}
};

static ListBoxActionListener* listBoxActionListener;

class FolderRequesterButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdOK)
		{
			dialogResult = true;
			dialogFinished = true;
		}
		else if (actionEvent.getSource() == cmdCancel)
		{
			dialogResult = false;
			dialogFinished = true;
		}
		else if (actionEvent.getSource() == cmdCreateFolder)
		{
			wndSelectFolder->releaseModalFocus();
			if (Create_Folder(workingDir))
			{
				checkfoldername(workingDir);
				wndSelectFolder->requestModalFocus();
			}
		}
	}
};
static FolderRequesterButtonActionListener* folderButtonActionListener;

class EditDirPathActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		checkfoldername(txtCurrent->getText());
	}
};
static EditDirPathActionListener* editDirPathActionListener;

static void InitSelectFolder(const std::string& title)
{
	wndSelectFolder = new gcn::Window("Load");
	wndSelectFolder->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
	wndSelectFolder->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
	wndSelectFolder->setBaseColor(gui_base_color);
	wndSelectFolder->setForegroundColor(gui_foreground_color);
	wndSelectFolder->setCaption(title);
	wndSelectFolder->setTitleBarHeight(TITLEBAR_HEIGHT);
	wndSelectFolder->setMovable(false);

	folderButtonActionListener = new FolderRequesterButtonActionListener();

	cmdCreateFolder = new gcn::Button("Create Folder");
	cmdCreateFolder->setSize(BUTTON_WIDTH * 2, BUTTON_HEIGHT);
	cmdCreateFolder->setPosition(DISTANCE_BORDER, DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdCreateFolder->setBaseColor(gui_base_color);
	cmdCreateFolder->setForegroundColor(gui_foreground_color);
	cmdCreateFolder->addActionListener(folderButtonActionListener);

	cmdOK = new gcn::Button("Ok");
	cmdOK->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH - DISTANCE_NEXT_X,
					   DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdOK->setBaseColor(gui_base_color);
	cmdOK->setForegroundColor(gui_foreground_color);
	cmdOK->addActionListener(folderButtonActionListener);

	cmdCancel = new gcn::Button("Cancel");
	cmdCancel->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdCancel->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH,
						   DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdCancel->setBaseColor(gui_base_color);
	cmdCancel->setForegroundColor(gui_foreground_color);
	cmdCancel->addActionListener(folderButtonActionListener);

	txtCurrent = new gcn::TextField();
	txtCurrent->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER - 4, TEXTFIELD_HEIGHT);
	txtCurrent->setPosition(DISTANCE_BORDER, 10);
	txtCurrent->setBaseColor(gui_base_color);
	txtCurrent->setBackgroundColor(gui_background_color);
	txtCurrent->setForegroundColor(gui_foreground_color);
	txtCurrent->setEnabled(true);
	editDirPathActionListener = new EditDirPathActionListener();
	txtCurrent->addActionListener(editDirPathActionListener);

	listBoxActionListener = new ListBoxActionListener();

	lstFolders = new gcn::ListBox(&dirList);
	lstFolders->setSize(DIALOG_WIDTH - 45, DIALOG_HEIGHT - 108);
	lstFolders->setBaseColor(gui_base_color);
	lstFolders->setBackgroundColor(gui_background_color);
	lstFolders->setForegroundColor(gui_foreground_color);
	lstFolders->setSelectionColor(gui_selection_color);
	lstFolders->setWrappingEnabled(true);
	lstFolders->addActionListener(listBoxActionListener);

	scrAreaFolders = new gcn::ScrollArea(lstFolders);
	scrAreaFolders->setFrameSize(1);
	scrAreaFolders->setPosition(DISTANCE_BORDER, 10 + TEXTFIELD_HEIGHT + 10);
	scrAreaFolders->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER - 4, DIALOG_HEIGHT - 128);
	scrAreaFolders->setScrollbarWidth(SCROLLBAR_WIDTH);
	scrAreaFolders->setBaseColor(gui_base_color);
	scrAreaFolders->setBackgroundColor(gui_background_color);
	scrAreaFolders->setForegroundColor(gui_foreground_color);
	scrAreaFolders->setSelectionColor(gui_selection_color);
	scrAreaFolders->setHorizontalScrollPolicy(gcn::ScrollArea::ShowAuto);
	scrAreaFolders->setVerticalScrollPolicy(gcn::ScrollArea::ShowAuto);

	wndSelectFolder->add(cmdCreateFolder);
	wndSelectFolder->add(cmdOK);
	wndSelectFolder->add(cmdCancel);
	wndSelectFolder->add(txtCurrent);
	wndSelectFolder->add(scrAreaFolders);

	gui_top->add(wndSelectFolder);

	wndSelectFolder->requestModalFocus();
	focus_bug_workaround(wndSelectFolder);
	lstFolders->requestFocus();
	lstFolders->setSelected(0);
}

static void ExitSelectFolder()
{
	wndSelectFolder->releaseModalFocus();
	gui_top->remove(wndSelectFolder);

	delete cmdCreateFolder;
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
	const auto* const focusHdl = gui_top->_getFocusHandler();
	const auto* const activeWidget = focusHdl->getFocused();
	if (activeWidget == lstFolders)
		cmdCreateFolder->requestFocus();
	else if (activeWidget == cmdCancel)
		lstFolders->requestFocus();
	else if (activeWidget == cmdOK)
		cmdCancel->requestFocus();
	else if (activeWidget == cmdCreateFolder)
		cmdOK->requestFocus();
}

static void navigate_left()
{
	const auto* const focusHdl = gui_top->_getFocusHandler();
	const auto* const activeWidget = focusHdl->getFocused();
	if (activeWidget == lstFolders)
		cmdCancel->requestFocus();
	else if (activeWidget == cmdCancel)
		cmdOK->requestFocus();
	else if (activeWidget == cmdOK)
		cmdCreateFolder->requestFocus();
	else if (activeWidget == cmdCreateFolder)
		lstFolders->requestFocus();
}

static void SelectFolderLoop()
{
	const AmigaMonitor* mon = &AMonitors[0];

	bool got_event = false;
	SDL_Event event;
	SDL_Event touch_event;
	bool nav_left, nav_right;
	while (SDL_PollEvent(&event))
	{
		const auto* focus_hdl = gui_top->_getFocusHandler();
		const auto* active_widget = focus_hdl->getFocused();
		nav_left = nav_right = false;
		switch (event.type)
		{
		case SDL_KEYDOWN:
			if (active_widget == txtCurrent)
				break;
			got_event = handle_keydown(event, dialogFinished, nav_left, nav_right);
			break;

		case SDL_JOYBUTTONDOWN:
		case SDL_JOYHATMOTION:
			if (gui_joystick)
			{
				got_event = handle_joybutton(&di_joystick[0], dialogFinished, nav_left, nav_right);
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

std::string SelectFolder(const std::string& title, std::string value)
{
	const AmigaMonitor* mon = &AMonitors[0];

	dialogResult = false;
	dialogFinished = false;

	InitSelectFolder(title);
	checkfoldername(value);

	// Prepare the screen once
	uae_gui->logic();

	SDL_RenderClear(mon->gui_renderer);

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
		value = workingDir;
	else
		value = "";

	return value;
}
