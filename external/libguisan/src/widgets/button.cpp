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

#include "guisan/widgets/button.hpp"

#include "guisan/exception.hpp"
#include "guisan/font.hpp"
#include "guisan/graphics.hpp"
#include "guisan/key.hpp"
#include "guisan/mouseevent.hpp"
#include "guisan/mouseinput.hpp"

namespace gcn
{
    Button::Button() : Button("")
    {}

    Button::Button(std::string caption) : mCaption(std::move(caption))
    {
        setFocusable(true);
        adjustSize();
        setFrameSize(1);

        addMouseListener(this);
        addKeyListener(this);
        addFocusListener(this);
    }

    void Button::setCaption(const std::string& caption)
    {
        mCaption = caption;
    }

    const std::string& Button::getCaption() const
    {
        return mCaption;
    }

    void Button::setAlignment(Graphics::Alignment alignment)
    {
        mAlignment = alignment;
    }

    Graphics::Alignment Button::getAlignment() const
    {
        return mAlignment;
    }

    void Button::setSpacing(unsigned int spacing)
    {
        mSpacing = spacing;
    }

    unsigned int Button::getSpacing() const
    {
      return mSpacing;
    }

    void Button::draw(Graphics* graphics)
    {
        Color faceColor = getBaseColor();
        Color highlightColor, shadowColor;
        int alpha = getBaseColor().a;

        if (isPressed())
        {
            faceColor = faceColor - 0x303030;
            faceColor.a = alpha;
            highlightColor = faceColor - 0x303030;
            highlightColor.a = alpha;
            shadowColor = faceColor + 0x303030;
            shadowColor.a = alpha;
        }
        else
        {
            highlightColor = faceColor + 0x303030;
            highlightColor.a = alpha;
            shadowColor = faceColor - 0x303030;
            shadowColor.a = alpha;
        }

        graphics->setColor(faceColor);
        graphics->fillRectangle(Rectangle(1, 1, getDimension().width-1, getHeight() - 1));

        graphics->setColor(highlightColor);
        graphics->drawLine(0, 0, getWidth() - 1, 0);
        graphics->drawLine(0, 1, 0, getHeight() - 1);

        graphics->setColor(shadowColor);
        graphics->drawLine(getWidth() - 1, 1, getWidth() - 1, getHeight() - 1);
        graphics->drawLine(1, getHeight() - 1, getWidth() - 1, getHeight() - 1);

        if (isEnabled())
            graphics->setColor(getForegroundColor());
        else
            graphics->setColor(Color(128, 128, 128));

        int textX;
        int textY = getHeight() / 2 - getFont()->getHeight() / 2;

        switch (getAlignment())
        {
          case Graphics::Left:
              textX = mSpacing;
              break;
          case Graphics::Center:
              textX = getWidth() / 2;
              break;
          case Graphics::Right:
              textX = getWidth() - mSpacing;
              break;
          default:
              throw GCN_EXCEPTION("Unknown alignment.");
        }

        graphics->setFont(getFont());

        if (isPressed())
        {
            graphics->drawText(getCaption(), textX + 1, textY + 1, getAlignment(), isEnabled());
        }
        else
        {
            graphics->drawText(getCaption(), textX, textY, getAlignment(), isEnabled());

            if (isFocused())
            {
                graphics->drawRectangle(Rectangle(2, 2, getWidth() - 4,
                                                  getHeight() - 4));
            }
        }
    }

    void Button::adjustSize()
    {
        setWidth(getFont()->getWidth(mCaption) + 2 * mSpacing);
        setHeight(getFont()->getHeight() + 2 * mSpacing);
    }

    bool Button::isPressed() const
    {
        if (mMousePressed)
        {
            return mHasMouse;
        }
        return mKeyPressed || mHotKeyPressed;
    }

    void Button::mousePressed(MouseEvent& mouseEvent)
    {
        mHasMouse = gcn::Rectangle(0, 0, getWidth(), getHeight())
                        .isContaining(mouseEvent.getX(), mouseEvent.getY());
        if (mouseEvent.isConsumed())
        {
            return;
        }
        if (mouseEvent.getButton() == MouseEvent::Left)
        {
            mMousePressed = true;
            mouseEvent.consume();
        }
    }

    void Button::mouseExited(MouseEvent& mouseEvent)
    {
        mHasMouse = false;
    }

    void Button::mouseEntered(MouseEvent& mouseEvent)
    {
        mHasMouse = true;
    }

    void Button::mouseReleased(MouseEvent& mouseEvent)
    {
        if (mouseEvent.getButton() == MouseEvent::Left)
        {
            mMousePressed = false;
            mouseEvent.consume();
        }
    }

    void Button::mouseClicked(MouseEvent& mouseEvent)
    {
        if (mouseEvent.getButton() == MouseEvent::Left)
        {
            distributeActionEvent();
            mouseEvent.consume();
        }
    }

    void Button::mouseDragged(MouseEvent& mouseEvent)
    {
        mouseEvent.consume();
    }

    void Button::keyPressed(KeyEvent& keyEvent)
    {
        const Key key = keyEvent.getKey();

        if (key.getValue() == Key::Enter
            || key.getValue() == Key::Space)
        {
            mKeyPressed = true;
            keyEvent.consume();
        }
        mHotKeyPressed = false;
    }

    void Button::keyReleased(KeyEvent& keyEvent)
    {
        const Key key = keyEvent.getKey();

        if ((key.getValue() == Key::Enter
             || key.getValue() == Key::Space)
            && mKeyPressed)
        {
            mKeyPressed = false;
            distributeActionEvent();
            keyEvent.consume();
        }
    }

    void Button::hotKeyPressed()
    {
        mHotKeyPressed = true;
        mMousePressed = false;
    }

    void Button::hotKeyReleased()
    {
        if (mHotKeyPressed)
        {
            mHotKeyPressed = false;
            distributeActionEvent();
        }
    }

    void Button::focusLost(const Event& event)
    {
        mMousePressed = false;
        mKeyPressed = false;
    }
}
