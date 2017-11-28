#include <algorithm>
#include <guisan.hpp>
#include <iostream>
#include <sstream>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include "SelectorEntry.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "uae.h"
#include "fsdb.h"
#include "gui.h"
#include "gui_handling.h"

#include "options.h"
#include "inputdevice.h"

#ifdef ANDROIDSDL
#include "androidsdl_event.h"
#endif
#define DIALOG_WIDTH 520
#define DIALOG_HEIGHT 400

#if defined(AMIBERRY) || defined(ANDROID)
#define FILE_SELECT_KEEP_POSITION
#endif

static SDL_Joystick *GUIjoy;
extern struct host_input_button host_input_buttons[MAX_INPUT_DEVICES];

static bool dialogResult = false;
static bool dialogFinished = false;
static bool createNew = false;
static char workingDir[MAX_PATH];
static const char** filefilter;
static bool dialogCreated = false;
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
	vector<string> dirs;
	vector<string> files;

public:
	SelectFileListModel(const char* path)
	{
		changeDir(path);
	}

	int getNumberOfElements() override
	{
		return dirs.size() + files.size();
	}

	string getElementAt(int i) override
	{
		if (i >= dirs.size() + files.size() || i < 0)
			return "---";
		if (i < dirs.size())
			return dirs[i];
		return files[i - dirs.size()];
	}

	void changeDir(const char* path)
	{
		ReadDirectory(path, &dirs, &files);
		if (dirs.size() == 0)
			dirs.push_back("..");
		FilterFiles(&files, filefilter);
	}

	bool isDir(int i) const
	{
		return (i < dirs.size());
	}
};

static SelectFileListModel* fileList;


class FileButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdOK)
		{
			int selected_item = lstFiles->getSelected();
			if (createNew)
			{
				char tmp[MAX_PATH];
				if (txtFilename->getText().length() <= 0)
					return;
				strncpy(tmp, workingDir, MAX_PATH - 1);
          			strncat(tmp, "/", MAX_PATH - 1);
				strncat(tmp, txtFilename->getText().c_str(), MAX_PATH - 1);
				if (strstr(tmp, filefilter[0]) == nullptr)
					strncat(tmp, filefilter[0], MAX_PATH - 1);
				if (my_existsfile(tmp) == 1)
					return; // File already exists
				strncpy(workingDir, tmp, MAX_PATH - 1);
				dialogResult = true;
			}
			else
			{
				if (fileList->isDir(selected_item))
					return; // Directory selected -> Ok not possible
				strncat(workingDir, "/", MAX_PATH - 1);
          			strncat(workingDir, fileList->getElementAt(selected_item).c_str(), MAX_PATH - 1);
				dialogResult = true;
			}
		}
		dialogFinished = true;
	}
};

static FileButtonActionListener* fileButtonActionListener;


static void checkfoldername(char* current)
{
	char actualpath[MAX_PATH];
	DIR* dir;

	if ((dir = opendir(current)))
	{
		fileList->changeDir(current);
		char * ptr = realpath(current, actualpath);
		strncpy(workingDir, ptr, MAX_PATH);
		closedir(dir);
	}
	else
		strncpy(workingDir, start_path_data, MAX_PATH);
	txtCurrent->setText(workingDir);
}

static void checkfilename(char* current)
{
	char actfile[MAX_PATH];
	extractFileName(current, actfile);
	for (int i = 0; i < fileList->getNumberOfElements(); ++i)
	{
		if (!fileList->isDir(i) && !strcasecmp(fileList->getElementAt(i).c_str(), actfile))
		{
			lstFiles->setSelected(i);
			selectedOnStart = i;
			break;
		}
	}
}


class SelectFileActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		char foldername[MAX_PATH] = "";

		int selected_item = lstFiles->getSelected();
		strncpy(foldername, workingDir, MAX_PATH);
      		strncat(foldername, "/", MAX_PATH - 1);
      		strncat(foldername, fileList->getElementAt(selected_item).c_str(), MAX_PATH - 1);
		if (fileList->isDir(selected_item))
			checkfoldername(foldername);
		else if (!createNew)
		{
			strncpy(workingDir, foldername, sizeof(workingDir));
			dialogResult = true;
			dialogFinished = true;
		}
	}
};

static SelectFileActionListener* selectFileActionListener;


static void InitSelectFile(const char* title)
{
	wndSelectFile = new gcn::Window("Load");
	wndSelectFile->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
	wndSelectFile->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
	wndSelectFile->setBaseColor(gui_baseCol);
	wndSelectFile->setCaption(title);
	wndSelectFile->setTitleBarHeight(TITLEBAR_HEIGHT);

	fileButtonActionListener = new FileButtonActionListener();

	cmdOK = new gcn::Button("Ok");
	cmdOK->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH - DISTANCE_NEXT_X, DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdOK->setBaseColor(gui_baseCol);
	cmdOK->addActionListener(fileButtonActionListener);

	cmdCancel = new gcn::Button("Cancel");
	cmdCancel->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdCancel->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH, DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdCancel->setBaseColor(gui_baseCol);
	cmdCancel->addActionListener(fileButtonActionListener);

	txtCurrent = new gcn::TextField();
	txtCurrent->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER - 4, TEXTFIELD_HEIGHT);
	txtCurrent->setPosition(DISTANCE_BORDER, 10);
	txtCurrent->setEnabled(false);

	selectFileActionListener = new SelectFileActionListener();
	fileList = new SelectFileListModel(".");

	lstFiles = new gcn::ListBox(fileList);
	lstFiles->setSize(800, 252);
	lstFiles->setBaseColor(gui_baseCol);
	lstFiles->setWrappingEnabled(true);
	lstFiles->addActionListener(selectFileActionListener);

	scrAreaFiles = new gcn::ScrollArea(lstFiles);
	scrAreaFiles->setBorderSize(1);
	scrAreaFiles->setPosition(DISTANCE_BORDER, 10 + TEXTFIELD_HEIGHT + 10);
	scrAreaFiles->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER - 4, 272);
	scrAreaFiles->setScrollbarWidth(20);
	scrAreaFiles->setBaseColor(gui_baseCol);

	if (createNew)
	{
		scrAreaFiles->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER - 4, 272 - TEXTFIELD_HEIGHT - DISTANCE_NEXT_Y);
		lblFilename = new gcn::Label("Filename:");
		lblFilename->setSize(80, LABEL_HEIGHT);
		lblFilename->setAlignment(gcn::Graphics::LEFT);
		lblFilename->setPosition(DISTANCE_BORDER, scrAreaFiles->getY() + scrAreaFiles->getHeight() + DISTANCE_NEXT_Y);
		txtFilename = new gcn::TextField();
		txtFilename->setSize(120, TEXTFIELD_HEIGHT);
		txtFilename->setId("Filename");
		txtFilename->setPosition(lblFilename->getX() + lblFilename->getWidth() + DISTANCE_NEXT_X, lblFilename->getY());

		wndSelectFile->add(lblFilename);
		wndSelectFile->add(txtFilename);
	}

	wndSelectFile->add(cmdOK);
	wndSelectFile->add(cmdCancel);
	wndSelectFile->add(txtCurrent);
	wndSelectFile->add(scrAreaFiles);

	gui_top->add(wndSelectFile);

	lstFiles->requestFocus();
	lstFiles->setSelected(0);
	wndSelectFile->requestModalFocus();
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
	delete fileList;
	if (createNew)
	{
		delete lblFilename;
		delete txtFilename;
	}

	delete wndSelectFile;
}


static void SelectFileLoop()
{
//TODO: is this needed in guisan?
	//FocusBugWorkaround(wndSelectFile);
	GUIjoy = SDL_JoystickOpen(0); 

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
						if (activeWidget == lstFiles)
							cmdCancel->requestFocus();
						else if (activeWidget == cmdCancel)
							cmdOK->requestFocus();
						else if (activeWidget == cmdOK)
							if (createNew)
								txtFilename->requestFocus();
							else
								lstFiles->requestFocus();
						else if (activeWidget == txtFilename)
							lstFiles->requestFocus();
					continue;
					}
					break;

				case VK_RIGHT:
					{
						gcn::FocusHandler* focusHdl = gui_top->_getFocusHandler();
						gcn::Widget* activeWidget = focusHdl->getFocused();
						if (activeWidget == lstFiles)
							if (createNew)
								txtFilename->requestFocus();
							else
								cmdOK->requestFocus();
						else if (activeWidget == txtFilename)
							cmdOK->requestFocus();
						else if (activeWidget == cmdCancel)
							lstFiles->requestFocus();
						else if (activeWidget == cmdOK)
							cmdCancel->requestFocus();
					continue;
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
	else if (event.type == SDL_JOYBUTTONDOWN || event.type == SDL_JOYHATMOTION)
      {
        int hat = SDL_JoystickGetHat(GUIjoy, 0);
        
            if (SDL_JoystickGetButton(GUIjoy,host_input_buttons[0].south_button))
                {PushFakeKey(SDLK_RETURN);
                 break;}
            
            else if (SDL_JoystickGetButton(GUIjoy,host_input_buttons[0].east_button) || 
                     SDL_JoystickGetButton(GUIjoy,host_input_buttons[0].start_button))
                {dialogFinished = true;
                 break;}
        
            else if (SDL_JoystickGetButton(GUIjoy,host_input_buttons[0].dpad_left) || (hat & SDL_HAT_LEFT))
                {
                  gcn::FocusHandler* focusHdl = gui_top->_getFocusHandler();
                  gcn::Widget* activeWidget = focusHdl->getFocused();
                  if(activeWidget == lstFiles)
                    cmdCancel->requestFocus();
                  else if(activeWidget == cmdCancel)
                    cmdOK->requestFocus();
                  else if(activeWidget == cmdOK)
                    if(createNew)
                      txtFilename->requestFocus();
                    else
                      lstFiles->requestFocus();
                  else if(activeWidget == txtFilename)
                    lstFiles->requestFocus();
                  continue;
                }
               
                          
            else if (SDL_JoystickGetButton(GUIjoy,host_input_buttons[0].dpad_right) || (hat & SDL_HAT_RIGHT))
                {
                  gcn::FocusHandler* focusHdl = gui_top->_getFocusHandler();
                  gcn::Widget* activeWidget = focusHdl->getFocused();
                  if(activeWidget == lstFiles)
                    if(createNew)
                      txtFilename->requestFocus();
                    else
                      cmdOK->requestFocus();
                  else if(activeWidget == txtFilename)
                    cmdOK->requestFocus();
                  else if(activeWidget == cmdCancel)
                    lstFiles->requestFocus();
                  else if(activeWidget == cmdOK)
                    cmdCancel->requestFocus();
                  continue;
                }
                     
            else if (SDL_JoystickGetButton(GUIjoy,host_input_buttons[0].dpad_up) || (hat & SDL_HAT_UP))
                {PushFakeKey(SDLK_UP);
                 break;}
            
            else if (SDL_JoystickGetButton(GUIjoy,host_input_buttons[0].dpad_down) || (hat & SDL_HAT_DOWN))
                {PushFakeKey(SDLK_DOWN);
                 break;}  
        
        
       }

			//-------------------------------------------------
			// Send event to guisan-controls
			//-------------------------------------------------
#ifdef ANDROIDSDL
        androidsdl_event(event, gui_input);
#else
        gui_input->pushInput(event);
#endif
		}

		// Now we let the Gui object perform its logic.
		uae_gui->logic();
		// Now we let the Gui object draw itself.
		uae_gui->draw();
		// Finally we update the screen.

		UpdateGuiScreen();

		if (!dialogCreated)
		{
			dialogCreated = true;
			if (selectedOnStart >= 0)
				scrAreaFiles->setVerticalScrollAmount(selectedOnStart * 19);
		}
	}
}

#ifdef FILE_SELECT_KEEP_POSITION
static int Already_init = 0;
#endif

bool SelectFile(const char* title, char* value, const char* filter[], bool create)
{
	dialogResult = false;
	dialogFinished = false;
	createNew = create;
	filefilter = filter;
	dialogCreated = false;
	selectedOnStart = -1;

#ifdef FILE_SELECT_KEEP_POSITION
	if (Already_init == 0)
	{
		InitSelectFile(title);
		Already_init = 1;
	}
	else
	{
		strncpy(value, workingDir, MAX_PATH);
		gui_top->add(wndSelectFile);
		wndSelectFile->setCaption(title);
		wndSelectFile->requestModalFocus();
		wndSelectFile->setVisible(true);
		gui_top->moveToTop(wndSelectFile);
	}
#else
	InitSelectFile(title);
#endif
	extractPath(value, workingDir);
	checkfoldername(workingDir);
	checkfilename(value);

	SelectFileLoop();
#ifdef FILE_SELECT_KEEP_POSITION
	wndSelectFile->releaseModalFocus();
	wndSelectFile->setVisible(false);
#else
	ExitSelectFile();
#endif
	if (dialogResult)
		strncpy(value, workingDir, MAX_PATH);
#ifdef FILE_SELECT_KEEP_POSITION
	else
		strncpy(workingDir, value, MAX_PATH);
#endif
	return dialogResult;
}
