#include "UaeDropDown.hpp"
#include "guichan/widgets/dropdown.hpp"

#include "guichan/font.hpp"
#include "guichan/graphics.hpp"
#include "guichan/key.hpp"
#include "guichan/mouseinput.hpp"


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
            distributeActionEvent();
        }
        else if (key.getValue() == Key::DOWN)
        {
            setSelected(getSelected() + 1);
            keyEvent.consume();
            distributeActionEvent();
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
}
