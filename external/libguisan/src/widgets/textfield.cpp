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

#include "guisan/widgets/textfield.hpp"

#include "guisan/font.hpp"
#include "guisan/graphics.hpp"
#include "guisan/key.hpp"
#include "guisan/mouseinput.hpp"

namespace gcn
{
    TextField::TextField() : mEditable(true)
    {
        mCaretPosition = 0;
        mXScroll = 0;

        setFocusable(true);

        addMouseListener(this);
        addKeyListener(this);
        adjustHeight();
    }

    TextField::TextField(const std::string& text) : mEditable(true)
    {
        mCaretPosition = 0;
        mXScroll = 0;

        mText = text;
        adjustSize();

        setFocusable(true);

        addMouseListener(this);
        addKeyListener(this);
    }

    void TextField::setText(const std::string& text)
    {
        if (text.size() < mCaretPosition)
        {
            mCaretPosition = text.size();
        }

        mText = text;
    }

    void TextField::draw(Graphics* graphics)
    {
        Color faceColor = getBaseColor();
        Color highlightColor, shadowColor;
        int alpha = getBaseColor().a;
        highlightColor = faceColor + 0x303030;
        highlightColor.a = alpha;
        shadowColor = faceColor - 0x303030;
        shadowColor.a = alpha;

        // Draw a border.
        graphics->setColor(shadowColor);
        graphics->drawLine(0, 0, getWidth() - 1, 0);
        graphics->drawLine(0, 1, 0, getHeight() - 2);
        graphics->setColor(highlightColor);
        graphics->drawLine(getWidth() - 1, 1, getWidth() - 1, getHeight() - 1);
        graphics->drawLine(0, getHeight() - 1, getWidth() - 1, getHeight() - 1);

        // Push a clip area so the other drawings don't need to worry
        // about the border.
        graphics->pushClipArea(Rectangle(1, 1, getWidth() - 2, getHeight() - 2));

        graphics->setColor(getBackgroundColor());
        graphics->fillRectangle(Rectangle(0, 0, getWidth(), getHeight()));

        if (isFocused())
        {
            graphics->setColor(getSelectionColor());
            graphics->drawRectangle(Rectangle(0, 0, getWidth() - 2, getHeight() - 2));
            graphics->drawRectangle(Rectangle(1, 1, getWidth() - 4, getHeight() - 4));
        }

        if (isFocused() && isEditable())
        {
            drawCaret(graphics, getFont()->getWidth(mText.substr(0, mCaretPosition)) - mXScroll);
        }

        if (isEnabled())
            graphics->setColor(getForegroundColor());
        else
            graphics->setColor(Color(128, 128, 128));

        graphics->setFont(getFont());
        graphics->drawText(mText, 1 - mXScroll, 1);
        graphics->popClipArea();
    }

    void TextField::drawCaret(Graphics* graphics, int x)
    {
        // Check the current clip area as a clip area with a different
        // size than the widget might have been pushed (which is the
        // case in the draw method when we push a clip area after we have
        // drawn a border).
        const Rectangle clipArea = graphics->getCurrentClipArea();

        graphics->setColor(getForegroundColor());
        graphics->drawLine(x, clipArea.height - 2, x, 1);
    }

    void TextField::mousePressed(MouseEvent& mouseEvent)
    {
        if (mouseEvent.getButton() == MouseEvent::Left)
        {
            mCaretPosition = getFont()->getStringIndexAt(mText, mouseEvent.getX() + mXScroll);
            fixScroll();
        }
    }

    void TextField::mouseDragged(MouseEvent& mouseEvent)
    {
        mouseEvent.consume();
    }

    void TextField::keyPressed(KeyEvent& keyEvent)
    {
        const Key key = keyEvent.getKey();

        if (key.getValue() == Key::Left && mCaretPosition > 0)
        {
            --mCaretPosition;
        }

        else if (key.getValue() == Key::Right && mCaretPosition < mText.size())
        {
            ++mCaretPosition;
        }

        else if (key.getValue() == Key::Delete && mCaretPosition < mText.size() && mEditable)
        {
            mText.erase(mCaretPosition, 1);
        }

        else if (key.getValue() == Key::Backspace && mCaretPosition > 0 && mEditable)
        {
            mText.erase(mCaretPosition - 1, 1);
            --mCaretPosition;
        }

        else if (key.getValue() == Key::Enter)
        {
            distributeActionEvent();
        }

        else if (key.getValue() == Key::Home)
        {
            mCaretPosition = 0;
        }

        else if (key.getValue() == Key::End)
        {
            mCaretPosition = mText.size();
        }

        else if (key.isCharacter() && key.getValue() != Key::Tab && !keyEvent.isAltPressed()
                 && !keyEvent.isControlPressed() && mEditable)
        {
            if (keyEvent.isShiftPressed() && key.isLetter())
            {
                mText.insert(mCaretPosition, std::string(1, static_cast<char>(key.getValue() - 32)));
            }
            else
            {
                mText.insert(mCaretPosition, std::string(1, static_cast<char>(key.getValue())));
            }
            ++mCaretPosition;
        }

        if (key.getValue() != Key::Tab)
        {
            keyEvent.consume();
        }

        fixScroll();
    }

    void TextField::adjustSize()
    {
        setWidth(getFont()->getWidth(mText) + 7);
        adjustHeight();

        fixScroll();
    }

    void TextField::adjustHeight()
    {
        setHeight(getFont()->getHeight() + 2);
    }

    void TextField::fixScroll()
    {
        if (isFocused())
        {
            const int caretX = getFont()->getWidth(mText.substr(0, mCaretPosition));

            if (caretX - mXScroll > getWidth() - 4)
            {
                mXScroll = caretX - getWidth() + 4;
            }
            else if (caretX - mXScroll < getFont()->getWidth(" "))
            {
                mXScroll = caretX - getFont()->getWidth(" ");

                if (mXScroll < 0)
                {
                    mXScroll = 0;
                }
            }
        }
    }

    void TextField::setCaretPosition(unsigned int position)
    {
        if (position > mText.size())
        {
            mCaretPosition = mText.size();
        }
        else
        {
            mCaretPosition = position;
        }

        fixScroll();
    }

    unsigned int TextField::getCaretPosition() const
    {
        return mCaretPosition;
    }

    const std::string& TextField::getText() const
    {
        return mText;
    }

    void TextField::fontChanged()
    {
        fixScroll();
    }
}
