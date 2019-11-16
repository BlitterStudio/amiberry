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

#include "guisan/widgets/imagetextbutton.hpp"

#include "guisan/exception.hpp"
#include "guisan/font.hpp"
#include "guisan/graphics.hpp"
#include "guisan/image.hpp"

namespace gcn
{
    ImageTextButton::ImageTextButton(const std::string& filename, std::string& caption) : ImageButton(filename)
    {
        setCaption(caption);
        setWidth(mImage->getWidth() + mImage->getWidth() / 2);
        setHeight(mImage->getHeight() + mImage->getHeight() / 2);
        mAlignment = ImageTextButton::BOTTOM;
    }

    ImageTextButton::ImageTextButton(Image* image, std::string& caption) : ImageButton(image)
    {
        setCaption(caption);
        setWidth(mImage->getWidth() + mImage->getWidth() / 2);
        setHeight(mImage->getHeight() + mImage->getHeight() / 2);
        mAlignment = ImageTextButton::BOTTOM;
    }

    ImageTextButton::~ImageTextButton()
    {
	    if (mInternalImage)
            delete mImage;
    }
	
	void ImageTextButton::adjustSize()
	{
		switch(getAlignment())
		{
			case LEFT: //fallthrough
			case RIGHT:
			  setWidth(mImage->getWidth() + getFont()->getWidth(mCaption) + 2*mSpacing);
			  setHeight(mImage->getHeight() + 2*mSpacing);
			  break;
			case TOP: //fallthrough
			case BOTTOM:
			  if(mImage->getWidth() > getFont()->getWidth(mCaption))
			  {
				  setWidth(mImage->getWidth() + 2*mSpacing);
			  }
			  else
			  {
				  setWidth(getFont()->getWidth(mCaption) + 2*mSpacing);
			  }
			  setHeight(mImage->getHeight() + getFont()->getHeight() + 2*mSpacing);
			  break;
			default:
              throw GCN_EXCEPTION("Unknown alignment.");
		}
	}

	void ImageTextButton::setImage(Image* image)
	{
        if (mInternalImage)
            delete mImage;
		mImage = image;
        mInternalImage = false;
	}

	Image* ImageTextButton::getImage()
	{
		return mImage;
	}

    void ImageTextButton::draw(Graphics* graphics)
    {
        gcn::Color faceColor = getBaseColor();
        gcn::Color highlightColor, shadowColor;
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

        graphics->setColor(getForegroundColor());

        int imageX, imageY;
        int textX, textY;
        
        switch(getAlignment())
        {
            case LEFT: 
              imageX = mSpacing + getFont()->getWidth(mCaption);
              textX = mSpacing;
              imageY = mSpacing;
              textY = getHeight() / 2 - getFont()->getHeight() / 2;
              break;
			case RIGHT:
			  imageX = mSpacing;
              textX = mSpacing + mImage->getWidth();
              imageY = mSpacing;
              textY = getHeight() / 2 - getFont()->getHeight() / 2;
			  break;
			case TOP:
			  imageY = mSpacing + getFont()->getHeight();
			  textY = mSpacing;
			  imageX = getWidth() / 2 - mImage->getWidth() / 2;
			  textX = getWidth() / 2 - getFont()->getWidth(mCaption) / 2; 
			  break;
			case BOTTOM:
			  imageY = mSpacing;
			  textY = mSpacing + mImage->getHeight();
			  imageX = getWidth() / 2 - mImage->getWidth() / 2;
			  textX = getWidth() / 2 - getFont()->getWidth(mCaption) / 2; 
			  break;
			default:
              throw GCN_EXCEPTION("Unknown alignment.");
        }

        if (isPressed())
        {
            graphics->drawImage(mImage, imageX + 1, imageY + 1);
            graphics->drawText(mCaption, textX + 1, textY + 1, Graphics::LEFT);
        }
        else
        {
            graphics->drawImage(mImage, imageX, imageY);
            graphics->drawText(mCaption, textX, textY, Graphics::LEFT);
           
            if (isFocused())
            {
                graphics->drawRectangle(Rectangle(2, 
                                                  2, 
                                                  getWidth() - 4,
                                                  getHeight() - 4));
            }
        }
    }
    
    void ImageTextButton::setAlignment(unsigned int alignment)
    {
        mAlignment = alignment;
    }

    unsigned int ImageTextButton::getAlignment() const
    {
        return mAlignment;
    }
}
