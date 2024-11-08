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
    TabbedArea::TabbedArea() :
        mTabContainer(std::make_unique<Container>()),
        mWidgetContainer(std::make_unique<Container>())
    {
        setFrameSize(1);
        setFocusable(true);

        addKeyListener(this);
        addMouseListener(this);

        mTabContainer->setOpaque(false);

        add(mTabContainer.get());
        add(mWidgetContainer.get());
    }

    TabbedArea::~TabbedArea()
    {
        remove(mTabContainer.get());
        remove(mWidgetContainer.get());
    }

    void TabbedArea::addTab(const std::string& caption, Widget* widget)
    {
        mTabsToDelete.emplace_back(std::make_unique<Tab>());
        auto tab = mTabsToDelete.back().get();
        tab->setCaption(caption);

        addTab(tab, widget);
    }

    void TabbedArea::addTab(Tab* tab, Widget* widget)
    {
        tab->setTabbedArea(this);
        tab->addActionListener(this);

        mTabContainer->add(tab);
        mTabs.push_back(std::pair<Tab*, Widget*>(tab, widget));

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
        int tabIndexToBeSelected = - 1;

        if (tab == mSelectedTab)
        {
            int index = getSelectedTabIndex();

            if (index == (int)mTabs.size() - 1
                && mTabs.size() >= 2)
            {
                tabIndexToBeSelected = index--;
            }
            else if (index == (int)mTabs.size() - 1
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

        auto it2 = std::find_if(mTabsToDelete.begin(), mTabsToDelete.end(), [&](const auto& p) {
            return p.get() == tab;
        });
        if (it2 != mTabsToDelete.end())
        {
            mTabsToDelete.erase(it2);
        }

        if (tabIndexToBeSelected == -1)
        {
            mSelectedTab = nullptr;
            mWidgetContainer->clear();
        }
        else
        {
            setSelectedTab(tabIndexToBeSelected);
        }

        adjustSize();
        adjustTabPositions();
    }

    int TabbedArea::getNumberOfTabs() const
    {
        return mTabs.size();
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

    void TabbedArea::setSelectedTab(unsigned int index)
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
        unsigned int i;
        for (i = 0; i < mTabs.size(); i++)
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

    void TabbedArea::setOpaque(bool opaque)
    {
        mOpaque = opaque;
    }

    bool TabbedArea::isOpaque() const
    {
        return mOpaque;
    }

    bool TabbedArea::isTabActive() const
    {
        return tabActive;
    }

    void TabbedArea::draw(Graphics *graphics)
    {
        const Color& faceColor = getBaseColor();
        int alpha = getBaseColor().a;
        Color highlightColor = faceColor + 0x303030;
        highlightColor.a = alpha;
        Color shadowColor = faceColor - 0x303030;
        shadowColor.a = alpha;

        // Draw a border.
        graphics->setColor(highlightColor);
        graphics->drawLine(0, mTabContainer->getHeight(), 0, getHeight() - 2);
        graphics->setColor(shadowColor);
        graphics->drawLine(
            getWidth() - 1, mTabContainer->getHeight() + 1, getWidth() - 1, getHeight() - 1);
        graphics->drawLine(1, getHeight() - 1, getWidth() - 1, getHeight() - 1);

        if (mOpaque)
        {
            graphics->setColor(getBaseColor());
            graphics->fillRectangle(Rectangle(1, 1, getWidth() - 2, getHeight() - 2));
        }

        // Draw a line underneath the tabs.
        graphics->setColor(highlightColor);
        graphics->drawLine(1,
                           mTabContainer->getHeight(),
                           getWidth() - 1,
                           mTabContainer->getHeight());

        // If a tab is selected,
        // remove the line right underneath the selected tab.
        if (mSelectedTab != nullptr)
        {
            graphics->setColor(getBaseColor());
            graphics->drawLine(mSelectedTab->getX() + 1,
                               mTabContainer->getHeight(),
                               mSelectedTab->getX() + mSelectedTab->getWidth() - 2,
                               mTabContainer->getHeight());
        }

        // Draw the widget from a select tab.
        for (const auto& p : mTabs)
        {
            p.first->_draw(graphics);
            if (p.first == mSelectedTab)
            {
                p.second->_draw(graphics);
            }
        }
    }

    void TabbedArea::adjustSize()
    {
        int maxTabHeight = 0;
        for (unsigned int i = 0; i < mTabs.size(); i++)
        {
            if (mTabs[i].first->getHeight() > maxTabHeight)
            {
                maxTabHeight = mTabs[i].first->getHeight();
            }
        }

        mTabContainer->setSize(getWidth() - 2, maxTabHeight);

        mWidgetContainer->setPosition(1, maxTabHeight + 1);
        mWidgetContainer->setSize(getWidth() - 2, getHeight() - maxTabHeight - 2);
    }

    void TabbedArea::adjustTabPositions()
    {
        int maxTabHeight = 0;
        unsigned int i;
        for (i = 0; i < mTabs.size(); i++)
        {
            if (mTabs[i].first->getHeight() > maxTabHeight)
            {
                maxTabHeight = mTabs[i].first->getHeight();
            }
        }

        int x = 0;
        for (i = 0; i < mTabs.size(); i++)
        {
            Tab* tab = mTabs[i].first;

            tab->setPosition(x, maxTabHeight - tab->getHeight());
            x += tab->getWidth();

            Widget* widget = mTabs[i].second;
            widget->setX(mWidgetContainer->getX());
            widget->setY(mWidgetContainer->getY());
        }
    }

    void TabbedArea::setWidth(int width)
    {
        // This may seem odd, but we want the TabbedArea to adjust
        // its size properly before we call Widget::setWidth as
        // Widget::setWidth might distribute a resize event.
        gcn::Rectangle dim = mDimension;
        mDimension.width = width;
        adjustSize();
        mDimension = dim;
        Widget::setWidth(width);
    }

    void TabbedArea::setHeight(int height)
    {
        // This may seem odd, but we want the TabbedArea to adjust
        // its size properly before we call Widget::setHeight as
        // Widget::setHeight might distribute a resize event.
        gcn::Rectangle dim = mDimension;
        mDimension.height = height;
        adjustSize();
        mDimension = dim;
        Widget::setHeight(height);
    }

    void TabbedArea::setSize(int width, int height)
    {
        // This may seem odd, but we want the TabbedArea to adjust
        // its size properly before we call Widget::setSize as
        // Widget::setSize might distribute a resize event.
        gcn::Rectangle dim = mDimension;
        mDimension.width = width;
        mDimension.height = height;
        adjustSize();
        mDimension = dim;
        Widget::setSize(width, height);
    }

    void TabbedArea::setDimension(const Rectangle& dimension)
    {
        // This may seem odd, but we want the TabbedArea to adjust
        // it's size properly before we call Widget::setDimension as
        // Widget::setDimension might distribute a resize event.
        gcn::Rectangle dim = mDimension;
        mDimension = dimension;
        adjustSize();
        mDimension = dim;
        Widget::setDimension(dimension);
    }

    void TabbedArea::keyPressed(KeyEvent& keyEvent)
    {
        if (keyEvent.isConsumed() || !isFocused())
        {
            return;
        }

        if (keyEvent.getKey().getValue() == Key::Left)
        {
            int index = getSelectedTabIndex();
            index--;

            if (index < 0)
            {
                return;
            }
            else
            {
                setSelectedTab(mTabs[index].first);
            }

            keyEvent.consume();
        }
        else if (keyEvent.getKey().getValue() == Key::Right)
        {
            int index = getSelectedTabIndex();
            index++;
            
            if (index >= (int)mTabs.size())
            {
                return;
            }
            else
            {
                setSelectedTab(mTabs[index].first);
            }

            keyEvent.consume();
        }
    }

    void TabbedArea::mousePressed(MouseEvent& mouseEvent)
    {
        if (mouseEvent.isConsumed())
        {
            return;
        }

        if (mouseEvent.getButton() == MouseEvent::Left)
        {
            Widget* widget = mTabContainer->getWidgetAt(mouseEvent.getX(), mouseEvent.getY());
            Tab* tab = dynamic_cast<Tab*>(widget);

            if (tab != nullptr)
            {
                setSelectedTab(tab);
                tabActive = true;
                mouseEvent.consume();
            }
            else
            {
                widget = mWidgetContainer->getWidgetAt(mouseEvent.getX(), mouseEvent.getY());
                if (widget == nullptr)
                {
                    mouseEvent.consume();
                }
                tabActive = false;
            }
        }

        // Request focus only if the source of the event is not focusable.
        // If the source of the event is focused
        // we don't want to steal the focus.
        if (!mouseEvent.getSource()->isFocusable())
        {
            requestFocus();
        }
    }

    void TabbedArea::death(const Event& event)
    {
        if (Tab* tab = dynamic_cast<Tab*>(event.getSource()))
        {
            removeTab(tab);
        }
        else
        {
            //BasicContainer::death(event);
        }
    }

    void TabbedArea::action(const ActionEvent& actionEvent)
    {
        Widget* source = actionEvent.getSource();
        Tab* tab = dynamic_cast<Tab*>(source);

        if (tab == nullptr)
        {
            throw GCN_EXCEPTION("Received an action from a widget that's not a tab!");
        }

        setSelectedTab(tab);
    }

    void TabbedArea::setBaseColor(const Color& color)
    {
        Widget::setBaseColor(color);
        mWidgetContainer->setBaseColor(color);
        mTabContainer->setBaseColor(color);
    }
} // namespace gcn
