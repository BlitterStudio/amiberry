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

#ifndef GCN_SCROLLAREA_HPP
#define GCN_SCROLLAREA_HPP

#include <string>

#include "guisan/basiccontainer.hpp"
#include "guisan/mouselistener.hpp"
#include "guisan/platform.hpp"

namespace gcn
{
    /**
     * A ScrollArea in which another Widget can be added. It the other Widget
     * is bigger then the ScrollArea, the ScrollArea will only display the part
     * of the Widget that fits the ScrollArea and make it possible to scroll
     * to the other areas of the Widget.
     *
     * NOTE: A TextBox or a ListBox looks really ugly unless they exist in a
     *       ScrollArea.
     */
    class GCN_CORE_DECLSPEC ScrollArea:
        public BasicContainer,
        public MouseListener
    {
    public:

        /**
         * Constructor.
         */
        ScrollArea();

        /**
         * Constructor.
         *
         * @param content the content of the ScrollArea.
         */
        ScrollArea(Widget *content);

        /**
         * Constructor.
         *
         * @param content the content of the ScrollArea.
         * @param hPolicy the policy for the horizontal scrollbar. See enum with
         *                policies.
         * @param vPolicy the policy for the vertical scrollbar. See enum with
         *                policies.
         */
        ScrollArea(Widget *content, unsigned int hPolicy, unsigned int vPolicy);

        /**
         * Destructor.
         */
        virtual ~ScrollArea();

        /**
         * Sets the content.
         *
         * @param widget the content of the ScrollArea.
         */
        void setContent(Widget* widget);

        /**
         * Gets the content.
         *
         * @return the content of the ScrollArea.
         */
        Widget* getContent();

        /**
         * Sets the horizontal scrollbar policy. See enum with policies.
         *
         * @param hPolicy the policy for the horizontal scrollbar. See enum with
         *                policies.
         */
        void setHorizontalScrollPolicy(unsigned int hPolicy);

        /**
         * Gets the horizontal scrollbar policy. See enum with policies.
         *
         * @return the policy for the horizontal scrollbar policy. See enum with
         *         policies.
         */
        unsigned int getHorizontalScrollPolicy() const;

        /**
         * Sets the vertical scrollbar policy. See enum with policies.
         *
         * @param vPolicy the policy for the vertical scrollbar. See enum with
         *                policies.
         */
        void setVerticalScrollPolicy(unsigned int vPolicy);

        /**
         * Gets the vertical scrollbar policy. See enum with policies.
         *
         * @return the policy for the vertical scrollbar. See enum with
         *         policies.
         */
        unsigned int getVerticalScrollPolicy() const;

        /**
         * Sets the horizontal and vertical scrollbar policy. See enum with policies.
         *
         * @param hPolicy the policy for the horizontal scrollbar. See enum with
         *                policies.
         * @param vPolicy the policy for the vertical scrollbar. See enum with
         *                policies.
         */
        void setScrollPolicy(unsigned int hPolicy, unsigned int vPolicy);

        /**
         * Sets the amount to scroll vertically.
         *
         * @param vScroll the amount to scroll.
         */
        void setVerticalScrollAmount(int vScroll);

        /**
         * Gets the amount that is scrolled vertically.
         * @return the scroll amount on vertical scroll.
         */
        int getVerticalScrollAmount() const;

        /**
         * Sets the amount to scroll horizontally.
         *
         * @param hScroll the amount to scroll.
         */
        void setHorizontalScrollAmount(int hScroll);

        /**
         * Gets the amount that is scrolled horizontally.
         *
         * @return the scroll amount on horizontal scroll.
         */
        int getHorizontalScrollAmount() const;

        /**
         * Sets the amount to scroll horizontally and vertically.
         *
         * @param hScroll the amount to scroll on horizontal scroll.
         * @param vScroll the amount to scroll on vertical scroll.
         */
        void setScrollAmount(int hScroll, int vScroll);

        /**
         * Gets the maximum amount of horizontal scroll.
         *
         * @return the horizontal max scroll.
         */
        int getHorizontalMaxScroll();

        /**
         * Gets the maximum amount of vertical scroll.
         *
         * @return the vertical max scroll.
         */
        int getVerticalMaxScroll();

        /**
         * Sets the width.
         *
         * @param width the width of the ScrollBar.
         */
        void setScrollbarWidth(int width);

        /**
         * Gets the width.

         * @return the width of the ScrollBar.
         */
        int getScrollbarWidth() const;

        /**
         * Sets the amount to scroll in pixels when the left scroll button is
         * pushed.
         */
        void setLeftButtonScrollAmount(int amount);

        /**
         * Sets the amount to scroll in pixels when the right scroll button is
         * pushed.
         */
        void setRightButtonScrollAmount(int amount);

        /**
         * Sets the amount to scroll in pixels when the up scroll button is
         * pushed.
         */
        void setUpButtonScrollAmount(int amount);

        /**
         * Sets the amount to scroll in pixels when the down scroll button is
         * pushed.
         */
        void setDownButtonScrollAmount(int amount);

        /**
         * Gets the amount to scroll in pixels when the left scroll button is
         * pushed.
         *
         * @return the amount to scroll when the left scroll button is pushed.
         */
        int getLeftButtonScrollAmount() const;

        /**
         * Gets the amount to scroll in pixels when the right scroll button is
         * pushed.
         *
         * @return the amount to scroll when the right scroll button is pushed.
         */
        int getRightButtonScrollAmount() const;

        /**
         * Gets the amount to scroll in pixels when the up scroll button is
         * pushed.
         *
         * @return the amount to scroll when the up scroll button is pushed.
         */
        int getUpButtonScrollAmount() const;

        /**
         * Gets the amount to scroll in pixels when the down scroll button is
         * pushed.
         *
         * @return the amount to scroll when the down scroll button is pushed.
         */
        int getDownButtonScrollAmount() const;


        // Inherited from BasicContainer

        virtual void showWidgetPart(Widget* widget, Rectangle area);

        virtual Rectangle getChildrenArea();

        virtual Widget *getWidgetAt(int x, int y);


        // Inherited from Widget

        virtual void draw(Graphics *graphics);

        virtual void drawBorder(Graphics* graphics);

        virtual void logic();

        void setWidth(int width);

        void setHeight(int height);

        void setDimension(const Rectangle& dimension);


        // Inherited from MouseListener

        virtual void mousePressed(MouseEvent& mouseEvent);

        virtual void mouseReleased(MouseEvent& mouseEvent);

        virtual void mouseDragged(MouseEvent& mouseEvent);

        virtual void mouseWheelMovedUp(MouseEvent& mouseEvent);

        virtual void mouseWheelMovedDown(MouseEvent& mouseEvent);


        /**
         * Scrollpolicies for the horizontal and vertical scrollbar.
         * The policies are:
         *
         * SHOW_ALWAYS - Always show the scrollbars no matter what.
         * SHOW_NEVER  - Never show the scrollbars no matter waht.
         * SHOW_AUTO   - Show the scrollbars only when needed. That is if the
         *               content grows larger then the ScrollArea.
         */
        enum
        {
            SHOW_ALWAYS,
            SHOW_NEVER,
            SHOW_AUTO
        };

    protected:
        /**
         * Draws the background of the ScrollArea
         * (the area behind the content).
         *
         * @param graphics a Graphics object to draw with.
         */
        virtual void drawBackground(Graphics *graphics);

        /**
         * Draws the up button.
         *
         * @param graphics a Graphics object to draw with.
         */
        virtual void drawUpButton(Graphics *graphics);

        /**
         * Draws the down button.
         *
         * @param graphics a Graphics object to draw with.
         */
        virtual void drawDownButton(Graphics *graphics);

        /**
         * Draws the left button.
         *
         * @param graphics a Graphics object to draw with.
         */
        virtual void drawLeftButton(Graphics *graphics);

        /**
         * Draws the right button.
         *
         * @param graphics a Graphics object to draw with.
         */
        virtual void drawRightButton(Graphics *graphics);

        /**
         * Draws the vertical scrollbar.
         *
         * @param graphics a Graphics object to draw with.
         */
        virtual void drawVBar(Graphics* graphics);

        /**
         * Draws the horizontal scrollbar.
         *
         * @param graphics a Graphics object to draw with.
         */
        virtual void drawHBar(Graphics* graphics);

        /**
         * Draws the vertical marker.
         *
         * @param graphics a Graphics object to draw with.
         */
        virtual void drawVMarker(Graphics* graphics);

        /**
         * Draws the horizontal marker.
         *
         * @param graphics a Graphics object to draw with.
         */
        virtual void drawHMarker(Graphics* graphics);

        /**
         * Checks the policies for the scrollbars.
         */
        virtual void checkPolicies();

        /**
         * Gets the up button dimension.
         *
         * @return the dimension of the up button.
         */
        Rectangle getUpButtonDimension();

        /**
         * Gets the down button dimension.
         *
         * @return the dimension of the down button.
         */
        Rectangle getDownButtonDimension();

        /**
         * Gets the left button dimension.
         *
         * @return the dimension of the left button.
         */
        Rectangle getLeftButtonDimension();

        /**
         * Gets the right button dimension.
         *
         * @return the dimension of the right button.
         */
        Rectangle getRightButtonDimension();

        /**
         * Gets the vertical scrollbar dimension.
         *
         * @return the dimension of the vertical scrollbar.
         */
        Rectangle getVerticalBarDimension();

        /**
         * Gets the horizontal scrollbar dimension.
         *
         * @return the dimension of the horizontal scrollbar.
         */
        Rectangle getHorizontalBarDimension();

        /**
         * Gets the vertical marker dimension.
         *
         * @return the dimension of the vertical marker.
         */
        Rectangle getVerticalMarkerDimension();

        /**
         * Gets the horizontal marker dimension.
         *
         * @return the dimension of the horizontal marker.
         */
        Rectangle getHorizontalMarkerDimension();

        int mVScroll;
        int mHScroll;
        int mScrollbarWidth;
        unsigned int mHPolicy;
        unsigned int mVPolicy;
        bool mVBarVisible;
        bool mHBarVisible;
        bool mUpButtonPressed;
        bool mDownButtonPressed;
        bool mLeftButtonPressed;
        bool mRightButtonPressed;
        int mUpButtonScrollAmount;
        int mDownButtonScrollAmount;
        int mLeftButtonScrollAmount;
        int mRightButtonScrollAmount;
        bool mIsVerticalMarkerDragged;
        bool mIsHorizontalMarkerDragged;
        int mHorizontalMarkerDragOffset;
        int mVerticalMarkerDragOffset;
    };
}

#endif // end GCN_SCROLLAREA_HPP
