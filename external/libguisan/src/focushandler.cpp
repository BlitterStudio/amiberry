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

#include "guisan/focushandler.hpp"

#include "guisan/focuslistener.hpp"
#include "guisan/exception.hpp"
#include "guisan/keyinput.hpp"
#include "guisan/widget.hpp"

#include <algorithm>

namespace gcn
{

    void FocusHandler::requestFocus(Widget* widget)
    {
        if (widget == nullptr
            || widget == mFocusedWidget)
        {
            return;
        }

        int toBeFocusedIndex = -1;
        for (unsigned int i = 0; i < mWidgets.size(); ++i)
        {
            if (mWidgets[i] == widget)
            {
                toBeFocusedIndex = i;
                break;
            }
        }

        if (toBeFocusedIndex < 0)
        {
            throw GCN_EXCEPTION("Trying to focus a non-existing widget.");
        }

        Widget *oldFocused = mFocusedWidget;

        if (oldFocused != widget)
        {
            mFocusedWidget = mWidgets.at(toBeFocusedIndex);

            if (oldFocused != nullptr)
            {
                Event focusEvent(oldFocused);
                distributeFocusLostEvent(focusEvent);
            }
            Event focusEvent(mWidgets.at(toBeFocusedIndex));
            distributeFocusGainedEvent(focusEvent);
        }
    }

    void FocusHandler::requestModalFocus(Widget* widget)
    {
        if (mModalFocusedWidget != nullptr && mModalFocusedWidget != widget)
        {
            throw GCN_EXCEPTION("Another widget already has modal focus.");
        }

        mModalFocusedWidget = widget;

        if (mFocusedWidget != nullptr && !mFocusedWidget->isModalFocused())
        {
            focusNone();
        }
    }

    void FocusHandler::requestModalMouseInputFocus(Widget* widget)
    {
        if (mModalMouseInputFocusedWidget != nullptr
            && mModalMouseInputFocusedWidget != widget)
        {
            throw GCN_EXCEPTION("Another widget already has modal input focus.");
        }

        mModalMouseInputFocusedWidget = widget;
    }

    void FocusHandler::releaseModalFocus(Widget* widget)
    {
        if (mModalFocusedWidget == widget)
        {
            mModalFocusedWidget = nullptr;
        }
    }

    void FocusHandler::releaseModalMouseInputFocus(Widget* widget)
    {
        if (mModalMouseInputFocusedWidget == widget)
        {
            mModalMouseInputFocusedWidget = nullptr;
        }
    }

    Widget* FocusHandler::getFocused() const
    {
        return mFocusedWidget;
    }

    Widget* FocusHandler::getModalFocused() const
    {
        return mModalFocusedWidget;
    }

    Widget* FocusHandler::getModalMouseInputFocused() const
    {
        return mModalMouseInputFocusedWidget;
    }

    void FocusHandler::focusNext()
    {
        int i;
        int focusedWidget = -1;
        for (i = 0; i < (int)mWidgets.size(); ++i)
        {
            if (mWidgets[i] == mFocusedWidget)
            {
                focusedWidget = i;
            }
        }
        int focused = focusedWidget;

        // i is a counter that ensures that the following loop
        // won't get stuck in an infinite loop
        i = (int)mWidgets.size();
        do
        {
            ++focusedWidget;

            if (i == 0)
            {
                focusedWidget = -1;
                break;
            }

            --i;

            if (focusedWidget >= (int)mWidgets.size())
            {
                focusedWidget = 0;
            }

            if (focusedWidget == focused)
            {
                return;
            }
        }
        while (!mWidgets.at(focusedWidget)->isFocusable());

        if (focusedWidget >= 0)
        {
            mFocusedWidget = mWidgets.at(focusedWidget);

            const Event focusEvent(mFocusedWidget);
            distributeFocusGainedEvent(focusEvent);
        }

        if (focused >= 0)
        {
            const Event focusEvent(mWidgets.at(focused));
            distributeFocusLostEvent(focusEvent);
        }
    }

    void FocusHandler::focusPrevious()
    {
        if (mWidgets.size() == 0)
        {
            mFocusedWidget = nullptr;
            return;
        }

        int i;
        int focusedWidget = -1;
        for (i = 0; i < (int)mWidgets.size(); ++i)
        {
            if (mWidgets[i] == mFocusedWidget)
            {
                focusedWidget = i;
            }
        }
        const int focused = focusedWidget;

        // i is a counter that ensures that the following loop
        // won't get stuck in an infinite loop
        i = (int)mWidgets.size();
        do
        {
            --focusedWidget;

            if (i == 0)
            {
                focusedWidget = -1;
                break;
            }

            --i;

            if (focusedWidget <= 0)
            {
                focusedWidget = mWidgets.size() - 1;
            }

            if (focusedWidget == focused)
            {
                return;
            }
        }
        while (!mWidgets.at(focusedWidget)->isFocusable());

        if (focusedWidget >= 0)
        {
            mFocusedWidget = mWidgets.at(focusedWidget);
            const Event focusEvent(mFocusedWidget);
            distributeFocusGainedEvent(focusEvent);
        }

        if (focused >= 0)
        {
            const Event focusEvent(mWidgets.at(focused));
            distributeFocusLostEvent(focusEvent);
        }
    }

    bool FocusHandler::isFocused(const Widget* widget) const
    {
        return mFocusedWidget == widget;
    }

    void FocusHandler::add(Widget* widget)
    {
        mWidgets.push_back(widget);
    }

    void FocusHandler::remove(Widget* widget)
    {
        if (isFocused(widget))
        {
            mFocusedWidget = nullptr;
        }

        const auto iter = std::find(mWidgets.begin(), mWidgets.end(), widget);
        if (iter != mWidgets.end())
        {
            mWidgets.erase(iter);
        }

        if (mDraggedWidget == widget)
        {
            mDraggedWidget = nullptr;
            return;
        }

        if (mLastWidgetWithMouse == widget)
        {
            mLastWidgetWithMouse = nullptr;
            return;
        }

        if (mLastWidgetWithModalFocus == widget)
        {
            mLastWidgetWithModalFocus = nullptr;
            return;
        }

        if (mLastWidgetWithModalMouseInputFocus == widget)
        {
            mLastWidgetWithModalMouseInputFocus = nullptr;
            return;
        }

        if (mLastWidgetPressed == widget)
        {
            mLastWidgetPressed = nullptr;
            return;
        }
    }

    void FocusHandler::focusNone()
    {
        if (mFocusedWidget != nullptr)
        {
            Widget* focused = mFocusedWidget;
            mFocusedWidget = nullptr;

            Event focusEvent(focused);
            distributeFocusLostEvent(focusEvent);
        }
    }

    void FocusHandler::tabNext()
    {
        if (mFocusedWidget != nullptr)
        {
            if (!mFocusedWidget->isTabOutEnabled())
            {
                return;
            }
        }

        if (mWidgets.empty())
        {
            mFocusedWidget = nullptr;
            return;
        }

        int i;
        int focusedWidget = -1;
        for (i = 0; i < (int)mWidgets.size(); ++i)
        {
            if (mWidgets[i] == mFocusedWidget)
            {
                focusedWidget = i;
            }
        }
        const int focused = focusedWidget;
        bool done = false;

        // i is a counter that ensures that the following loop
        // won't get stuck in an infinite loop
        i = (int)mWidgets.size();
        do
        {
            ++focusedWidget;

            if (i == 0)
            {
                focusedWidget = -1;
                break;
            }

            --i;

            if (focusedWidget >= (int)mWidgets.size())
            {
                focusedWidget = 0;
            }

            if (focusedWidget == focused)
            {
                return;
            }

            if (mWidgets.at(focusedWidget)->isFocusable() &&
                mWidgets.at(focusedWidget)->isTabInEnabled() &&
                (mModalFocusedWidget == nullptr ||
                    mWidgets.at(focusedWidget)->isModalFocused()))
            {
                done = true;
            }
        }
        while (!done);

        if (focusedWidget >= 0)
        {
            mFocusedWidget = mWidgets.at(focusedWidget);
            const Event focusEvent(mFocusedWidget);
            distributeFocusGainedEvent(focusEvent);
        }

        if (focused >= 0)
        {
            const Event focusEvent(mWidgets.at(focused));
            distributeFocusLostEvent(focusEvent);
        }
    }

    void FocusHandler::tabPrevious()
    {
        if (mFocusedWidget != nullptr)
        {
            if (!mFocusedWidget->isTabOutEnabled())
            {
                return;
            }
        }

        if (mWidgets.empty())
        {
            mFocusedWidget = nullptr;
            return;
        }

        int i;
        int focusedWidget = -1;
        for (i = 0; i < (int)mWidgets.size(); ++i)
        {
            if (mWidgets[i] == mFocusedWidget)
            {
                focusedWidget = i;
            }
        }
        const int focused = focusedWidget;
        bool done = false;

        // i is a counter that ensures that the following loop
        // won't get stuck in an infinite loop
        i = (int)mWidgets.size();
        do
        {
            --focusedWidget;

            if (i == 0)
            {
                focusedWidget = -1;
                break;
            }

            --i;

            if (focusedWidget <= 0)
            {
                focusedWidget = mWidgets.size() - 1;
            }

            if (focusedWidget == focused)
            {
                return;
            }

            if (mWidgets.at(focusedWidget)->isFocusable() &&
                mWidgets.at(focusedWidget)->isTabInEnabled() &&
                (mModalFocusedWidget == nullptr ||
                    mWidgets.at(focusedWidget)->isModalFocused()))
            {
                done = true;
            }
        }
        while (!done);

        if (focusedWidget >= 0)
        {
            mFocusedWidget = mWidgets.at(focusedWidget);
            const Event focusEvent(mFocusedWidget);
            distributeFocusGainedEvent(focusEvent);
        }

        if (focused >= 0)
        {
            const Event focusEvent(mWidgets.at(focused));
            distributeFocusLostEvent(focusEvent);
        }
    }

    void FocusHandler::distributeFocusLostEvent(const Event& focusEvent)
    {
        Widget* sourceWidget = focusEvent.getSource();

        // Send the event to all focus listeners of the widget.
        for (auto* focusListener : sourceWidget->_getFocusListeners())
        {
            focusListener->focusLost(focusEvent);
        }
    }

    void FocusHandler::distributeFocusGainedEvent(const Event& focusEvent)
    {
        Widget* sourceWidget = focusEvent.getSource();

        // Send the event to all focus listeners of the widget.
        for (auto* focusListener : sourceWidget->_getFocusListeners())
        {
            focusListener->focusGained(focusEvent);
        }
    }

    Widget* FocusHandler::getDraggedWidget()
    {
        return mDraggedWidget;
    }

    void FocusHandler::setDraggedWidget(Widget* draggedWidget)
    {
        mDraggedWidget = draggedWidget;
    }

    Widget* FocusHandler::getLastWidgetWithMouse()
    {
        return mLastWidgetWithMouse;
    }

    void FocusHandler::setLastWidgetWithMouse(Widget* lastWidgetWithMouse)
    {
        mLastWidgetWithMouse = lastWidgetWithMouse;
    }

    Widget* FocusHandler::getLastWidgetWithModalFocus()
    {
        return mLastWidgetWithModalFocus;
    }

    void FocusHandler::setLastWidgetWithModalFocus(Widget* lastWidgetWithModalFocus)
    {
        mLastWidgetWithModalFocus = lastWidgetWithModalFocus;
    }

    Widget* FocusHandler::getLastWidgetWithModalMouseInputFocus()
    {
        return mLastWidgetWithModalMouseInputFocus;
    }

    void FocusHandler::setLastWidgetWithModalMouseInputFocus(Widget* lastWidgetWithModalMouseInputFocus)
    {
        mLastWidgetWithModalMouseInputFocus = lastWidgetWithModalMouseInputFocus;
    }

    Widget* FocusHandler::getLastWidgetPressed()
    {
        return mLastWidgetPressed;
    }

    void FocusHandler::setLastWidgetPressed(Widget* lastWidgetPressed)
    {
        mLastWidgetPressed = lastWidgetPressed;
    }

    void FocusHandler::checkHotKey(const KeyInput& keyInput)
    {
        const int keyin = keyInput.getKey().getValue();

        for (auto widget : mWidgets)
        {
            int hotKey = widget->getHotKey();

            if (hotKey == 0)
            {
                continue;
            }
            hotKey = isascii(hotKey) ? tolower(hotKey) : hotKey;

            if ((isascii(keyin) && tolower(keyin) == hotKey) || keyin == hotKey)
            {
                if (keyInput.getType() == KeyInput::Pressed)
                {
                    widget->hotKeyPressed();
                    if (widget->isFocusable())
                    {
                        this->requestFocus(widget);
                    }
                }
                else
                {
                    widget->hotKeyReleased();
                }
                break;
            }
        }
    }
}
