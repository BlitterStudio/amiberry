#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "include/memory.h"
#include "uae.h"
#include "autoconf.h"
#include "filesys.h"
#include "blkdev.h"
#include "gui_handling.h"

enum { COL_TYPE, COL_NAME, COL_START, COL_END, COL_SIZE, COL_ID, COL_COUNT };

static const char* column_caption[] =
{
	"Type",
	"Name",
	"Start",
	"End",
	"Size",
	"ID"
};
static const int column_size[] =
{
	40, // Type
	200, // Name
	100, // Start
	100, // End
	100, // Size
	100 // ID
};

static gcn::Window* grpHwInfo;
static gcn::Label* lblList[COL_COUNT];
static gcn::Container* listEntry[MAX_INFOS];
static gcn::TextField* listCells[MAX_INFOS][COL_COUNT];
static gcn::ScrollArea* scrlHwInfo;
static gcn::CheckBox* chkCustomBoardOrder;
static gcn::Button* cmdMoveUp;
static gcn::Button* cmdMoveDown;

static int selected = -1;

static void BoardsEnable(const int selected_board)
{
	bool move_up = expansion_can_move(&changed_prefs, selected_board);
	bool move_down = move_up;
	if (move_up) {
		if (expansion_autoconfig_move(&changed_prefs, selected_board, -1, true) < 0)
			move_up = false;
	}
	if (move_down) {
		if (expansion_autoconfig_move(&changed_prefs, selected_board, 1, true) < 0)
			move_down = false;
	}
	cmdMoveUp->setEnabled(move_up);
	cmdMoveDown->setEnabled(move_down);
}

class HWInfoActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		const auto source = actionEvent.getSource();

		if (source == chkCustomBoardOrder)
		{
			changed_prefs.autoconfig_custom_sort = chkCustomBoardOrder->isSelected();
			expansion_set_autoconfig_sort(&changed_prefs);
		}
		else if (source == cmdMoveUp)
		{
			if (selected >= 0)
			{
				int newpos = expansion_autoconfig_move(&changed_prefs, selected, -1, false);
				if (newpos >= 0)
				{
					selected = newpos;
					BoardsEnable(selected);
					listCells[selected][COL_NAME]->requestFocus();
				}
			}
		}
		else if (source == cmdMoveDown)
		{
			if (selected >= 0 && selected < MAX_INFOS - 1)
			{
				int newpos = expansion_autoconfig_move(&changed_prefs, selected, 1, false);
				if (newpos >= 0)
				{
					selected = newpos;
					BoardsEnable(selected);
					listCells[selected][COL_NAME]->requestFocus();
				}
			}
		}

		RefreshPanelHWInfo();
	}
};
static HWInfoActionListener* hwInfoActionListener;

class HWInfoFocusListener : public gcn::FocusListener
{
public:
	void focusGained(const gcn::Event& event) override
	{
		const auto source = event.getSource();
		for (int row = 0; row < MAX_INFOS; ++row)
		{
			for (int col = 0; col < COL_COUNT; ++col)
			{
				if (source == listCells[row][col]
					&& !listCells[row][COL_NAME]->getText().empty())
				{
					selected = row;
					BoardsEnable(selected);
					break;
				}
			}
		}
		RefreshPanelHWInfo();
	}
};
static HWInfoFocusListener* hwInfoFocusListener;

void RefreshPanelHWInfo()
{
	uaecptr highest_expamem = 0;

	expansion_generate_autoconfig_info(&changed_prefs);

	int row = 0;
	for (;;)
	{
		TCHAR tmp[200];
		struct autoconfig_info* aci = expansion_get_autoconfig_data(&changed_prefs, row);
		if (aci)
		{
			if (aci->zorro == 3 && aci->size != 0 && aci->start + aci->size > highest_expamem)
				highest_expamem = aci->start + aci->size;
		}
		if (!aci && highest_expamem <= Z3BASE_UAE)
			break;
		if (aci && aci->zorro >= 1 && aci->zorro <= 3)
			_sntprintf(tmp, sizeof tmp, _T("Z%d"), aci->zorro);
		else
			_tcscpy(tmp, _T("-"));

		listCells[row][COL_TYPE]->setText(tmp);
		tmp[0] = 0;

		if (aci && aci->parent_of_previous) {
			_tcscat(tmp, _T(" - "));
		}
		if (aci && (aci->parent_address_space || aci->parent_romtype) && !aci->parent_of_previous) {
			_tcscat(tmp, _T("? "));
		}
		if (aci && aci->name[0])
			_tcscat(tmp, aci->name);
		listCells[row][COL_NAME]->setText(tmp);

		if (aci)
		{
			if (aci->start != 0xffffffff)
				_sntprintf(tmp, sizeof tmp, _T("0x%08x"), aci->start);
			else
				_tcscpy(tmp, _T("-"));
			listCells[row][COL_START]->setText(tmp);

			if (aci->size != 0)
				_sntprintf(tmp, sizeof tmp, _T("0x%08x"), aci->start + aci->size - 1);
			else
				_tcscpy(tmp, _T("-"));
			listCells[row][COL_END]->setText(tmp);

			if (aci->size != 0)
				_sntprintf(tmp, sizeof tmp, _T("0x%08x"), aci->size);
			else
				_tcscpy(tmp, _T("-"));
			listCells[row][COL_SIZE]->setText(tmp);

			if (aci->autoconfig_bytes[0] != 0xff)
				_sntprintf(tmp, sizeof tmp, _T("0x%04x/0x%02x"),
					(aci->autoconfig_bytes[4] << 8) | aci->autoconfig_bytes[5], aci->autoconfig_bytes[1]);
			else
				_tcscpy(tmp, _T("-"));
			listCells[row][COL_ID]->setText(tmp);
		}
		else
		{
			_sntprintf(tmp, sizeof tmp, _T("0x%08x"), highest_expamem);
			listCells[row][COL_START]->setText(tmp);
			listCells[row][COL_END]->setText("");
			listCells[row][COL_SIZE]->setText("");
			listCells[row][COL_ID]->setText("");
		}
		row++;
		if (!aci)
			break;
	}

	for (; row < MAX_INFOS; ++row)
	{
		for (int col = 0; col < COL_COUNT; ++col)
			listCells[row][col]->setText("");
	}

	chkCustomBoardOrder->setSelected(changed_prefs.autoconfig_custom_sort);
	cmdMoveUp->setEnabled(chkCustomBoardOrder->isSelected() && selected >= 0);
	cmdMoveDown->setEnabled(chkCustomBoardOrder->isSelected() && selected >= 0 && selected < MAX_INFOS - 1);
}

void InitPanelHWInfo(const config_category& category)
{
	int row, col;
	int posY = DISTANCE_BORDER;

	hwInfoActionListener = new HWInfoActionListener();
	hwInfoFocusListener = new HWInfoFocusListener();

	grpHwInfo = new gcn::Window("Hardware Information");
	grpHwInfo->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	grpHwInfo->setBaseColor(gui_base_color);
	grpHwInfo->setForegroundColor(gui_foreground_color);
	grpHwInfo->setTitleBarHeight(1);

	for (col = 0; col < COL_COUNT; ++col)
		lblList[col] = new gcn::Label(column_caption[col]);

	for (row = 0; row < MAX_INFOS; ++row)
	{
		listEntry[row] = new gcn::Container();
		listEntry[row]->setSize(category.panel->getWidth() + 50, TEXTFIELD_HEIGHT + 4);
		listEntry[row]->setBaseColor(gui_base_color);
		listEntry[row]->setForegroundColor(gui_foreground_color);
		listEntry[row]->setFrameSize(0);

		for (col = 0; col < COL_COUNT; ++col)
		{
			listCells[row][col] = new gcn::TextField();
			listCells[row][col]->setSize(column_size[col] - 8, TEXTFIELD_HEIGHT);
			listCells[row][col]->setEnabled(true);
			listCells[row][col]->setBaseColor(gui_base_color);
			listCells[row][col]->setBackgroundColor(gui_background_color);
			listCells[row][col]->setForegroundColor(gui_foreground_color);
			listCells[row][col]->addFocusListener(hwInfoFocusListener);
		}
	}

	int posX = DISTANCE_BORDER;
	for (col = 0; col < COL_COUNT; ++col)
	{
		grpHwInfo->add(lblList[col], posX, posY);
		posX += column_size[col];
	}
	posY += lblList[0]->getHeight() + 2;

	for (row = 0; row < MAX_INFOS; ++row)
	{
		posX = 0;
		for (col = 0; col < COL_COUNT; ++col)
		{
			listEntry[row]->add(listCells[row][col], posX, 2);
			posX += column_size[col];
		}
		grpHwInfo->add(listEntry[row], DISTANCE_BORDER, posY);
		posY += listEntry[row]->getHeight() + 4;
	}
	grpHwInfo->setSize(category.panel->getWidth() + 50, category.panel->getHeight() - DISTANCE_BORDER * 8);

	scrlHwInfo = new gcn::ScrollArea(grpHwInfo);
	scrlHwInfo->setBackgroundColor(gui_base_color);
	scrlHwInfo->setForegroundColor(gui_foreground_color);
	scrlHwInfo->setSelectionColor(gui_selection_color);
	scrlHwInfo->setBaseColor(gui_base_color);
	scrlHwInfo->setWidth(category.panel->getWidth() - DISTANCE_BORDER * 2);
	scrlHwInfo->setHeight(category.panel->getHeight() - DISTANCE_BORDER * 8);
	scrlHwInfo->setFrameSize(1);
	category.panel->add(scrlHwInfo, DISTANCE_BORDER, DISTANCE_BORDER);

	posY = scrlHwInfo->getHeight() + DISTANCE_NEXT_Y * 2;

	chkCustomBoardOrder = new gcn::CheckBox("Custom board order", true);
	chkCustomBoardOrder->setId("chkCustomBoardOrder");
	chkCustomBoardOrder->setBaseColor(gui_base_color);
	chkCustomBoardOrder->setBackgroundColor(gui_background_color);
	chkCustomBoardOrder->setForegroundColor(gui_foreground_color);
	chkCustomBoardOrder->addActionListener(hwInfoActionListener);

	cmdMoveUp = new gcn::Button("Move up");
	cmdMoveUp->setId("cmdMoveUp");
	cmdMoveUp->setSize(BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdMoveUp->setBaseColor(gui_base_color);
	cmdMoveUp->setBackgroundColor(gui_background_color);
	cmdMoveUp->setForegroundColor(gui_foreground_color);
	cmdMoveUp->setSelectionColor(gui_selection_color);
	cmdMoveUp->addActionListener(hwInfoActionListener);

	cmdMoveDown = new gcn::Button("Move down");
	cmdMoveDown->setId("cmdMoveDown");
	cmdMoveDown->setSize(BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdMoveDown->setBaseColor(gui_base_color);
	cmdMoveDown->setBackgroundColor(gui_background_color);
	cmdMoveDown->setForegroundColor(gui_foreground_color);
	cmdMoveDown->setSelectionColor(gui_selection_color);
	cmdMoveDown->addActionListener(hwInfoActionListener);

	category.panel->add(chkCustomBoardOrder, DISTANCE_BORDER, posY);
	category.panel->add(cmdMoveUp, chkCustomBoardOrder->getX() + chkCustomBoardOrder->getWidth() + DISTANCE_NEXT_X * 3, posY);
	category.panel->add(cmdMoveDown, cmdMoveUp->getX() + cmdMoveUp->getWidth() + DISTANCE_NEXT_X, posY);

	RefreshPanelHWInfo();
}

void ExitPanelHWInfo()
{
	delete hwInfoActionListener;
	delete hwInfoFocusListener;

	int col;

	for (col = 0; col < COL_COUNT; ++col)
		delete lblList[col];

	for (auto row = 0; row < MAX_INFOS; ++row)
	{
		for (col = 0; col < COL_COUNT; ++col)
			delete listCells[row][col];
		delete listEntry[row];
	}

	delete scrlHwInfo;
	delete chkCustomBoardOrder;
	delete cmdMoveUp;
	delete cmdMoveDown;
	delete grpHwInfo;
}

bool HelpPanelHWInfo(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("This panel shows the information about the configured hardware being emulated.");
	helptext.emplace_back("You can change the order of the autoconfig boards by enabling the 'Custom board order' checkbox.");
	helptext.emplace_back("When enabled, you can use the 'Move up' and 'Move down' buttons to change the order of the boards.");
	return true;
}
