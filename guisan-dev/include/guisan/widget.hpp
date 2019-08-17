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

#ifndef GCN_WIDGET_HPP
#define GCN_WIDGET_HPP

#include <list>
#include <string>

#include "guisan/color.hpp"
#include "guisan/rectangle.hpp"

namespace gcn
{
    class ActionListener;
    class BasicContainer;
    class DeathListener;
    class DefaultFont;
    class FocusHandler;
    class FocusListener;
    class Font;
    class Graphics;
    class KeyInput;
    class KeyListener;
    class MouseInput;
    class MouseListener;
    class WidgetListener;

    /**
     * Widget base class. Contains basic widget functions every widget should
     * have. Widgets should inherit from this class and implements it's
     * functions.
     *
     * NOTE: Functions begining with underscore "_" should not
     *       be overloaded unless you know what you are doing
     *
     * @author Olof Naessén
     * @author Per Larsson.
     */
    class GCN_CORE_DECLSPEC Widget
    {
    public:
        /**
         * Constructor. Resets member variables. Noteable, a widget is not
         * focusable as default, therefore, widgets that are supposed to be
         * focusable should overide this default in their own constructor.
         */
        Widget();

        /**
         * Default destructor.
         */
        virtual ~Widget();

        /**
         * Draws the widget. It is called by the parent widget when it is time
         * for the widget to draw itself. The graphics object is set up so
         * that all drawing is relative to the widget, i.e coordinate (0,0) is
         * the top-left corner of the widget. It is not possible to draw
         * outside of a widgets dimension.
         *
         * @param graphics a Graphics object to draw with.
         */
        virtual void draw(Graphics* graphics) = 0;

        /**
         * Draws the widget border. A border is drawn around a widget.
         * The width and height of the border is therefore the widgets
         * height+2*bordersize. Think of a painting that has a certain size,
         * the border surrounds the painting.
         *
         * @param graphics a Graphics object to draw with.
         */
        virtual void drawBorder(Graphics* graphics) { }

        /**
         * Called for all widgets in the gui each time Gui::logic is called.
         * You can do logic stuff here like playing an animation.
         *
         * @see Gui
         */
        virtual void logic() { }

        /**
         * Gets the widget parent container.
         *
         * @return the widget parent container. Returns NULL if the widget
         *         has no parent.
         */
        virtual Widget* getParent() const;

        /**
         * Sets the width of the widget in pixels.
         *
         * @param width the widget width in pixels.
         */
        void setWidth(int width);

        /**
         * Gets the width of the widget in pixels.
         *
         * @return the widget with in pixels.
         */
        int getWidth() const;

        /**
         * Sets the height of the widget in pixels.
         *
         * @param height the widget height in pixels.
         */
        void setHeight(int height);

        /**
         * Gets the height of the widget in pixels.
         *
         * @return the widget height in pixels.
         */
        int getHeight() const;

        /**
         * Sets the size of the widget.
         *
         * @param width the width.
         * @param height the height.
         */
        void setSize(int width, int height);

        /**
         * Set the widget x coordinate. It is relateive to it's parent.
         *
         * @param x the widget x coordinate.
         */
        void setX(int x);

        /**
         * Gets the widget x coordinate. It is relative to it's parent.
         *
         * @return the widget x coordinate.
         */
        int getX() const;

        /**
         * Set the widget y coordinate. It is relative to it's parent.
         *
         * @param y the widget y coordinate.
         */
        void setY(int y);

        /**
         * Gets the widget y coordinate. It is relative to it's parent.
         *
         * @return the widget y coordinate.
         */
        int getY() const;

        /**
         * Sets the widget position. It is relative to it's parent.
         *
         * @param x the widget x coordinate.
         * @param y the widgets y coordinate.
         */
        void setPosition(int x, int y);

        /**
         * Sets the dimension of the widget. It is relative to it's parent.
         *
         * @param dimension the widget dimension.
         */
        void setDimension(const Rectangle& dimension);

        /**
         * Sets the size of the border, or the width if you so like. The size
         * is the number of pixels that the border extends outside the widget.
         * Border size = 0 means no border.
         *
         * @param borderSize the size of the border.
         * @see drawBorder
         */
        void setBorderSize(unsigned int borderSize);

        /**
         * Gets the size of the border, or the width if you so like. The size
         * is the number of pixels that the border extends outside the widget.
         * Border size = 0 means no border.
         *
         * @return the size of the border.
         * @see drawBorder
         */
        unsigned int getBorderSize() const;

        /**
         * Gets the dimension of the widget. It is relative to it's parent.
         *
         * @return the widget dimension.
         */
        const Rectangle& getDimension() const;

        /**
         * Sets a widgets focusability.
         *
         * @param focusable true if the widget should be focusable.
         */
        void setFocusable(bool focusable);

        /**
         * Checks whether the widget is focusable.
         *
         * @return true if the widget is focusable.
         */
        bool isFocusable() const;

        /**
         * Checks if the widget is focused.
         *
         * @return true if the widget currently has focus.
         */
        virtual bool isFocused() const;

        /**
         * Sets the widget to be disabled or enabled. A disabled
         * widget will never recieve mouse or key input.
         *
         * @param enabled true if widget is enabled.
         */
        void setEnabled(bool enabled);

        /**
         * Checks if a widget is disabled or not.
         *
         * @return true if the widget should be enabled.
         */
        bool isEnabled() const;

        /**
         * Sets the widget to be visible.
         *
         * @param visible true if the widget should be visiable.
         */
        void setVisible(bool visible);

        /**
         * Checks if the widget is visible.
         *
         * @return true if the widget is visible.
         */
        bool isVisible() const;

        /**
         * Sets the base color. The base color is the background
         * color for many widgets like the Button and Contianer widgets.
         *
         * @param color the baseground color.
         */
        void setBaseColor(const Color& color);

        /**
         * Gets the base color.
         *
         * @return the foreground color.
         */
        const Color& getBaseColor() const;

        /**
         * Sets the foreground color.
         *
         * @param color the foreground color.
         */
        void setForegroundColor(const Color& color);

        /**
         * Gets the foreground color.
         *
         * @return the foreground color.
         */
        const Color& getForegroundColor() const;

        /**
         * Sets the background color.
         *
         * @param color the background Color.
         */
        void setBackgroundColor(const Color& color);

        /**
         * Gets the background color.
         *
         * @return the background color.
         */
        const Color& getBackgroundColor() const;

        /**
         * Sets the selection color.
         *
         * @param color the selection color.
         */
        void setSelectionColor(const Color& color);

        /**
         * Gets the selection color.
         *
         * @return the selection color.
         */
        const Color& getSelectionColor() const;
        
        /**
         * Requests focus for the widget. A widget will only recieve focus
         * if it is focusable.
         */
        virtual void requestFocus();

        /**
         * Requests a move to the top in the parent widget.
         */
        virtual void requestMoveToTop();

        /**
         * Requests a move to the bottom in the parent widget.
         */
        virtual void requestMoveToBottom();

        /**
         * Sets the FocusHandler to be used.
         *
         * WARNING: This function is used internally and should not
         *          be called or overloaded unless you know what you
         *          are doing.
         *
         * @param focusHandler the FocusHandler to use.
         */
        virtual void _setFocusHandler(FocusHandler* focusHandler);

        /**
         * Gets the FocusHandler used.
         *
         * WARNING: This function is used internally and should not
         *          be called or overloaded unless you know what you
         *          are doing.
         *
         * @return the FocusHandler used.
         */
        virtual FocusHandler* _getFocusHandler();

        /**
         * Adds an ActionListener to the widget. When an action is triggered
         * by the widget, the action function in all the widget's
         * ActionListeners will be called.
         *
         * @param actionListener the ActionListener to add.
         */
        void addActionListener(ActionListener* actionListener);

        /**
         * Removes an added ActionListener from the widget.
         *
         * @param actionListener the ActionListener to remove.
         */
        void removeActionListener(ActionListener* actionListener);

        /**
         * Adds a DeathListener to the widget. When the widget dies
         * the death function in all the widget's DeathListeners will be called.
         *
         * @param actionListener the DeathListener to add.
         */
        void addDeathListener(DeathListener* deathListener);

        /**
         * Removes an added DeathListener from the widget.
         *
         * @param deathListener the DeathListener to remove.
         */
        void removeDeathListener(DeathListener* deathListener);

        /**
         * Adds a MouseListener to the widget. When a mouse message is
         * recieved, it will be sent to the widget's MouseListeners.
         *
         * @param mouseListener the MouseListener to add.
         */
        void addMouseListener(MouseListener* mouseListener);

        /**
         * Removes an added MouseListener from the widget.
         *
         * @param mouseListener the MouseListener to remove.
         */
        void removeMouseListener(MouseListener* mouseListener);

        /**
         * Adds a KeyListener to the widget. When a key message is recieved,
         * it will be sent to the widget's KeyListeners.
         *
         * @param keyListener the KeyListener to add.
         */
        void addKeyListener(KeyListener* keyListener);

        /**
         * Removes an added KeyListener from the widget.
         *
         * @param keyListener the KeyListener to remove.
         */
        void removeKeyListener(KeyListener* keyListener);

        /**
         * Adds a FocusListener to the widget. When a focus event is recieved,
         * it will be sent to the widget's FocusListeners.
         *
         * @param focusListener the FocusListener to add.
         * @author Olof Naessén
         * @since 0.7.0
         */
        void addFocusListener(FocusListener* focusListener);

        /**
         * Removes an added FocusListener from the widget.
         *
         * @param focusListener the FocusListener to remove.
         * @author Olof Naessén
         * @since 0.7.0
         */
        void removeFocusListener(FocusListener* focusListener);

        /**
         * Adds a WidgetListener to the widget.
         *
         * @param widgetListener the WidgetListener to add.
         * @author Olof Naessén
         * @since 0.8.0
         */
        void addWidgetListener(WidgetListener* widgetListener);

        /**
         * Removes an added WidgetListener from the widget.
         *
         * @param widgetListener the WidgetListener to remove.
         * @author Olof Naessén
         * @since 0.8.0
         */
        void removeWidgetListener(WidgetListener* widgetListener);

        /**
         * Sets the action event identifier of the widget. The identifier is
         * used to be able to identify which action has occured.
         *
         * NOTE: An action event identifier should not be used to identify a
         *       certain widget but rather a certain event in your application.
         *       Several widgets can have the same action event identifer.
         *
         * @param actionEventId the action event identifier.
         * @since 0.6.0
         */
        void setActionEventId(const std::string& actionEventId);

        /**
         * Gets the action event identifier.
         *
         * @return the action event identifier.
         */
        const std::string& getActionEventId() const;

        /**
         * Gets the absolute position on the screen for the widget.
         *
         * @param x absolute x coordinate will be stored in this parameter.
         * @param y absolute y coordinate will be stored in this parameter.
         */
        virtual void getAbsolutePosition(int& x, int& y) const;

        /**
         * Sets the parent of the widget. A parent must be a BasicContainer.
         *
         * WARNING: This function is used internally and should not
         *          be called or overloaded unless you know what you
         *          are doing.
         *
         * @param parent the parent BasicContainer..
         */
        virtual void _setParent(Widget* parent);

        /**
         * Gets the font used. If no font has been set, the global font will
         * be returned instead. If no global font has been set, the default
         * font will be returend.
         * ugly default.
         *
         * @return the used Font.
         */
        Font *getFont() const;

        /**
         * Sets the global font to be used by default for all widgets.
         *
         * @param font the global Font.
         */
        static void setGlobalFont(Font* font);

        /**
         * Sets the font. If font is NULL, the global font will be used.
         *
         * @param font the Font.
         */
        void setFont(Font* font);

        /**
         * Called when the font has changed. If the change is global,
         * this function will only be called if the widget don't have a
         * font already set.
         */
        virtual void fontChanged() { }

        /**
         * Checks whether a widget exists or not, that is if it still exists
         * an instance of the object.
         *
         * @param widget the widget to check.
         */
        static bool widgetExists(const Widget* widget);

        /**
         * Check if tab in is enabled. Tab in means that you can set focus
         * to this widget by pressing the tab button. If tab in is disabled
         * then the FocusHandler will skip this widget and focus the next
         * in its focus order.
         *
         * @return true if tab in is enabled.
         */
        bool isTabInEnabled() const;

        /**
         * Sets tab in enabled. Tab in means that you can set focus
         * to this widget by pressing the tab button. If tab in is disabled
         * then the FocusHandler will skip this widget and focus the next
         * in its focus order.
         *
         * @param enabled true if tab in should be enabled.
         */
        void setTabInEnabled(bool enabled);

        /**
         * Checks if tab out is enabled. Tab out means that you can lose
         * focus to this widget by pressing the tab button. If tab out is
         * disabled then the FocusHandler ignores tabbing and focus will
         * stay with this widget.
         *
         * @return true if tab out is enabled.
         */
        bool isTabOutEnabled() const;

        /**
         * Sets tab out enabled. Tab out means that you can lose
         * focus to this widget by pressing the tab button. If tab out is
         * disabled then the FocusHandler ignores tabbing and focus will
         * stay with this widget.
         *
         * @param enabled true if tab out should be enabled.
         */
        void setTabOutEnabled(bool enabled);

        /**
         * Requests modal focus. When a widget has modal focus, only that
         * widget and it's children may recieve input.
         *
         * @throws Exception if another widget already has modal focus.
         */
        virtual void requestModalFocus();

        /**
         * Requests modal mouse input focus. When a widget has modal input focus
         * that widget will be the only widget receiving input even if the input
         * occurs outside of the widget and no matter what the input is.
         *
         * @throws Exception if another widget already has modal focus.
         * @since 0.6.0
         */
        virtual void requestModalMouseInputFocus();

        /**
         * Releases modal focus. Modal focus will only be released if the
         * widget has modal focus.
         */
        virtual void releaseModalFocus();

        /**
         * Releases modal mouse input focus. Modal mouse input focus will only
         * be released if the widget has modal mouse input focus.
         *
         * @since 0.6.0
         */
        virtual void releaseModalMouseInputFocus();

        /**
         * Checks if the widget or it's parent has modal focus.
         */
        virtual bool hasModalFocus() const;

        /**
         * Checks if the widget or it's parent has modal mouse input focus.
         *
         * @since 0.6.0
         */
        virtual bool hasModalMouseInputFocus() const;

        /**
         * Gets a widget from a certain position in the widget.
         * This function is used to decide which gets mouse input,
         * thus it can be overloaded to change that behaviour.
         *
         * NOTE: This always returns NULL if the widget is not
         *       a container.
         *
         * @param x the x coordinate.
         * @param y the y coordinate.
         * @return the widget at the specified coodinate, or NULL
         *         if no such widget exists.
         * @since 0.6.0
         */
        virtual Widget *getWidgetAt(int x, int y);

        /**
         * Gets the mouse listeners of the widget.
         *
         * @return the mouse listeners of the widget.
         * @since 0.6.0
         */
        virtual const std::list<MouseListener*>& _getMouseListeners();

        /**
         * Gets the key listeners of the widget.
         *
         * @return the key listeners of the widget.
         * @since 0.6.0
         */
        virtual const std::list<KeyListener*>& _getKeyListeners();

        /**
         * Gets the focus listeners of the widget.
         *
         * @return the focus listeners of the widget.
         * @since 0.7.0
         */
        virtual const std::list<FocusListener*>& _getFocusListeners();

        /**
         * Gets the subarea of the widget that the children occupy.
         *
         * @return the subarea as a Rectangle.
         */
        virtual Rectangle getChildrenArea();

        /**
         * Gets the internal FocusHandler used.
         *
         * @return the internalFocusHandler used. If no internal FocusHandler
         *         is used, NULL will be returned.
         */
        virtual FocusHandler* _getInternalFocusHandler();

        /**
         * Sets the internal FocusHandler. An internal focushandler is
         * needed if both a widget in the widget and the widget itself
         * should be foucsed at the same time.
         *
         * @param focusHandler the FocusHandler to be used.
         */
        void setInternalFocusHandler(FocusHandler* focusHandler);

        /**
         * Moves a widget to the top of this widget. The moved widget will be
         * drawn above all other widgets in this widget.
         *
         * @param widget the widget to move.
         */
        virtual void moveToTop(Widget* widget) { };

        /**
         * Moves a widget in this widget to the bottom of this widget.
         * The moved widget will be drawn below all other widgets in this widget.
         *
         * @param widget the widget to move.
         */
        virtual void moveToBottom(Widget* widget) { };

        /**
         * Focuses the next widget in the widget.
         */
        virtual void focusNext() { };

        /**
         * Focuses the previous widget in the widget.
         */
        virtual void focusPrevious() { };

        /**
         * Tries to show a specific part of a widget by moving it. Used if the
         * widget should act as a container.
         *
         * @param widget the target widget.
         * @param area the area to show.
         */
        virtual void showWidgetPart(Widget* widget, Rectangle area) { };

        /**
         * Sets an id of a widget. An id can be useful if a widget needs to be
         * identified in a container. For example, if widgets are created by an
         * XML document, a certain widget can be retrieved given that the widget
         * has an id.
         *
         * @param id the id to set to the widget.
         * @see BasicContainer::findWidgetById
         */
        void setId(const std::string& id);

        /**
         * Gets the id of a widget. An id can be useful if a widget needs to be
         * identified in a container. For example, if widgets are created by an
         * XML document, a certain widget can be retrieved given that the widget
         * has an id.
         *
         * @param id the id to set to the widget.
         * @see BasicContainer::findWidgetById
         */
        const std::string& getId();

    protected:
        /**
         * Generates an action to the widget's ActionListeners.
         */
        void generateAction();

        /**
         * Distributes resized events to all of the widget's listeners.
         *
         * @since 0.8.0
         * @author Olof Naessén
         */
        void distributeResizedEvent();

        /**
         * Distributes moved events to all of the widget's listeners.
         *
         * @since 0.8.0
         * @author Olof Naessén
         */
        void distributeMovedEvent();

        /**
         * Distributes hidden events to all of the widget's listeners.
         *
         * @since 0.8.0
         * @author Olof Naessén
         */
        void distributeHiddenEvent();

        /**
         * Distributes shown events to all of the widget's listeners.
         *
         * @since 0.8.0
         * @author Olof Naessén
         */
        void distributeShownEvent();

        typedef std::list<MouseListener*> MouseListenerList;
        typedef MouseListenerList::iterator MouseListenerIterator;
        MouseListenerList mMouseListeners;

        typedef std::list<KeyListener*> KeyListenerList;
        KeyListenerList mKeyListeners;
        typedef KeyListenerList::iterator KeyListenerIterator;

        typedef std::list<ActionListener*> ActionListenerList;
        ActionListenerList mActionListeners;
        typedef ActionListenerList::iterator ActionListenerIterator;

        typedef std::list<DeathListener*> DeathListenerList;
        DeathListenerList mDeathListeners;
        typedef DeathListenerList::iterator DeathListenerIterator;

        typedef std::list<FocusListener*> FocusListenerList;
        FocusListenerList mFocusListeners;
        typedef FocusListenerList::iterator FocusListenerIterator;

        typedef std::list<WidgetListener*> WidgetListenerList;
        WidgetListenerList mWidgetListeners;
        typedef WidgetListenerList::iterator WidgetListenerIterator;

        Color mForegroundColor;
        Color mBackgroundColor;
        Color mBaseColor;
        Color mSelectionColor;
        FocusHandler* mFocusHandler;
        FocusHandler* mInternalFocusHandler;
        Widget* mParent;
        Rectangle mDimension;
        unsigned int mBorderSize;
        std::string mActionEventId;
        bool mFocusable;
        bool mVisible;
        bool mTabIn;
        bool mTabOut;
        bool mEnabled;
        std::string mId;

        Font* mCurrentFont;
        static DefaultFont mDefaultFont;
        static Font* mGlobalFont;
        static std::list<Widget*> mWidgets;
    };
}

#endif // end GCN_WIDGET_HPP

/*
 * yakslem  - "I have a really great idea! Why not have an
 *             interesting and funny quote or story at the
 *             end of every source file."
 * finalman - "Yeah - good idea! .... do you know any funny
 *             quotes?"
 * yakslem  - "Well.. I guess not. I just thought it would be
 *             fun to tell funny quotes."
 * finalman - "That's not a very funny quote. But i believe
 *             pointless quotes are funny in their own pointless
 *             way."
 * yakslem  - "...eh...ok..."
 */
