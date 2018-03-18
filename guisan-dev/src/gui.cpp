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

#include "guisan/basiccontainer.hpp"
#include "guisan/exception.hpp"
#include "guisan/focushandler.hpp"
#include "guisan/graphics.hpp"
#include "guisan/input.hpp"
#include "guisan/keyinput.hpp"
#include "guisan/keylistener.hpp"
#include "guisan/mouseinput.hpp"
#include "guisan/mouselistener.hpp"
#include "guisan/widget.hpp"

namespace gcn
{
	Gui::Gui()
		: mTop(nullptr),
		  mGraphics(nullptr),
		  mInput(nullptr),
		  mTabbing(true),
		  mShiftPressed(false),
		  mMetaPressed(false),
		  mControlPressed(false),
		  mAltPressed(false),
		  mLastMousePressButton(0),
		  mLastMousePressTimeStamp(0),
		  mLastMouseX(0),
		  mLastMouseY(0),
		  mClickCount(1),
		  mLastMouseDragButton(0)
	{
		mFocusHandler = new FocusHandler();
	}

	Gui::~Gui()
	{
		if (Widget::widgetExists(mTop))
		{
			Gui::setTop(nullptr);
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
		} // end if

		mTop->logic();
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

		// If top has a border,
		// draw it before drawing top
		if (mTop->getBorderSize() > 0)
		{
			auto rec = mTop->getDimension();
			rec.x -= mTop->getBorderSize();
			rec.y -= mTop->getBorderSize();
			rec.width += 2 * mTop->getBorderSize();
			rec.height += 2 * mTop->getBorderSize();
			mGraphics->pushClipArea(rec);
			mTop->drawBorder(mGraphics);
			mGraphics->popClipArea();
		}

		mGraphics->pushClipArea(mTop->getDimension());
		mTop->draw(mGraphics);
		mGraphics->popClipArea();

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
			auto mouseInput = mInput->dequeueMouseInput();

			// Save the current mouse state. It will be needed if modal focus
			// changes or modal mouse input focus changes.
			mLastMouseX = mouseInput.getX();
			mLastMouseY = mouseInput.getY();

			switch (mouseInput.getType())
			{
			case MouseInput::PRESSED:
				handleMousePressed(mouseInput);
				break;
			case MouseInput::RELEASED:
				handleMouseReleased(mouseInput);
				break;
			case MouseInput::MOVED:
				handleMouseMoved(mouseInput);
				break;
			case MouseInput::WHEEL_MOVED_DOWN:
				handleMouseWheelMovedDown(mouseInput);
				break;
			case MouseInput::WHEEL_MOVED_UP:
				handleMouseWheelMovedUp(mouseInput);
				break;
			default:
				throw GCN_EXCEPTION("Unknown mouse input type.");
			}
		}
	}

	void Gui::handleKeyInput()
	{
		while (!mInput->isKeyQueueEmpty())
		{
			auto keyInput = mInput->dequeueKeyInput();

			// Save modifiers state
			mShiftPressed = keyInput.isShiftPressed();
			mMetaPressed = keyInput.isMetaPressed();
			mControlPressed = keyInput.isControlPressed();
			mAltPressed = keyInput.isAltPressed();

			KeyEvent keyEventToGlobalKeyListeners(nullptr,
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
				KeyEvent keyEvent(getKeyEventSource(),
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

			// If the key event hasn't been consumed and
			// tabbing is enable check for tab press and
			// change focus.
			if (!keyEventConsumed
				&& mTabbing
				&& keyInput.getKey().getValue() == Key::TAB
				&& keyInput.getType() == KeyInput::PRESSED)
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
		} // end while
	}

	void Gui::handleMouseMoved(const MouseInput& mouseInput)
	{
		// Check if the mouse leaves the application window.
		if (!mWidgetWithMouseQueue.empty()
			&& (mouseInput.getX() < 0
				|| mouseInput.getY() < 0
				|| !mTop->getDimension().isPointInRect(mouseInput.getX(), mouseInput.getY()))
		)
		{
			// Distribute an event to all widgets in the "widget with mouse" queue.
			while (!mWidgetWithMouseQueue.empty())
			{
				auto widget = mWidgetWithMouseQueue.front();

				if (Widget::widgetExists(widget))
				{
					distributeMouseEvent(widget,
					                     MouseEvent::EXITED,
					                     mouseInput.getButton(),
					                     mouseInput.getX(),
					                     mouseInput.getX(),
					                     true,
					                     true);
				}

				mWidgetWithMouseQueue.pop_front();
			}

			return;
		}

		// Check if there is a need to send mouse exited events by
		// traversing the "widget with mouse" queue.
		bool widgetWithMouseQueueCheckDone = mWidgetWithMouseQueue.empty();
		while (!widgetWithMouseQueueCheckDone)
		{
			unsigned int iterations = 0;
			for (auto iter = mWidgetWithMouseQueue.begin();
			     iter != mWidgetWithMouseQueue.end();
			     ++iter)
			{
				auto widget = *iter;

				// If a widget in the "widget with mouse queue" doesn't
				// exists anymore it should be removed from the queue.
				if (!Widget::widgetExists(widget))
				{
					mWidgetWithMouseQueue.erase(iter);
					break;
				}
				int x, y;
				widget->getAbsolutePosition(x, y);

				if (x > mouseInput.getX()
					|| y > mouseInput.getY()
					|| x + widget->getWidth() <= mouseInput.getX()
					|| y + widget->getHeight() <= mouseInput.getY()
					|| !widget->isVisible())
				{
					distributeMouseEvent(widget,
					                     MouseEvent::EXITED,
					                     mouseInput.getButton(),
					                     mouseInput.getX(),
					                     mouseInput.getX(),
					                     true,
					                     true);
					mClickCount = 1;
					mLastMousePressTimeStamp = 0;
					mWidgetWithMouseQueue.erase(iter);
					break;
				}

				iterations++;
			}

			widgetWithMouseQueueCheckDone = iterations == mWidgetWithMouseQueue.size();
		}

		// Check all widgets below the mouse to see if they are
		// present in the "widget with mouse" queue. If a widget
		// is not then it should be added and an entered event should
		// be sent to it.
		auto parent = getMouseEventSource(mouseInput.getX(), mouseInput.getY());
		auto widget = parent;

		// If a widget has modal mouse input focus then it will
		// always be returned from getMouseEventSource, but we only wan't to send
		// mouse entered events if the mouse has actually entered the widget with
		// modal mouse input focus, hence we need to check if that's the case. If
		// it's not we should simply ignore to send any mouse entered events.
		if (mFocusHandler->getModalMouseInputFocused() != nullptr
			&& widget == mFocusHandler->getModalMouseInputFocused()
			&& Widget::widgetExists(widget))
		{
			int x, y;
			widget->getAbsolutePosition(x, y);

			if (x > mouseInput.getX()
				|| y > mouseInput.getY()
				|| x + widget->getWidth() <= mouseInput.getX()
				|| y + widget->getHeight() <= mouseInput.getY())
			{
				parent = nullptr;
			}
		}

		while (parent != nullptr)
		{
			parent = static_cast<Widget*>(widget->getParent());

			// Check if the widget is present in the "widget with mouse" queue.
			auto widgetIsPresentInQueue = false;
			for (auto& iter : mWidgetWithMouseQueue)
			{
				if (iter == widget)
				{
					widgetIsPresentInQueue = true;
					break;
				}
			}

			// Widget is not present, send an entered event and add
			// it to the "widget with mouse" queue.
			if (!widgetIsPresentInQueue
				&& Widget::widgetExists(widget))
			{
				distributeMouseEvent(widget,
				                     MouseEvent::ENTERED,
				                     mouseInput.getButton(),
				                     mouseInput.getX(),
				                     mouseInput.getY(),
				                     true,
				                     true);
				mWidgetWithMouseQueue.push_front(widget);
			}

			auto swap = widget;
			widget = parent;
			parent = static_cast<Widget*>(swap->getParent());
		}

		if (mFocusHandler->getDraggedWidget() != nullptr)
		{
			distributeMouseEvent(mFocusHandler->getDraggedWidget(),
			                     MouseEvent::DRAGGED,
			                     mLastMouseDragButton,
			                     mouseInput.getX(),
			                     mouseInput.getY());
		}
		else
		{
			auto sourceWidget = getMouseEventSource(mouseInput.getX(), mouseInput.getY());
			distributeMouseEvent(sourceWidget,
			                     MouseEvent::MOVED,
			                     mouseInput.getButton(),
			                     mouseInput.getX(),
			                     mouseInput.getY());
		}
	}

	void Gui::handleMousePressed(const MouseInput& mouseInput)
	{
		auto sourceWidget = getMouseEventSource(mouseInput.getX(), mouseInput.getY());

		if (mFocusHandler->getDraggedWidget() != nullptr)
		{
			sourceWidget = mFocusHandler->getDraggedWidget();
		}

		int sourceWidgetX, sourceWidgetY;
		sourceWidget->getAbsolutePosition(sourceWidgetX, sourceWidgetY);

		distributeMouseEvent(sourceWidget,
		                     MouseEvent::PRESSED,
		                     mouseInput.getButton(),
		                     mouseInput.getX(),
		                     mouseInput.getY());

		mFocusHandler->setLastWidgetPressed(sourceWidget);

		if ((mFocusHandler->getModalFocused() != nullptr
			&& sourceWidget->hasModalFocus())
			|| mFocusHandler->getModalFocused() == nullptr)
		{
			sourceWidget->requestFocus();
		}

		mFocusHandler->setDraggedWidget(sourceWidget);
		mLastMouseDragButton = mouseInput.getButton();

		if (mLastMousePressTimeStamp < 300
			&& mLastMousePressButton == mouseInput.getButton())
		{
			mClickCount++;
		}
		else
		{
			mClickCount = 1;
		}

		mLastMousePressButton = mouseInput.getButton();
		mLastMousePressTimeStamp = mouseInput.getTimeStamp();
	}

	void Gui::handleMouseWheelMovedDown(const MouseInput& mouseInput)
	{
		auto sourceWidget = getMouseEventSource(mouseInput.getX(), mouseInput.getY());

		if (mFocusHandler->getDraggedWidget() != nullptr)
		{
			sourceWidget = mFocusHandler->getDraggedWidget();
		}

		int sourceWidgetX, sourceWidgetY;
		sourceWidget->getAbsolutePosition(sourceWidgetX, sourceWidgetY);

		distributeMouseEvent(sourceWidget,
		                     MouseEvent::WHEEL_MOVED_DOWN,
		                     mouseInput.getButton(),
		                     mouseInput.getX(),
		                     mouseInput.getY());
	}

	void Gui::handleMouseWheelMovedUp(const MouseInput& mouseInput)
	{
		auto sourceWidget = getMouseEventSource(mouseInput.getX(), mouseInput.getY());

		if (mFocusHandler->getDraggedWidget() != nullptr)
		{
			sourceWidget = mFocusHandler->getDraggedWidget();
		}

		int sourceWidgetX, sourceWidgetY;
		sourceWidget->getAbsolutePosition(sourceWidgetX, sourceWidgetY);

		distributeMouseEvent(sourceWidget,
		                     MouseEvent::WHEEL_MOVED_UP,
		                     mouseInput.getButton(),
		                     mouseInput.getX(),
		                     mouseInput.getY());
	}

	void Gui::handleMouseReleased(const MouseInput& mouseInput)
	{
		auto sourceWidget = getMouseEventSource(mouseInput.getX(), mouseInput.getY());

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
		                     MouseEvent::RELEASED,
		                     mouseInput.getButton(),
		                     mouseInput.getX(),
		                     mouseInput.getY());

		if (mouseInput.getButton() == mLastMousePressButton
			&& mFocusHandler->getLastWidgetPressed() == sourceWidget)
		{
			distributeMouseEvent(sourceWidget,
			                     MouseEvent::CLICKED,
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
		auto parent = mTop;
		auto child = mTop;

		while (child != nullptr)
		{
			auto swap = child;
			int parentX, parentY;
			parent->getAbsolutePosition(parentX, parentY);
			child = parent->getWidgetAt(x - parentX, y - parentY);
			parent = swap;
		}

		return parent;
	}

	Widget* Gui::getMouseEventSource(int x, int y)
	{
		auto widget = getWidgetAt(x, y);

		if (mFocusHandler->getModalMouseInputFocused() != nullptr
			&& !widget->hasModalMouseInputFocus())
		{
			return mFocusHandler->getModalMouseInputFocused();
		}

		return widget;
	}

	Widget* Gui::getKeyEventSource()
	{
		auto widget = mFocusHandler->getFocused();

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
		auto parent = source;
		auto widget = source;

		if (mFocusHandler->getModalFocused() != nullptr
			&& !widget->hasModalFocus()
			&& !force)
		{
			return;
		}

		if (mFocusHandler->getModalMouseInputFocused() != nullptr
			&& !widget->hasModalMouseInputFocus()
			&& !force)
		{
			return;
		}

		MouseEvent mouseEvent(source,
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

			parent = static_cast<Widget*>(widget->getParent());

			if (widget->isEnabled() || force)
			{
				int widgetX, widgetY;
				widget->getAbsolutePosition(widgetX, widgetY);

				mouseEvent.mX = x - widgetX;
				mouseEvent.mY = y - widgetY;

				auto mouseListeners = widget->_getMouseListeners();

				// Send the event to all mouse listeners of the widget.
				for (auto& mouseListener : mouseListeners)
				{
					switch (mouseEvent.getType())
					{
					case MouseEvent::ENTERED:
						mouseListener->mouseEntered(mouseEvent);
						break;
					case MouseEvent::EXITED:
						mouseListener->mouseExited(mouseEvent);
						break;
					case MouseEvent::MOVED:
						mouseListener->mouseMoved(mouseEvent);
						break;
					case MouseEvent::PRESSED:
						mouseListener->mousePressed(mouseEvent);
						break;
					case MouseEvent::RELEASED:
						mouseListener->mouseReleased(mouseEvent);
						break;
					case MouseEvent::WHEEL_MOVED_UP:
						mouseListener->mouseWheelMovedUp(mouseEvent);
						break;
					case MouseEvent::WHEEL_MOVED_DOWN:
						mouseListener->mouseWheelMovedDown(mouseEvent);
						break;
					case MouseEvent::DRAGGED:
						mouseListener->mouseDragged(mouseEvent);
						break;
					case MouseEvent::CLICKED:
						mouseListener->mouseClicked(mouseEvent);
						break;
					default:
						throw GCN_EXCEPTION("Unknown mouse event type.");
					}
				}

				if (toSourceOnly)
				{
					break;
				}
			}

			auto swap = widget;
			widget = parent;
			parent = static_cast<Widget*>(swap->getParent());

			// If a non modal focused widget has been reach
			// and we have modal focus cancel the distribution.
			if (mFocusHandler->getModalFocused() != nullptr
				&& !widget->hasModalFocus())
			{
				break;
			}

			// If a non modal mouse input focused widget has been reach
			// and we have modal mouse input focus cancel the distribution.
			if (mFocusHandler->getModalMouseInputFocused() != nullptr
				&& !widget->hasModalMouseInputFocus())
			{
				break;
			}
		}
	}

	void Gui::distributeKeyEvent(KeyEvent& keyEvent)
	{
		auto parent = keyEvent.getSource();
		auto widget = keyEvent.getSource();

		if (mFocusHandler->getModalFocused() != nullptr
			&& !widget->hasModalFocus())
		{
			return;
		}

		if (mFocusHandler->getModalMouseInputFocused() != nullptr
			&& !widget->hasModalMouseInputFocus())
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

			parent = static_cast<Widget*>(widget->getParent());

			if (widget->isEnabled())
			{
				auto keyListeners = widget->_getKeyListeners();

				// Send the event to all key listeners of the source widget.
				for (auto& keyListener : keyListeners)
				{
					switch (keyEvent.getType())
					{
					case KeyEvent::PRESSED:
						keyListener->keyPressed(keyEvent);
						break;
					case KeyEvent::RELEASED:
						keyListener->keyReleased(keyEvent);
						break;
					default:
						throw GCN_EXCEPTION("Unknown key event type.");
					}
				}
			}

			auto swap = widget;
			widget = parent;
			parent = static_cast<Widget*>(swap->getParent());

			// If a non modal focused widget has been reach
			// and we have modal focus cancel the distribution.
			if (mFocusHandler->getModalFocused() != nullptr
				&& !widget->hasModalFocus())
			{
				break;
			}
		}
	}

	void Gui::distributeKeyEventToGlobalKeyListeners(KeyEvent& keyEvent)
	{
		for (auto& mKeyListener : mKeyListeners)
		{
			switch (keyEvent.getType())
			{
			case KeyEvent::PRESSED:
				mKeyListener->keyPressed(keyEvent);
				break;
			case KeyEvent::RELEASED:
				mKeyListener->keyReleased(keyEvent);
				break;
			default:
				throw GCN_EXCEPTION("Unknown key event type.");
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
		if (mFocusHandler->getLastWidgetWithModalMouseInputFocus()
			!= mFocusHandler->getModalMouseInputFocused()
			&& (mFocusHandler->getLastWidgetWithModalMouseInputFocus() == nullptr))
		{
			handleModalFocusGained();
			mFocusHandler->setLastWidgetWithModalMouseInputFocus(mFocusHandler->getModalMouseInputFocused());
		}
			// Check if modal mouse input focus has been released.
		else if (mFocusHandler->getLastWidgetWithModalMouseInputFocus()
			!= mFocusHandler->getModalMouseInputFocused()
			&& mFocusHandler->getLastWidgetWithModalMouseInputFocus() != nullptr)
		{
			handleModalFocusReleased();
			mFocusHandler->setLastWidgetWithModalMouseInputFocus(nullptr);
		}
	}

	void Gui::handleModalFocus()
	{
		// Check if modal focus has been gained by a widget.
		if (mFocusHandler->getLastWidgetWithModalFocus()
			!= mFocusHandler->getModalFocused()
			&& mFocusHandler->getLastWidgetWithModalFocus() == nullptr)
		{
			handleModalFocusGained();
			mFocusHandler->setLastWidgetWithModalFocus(mFocusHandler->getModalFocused());
		}
			// Check if modal focus has been released.
		else if (mFocusHandler->getLastWidgetWithModalFocus()
			!= mFocusHandler->getModalFocused()
			&& mFocusHandler->getLastWidgetWithModalFocus() != nullptr)
		{
			handleModalFocusReleased();
			mFocusHandler->setLastWidgetWithModalFocus(nullptr);
		}
	}

	void Gui::handleModalFocusGained()
	{
		// Distribute an event to all widgets in the "widget with mouse" queue.
		while (!mWidgetWithMouseQueue.empty())
		{
			auto widget = mWidgetWithMouseQueue.front();

			if (Widget::widgetExists(widget))
			{
				distributeMouseEvent(widget,
				                     MouseEvent::EXITED,
				                     mLastMousePressButton,
				                     mLastMouseX,
				                     mLastMouseY,
				                     true,
				                     true);
			}

			mWidgetWithMouseQueue.pop_front();
		}

		mFocusHandler->setLastWidgetWithModalMouseInputFocus(mFocusHandler->getModalMouseInputFocused());
	}

	void Gui::handleModalFocusReleased()
	{
		// Check all widgets below the mouse to see if they are
		// present in the "widget with mouse" queue. If a widget
		// is not then it should be added and an entered event should
		// be sent to it.
		auto widget = getMouseEventSource(mLastMouseX, mLastMouseY);
		auto parent = widget;

		while (parent != nullptr)
		{
			parent = static_cast<Widget*>(widget->getParent());

			// Check if the widget is present in the "widget with mouse" queue.
			auto widgetIsPresentInQueue = false;
			for (auto& iter : mWidgetWithMouseQueue)
			{
				if (iter == widget)
				{
					widgetIsPresentInQueue = true;
					break;
				}
			}

			// Widget is not present, send an entered event and add
			// it to the "widget with mouse" queue.
			if (!widgetIsPresentInQueue
				&& Widget::widgetExists(widget))
			{
				distributeMouseEvent(widget,
				                     MouseEvent::ENTERED,
				                     mLastMousePressButton,
				                     mLastMouseX,
				                     mLastMouseY,
				                     false,
				                     true);
				mWidgetWithMouseQueue.push_front(widget);
			}

			auto swap = widget;
			widget = parent;
			parent = static_cast<Widget*>(swap->getParent());
		}
	}
}
