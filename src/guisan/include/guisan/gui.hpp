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

#ifndef GCN_GUI_HPP
#define GCN_GUI_HPP

#include <list>
#include <deque>

#include "guisan/keyevent.hpp"
#include "guisan/mouseevent.hpp"
#include "guisan/mouseinput.hpp"
#include "guisan/platform.hpp"

namespace gcn
{
    class FocusHandler;
    class Graphics;
    class Input;
    class KeyListener;
    class Widget;

    // The following comment will appear in the doxygen main page.
    /**
     * @mainpage
     * @section Introduction
     * This documentation is mostly intended as a reference to the API. If you want to get started with Guichan, we suggest you check out the programs in the examples directory of the Guichan release.
     * @n
     * @n
     * This documentation is, and will always be, work in progress. If you find any errors, typos or inconsistencies, or if you feel something needs to be explained in more detail - don't hesitate to tell us.
     */

    /**
     * Gui core class. Contains a special widget called the top widget.
     * If you want to be able to have more then one Widget in your Gui,
     * the top widget should be a Container.
     *
     * NOTE: For the Gui to function properly you need to set a Graphics
     *       object to use and an Input object to use.
     */
    class GCN_CORE_DECLSPEC Gui
    {
    public:

        /**
         * Constructor.
         */
        Gui();

        /**
         * Destructor.
         */
        virtual ~Gui();

        /**
         * Sets the top Widget.
         *
         * @param top the top Widget.
         */
        virtual void setTop(Widget* top);

        /**
         * Gets the top Widget.
         *
         * @return the top widget. NULL if no top widget has been set.
         */
        virtual Widget* getTop() const;

        /**
         * Sets the Graphics object to use for drawing.
         *
         * @param graphics the Graphics object to use for drawing.
         * @see SDLGraphics, OpenGLGraphics, AllegroGraphics
         */
        virtual void setGraphics(Graphics* graphics);

        /**
         * Gets the Graphics object used for drawing.
         *
         *  @return the Graphics object used for drawing. NULL if no
         *          Graphics object has been set.
         */
        virtual Graphics* getGraphics() const;

        /**
         * Sets the Input object to use for input handling.
         *
         * @param input the Input object to use for input handling.
         * @see SDLInput, AllegroInput
         */
        virtual void setInput(Input* input);

        /**
         * Gets the Input object being used for input handling.
         *
         *  @return the Input object used for handling input. NULL if no
         *          Input object has been set.
         */
        virtual Input* getInput() const;

        /**
         * Performs the Gui logic. By calling this function all logic
         * functions down in the Gui heirarchy will be called.
         * What performs in Logic can be just about anything like
         * adjusting a Widgets size or doing some calculations.
         *
         * NOTE: Logic also deals with user input (Mouse and Keyboard)
         *       for Widgets.
         */
        virtual void logic();

        /**
         * Draws the Gui. By calling this funcion all draw functions
         * down in the Gui hierarchy will be called.
         */
        virtual void draw();

        /**
         * Focus none of the Widgets in the Gui.
         */
        virtual void focusNone();

        /**
         * Toggles the use of the tab key to focus Widgets.
         * By default, tabbing is enabled.
         *
         * @param tabbing set to false if you want to disable tabbing.
         */
        virtual void setTabbingEnabled(bool tabbing);

        /**
         * Checks if tabbing is enabled.
         *
         * @return true if tabbing is enabled.
         */
        virtual bool isTabbingEnabled();

        /**
         * Adds a global KeyListener to the Gui.
         *
         * @param keyListener a KeyListener to add.
         */
        virtual void addGlobalKeyListener(KeyListener* keyListener);

        /**
         * Remove global KeyListener from the Gui.
         *
         * @param keyListener a KeyListener to remove.
         * @throws Exception if the KeyListener hasn't been added.
         */
        virtual void removeGlobalKeyListener(KeyListener* keyListener);

    protected:
        /**
         * Handles all mouse input.
         *
         * @since 0.6.0
         */
        virtual void handleMouseInput();

        /**
         * Handles key input.
         *
         * @since 0.6.0
         */
        virtual void handleKeyInput();

        /**
         * Handles mouse moved input.
         *
         * @param mouseInput the mouse input to handle.
         * @since 0.6.0
         */
        virtual void handleMouseMoved(const MouseInput& mouseInput);

        /**
         * Handles mouse pressed input.
         *
         * @param mouseInput the mouse input to handle.
         * @since 0.6.0
         */
        virtual void handleMousePressed(const MouseInput& mouseInput);

        /**
         *
         * Handles mouse wheel moved down input.
         *
         * @param mouseInput the mouse input to handle.
         * @since 0.6.0
         */
        virtual void handleMouseWheelMovedDown(const MouseInput& mouseInput);

        /**
         * Handles mouse wheel moved up input.
         *
         * @param mouseInput the mouse input to handle.
         * @since 0.6.0
         */
        virtual void handleMouseWheelMovedUp(const MouseInput& mouseInput);

        /**
         * Handles mouse released input.
         *
         * @param mouseInput the mouse input to handle.
         * @since 0.6.0
         */
        virtual void handleMouseReleased(const MouseInput& mouseInput);

        /**
         * Handles modal focus. Modal focus needs to be checked at 
         * each logic iteration as it might be necessary to distribute
         * mouse entered or mouse exited events.
         *
         * @since 0.8.0
         */
        virtual void handleModalFocus();

        /**
         * Handles modal mouse input focus. Modal mouse input focus needs 
         * to be checked at each logic iteration as it might be necessary to 
         * distribute mouse entered or mouse exited events.
         *
         * @since 0.8.0
         */
        virtual void handleModalMouseInputFocus();

        /**
         * Handles modal focus gained. If modal focus has been gaind it might 
         * be necessary to distribute mouse entered or mouse exited events.
         *
         * @since 0.8.0
         */
        virtual void handleModalFocusGained();

        /**
         * Handles modal mouse input focus gained. If modal focus has been gaind 
         * it might be necessary to distribute mouse entered or mouse exited events.
         *
         * @since 0.8.0
         */
        virtual void handleModalFocusReleased();

        /**
         * Distributes a mouse event.
         *
         * @param type The type of the event to distribute,
         * @param button The button of the event (if any used) to distribute.
         * @param x The x coordinate of the event.
         * @param y The y coordinate of the event.
         * @param fource indicates whether the distribution should be forced or not.
         *               A forced distribution distributes the event even if a widget
         *               is not enabled, not visible, another widget has modal
         *               focus or another widget has modal mouse input focus. 
         *               Default value is false.
         * @param toSourceOnly indicates whether the distribution should be to the
         *                     source widget only or to it's parent's mouse listeners
         *                     as well.
         *
         * @since 0.6.0
         */
        virtual void distributeMouseEvent(Widget* source,
                                          int type,
                                          int button,
                                          int x,
                                          int y,
                                          bool force = false,
                                          bool toSourceOnly = false);

        /**
         * Distributes a key event.
         *
         * @param keyEvent the key event to distribute.

         * @since 0.6.0
         */
        virtual void distributeKeyEvent(KeyEvent& keyEvent);

        /**
         * Distributes a key event to the global key listeners.
         *
         * @param keyEvent the key event to distribute.
         *
         * @since 0.6.0
         */
        virtual void distributeKeyEventToGlobalKeyListeners(KeyEvent& keyEvent);

        /**
         * Gets the widget at a certain position.
         *
         * @return the widget at a certain position.
         * @since 0.6.0
         */
        virtual Widget* getWidgetAt(int x, int y);

        /**
         * Gets the source of the mouse event.
         *
         * @return the source widget of the mouse event.
         * @since 0.6.0
         */
        virtual Widget* getMouseEventSource(int x, int y);

        /**
         * Gets the source of the key event.
         *
         * @return the source widget of the key event.
         * @since 0.6.0
         */
        virtual Widget* getKeyEventSource();

        Widget* mTop;
        Graphics* mGraphics;
        Input* mInput;
        FocusHandler* mFocusHandler;

        bool mTabbing;

        typedef std::list<KeyListener*> KeyListenerList;
        typedef KeyListenerList::iterator KeyListenerListIterator;

        KeyListenerList mKeyListeners;
        
        // Current input state
        bool mShiftPressed;
        bool mMetaPressed;
        bool mControlPressed;
        bool mAltPressed;

        // Last mouse state
        unsigned int mLastMousePressButton;
        int mLastMousePressTimeStamp;
        int mLastMouseX;
        int mLastMouseY;
        int mClickCount;
        int mLastMouseDragButton;

        // Widget with mouse stack
        std::deque<Widget*> mWidgetWithMouseQueue;
    };
}

#endif // end GCN_GUI_HPP

/* yakslem  - "Women, it's a constant struggle."
 * finalman - "Yes, but sometimes they succeed with their guesses."
 * yaklsem  - "...eh...I was talking about love."
 * finalman - "Oh...ok..."
 * An awkward silence followed.
 */
