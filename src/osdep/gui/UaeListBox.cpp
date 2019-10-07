#include "UaeListBox.hpp"

#include <guisan/widgets/listbox.hpp>
#include <guisan/basiccontainer.hpp>
#include <guisan/font.hpp>
#include <guisan/graphics.hpp>
#include <guisan/listmodel.hpp>

namespace gcn
{
	Color colSelectorInactive;
	Color colSelectorActive;

	UaeListBox::UaeListBox()
	= default;

	UaeListBox::UaeListBox(ListModel* listModel)
		: ListBox(listModel)
	{
	}

	UaeListBox::~UaeListBox()
	= default;

	void UaeListBox::draw(Graphics* graphics)
	{
		graphics->setColor(getBaseColor());
		graphics->fillRectangle(Rectangle(0, 0, getWidth(), getHeight()));

		if (mListModel == nullptr)
		{
			return;
		}

		graphics->setColor(getForegroundColor());
		graphics->setFont(getFont());

		// Check the current clip area so we don't draw unnecessary items
		// that are not visible.
		const auto currentClipArea = graphics->getCurrentClipArea();
		const int rowHeight = getFont()->getHeight();

		// Calculate the number of rows to draw by checking the clip area.
		// The addition of two makes covers a partial visible row at the top
		// and a partial visible row at the bottom.
		auto numberOfRows = currentClipArea.height / rowHeight + 2;

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

		colSelectorInactive.r = 170;
		colSelectorInactive.g = 170;
		colSelectorInactive.b = 170;
		colSelectorActive.r = 103;
		colSelectorActive.g = 136;
		colSelectorActive.b = 187;

		// The y coordinate where we start to draw the text is
		// simply the y coordinate multiplied with the font height.
		auto y = rowHeight * startRow;
		for (auto i = startRow; i < startRow + numberOfRows; ++i)
		{
			if (i == mSelected)
			{
				if (isFocused())
					graphics->setColor(colSelectorActive);
				else
					graphics->setColor(colSelectorInactive);
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
