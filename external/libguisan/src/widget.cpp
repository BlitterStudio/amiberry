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

#include "guisan/widget.hpp"

#include "guisan/actionevent.hpp"
#include "guisan/actionlistener.hpp"
#include "guisan/basiccontainer.hpp"
#include "guisan/deathlistener.hpp"
#include "guisan/defaultfont.hpp"
#include "guisan/event.hpp"
#include "guisan/exception.hpp"
#include "guisan/focushandler.hpp"
#include "guisan/keyinput.hpp"
#include "guisan/keylistener.hpp"
#include "guisan/mouseinput.hpp"
#include "guisan/mouselistener.hpp"
#include "guisan/widgetlistener.hpp"

namespace gcn
{
    Font* Widget::mGlobalFont = NULL;
    DefaultFont Widget::mDefaultFont;
    std::list<Widget*> Widget::mWidgets;

    Widget::Widget()
            : mForegroundColor(0x000000),
              mBackgroundColor(0xffffff),
              mBaseColor(0xDDDDE3),
              mSelectionColor(0xc3d9ff),
              mFocusHandler(NULL),
              mInternalFocusHandler(NULL),
              mParent(NULL),
              mBorderSize(0),
              mFocusable(false),
              mVisible(true),
              mTabIn(true),
              mTabOut(true),
              mEnabled(true),
              mCurrentFont(NULL)
    {
        mWidgets.push_back(this);
    }

    Widget::~Widget()
    {
        DeathListenerIterator iter;

        for (iter = mDeathListeners.begin(); iter != mDeathListeners.end(); ++iter)
        {
            Event event(this);
            (*iter)->death(event);
        }

        _setFocusHandler(NULL);

        mWidgets.remove(this);
    }

    void Widget::_setParent(Widget* parent)
    {
        mParent = parent;
    }

    Widget* Widget::getParent() const
    {
        return mParent;
    }

    void Widget::setWidth(int width)
    {
        Rectangle newDimension = mDimension;
        newDimension.width = width;

        setDimension(newDimension);
    }

    int Widget::getWidth() const
    {
        return mDimension.width;
    }

    void Widget::setHeight(int height)
    {
        Rectangle newDimension = mDimension;
        newDimension.height = height;

        setDimension(newDimension);
    }

    int Widget::getHeight() const
    {
        return mDimension.height;
    }

    void Widget::setX(int x)
    {
        Rectangle newDimension = mDimension;
        newDimension.x = x;

        setDimension(newDimension);
    }

    int Widget::getX() const
    {
        return mDimension.x;
    }

    void Widget::setY(int y)
    {
        Rectangle newDimension = mDimension;
        newDimension.y = y;

        setDimension(newDimension);
    }

    int Widget::getY() const
    {
        return mDimension.y;
    }

    void Widget::setPosition(int x, int y)
    {
        Rectangle newDimension = mDimension;
        newDimension.x = x;
        newDimension.y = y;
        
        setDimension(newDimension);
    }

    void Widget::setDimension(const Rectangle& dimension)
    { 
        Rectangle oldDimension = mDimension;
        mDimension = dimension;

        if (mDimension.width != oldDimension.width
            || mDimension.height != oldDimension.height)
        {
            distributeResizedEvent();
        }

        if (mDimension.x != oldDimension.x
            || mDimension.y != oldDimension.y)
        {
            distributeMovedEvent();
        }
    }

    void Widget::setBorderSize(unsigned int borderSize)
    {
        mBorderSize = borderSize;
    }

    unsigned int Widget::getBorderSize() const
    {
        return mBorderSize;
    }

    const Rectangle& Widget::getDimension() const
    {
        return mDimension;
    }

    const std::string& Widget::getActionEventId() const
    {
        return mActionEventId;
    }

    void Widget::setActionEventId(const std::string& actionEventId)
    {
        mActionEventId = actionEventId;
    }

    bool Widget::isFocused() const
    {
        if (!mFocusHandler)
        {
            return false;
        }

        return (mFocusHandler->isFocused(this));
    }

    void Widget::setFocusable(bool focusable)
    {
        if (!focusable && isFocused())
        {
            mFocusHandler->focusNone();
        }

        mFocusable = focusable;
    }

    bool Widget::isFocusable() const
    {
        return mFocusable && isVisible() && isEnabled();
    }

    void Widget::requestFocus()
    {
        if (mFocusHandler == NULL)
        {
            throw GCN_EXCEPTION("No focushandler set (did you add the widget to the gui?).");
        }

        if (isFocusable())
        {
            mFocusHandler->requestFocus(this);
        }
    }

    void Widget::requestMoveToTop()
    {
        if (mParent)
        {
            mParent->moveToTop(this);
        }
    }

    void Widget::requestMoveToBottom()
    {
        if (mParent)
        {
            mParent->moveToBottom(this);
        }
    }

    void Widget::setVisible(bool visible)
    {
        if (!visible && isFocused())
        {
            mFocusHandler->focusNone();
        }
        
        if (visible)
        {
            distributeShownEvent();
        }
        else if(!visible)
        {
            distributeHiddenEvent();
        }

        mVisible = visible;
    }

    bool Widget::isVisible() const
    {
        if (getParent() == NULL)
        {
            return mVisible;
        }
        else
        {
            return mVisible && getParent()->isVisible();
        }
    }

    void Widget::setBaseColor(const Color& color)
    {
        mBaseColor = color;
    }

    const Color& Widget::getBaseColor() const
    {
        return mBaseColor;
    }

    void Widget::setForegroundColor(const Color& color)
    {
        mForegroundColor = color;
    }

    const Color& Widget::getForegroundColor() const
    {
        return mForegroundColor;
    }

    void Widget::setBackgroundColor(const Color& color)
    {
        mBackgroundColor = color;
    }

    const Color& Widget::getBackgroundColor() const
    {
        return mBackgroundColor;
    }

    void Widget::setSelectionColor(const Color& color)
    {
        mSelectionColor = color;
    }

    const Color& Widget::getSelectionColor() const
    {
        return mSelectionColor;
    }    
    
    void Widget::_setFocusHandler(FocusHandler* focusHandler)
    {
        if (mFocusHandler)
        {
            releaseModalFocus();
            mFocusHandler->remove(this);
        }

        if (focusHandler)
        {
            focusHandler->add(this);
        }

        mFocusHandler = focusHandler;
    }

    FocusHandler* Widget::_getFocusHandler()
    {
        return mFocusHandler;
    }

    void Widget::addActionListener(ActionListener* actionListener)
    {
        mActionListeners.push_back(actionListener);
    }

    void Widget::removeActionListener(ActionListener* actionListener)
    {
        mActionListeners.remove(actionListener);
    }

    void Widget::addDeathListener(DeathListener* deathListener)
    {
        mDeathListeners.push_back(deathListener);
    }

    void Widget::removeDeathListener(DeathListener* deathListener)
    {
        mDeathListeners.remove(deathListener);
    }

    void Widget::addKeyListener(KeyListener* keyListener)
    {
        mKeyListeners.push_back(keyListener);
    }

    void Widget::removeKeyListener(KeyListener* keyListener)
    {
        mKeyListeners.remove(keyListener);
    }

    void Widget::addFocusListener(FocusListener* focusListener)
    {
        mFocusListeners.push_back(focusListener);
    }

    void Widget::removeFocusListener(FocusListener* focusListener)
    {
        mFocusListeners.remove(focusListener);
    }

    void Widget::addMouseListener(MouseListener* mouseListener)
    {
        mMouseListeners.push_back(mouseListener);
    }

    void Widget::removeMouseListener(MouseListener* mouseListener)
    {
        mMouseListeners.remove(mouseListener);
    }

    void Widget::addWidgetListener(WidgetListener* widgetListener)
    {
        mWidgetListeners.push_back(widgetListener);
    }

    void Widget::removeWidgetListener(WidgetListener* widgetListener)
    {
        mWidgetListeners.remove(widgetListener);
    }

    void Widget::getAbsolutePosition(int& x, int& y) const
    {
        if (getParent() == NULL)
        {
            x = mDimension.x;
            y = mDimension.y;
            return;
        }

        int parentX;
        int parentY;

        getParent()->getAbsolutePosition(parentX, parentY);

        x = parentX + mDimension.x + getParent()->getChildrenArea().x;
        y = parentY + mDimension.y + getParent()->getChildrenArea().y;
    }

    void Widget::generateAction()
    {
        ActionListenerIterator iter;
        for (iter = mActionListeners.begin(); iter != mActionListeners.end(); ++iter)
        {
            ActionEvent actionEvent(this, mActionEventId);
            (*iter)->action(actionEvent);
        }
    }

    Font* Widget::getFont() const
    {
        if (mCurrentFont == NULL)
        {
            if (mGlobalFont == NULL)
            {
                return &mDefaultFont;
            }

            return mGlobalFont;
        }

        return mCurrentFont;
    }

    void Widget::setGlobalFont(Font* font)
    {
        mGlobalFont = font;

        std::list<Widget*>::iterator iter;
        for (iter = mWidgets.begin(); iter != mWidgets.end(); ++iter)
        {
            if ((*iter)->mCurrentFont == NULL)
            {
                (*iter)->fontChanged();
            }
        }
    }

    void Widget::setFont(Font* font)
    {
        mCurrentFont = font;
        fontChanged();
    }

    bool Widget::widgetExists(const Widget* widget)
    {
        bool result = false;

        std::list<Widget*>::iterator iter;
        for (iter = mWidgets.begin(); iter != mWidgets.end(); ++iter)
        {
            if (*iter == widget)
            {
                return true;
            }
        }

        return result;
    }

    bool Widget::isTabInEnabled() const
    {
        return mTabIn;
    }

    void Widget::setTabInEnabled(bool enabled)
    {
        mTabIn = enabled;
    }

    bool Widget::isTabOutEnabled() const
    {
        return mTabOut;
    }

    void Widget::setTabOutEnabled(bool enabled)
    {
        mTabOut = enabled;
    }

    void Widget::setSize(int width, int height)
    {
        Rectangle newDimension = mDimension;
        newDimension.width = width;
        newDimension.height = height;

        setDimension(newDimension);
    }

    void Widget::setEnabled(bool enabled)
    {
        mEnabled = enabled;
    }

    bool Widget::isEnabled() const
    {
        return mEnabled && isVisible();
    }

    void Widget::requestModalFocus()
    {
        if (mFocusHandler == NULL)
        {
            throw GCN_EXCEPTION("No focushandler set (did you add the widget to the gui?).");
        }

        mFocusHandler->requestModalFocus(this);
    }

    void Widget::requestModalMouseInputFocus()
    {
        if (mFocusHandler == NULL)
        {
            throw GCN_EXCEPTION("No focushandler set (did you add the widget to the gui?).");
        }

        mFocusHandler->requestModalMouseInputFocus(this);
    }

    void Widget::releaseModalFocus()
    {
        if (mFocusHandler == NULL)
        {
            return;
        }

        mFocusHandler->releaseModalFocus(this);
    }

    void Widget::releaseModalMouseInputFocus()
    {
        if (mFocusHandler == NULL)
        {
            return;
        }

        mFocusHandler->releaseModalMouseInputFocus(this);
    }

    bool Widget::hasModalFocus() const
    {
        if (mFocusHandler == NULL)
        {
            throw GCN_EXCEPTION("No focushandler set (did you add the widget to the gui?).");
        }

        if (getParent() != NULL)
        {
            return (mFocusHandler->getModalFocused() == this) || getParent()->hasModalFocus();
        }

        return mFocusHandler->getModalFocused() == this;
    }

    bool Widget::hasModalMouseInputFocus() const
    {
        if (mFocusHandler == NULL)
        {
            throw GCN_EXCEPTION("No focushandler set (did you add the widget to the gui?).");
        }

        if (getParent() != NULL)
        {
            return (mFocusHandler->getModalMouseInputFocused() == this) || getParent()->hasModalMouseInputFocus();
        }

        return mFocusHandler->getModalMouseInputFocused() == this;
    }

    Widget *Widget::getWidgetAt(int x, int y)
    {
        return NULL;
    }

    const std::list<MouseListener*>& Widget::_getMouseListeners()
    {
        return mMouseListeners;
    }

    const std::list<KeyListener*>& Widget::_getKeyListeners()
    {
        return mKeyListeners;
    }

    const std::list<FocusListener*>& Widget::_getFocusListeners()
    {
        return mFocusListeners;
    }

    Rectangle Widget::getChildrenArea()
    {
        return Rectangle(0, 0, 0, 0);
    }

    FocusHandler* Widget::_getInternalFocusHandler()
    {
        return mInternalFocusHandler;
    }

    void Widget::setInternalFocusHandler(FocusHandler* focusHandler)
    {
        mInternalFocusHandler = focusHandler;
    }

    void Widget::setId(const std::string& id)
    {
        mId = id;
    }

    const std::string& Widget::getId()
    {
        return mId;
    }

    void Widget::distributeResizedEvent()
    {
        WidgetListenerIterator iter;

        for (iter = mWidgetListeners.begin(); iter != mWidgetListeners.end(); ++iter)
        {
            Event event(this);
            (*iter)->widgetResized(event);
        }
    }

    void Widget::distributeMovedEvent()
    {
        WidgetListenerIterator iter;

        for (iter = mWidgetListeners.begin(); iter != mWidgetListeners.end(); ++iter)
        {
            Event event(this);
            (*iter)->widgetMoved(event);
        }
    }

    void Widget::distributeHiddenEvent()
    {
        WidgetListenerIterator iter;

        for (iter = mWidgetListeners.begin(); iter != mWidgetListeners.end(); ++iter)
        {
            Event event(this);
            (*iter)->widgetHidden(event);
        }
    }

    void Widget::distributeShownEvent()
    {
        WidgetListenerIterator iter;

        for (iter = mWidgetListeners.begin(); iter != mWidgetListeners.end(); ++iter)
        {
            Event event(this);
            (*iter)->widgetShown(event);
        }
    }
}
