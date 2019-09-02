#include "UaeDropDown.hpp"
#ifdef USE_SDL1
#include "guichan/widgets/dropdown.hpp"

#include "guichan/font.hpp"
#include "guichan/graphics.hpp"
#include "guichan/key.hpp"
#include "guichan/mouseinput.hpp"
#elif USE_SDL2
#include <guisan/widgets/dropdown.hpp>
#include <guisan/key.hpp>
#endif

namespace gcn
{
	UaeDropDown::UaeDropDown(ListModel* listModel,
	                         ScrollArea* scrollArea,
	                         ListBox* listBox)
		: DropDown(listModel, scrollArea, listBox)
	{
#ifdef ANDROID
    mScrollArea->setScrollbarWidth(30);
#else
    mScrollArea->setScrollbarWidth(20);
#endif
	}


	UaeDropDown::~UaeDropDown()
	= default;


	void UaeDropDown::keyPressed(KeyEvent& keyEvent)
	{
		if (keyEvent.isConsumed())
			return;

		auto key = keyEvent.getKey();

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
#ifdef USE_SDL1
			distributeActionEvent();
#elif USE_SDL2
			distributeValueChangedEvent();
#endif
		}
		else if (key.getValue() == Key::DOWN)
		{
			setSelected(getSelected() + 1);
			keyEvent.consume();
#ifdef USE_SDL1
			distributeActionEvent();
#elif USE_SDL2
			distributeValueChangedEvent();
#endif
		}
	}

	void UaeDropDown::clearSelected() const
	{
		mListBox->setSelected(-1);
	}

	bool UaeDropDown::isDroppedDown() const
	{
		return mDroppedDown;
	}

	bool UaeDropDown::getEnabled() const
	{
		return mEnabled;
	}

	void UaeDropDown::setEnabled(bool enabled)
	{
		if (mEnabled != enabled)
		{
			mEnabled = enabled;
			if (mEnabled)
				mBackgroundColor = mBackgroundColorBackup;
			else
			{
				mBackgroundColorBackup = mBackgroundColor;
				mBackgroundColor = mBackgroundColor - 0x303030;
			}
		}
	}
}
