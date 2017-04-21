#include <algorithm>
#include <guisan.hpp>
#include <iostream>
#include <sstream>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include "guisan/sdl/sdltruetypefont.hpp"
#include "SelectorEntry.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "uae.h"
#include "gui_handling.h"

#define DIALOG_WIDTH 520
#define DIALOG_HEIGHT 400

std::string volName;
static bool dialogResult = false;
static bool dialogFinished = false;
static char workingDir[MAX_PATH];

static gcn::Window* wndSelectFolder;
static gcn::Button* cmdOK;
static gcn::Button* cmdCancel;
static gcn::ListBox* lstFolders;
static gcn::ScrollArea* scrAreaFolders;
static gcn::TextField* txtCurrent;


class ButtonActionListener : public gcn::ActionListener
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

static ButtonActionListener* buttonActionListener;


class DirListModel : public gcn::ListModel
{
	vector<string> dirs;

public:
	DirListModel(const char* path)
	{
		changeDir(path);
	}

	int getNumberOfElements() override
	{
		return dirs.size();
	}

	string getElementAt(int i) override
	{
		if (i >= dirs.size() || i < 0)
			return "---";
		return dirs[i];
	}

	void changeDir(const char* path)
	{
		ReadDirectory(path, &dirs, nullptr);
		if (dirs.size() == 0)
			dirs.push_back("..");
	}
};

static DirListModel dirList(".");


static void checkfoldername(char* current)
{
	char* ptr;
	char actualpath[PATH_MAX];
	DIR* dir;

	if ((dir = opendir(current)))
	{
		dirList = current;
		ptr = realpath(current, actualpath);
		strcpy(workingDir, ptr);
		closedir(dir);
	}
	else
		strcpy(workingDir, start_path_data);
	txtCurrent->setText(workingDir);
}


class ListBoxActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		int selected_item;
		char foldername[MAX_PATH] = "";

		selected_item = lstFolders->getSelected();
		strcpy(foldername, workingDir);
		strcat(foldername, "/");
		strcat(foldername, dirList.getElementAt(selected_item).c_str());
		volName = dirList.getElementAt(selected_item).c_str();
		checkfoldername(foldername);
	}
};

static ListBoxActionListener* listBoxActionListener;


static void InitSelectFolder(const char* title)
{
	wndSelectFolder = new gcn::Window("Load");
	wndSelectFolder->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
	wndSelectFolder->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
	wndSelectFolder->setBaseColor(gui_baseCol);
	wndSelectFolder->setCaption(title);
	wndSelectFolder->setTitleBarHeight(TITLEBAR_HEIGHT);

	buttonActionListener = new ButtonActionListener();

	cmdOK = new gcn::Button("Ok");
	cmdOK->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH - DISTANCE_NEXT_X, DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdOK->setBaseColor(gui_baseCol);
	cmdOK->addActionListener(buttonActionListener);

	cmdCancel = new gcn::Button("Cancel");
	cmdCancel->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdCancel->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH, DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdCancel->setBaseColor(gui_baseCol);
	cmdCancel->addActionListener(buttonActionListener);

	txtCurrent = new gcn::TextField();
	txtCurrent->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER - 4, TEXTFIELD_HEIGHT);
	txtCurrent->setPosition(DISTANCE_BORDER, 10);
	txtCurrent->setEnabled(false);

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
	scrAreaFolders->setScrollbarWidth(20);
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
	delete buttonActionListener;

	delete txtCurrent;
	delete lstFolders;
	delete scrAreaFolders;
	delete listBoxActionListener;

	delete wndSelectFolder;
}


static void SelectFolderLoop()
{
	while (!dialogFinished)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_KEYDOWN)
			{
				switch (event.key.keysym.scancode)
				{
				case VK_ESCAPE:
					dialogFinished = true;
					break;

				case VK_LEFT:
					{
						gcn::FocusHandler* focusHdl = gui_top->_getFocusHandler();
						gcn::Widget* activeWidget = focusHdl->getFocused();
						if (activeWidget == lstFolders)
							cmdCancel->requestFocus();
						else if (activeWidget == cmdCancel)
							cmdOK->requestFocus();
						else if (activeWidget == cmdOK)
							lstFolders->requestFocus();
					}
					break;

				case VK_RIGHT:
					{
						gcn::FocusHandler* focusHdl = gui_top->_getFocusHandler();
						gcn::Widget* activeWidget = focusHdl->getFocused();
						if (activeWidget == lstFolders)
							cmdOK->requestFocus();
						else if (activeWidget == cmdCancel)
							lstFolders->requestFocus();
						else if (activeWidget == cmdOK)
							cmdCancel->requestFocus();
					}
					break;

				case VK_Red:
				case VK_Green:
					event.key.keysym.scancode = SDL_SCANCODE_RETURN;
					gui_input->pushInput(event); // Fire key down
					event.type = SDL_KEYUP; // and the key up
					break;
				default: 
					break;
				}
			}

			//-------------------------------------------------
			// Send event to guisan-controls
			//-------------------------------------------------
			gui_input->pushInput(event);
		}

		// Now we let the Gui object perform its logic.
		uae_gui->logic();
		// Now we let the Gui object draw itself.
		uae_gui->draw();
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
	SelectFolderLoop();
	ExitSelectFolder();
	if (dialogResult)
	{
		strncpy(value, workingDir, sizeof value);
		if (value[strlen(value) - 1] != '/')
			strcat(value, "/");
	}
	return dialogResult;
}
