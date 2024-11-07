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

#include "guisan/gui.hpp"

#include "guisan/exception.hpp"
#include "guisan/focushandler.hpp"
#include "guisan/graphics.hpp"
#include "guisan/input.hpp"
#include "guisan/keyinput.hpp"
#include "guisan/keylistener.hpp"
#include "guisan/mouseinput.hpp"
#include "guisan/mouselistener.hpp"
#include "guisan/widget.hpp"

#include <algorithm>

namespace gcn
{
    Gui::Gui() : mFocusHandler(new FocusHandler())
    {}

    Gui::~Gui()
    {
        if (Widget::widgetExists(mTop))
        {
            setTop(nullptr);
        }

        delete mFocusHandler;
    }

    void Gui::setTop(Widget* top)
    {
        if (mTop != nullptr)
        {
            mTop->_setFocusHandler(nullptr);
        }
        if (top != nullptr)
        {
            top->_setFocusHandler(mFocusHandler);
        }

        mTop = top;
    }

    Widget* Gui::getTop() const
    {
        return mTop;
    }

    void Gui::setGraphics(Graphics* graphics)
    {
        mGraphics = graphics;
    }

    Graphics* Gui::getGraphics() const
    {
        return mGraphics;
    }

    void Gui::setInput(Input* input)
    {
        mInput = input;
    }

    Input* Gui::getInput() const
    {
        return mInput;
    }

    void Gui::logic()
    {
        if (mTop == nullptr)
        {
            throw GCN_EXCEPTION("No top widget set");
        }

        handleModalFocus();
        handleModalMouseInputFocus();

        if (mInput != nullptr)
        {
            mInput->_pollInput();

            handleKeyInput();
            handleMouseInput();
        }

        mTop->_logic();
    }

    void Gui::draw()
    {
        if (mTop == nullptr)
        {
            throw GCN_EXCEPTION("No top widget set");
        }
        if (mGraphics == nullptr)
        {
            throw GCN_EXCEPTION("No graphics set");
        }

        if (!mTop->isVisible())
        {
            return;
        }

        mGraphics->_beginDraw();
        mTop->_draw(mGraphics);
        mGraphics->_endDraw();
    }

    void Gui::focusNone()
    {
        mFocusHandler->focusNone();
    }

    void Gui::setTabbingEnabled(bool tabbing)
    {
        mTabbing = tabbing;
    }

    bool Gui::isTabbingEnabled()
    {
        return mTabbing;
    }

    void Gui::addGlobalKeyListener(KeyListener* keyListener)
    {
        mKeyListeners.push_back(keyListener);
    }

    void Gui::removeGlobalKeyListener(KeyListener* keyListener)
    {
        mKeyListeners.remove(keyListener);
    }

    void Gui::handleMouseInput()
    {
        while (!mInput->isMouseQueueEmpty())
        {
            MouseInput mouseInput = mInput->dequeueMouseInput();

            switch (mouseInput.getType())
            {
                case MouseInput::Pressed: handleMousePressed(mouseInput); break;
                case MouseInput::Released: handleMouseReleased(mouseInput); break;
                case MouseInput::Moved: handleMouseMoved(mouseInput); break;
                case MouseInput::WheelMovedDown: handleMouseWheelMovedDown(mouseInput); break;
                case MouseInput::WheelMovedUp: handleMouseWheelMovedUp(mouseInput); break;
                default: throw GCN_EXCEPTION("Unknown mouse input type."); break;
            }

            // Save the current mouse state. It's needed to send
            // mouse exited events and mouse entered events when
            // the mouse exits a widget and when a widget releases
            // modal mouse input focus.
            mLastMouseX = mouseInput.getX();
            mLastMouseY = mouseInput.getY();
        }
    }

    void Gui::handleKeyInput()
    {
        while (!mInput->isKeyQueueEmpty())
        {
            KeyInput keyInput = mInput->dequeueKeyInput();

            // Save modifiers state
            mShiftPressed = keyInput.isShiftPressed();
            mMetaPressed = keyInput.isMetaPressed();
            mControlPressed = keyInput.isControlPressed();
            mAltPressed = keyInput.isAltPressed();

            KeyEvent keyEventToGlobalKeyListeners(nullptr,
                                                  nullptr,
                                                  mShiftPressed,
                                                  mControlPressed,
                                                  mAltPressed,
                                                  mMetaPressed,
                                                  keyInput.getType(),
                                                  keyInput.isNumericPad(),
                                                  keyInput.getKey());

            distributeKeyEventToGlobalKeyListeners(keyEventToGlobalKeyListeners);

            // If a global key listener consumes the event it will not be
            // sent further to the source of the event.
            if (keyEventToGlobalKeyListeners.isConsumed())
            {
                continue;
            }

            bool keyEventConsumed = false;

            // Send key inputs to the focused widgets
            if (mFocusHandler->getFocused() != nullptr)
            {
                Widget* source = getKeyEventSource();
                KeyEvent keyEvent(source,
                                  source,
                                  mShiftPressed,
                                  mControlPressed,
                                  mAltPressed,
                                  mMetaPressed,
                                  keyInput.getType(),
                                  keyInput.isNumericPad(),
                                  keyInput.getKey());


                if (!mFocusHandler->getFocused()->isFocusable())
                {
                    mFocusHandler->focusNone();
                }
                else
                {
                    distributeKeyEvent(keyEvent);
                }

                keyEventConsumed = keyEvent.isConsumed();
            }

            if (!keyEventConsumed)
            {
                mFocusHandler->checkHotKey(keyInput);
            }

            // If the key event hasn't been consumed and
            // tabbing is enable check for tab press and
            // change focus.
            if (!keyEventConsumed
                && mTabbing
                && keyInput.getKey().getValue() == Key::Tab
                && keyInput.getType() == KeyInput::Pressed)
            {
                if (keyInput.isShiftPressed())
                {
                    mFocusHandler->tabPrevious();
                }
                else
                {
                    mFocusHandler->tabNext();
                }
            }
        }
    }

    void Gui::handleMouseMoved(const MouseInput& mouseInput)
    {
        // Get the last widgets with the mouse
        // using the last known mouse position.
        std::set<Widget*> mLastWidgetsWithMouse = getWidgetsAt(mLastMouseX, mLastMouseY);

        // Check if the mouse has left the application window.
        if (mouseInput.getX() < 0 || mouseInput.getY() < 0
            || !mTop->getDimension().isContaining(mouseInput.getX(), mouseInput.getY()))
        {
            for (Widget* w : mLastWidgetsWithMouse)
            {
                distributeMouseEvent(w,
                                     MouseEvent::Exited,
                                     mouseInput.getButton(),
                                     mouseInput.getX(),
                                     mouseInput.getY(),
                                     true,
                                     true);
            }
        }
        // The mouse is in the application window.
        else
        {
            // Calculate which widgets should receive a mouse exited event
            // and which should receive a mouse entered event by using the
            // last known mouse position and the latest mouse position.
            std::set<Widget*> mWidgetsWithMouse =
                getWidgetsAt(mouseInput.getX(), mouseInput.getY());
            std::set<Widget*> mWidgetsWithMouseExited;
            std::set<Widget*> mWidgetsWithMouseEntered;
            std::set_difference(
                mLastWidgetsWithMouse.begin(),
                mLastWidgetsWithMouse.end(),
                mWidgetsWithMouse.begin(),
                mWidgetsWithMouse.end(),
                std::inserter(mWidgetsWithMouseExited, mWidgetsWithMouseExited.begin()));
            std::set_difference(
                mWidgetsWithMouse.begin(),
                mWidgetsWithMouse.end(),
                mLastWidgetsWithMouse.begin(),
                mLastWidgetsWithMouse.end(),
                std::inserter(mWidgetsWithMouseEntered, mWidgetsWithMouseEntered.begin()));

            for (Widget* w : mWidgetsWithMouseExited)
            {
                distributeMouseEvent(w,
                                     MouseEvent::Exited,
                                     mouseInput.getButton(),
                                     mouseInput.getX(),
                                     mouseInput.getY(),
                                     true,
                                     true);
                // As the mouse has exited a widget we need
                // to reset the click count and the last mouse
                // press time stamp.
                mClickCount = 1;
                mLastMousePressTimeStamp = 0;
            }

            for (Widget* widget : mWidgetsWithMouseEntered)
            {
                // If a widget has modal mouse input focus we
                // only want to send entered events to that widget
                // and the widget's parents.
                if ((mFocusHandler->getModalMouseInputFocused() != nullptr
                     && widget->isModalMouseInputFocused())
                    || mFocusHandler->getModalMouseInputFocused() == nullptr)
                {
                    distributeMouseEvent(widget,
                                         MouseEvent::Entered,
                                         mouseInput.getButton(),
                                         mouseInput.getX(),
                                         mouseInput.getY(),
                                         true,
                                         true);
                }
            }
        }
        if (mFocusHandler->getDraggedWidget() != nullptr)
        {
            distributeMouseEvent(mFocusHandler->getDraggedWidget(),
                                 MouseEvent::Dragged,
                                 mLastMouseDragButton,
                                 mouseInput.getX(),
                                 mouseInput.getY());
        }
        else
        {
            Widget* sourceWidget = getMouseEventSource(mouseInput.getX(), mouseInput.getY());
            distributeMouseEvent(sourceWidget,
                                 MouseEvent::Moved,
                                 mouseInput.getButton(),
                                 mouseInput.getX(),
                                 mouseInput.getY());
        }
    }

    void Gui::handleMousePressed(const MouseInput& mouseInput)
    {
        Widget* sourceWidget = getMouseEventSource(mouseInput.getX(), mouseInput.getY());

        if (mFocusHandler->getDraggedWidget() != nullptr)
        {
            sourceWidget = mFocusHandler->getDraggedWidget();
        }

        int sourceWidgetX, sourceWidgetY;
        sourceWidget->getAbsolutePosition(sourceWidgetX, sourceWidgetY);

        if ((mFocusHandler->getModalFocused() != nullptr && sourceWidget->isModalFocused())
            || mFocusHandler->getModalFocused() == nullptr)
        {
            sourceWidget->requestFocus();
        }

        if (mouseInput.getTimeStamp() - mLastMousePressTimeStamp < 250
            && mLastMousePressButton == mouseInput.getButton())
        {
            mClickCount++;
        }
        else
        {
            mClickCount = 1;
        }

        distributeMouseEvent(sourceWidget,
                             MouseEvent::Pressed,
                             mouseInput.getButton(),
                             mouseInput.getX(),
                             mouseInput.getY());

        mFocusHandler->setLastWidgetPressed(sourceWidget);

        mFocusHandler->setDraggedWidget(sourceWidget);
        mLastMouseDragButton = mouseInput.getButton();

        mLastMousePressButton = mouseInput.getButton();
        mLastMousePressTimeStamp = mouseInput.getTimeStamp();
    }

    void Gui::handleMouseWheelMovedDown(const MouseInput& mouseInput)
    {
        Widget* sourceWidget = getMouseEventSource(mouseInput.getX(), mouseInput.getY());

        if (mFocusHandler->getDraggedWidget() != nullptr)
        {
            sourceWidget = mFocusHandler->getDraggedWidget();
        }

        int sourceWidgetX, sourceWidgetY;
        sourceWidget->getAbsolutePosition(sourceWidgetX, sourceWidgetY);

        distributeMouseEvent(sourceWidget,
                             MouseEvent::WheelMovedDown,
                             mouseInput.getButton(),
                             mouseInput.getX(),
                             mouseInput.getY());
    }

    void Gui::handleMouseWheelMovedUp(const MouseInput& mouseInput)
    {
        Widget* sourceWidget = getMouseEventSource(mouseInput.getX(), mouseInput.getY());

        if (mFocusHandler->getDraggedWidget() != nullptr)
        {
            sourceWidget = mFocusHandler->getDraggedWidget();
        }

        int sourceWidgetX, sourceWidgetY;
        sourceWidget->getAbsolutePosition(sourceWidgetX, sourceWidgetY);

        distributeMouseEvent(sourceWidget,
                             MouseEvent::WheelMovedUp,
                             mouseInput.getButton(),
                             mouseInput.getX(),
                             mouseInput.getY());
    }

    void Gui::handleMouseReleased(const MouseInput& mouseInput)
    {
        Widget* sourceWidget = getMouseEventSource(mouseInput.getX(), mouseInput.getY());

        if (mFocusHandler->getDraggedWidget() != nullptr)
        {
            if (sourceWidget != mFocusHandler->getLastWidgetPressed())
            {
                mFocusHandler->setLastWidgetPressed(nullptr);
            }

            sourceWidget = mFocusHandler->getDraggedWidget();
        }

        int sourceWidgetX, sourceWidgetY;
        sourceWidget->getAbsolutePosition(sourceWidgetX, sourceWidgetY);

        distributeMouseEvent(sourceWidget,
                             MouseEvent::Released,
                             mouseInput.getButton(),
                             mouseInput.getX(),
                             mouseInput.getY());

        if (mouseInput.getButton() == mLastMousePressButton
            && mFocusHandler->getLastWidgetPressed() == sourceWidget)
        {
            distributeMouseEvent(sourceWidget,
                                 MouseEvent::Clicked,
                                 mouseInput.getButton(),
                                 mouseInput.getX(),
                                 mouseInput.getY());

            mFocusHandler->setLastWidgetPressed(nullptr);
        }
        else
        {
            mLastMousePressButton = 0;
            mClickCount = 0;
        }

        if (mFocusHandler->getDraggedWidget() != nullptr)
        {
            mFocusHandler->setDraggedWidget(nullptr);
        }
    }

    Widget* Gui::getWidgetAt(int x, int y)
    {
        // If the widget's parent has no child then we have found the widget..
        Widget* parent = mTop;
        Widget* child = mTop;

        while (child != nullptr)
        {
            Widget* swap = child;
            int parentX, parentY;
            parent->getAbsolutePosition(parentX, parentY);
            child = parent->getWidgetAt(x - parentX, y - parentY);
            parent = swap;
        }

        return parent;
    }

    std::set<Widget*> Gui::getWidgetsAt(int x, int y)
    {
        std::set<Widget*> result;

        Widget* widget = mTop;

        while (widget != nullptr)
        {
            result.insert(widget);
            int absoluteX, absoluteY;
            widget->getAbsolutePosition(absoluteX, absoluteY);
            widget = widget->getWidgetAt(x - absoluteX, y - absoluteY);
        }

        return result;
    }

    Widget* Gui::getMouseEventSource(int x, int y)
    {
        Widget* widget = getWidgetAt(x, y);

        if (mFocusHandler->getModalMouseInputFocused() != nullptr
            && !widget->isModalMouseInputFocused())
        {
            return mFocusHandler->getModalMouseInputFocused();
        }

        return widget;
    }

    Widget* Gui::getKeyEventSource()
    {
        Widget* widget = mFocusHandler->getFocused();

        while (widget->_getInternalFocusHandler() != nullptr
               && widget->_getInternalFocusHandler()->getFocused() != nullptr)
        {
            widget = widget->_getInternalFocusHandler()->getFocused();
        }

        return widget;
    }

    void Gui::distributeMouseEvent(Widget* source,
                                   int type,
                                   int button,
                                   int x,
                                   int y,
                                   bool force,
                                   bool toSourceOnly)
    {
        Widget* parent = source;
        Widget* widget = source;

        if (mFocusHandler->getModalFocused() != nullptr
            && !widget->isModalFocused()
            && !force)
        {
            return;
        }

        if (mFocusHandler->getModalMouseInputFocused() != nullptr
            && !widget->isModalMouseInputFocused()
            && !force)
        {
            return;
        }

        MouseEvent mouseEvent(source,
                              source,
                              mShiftPressed,
                              mControlPressed,
                              mAltPressed,
                              mMetaPressed,
                              type,
                              button,
                              x,
                              y,
                              mClickCount);

        while (parent != nullptr)
        {
            // If the widget has been removed due to input
            // cancel the distribution.
            if (!Widget::widgetExists(widget))
            {
                break;
            }

            parent = widget->getParent();

            if (widget->isEnabled() || force)
            {
                int widgetX, widgetY;
                widget->getAbsolutePosition(widgetX, widgetY);

                mouseEvent.mX = x - widgetX;
                mouseEvent.mY = y - widgetY;
                mouseEvent.mDistributor = widget;

                // Send the event to all mouse listeners of the widget.
                for (MouseListener* mouseListener : widget->_getMouseListeners())
                {
                    switch (mouseEvent.getType())
                    {
                        case MouseEvent::Entered: mouseListener->mouseEntered(mouseEvent); break;
                        case MouseEvent::Exited: mouseListener->mouseExited(mouseEvent); break;
                        case MouseEvent::Moved: mouseListener->mouseMoved(mouseEvent); break;
                        case MouseEvent::Pressed: mouseListener->mousePressed(mouseEvent); break;
                        case MouseEvent::Released: mouseListener->mouseReleased(mouseEvent); break;
                        case MouseEvent::WheelMovedUp:
                            mouseListener->mouseWheelMovedUp(mouseEvent);
                            break;
                        case MouseEvent::WheelMovedDown:
                            mouseListener->mouseWheelMovedDown(mouseEvent);
                            break;
                        case MouseEvent::Dragged: mouseListener->mouseDragged(mouseEvent); break;
                        case MouseEvent::Clicked: mouseListener->mouseClicked(mouseEvent); break;
                        default: throw GCN_EXCEPTION("Unknown mouse event type.");
                    }
                }

                if (toSourceOnly)
                {
                    break;
                }
            }

            Widget* swap = widget;
            widget = parent;
            parent = swap->getParent();

            // If a non modal focused widget has been reach
            // and we have modal focus cancel the distribution.
            if (mFocusHandler->getModalFocused() != nullptr
                && widget != nullptr
                && !widget->isModalFocused())
            {
                break;
            }

            // If a non modal mouse input focused widget has been reach
            // and we have modal mouse input focus cancel the distribution.
            if (mFocusHandler->getModalMouseInputFocused() != nullptr
                && widget != nullptr
                && !widget->isModalMouseInputFocused())
            {
                break;
            }
        }
    }

    void Gui::distributeKeyEvent(KeyEvent& keyEvent)
    {
        Widget* parent = keyEvent.getSource();
        Widget* widget = keyEvent.getSource();

        if (mFocusHandler->getModalFocused() != nullptr
            && !widget->isModalFocused())
        {
            return;
        }

        if (mFocusHandler->getModalMouseInputFocused() != nullptr
            && !widget->isModalMouseInputFocused())
        {
            return;
        }

        while (parent != nullptr)
        {
            // If the widget has been removed due to input
            // cancel the distribution.
            if (!Widget::widgetExists(widget))
            {
                break;
            }

            parent = widget->getParent();

            if (widget->isEnabled())
            {
                keyEvent.mDistributor = widget;

                // Send the event to all key listeners of the source widget.
                for (KeyListener* keyListener : widget->_getKeyListeners())
                {
                    switch (keyEvent.getType())
                    {
                        case KeyEvent::Pressed: keyListener->keyPressed(keyEvent); break;
                        case KeyEvent::Released: keyListener->keyReleased(keyEvent); break;
                        default: throw GCN_EXCEPTION("Unknown key event type.");
                    }
                }
            }

            Widget* swap = widget;
            widget = parent;
            parent = swap->getParent();

            // If a non modal focused widget has been reach
            // and we have modal focus cancel the distribution.
            if (mFocusHandler->getModalFocused() != nullptr
                && !widget->isModalFocused())
            {
                break;
            }
        }
    }

    void Gui::distributeKeyEventToGlobalKeyListeners(KeyEvent& keyEvent)
    {
        for (KeyListener* keyListener : mKeyListeners)
        {
            switch (keyEvent.getType())
            {
                case KeyEvent::Pressed: keyListener->keyPressed(keyEvent); break;
                case KeyEvent::Released: keyListener->keyReleased(keyEvent); break;
                default: throw GCN_EXCEPTION("Unknown key event type.");
            }

            if (keyEvent.isConsumed())
            {
                break;
            }
        }
    }

    void Gui::handleModalMouseInputFocus()
    {
        // Check if modal mouse input focus has been gained by a widget.
        if ((mFocusHandler->getLastWidgetWithModalMouseInputFocus() 
                != mFocusHandler->getModalMouseInputFocused())
             && (mFocusHandler->getLastWidgetWithModalMouseInputFocus() == nullptr))
        {
            handleModalFocusGained();
            mFocusHandler->setLastWidgetWithModalMouseInputFocus(mFocusHandler->getModalMouseInputFocused());
        }
        // Check if modal mouse input focus has been released.
        else if ((mFocusHandler->getLastWidgetWithModalMouseInputFocus()
                    != mFocusHandler->getModalMouseInputFocused())
                    && (mFocusHandler->getLastWidgetWithModalMouseInputFocus() != nullptr))
        {
            handleModalFocusReleased();
            mFocusHandler->setLastWidgetWithModalMouseInputFocus(nullptr);
        }
    }

     void Gui::handleModalFocus()
    {
        // Check if modal focus has been gained by a widget.
        if ((mFocusHandler->getLastWidgetWithModalFocus() 
                != mFocusHandler->getModalFocused())
             && (mFocusHandler->getLastWidgetWithModalFocus() == nullptr))
        {
            handleModalFocusGained();
            mFocusHandler->setLastWidgetWithModalFocus(mFocusHandler->getModalFocused());
        }
        // Check if modal focus has been released.
        else if ((mFocusHandler->getLastWidgetWithModalFocus()
                    != mFocusHandler->getModalFocused())
                    && (mFocusHandler->getLastWidgetWithModalFocus() != nullptr))
        {
            handleModalFocusReleased();
            mFocusHandler->setLastWidgetWithModalFocus(nullptr);
        }
    }

    void Gui::handleModalFocusGained()
    {
        // Get all widgets at the last known mouse position
        // and send them a mouse exited event.
        std::set<Widget*> mWidgetsWithMouse = getWidgetsAt(mLastMouseX, mLastMouseY);

        for (Widget* w : mWidgetsWithMouse)
        {
            distributeMouseEvent(
                w, MouseEvent::Exited, mLastMousePressButton, mLastMouseX, mLastMouseY, true, true);
        }

        mFocusHandler->setLastWidgetWithModalMouseInputFocus(mFocusHandler->getModalMouseInputFocused());
    }

    void Gui::handleModalFocusReleased()
    {
        // Get all widgets at the last known mouse position
        // and send them a mouse entered event.
        std::set<Widget*> mWidgetsWithMouse = getWidgetsAt(mLastMouseX, mLastMouseY);

        for (Widget* w : mWidgetsWithMouse)
        {
            distributeMouseEvent(w,
                                 MouseEvent::Entered,
                                 mLastMousePressButton,
                                 mLastMouseX,
                                 mLastMouseY,
                                 false,
                                 true);
        }
    }
}
