#include "UaeListBox.hpp"
#include "guisan/widgets/listbox.hpp"

#include "guisan/basiccontainer.hpp"
#include "guisan/font.hpp"
#include "guisan/graphics.hpp"
#include "guisan/key.hpp"
#include "guisan/listmodel.hpp"
#include "guisan/mouseinput.hpp"
#include "guisan/selectionlistener.hpp"


namespace gcn
{

UaeListBox::UaeListBox()
    : ListBox()
{
}

UaeListBox::UaeListBox(ListModel *listModel)
    : ListBox(listModel)
{
}

UaeListBox::~UaeListBox()
{
}

void UaeListBox::draw(Graphics* graphics)
{
    graphics->setColor(getBackgroundColor());
    graphics->fillRectangle(Rectangle(0, 0, getWidth(), getHeight()));

    if (mListModel == NULL)
    {
        return;
    }

    graphics->setColor(getForegroundColor());
    graphics->setFont(getFont());

    // Check the current clip area so we don't draw unnecessary items
    // that are not visible.
    const ClipRectangle currentClipArea = graphics->getCurrentClipArea();
    int rowHeight = getHeight();

    // Calculate the number of rows to draw by checking the clip area.
    // The addition of two makes covers a partial visible row at the top
    // and a partial visible row at the bottom.
    int numberOfRows = currentClipArea.height / rowHeight + 2;

    if (numberOfRows > mListModel->getNumberOfElements())
    {
        numberOfRows = mListModel->getNumberOfElements();
    }

    // Calculate which row to start drawing. If the list box
    // has a negative y coordinate value we should check if
    // we should drop rows in the begining of the list as
    // they might not be visible. A negative y value is very
    // common if the list box for instance resides in a scroll
    // area and the user has scrolled the list box downwards.
    int startRow;
    if (getY() < 0)
    {
        startRow = -1 * (getY() / rowHeight);
    }
    else
    {
        startRow = 0;
    }

    int i;
    // The y coordinate where we start to draw the text is
    // simply the y coordinate multiplied with the font height.
    int y = rowHeight * startRow;
    for (i = startRow; i < startRow + numberOfRows; ++i)
    {
        if (i == mSelected)
        {
            if(isFocused())
                graphics->setColor(getSelectionColor());
            else
                graphics->setColor(0xd0d0d0);
            graphics->fillRectangle(Rectangle(0, y, getWidth(), rowHeight));
            graphics->setColor(getForegroundColor());
        }

        // If the row height is greater than the font height we
        // draw the text with a center vertical alignment.
        if (rowHeight > getFont()->getHeight())
        {
            graphics->drawText(mListModel->getElementAt(i), 1, y + rowHeight / 2 - getFont()->getHeight() / 2);
        }
        else
        {
            graphics->drawText(mListModel->getElementAt(i), 1, y);
        }

        y += rowHeight;
    }
}

}
