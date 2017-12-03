/*      _______   __   __   __   ______   __   __   _______   __   __
 *     / _____/\ / /\ / /\ / /\ / ____/\ / /\ / /\ / ___  /\ /  |\/ /\
 *    / /\____\// / // / // / // /\___\// /_// / // /\_/ / // , |/ / /
 *   / / /__   / / // / // / // / /    / ___  / // ___  / // /| ' / /
 *  / /_// /\ / /_// / // / // /_/_   / / // / // /\_/ / // / |  / /
 * /______/ //______/ //_/ //_____/\ /_/ //_/ //_/ //_/ //_/ /|_/ /
 * \______\/ \______\/ \_\/ \_____\/ \_\/ \_\/ \_\/ \_\/ \_\/ \_\/
 *
 * Copyright (c) 2004, 2005, 2006, 2007 Olof Naessén and Per Larsson
 * Copyright (c) 2017 Gwilherm Baudic
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

#include "guisan/widgets/progressbar.hpp"

#include "guisan/exception.hpp"
#include "guisan/font.hpp"
#include "guisan/graphics.hpp"

namespace gcn
{
    ProgressBar::ProgressBar() : Label()
    {
        mAlignment = Graphics::CENTER;
        mStart = 0;
        mValue = 0;
        mEnd = 100;
        
        setHeight(getFont()->getHeight());
        setBorderSize(1);
    }
    
    ProgressBar::ProgressBar(const unsigned int start, 
            const unsigned int end, const unsigned int value) : Label()
    {
        mAlignment = Graphics::CENTER;
        
        if(start > end)
        {
            mStart = end;
            mEnd = start;
        }
        else
        {
            mStart = start;
            mEnd = end;
        }
        
        if((value >= start && value <= end) || (start == 0 && end == 0))
        {
            mValue = value;
        }
        else
        {
            mValue = start;
        }
        
        setHeight(getFont()->getHeight());
        setBorderSize(1);
    }

    ProgressBar::ProgressBar(const std::string& caption) : Label(caption)
    {
        mCaption = caption;
        mAlignment = Graphics::CENTER;

        setHeight(getFont()->getHeight());
        setBorderSize(1);
    }

    const std::string &ProgressBar::getCaption() const
    {
        return mCaption;
    }

    void ProgressBar::setCaption(const std::string& caption)
    {
        mCaption = caption;
    }

    void ProgressBar::setAlignment(unsigned int alignment)
    {
        mAlignment = alignment;
    }

    unsigned int ProgressBar::getAlignment() const
    {
        return mAlignment;
    }

    void ProgressBar::draw(Graphics* graphics)
    {
        graphics->setColor(getBackgroundColor());
        graphics->fillRectangle(Rectangle(0, 0, getWidth(), getHeight()));
        
        int textX;
        int textY = getHeight() / 2 - getFont()->getHeight() / 2;
        
        graphics->setColor(getSelectionColor());
        int progressWidth;
        if(mStart == 0 && mEnd == 0)
        {
            // Infinite scrollbar
            progressWidth = getWidth() / 5;
            int barX = getWidth() * mValue / 100;
            
            if(barX + progressWidth > getWidth())
            {
                graphics->fillRectangle(Rectangle(barX, 0, getWidth() - barX, getHeight()));
                graphics->fillRectangle(Rectangle(0, 0, progressWidth - (getWidth() - barX), getHeight()));
            }
            else
            {
                graphics->fillRectangle(Rectangle(barX,0,progressWidth,getHeight()));
            }
        }
        else
        {
            // Standard scrollbar
            progressWidth = getWidth() * mValue / (mEnd - mStart);
            graphics->fillRectangle(Rectangle(0,0,progressWidth,getHeight()));
        }

        switch (getAlignment())
        {
          case Graphics::LEFT:
              textX = 0;
              break;
          case Graphics::CENTER:
              textX = getWidth() / 2;
              break;
          case Graphics::RIGHT:
              textX = getWidth();
              break;
          default:
              throw GCN_EXCEPTION("Unknown alignment.");
        }

        graphics->setFont(getFont());
        graphics->setColor(getForegroundColor());
        graphics->drawText(getCaption(), textX, textY, getAlignment());
    }

    void ProgressBar::drawBorder(Graphics* graphics)
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

    void ProgressBar::adjustSize()
    {
        setHeight(getFont()->getHeight());
    }
    
    void ProgressBar::setStart(const unsigned int start)
    {
        if(start <= mEnd)
        {
            mStart = start;
        }
    }
        
    unsigned int ProgressBar::getStart() const
    {
        return mStart;
    }
        
    void ProgressBar::setEnd(const unsigned int end)
    {
        if(end >= mStart)
        {
            mEnd = end;
        }
    }
        
    unsigned int ProgressBar::getEnd() const
    {
        return mEnd;
    }
        
    void ProgressBar::setValue(const unsigned int value)
    {
        if(value >= mStart && value <= mEnd)
        {
            mValue = value;
        }
        else
        {
            if(mStart == 0 && mEnd == 0)
            {
                mValue = value % 100;
            }
        }
    }
    
    unsigned int ProgressBar::getValue() const
    {
        return mValue;
    }
}
