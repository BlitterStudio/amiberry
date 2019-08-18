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

#include "guisan/widgets/messagebox.hpp"

#include "guisan/exception.hpp"
#include "guisan/font.hpp"
#include "guisan/graphics.hpp"
#include "guisan/mouseinput.hpp"

namespace gcn
{

    MessageBox::MessageBox(const std::string& caption, const std::string& message)
            :Window(caption),mMessage(message),mClickedButton(-1)
    {
        setCaption(caption);
        addMouseListener(this);
        setMovable(false);
        
        mLabel = new Label(message);
        mLabel->setAlignment(Graphics::LEFT);
        mLabel->adjustSize();
        
        mNbButtons = 1;
        mButtons = new Button*[1];
        mButtons[0] = new Button("OK");
        mButtons[0]->setAlignment(Graphics::CENTER);
        mButtons[0]->addMouseListener(this);
        
        setHeight((int)getTitleBarHeight() + mLabel->getHeight() + 4*mPadding + mButtons[0]->getHeight());
		setWidth(mLabel->getWidth() + 4*mPadding);
        if(mButtons[0]->getWidth() + 4*mPadding > getWidth()) 
        {
            setWidth(mButtons[0]->getWidth() + 4*mPadding);
        }
        
		this->add(mLabel, (getWidth() - mLabel->getWidth())/2 - mPadding, mPadding);
        this->add(mButtons[0], (getWidth() - mButtons[0]->getWidth())/2, getHeight() - (int)getTitleBarHeight() - mPadding - mButtons[0]->getHeight());
        
        try
        {
        	requestModalFocus();
        } 
        catch (Exception e) 
        {
        	// Not having modal focus is not critical
        }
    }
    
    MessageBox::MessageBox(const std::string& caption, const std::string& message,
            const std::string *buttons, int size)
            :Window(caption),mMessage(message),mClickedButton(-1)
    {
        setCaption(caption);
        addMouseListener(this);
        setMovable(false);
        
        mLabel = new Label(message);
        mLabel->setAlignment(Graphics::LEFT);
        mLabel->adjustSize();
		setWidth(mLabel->getWidth() + 4*mPadding);
        
        //Create buttons and label
        if(size > 0) 
        {
            mNbButtons = size;
            mButtons = new Button*[size];
            int maxBtnWidth = 0;
            
            for(int i = 0 ; i < size ; i++)
            {
                mButtons[i] = new Button(*(buttons+i));
                mButtons[i]->setAlignment(Graphics::CENTER);
                mButtons[i]->addMouseListener(this);
                maxBtnWidth = maxBtnWidth > mButtons[i]->getWidth() ? maxBtnWidth : mButtons[i]->getWidth();
            }
            //Find the widest button, apply same width to all
            for(int i = 0 ; i < size ; i++)
            {
                mButtons[i]->setWidth(maxBtnWidth);
            }
            
            //Make sure everything fits into the window
            int padding = mPadding;
            if(mButtons[0]->getWidth()*size + 4*mPadding + mPadding*(size-1) > getWidth()) 
            {
                setWidth(mButtons[0]->getWidth()*size + 4*mPadding + mPadding*(size-1));
            } 
            else 
            {
                padding += (getWidth() - (mButtons[0]->getWidth()*size + 4*mPadding + mPadding*(size-1)))/2;
            }
			add(mLabel, (getWidth() - mLabel->getWidth())/2 - mPadding, mPadding);
            
			setHeight((int)getTitleBarHeight() + mLabel->getHeight() + 4*mPadding + mButtons[0]->getHeight());
            for(int i = 0 ; i < size ; i++)
            {
                add(mButtons[i], padding + (maxBtnWidth + mPadding)*i, getHeight() - (int)getTitleBarHeight() - mPadding - mButtons[0]->getHeight());
            }			
        }
        
        try
        {
        	requestModalFocus();
        } 
        catch (Exception e) 
        {
        	// Not having modal focus is not critical
        }
    }

    MessageBox::~MessageBox()
    {
    	releaseModalFocus();
    	
        delete mLabel;
        for(int i = 0 ; i < mNbButtons ; i++)
        {
            delete mButtons[i];
        }
        delete mButtons;
    }

    void MessageBox::setPadding(unsigned int padding)
    {
        mPadding = padding;
    }

    unsigned int MessageBox::getPadding() const
    {
        return mPadding;
    }

    void MessageBox::setTitleBarHeight(unsigned int height)
    {
        mTitleBarHeight = height;
    }

    unsigned int MessageBox::getTitleBarHeight()
    {
        return mTitleBarHeight;
    }

    void MessageBox::setCaption(const std::string& caption)
    {
        mCaption = caption;
    }

    const std::string& MessageBox::getCaption() const
    {
        return mCaption;
    }

    void MessageBox::setAlignment(unsigned int alignment)
    {
        mAlignment = alignment;
    }

    unsigned int MessageBox::getAlignment() const
    {
        return mAlignment;
    }
    
    void MessageBox::setButtonAlignment(unsigned int alignment)
    {
        mButtonAlignment = alignment;
        
        int leftPadding = mPadding;
        if(mNbButtons > 0)
        {
            switch (alignment)
            {
              case Graphics::LEFT:
                  // Nothing to do
                  break;
              case Graphics::CENTER:
                  leftPadding += (getWidth() - (mButtons[0]->getWidth()*mNbButtons + 2*mPadding + mPadding*(mNbButtons-1)))/2;
                  break;
              case Graphics::RIGHT:
                  leftPadding += (getWidth() - (mButtons[0]->getWidth()*mNbButtons + 2*mPadding + mPadding*(mNbButtons-1)));
                  break;
              default:
                  throw GCN_EXCEPTION("Unknown alignment.");
            }
            for(int i = 0 ; i < mNbButtons ; i++)
            {
                mButtons[i]->setX(leftPadding + (mButtons[0]->getWidth() + mPadding)*i);
            }
        }
    }
    
    unsigned int MessageBox::getButtonAlignment() const
    {
        return mButtonAlignment;
    }

    void MessageBox::draw(Graphics* graphics)
    {
        Color faceColor = getBaseColor();
        Color highlightColor, shadowColor;
        int alpha = getBaseColor().a;
        //int width = getWidth() + getBorderSize() * 2 - 1;
        //int height = getHeight() + getBorderSize() * 2 - 1;
        highlightColor = faceColor + 0x303030;
        highlightColor.a = alpha;
        shadowColor = faceColor - 0x303030;
        shadowColor.a = alpha;

        Rectangle d = getChildrenArea();

        // Fill the background around the content
        graphics->setColor(faceColor);
        // Fill top
        graphics->fillRectangle(Rectangle(0,0,getWidth(),d.y - 1));
        // Fill left
        graphics->fillRectangle(Rectangle(0,d.y - 1, d.x - 1, getHeight() - d.y + 1));
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
        int textY;

        textY = ((int)getTitleBarHeight() - getFont()->getHeight()) / 2;

        switch (getAlignment())
        {
          case Graphics::LEFT:
              textX = 4;
              break;
          case Graphics::CENTER:
              textX = getWidth() / 2;
              break;
          case Graphics::RIGHT:
              textX = getWidth() - 4;
              break;
          default:
              throw GCN_EXCEPTION("Unknown alignment.");
        }

        graphics->setColor(getForegroundColor());
        graphics->setFont(getFont());
        graphics->pushClipArea(Rectangle(0, 0, getWidth(), getTitleBarHeight() - 1));
        graphics->drawText(getCaption(), textX, textY, getAlignment());
        graphics->popClipArea();
    }

    void MessageBox::drawBorder(Graphics* graphics)
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
            graphics->setColor(highlightColor);
            graphics->drawLine(i,i, width - i, i);
            graphics->drawLine(i,i + 1, i, height - i - 1);
            graphics->setColor(shadowColor);
            graphics->drawLine(width - i,i + 1, width - i, height - i);
            graphics->drawLine(i,height - i, width - i - 1, height - i);
        }
    }

    void MessageBox::mousePressed(MouseEvent& mouseEvent)
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
        
        mIsMoving = mouseEvent.getY() <= (int)mTitleBarHeight;
    }

    void MessageBox::mouseReleased(MouseEvent& mouseEvent)
    {
        if (mouseEvent.getSource() != this)
        {
            for(int i = 0 ; i < mNbButtons ; i++)
            {
                if(mouseEvent.getSource() == mButtons[i])
                {
                    mClickedButton = i;
                    generateAction();
                    break;
                }
            }
        }
        else
        {
            mIsMoving = false;
        }
    }

    void MessageBox::mouseDragged(MouseEvent& mouseEvent)
    {
        if (mouseEvent.isConsumed() || mouseEvent.getSource() != this)
        {
            return;
        }
        
        if (isMovable() && mIsMoving)
        {
            setPosition(mouseEvent.getX() - mDragOffsetX + getX(),
                        mouseEvent.getY() - mDragOffsetY + getY());
        }

        mouseEvent.consume();
    }

    Rectangle MessageBox::getChildrenArea()
    {
        return Rectangle(getPadding(),
                         getTitleBarHeight(),
                         getWidth() - getPadding() * 2,
                         getHeight() - getPadding() - getTitleBarHeight());
    }

    bool MessageBox::isMovable() const
    {
        return mMovable;
    }

    void MessageBox::setOpaque(bool opaque)
    {
        mOpaque = opaque;
    }

    bool MessageBox::isOpaque()
    {
        return mOpaque;
    }

    void MessageBox::resizeToContent()
    {
        WidgetListIterator it;

        int w = 0, h = 0;
        for (it = mWidgets.begin(); it != mWidgets.end(); it++)
        {
            if ((*it)->getX() + (*it)->getWidth() > w)
            {
                w = (*it)->getX() + (*it)->getWidth();
            }

            if ((*it)->getY() + (*it)->getHeight() > h)
            {
                h = (*it)->getY() + (*it)->getHeight();
            }
        }

        setSize(w + 2* getPadding(), h + getPadding() + getTitleBarHeight());
    }
    
    int MessageBox::getClickedButton() const
    {
        return mClickedButton;
    }
	
	void MessageBox::addToContainer(Container* container)
	{
		int x = container->getWidth() - getWidth();
		int y = container->getHeight() - getHeight();
		container->add(this, x/2, y/2);
	}
}
