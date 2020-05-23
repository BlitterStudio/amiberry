/*      _______   __   __   __   ______   __   __   _______   __   __
 *     / _____/\ / /\ / /\ / /\ / ____/\ / /\ / /\ / ___  /\ /  |\/ /\
 *    / /\____\// / // / // / // /\___\// /_// / // /\_/ / // , |/ / /
 *   / / /__   / / // / // / // / /    / ___  / // ___  / // /| ' / /
 *  / /_// /\ / /_// / // / // /_/_   / / // / // /\_/ / // / |  / /
 * /______/ //______/ //_/ //_____/\ /_/ //_/ //_/ //_/ //_/ /|_/ /
 * \______\/ \______\/ \_\/ \_____\/ \_\/ \_\/ \_\/ \_\/ \_\/ \_\/
 *
 * Copyright (c) 2004, 2005, 2006, 2007 Olof Naess�n and Per Larsson
 *
 *                                                         Js_./
 * Per Larsson a.k.a finalman                          _RqZ{a<^_aa
 * Olof Naess�n a.k.a jansem/yakslem                _asww7!uY`>  )\a//
 *                                                 _Qhm`] _f "'c  1!5m
 * Visit: http://guichan.darkbits.org             )Qk<P ` _: :+' .'  "{[
 *                                               .)j(] .d_/ '-(  P .   S
 * License: (BSD)                                <Td/Z <fP"5(\"??"\a.  .L
 * Redistribution and use in source and          _dV>ws?a-?'      ._/L  #'
 * binary forms, with or without                 )4d[#7r, .   '     )d`)[
 * modification, are permitted provided         _Q-5'5W..j/?'   -?!\)cam'
 * that the following conditions are met:       j<<WP+k/);.        _W=j f
 * 1. Redistributions of source code must       .$%w\/]Q  . ."'  .  mj$
 *    retain the above copyright notice,        ]E.pYY(Q]>.   a     J@\
 *    this list of conditions and the           j(]1u<sE"L,. .   ./^ ]{a
 *    following disclaimer.                     4'_uomm\.  )L);-4     (3=
 * 2. Redistributions in binary form must        )_]X{Z('a_"a7'<a"a,  ]"[
 *    reproduce the above copyright notice,       #}<]m7`Za??4,P-"'7. ).m
 *    this list of conditions and the            ]d2e)Q(<Q(  ?94   b-  LQ/
 *    following disclaimer in the                <B!</]C)d_, '(<' .f. =C+m
 *    documentation and/or other materials      .Z!=J ]e []('-4f _ ) -.)m]'
 *    provided with the distribution.          .w[5]' _[ /.)_-"+?   _/ <W"
 * 3. Neither the name of Guichan nor the      :$we` _! + _/ .        j?
 *    names of its contributors may be used     =3)= _f  (_yQmWW$#(    "
 *    to endorse or promote products derived     -   W,  sQQQQmZQ#Wwa]..
 *    from this software without specific        (js, \[QQW$QWW#?!V"".
 *    prior written permission.                    ]y:.<\..          .
 *                                                 -]n w/ '         [.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT       )/ )/           !
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY         <  (; sac    ,    '
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING,               ]^ .-  %
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF            c <   r
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR            aga<  <La
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE          5%  )P'-3L
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR        _bQf` y`..)a
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          ,J?4P'.P"_(\?d'.,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES               _Pa,)!f/<[]/  ?"
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT      _2-..:. .r+_,.. .
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     ?a.<%"'  " -'.a_ _,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION)                     ^
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * For comments regarding functions please see the header file.
 */

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
	ListBox::ListBox()
	{
		mSelected = -1;
		mListModel = nullptr;
		mWrappingEnabled = false;
		Widget::setWidth(100);
		setFocusable(true);

		addMouseListener(this);
		addKeyListener(this);
	}

	ListBox::ListBox(ListModel* listModel)
	{
		mSelected = -1;
		mWrappingEnabled = false;
		Widget::setWidth(100);
		setListModel(listModel);
		setFocusable(true);

		addMouseListener(this);
		addKeyListener(this);
	}

	void ListBox::draw(Graphics* graphics)
	{
		graphics->setColor(getBackgroundColor());
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
		// we should drop rows in the beginning of the list as
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

		const auto inactive_color = Color(170, 170, 170);
		
		// The y coordinate where we start to draw the text is
		// simply the y coordinate multiplied with the font height.
		auto y = rowHeight * startRow;
		
		for (auto i = startRow; i < startRow + numberOfRows; ++i)
		{
			if (i == mSelected)
			{
				if (isFocused())
					graphics->setColor(getSelectionColor());
				else
					graphics->setColor(inactive_color);
				graphics->fillRectangle(Rectangle(0, y, getWidth(), rowHeight));
				graphics->setColor(getForegroundColor());
			}

			// If the row height is greater than the font height we
			// draw the text with a center vertical alignment.
			if (rowHeight > getFont()->getHeight())
			{
				graphics->drawText(mListModel->getElementAt(i), 2, y + rowHeight / 2 - getFont()->getHeight() / 2);
			}
			else
			{
				graphics->drawText(mListModel->getElementAt(i), 2, y);
			}

			y += rowHeight;
		}
	}

	void ListBox::drawBorder(Graphics* graphics)
	{
		const auto faceColor = getBaseColor();
		const auto alpha = getBaseColor().a;
		const auto width = getWidth() + static_cast<int>(getBorderSize()) * 2 - 1;
		const auto height = getHeight() + static_cast<int>(getBorderSize()) * 2 - 1;
		auto highlightColor = faceColor + 0x303030;
		highlightColor.a = alpha;
		auto shadowColor = faceColor - 0x303030;
		shadowColor.a = alpha;

		for (auto i = 0; i < static_cast<int>(getBorderSize()); ++i)
		{
			graphics->setColor(shadowColor);
			graphics->drawLine(i, i, width - i, i);
			graphics->drawLine(i, i + 1, i, height - i - 1);
			graphics->setColor(highlightColor);
			graphics->drawLine(width - i, i + 1, width - i, height - i);
			graphics->drawLine(i, height - i, width - i - 1, height - i);
		}
	}

	void ListBox::logic()
	{
		adjustSize();
	}

	int ListBox::getSelected() const
	{
		return mSelected;
	}

	void ListBox::setSelected(int selected)
	{
		if (mListModel == nullptr)
		{
			mSelected = -1;
		}
		else
		{
			if (selected < 0)
			{
				mSelected = -1;
			}
			else if (selected >= mListModel->getNumberOfElements())
			{
				mSelected = mListModel->getNumberOfElements() - 1;
			}
			else
			{
				mSelected = selected;
			}

			auto* par = getParent();
			if (par == nullptr)
			{
				return;
			}

			Rectangle scroll;

			if (mSelected < 0)
			{
				scroll.y = 0;
			}
			else
			{
				scroll.y = getFont()->getHeight() * mSelected;
			}

			scroll.height = getFont()->getHeight();
			par->showWidgetPart(this, scroll);
		}

		distributeValueChangedEvent();
	}

	void ListBox::keyPressed(KeyEvent& keyEvent)
	{
		const auto key = keyEvent.getKey();

		if (key.getValue() == Key::ENTER || key.getValue() == Key::SPACE)
		{
			generateAction();
			keyEvent.consume();
		}
		else if (key.getValue() == Key::UP)
		{
			setSelected(mSelected - 1);

			if (mSelected == -1)
			{
				if (mWrappingEnabled)
				{
					setSelected(getListModel()->getNumberOfElements() - 1);
				}
				else
				{
					setSelected(0);
				}
			}

			keyEvent.consume();
		}
		else if (key.getValue() == Key::DOWN)
		{
			if (mWrappingEnabled
				&& getSelected() == getListModel()->getNumberOfElements() - 1)
			{
				setSelected(0);
			}
			else
			{
				setSelected(getSelected() + 1);
			}

			keyEvent.consume();
		}
		else if (key.getValue() == Key::HOME)
		{
			setSelected(0);
			keyEvent.consume();
		}
		else if (key.getValue() == Key::END)
		{
			setSelected(getListModel()->getNumberOfElements() - 1);
			keyEvent.consume();
		}
	}

	void ListBox::mousePressed(MouseEvent& mouseEvent)
	{
		if (mouseEvent.getButton() == MouseEvent::LEFT)
		{
			setSelected(mouseEvent.getY() / getFont()->getHeight());
			generateAction();
		}
	}

	void ListBox::mouseWheelMovedUp(MouseEvent& mouseEvent)
	{
		if (isFocused())
		{
			if (getSelected() > 0)
			{
				setSelected(getSelected() - 1);
			}

			mouseEvent.consume();
		}
	}

	void ListBox::mouseWheelMovedDown(MouseEvent& mouseEvent)
	{
		if (isFocused())
		{
			setSelected(getSelected() + 1);

			mouseEvent.consume();
		}
	}

	void ListBox::mouseDragged(MouseEvent& mouseEvent)
	{
		mouseEvent.consume();
	}

	void ListBox::setListModel(ListModel* listModel)
	{
		mSelected = -1;
		mListModel = listModel;
		adjustSize();
	}

	ListModel* ListBox::getListModel() const
	{
		return mListModel;
	}

	void ListBox::adjustSize()
	{
		if (mListModel != nullptr)
		{
			setHeight(getFont()->getHeight() * mListModel->getNumberOfElements());
		}
	}

	bool ListBox::isWrappingEnabled() const
	{
		return mWrappingEnabled;
	}

	void ListBox::setWrappingEnabled(bool wrappingEnabled)
	{
		mWrappingEnabled = wrappingEnabled;
	}

	void ListBox::addSelectionListener(SelectionListener* selectionListener)
	{
		mSelectionListeners.push_back(selectionListener);
	}

	void ListBox::removeSelectionListener(SelectionListener* selectionListener)
	{
		mSelectionListeners.remove(selectionListener);
	}

	void ListBox::distributeValueChangedEvent()
	{
		for (auto& mSelectionListener : mSelectionListeners)
		{
			SelectionEvent event(this);
			mSelectionListener->valueChanged(event);
		}
	}
}
