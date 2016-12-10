#include "UaeDropDown.hpp"
#include "guisan/widgets/dropdown.hpp"

#include "guisan/font.hpp"
#include "guisan/graphics.hpp"
#include "guisan/key.hpp"
#include "guisan/mouseinput.hpp"


namespace gcn
{
UaeDropDown::UaeDropDown(ListModel *listModel,
                         ScrollArea *scrollArea,
                         ListBox *listBox)
    : DropDown(listModel, scrollArea, listBox)
{
    mScrollArea->setScrollbarWidth(20);
}


UaeDropDown::~UaeDropDown()
{
}


void UaeDropDown::keyPressed(KeyEvent& keyEvent)
{
    if (keyEvent.isConsumed())
        return;

    Key key = keyEvent.getKey();

    if ((key.getValue() == Key::ENTER || key.getValue() == Key::SPACE)
            && !mDroppedDown)
    {
        dropDown();
        keyEvent.consume();
    }
    else if (key.getValue() == Key::UP)
    {
        setSelected(getSelected() - 1);
        keyEvent.consume();
	    distributeValueChangedEvent();
    }
    else if (key.getValue() == Key::DOWN)
    {
        setSelected(getSelected() + 1);
        keyEvent.consume();
	    distributeValueChangedEvent();
    }
}

void UaeDropDown::clearSelected(void)
{
    mListBox->setSelected(-1);
}

bool UaeDropDown::isDroppedDown(void)
{
    return mDroppedDown;
}

void UaeDropDown::setEnabled(bool enabled)
{
    if(mEnabled != enabled)
    {
        mEnabled = enabled;
        if(mEnabled)
            mBackgroundColor = mBackgroundColorBackup;
        else
        {
            mBackgroundColorBackup = mBackgroundColor;
            mBackgroundColor = mBackgroundColor - 0x303030;
        }
    }
}
}
