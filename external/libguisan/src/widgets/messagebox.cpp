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

#include <vector>

namespace gcn
{

    MessageBox::MessageBox(const std::string& caption, const std::string& message) :
        MessageBox(caption, message, {"OK"})
    {}

    MessageBox::MessageBox(const std::string& caption,
                           const std::string& message,
                           const std::vector<std::string>& button_captions) :
        MessageBox(caption, message, button_captions.data(), button_captions.size())
    {}

    MessageBox::MessageBox(const std::string& caption,
                           const std::string& message,
                           const std::string* button_captions,
                           int size) :
        Window(caption)
    {
        setMovable(false);

        mLabel = std::make_unique<Label>(message);
        mLabel->setAlignment(Graphics::Left);
        mLabel->adjustSize();
        setWidth(mLabel->getWidth() + 4 * mPadding);

        //Create buttons
        int maxButtonWidth = 0;
        int maxButtonHeight = 0;
        for (int i = 0; i < size; i++)
        {
            mButtons.emplace_back(std::make_unique<Button>(button_captions[i]));
            auto& button = mButtons.back();
            button->setAlignment(Graphics::Center);
            button->addActionListener(this);
            maxButtonWidth = std::max(maxButtonWidth, button->getWidth());
            maxButtonHeight = std::max(maxButtonHeight, button->getHeight());
        }
        // Find the widest button, apply same width to all
        for (auto& button : mButtons)
        {
            button->setWidth(maxButtonWidth);
        }
        //Make sure everything fits into the window
        int padding = mPadding;
        if (maxButtonWidth * size + 4 * mPadding + mPadding * (size - 1) > getWidth())
        {
            setWidth(maxButtonWidth * size + 4 * mPadding + mPadding * (size - 1));
        }
        else
        {
            padding +=
                (getWidth() - (maxButtonWidth * size + 4 * mPadding + mPadding * (size - 1))) / 2;
        }
        setHeight((int) getTitleBarHeight() + mLabel->getHeight() + 4 * mPadding + maxButtonHeight);
        add(mLabel.get(), (getWidth() - mLabel->getWidth()) / 2 - mPadding, mPadding);

        for (std::size_t i = 0; i < mButtons.size(); ++i)
        {
            add(mButtons[i].get(),
                padding + (maxButtonWidth + mPadding) * i,
                getHeight() - (int) getTitleBarHeight() - mPadding - mButtons[0]->getHeight());
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
    }

    void MessageBox::setButtonAlignment(Graphics::Alignment alignment)
    {
        mButtonAlignment = alignment;

        int leftPadding = mPadding;
        if (!mButtons.empty())
        {
            switch (alignment)
            {
                case Graphics::Left:
                    // Nothing to do
                    break;
                case Graphics::Center:
                    leftPadding += (getWidth()
                                    - (mButtons[0]->getWidth() * mButtons.size() + 2 * mPadding
                                       + mPadding * (mButtons.size() - 1)))
                                 / 2;
                    break;
                case Graphics::Right:
                    leftPadding += (getWidth()
                                    - (mButtons[0]->getWidth() * mButtons.size() + 2 * mPadding
                                       + mPadding * (mButtons.size() - 1)));
                    break;
                default: throw GCN_EXCEPTION("Unknown alignment.");
            }
            for (std::size_t i = 0; i != mButtons.size(); ++i)
            {
                mButtons[i]->setX(leftPadding + (mButtons[0]->getWidth() + mPadding) * i);
            }
        }
    }

    Graphics::Alignment MessageBox::getButtonAlignment() const
    {
        return mButtonAlignment;
    }

    void MessageBox::action(const ActionEvent& actionEvent)
    {
        for (std::size_t i = 0; i != mButtons.size(); ++i)
        {
            if (actionEvent.getSource() == mButtons[i].get())
            {
                mClickedButton = static_cast<int>(i);
                distributeActionEvent();
                break;
            }
        }
    }

    int MessageBox::getClickedButton() const
    {
        return mClickedButton;
    }

    void MessageBox::addToContainer(Container* container)
    {
        int x = container->getWidth() - getWidth();
        int y = container->getHeight() - getHeight();
        container->add(this, x / 2, y / 2);
    }
} // namespace gcn
