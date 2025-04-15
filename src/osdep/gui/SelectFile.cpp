#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/types.h>
#include <dirent.h>

#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"
#include "StringListModel.h"

#include "sysdeps.h"
#include "config.h"
#include "uae.h"
#include "fsdb.h"
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

static bool dialogResult = false;
static bool dialogFinished = false;
static bool createNew = false;
static std::string workingDir;
static const char** filefilter;
static int selectedOnStart = -1;

static gcn::Window* wndSelectFile;
static gcn::Button* cmdOK;
static gcn::Button* cmdCancel;
static gcn::ListBox* lstFiles;
static gcn::ScrollArea* scrAreaFiles;
static gcn::TextField* txtCurrent;
static gcn::Label* lblFilename;
static gcn::TextField* txtFilename;

class SelectFileListModel : public gcn::ListModel
{
	std::vector<std::string> dirs;
	std::vector<std::string> files;

public:
	explicit SelectFileListModel(const std::string& path)
	{
		changeDir(path);
	}

	int getNumberOfElements() override
	{
		return static_cast<int>(dirs.size() + files.size());
	}

	void add(const std::string& elem) override
	{
		dirs.push_back(elem);
	}

	void clear() override
	{
		dirs.clear();
		files.clear();
	}
	
	std::string getElementAt(int i) override
	{
		if (i < 0 || i >= getNumberOfElements())
			return "---";
		if (i < dirs.size())
			return dirs[i];
		return files[i - dirs.size()];
	}

	void changeDir(const std::string& path)
	{
		read_directory(path, &dirs, &files);
		if (std::find(dirs.begin(), dirs.end(), "..") == dirs.end())
			dirs.insert(dirs.begin(), "..");
		if (filefilter != nullptr)
			FilterFiles(&files, filefilter);
	}

	[[nodiscard]] bool isDir(unsigned int i) const
	{
		return (i < dirs.size());
	}
};

static SelectFileListModel* fileList;

static void checkfoldername(const std::string& current)
{
	DIR* dir;

	if ((dir = opendir(current.c_str())))
	{
		char actualpath[MAX_DPATH];
		fileList->changeDir(current);
		auto* const ptr = realpath(current.c_str(), actualpath);
		workingDir = std::string(ptr);
		closedir(dir);
		lstFiles->adjustSize();
	}
	else
	{
		workingDir = home_dir;
		fileList->changeDir(workingDir);
		lstFiles->adjustSize();
	}
	txtCurrent->setText(workingDir);
}

static void checkfilename(const std::string& current)
{
	char actfile[MAX_DPATH];
	extract_filename(current.c_str(), actfile);
	for (auto i = 0; i < fileList->getNumberOfElements(); ++i)
	{
		if (!fileList->isDir(i) && !stricmp(fileList->getElementAt(i).c_str(), actfile))
		{
			lstFiles->setSelected(i);
			selectedOnStart = i;
			break;
		}
	}
}

class FileButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdOK)
		{
			const auto selected_item = lstFiles->getSelected();
			if (createNew)
			{
				if (txtFilename->getText().empty())
					return;
				std::string tmp = workingDir + "/" + txtFilename->getText();

				if (filefilter != nullptr) {
					if (tmp.find(filefilter[0]) == std::string::npos)
						tmp = tmp.append(filefilter[0]);
				}

				workingDir = tmp;
				dialogResult = true;
			}
			else
			{
				if (fileList->isDir(selected_item))
					return; // Directory selected -> Ok not possible
				workingDir = workingDir + "/" + fileList->getElementAt(selected_item);
				dialogResult = true;
			}
			dialogFinished = true;
		}
		else if (actionEvent.getSource() == cmdCancel)
		{
			dialogResult = false;
			dialogFinished = true;
		}
	}
};

static FileButtonActionListener* fileButtonActionListener;

class SelectFileActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		const auto selected_item = lstFiles->getSelected();

		const std::string foldername = workingDir + "/" + fileList->getElementAt(selected_item);
		if (fileList->isDir(selected_item))
			checkfoldername(foldername);
		else if (!createNew)
		{
			workingDir = foldername;
			dialogResult = true;
			dialogFinished = true;
		}
		else if (createNew && selected_item > 0)
		{
			txtFilename->setText(fileList->getElementAt(selected_item));
		}
	}
};

static SelectFileActionListener* selectFileActionListener;

class EditFilePathActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		checkfoldername(txtCurrent->getText());
	}
};
static EditFilePathActionListener* editFilePathActionListener;

static void InitSelectFile(const std::string& title)
{
	wndSelectFile = new gcn::Window("Load");
	wndSelectFile->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
	wndSelectFile->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
	wndSelectFile->setBaseColor(gui_base_color);
	wndSelectFile->setForegroundColor(gui_foreground_color);
	wndSelectFile->setCaption(title);
	wndSelectFile->setTitleBarHeight(TITLEBAR_HEIGHT);
	wndSelectFile->setMovable(false);

	fileButtonActionListener = new FileButtonActionListener();

	cmdOK = new gcn::Button("Ok");
	cmdOK->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH - DISTANCE_NEXT_X,
					   DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdOK->setBaseColor(gui_base_color);
	cmdOK->setForegroundColor(gui_foreground_color);
	cmdOK->addActionListener(fileButtonActionListener);

	cmdCancel = new gcn::Button("Cancel");
	cmdCancel->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdCancel->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH,
						   DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdCancel->setBaseColor(gui_base_color);
	cmdCancel->setForegroundColor(gui_foreground_color);
	cmdCancel->addActionListener(fileButtonActionListener);

	txtCurrent = new gcn::TextField();
	txtCurrent->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER - 4, TEXTFIELD_HEIGHT);
	txtCurrent->setPosition(DISTANCE_BORDER, 10);
	txtCurrent->setBaseColor(gui_base_color);
	txtCurrent->setBackgroundColor(gui_background_color);
	txtCurrent->setForegroundColor(gui_foreground_color);
	txtCurrent->setEnabled(true);
	editFilePathActionListener =  new EditFilePathActionListener();
	txtCurrent->addActionListener(editFilePathActionListener);

	selectFileActionListener = new SelectFileActionListener();
	fileList = new SelectFileListModel(".");

	lstFiles = new gcn::ListBox(fileList);
	lstFiles->setSize(DIALOG_WIDTH - 45, DIALOG_HEIGHT - 108);
	lstFiles->setBaseColor(gui_base_color);
	lstFiles->setBackgroundColor(gui_background_color);
	lstFiles->setForegroundColor(gui_foreground_color);
	lstFiles->setSelectionColor(gui_selection_color);
	lstFiles->setWrappingEnabled(true);
	lstFiles->addActionListener(selectFileActionListener);

	scrAreaFiles = new gcn::ScrollArea(lstFiles);
	scrAreaFiles->setFrameSize(1);
	scrAreaFiles->setPosition(DISTANCE_BORDER, 10 + TEXTFIELD_HEIGHT + 10);
	scrAreaFiles->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER - 4, DIALOG_HEIGHT - 128);
	scrAreaFiles->setScrollbarWidth(SCROLLBAR_WIDTH);
	scrAreaFiles->setBaseColor(gui_base_color);
	scrAreaFiles->setBackgroundColor(gui_background_color);
	scrAreaFiles->setForegroundColor(gui_foreground_color);
	scrAreaFiles->setSelectionColor(gui_selection_color);
	scrAreaFiles->setHorizontalScrollPolicy(gcn::ScrollArea::ShowAuto);
	scrAreaFiles->setVerticalScrollPolicy(gcn::ScrollArea::ShowAuto);

	if (createNew)
	{
		scrAreaFiles->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER - 4, DIALOG_HEIGHT - 128 - TEXTFIELD_HEIGHT - DISTANCE_NEXT_Y);
		lblFilename = new gcn::Label("Filename:");
		lblFilename->setSize(80, LABEL_HEIGHT);
		lblFilename->setAlignment(gcn::Graphics::Left);
		lblFilename->setPosition(DISTANCE_BORDER, scrAreaFiles->getY() + scrAreaFiles->getHeight() + DISTANCE_NEXT_Y);
		txtFilename = new gcn::TextField();
		txtFilename->setSize(350, TEXTFIELD_HEIGHT);
		txtFilename->setId("Filename");
		txtFilename->setBaseColor(gui_base_color);
		txtFilename->setBackgroundColor(gui_background_color);
		txtFilename->setForegroundColor(gui_foreground_color);
		txtFilename->setPosition(lblFilename->getX() + lblFilename->getWidth() + DISTANCE_NEXT_X, lblFilename->getY());

		wndSelectFile->add(lblFilename);
		wndSelectFile->add(txtFilename);
	}

	wndSelectFile->add(cmdOK);
	wndSelectFile->add(cmdCancel);
	wndSelectFile->add(txtCurrent);
	wndSelectFile->add(scrAreaFiles);

	gui_top->add(wndSelectFile);

	wndSelectFile->requestModalFocus();
	focus_bug_workaround(wndSelectFile);
	if (createNew)
	{
		txtFilename->requestFocus();
	}
	else
	{
		lstFiles->requestFocus();
		lstFiles->setSelected(0);
	}
}

static void ExitSelectFile()
{
	wndSelectFile->releaseModalFocus();
	gui_top->remove(wndSelectFile);

	delete cmdOK;
	delete cmdCancel;
	delete fileButtonActionListener;

	delete txtCurrent;
	delete lstFiles;
	delete scrAreaFiles;
	delete selectFileActionListener;
	delete editFilePathActionListener;

	delete fileList;
	if (createNew)
	{
		delete lblFilename;
		delete txtFilename;
	}

	delete wndSelectFile;
}

static void navigate_right()
{
	const auto* const focus_hdl = gui_top->_getFocusHandler();
	const auto* const active_widget = focus_hdl->getFocused();
	if (active_widget == lstFiles)
		if (createNew)
			txtFilename->requestFocus();
		else
			cmdOK->requestFocus();
	else if (active_widget == txtFilename)
		cmdOK->requestFocus();
	else if (active_widget == cmdCancel)
		lstFiles->requestFocus();
	else if (active_widget == cmdOK)
		cmdCancel->requestFocus();
}

static void navigate_left()
{
	const auto* const focus_hdl = gui_top->_getFocusHandler();
	const auto* const active_widget = focus_hdl->getFocused();
	if (active_widget == lstFiles)
		cmdCancel->requestFocus();
	else if (active_widget == cmdCancel)
		cmdOK->requestFocus();
	else if (active_widget == cmdOK)
		if (createNew)
			txtFilename->requestFocus();
		else
			lstFiles->requestFocus();
	else if (active_widget == txtFilename)
		lstFiles->requestFocus();
}

static void SelectFileLoop()
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
			if (active_widget == txtCurrent || active_widget == txtFilename)
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

std::string SelectFile(const std::string& title, std::string value, const char* filter[], const bool create)
{
	const AmigaMonitor* mon = &AMonitors[0];

	dialogResult = false;
	dialogFinished = false;
	createNew = create;
	filefilter = filter;
	selectedOnStart = -1;

	InitSelectFile(title);

	workingDir = extract_path(value);
	checkfoldername(workingDir);
	checkfilename(value);

	if (selectedOnStart >= 0 && fileList->getNumberOfElements() > 32)
	{
		scrAreaFiles->setVerticalScrollAmount(selectedOnStart * 15);
	}

	// Prepare the screen once
	uae_gui->logic();
	SDL_RenderClear(mon->gui_renderer);
	uae_gui->draw();
	update_gui_screen();

	while (!dialogFinished)
	{
		const auto start = SDL_GetPerformanceCounter();
		SelectFileLoop();
		cap_fps(start);
	}

	ExitSelectFile();

	if (dialogResult)
		value = workingDir;
	else 		
		value = "";

	return value;
}
