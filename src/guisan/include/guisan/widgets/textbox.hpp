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

#ifndef GCN_TEXTBOX_HPP
#define GCN_TEXTBOX_HPP

#include <ctime>
#include <string>
#include <vector>

#include "guisan/keylistener.hpp"
#include "guisan/mouselistener.hpp"
#include "guisan/platform.hpp"
#include "guisan/widget.hpp"

namespace gcn
{
    /**
     * A TextBox in which you can write and/or display a lines of text.
     *
     * NOTE: A plain TextBox is really uggly and looks much better inside a
     *       ScrollArea.
     */
    class GCN_CORE_DECLSPEC TextBox:
        public Widget,
        public MouseListener,
        public KeyListener
    {
    public:
        /**
         * Constructor.
         */
        TextBox();

        /**
         * Constructor.
         *
         * @param text the text of the TextBox.
         */
        TextBox(const std::string& text);

        /**
         * Sets the text.
         *
         * @param text the text of the TextBox.
         */
        void setText(const std::string& text);

        /**
         * Gets the text.
         * @return the text of the TextBox.
         */
        std::string getText() const;

        /**
         * Gets the row of a text.
         *
         * @return the text of a certain row in the TextBox.
         */
        const std::string& getTextRow(int row) const;

        /**
         * Sets the text of a certain row in a TextBox.
         *
         * @param row the row number.
         * @param text the text of a certain row in the TextBox.
         */
        void setTextRow(int row, const std::string& text);

        /**
         * Gets the number of rows in the text.
         *
         * @return the number of rows in the text.
         */
        unsigned int getNumberOfRows() const;

        /**
         * Gets the caret position in the text.
         *
         * @return the caret position in the text.
         */
        unsigned int getCaretPosition() const;

        /**
         * Sets the position of the caret in the text.
         *
         * @param position the positon of the caret.
         */
        void setCaretPosition(unsigned int position);

        /**
         * Gets the row the caret is in in the text.
         *
         * @return the row the caret is in in the text.
         */
        unsigned int getCaretRow() const;

        /**
         * Sets the row the caret should be in in the text.
         *
         * @param row the row number.
         */
        void setCaretRow(int row);

        /**
         * Gets the column the caret is in in the text.
         *
         * @return the column the caret is in in the text.
         */
        unsigned int getCaretColumn() const;

        /**
         * Sets the column the caret should be in in the text.
         *
         * @param column the column number.
         */
        void setCaretColumn(int column);

        /**
         * Sets the row and the column the caret should be in in the text.
         *
         * @param row the row number.
         * @param column the column number.
         */
        void setCaretRowColumn(int row, int column);

        /**
         * Scrolls the text to the caret if the TextBox is in a ScrollArea.
         */
        virtual void scrollToCaret();

        /**
         * Checks if the TextBox is editable.
         *
         * @return true it the TextBox is editable.
         */
        bool isEditable() const;

        /**
         * Sets if the TextBox should be editable or not.
         *
         * @param editable true if the TextBox should be editable.
         */
        void setEditable(bool editable);

        /**
         * Adds a text row to the text.
         *
         * @param row a row.
         */
        virtual void addRow(const std::string row);

        /**
         * Checks if the TextBox is opaque
         *
         * @return true if the TextBox is opaque
         */
        bool isOpaque();

        /**
         * Sets the TextBox to be opaque.
         *
         * @param opaque true if the TextBox should be opaque.
         */
        void setOpaque(bool opaque);


        // Inherited from Widget

        virtual void draw(Graphics* graphics);

        virtual void drawBorder(Graphics* graphics);

        virtual void fontChanged();


        // Inherited from KeyListener

        virtual void keyPressed(KeyEvent& keyEvent);


        // Inherited from MouseListener

        virtual void mousePressed(MouseEvent& mouseEvent);

        virtual void mouseDragged(MouseEvent& mouseEvent);

    protected:
        /**
         * Draws the caret.
         *
         * @param graphics a Graphics object to draw with.
         * @param x the x position.
         * @param y the y position.
         */
        virtual void drawCaret(Graphics* graphics, int x, int y);

        /**
         * Adjusts the TextBox size to fit the font size.
         */
        virtual void adjustSize();

        std::vector<std::string> mTextRows;
        int mCaretColumn;
        int mCaretRow;
        bool mEditable;
        bool mOpaque;
    };
}

#endif // end GCN_TEXTBOX_HPP
