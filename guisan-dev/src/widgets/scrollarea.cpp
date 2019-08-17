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

#include "guisan/widgets/scrollarea.hpp"

#include "guisan/exception.hpp"
#include "guisan/graphics.hpp"

namespace gcn
{
    ScrollArea::ScrollArea()
    {
        mVScroll = 0;
        mHScroll = 0;
        mHPolicy = SHOW_AUTO;
        mVPolicy = SHOW_AUTO;
        mScrollbarWidth = 12;
        mUpButtonPressed = false;
        mDownButtonPressed = false;
        mLeftButtonPressed = false;
        mRightButtonPressed = false;
        mUpButtonScrollAmount = 10;
        mDownButtonScrollAmount = 10;
        mLeftButtonScrollAmount = 10;
        mRightButtonScrollAmount = 10;
        mIsVerticalMarkerDragged = false;
        mIsHorizontalMarkerDragged =false;

        addMouseListener(this);
    }

    ScrollArea::ScrollArea(Widget *content)
    {
        mVScroll = 0;
        mHScroll = 0;
        mHPolicy = SHOW_AUTO;
        mVPolicy = SHOW_AUTO;
        mScrollbarWidth = 12;
        mUpButtonPressed = false;
        mDownButtonPressed = false;
        mLeftButtonPressed = false;
        mRightButtonPressed = false;
        mUpButtonScrollAmount = 10;
        mDownButtonScrollAmount = 10;
        mLeftButtonScrollAmount = 10;
        mRightButtonScrollAmount = 10;
        mIsVerticalMarkerDragged = false;
        mIsHorizontalMarkerDragged =false;

        setContent(content);
        addMouseListener(this);
    }

    ScrollArea::ScrollArea(Widget *content, unsigned int hPolicy, unsigned int vPolicy)
    {
        mVScroll = 0;
        mHScroll = 0;
        mHPolicy = hPolicy;
        mVPolicy = vPolicy;
        mScrollbarWidth = 12;
        mUpButtonPressed = false;
        mDownButtonPressed = false;
        mLeftButtonPressed = false;
        mRightButtonPressed = false;
        mUpButtonScrollAmount = 10;
        mDownButtonScrollAmount = 10;
        mLeftButtonScrollAmount = 10;
        mRightButtonScrollAmount = 10;
        mIsVerticalMarkerDragged = false;
        mIsHorizontalMarkerDragged =false;

        setContent(content);
        addMouseListener(this);
    }

    ScrollArea::~ScrollArea()
    {
        setContent(NULL);
    }

    void ScrollArea::setContent(Widget* widget)
    {
        if (widget != NULL)
        {
            clear();
            add(widget);
            widget->setPosition(0,0);
        }
        else
        {
            clear();
        }

        checkPolicies();
    }

    Widget* ScrollArea::getContent()
    {
        if (mWidgets.size() > 0)
        {
            return *mWidgets.begin();
        }

        return NULL;
    }

    void ScrollArea::setHorizontalScrollPolicy(unsigned int hPolicy)
    {
        mHPolicy = hPolicy;
        checkPolicies();
    }

    unsigned int ScrollArea::getHorizontalScrollPolicy() const
    {
        return mHPolicy;
    }

    void ScrollArea::setVerticalScrollPolicy(unsigned int vPolicy)
    {
        mVPolicy = vPolicy;
        checkPolicies();
    }

    unsigned int ScrollArea::getVerticalScrollPolicy() const
    {
        return mVPolicy;
    }

    void ScrollArea::setScrollPolicy(unsigned int hPolicy, unsigned int vPolicy)
    {
        mHPolicy = hPolicy;
        mVPolicy = vPolicy;
        checkPolicies();
    }

    void ScrollArea::setVerticalScrollAmount(int vScroll)
    {
        int max = getVerticalMaxScroll();

        mVScroll = vScroll;

        if (vScroll > max)
        {
            mVScroll = max;
        }

        if (vScroll < 0)
        {
            mVScroll = 0;
        }
    }

    int ScrollArea::getVerticalScrollAmount() const
    {
        return mVScroll;
    }

    void ScrollArea::setHorizontalScrollAmount(int hScroll)
    {
        int max = getHorizontalMaxScroll();

        mHScroll = hScroll;

        if (hScroll > max)
        {
            mHScroll = max;
        }
        else if (hScroll < 0)
        {
            mHScroll = 0;
        }
    }

    int ScrollArea::getHorizontalScrollAmount() const
    {
        return mHScroll;
    }

    void ScrollArea::setScrollAmount(int hScroll, int vScroll)
    {
        setHorizontalScrollAmount(hScroll);
        setVerticalScrollAmount(vScroll);
    }

    int ScrollArea::getHorizontalMaxScroll()
    {
        checkPolicies();

        if (getContent() == NULL)
        {
            return 0;
        }

        int value = getContent()->getWidth() - getChildrenArea().width +
            2 * getContent()->getBorderSize();

        if (value < 0)
        {
            return 0;
        }

        return value;
    }

    int ScrollArea::getVerticalMaxScroll()
    {
        checkPolicies();

        if (getContent() == NULL)
        {
            return 0;
        }

        int value;

        value = getContent()->getHeight() - getChildrenArea().height +
            2 * getContent()->getBorderSize();

        if (value < 0)
        {
            return 0;
        }

        return value;
    }

    void ScrollArea::setScrollbarWidth(int width)
    {
        if (width > 0)
        {
            mScrollbarWidth = width;
        }
        else
        {
            throw GCN_EXCEPTION("Width should be greater then 0.");
        }
    }

    int ScrollArea::getScrollbarWidth() const
    {
        return mScrollbarWidth;
    }

    void ScrollArea::mousePressed(MouseEvent& mouseEvent)
    {
        int x = mouseEvent.getX();
        int y = mouseEvent.getY();

        if (getUpButtonDimension().isPointInRect(x, y))
        {
            setVerticalScrollAmount(getVerticalScrollAmount()
                                    - mUpButtonScrollAmount);
            mUpButtonPressed = true;
        }
        else if (getDownButtonDimension().isPointInRect(x, y))
        {
            setVerticalScrollAmount(getVerticalScrollAmount()
                                    + mDownButtonScrollAmount);
            mDownButtonPressed = true;
        }
        else if (getLeftButtonDimension().isPointInRect(x, y))
        {
            setHorizontalScrollAmount(getHorizontalScrollAmount()
                                      - mLeftButtonScrollAmount);
            mLeftButtonPressed = true;
        }
        else if (getRightButtonDimension().isPointInRect(x, y))
        {
            setHorizontalScrollAmount(getHorizontalScrollAmount()
                                      + mRightButtonScrollAmount);
            mRightButtonPressed = true;
        }
        else if (getVerticalMarkerDimension().isPointInRect(x, y))
        {
            mIsHorizontalMarkerDragged = false;
            mIsVerticalMarkerDragged = true;

            mVerticalMarkerDragOffset = y - getVerticalMarkerDimension().y;
        }
        else if (getVerticalBarDimension().isPointInRect(x,y))
        {
            if (y < getVerticalMarkerDimension().y)
            {
                setVerticalScrollAmount(getVerticalScrollAmount()
                                        - (int)(getChildrenArea().height * 0.95));
            }
            else
            {
                setVerticalScrollAmount(getVerticalScrollAmount()
                                        + (int)(getChildrenArea().height * 0.95));
            }
        }
        else if (getHorizontalMarkerDimension().isPointInRect(x, y))
        {
            mIsHorizontalMarkerDragged = true;
            mIsVerticalMarkerDragged = false;

            mHorizontalMarkerDragOffset = x - getHorizontalMarkerDimension().x;
        }
        else if (getHorizontalBarDimension().isPointInRect(x,y))
        {
            if (x < getHorizontalMarkerDimension().x)
            {
                setHorizontalScrollAmount(getHorizontalScrollAmount()
                                          - (int)(getChildrenArea().width * 0.95));
            }
            else
            {
                setHorizontalScrollAmount(getHorizontalScrollAmount()
                                          + (int)(getChildrenArea().width * 0.95));
            }
        }
    }

    void ScrollArea::mouseReleased(MouseEvent& mouseEvent)
    {
        mUpButtonPressed = false;
        mDownButtonPressed = false;
        mLeftButtonPressed = false;
        mRightButtonPressed = false;
        mIsHorizontalMarkerDragged = false;
        mIsVerticalMarkerDragged = false;

	mouseEvent.consume();
    }

    void ScrollArea::mouseDragged(MouseEvent& mouseEvent)
    {
        if (mIsVerticalMarkerDragged)
        {
            int pos = mouseEvent.getY() - getVerticalBarDimension().y  - mVerticalMarkerDragOffset;
            int length = getVerticalMarkerDimension().height;

            Rectangle barDim = getVerticalBarDimension();

            if ((barDim.height - length) > 0)
            {
                setVerticalScrollAmount((getVerticalMaxScroll() * pos)
                                         / (barDim.height - length));
            }
            else
            {
                setVerticalScrollAmount(0);
            }
        }

        if (mIsHorizontalMarkerDragged)
        {
            int pos = mouseEvent.getX() - getHorizontalBarDimension().x  - mHorizontalMarkerDragOffset;
            int length = getHorizontalMarkerDimension().width;

            Rectangle barDim = getHorizontalBarDimension();

            if ((barDim.width - length) > 0)
            {
                setHorizontalScrollAmount((getHorizontalMaxScroll() * pos)
                                          / (barDim.width - length));
            }
            else
            {
                setHorizontalScrollAmount(0);
            }
        }

        mouseEvent.consume();
    }

    void ScrollArea::draw(Graphics *graphics)
    {
        drawBackground(graphics);

        if (mVBarVisible)
        {
            drawUpButton(graphics);
            drawDownButton(graphics);
            drawVBar(graphics);
            drawVMarker(graphics);
        }

        if (mHBarVisible)
        {
            drawLeftButton(graphics);
            drawRightButton(graphics);
            drawHBar(graphics);
            drawHMarker(graphics);
        }

        if (mHBarVisible && mVBarVisible)
        {
            graphics->setColor(getBaseColor());
            graphics->fillRectangle(Rectangle(getWidth() - mScrollbarWidth,
                                              getHeight() - mScrollbarWidth,
                                              mScrollbarWidth,
                                              mScrollbarWidth));
        }

        drawChildren(graphics);
    }

    void ScrollArea::drawBorder(Graphics* graphics)
    {
        Color faceColor = getBaseColor();
        Color highlightColor, shadowColor;
        int alpha = getBaseColor().a;
        int width = getWidth() + getBorderSize() * 2 - 1;
        int height = getHeight() + getBorderSize() * 2 - 1;
        highlightColor = faceColor + 0x303030;
        highlightColor.a = alpha;
        shadowColor = faceColor - 0x303030;
        shadowColor.a = alpha;

        unsigned int i;
        for (i = 0; i < getBorderSize(); ++i)
        {
            graphics->setColor(shadowColor);
            graphics->drawLine(i,i, width - i, i);
            graphics->drawLine(i,i + 1, i, height - i - 1);
            graphics->setColor(highlightColor);
            graphics->drawLine(width - i,i + 1, width - i, height - i);
            graphics->drawLine(i,height - i, width - i - 1, height - i);
        }
    }

    void ScrollArea::drawHBar(Graphics* graphics)
    {
        Rectangle dim = getHorizontalBarDimension();

        graphics->pushClipArea(dim);

        int alpha = getBaseColor().a;
        Color trackColor = getBaseColor() - 0x101010;
        trackColor.a = alpha;
        Color shadowColor = getBaseColor() - 0x303030;
        shadowColor.a = alpha;

        graphics->setColor(trackColor);
        graphics->fillRectangle(Rectangle(0, 0, dim.width, dim.height));

        graphics->setColor(shadowColor);
        graphics->drawLine(0, 0, dim.width, 0);

        graphics->popClipArea();
    }

    void ScrollArea::drawVBar(Graphics* graphics)
    {
        Rectangle dim = getVerticalBarDimension();

        graphics->pushClipArea(dim);

        int alpha = getBaseColor().a;
        Color trackColor = getBaseColor() - 0x101010;
        trackColor.a = alpha;
        Color shadowColor = getBaseColor() - 0x303030;
        shadowColor.a = alpha;

        graphics->setColor(trackColor);
        graphics->fillRectangle(Rectangle(0, 0, dim.width, dim.height));

        graphics->setColor(shadowColor);
        graphics->drawLine(0, 0, 0, dim.height);

        graphics->popClipArea();
    }

    void ScrollArea::drawBackground(Graphics *graphics)
    {
        graphics->setColor(getBackgroundColor());
        graphics->fillRectangle(getChildrenArea());
    }

    void ScrollArea::drawUpButton(Graphics* graphics)
    {
        Rectangle dim = getUpButtonDimension();
        graphics->pushClipArea(dim);

        Color highlightColor;
        Color shadowColor;
        Color faceColor;
        int offset;
        int alpha = getBaseColor().a;

        if (mUpButtonPressed)
        {
            faceColor = getBaseColor() - 0x303030;
            faceColor.a = alpha;
            highlightColor = faceColor - 0x303030;
            highlightColor.a = alpha;
            shadowColor = getBaseColor();
            shadowColor.a = alpha;

            offset = 1;
        }
        else
        {
            faceColor = getBaseColor();
            faceColor.a = alpha;
            highlightColor = faceColor + 0x303030;
            highlightColor.a = alpha;
            shadowColor = faceColor - 0x303030;
            shadowColor.a = alpha;

            offset = 0;
        }

        graphics->setColor(faceColor);
        graphics->fillRectangle(Rectangle(0, 0, dim.width, dim.height));

        graphics->setColor(highlightColor);
        graphics->drawLine(0, 0, dim.width - 1, 0);
        graphics->drawLine(0, 1, 0, dim.height - 1);

        graphics->setColor(shadowColor);
        graphics->drawLine(dim.width - 1, 0, dim.width - 1, dim.height - 1);
        graphics->drawLine(1, dim.height - 1, dim.width - 1, dim.height - 1);

        graphics->setColor(getForegroundColor());

        int i;
        int w = dim.height / 2;
        int h = w / 2 + 2;
        for (i = 0; i < w / 2; ++i)
        {
            graphics->drawLine(w - i + offset,
                               i + h + offset,
                               w + i + offset,
                               i + h + offset);
        }

        graphics->popClipArea();
    }

    void ScrollArea::drawDownButton(Graphics* graphics)
    {
        Rectangle dim = getDownButtonDimension();
        graphics->pushClipArea(dim);

        Color highlightColor;
        Color shadowColor;
        Color faceColor;
        int offset;
        int alpha = getBaseColor().a;

        if (mDownButtonPressed)
        {
            faceColor = getBaseColor() - 0x303030;
            faceColor.a = alpha;
            highlightColor = faceColor - 0x303030;
            highlightColor.a = alpha;
            shadowColor = getBaseColor();
            shadowColor.a = alpha;

            offset = 1;
        }
        else
        {
            faceColor = getBaseColor();
            faceColor.a = alpha;
            highlightColor = faceColor + 0x303030;
            highlightColor.a = alpha;
            shadowColor = faceColor - 0x303030;
            shadowColor.a = alpha;

            offset = 0;
        }

        graphics->setColor(faceColor);
        graphics->fillRectangle(Rectangle(0, 0, dim.width, dim.height));

        graphics->setColor(highlightColor);
        graphics->drawLine(0, 0, dim.width - 1, 0);
        graphics->drawLine(0, 1, 0, dim.height - 1);

        graphics->setColor(shadowColor);
        graphics->drawLine(dim.width - 1, 0, dim.width - 1, dim.height - 1);
        graphics->drawLine(1, dim.height - 1, dim.width - 1, dim.height - 1);

        graphics->setColor(getForegroundColor());

        int i;
        int w = dim.height / 2;
        int h = w + 1;
        for (i = 0; i < w / 2; ++i)
        {
            graphics->drawLine(w - i + offset,
                               -i + h + offset,
                               w + i + offset,
                               -i + h + offset);
        }

        graphics->popClipArea();
    }

    void ScrollArea::drawLeftButton(Graphics* graphics)
    {
        Rectangle dim = getLeftButtonDimension();
        graphics->pushClipArea(dim);

        Color highlightColor;
        Color shadowColor;
        Color faceColor;
        int offset;
        int alpha = getBaseColor().a;

        if (mLeftButtonPressed)
        {
            faceColor = getBaseColor() - 0x303030;
            faceColor.a = alpha;
            highlightColor = faceColor - 0x303030;
            highlightColor.a = alpha;
            shadowColor = getBaseColor();
            shadowColor.a = alpha;

            offset = 1;
        }
        else
        {
            faceColor = getBaseColor();
            faceColor.a = alpha;
            highlightColor = faceColor + 0x303030;
            highlightColor.a = alpha;
            shadowColor = faceColor - 0x303030;
            shadowColor.a = alpha;

            offset = 0;
        }

        graphics->setColor(faceColor);
        graphics->fillRectangle(Rectangle(0, 0, dim.width, dim.height));

        graphics->setColor(highlightColor);
        graphics->drawLine(0, 0, dim.width - 1, 0);
        graphics->drawLine(0, 1, 0, dim.height - 1);

        graphics->setColor(shadowColor);
        graphics->drawLine(dim.width - 1, 0, dim.width - 1, dim.height - 1);
        graphics->drawLine(1, dim.height - 1, dim.width - 1, dim.height - 1);

        graphics->setColor(getForegroundColor());

        int i;
        int w = dim.width / 2;
        int h = w - 2;
        for (i = 0; i < w / 2; ++i)
        {
            graphics->drawLine(i + h + offset,
                               w - i + offset,
                               i + h + offset,
                               w + i + offset);
        }

        graphics->popClipArea();
    }

    void ScrollArea::drawRightButton(Graphics* graphics)
    {
        Rectangle dim = getRightButtonDimension();
        graphics->pushClipArea(dim);

        Color highlightColor;
        Color shadowColor;
        Color faceColor;
        int offset;
        int alpha = getBaseColor().a;

        if (mRightButtonPressed)
        {
            faceColor = getBaseColor() - 0x303030;
            faceColor.a = alpha;
            highlightColor = faceColor - 0x303030;
            highlightColor.a = alpha;
            shadowColor = getBaseColor();
            shadowColor.a = alpha;

            offset = 1;
        }
        else
        {
            faceColor = getBaseColor();
            faceColor.a = alpha;
            highlightColor = faceColor + 0x303030;
            highlightColor.a = alpha;
            shadowColor = faceColor - 0x303030;
            shadowColor.a = alpha;

            offset = 0;
        }

        graphics->setColor(faceColor);
        graphics->fillRectangle(Rectangle(0, 0, dim.width, dim.height));

        graphics->setColor(highlightColor);
        graphics->drawLine(0, 0, dim.width - 1, 0);
        graphics->drawLine(0, 1, 0, dim.height - 1);

        graphics->setColor(shadowColor);
        graphics->drawLine(dim.width - 1, 0, dim.width - 1, dim.height - 1);
        graphics->drawLine(1, dim.height - 1, dim.width - 1, dim.height - 1);

        graphics->setColor(getForegroundColor());

        int i;
        int w = dim.width / 2;
        int h = w + 1;
        for (i = 0; i < w / 2; ++i)
        {
            graphics->drawLine(-i + h + offset,
                               w - i + offset,
                               -i + h + offset,
                               w + i + offset);
        }

        graphics->popClipArea();
    }

    void ScrollArea::drawVMarker(Graphics* graphics)
    {
        Rectangle dim = getVerticalMarkerDimension();
        graphics->pushClipArea(dim);

        int alpha = getBaseColor().a;
        Color faceColor = getBaseColor();
        faceColor.a = alpha;
        Color highlightColor = faceColor + 0x303030;
        highlightColor.a = alpha;
        Color shadowColor = faceColor - 0x303030;
        shadowColor.a = alpha;

        graphics->setColor(faceColor);
        graphics->fillRectangle(Rectangle(1, 1, dim.width - 1, dim.height - 1));

        graphics->setColor(highlightColor);
        graphics->drawLine(0, 0, dim.width - 1, 0);
        graphics->drawLine(0, 1, 0, dim.height - 1);

        graphics->setColor(shadowColor);
        graphics->drawLine(1, dim.height - 1, dim.width - 1, dim.height - 1);
        graphics->drawLine(dim.width - 1, 0, dim.width - 1, dim.height - 1);

        graphics->popClipArea();
    }

    void ScrollArea::drawHMarker(Graphics* graphics)
    {
        Rectangle dim = getHorizontalMarkerDimension();
        graphics->pushClipArea(dim);

        int alpha = getBaseColor().a;
        Color faceColor = getBaseColor();
        faceColor.a = alpha;
        Color highlightColor = faceColor + 0x303030;
        highlightColor.a = alpha;
        Color shadowColor = faceColor - 0x303030;
        shadowColor.a = alpha;

        graphics->setColor(faceColor);
        graphics->fillRectangle(Rectangle(1, 1, dim.width - 1, dim.height - 1));

        graphics->setColor(highlightColor);
        graphics->drawLine(0, 0, dim.width - 1, 0);
        graphics->drawLine(0, 1, 0, dim.height - 1);

        graphics->setColor(shadowColor);
        graphics->drawLine(1, dim.height - 1, dim.width - 1, dim.height - 1);
        graphics->drawLine(dim.width - 1, 0, dim.width - 1, dim.height - 1);

        graphics->popClipArea();
    }

    void ScrollArea::logic()
    {
        checkPolicies();

        setVerticalScrollAmount(getVerticalScrollAmount());
        setHorizontalScrollAmount(getHorizontalScrollAmount());

        if (getContent() != NULL)
        {
            getContent()->setPosition(-mHScroll + getContent()->getBorderSize(),
                                      -mVScroll + getContent()->getBorderSize());
            getContent()->logic();
        }
    }

    void ScrollArea::checkPolicies()
    {
        int w = getWidth();
        int h = getHeight();

        mHBarVisible = false;
        mVBarVisible = false;


        if (!getContent())
        {
            mHBarVisible = (mHPolicy == SHOW_ALWAYS);
            mVBarVisible = (mVPolicy == SHOW_ALWAYS);
            return;
        }

        if (mHPolicy == SHOW_AUTO &&
            mVPolicy == SHOW_AUTO)
        {
            if (getContent()->getWidth() <= w
                && getContent()->getHeight() <= h)
            {
                mHBarVisible = false;
                mVBarVisible = false;
            }

            if (getContent()->getWidth() > w)
            {
                mHBarVisible = true;
            }

            if ((getContent()->getHeight() > h)
                || (mHBarVisible && getContent()->getHeight() > h - mScrollbarWidth))
            {
                mVBarVisible = true;
            }

            if (mVBarVisible && getContent()->getWidth() > w - mScrollbarWidth)
            {
                mHBarVisible = true;
            }

            return;
        }

        switch (mHPolicy)
        {
          case SHOW_NEVER:
              mHBarVisible = false;
              break;

          case SHOW_ALWAYS:
              mHBarVisible = true;
              break;

          case SHOW_AUTO:
              if (mVPolicy == SHOW_NEVER)
              {
                  mHBarVisible = getContent()->getWidth() > w;
              }
              else // (mVPolicy == SHOW_ALWAYS)
              {
                  mHBarVisible = getContent()->getWidth() > w - mScrollbarWidth;
              }
              break;

          default:
              throw GCN_EXCEPTION("Horizontal scroll policy invalid.");
        }

        switch (mVPolicy)
        {
          case SHOW_NEVER:
              mVBarVisible = false;
              break;

          case SHOW_ALWAYS:
              mVBarVisible = true;
              break;

          case SHOW_AUTO:
              if (mHPolicy == SHOW_NEVER)
              {
                  mVBarVisible = getContent()->getHeight() > h;
              }
              else // (mHPolicy == SHOW_ALWAYS)
              {
                  mVBarVisible = getContent()->getHeight() > h - mScrollbarWidth;
              }
              break;
          default:
              throw GCN_EXCEPTION("Vertical scroll policy invalid.");
        }
    }

    Rectangle ScrollArea::getUpButtonDimension()
    {
        if (!mVBarVisible)
        {
            return Rectangle(0, 0, 0, 0);
        }

        return Rectangle(getWidth() - mScrollbarWidth,
                         0,
                         mScrollbarWidth,
                         mScrollbarWidth);
    }

    Rectangle ScrollArea::getDownButtonDimension()
    {
        if (!mVBarVisible)
        {
            return Rectangle(0, 0, 0, 0);
        }

        if (mVBarVisible && mHBarVisible)
        {
            return Rectangle(getWidth() - mScrollbarWidth,
                             getHeight() - mScrollbarWidth*2,
                             mScrollbarWidth,
                             mScrollbarWidth);
        }

        return Rectangle(getWidth() - mScrollbarWidth,
                         getHeight() - mScrollbarWidth,
                         mScrollbarWidth,
                         mScrollbarWidth);
    }

    Rectangle ScrollArea::getLeftButtonDimension()
    {
        if (!mHBarVisible)
        {
            return Rectangle(0, 0, 0, 0);
        }

        return Rectangle(0,
                         getHeight() - mScrollbarWidth,
                         mScrollbarWidth,
                         mScrollbarWidth);
    }

    Rectangle ScrollArea::getRightButtonDimension()
    {
        if (!mHBarVisible)
        {
            return Rectangle(0, 0, 0, 0);
        }

        if (mVBarVisible && mHBarVisible)
        {
            return Rectangle(getWidth() - mScrollbarWidth*2,
                             getHeight() - mScrollbarWidth,
                             mScrollbarWidth,
                             mScrollbarWidth);
        }

        return Rectangle(getWidth() - mScrollbarWidth,
                         getHeight() - mScrollbarWidth,
                         mScrollbarWidth,
                         mScrollbarWidth);
    }

    Rectangle ScrollArea::getChildrenArea()
    {
        if (mVBarVisible && mHBarVisible)
        {
            return Rectangle(0, 0, getWidth() - mScrollbarWidth,
                             getHeight() - mScrollbarWidth);
        }

        if (mVBarVisible)
        {
            return Rectangle(0, 0, getWidth() - mScrollbarWidth, getHeight());
        }

        if (mHBarVisible)
        {
            return Rectangle(0, 0, getWidth(), getHeight() - mScrollbarWidth);
        }

        return Rectangle(0, 0, getWidth(), getHeight());
    }

    Rectangle ScrollArea::getVerticalBarDimension()
    {
        if (!mVBarVisible)
        {
            return Rectangle(0, 0, 0, 0);
        }

        if (mHBarVisible)
        {
            return Rectangle(getWidth() - mScrollbarWidth,
                             getUpButtonDimension().height,
                             mScrollbarWidth,
                             getHeight()
                             - getUpButtonDimension().height
                             - getDownButtonDimension().height
                             - mScrollbarWidth);
        }

        return Rectangle(getWidth() - mScrollbarWidth,
                         getUpButtonDimension().height,
                         mScrollbarWidth,
                         getHeight()
                         - getUpButtonDimension().height
                         - getDownButtonDimension().height);
    }

    Rectangle ScrollArea::getHorizontalBarDimension()
    {
        if (!mHBarVisible)
        {
            return Rectangle(0, 0, 0, 0);
        }

        if (mVBarVisible)
        {
            return Rectangle(getLeftButtonDimension().width,
                             getHeight() - mScrollbarWidth,
                             getWidth()
                             - getLeftButtonDimension().width
                             - getRightButtonDimension().width
                             - mScrollbarWidth,
                             mScrollbarWidth);
        }

        return Rectangle(getLeftButtonDimension().width,
                         getHeight() - mScrollbarWidth,
                         getWidth()
                         - getLeftButtonDimension().width
                         - getRightButtonDimension().width,
                         mScrollbarWidth);
    }

    Rectangle ScrollArea::getVerticalMarkerDimension()
    {
        if (!mVBarVisible)
        {
            return Rectangle(0, 0, 0, 0);
        }

        int length, pos;
        Rectangle barDim = getVerticalBarDimension();

        if (getContent() && getContent()->getHeight() != 0)
        {
            length = (barDim.height * getChildrenArea().height)
                / getContent()->getHeight();
        }
        else
        {
            length = barDim.height;
        }

        if (length < mScrollbarWidth)
        {
            length = mScrollbarWidth;
        }

        if (length > barDim.height)
        {
            length = barDim.height;
        }

        if (getVerticalMaxScroll() != 0)
        {
            pos = ((barDim.height - length) * getVerticalScrollAmount())
                / getVerticalMaxScroll();
        }
        else
        {
            pos = 0;
        }

        return Rectangle(barDim.x, barDim.y + pos, mScrollbarWidth, length);
    }

    Rectangle ScrollArea::getHorizontalMarkerDimension()
    {
        if (!mHBarVisible)
        {
            return Rectangle(0, 0, 0, 0);
        }

        int length, pos;
        Rectangle barDim = getHorizontalBarDimension();

        if (getContent() && getContent()->getWidth() != 0)
        {
            length = (barDim.width * getChildrenArea().width)
                / getContent()->getWidth();
        }
        else
        {
            length = barDim.width;
        }

        if (length < mScrollbarWidth)
        {
            length = mScrollbarWidth;
        }

        if (length > barDim.width)
        {
            length = barDim.width;
        }

        if (getHorizontalMaxScroll() != 0)
        {
            pos = ((barDim.width - length) * getHorizontalScrollAmount())
                / getHorizontalMaxScroll();
        }
        else
        {
            pos = 0;
        }

        return Rectangle(barDim.x + pos, barDim.y, length, mScrollbarWidth);
    }

    void ScrollArea::showWidgetPart(Widget* widget, Rectangle area)
    {
        if (widget != getContent())
        {
            throw GCN_EXCEPTION("Widget not content widget");
        }

        BasicContainer::showWidgetPart(widget, area);

        setHorizontalScrollAmount(getContent()->getBorderSize() - getContent()->getX());
        setVerticalScrollAmount(getContent()->getBorderSize() - getContent()->getY());
    }

    Widget *ScrollArea::getWidgetAt(int x, int y)
    {
        if (getChildrenArea().isPointInRect(x, y))
        {
            return getContent();
        }

        return NULL;
    }

    void ScrollArea::mouseWheelMovedUp(MouseEvent& mouseEvent)
    {
        if (mouseEvent.isConsumed())
        {
            return;
        }
        
        setVerticalScrollAmount(getVerticalScrollAmount() - getChildrenArea().height / 8);

        mouseEvent.consume();
    }

    void ScrollArea::mouseWheelMovedDown(MouseEvent& mouseEvent)
    {
        if (mouseEvent.isConsumed())
        {
            return;
        }

        setVerticalScrollAmount(getVerticalScrollAmount() + getChildrenArea().height / 8);

        mouseEvent.consume();
    }

    void ScrollArea::setWidth(int width)
    {
        Widget::setWidth(width);
        checkPolicies();
    }

    void ScrollArea::setHeight(int height)
    {
        Widget::setHeight(height);
        checkPolicies();
    }

    void ScrollArea::setDimension(const Rectangle& dimension)
    {
        Widget::setDimension(dimension);
        checkPolicies();
    }

    void ScrollArea::setLeftButtonScrollAmount(int amount)
    {
        mLeftButtonScrollAmount = amount;
    }

    void ScrollArea::setRightButtonScrollAmount(int amount)
    {
        mRightButtonScrollAmount = amount;
    }

    void ScrollArea::setUpButtonScrollAmount(int amount)
    {
        mUpButtonScrollAmount = amount;
    }

    void ScrollArea::setDownButtonScrollAmount(int amount)
    {
        mDownButtonScrollAmount = amount;
    }

    int ScrollArea::getLeftButtonScrollAmount() const
    {
        return mLeftButtonScrollAmount;
    }

    int ScrollArea::getRightButtonScrollAmount() const
    {
        return mRightButtonScrollAmount;
    }

    int ScrollArea::getUpButtonScrollAmount() const
    {
        return mUpButtonScrollAmount;
    }

    int ScrollArea::getDownButtonScrollAmount() const
    {
        return mDownButtonScrollAmount;
    }
}

/*
 * Wow! This is a looooong source file.
 */
