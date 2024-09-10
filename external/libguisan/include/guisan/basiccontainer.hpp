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

#ifndef GCN_BASICCONTAINER_HPP
#define GCN_BASICCONTAINER_HPP

#include <list>

#include "guisan/deathlistener.hpp"
#include "guisan/platform.hpp"
#include "guisan/widget.hpp"

namespace gcn
{
    /**
     * A base class for containers. The class implements the most
     * common things for a container. If you are implementing a
     * container, consider inheriting from this class.
     *
     * @see Container
     * @since 0.6.0
     */
    class GCN_CORE_DECLSPEC BasicContainer : public Widget, public DeathListener
    {
    public:
        /**
         * Constructor.
         */
        BasicContainer();

        /**
         * Destructor
         */
        virtual ~BasicContainer();

        // Inherited from Widget

        /**
         * Shows a certain part of a widget in the basic container.
         * Used when widgets want a specific part to be visible in
         * its parent. An example is a TextArea that wants a specific
         * part of its text to be visible when a TextArea is a child
         * of a ScrollArea.
         *
         * @param widget The widget whom wants a specific part of
         *               itself to be visible.
         * @param rectangle The rectangle to be visible.
         */
        virtual void showWidgetPart(Widget* widget, Rectangle area);

        virtual void moveToTop(Widget* widget);

        virtual void moveToBottom(Widget* widget);

        virtual Rectangle getChildrenArea();

        virtual void focusNext();

        virtual void focusPrevious();

        virtual void logic();

        virtual void _setFocusHandler(FocusHandler* focusHandler);

        void setInternalFocusHandler(FocusHandler* focusHandler);

        virtual Widget *getWidgetAt(int x, int y);

        virtual std::list<Widget*> getWidgetsIn(const Rectangle& area, Widget* ignore = NULL);

        // Inherited from DeathListener

        virtual void death(const Event& event);

    protected:
        /**
         * Moves a widget to the top. Normally one wants to use
         * Widget::moveToTop instead.
         *
         * THIS METHOD IS NOT SAFE TO CALL INSIDE A WIDGETS LOGIC FUNCTION
         * INSIDE ANY LISTENER FUNCTIONS!
         *
         * @param widget The widget to move to the top.
         * @since 1.1.0
         */
        void _moveToTopWithNoChecks(Widget* widget);

        /**
         * Moves a widget to the bottom. Normally one wants to use
         * Widget::moveToBottom instead.
         *
         * THIS METHOD IS NOT SAFE TO CALL INSIDE A WIDGETS LOGIC FUNCTION
         * INSIDE ANY LISTENER FUNCTIONS!
         *
         * @param The widget to move to the bottom.
         * @since 1.1.0
         */
        void _moveToBottomWithNoChecks(Widget* widget);

        /**
         * Adds a widget to the basic container.
         *
         * THIS METHOD IS NOT SAFE TO CALL INSIDE A WIDGETS LOGIC FUNCTION
         * INSIDE ANY LISTENER FUNCTIONS!
         *
         * @param widget The widget to add.
         * @see remove, clear
         */
        void add(Widget* widget);

        /**
         * Removes a widget from the basic container.
         *
         * THIS METHOD IS NOT SAFE TO CALL INSIDE A WIDGETS LOGIC FUNCTION
         * INSIDE ANY LISTENER FUNCTIONS!
         *
         * @param widget The widget to remove.
         * @see add, clear
         */
        virtual void remove(Widget* widget);

        /**
         * Clears the basic container from all widgets.
         *
         * THIS METHOD IS NOT SAFE TO CALL INSIDE A WIDGETS LOGIC FUNCTION
         * INSIDE ANY LISTENER FUNCTIONS!
         *
         * @see remove, clear
         */
        virtual void clear();

        /**
         * Draws the children widgets of the basic container.
         *
         * @param graphics A graphics object to draw with.
         */
        virtual void drawChildren(Graphics* graphics);

        /**
         * Calls logic for the children widgets of the basic
         * container.
         */
        virtual void logicChildren();

        /**
         * Finds a widget given an id. This function can be useful
         * when implementing a GUI generator for Guisan, such as
         * the ability to create a Guisan GUI from an XML file.
         *
         * @param id The id to find a widget by.
         * @return The widget with the corrosponding id, 
                   NULL of no widget is found.
         */
        virtual Widget* findWidgetById(const std::string& id);

        /**
         * Resizes the BasicContainer to fit it's content exactly.
         */
        void resizeToContent();

        typedef std::list<Widget *> WidgetList;
        typedef WidgetList::iterator WidgetListIterator;
        typedef WidgetList::reverse_iterator WidgetListReverseIterator;

        /**
         * Holds all widgets of the basic container.
         */
        WidgetList mWidgets;

        /**
         * Holds a widget that should be moved to the top.
         */
        Widget* mWidgetToBeMovedToTheTop;

        /**
         * Holds a widget that should be moved to the bottom.
         */
        Widget* mWidgetToBeMovedToTheBottom;

        /**
         * True if logic is currently being processed, false otherwise.
         */
        bool mLogicIsProcessing;
    };
}

#endif // end GCN_BASICCONTAINER_HPP
