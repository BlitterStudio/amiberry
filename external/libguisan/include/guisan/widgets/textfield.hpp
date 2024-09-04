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

#ifndef GCN_TEXTFIELD_HPP
#define GCN_TEXTFIELD_HPP

#include "guisan/keylistener.hpp"
#include "guisan/mouselistener.hpp"
#include "guisan/platform.hpp"
#include "guisan/widget.hpp"

#include <string>

namespace gcn
{
    /**
     * An implementation of a text field where a user can enter a line of text.
     */
    class GCN_CORE_DECLSPEC TextField:
        public Widget,
        public MouseListener,
        public KeyListener
    {
    public:
        /**
         * Constructor.
         */
        TextField();

        /**
         * Constructor. The text field will be automatically resized
         * to fit the text.
         *
         * @param text The default text of the text field.
         */
        TextField(const std::string& text);

        /**
         * Sets the text of the text field.
         *
         * @param text The text of the text field.
         * @see getText
         */
        void setText(const std::string& text);

        /**
         * Gets the text of the text field.
         *
         * @return The text of the text field.
         * @see setText
         */
        const std::string& getText() const;

        /**
         * Adjusts the size of the text field to fit the text.
         */
        void adjustSize();

        /**
         * Adjusts the height of the text field to fit caption.
         */
        void adjustHeight();

        /**
         * Checks if the text field is editable.
         *
         * @return True it the text field is editable, false otherwise.
         * @see setEditable
         */
        bool isEditable() const { return mEditable; }

        /**
         * Sets the text field to be editable or not. A text field is editable
         * by default.
         *
         * @param editable True if the text field should be editable, false
         *                 otherwise.
         */
        void setEditable(bool editable) { mEditable = editable; }

        /**
         * Sets the caret position. As there is only one line of text
         * in a text field the position is the caret's x coordinate.
         *
         * @param position The caret position.
         * @see getCaretPosition
         */
        void setCaretPosition(unsigned int position);

        /**
         * Gets the caret position. As there is only one line of text
         * in a text field the position is the caret's x coordinate.
         *
         * @return The caret position.
         * @see setCaretPosition
         */
        unsigned int getCaretPosition() const;


        // Inherited from Widget

        virtual void fontChanged();

        virtual void draw(Graphics* graphics);


        // Inherited from MouseListener

        virtual void mousePressed(MouseEvent& mouseEvent);

        virtual void mouseDragged(MouseEvent& mouseEvent);

        // Inherited from KeyListener

        virtual void keyPressed(KeyEvent& keyEvent);

    protected:
        /**
         * Draws the caret. Overloaded this method if you want to
         * change the style of the caret.
         *
         * @param graphics the Graphics object to draw with.
         * @param x the caret's x-position.
         */
        virtual void drawCaret(Graphics* graphics, int x);

        /**
         * Scrolls the text horizontally so that the caret shows if needed.
         * The method is used any time a user types in the text field so the
         * caret always will be shown.
         */
        void fixScroll();

        /**
         * Holds the text of the text box.
         */
        std::string mText;

        /**
         * Holds the caret position.
         */
        unsigned int mCaretPosition;

        /**
         * Holds the amount scrolled in x. If a user types more characters than
         * the text field can display, due to the text field being to small, the
         * text needs to scroll in order to show the last type character.
         */
        int mXScroll;

        /**
         * True if the text field is editable, false otherwise.
         */
        bool mEditable;
    };
}

#endif // end GCN_TEXTFIELD_HPP
