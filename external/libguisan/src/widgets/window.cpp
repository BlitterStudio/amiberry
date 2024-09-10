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

#include "guisan/widgets/window.hpp"

#include "guisan/exception.hpp"
#include "guisan/font.hpp"
#include "guisan/graphics.hpp"
#include "guisan/mouseinput.hpp"

namespace gcn
{
    Window::Window()
        :mMoved(false)
    {
        setFrameSize(1);
        setPadding(2);
        setTitleBarHeight(getFont()->getHeight() + 2);
        setAlignment(Graphics::Center);
        addMouseListener(this);
        setMovable(true);
        setOpaque(true);
    }

    Window::Window(const std::string& caption)
        :mMoved(false)
    {
        setCaption(caption);
        setFrameSize(1);
        setPadding(2);
        setTitleBarHeight(getFont()->getHeight() + 2);
        setAlignment(Graphics::Center);
        addMouseListener(this);
        setMovable(true);
        setOpaque(true);
    }

    Window::~Window()
    = default;

    void Window::setPadding(unsigned int padding)
    {
        mPadding = padding;
    }

    unsigned int Window::getPadding() const
    {
        return mPadding;
    }

    void Window::setTitleBarHeight(unsigned int height)
    {
        mTitleBarHeight = height;
    }

    unsigned int Window::getTitleBarHeight()
    {
        return mTitleBarHeight;
    }

    void Window::setCaption(const std::string& caption)
    {
        mCaption = caption;
    }

    const std::string& Window::getCaption() const
    {
        return mCaption;
    }

    void Window::setAlignment(Graphics::Alignment alignment)
    {
        mAlignment = alignment;
    }

    Graphics::Alignment Window::getAlignment() const
    {
        return mAlignment;
    }

    void Window::draw(Graphics* graphics)
    {
        const Color& faceColor = getBaseColor();
        Color highlightColor, shadowColor;
        const int alpha = getBaseColor().a;
        highlightColor = faceColor + 0x303030;
        highlightColor.a = alpha;
        shadowColor = faceColor - 0x303030;
        shadowColor.a = alpha;

        Rectangle d = getChildrenArea();

        // Fill the background around the content
        graphics->setColor(faceColor);
        // Fill top
        graphics->fillRectangle(Rectangle(0, 0, getWidth(), d.y - 1));
        // Fill left
        graphics->fillRectangle(Rectangle(0, d.y - 1, d.x - 1, getHeight() - d.y + 1));
        // Fill right
        graphics->fillRectangle(Rectangle(d.x + d.width + 1,
            d.y - 1,
            getWidth() - d.x - d.width - 1,
            getHeight() - d.y + 1));
        // Fill bottom
        graphics->fillRectangle(Rectangle(d.x - 1,
            d.y + d.height + 1,
            d.width + 2,
            getHeight() - d.height - d.y - 1));

        if (isOpaque())
        {
            graphics->fillRectangle(d);
        }

        // Construct a rectangle one pixel bigger than the content
        d.x -= 1;
        d.y -= 1;
        d.width += 2;
        d.height += 2;

        // Draw a border around the content
        graphics->setColor(shadowColor);
        // Top line
        graphics->drawLine(d.x,
            d.y,
            d.x + d.width - 2,
            d.y);

        // Left line
        graphics->drawLine(d.x,
            d.y + 1,
            d.x,
            d.y + d.height - 1);

        graphics->setColor(highlightColor);
        // Right line
        graphics->drawLine(d.x + d.width - 1,
            d.y,
            d.x + d.width - 1,
            d.y + d.height - 2);
        // Bottom line
        graphics->drawLine(d.x + 1,
            d.y + d.height - 1,
            d.x + d.width - 1,
            d.y + d.height - 1);

        drawChildren(graphics);

        int textX;

        const int textY = (static_cast<int>(getTitleBarHeight()) - getFont()->getHeight()) / 2;

        switch (getAlignment())
        {
        case Graphics::Left:
            textX = 4;
            break;
        case Graphics::Center:
            textX = getWidth() / 2;
            break;
        case Graphics::Right:
            textX = getWidth() - 4;
            break;
        default:
            throw GCN_EXCEPTION("Unknown alignment.");
        }

        graphics->setColor(getForegroundColor());
        graphics->setFont(getFont());
        graphics->pushClipArea(Rectangle(0, 0, getWidth(), static_cast<int>(getTitleBarHeight() - 1)));
        graphics->drawText(getCaption(), textX, textY, getAlignment(), isEnabled());
        graphics->popClipArea();
    }

    void Window::drawFrame(Graphics* graphics)
    {
        Color faceColor = getBaseColor();
        Color highlightColor, shadowColor;
        int alpha = getBaseColor().a;
        int width = getWidth() + getFrameSize() * 2 - 1;
        int height = getHeight() + getFrameSize() * 2 - 1;
        highlightColor = faceColor + 0x303030;
        highlightColor.a = alpha;
        shadowColor = faceColor - 0x303030;
        shadowColor.a = alpha;

        unsigned int i;
        for (i = 0; i < getFrameSize(); ++i)
        {
            graphics->setColor(highlightColor);
            graphics->drawLine(i, i, width - i, i);
            graphics->drawLine(i, i + 1, i, height - i - 1);
            graphics->setColor(shadowColor);
            graphics->drawLine(width - i, i + 1, width - i, height - i);
            graphics->drawLine(i, height - i, width - i - 1, height - i);
        }
    }

    void Window::mousePressed(MouseEvent& mouseEvent)
    {
        if (mouseEvent.getSource() != this)
        {
            return;
        }

        if (getParent() != NULL)
        {
            getParent()->moveToTop(this);
        }

        mDragOffsetX = mouseEvent.getX();
        mDragOffsetY = mouseEvent.getY();

        mMoved = mouseEvent.getY() <= (int)mTitleBarHeight;
    }

    void Window::mouseReleased(MouseEvent& mouseEvent)
    {
        mMoved = false;
    }

    void Window::mouseDragged(MouseEvent& mouseEvent)
    {
        if (mouseEvent.isConsumed() || mouseEvent.getSource() != this)
        {
            return;
        }

        if (isMovable() && mMoved)
        {
            setPosition(mouseEvent.getX() - mDragOffsetX + getX(),
                mouseEvent.getY() - mDragOffsetY + getY());
        }

        mouseEvent.consume();
    }

    Rectangle Window::getChildrenArea()
    {
        return Rectangle(static_cast<int>(getPadding()),
            static_cast<int>(getTitleBarHeight()),
            static_cast<int>(getWidth() - getPadding() * 2),
            static_cast<int>(getHeight() - getPadding() - getTitleBarHeight()));
    }

    void Window::setMovable(bool movable)
    {
        mMovable = movable;
    }

    bool Window::isMovable() const
    {
        return mMovable;
    }

    void Window::setOpaque(bool opaque)
    {
        mOpaque = opaque;
    }

    bool Window::isOpaque()
    {
        return mOpaque;
    }

    void Window::resizeToContent()
    {
        BasicContainer::resizeToContent();
        setSize(getWidth() + 2 * getPadding(), getHeight() + getPadding() + getTitleBarHeight());
    }
}
