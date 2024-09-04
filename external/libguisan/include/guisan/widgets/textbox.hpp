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
     * An implementation of a text box where a user can enter text that contains many lines.
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
         * @param text The default text of the text box.
         */
        TextBox(const std::string& text);

        /**
         * Sets the text of the text box.
         *
         * @param text The text of the text box.
         * @see getText
         */
        void setText(const std::string& text);

        /**
         * Gets the text of the text box.
         *
         * @return The text of the text box.
         * @see setText
         */
        std::string getText() const;

        /**
         * Gets a certain row from the text.
         *
         * @param row The number of the row to get from the text.
         * @return A row from the text of the text box.
         * @see setTextRow
         */
        const std::string& getTextRow(int row) const;

        /**
         * Sets the text of a certain row of the text.
         *
         * @param row The number of the row to set in the text.
         * @param text The text to set in the given row number.
         * @see getTextRow
         */
        void setTextRow(int row, const std::string& text);

        /**
         * Gets the number of rows in the text.
         *
         * @return The number of rows in the text.
         */
        unsigned int getNumberOfRows() const;

        /**
         * Gets the caret position in the text.
         *
         * @return The caret position in the text.
         * @see setCaretPosition
         */
        unsigned int getCaretPosition() const;

        /**
         * Sets the position of the caret in the text.
         *
         * @param position the positon of the caret.
         * @see getCaretPosition
         */
        void setCaretPosition(unsigned int position);

        /**
         * Gets the row number where the caret is currently located.
         *
         * @return The row number where the caret is currently located.
         * @see setCaretRow
         */
        unsigned int getCaretRow() const;

        /**
         * Sets the row where the caret should be currently located.
         *
         * @param The row where the caret should be currently located.
         * @see getCaretRow
         */
        void setCaretRow(int row);

        /**
         * Gets the column where the caret is currently located.
         *
         * @return The column where the caret is currently located.
         * @see setCaretColumn
         */
        unsigned int getCaretColumn() const;

        /**
         * Sets the column where the caret should be currently located.
         *
         * @param The column where the caret should be currently located.
         * @see getCaretColumn
         */
        void setCaretColumn(int column);

        /**
         * Sets the row and the column where the caret should be currently
         * located.
         *
         * @param row The row where the caret should be currently located.
         * @param column The column where the caret should be currently located.
         * @see getCaretRow, getCaretColumn
         */
        void setCaretRowColumn(int row, int column);

        /**
         * Scrolls the text to the caret if the text box is in a scroll area.
         * 
         * @see ScrollArea
         */
        virtual void scrollToCaret();

        /**
         * Checks if the text box is editable.
         *
         * @return True it the text box is editable, false otherwise.
         * @see setEditable
         */
        bool isEditable() const;

        /**
         * Sets the text box to be editable or not.
         *
         * @param editable True if the text box should be editable, false otherwise.
         */
        void setEditable(bool editable);

        /**
         * Adds a row of text to the end of the text.
         *
         * @param row The row to add.
         */
        virtual void addRow(const std::string& row);

        /**
         * Checks if the text box is opaque. An opaque text box will draw
         * its background and its text. A non opaque text box only draw its
         * text making it transparent.
         *
         * @return True if the text box is opaque, false otherwise.
         * @see setOpaque
         */
        bool isOpaque();

        /**
         * Sets the text box to be opaque or not. An opaque text box will draw
         * its background and its text. A non opaque text box only draw its
         * text making it transparent.
         *
         * @param opaque True if the text box should be opaque, false otherwise.
         * @see isOpaque
         */
        void setOpaque(bool opaque);


        // Inherited from Widget

        virtual void draw(Graphics* graphics);

        virtual void fontChanged();


        // Inherited from KeyListener

        virtual void keyPressed(KeyEvent& keyEvent);


        // Inherited from MouseListener

        virtual void mousePressed(MouseEvent& mouseEvent);

        virtual void mouseDragged(MouseEvent& mouseEvent);

    protected:
        /**
         * Draws the caret. Overloaded this method if you want to
         * change the style of the caret.
         *
         * @param graphics a Graphics object to draw with.
         * @param x the x position.
         * @param y the y position.
         */
        virtual void drawCaret(Graphics* graphics, int x, int y);

        /**
         * Adjusts the text box's size to fit the text.
         */
        virtual void adjustSize();

        /**
         * Holds all the rows of the text.
         */
        std::vector<std::string> mTextRows;

        /**
         * Holds the current column of the caret.
         */
        int mCaretColumn;

        /**
         * Holds the current row of the caret.
         */
        int mCaretRow;

        /**
         * True if the text box is editable, false otherwise.
         */
        bool mEditable;

        /**
         * True if the text box is editable, false otherwise.
         */
        bool mOpaque;
    };
}

#endif // end GCN_TEXTBOX_HPP
