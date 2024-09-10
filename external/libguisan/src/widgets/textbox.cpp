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

#include "guisan/widgets/textbox.hpp"

#include "guisan/basiccontainer.hpp"
#include "guisan/font.hpp"
#include "guisan/graphics.hpp"
#include "guisan/key.hpp"
#include "guisan/mouseinput.hpp"
#include "guisan/text.hpp"

namespace gcn
{
    TextBox::TextBox() : mEditable(true), mOpaque(true)
    {
        mText = new Text();

        setFocusable(true);

        addMouseListener(this);
        addKeyListener(this);
        adjustSize();
        setFrameSize(1);
    }

    TextBox::TextBox(const std::string& text) : mEditable(true), mOpaque(true)
    {
        mText = new Text(text);

        setFocusable(true);

        addMouseListener(this);
        addKeyListener(this);
        adjustSize();
        setFrameSize(1);
    }

    void TextBox::setText(const std::string& text)
    {
        mText->setContent(text);

        adjustSize();
    }

    void TextBox::draw(Graphics* graphics)
    {
        if (mOpaque)
        {
            graphics->setColor(getBackgroundColor());
            graphics->fillRectangle(Rectangle(0, 0, getWidth(), getHeight()));
        }

        if (isFocused() && isEditable())
        {
            drawCaret(graphics, mText->getCaretX(getFont()), mText->getCaretY(getFont()));
        }

        graphics->setColor(getForegroundColor());
        graphics->setFont(getFont());

        for (unsigned i = 0; i < mText->getNumberOfRows(); i++)
        {
            // Move the text one pixel so we can have a caret before a letter.
            graphics->drawText(mText->getRow(i), 1, i * getFont()->getHeight());
        }
    }

    void TextBox::drawCaret(Graphics* graphics, int x, int y)
    {
        graphics->setColor(getForegroundColor());
        graphics->drawLine(x, y, x, y + getFont()->getHeight());
    }

    void TextBox::mousePressed(MouseEvent& mouseEvent)
    {
        if (mouseEvent.getButton() == MouseEvent::Left)
        {
            mText->setCaretPosition(mouseEvent.getX(), mouseEvent.getY(), getFont());
            mouseEvent.consume();
        }
    }

    void TextBox::mouseDragged(MouseEvent& mouseEvent)
    {
        mouseEvent.consume();
    }

    void TextBox::keyPressed(KeyEvent& keyEvent)
    {
        const Key key = keyEvent.getKey();

        if (key.getValue() == Key::Left)
        {
            mText->setCaretPosition(mText->getCaretPosition() - 1);
        }

        else if (key.getValue() == Key::Right)
        {
            mText->setCaretPosition(mText->getCaretPosition() + 1);
        }

        else if (key.getValue() == Key::Down)
        {
            mText->setCaretRow(mText->getCaretRow() + 1);
        }

        else if (key.getValue() == Key::Up)
        {
            mText->setCaretRow(mText->getCaretRow() - 1);
        }

        else if (key.getValue() == Key::Home)
        {
            mText->setCaretColumn(0);
        }

        else if (key.getValue() == Key::End)
        {
            mText->setCaretColumn(mText->getNumberOfCharacters(mText->getCaretRow()));
        }

        else if (key.getValue() == Key::Enter && mEditable)
        {
            mText->insert('\n');
        }

        else if (key.getValue() == Key::Backspace && mEditable)
        {
            mText->remove(-1);
        }

        else if (key.getValue() == Key::Delete && mEditable)
        {
            mText->remove(1);
        }

        else if (key.getValue() == Key::PageUp)
        {
            Widget* par = getParent();

            if (par != NULL)
            {
                int rowsPerPage = par->getChildrenArea().height / getFont()->getHeight();
                mText->setCaretRow(mText->getCaretRow() - rowsPerPage);
            }
        }

        else if (key.getValue() == Key::PageDown)
        {
            Widget* par = getParent();

            if (par != NULL)
            {
                int rowsPerPage = par->getChildrenArea().height / getFont()->getHeight();
                mText->setCaretRow(mText->getCaretRow() + rowsPerPage);
            }
        }

        else if (key.getValue() == Key::Tab && mEditable)
        {
            mText->insert(' ');
            mText->insert(' ');
            mText->insert(' ');
            mText->insert(' ');
        }

        else if (key.isCharacter() && mEditable && !keyEvent.isAltPressed()
                 && !keyEvent.isControlPressed())
        {
            if (keyEvent.isShiftPressed() && key.isLetter())
            {
                mText->insert(key.getValue() - 32);
            }
            else
            {
                mText->insert(key.getValue());
            }
        }

        adjustSize();
        scrollToCaret();

        keyEvent.consume();
    }

    void TextBox::adjustSize()
    {
        const Rectangle& dim = mText->getDimension(getFont());
        setSize(dim.width, dim.height);
    }

    void TextBox::setCaretPosition(unsigned int position)
    {
        mText->setCaretPosition(position);
    }

    unsigned int TextBox::getCaretPosition() const
    {
        return mText->getCaretPosition();
    }

    void TextBox::setCaretRowColumn(int row, int column)
    {
        mText->setCaretRow(row);
        mText->setCaretColumn(column);
    }

    void TextBox::setCaretRow(int row)
    {
        mText->setCaretRow(row);
    }

    unsigned int TextBox::getCaretRow() const
    {
        return mText->getCaretRow();
    }

    void TextBox::setCaretColumn(int column)
    {
        mText->setCaretColumn(column);
    }

    unsigned int TextBox::getCaretColumn() const
    {
        return mText->getCaretColumn();
    }

    std::string TextBox::getTextRow(int row) const
    {
        return mText->getRow(row);
    }

    void TextBox::setTextRow(int row, const std::string& text)
    {
        mText->setRow(row, text);

        adjustSize();
    }

    unsigned int TextBox::getNumberOfRows() const
    {
        return mText->getNumberOfRows();
    }

    std::string TextBox::getText() const
    {
        return mText->getContent();
    }

    void TextBox::fontChanged()
    {
        adjustSize();
    }

    void TextBox::scrollToCaret()
    {
        showPart(mText->getCaretDimension(getFont()));
    }

    void TextBox::setEditable(bool editable)
    {
        mEditable = editable;
    }

    bool TextBox::isEditable() const
    {
        return mEditable;
    }

    void TextBox::addRow(const std::string& row)
    {
        mText->addRow(row);
        adjustSize();
    }

    bool TextBox::isOpaque()
    {
        return mOpaque;
    }

    void TextBox::setOpaque(bool opaque)
    {
        mOpaque = opaque;
    }
}
