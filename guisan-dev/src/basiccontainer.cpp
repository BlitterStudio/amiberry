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

#include "guisan/basiccontainer.hpp"

#include <algorithm>

#include "guisan/exception.hpp"
#include "guisan/focushandler.hpp"
#include "guisan/graphics.hpp"
#include "guisan/mouseinput.hpp"

namespace gcn
{
    BasicContainer::~BasicContainer()
    {
        clear();
    }

    void BasicContainer::moveToTop(Widget* widget)
    {
        WidgetListIterator iter;
        for (iter = mWidgets.begin(); iter != mWidgets.end(); iter++)
        {
            if (*iter == widget)
            {
                mWidgets.erase(iter);
                mWidgets.push_back(widget);
                return;
            }
        }

        throw GCN_EXCEPTION("There is no such widget in this container.");
    }

    void BasicContainer::moveToBottom(Widget* widget)
    {
        WidgetListIterator iter;
        iter = find(mWidgets.begin(), mWidgets.end(), widget);

        if (iter == mWidgets.end())
        {
            throw GCN_EXCEPTION("There is no such widget in this container.");
        }
        mWidgets.erase(iter);
        mWidgets.push_front(widget);
    }

    void BasicContainer::death(const Event& event)
    {
        WidgetListIterator iter;
        iter = find(mWidgets.begin(), mWidgets.end(), event.getSource());

        if (iter == mWidgets.end())
        {
            throw GCN_EXCEPTION("There is no such widget in this container.");
        }

        mWidgets.erase(iter);
    }

    Rectangle BasicContainer::getChildrenArea()
    {
        return Rectangle(0, 0, getWidth(), getHeight());
    }

    void BasicContainer::focusNext()
    {
        WidgetListIterator it;

        for (it = mWidgets.begin(); it != mWidgets.end(); it++)
        {
            if ((*it)->isFocused())
            {
                break;
            }
        }

        WidgetListIterator end = it;

        if (it == mWidgets.end())
        {
            it = mWidgets.begin();
        }

        it++;

        for ( ; it != end; it++)
        {
            if (it == mWidgets.end())
            {
                it = mWidgets.begin();
            }

            if ((*it)->isFocusable())
            {
                (*it)->requestFocus();
                return;
            }
        }
    }

    void BasicContainer::focusPrevious()
    {
        WidgetListReverseIterator it;

        for (it = mWidgets.rbegin(); it != mWidgets.rend(); it++)
        {
            if ((*it)->isFocused())
            {
                break;
            }
        }

        WidgetListReverseIterator end = it;

        it++;

        if (it == mWidgets.rend())
        {
            it = mWidgets.rbegin();
        }

        for ( ; it != end; it++)
        {
            if (it == mWidgets.rend())
            {
                it = mWidgets.rbegin();
            }

            if ((*it)->isFocusable())
            {
                (*it)->requestFocus();
                return;
            }
        }
    }

    Widget *BasicContainer::getWidgetAt(int x, int y)
    {
        Rectangle r = getChildrenArea();

        if (!r.isPointInRect(x, y))
        {
            return NULL;
        }

        x -= r.x;
        y -= r.y;

        WidgetListReverseIterator it;
        for (it = mWidgets.rbegin(); it != mWidgets.rend(); it++)
        {
            if ((*it)->isVisible() && (*it)->getDimension().isPointInRect(x, y))
            {
                return (*it);
            }
        }

        return NULL;
    }

    void BasicContainer::logic()
    {
        logicChildren();
    }

    void BasicContainer::_setFocusHandler(FocusHandler* focusHandler)
    {
        Widget::_setFocusHandler(focusHandler);

        if (mInternalFocusHandler != NULL)
        {
            return;
        }

        WidgetListIterator iter;
        for (iter = mWidgets.begin(); iter != mWidgets.end(); iter++)
        {
            (*iter)->_setFocusHandler(focusHandler);
        }
    }

    void BasicContainer::add(Widget* widget)
    {
        mWidgets.push_back(widget);

        if (mInternalFocusHandler == NULL)
        {
            widget->_setFocusHandler(_getFocusHandler());
        }
        else
        {
            widget->_setFocusHandler(mInternalFocusHandler);
        }

        widget->_setParent(this);
        widget->addDeathListener(this);
    }

    void BasicContainer::remove(Widget* widget)
    {
        WidgetListIterator iter;
        for (iter = mWidgets.begin(); iter != mWidgets.end(); iter++)
        {
            if (*iter == widget)
            {
                mWidgets.erase(iter);
                widget->_setFocusHandler(NULL);
                widget->_setParent(NULL);
                widget->removeDeathListener(this);
                return;
            }
        }

        throw GCN_EXCEPTION("There is no such widget in this container.");
    }

    void BasicContainer::clear()
    {
        WidgetListIterator iter;

        for (iter = mWidgets.begin(); iter != mWidgets.end(); iter++)
        {
            (*iter)->_setFocusHandler(NULL);
            (*iter)->_setParent(NULL);
            (*iter)->removeDeathListener(this);
        }

        mWidgets.clear();
    }

    void BasicContainer::drawChildren(Graphics* graphics)
    {
        graphics->pushClipArea(getChildrenArea());

        WidgetListIterator iter;
        for (iter = mWidgets.begin(); iter != mWidgets.end(); iter++)
        {
            if ((*iter)->isVisible())
            {
                // If the widget has a border,
                // draw it before drawing the widget
                if ((*iter)->getBorderSize() > 0)
                {
                    Rectangle rec = (*iter)->getDimension();
                    rec.x -= (*iter)->getBorderSize();
                    rec.y -= (*iter)->getBorderSize();
                    rec.width += 2 * (*iter)->getBorderSize();
                    rec.height += 2 * (*iter)->getBorderSize();
                    graphics->pushClipArea(rec);
                    (*iter)->drawBorder(graphics);
                    graphics->popClipArea();
                }

                graphics->pushClipArea((*iter)->getDimension());
                (*iter)->draw(graphics);
                graphics->popClipArea();
            }
        }

        graphics->popClipArea();
    }

    void BasicContainer::logicChildren()
    {
        WidgetListIterator iter;
        for (iter = mWidgets.begin(); iter != mWidgets.end(); iter++)
        {
            (*iter)->logic();
        }
    }

    void BasicContainer::showWidgetPart(Widget* widget, Rectangle area)
    {
        Rectangle widgetArea = getChildrenArea();
        area.x += widget->getX();
        area.y += widget->getY();

        if (area.x + area.width > widgetArea.width)
        {
            widget->setX(widget->getX() - area.x - area.width + widgetArea.width);
        }

        if (area.y + area.height > widgetArea.height)
        {
            widget->setY(widget->getY() - area.y - area.height + widgetArea.height);
        }

        if (area.x < 0)
        {
            widget->setX(widget->getX() - area.x);
        }

        if (area.y < 0)
        {
            widget->setY(widget->getY() - area.y);
        }
    }


    void BasicContainer::setInternalFocusHandler(FocusHandler* focusHandler)
    {
        Widget::setInternalFocusHandler(focusHandler);

        WidgetListIterator iter;
        for (iter = mWidgets.begin(); iter != mWidgets.end(); iter++)
        {
            if (mInternalFocusHandler == NULL)
            {
                (*iter)->_setFocusHandler(_getFocusHandler());
            }
            else
            {
                (*iter)->_setFocusHandler(mInternalFocusHandler);
            }
        }
    }

    Widget* BasicContainer::findWidgetById(const std::string& id)
    {
        WidgetListIterator iter;
        for (iter = mWidgets.begin(); iter != mWidgets.end(); iter++)
        {
            if ((*iter)->getId() == id)
            {
                return (*iter);
            }
            
            BasicContainer *basicContainer = dynamic_cast<BasicContainer*>(*iter);
            
            if (basicContainer != NULL)
            {
                Widget *widget = basicContainer->findWidgetById(id);
                
                if (widget != NULL)
                {
                    return widget;
                }
            }
        }

        return NULL;
    }
}
