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

#include "guisan/widgets/tab.hpp"

#include "guisan/font.hpp"
#include "guisan/graphics.hpp"
#include "guisan/widgets/button.hpp"
#include "guisan/widgets/label.hpp"
#include "guisan/widgets/tabbedarea.hpp"

namespace gcn
{
    Tab::Tab() : mLabel(std::make_unique<Label>())
    {
        mLabel->setPosition(6, 6);
        add(mLabel.get());
        setFrameSize(1);

        addMouseListener(this);
    }

    Tab::~Tab() = default;

    void Tab::adjustSize()
    {
        setSize(mLabel->getWidth() + 12, mLabel->getHeight() + 12);

        if (mTabbedArea != nullptr)
        {
            mTabbedArea->adjustTabPositions();
        }
    }

    void Tab::setTabbedArea(TabbedArea* tabbedArea)
    {
        mTabbedArea = tabbedArea;
    }

    TabbedArea* Tab::getTabbedArea()
    {
        return mTabbedArea;
    }

    void Tab::setCaption(const std::string& caption)
    {
        mLabel->setCaption(caption);
        mLabel->adjustSize();
        adjustSize();
    }

    const std::string& Tab::getCaption() const
    {
        return mLabel->getCaption();
    }

    void Tab::draw(Graphics *graphics)
    {
        const Color& faceColor = getBaseColor();
        const int alpha = getBaseColor().a;
        Color highlightColor = faceColor + 0x303030;
        highlightColor.a = alpha;
        Color shadowColor = faceColor - 0x303030;
        shadowColor.a = alpha;

        Color borderColor;
        Color baseColor;

        if ((mTabbedArea != nullptr && mTabbedArea->isTabSelected(this)) || mHasMouse)
        {
            // Draw a border.
            graphics->setColor(highlightColor);
            graphics->drawLine(0, 0, getWidth() - 1, 0);
            graphics->drawLine(0, 1, 0, getHeight() - 1);
            graphics->setColor(shadowColor);
            graphics->drawLine(getWidth() - 1, 1, getWidth() - 1, getHeight() - 1);

            borderColor = highlightColor;
            baseColor = getBaseColor();
        }
        else
        {
            // Draw a border.
            graphics->setColor(shadowColor);
            graphics->drawLine(0, 0, getWidth() - 1, 0);
            graphics->drawLine(0, 1, 0, getHeight() - 1);
            graphics->drawLine(getWidth() - 1, 1, getWidth() - 1, getHeight() - 1);

            baseColor = getBaseColor() - 0x151515;
            baseColor.a = alpha;
        }

        // Push a clip area so the other drawings don't need to worry
        // about the border.
        graphics->pushClipArea(Rectangle(1, 1, getWidth() - 2, getHeight() - 1));
        const Rectangle currentClipArea = graphics->getCurrentClipArea();

        graphics->setColor(baseColor);
        graphics->fillRectangle(Rectangle(0, 0, currentClipArea.width, currentClipArea.height));

        if (mTabbedArea != nullptr && mTabbedArea->isFocused() && mTabbedArea->isTabSelected(this)
            //            && mHasMouse)
            && mTabbedArea->isTabActive())
        {
            graphics->setColor(Color(0x000000));
            graphics->drawRectangle(
                Rectangle(2, 2, currentClipArea.width - 4, currentClipArea.height - 4));
        }
        mLabel->setAlignment(Graphics::Center);
        mLabel->_draw(graphics);
        graphics->popClipArea();
    }

    void Tab::mouseEntered(MouseEvent& mouseEvent)
    {
        mHasMouse = true;
    }

    void Tab::mouseExited(MouseEvent& mouseEvent)
    {
        mHasMouse = false;
    }
}
