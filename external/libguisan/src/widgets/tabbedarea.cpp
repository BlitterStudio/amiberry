/*      _______   __   __   __   ______   __   __   _______   __   __
 *     / _____/\ / /\ / /\ / /\ / ____/\ / /\ / /\ / ___  /\ /  |\/ /\
 *    / /\____\// / // / // / // /\___\// /_// / // /\_/ / // , |/ / /
 *   / / /__   / / // / // / // / /    / ___  / // ___  / // /| ' / /
 *  / /_// /\ / /_// / // / // /_/_   / / // / // /\_/ / // / |  / /
 * /______/ //______/ //_/ //_____/\ /_/ //_/ //_/ //_/ //_/ /|_/ /
 * \______\/ \______\/ \_\/ \_____\/ \_\/ \_\/ \_\/ \_\/ \_\/ \_\/
 *
 * Copyright (c) 2004, 2005, 2006, 2007 Olof Naessén and Per Larsson
 *
 *                                                         Js_./
 * Per Larsson a.k.a finalman                          _RqZ{a<^_aa
 * Olof Naessén a.k.a jansem/yakslem                _asww7!uY`>  )\a//
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

#include "guisan/widgets/tabbedarea.hpp"

#include "guisan/exception.hpp"
#include "guisan/focushandler.hpp"
#include "guisan/font.hpp"
#include "guisan/graphics.hpp"

#include "guisan/widgets/container.hpp"
#include "guisan/widgets/tab.hpp"

#include <algorithm>

namespace gcn
{
	TabbedArea::TabbedArea()
		: mSelectedTab(nullptr)
	{
		setBorderSize(1);
		setFocusable(true);
		addKeyListener(this);
		addMouseListener(this);

		mTabContainer = new Container();
		mTabContainer->setOpaque(false);
		mWidgetContainer = new Container();

		BasicContainer::add(mTabContainer);
		BasicContainer::add(mWidgetContainer);
	}

	TabbedArea::~TabbedArea()
	{
		BasicContainer::remove(mTabContainer);
		BasicContainer::remove(mWidgetContainer);

		delete mTabContainer;
		delete mWidgetContainer;

		for (auto& i : mTabsToCleanUp)
		{
			delete i;
		}
	}

	void TabbedArea::addTab(const std::string& caption, Widget* widget)
	{
		auto* tab = new Tab();
		tab->setSize(70, 20);
		tab->setCaption(caption);
		mTabsToCleanUp.push_back(tab);

		addTab(tab, widget);
	}

	void TabbedArea::addTab(Tab* tab, Widget* widget)
	{
		tab->setTabbedArea(this);
		tab->addActionListener(this);

		mTabContainer->add(tab);
		mTabs.emplace_back(tab, widget);


		if (mSelectedTab == nullptr)
		{
			setSelectedTab(tab);
		}

		adjustTabPositions();
		adjustSize();
	}

	void TabbedArea::removeTabWithIndex(unsigned int index)
	{
		if (index >= mTabs.size())
		{
			throw GCN_EXCEPTION("No such tab index.");
		}

		removeTab(mTabs[index].first);
	}

	void TabbedArea::removeTab(Tab* tab)
	{
		auto tabIndexToBeSelected = - 1;

		if (tab == mSelectedTab)
		{
			auto index = getSelectedTabIndex();

			if (index == static_cast<int>(mTabs.size()) - 1
				&& mTabs.size() >= 2)
			{
				tabIndexToBeSelected = index--;
			}
			else if (index == static_cast<int>(mTabs.size()) - 1
				&& mTabs.size() == 1)
			{
				tabIndexToBeSelected = -1;
			}
			else
			{
				tabIndexToBeSelected = index;
			}
		}

		for (auto iter = mTabs.begin(); iter != mTabs.end(); ++iter)
		{
			if (iter->first == tab)
			{
				mTabContainer->remove(tab);
				mTabs.erase(iter);
				break;
			}
		}

		for (auto iter2 = mTabsToCleanUp.begin(); iter2 != mTabsToCleanUp.end(); ++iter2)
		{
			if (*iter2 == tab)
			{
				mTabsToCleanUp.erase(iter2);
				delete tab;
				break;
			}
		}

		if (tabIndexToBeSelected == -1)
		{
			mSelectedTab = nullptr;
			mWidgetContainer->clear();
		}
		else
		{
			setSelectedTabWithIndex(tabIndexToBeSelected);
		}

		adjustSize();
		adjustTabPositions();
	}

	bool TabbedArea::isTabSelected(unsigned int index) const
	{
		if (index >= mTabs.size())
		{
			throw GCN_EXCEPTION("No such tab index.");
		}

		return mSelectedTab == mTabs[index].first;
	}

	bool TabbedArea::isTabSelected(Tab* tab)
	{
		return mSelectedTab == tab;
	}

	void TabbedArea::setSelectedTabWithIndex(unsigned int index)
	{
		if (index >= mTabs.size())
		{
			throw GCN_EXCEPTION("No such tab index.");
		}

		setSelectedTab(mTabs[index].first);
	}

	void TabbedArea::setSelectedTab(Tab* tab)
	{
		unsigned int i;
		for (i = 0; i < mTabs.size(); i++)
		{
			if (mTabs[i].first == mSelectedTab)
			{
				mWidgetContainer->remove(mTabs[i].second);
			}
		}

		for (i = 0; i < mTabs.size(); i++)
		{
			if (mTabs[i].first == tab)
			{
				mSelectedTab = tab;
				mWidgetContainer->add(mTabs[i].second);
			}
		}
	}

	int TabbedArea::getSelectedTabIndex() const
	{
		for (auto i = 0; i < static_cast<int>(mTabs.size()); i++)
		{
			if (mTabs[i].first == mSelectedTab)
			{
				return i;
			}
		}

		return -1;
	}

	Tab* TabbedArea::getSelectedTab()
	{
		return mSelectedTab;
	}


	void TabbedArea::draw(Graphics* graphics)
	{
		graphics->setColor(getBaseColor() + 0x303030);
		graphics->drawLine(0,
						   mTabContainer->getHeight(),
						   getWidth(),
						   mTabContainer->getHeight());

		if (mSelectedTab != nullptr)
		{
			graphics->setColor(getBaseColor());
			graphics->drawLine(mSelectedTab->getX(),
							   mTabContainer->getHeight(),
							   mSelectedTab->getX() + mSelectedTab->getWidth(),
							   mTabContainer->getHeight());
		}

		drawChildren(graphics);
	}

	void TabbedArea::drawBorder(Graphics* graphics)
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
			graphics->setColor(highlightColor);
			graphics->drawLine(i, i + mWidgetContainer->getY(), i, height - i - 1);
			graphics->setColor(shadowColor);
			graphics->drawLine(width - i, i + 1 + mWidgetContainer->getY(), width - i, height - i);
			graphics->drawLine(i, height - i, width - i - 1, height - i);
		}
	}


	void TabbedArea::logic()
	{
	}

	void TabbedArea::adjustSize()
	{
		auto max_tab_height = 0;
		for (auto& mTab : mTabs)
		{
			if (mTab.first->getHeight() > max_tab_height)
			{
				max_tab_height = mTab.first->getHeight();
			}
		}

		if (getHeight() < max_tab_height)
		{
			mTabContainer->setHeight(max_tab_height);
		}
		else
		{
			mTabContainer->setHeight(max_tab_height);
			mWidgetContainer->setHeight(getHeight() - max_tab_height - 1);
			mWidgetContainer->setY(max_tab_height + 1);
		}

		mTabContainer->setWidth(getWidth());
		mWidgetContainer->setWidth(getWidth());
	}

	void TabbedArea::adjustTabPositions()
	{
		auto max_tab_height = 0;
		unsigned int i;
		for (i = 0; i < mTabs.size(); i++)
		{
			if (mTabs[i].first->getHeight() > max_tab_height)
			{
				max_tab_height = mTabs[i].first->getHeight();
			}
		}

		auto x = 0;
		for (i = 0; i < mTabs.size(); i++)
		{
			auto* tab = mTabs[i].first;

			if (x == 0)
			{
				x = tab->getBorderSize() + 2;
			}

			tab->setX(x);

			if (tab->getHeight() < max_tab_height)
			{
				tab->setY(max_tab_height
					- tab->getHeight()
					+ static_cast<int>(tab->getBorderSize()));
			}
			else
			{
				tab->setY(static_cast<int>(mTabs[i].first->getBorderSize()));
			}

			x += tab->getWidth() + tab->getBorderSize() * 2;
		}
	}

	void TabbedArea::setWidth(int width)
	{
		Widget::setWidth(width);
		adjustSize();
	}


	void TabbedArea::setHeight(int height)
	{
		Widget::setHeight(height);
		adjustSize();
	}

	void TabbedArea::setSize(int width, int height)
	{
		setWidth(width);
		setHeight(height);
	}

	void TabbedArea::setDimension(const Rectangle& dimension)
	{
		setX(dimension.x);
		setY(dimension.y);
		setWidth(dimension.width);
		setHeight(dimension.height);
	}

	void TabbedArea::keyPressed(KeyEvent& keyEvent)
	{
		if (keyEvent.isConsumed() || !isFocused())
		{
			return;
		}

		if (keyEvent.getKey().getValue() == Key::LEFT)
		{
			auto index = getSelectedTabIndex();
			index--;

			if (index < 0)
			{
				return;
			}
			setSelectedTab(mTabs[index].first);

			keyEvent.consume();
		}
		else if (keyEvent.getKey().getValue() == Key::RIGHT)
		{
			auto index = getSelectedTabIndex();
			index++;

			if (index >= static_cast<int>(mTabs.size()))
			{
				return;
			}
			setSelectedTab(mTabs[index].first);

			keyEvent.consume();
		}
	}


	void TabbedArea::mousePressed(MouseEvent& mouseEvent)
	{
		if (mouseEvent.isConsumed()
			&& mouseEvent.getSource()->isFocusable())
		{
			return;
		}

		if (mouseEvent.getButton() == MouseEvent::LEFT)
		{
			auto* const widget = mTabContainer->getWidgetAt(mouseEvent.getX(), mouseEvent.getY());
			auto* const tab = dynamic_cast<Tab*>(widget);

			if (tab != nullptr)
			{
				setSelectedTab(tab);
			}
		}

		requestFocus();
	}

	void TabbedArea::death(const Event& event)
	{
		auto* const source = event.getSource();
		auto* const tab = dynamic_cast<Tab*>(source);

		if (tab != nullptr)
		{
			removeTab(tab);
		}
		else
		{
			BasicContainer::death(event);
		}
	}

	void TabbedArea::action(const ActionEvent& actionEvent)
	{
		auto* const source = actionEvent.getSource();
		auto* const tab = dynamic_cast<Tab*>(source);

		if (tab == nullptr)
		{
			throw GCN_EXCEPTION("Received an action from a widget that's not a tab!");
		}

		setSelectedTab(tab);
	}
}
